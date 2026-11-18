// Minion https://github.com/minion/minion
// SPDX-License-Identifier: MPL-2.0
#include "minion.h"
#include "BuildVariables.h"
#include "annotations.h"
#include <set>

/* @help switches;-annotations Description
Write execution annotations to the specified file, in the compact binary form
described in lean/ANNOTATIONS.md. This prototype records domains, branching
and nested reasoning contexts. Propagation reasons are not yet proofs.
minion-replay reads the file and can rewrite it as JSON lines.
Unused contexts and duplicate branch/probe mutations are omitted. Add
-annotations-all-contexts to retain them for diagnostics.
*/

namespace {
std::string quote(const std::string& s) { return picojson::value(s).serialize(); }

std::string integerList(const vector<DomainInt>& values) {
  std::ostringstream out;
  out << '[';
  for(size_t i = 0; i < values.size(); ++i) out << (i ? "," : "") << values[i];
  out << ']';
  return out.str();
}

std::string term(Var v, const std::map<std::pair<int, int>, size_t>& ids) {
  if(v.type() == VAR_CONSTANT)
    return "{\"kind\":\"constant\",\"value\":" + tostring(v.pos()) + "}";
  bool negated = v.type() == VAR_NOTBOOL;
  auto it = ids.find({negated ? VAR_BOOL : v.type(), v.pos()});
  if(it == ids.end()) throw minion_user_error("annotation refers to an undeclared variable");
  return "{\"kind\":" + quote(negated ? "not_bool" : "var") +
         ",\"index\":" + tostring(it->second) + "}";
}

std::string sourceConstraint(ConstraintBlob& b, const std::string& path,
    std::map<const ConstraintBlob*, std::string>& paths,
    const std::map<std::pair<int, int>, size_t>& ids) {
  paths[&b] = path;
  std::vector<std::string> args;
  size_t vi = 0, ci = 0;
  for(int i = 0; i < b.constraint->numberOfParams; ++i) {
    auto kind = b.constraint->read_types[i];
    if(kind == read_var || kind == read_list || kind == read_2_vars) {
      auto& vs = b.vars.at(vi++);
      if(kind == read_2_vars) {
        for(auto v : vs) args.push_back("[" + term(v, ids) + "]");
      } else {
        std::string arg = "[";
        for(size_t j = 0; j < vs.size(); ++j) arg += (j ? "," : "") + term(vs[j], ids);
        args.push_back(arg + "]");
      }
    } else if(kind == read_constant || kind == read_constant_list) {
      std::string arg = "[";
      auto& values = b.constants.at(ci++);
      for(size_t j = 0; j < values.size(); ++j)
        arg += (j ? "," : "") + term(Var(VAR_CONSTANT, values[j]), ids);
      args.push_back(arg + "]");
    }
  }
  std::ostringstream out;
  out << "{\"name\":" << quote(b.constraint->name) << ",\"source_path\":" << quote(path)
      << ",\"args\":[";
  for(size_t i = 0; i < args.size(); ++i) out << (i ? "," : "") << args[i];
  out << "],\"children\":[";
  for(size_t i = 0; i < b.internal_constraints.size(); ++i)
    out << (i ? "," : "") << sourceConstraint(b.internal_constraints[i],
        path + "/" + tostring(i), paths, ids);
  out << "],\"tuples\":";
  if(!b.tuples) out << "null";
  else {
    out << '[';
    for(int i = 0; i < b.tuples->size(); ++i)
      out << (i ? "," : "") << integerList(b.tuples->getVector(i));
    out << ']';
  }
  out << ",\"short_tuples\":";
  if(!b.shortTuples) out << "null";
  else {
    out << '[';
    const auto& tuples = *b.shortTuples->tuplePtr();
    for(size_t i = 0; i < tuples.size(); ++i) {
      out << (i ? "," : "") << '[';
      for(size_t j = 0; j < tuples[i].size(); ++j)
        out << (j ? "," : "") << '[' << tuples[i][j].first << ',' << tuples[i][j].second << ']';
      out << ']';
    }
    out << ']';
  }
  out << '}';
  return out.str();
}
}

// Record tags. Every record except a string definition is one event, numbered
// consecutively from 1; the reader reconstructs sequence numbers and the ids
// which were derived from them. Unsigned integers are LEB128, signed integers
// zigzag. Rare events are carried as their JSON fields.
namespace {
enum AnnotationTag : unsigned char {
  TagJson = 0, TagString = 1, TagOpen = 2, TagOpenInfo = 3, TagClose = 4,
  TagPrune = 5,        // + operation (remove, assign, lower_bound, upper_bound)
  TagPruneWitness = 9, // + operation
  TagPush = 13, TagPop = 14, TagBranchLeft = 15, TagBranchRight = 16, TagBranchClose = 17,
  TagConflict = 18, TagConflictWitness = 19, TagBacktrack = 20, TagSolution = 21,
  TagBranchSkip = 22, TagWitness = 23
};
}

AnnotationTrace::AnnotationTrace(const std::string& path, bool includeAllContexts)
    : output(path, std::ios::out | std::ios::trunc | std::ios::binary), allContexts(includeAllContexts) {
  if(!output) throw minion_user_error("Cannot open annotation file: " + path);
  buffer = "MINIONTR";
  put(1);
}

AnnotationTrace::~AnnotationTrace() {
  output.write(buffer.data(), buffer.size());
  output.flush();
}

void AnnotationTrace::flushBuffer() {
  output.write(buffer.data(), buffer.size());
  buffer.clear();
  if(!output) throw minion_user_error("Failed writing annotation file");
}

void AnnotationTrace::putUnsigned(std::uint64_t value) {
  while(value >= 128) {
    put((unsigned char)(value & 127) | 128);
    value >>= 7;
  }
  put((unsigned char)value);
}

void AnnotationTrace::putSigned(long long value) {
  putUnsigned(value >= 0 ? (std::uint64_t)value << 1 : (((std::uint64_t)(-(value + 1))) << 1) | 1);
}

std::uint64_t AnnotationTrace::stringId(const std::string& text) {
  auto found = stringIds.find(text);
  if(found != stringIds.end()) return found->second;
  auto id = stringIds.size();
  stringIds[text] = id;
  put(TagString);
  putUnsigned(text.size());
  buffer += text;
  return id;
}

void AnnotationTrace::record(unsigned char tag) {
  if(buffer.size() >= (1u << 16)) flushBuffer();
  ++sequence;
  put(tag);
}

void AnnotationTrace::emit(const std::string& event, const std::string& fields) {
  std::string body = "\"event\":" + quote(event) + fields;
  record(TagJson);
  putUnsigned(body.size());
  buffer += body;
}

void AnnotationTrace::begin(CSPInstance& instance) {
  const auto vars = instance.vars.getAllVars();
  for(size_t i = 0; i < vars.size(); ++i) variableIds[{vars[i].type(), vars[i].pos()}] = i;
  std::ostringstream out;
  out << ",\"format\":\"minion-annotations-v1\",\"propagation_checked\":false,\"model\":{"
      << "\"format\":\"minion-solutions-v1\",\"variables\":[";
  for(size_t i = 0; i < vars.size(); ++i) {
    auto domain = instance.vars.getDomain(vars[i]);
    out << (i ? "," : "") << "{\"name\":" << quote(instance.vars.getName(vars[i]));
    // Retain large interval domains compactly. Replay may impose its own budget.
    out << (domain.first == Bound_Yes ? ",\"interval\":" : ",\"domain\":")
        << integerList(domain.second) << '}';
  }
  out << "],\"constraints\":[";
  size_t i = 0;
  for(auto& c : instance.constraints) {
    out << (i ? "," : "") << sourceConstraint(c, tostring(i), sourcePaths, variableIds);
    ++i;
  }
  out << "],\"solutions\":[]}";
  // Variables in an AUX block, including those in no VARORDER: solutions
  // differing only in these are reported once.
  out << ",\"auxiliary\":[";
  bool firstAuxiliary = true;
  for(const auto& order : instance.searchOrder) {
    if(!order.findOneAssignment) continue;
    for(const auto& v : order.varOrder) {
      auto found = variableIds.find({v.type() == VAR_NOTBOOL ? VAR_BOOL : v.type(), v.pos()});
      if(found == variableIds.end()) throw minion_user_error("annotation refers to an undeclared variable");
      out << (firstAuxiliary ? "" : ",") << found->second;
      firstAuxiliary = false;
    }
  }
  out << ']';
  if(instance.is_optimisation_problem) {
    if(instance.optimiseVariables.size() != 1)
      throw minion_user_error("annotations support a single optimisation variable");
    out << ",\"optimise\":{\"minimising\":" << (instance.optimiseMinimising ? "true" : "false")
        << ",\"variable\":" << term(instance.optimiseVariables[0], variableIds) << '}';
  }
  emit("model", out.str());
  started = true;
}

std::uint64_t AnnotationTrace::constraint(AbstractConstraint* c, const ConstraintBlob* source) {
  if(!active() || !c) return 0;
  if(c->annotationId) return c->annotationId;
  // Builders can delegate to a specialised builder using a temporary blob
  // (e.g. reify(eq(...),b) -> __reify_eq). Wait for the enclosing original
  // blob to declare that object, rather than permanently losing its source.
  // Genuinely generated objects are still declared with source == nullptr.
  const auto it = sourcePaths.find(source);
  if(source && it == sourcePaths.end()) return 0;
  auto id = c->annotationId = ++nextConstraint;
  if(it != sourcePaths.end() && it->second.find('/') == std::string::npos)
    rootSources[id] = source;
  std::ostringstream declaration;
  declaration << ",\"id\":" << id << ",\"name\":" << quote(c->constraintName())
      << ",\"source_path\":" << (it == sourcePaths.end() ? "null" : quote(it->second))
      << ",\"variables\":[";
  const auto& vars = *c->getVarsSingleton();
  for(size_t i = 0; i < vars.size(); ++i) {
    declaration << (i ? "," : "") << "{\"term\":" << term(vars[i].getBaseVar(), variableIds)
        << ",\"mappers\":[";
    auto mappers = vars[i].getMapperStack();
    for(size_t j = 0; j < mappers.size(); ++j) {
      auto type = mappers[j].type();
      declaration << (j ? "," : "") << "{\"type\":" << type;
      if(type != MAP_NEG && type != MAP_NOT) declaration << ",\"value\":" << mappers[j].val();
      declaration << '}';
    }
    declaration << "]}";
  }
  declaration << ']';
  emit("constraint", declaration.str());
  const auto& children = c->annotationChildren();
  for(size_t i = 0; i < children.size(); ++i) {
    auto child = constraint(children[i]);
    emit("child", ",\"parent\":" + tostring(id) + ",\"child\":" + tostring(child) +
         ",\"position\":" + tostring(i));
  }
  return id;
}

std::uint64_t AnnotationTrace::open(AbstractConstraint* c, const char* rule, const std::string& info) {
  auto cid = constraint(c);
  auto handle = ++nextContextHandle;
  contexts.push_back({handle, 0, cid, rule, info});
  if(allContexts) materializeContexts();
  return handle;
}

void AnnotationTrace::materializeContexts() {
  for(auto& context : contexts) {
    if(!context.id) {
      // The parent is the enclosing open context. Only probe contexts carry
      // information the checker reads; other diagnostic details are omitted.
      bool keepInfo = context.rule.compare(0, 6, "probe.") == 0;
      auto rule = stringId(context.rule);
      context.id = sequence + 1;
      record(keepInfo ? TagOpenInfo : TagOpen);
      putUnsigned(context.constraint);
      putUnsigned(rule);
      if(keepInfo) {
        putUnsigned(context.info.size());
        buffer += context.info;
      }
    }
  }
}

void AnnotationTrace::close(std::uint64_t handle) {
  if(contexts.empty() || contexts.back().handle != handle) throw minion_user_error("annotation context mismatch");
  if(active() && contexts.back().id) record(TagClose);
  contexts.pop_back();
}

void AnnotationTrace::push() {
  if(!active()) return;
  auto id = sequence + 1;
  record(TagPush);
  worlds.push_back(id);
}

void AnnotationTrace::pop() {
  if(!active()) return;
  if(worlds.empty()) throw minion_user_error("annotation world stack underflow");
  record(TagPop);
  worlds.pop_back();
}

void AnnotationTrace::prune(int type, int pos, const char* op, long long value,
                            const char* file, unsigned line) {
  if(assumptionDepth && !allContexts) return;
  if(siteFile) { file = siteFile; line = siteLine; }
  auto it = variableIds.find({type, pos});
  if(it == variableIds.end()) throw minion_user_error("annotation pruning an undeclared variable");
  materializeContexts();
  auto witness = useWitness();
  auto site = siteIds.find({file, line});
  if(site == siteIds.end())
    site = siteIds.insert({{file, line}, stringId(std::string(file) + ":" + tostring(line))}).first;
  unsigned char operation = op[0] == 'r' ? 0 : op[0] == 'a' ? 1 : op[0] == 'l' ? 2 : 3;
  // The context is the innermost open one.
  record((witness ? TagPruneWitness : TagPrune) + operation);
  putUnsigned(it->second);
  putSigned(value);
  putUnsigned(site->second);
  if(witness) putUnsigned(sequence - witness);
}

void AnnotationTrace::beginProbe() {
  if(!active()) return;
  if(probeOpen) throw minion_user_error("annotation probe inside the assumption of another");
  probeOpen = true;
  probeHasLower = probeHasUpper = false;
}

void AnnotationTrace::assume(int type, int pos, const char* operation, long long value) {
  auto it = variableIds.find({type, pos});
  if(it == variableIds.end()) throw minion_user_error("annotation probe on undeclared variable");
  if((probeHasLower || probeHasUpper) && probeVariable != it->second)
    throw minion_user_error("annotation probe restricts two variables");
  probeVariable = it->second;
  if(operation[0] == 'a' || operation[0] == 'l') { probeLower = value; probeHasLower = true; }
  if(operation[0] == 'a' || operation[0] == 'u') { probeUpper = value; probeHasUpper = true; }
  if(operation[0] == 'r') throw minion_user_error("annotation probe assumes a removal");
}

std::uint64_t AnnotationTrace::endProbe() {
  if(!active()) return 0;
  if(worlds.empty()) throw minion_user_error("annotation probe without a saved world");
  if(!probeOpen || !probeHasLower || !probeHasUpper)
    throw minion_user_error("annotation probe without an interval assumption");
  probeOpen = false;
  auto id = sequence + 1;
  emit("probe", ",\"id\":" + tostring(id) + ",\"world\":" + tostring(worlds.back()) +
       ",\"variable\":" + tostring(probeVariable) + ",\"lower\":" + tostring(probeLower) +
       ",\"upper\":" + tostring(probeUpper));
  return id;
}

void AnnotationTrace::probeResult(std::uint64_t id, bool failed) {
  if(!active()) return;
  if(worlds.empty()) throw minion_user_error("annotation probe result without a saved world");
  emit("probe_result", ",\"id\":" + tostring(id) + ",\"world\":" + tostring(worlds.back()) +
       ",\"failed\":" + std::string(failed ? "true" : "false"));
  completedProbe = failed ? id : 0;
}

std::uint64_t AnnotationTrace::branch(int type, int pos, long long value, bool equal, std::uint64_t split) {
  if(!active()) return 0;
  auto it = variableIds.find({type == VAR_NOTBOOL ? VAR_BOOL : type, pos});
  if(it == variableIds.end()) throw minion_user_error("annotation branch on undeclared variable");
  if(type == VAR_NOTBOOL) value = 1 - value;
  if(equal) split = sequence + 1;
  record(equal ? TagBranchLeft : TagBranchRight);
  putUnsigned(it->second);
  putSigned(value);
  if(!equal) putUnsigned(sequence - split);
  return split;
}

void AnnotationTrace::conflict() {
  if(active()) {
    materializeContexts();
    auto witness = useWitness();
    record(witness ? TagConflictWitness : TagConflict);
    if(witness) putUnsigned(sequence - witness);
  }
}

bool AnnotationTrace::directRoot(AbstractConstraint* c) const {
  return c && rootSources.find(c->annotationId) != rootSources.end();
}

std::uint64_t AnnotationTrace::useWitness() {
  if(!candidate) return 0;
  if(!candidate->id) {
    // Builders return one JSON field: ,"positions":[...] or ,"values":[...].
    auto fields = candidate->build();
    bool values = fields.compare(0, 10, ",\"values\":") == 0;
    if(!values && fields.compare(0, 13, ",\"positions\":") != 0)
      throw minion_user_error("unsupported annotation witness: " + fields);
    std::vector<long long> items;
    const char* cursor = fields.c_str() + fields.find('[') + 1;
    while(*cursor && *cursor != ']') {
      char* next = nullptr;
      items.push_back(std::strtoll(cursor, &next, 10));
      cursor = *next == ',' ? next + 1 : next;
    }
    auto kind = stringId(candidate->kind);
    candidate->id = sequence + 1;
    record(TagWitness);
    putUnsigned(candidate->constraint->annotationId);
    putUnsigned(kind);
    put(values ? 1 : 0);
    putUnsigned(items.size());
    for(auto item : items) putSigned(item);
  }
  return candidate->id;
}

void AnnotationTrace::closeBranch(std::uint64_t split) {
  if(active()) {
    record(TagBranchClose);
    putUnsigned(sequence - split);
  }
}

// A solution was found below this decision on an auxiliary variable, and its
// other side is not searched.
void AnnotationTrace::skipBranch(std::uint64_t split) {
  if(active()) {
    record(TagBranchSkip);
    putUnsigned(sequence - split);
  }
}

void AnnotationTrace::backtrack() {
  if(active()) record(TagBacktrack);
}

void AnnotationTrace::solution() {
  if(!active()) return;
  const auto vars = getState().getInstance()->vars.getAllVars();
  record(TagSolution);
  putUnsigned(vars.size());
  for(size_t i = 0; i < vars.size(); ++i) {
    auto v = BuildCon::getAnyVarRefFromVar(vars[i]);
    if(!v.isAssigned()) throw minion_user_error("annotation solution contains an unassigned variable");
    putSigned(checked_cast<SysInt>(v.assignedValue()));
  }
}

void AnnotationTrace::finish(bool complete) {
  if(!active()) return;
  size_t visibleContexts = 0;
  for(const auto& context : contexts) if(context.id != 0) ++visibleContexts;
  emit("end", ",\"complete\":" + std::string(complete ? "true" : "false") +
       ",\"open_worlds\":" + tostring(worlds.size()) + ",\"open_contexts\":" + tostring(visibleContexts));
  flushBuffer();
  output.flush();
  if(!output) throw minion_user_error("Failed flushing annotation file");
  ended = true;
}

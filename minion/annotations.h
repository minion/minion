// Minion https://github.com/minion/minion
// SPDX-License-Identifier: MPL-2.0
#ifndef MINION_ANNOTATIONS_H
#define MINION_ANNOTATIONS_H

#include <cstdint>
#include <fstream>
#include <map>
#include <string>
#include <vector>
#include <functional>
#include <memory>

#if defined(__clang__) || defined(__GNUC__)
#define MINION_ANNOTATION_FILE __builtin_FILE()
#define MINION_ANNOTATION_LINE __builtin_LINE()
#else
#define MINION_ANNOTATION_FILE __FILE__
#define MINION_ANNOTATION_LINE __LINE__
#endif

class AbstractConstraint;
class AnnotationWitness;
namespace ProbSpec { struct CSPInstance; struct ConstraintBlob; }

// Explanatory contexts are independent of backtracking worlds. A context may
// close without having emitted a pruning (e.g. an unsuccessful support query).
// Such unused contexts are omitted unless verbose context output is requested.
class AnnotationTrace {
  friend class AnnotationSite;
  friend class AnnotationWitness;
  friend class AnnotationAssumption;
  std::ofstream output;
  // Records are buffered here and written in large blocks.
  std::string buffer;
  std::map<std::string, std::uint64_t> stringIds;
  std::map<std::pair<const char*, unsigned>, std::uint64_t> siteIds;
  void flushBuffer();
  void put(unsigned char byte) { buffer.push_back((char)byte); }
  void putUnsigned(std::uint64_t value);
  void putSigned(long long value);
  std::uint64_t stringId(const std::string& text);
  void record(unsigned char tag);
  std::uint64_t sequence = 0, nextConstraint = 0;
  struct ContextFrame {
    std::uint64_t handle, id, constraint;
    std::string rule, info;
  };
  std::vector<ContextFrame> contexts;
  std::uint64_t nextContextHandle = 0;
  std::vector<std::uint64_t> worlds;
  std::map<const ProbSpec::ConstraintBlob*, std::string> sourcePaths;
  std::map<std::pair<int, int>, std::size_t> variableIds;
  bool started = false, ended = false;
  bool allContexts = false;
  unsigned assumptionDepth = 0;
  bool probeOpen = false, probeHasLower = false, probeHasUpper = false;
  std::size_t probeVariable = 0;
  long long probeLower = 0, probeUpper = 0;
  const char* siteFile = nullptr;
  unsigned siteLine = 0;
  AnnotationWitness* candidate = nullptr;
  std::map<std::uint64_t, const ProbSpec::ConstraintBlob*> rootSources;
  std::uint64_t useWitness();
  void materializeContexts();
public:
  std::uint64_t completedProbe = 0;
  explicit AnnotationTrace(const std::string& path, bool allContexts = false);
  ~AnnotationTrace();
  void emit(const std::string& event, const std::string& fields = "");
  void begin(ProbSpec::CSPInstance& instance);
  std::uint64_t constraint(AbstractConstraint* c, const ProbSpec::ConstraintBlob* source = nullptr);
  std::uint64_t open(AbstractConstraint* c, const char* rule, const std::string& info);
  void close(std::uint64_t id);
  void push();
  void pop();
  // A probe's assumption is read from the domain operations which apply it,
  // made between beginProbe and endProbe.
  void beginProbe();
  void assume(int type, int pos, const char* operation, long long value);
  std::uint64_t endProbe();
  bool probing() const { return probeOpen; }
  void probeResult(std::uint64_t id, bool failed);
  void prune(int type, int pos, const char* operation, long long value,
             const char* file, unsigned line);
  std::uint64_t branch(int type, int pos, long long value, bool equal, std::uint64_t split = 0);
  void closeBranch(std::uint64_t split);
  void skipBranch(std::uint64_t split);
  void conflict();
  void backtrack();
  void solution();
  void finish(bool complete);
  bool active() const { return started && !ended; }
  bool directRoot(AbstractConstraint* c) const;
};

// The trace being written, or null. Without MINION_ANNOTATIONS it is always
// null, so every hook below compiles to nothing.
#ifdef MINION_ANNOTATIONS
// Defined alongside the per-context/global solver accessors.
inline AnnotationTrace* annotationTrace();
#else
inline AnnotationTrace* annotationTrace() { return nullptr; }
#endif

// Branch/probe events already state their assumptions. Avoid recording the
// same low-level mutations a second time; propagation runs outside this scope.
class AnnotationAssumption {
  AnnotationTrace* trace;
public:
  AnnotationAssumption() : trace(annotationTrace()) { if(trace) ++trace->assumptionDepth; }
  ~AnnotationAssumption() { if(trace) --trace->assumptionDepth; }
  AnnotationAssumption(const AnnotationAssumption&) = delete;
  AnnotationAssumption& operator=(const AnnotationAssumption&) = delete;
};

// A provisional, scoped reason. Neither its builder nor output runs unless a
// real pruning/conflict consumes it. Borrowed captures must outlive this scope;
// deferred pruning must instead retain owned snapshots.
class AnnotationWitness {
  friend class AnnotationTrace;
  AnnotationTrace* trace;
  AnnotationWitness* previous = nullptr;
  AbstractConstraint* constraint;
  const char* kind;
  std::function<std::string()> build;
  std::uint64_t id = 0;
public:
  template<class Builder>
  AnnotationWitness(AbstractConstraint* c, const char* k, Builder builder)
      : trace(annotationTrace()), constraint(c), kind(k) {
    if(trace && trace->active()) {
      previous = trace->candidate;
      if(k && *k && trace->directRoot(c)) { build = builder; trace->candidate = this; }
      else trace->candidate = nullptr;
    } else trace = nullptr;
  }
  ~AnnotationWitness() { if(trace) trace->candidate = previous; }
  AnnotationWitness(const AnnotationWitness&) = delete;
  AnnotationWitness& operator=(const AnnotationWitness&) = delete;
};

// Preserve the outer call site across type erasure and variable-view mappings.
class AnnotationSite {
  AnnotationTrace* trace;
  bool owns = false;
public:
  AnnotationSite(const char* file, unsigned line) : trace(annotationTrace()) {
    if(trace && trace->active() && !trace->siteFile) {
      trace->siteFile = file;
      trace->siteLine = line;
      owns = true;
    }
  }
  ~AnnotationSite() { if(owns) trace->siteFile = nullptr; }
  AnnotationSite(const AnnotationSite&) = delete;
  AnnotationSite& operator=(const AnnotationSite&) = delete;
};

class AnnotationContext {
  AnnotationTrace* trace;
  std::uint64_t id = 0;
public:
  struct LazyInfo {};
  AnnotationContext(AbstractConstraint* c, const char* rule, const std::string& info = "{}")
      : trace(annotationTrace()) {
    if(trace && trace->active()) id = trace->open(c, rule, info);
  }
  template<class Info>
  AnnotationContext(AbstractConstraint* c, const char* rule, Info info, LazyInfo)
      : trace(annotationTrace()) {
    if(trace && trace->active()) id = trace->open(c, rule, info());
  }
  void close() { if(id) { trace->close(id); id = 0; } }
  ~AnnotationContext() { close(); }
  AnnotationContext(const AnnotationContext&) = delete;
  AnnotationContext& operator=(const AnnotationContext&) = delete;
};

// Called at the lowest variable-reference layer, after view mappings have
// translated the operation back to a declared variable. No-op attempts are
// omitted. The event precedes mutation so a failing restriction is visible.
template<class Variable, class Value>
inline void annotateDomain(const Variable& v, const char* op, Value value,
                           const char* file, unsigned line) {
  auto* trace = annotationTrace();
  if(!trace || !trace->active()) return;
  if(trace->probing()) {
    auto base = v.getBaseVar();
    trace->assume(base.type(), base.pos(), op, checked_cast<SysInt>(value));
  }
  const std::string operation(op);
  if((operation == "lower_bound" && value <= v.min()) ||
     (operation == "upper_bound" && value >= v.max()) ||
     (operation == "remove" && !v.inDomain(value)) ||
     (operation == "assign" && v.isAssigned() && v.assignedValue() == value)) return;
  auto base = v.getBaseVar();
  trace->prune(base.type(), base.pos(), op, checked_cast<SysInt>(value), file, line);
}

#endif

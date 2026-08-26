#!/usr/bin/env python3
"""Check the documentation against the code it describes.

Compares the flags in minion/commandline_parse.cpp against both
usage/commandline.rst and the flag table in minion/help/help.cpp, and the
constraints in the /* JSON */ blocks against usage/constraints.rst, and the
statistics keys written to -jsontableout against usage/output.rst.  Also
checks that every '<#anchor>' link in usage/constraints.rst resolves, and that
the anchor Minion prints for a constraint is unique.

In the code but not the docs is an error unless listed in known-doc-gaps.txt.
In the docs but not the code is always an error.  An entry in
known-doc-gaps.txt that is no longer a gap is also an error, so that file can
only shrink.
"""

import json
import os
import re
import sys

SCRIPTDIR = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(SCRIPTDIR)

COMMANDLINE_CPP = os.path.join(ROOT, "minion", "commandline_parse.cpp")
HELP_CPP = os.path.join(ROOT, "minion", "help", "help.cpp")
COMMANDLINE_RST = os.path.join(SCRIPTDIR, "usage", "commandline.rst")
CONSTRAINTS_RST = os.path.join(SCRIPTDIR, "usage", "constraints.rst")
OUTPUT_RST = os.path.join(SCRIPTDIR, "usage", "output.rst")
GAPS_FILE = os.path.join(SCRIPTDIR, "known-doc-gaps.txt")

# Cannot be written in a .minion file, so must not be documented.
INTERNAL_CONSTRAINTS = {
    "()()collectevents()()",
    "__reify_diseq",
    "__reify_eq",
    "__reify_minuseq",
}

CHECKS = ("commandline.rst", "help.cpp", "constraints.rst", "output.rst")


def flags_in_code():
    """Every flag parseCommandLine() compares against."""
    text = open(COMMANDLINE_CPP).read()
    return set(re.findall(r'command == string\("([^"]*)"\)', text))


def rst_headings(path):
    """Section titles in an .rst file, as (title, underline char) pairs."""
    lines = open(path).read().split("\n")
    out = []
    for i, line in enumerate(lines[:-1]):
        title = line.strip()
        under = lines[i + 1].strip()
        if not title or not under:
            continue
        if len(under) < 3 or len(set(under)) != 1:
            continue
        if under[0] not in "=-~^\"'`#*+_:.":
            continue
        if len(under) < len(title):  # docutils warns, and it is usually a typo
            continue
        out.append((title, under[0]))
    return out


def flags_in_rst():
    """Flags with their own '~~~' subsection in commandline.rst."""
    flags = set()
    for title, under in rst_headings(COMMANDLINE_RST):
        if under == "~" and title.startswith("-"):
            flags.add(title.split()[0])
    return flags


def flags_in_help_cpp():
    """Flags listed in the flagTable that 'minion --help' prints from."""
    text = open(HELP_CPP).read()
    start = text.index("const FlagHelp flagTable[] = {")
    end = text.index("\n};", start)
    return set(re.findall(r'\{\s*"[^"]*"\s*,\s*"(-[^"]*)"', text[start:end]))


def strip_comments(text):
    """Remove // and /* */ comments, so an example in a comment is not mistaken
    for a real call."""
    text = re.sub(r"/\*.*?\*/", "", text, flags=re.S)
    return re.sub(r"//[^\n]*", "", text)


def tableout_keys_in_code():
    """Keys the code writes into the -jsontableout / -tableout object.

    Two spellings reach the table: a direct .set("Key", ...), and the timers,
    which take the key as the store_name argument of maybePrint*Store.
    """
    keys = set()
    for dirpath, _, filenames in os.walk(os.path.join(ROOT, "minion")):
        for filename in filenames:
            if not filename.endswith((".h", ".hpp", ".cpp")):
                continue
            text = strip_comments(open(os.path.join(dirpath, filename)).read())
            keys.update(re.findall(r'\.set\(\s*(?:string\()?"(\w+)"', text))
            keys.update(re.findall(r'maybePrint\w*Store\([^;]*?,\s*"[^"]*"\s*,\s*"(\w+)"', text))
    return keys


def tableout_keys_in_rst():
    """Keys listed in the statistics table in usage/output.rst."""
    text = open(OUTPUT_RST).read()
    table = text[text.index("   * - Key"):text.index("What is promised")]
    return set(re.findall(r"\* - ``(\w+)``", table))


def doc_anchor(name):
    """The anchor Sphinx gives a heading, and docAnchor() in help.cpp gives a
    constraint name: lower case, each run of non-alphanumerics becoming '-'."""
    return re.sub(r"-+", "-", re.sub(r"[^a-z0-9]", "-", name.lower())).strip("-")


def constraints_in_code():
    """Constraint names from the /* JSON */ blocks, exactly as configure.py
    reads them: .h and .hpp files under minion/ only."""
    names = set()
    for dirpath, _, filenames in os.walk(os.path.join(ROOT, "minion")):
        for filename in filenames:
            if not filename.endswith((".h", ".hpp")):
                continue
            path = os.path.join(dirpath, filename)
            for block in re.findall(r"/\* JSON(.*?)\*/", open(path).read(), re.S):
                try:
                    data = json.loads(block)
                except ValueError as err:
                    sys.exit("Invalid JSON block in %s: %s" % (path, err))
                if data.get("type") == "constraint":
                    names.add(data["name"])
    return names


def constraints_in_rst():
    """Constraints with their own '^^^' subsection in constraints.rst.

    The heading text is also the HTML anchor Sphinx generates.
    """
    return {title for title, under in rst_headings(CONSTRAINTS_RST) if under == "^"}


def read_gaps():
    """Parse known-doc-gaps.txt into {check: {name: reason}}."""
    gaps = {check: {} for check in CHECKS}
    if not os.path.exists(GAPS_FILE):
        return gaps
    for lineno, line in enumerate(open(GAPS_FILE), 1):
        line = line.split("#", 1)[0].strip()
        if not line:
            continue
        parts = line.split(None, 2)
        if len(parts) < 3:
            sys.exit("%s:%d: expected '<check> <name> <reason>'" % (GAPS_FILE, lineno))
        check, name, reason = parts
        if check not in CHECKS:
            sys.exit("%s:%d: unknown check %r (expected one of %s)"
                     % (GAPS_FILE, lineno, check, ", ".join(CHECKS)))
        gaps[check][name] = reason
    return gaps


def main():
    gaps = read_gaps()
    problems = []

    def compare(check, in_code, in_docs, noun):
        missing = in_code - in_docs
        allowed = set(gaps[check])

        for name in sorted(missing - allowed):
            problems.append("%s: %s '%s' is not documented.\n"
                            "    Document it, or add this line to %s:\n"
                            "        %s %s <why not>"
                            % (check, noun, name, os.path.relpath(GAPS_FILE, ROOT),
                               check, name))

        for name in sorted(allowed - missing):
            if name in in_docs:
                why = "it is documented now"
            else:
                why = "no such %s exists any more" % noun
            problems.append("%s: stale known-doc-gaps.txt entry '%s' -- %s.\n"
                            "    Delete that line; the list is only allowed to shrink."
                            % (check, name, why))

        for name in sorted(in_docs - in_code):
            problems.append("%s: documents %s '%s', which the code does not define."
                            % (check, noun, name))

    code_flags = flags_in_code()
    compare("commandline.rst", code_flags, flags_in_rst(), "flag")

    # The flag table is hand-maintained next to the parser, so it is required to
    # match exactly; there is nothing here for known-doc-gaps.txt to excuse.
    compare("help.cpp", code_flags, flags_in_help_cpp(), "flag")

    compare("output.rst", tableout_keys_in_code(), tableout_keys_in_rst(),
            "statistics key")

    code_constraints = constraints_in_code()

    absent = INTERNAL_CONSTRAINTS - code_constraints
    for name in sorted(absent):
        problems.append("constraints.rst: INTERNAL_CONSTRAINTS in %s lists '%s',\n"
                        "    which no longer exists.  Remove it from that set."
                        % (os.path.relpath(__file__, ROOT), name))

    documented = constraints_in_rst()
    compare("constraints.rst",
            code_constraints - INTERNAL_CONSTRAINTS,
            documented,
            "constraint")

    # 'minion --help constraints' prints a link per constraint, built from the
    # name by the same rule Sphinx uses, so two headings sharing an anchor would
    # send one of those links to the wrong place.
    anchors = {}
    for title, _ in rst_headings(CONSTRAINTS_RST):
        anchors.setdefault(doc_anchor(title), []).append(title)
    for name in sorted(documented):
        clash = [t for t in anchors[doc_anchor(name)] if t != name]
        if clash:
            problems.append("constraints.rst: '%s' shares the anchor '#%s' with %s.\n"
                            "    Sphinx will rename one of them, breaking the link\n"
                            "    'minion --help constraints' prints."
                            % (name, doc_anchor(name), ", ".join(sorted(clash))))

    # Links written as `text <#anchor>`__ inside constraints.rst.
    valid = set(anchors)
    for target in sorted(set(re.findall(r"<#([A-Za-z0-9_-]+)>",
                                        open(CONSTRAINTS_RST).read()))):
        if target in valid:
            continue
        guess = [a for a in valid if a == doc_anchor(target)]
        problems.append("constraints.rst: link to '#%s', which is not a heading anchor.%s"
                        % (target, "\n    Did you mean '#%s'?" % guess[0] if guess else ""))

    if problems:
        print("Documentation is out of sync with the code:\n", file=sys.stderr)
        for problem in problems:
            print("  " + problem.replace("\n", "\n  "), file=sys.stderr)
            print(file=sys.stderr)
        print("%d problem%s." % (len(problems), "" if len(problems) == 1 else "s"),
              file=sys.stderr)
        return 1

    print("Docs in sync: %d flags, %d constraints (%d gaps still recorded in %s)."
          % (len(code_flags),
             len(code_constraints - INTERNAL_CONSTRAINTS),
             sum(len(v) for v in gaps.values()),
             os.path.relpath(GAPS_FILE, ROOT)))
    return 0


if __name__ == "__main__":
    sys.exit(main())

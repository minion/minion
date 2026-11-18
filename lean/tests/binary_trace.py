"""Write replay events in Minion's compact binary trace form (see ANNOTATIONS.md).

Tests inspect and damage traces as JSON events, then encode them again so the
checker reads the same form Minion writes. An event sequence which the binary
form cannot express, such as a context closed out of order, raises ValueError.
"""
import json

OPERATIONS = {"remove": 0, "assign": 1, "lower_bound": 2, "upper_bound": 3}


def unsigned(n):
    if not isinstance(n, int) or isinstance(n, bool) or n < 0:
        raise ValueError(f"not an unsigned integer: {n!r}")
    out = bytearray()
    while n >= 128:
        out.append((n & 127) | 128)
        n >>= 7
    out.append(n)
    return bytes(out)


def signed(n):
    if not isinstance(n, int) or isinstance(n, bool):
        raise ValueError(f"not an integer: {n!r}")
    return unsigned(n << 1 if n >= 0 else ((-n - 1) << 1) | 1)


def text(s):
    data = s.encode()
    return unsigned(len(data)) + data


def encode(events):
    out = bytearray(b"MINIONTR\x01")
    strings, contexts, worlds = {}, [], []

    def string(s):
        if not isinstance(s, str):
            raise ValueError(f"not a string: {s!r}")
        if s not in strings:
            strings[s] = len(strings)
            out.extend(b"\x01" + text(s))
        return unsigned(strings[s])

    def need(condition, message):
        if not condition:
            raise ValueError(message)

    def back(sequence, target):
        need(isinstance(target, int) and 0 < target <= sequence, "reference outside the trace")
        return unsigned(sequence - target)

    for sequence, e in enumerate(events, 1):
        need(isinstance(e, dict) and e.get("seq") == sequence, "sequence numbers are implied by position")
        kind = e.get("event")
        top = contexts[-1] if contexts else 0
        known = lambda *keys: need(set(e) <= {"seq", "event", *keys}, f"unexpected field in {kind}")
        if kind == "context_open":
            known("id", "parent", "constraint", "rule", "info")
            need(e["id"] == sequence and e["parent"] == top, "context identifiers are implied")
            probe = isinstance(e["rule"], str) and e["rule"].startswith("probe.")
            # Other contexts' diagnostic information is not part of the format.
            rule = string(e["rule"])
            out.extend(bytes([3 if probe else 2]) + unsigned(e["constraint"]) + rule)
            if probe:
                out.extend(text(json.dumps(e.get("info", {}))))
            contexts.append(sequence)
        elif kind == "context_close":
            known("id")
            need(top and e["id"] == top, "contexts close innermost first")
            contexts.pop()
            out.append(4)
        elif kind == "prune":
            known("context", "variable", "operation", "value", "rule", "info", "witness")
            need(e["context"] == top, "a deletion belongs to the innermost context")
            need(e["operation"] in OPERATIONS and e.get("info", {}) == {}, "unknown deletion")
            # The site is "file:line:operation"; the operation part is implied.
            need(isinstance(e["rule"], str) and ":" in e["rule"], "deletion site")
            site = string(e["rule"].rsplit(":", 1)[0])
            out.append(5 + OPERATIONS[e["operation"]] + (4 if "witness" in e else 0))
            out.extend(unsigned(e["variable"]) + signed(e["value"]) + site)
            if "witness" in e:
                out.extend(back(sequence, e["witness"]))
        elif kind == "push":
            known("id")
            need(e["id"] == sequence, "world identifiers are implied")
            worlds.append(sequence)
            out.append(13)
        elif kind == "pop":
            known("id")
            need(worlds and e["id"] == worlds[-1], "worlds are restored innermost first")
            worlds.pop()
            out.append(14)
        elif kind == "branch":
            known("split", "variable", "value", "equal")
            need(isinstance(e["equal"], bool), "branch direction")
            if e["equal"]:
                need(e["split"] == sequence, "split identifiers are implied")
                out.extend(b"\x0f" + unsigned(e["variable"]) + signed(e["value"]))
            else:
                out.extend(b"\x10" + unsigned(e["variable"]) + signed(e["value"]) + back(sequence, e["split"]))
        elif kind == "branch_close":
            known("split")
            out.extend(b"\x11" + back(sequence, e["split"]))
        elif kind == "branch_skip":
            known("split")
            out.extend(b"\x16" + back(sequence, e["split"]))
        elif kind == "conflict":
            need(e["context"] == top, "a conflict belongs to the innermost context")
            out.extend(b"\x13" + back(sequence, e["witness"]) if "witness" in e else b"\x12")
        elif kind == "backtrack":
            known()
            out.append(20)
        elif kind == "solution":
            known("values")
            out.extend(b"\x15" + unsigned(len(e["values"])) + b"".join(signed(v) for v in e["values"]))
        elif kind == "witness":
            key = "values" if "values" in e else "positions"
            known("id", "constraint", "kind", key)
            need(e["id"] == sequence, "witness identifiers are implied")
            out.extend(b"\x17" + unsigned(e["constraint"]) + string(e["kind"]) + bytes([key == "values"]))
            out.extend(unsigned(len(e[key])) + b"".join(signed(v) for v in e[key]))
        else:
            fields = {k: v for k, v in e.items() if k != "seq"}
            body = json.dumps({"event": kind, **{k: v for k, v in fields.items() if k != "event"}})
            out.extend(b"\x00" + text(body[1:-1]))
    return bytes(out)

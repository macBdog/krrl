"""Minimal YAML subset: nested maps, scalars. Enough for machine.yaml."""

from pathlib import Path


def _parse_scalar(s):
    s = s.strip()
    if s in ("true", "True", "yes"):
        return True
    if s in ("false", "False", "no"):
        return False
    if len(s) >= 2 and s[0] == s[-1] and s[0] in "\"'":
        return s[1:-1]
    try:
        if "." in s or "e" in s.lower():
            return float(s)
        return int(s)
    except ValueError:
        return s


def load_yaml(text: str) -> dict:
    lines = []
    for raw in text.splitlines():
        if not raw.strip() or raw.lstrip().startswith("#"):
            continue
        indent = len(raw) - len(raw.lstrip(" "))
        key, _, rest = raw.lstrip(" ").partition(":")
        lines.append((indent, key.strip(), rest.strip()))

    def walk(i, parent_indent):
        node = {}
        while i < len(lines):
            indent, key, val = lines[i]
            if indent < parent_indent:
                break
            if indent > parent_indent:
                raise ValueError("bad indent at %s" % key)
            if val == "":
                child, i = walk(i + 1, indent + 2)
                node[key] = child
            else:
                node[key] = _parse_scalar(val)
                i += 1
        return node, i

    root, _ = walk(0, 0)
    return root


def dump_yaml(obj, indent=0) -> str:
    pad = " " * indent
    out = []
    for k, v in obj.items():
        if isinstance(v, dict):
            out.append("%s%s:" % (pad, k))
            out.append(dump_yaml(v, indent + 2))
        elif isinstance(v, bool):
            out.append("%s%s: %s" % (pad, k, "true" if v else "false"))
        else:
            out.append("%s%s: %s" % (pad, k, v))
    return "\n".join(out) + ("\n" if indent == 0 else "")


def load_machine(path) -> dict:
    p = Path(path)
    return load_yaml(p.read_text(encoding="utf-8"))


def save_machine(path, obj) -> None:
    Path(path).write_text(dump_yaml(obj), encoding="utf-8")

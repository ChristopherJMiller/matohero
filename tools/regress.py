#!/usr/bin/env python3
"""Compare two reccmp JSON reports (reccmp --json) and list what changed.

  tools/regress.py before.json after.json

Prints functions that lost an exact match (<), dropped in similarity (-), newly matched
exactly (>) or improved (+). Exits 1 if anything got worse, so a loop iteration can't miss
a regression that isn't a lost 100% (e.g. 97% -> 60%)."""
import json
import sys


def load(path):
    out = {}
    for e in json.load(open(path))["data"]:
        if e.get("stub"):
            continue
        out[e["address"]] = (e.get("name"), e.get("matching", 0.0))
    return out


def main():
    if len(sys.argv) != 3:
        sys.exit(__doc__)
    before, after = load(sys.argv[1]), load(sys.argv[2])
    worse = False
    for addr in sorted(set(before) | set(after), key=lambda a: int(a, 16)):
        name, old = before.get(addr, (None, None))
        new_name, new = after.get(addr, (None, None))
        name = new_name or name
        if old is None or new is None:
            continue
        if old == 1.0 and new < 1.0:
            print(f"< {addr} {name} {old:.2%} -> {new:.2%}")
            worse = True
        elif new < old - 0.005:
            print(f"- {addr} {name} {old:.2%} -> {new:.2%}")
            worse = True
        elif new == 1.0 and old < 1.0:
            print(f"> {addr} {name} {old:.2%} -> 100%")
        elif new > old + 0.005:
            print(f"+ {addr} {name} {old:.2%} -> {new:.2%}")
    new_exact = [a for a in after if after[a][1] == 1.0 and a not in before]
    for addr in new_exact:
        print(f"> {addr} {after[addr][0]} (new)")
    exact = lambda d: sum(1 for _, m in d.values() if m == 1.0)
    print(f"exact: {exact(before)} -> {exact(after)}")
    return 1 if worse else 0


if __name__ == "__main__":
    sys.exit(main())

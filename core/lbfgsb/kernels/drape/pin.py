#!/usr/bin/env python3
"""Pin each drape kernel's emitted Slang into its Lean module.

Every lean/Drape/SlangCodegen/<Module>.lean carries a block

    -- BEGIN PIN
    def expected : String := "..."
    example : LeanSlang.emit shader = expected := by native_decide
    example : shader.entryPointName = "main" := by native_decide
    -- END PIN

This rewrites that block from kernels/drape/slang/<kernel>.slang, the text
`lake exe emit_drape` wrote. Run it only after a deliberate change to a
kernel (then gen.sh, then `lake build` re-checks every pin); an unintended
emission change is exactly what the pins exist to catch.

    python kernels/drape/pin.py            # rewrite the pins
    python kernels/drape/pin.py --check    # exit 1 if any pin is stale
"""

import pathlib
import re
import sys

HERE = pathlib.Path(__file__).resolve().parent
LEAN = HERE.parent.parent / "lean" / "Drape" / "SlangCodegen"

MODULES = {
    "LbBoxProject": "lb_box_project",
    "LbPgInf": "lb_pg_inf",
    "LbPgInfSerial": "lb_pg_inf_serial",
    "LbStepMax": "lb_step_max",
    "LbStepMaxSerial": "lb_step_max_serial",
    "LbRingStore": "lb_ring_store",
    "LbMultiDot": "lb_multi_dot",
    "LbMultiDotSerial": "lb_multi_dot_serial",
    "LbCompact": "lb_compact",
    "LbApplyM": "lb_apply_m",
    "LbCauchy": "lb_cauchy",
    "LbSubspace": "lb_subspace",
}

BLOCK = re.compile(r"-- BEGIN PIN\n.*?-- END PIN\n", re.S)


def lean_string(text):
    return '"' + text.replace("\\", "\\\\").replace('"', '\\"') + '"'


def pin_block(slang):
    text = slang[:-1] if slang.endswith("\n") else slang
    return (
        "-- BEGIN PIN\n"
        "def expected : String :=\n"
        + lean_string(text) + "\n\n"
        "example : LeanSlang.emit shader = expected := by native_decide\n"
        "example : shader.entryPointName = \"main\" := by native_decide\n"
        "-- END PIN\n"
    )


def main():
    check = "--check" in sys.argv[1:]
    stale = []
    for module, kernel in MODULES.items():
        path = LEAN / (module + ".lean")
        src = path.read_text(encoding="utf-8")
        slang = (HERE / "slang" / (kernel + ".slang")).read_text(encoding="utf-8")
        new = BLOCK.sub(lambda _: pin_block(slang), src, count=1)
        if not BLOCK.search(src):
            raise SystemExit("%s: no PIN block" % path)
        if new != src:
            stale.append(module)
            if not check:
                path.write_text(new, encoding="utf-8", newline="\n")
    if check:
        print("stale pins: %s" % (", ".join(stale) or "none"))
        sys.exit(1 if stale else 0)
    print("pinned %d modules (%d changed)" % (len(MODULES), len(stale)))


if __name__ == "__main__":
    main()

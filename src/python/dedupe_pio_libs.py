Import("env")
import os
import shutil

# PlatformIO 6.2 installs both "NimBLE-Arduino" (BLE-Gamepad dep) and
# "NimBLE-Arduino@1.4.1" (explicit pin). Linking both duplicates every symbol.
deps = os.path.join(env["PROJECT_LIBDEPS_DIR"], env["PIOENV"])
if os.path.isdir(deps):
    nimbles = sorted(
        p for p in os.listdir(deps)
        if p == "NimBLE-Arduino" or p.startswith("NimBLE-Arduino@")
    )
    if len(nimbles) > 1:
        keep = next((p for p in nimbles if p.startswith("NimBLE-Arduino@")), nimbles[0])
        for p in nimbles:
            if p != keep:
                victim = os.path.join(deps, p)
                print("GerdaLRS: removing duplicate library %s (keep %s)" % (p, keep))
                shutil.rmtree(victim)

import os
import sys

POSSIBLE_TOOLCHAINS = ["aarch64-elf-", "aarch64-linux-gnu-"]

# Ripped from whichcraft python library
def which(cmd, mode=os.F_OK | os.X_OK, path=None):
    def _access_check(fn, mode):
        return os.path.exists(fn) and os.access(fn,
                                                mode) and not os.path.isdir(fn)

    if os.path.dirname(cmd):
        if _access_check(cmd, mode):
            return cmd
        return None
    if path is None:
        path = os.environ.get("PATH", os.defpath)
    if not path:
        return None
    path = path.split(os.pathsep)
    if sys.platform == "win32":
        if os.curdir not in path:
            path.insert(0, os.curdir)
        pathext = os.environ.get("PATHEXT", "").split(os.pathsep)
        if any(cmd.lower().endswith(ext.lower()) for ext in pathext):
            files = [cmd]
        else:
            files = [cmd + ext for ext in pathext]
    else:
        files = [cmd]
    seen = set()
    for dir in path:
        normdir = os.path.normcase(dir)
        if normdir not in seen:
            seen.add(normdir)
            for thefile in files:
                name = os.path.join(dir, thefile)
                if _access_check(name, mode):
                    return name
    return None

def check_requirements():
    TOOLCHAIN = None
    DEBUGGER = None
    QEMU = None

    for toolchain in POSSIBLE_TOOLCHAINS:
        if which(toolchain + "gcc") is not None:
            TOOLCHAIN = toolchain
            break

    if not TOOLCHAIN:
        print("")
        print(
            "ERROR: Cross compiler toolchain not found: possible alternatives are:"
        )
        for toolchain in POSSIBLE_TOOLCHAINS:
            print("\t{}gcc".format(toolchain))

    for toolchain in POSSIBLE_TOOLCHAINS:
        if which(toolchain + "gdb") is not None:
            DEBUGGER = toolchain + "gdb"
            break

    if not DEBUGGER:
        print("")
        print(
            "WARNING: no debugger (aarch64 gdb) found: possible alternatives are:")
        for toolchain in POSSIBLE_TOOLCHAINS:
            print("\t{}gdb".format(toolchain))

    if which("qemu-system-aarch64") is not None:
        QEMU = "qemu-system-aarch64"
    else:
        print("")
        print(
            "WARNING: no emulator (qemu for aarch64) found: possible alternatives are:"
        )
        print("\tqemu-system-aarch64")

    return TOOLCHAIN, DEBUGGER, QEMU

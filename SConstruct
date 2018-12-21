import os
import sys
import multiprocessing


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


# Creates a Phony target
def PhonyTargets(
        target,
        action,
        depends,
        env=None,
):
    if not env: env = DefaultEnvironment()
    t = env.Alias(target, depends, action)
    env.AlwaysBuild(t)


AddOption(
    '--app',
    dest='app',
    type='string',
    nargs=1,
    action='store',
    metavar='FILE',
    help='Application to be compiled on HAL')

POSSIBLE_TOOLCHAINS = ["aarch64-elf-", "aarch64-linux-gnu-"]
ELF = 'output.elf'
LDSCRIPT = 'hal.ld'
KERNEL = 'kernel8.img'
APP = GetOption('app')
TOOLCHAIN = None
BUILD = "build"
FLAGS = ['-Wall', '-ffreestanding', '-nostdlib', '-nostartfiles', '-O0', '-g'],

VariantDir(BUILD, "source", duplicate=0)

Help(
    """
Type: 'scons' to build the Hardware Abstraction Layer,
      'scons run' to run the compiled kernel under qemu.
      'scons debug' to run a gdb server as well.

      The variable app must be provided to link an executable to be run
      as main program over the HAL.
""",
    append=False)

num_cpu = multiprocessing.cpu_count()
SetOption('num_jobs', num_cpu)
print("Running with -j {}".format(GetOption('num_jobs')))

for toolchain in POSSIBLE_TOOLCHAINS:
    if which(toolchain + "gcc") is not None:
        TOOLCHAIN = toolchain
        break

if not TOOLCHAIN:
    print()
    print("Cross compiler toolchain not found: possible alternatives are:")
    for toolchain in POSSIBLE_TOOLCHAINS:
        print("\t{}gcc".format(toolchain))
    print("Use the ")
    Exit(1)

env_options = {
    "CC":
    "{}gcc".format(TOOLCHAIN),
    "LINK":
    "{}ld".format(TOOLCHAIN),
    "ENV": {
        'PATH': os.environ['PATH'],
        'DISPLAY': os.environ['DISPLAY']
    },
    "CPPPATH": ['include', 'include/app'],
    "ASFLAGS":
    FLAGS,
    "CCFLAGS":
    FLAGS,
    "LINKFLAGS": [
        '-nostdlib', '-nostartfiles', '-T{}'.format(LDSCRIPT),
        '-o{}'.format(ELF)
    ],
}

sources = Glob("{}/*.S".format(BUILD))
sources += Glob("{}/*.c".format(BUILD))
sources += ["res/font.bin"]

if APP:
    sources += [APP]

env = Environment(**env_options)

env.Program(ELF, sources)

kernel = env.Command(KERNEL, ELF, '{}objcopy {} -O binary {}'.format(
    TOOLCHAIN, ELF, KERNEL))

boot = env.Command("boot/{}".format(KERNEL), KERNEL, Copy("$TARGET", "$SOURCE"))

if 'debug' in COMMAND_LINE_TARGETS:
    print("Starting gdb server...")
    PhonyTargets(
        env=env,
        target='debug',
        depends=boot,
        action=
        "qemu-system-aarch64 -M raspi3 -kernel boot/{} -drive file=test.dd,if=sd,format=raw -serial stdio -s -S"
        .format(KERNEL))

elif 'run' in COMMAND_LINE_TARGETS:
    print("running emulator...")
    PhonyTargets(
        env=env,
        target='run',
        depends=boot,
        action=
        "qemu-system-aarch64 -M raspi3 -kernel boot/{} -drive file=test.dd,if=sd,format=raw -serial stdio"
        .format(KERNEL))

if not os.path.exists(BUILD):
    print("Creating build folder...")
    Mkdir(BUILD)
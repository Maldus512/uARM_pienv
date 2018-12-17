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


def PhonyTargets(env=None, **kw):
    if not env: env = DefaultEnvironment()
    for target, action in kw.items():
        t = env.Alias(target, [], action)
        env.AlwaysBuild(t)
        Depends(t,KERNEL)


AddOption(
    '--app',
    dest='app',
    type='string',
    nargs=1,
    action='store',
    metavar='FILE',
    help='Application to be compiled on HAL')

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
print("running with -j {}".format(GetOption('num_jobs')))

POSSIBLE_TOOLCHAINS = ["aarch64-elf-", "aarch64-linux-gnu-"]
ELF = 'output.elf'
LDSCRIPT = 'hal.ld'
KERNEL = 'kernel8.img'
APP = ARGUMENTS.get('app')
TOOLCHAIN = None
BUILD = "build"

for toolchain in POSSIBLE_TOOLCHAINS:
    if which(toolchain + "gcc") is not None:
        TOOLCHAIN = toolchain
        break

if not TOOLCHAIN:
    print("Cross compiler not found")
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
    "CCFLAGS":
    ['-Wall', '-ffreestanding', '-nostdlib', '-nostartfiles', '-O0', '-g'],
    "LINKFLAGS": [
        '-nostdlib', '-nostartfiles', '-T{}'.format(LDSCRIPT),
        '-o{}/{}'.format(BUILD, ELF)
    ],
}

sources = Glob("source/*.S")
sources += Glob("source/*.c")
sources += ["res/font.bin"]

if APP:
    sources += [APP]

env = Environment(**env_options)

env.Program(ELF, sources)

kernel = env.Command(
    KERNEL, ELF, '{0}objcopy {1}/output.elf -O binary {1}/{2}'.format(
        TOOLCHAIN, BUILD, KERNEL))

env.Command("boot/{}".format(KERNEL), KERNEL, "cp {0}/{1} boot".format(
    BUILD, KERNEL))

if 'debug' in COMMAND_LINE_TARGETS:
    print("Starting gdb server...")
    PhonyTargets(
        env=env,
        debug=
        "qemu-system-aarch64 -M raspi3 -kernel boot/{} -drive file=test.dd,if=sd,format=raw -serial stdio -s -S"
        .format(KERNEL))

elif 'run' in COMMAND_LINE_TARGETS:
    print("running emulator...")
    PhonyTargets(
        env=env,
        run=
        "qemu-system-aarch64 -M raspi3 -kernel boot/{} -drive file=test.dd,if=sd,format=raw -serial stdio"
        .format(KERNEL))

if not os.path.exists(BUILD):
    print("Creating build folder...")
    os.makedirs(BUILD)
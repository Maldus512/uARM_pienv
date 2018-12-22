import os
import multiprocessing
from requirements import check_requirements


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

ELF = 'output.elf'
LDSCRIPT = 'hal.ld'
KERNEL = 'kernel8.img'
APP = GetOption('app')
TOOLCHAIN = None
DEBUGGER = None
QEMU = None
BUILD = "build"
FLAGS = [
    '-Wall', '-ffreestanding', '-nostdlib', '-nostartfiles', '-O0', '-g',
    '-march=armv8.1-a', '-mtune=cortex-a53'
],

VariantDir(BUILD, "source", duplicate=0)

Help(
    """
Type:\t'scons' or 'scons hal' to build the Hardware Abstraction Layer,
     \t'scons all' to build hal and example code
     \t'scons example' to build
     \t'scons run' to run the compiled kernel under qemu.
     \t'scons debug' to run a gdb server as well.
     \t'scons -c' to clean the corresponding target

The '--app' option must be provided to link an executable to be run as main program over the HAL.
""",
    append=False)

num_cpu = multiprocessing.cpu_count()
SetOption('num_jobs', num_cpu)
print("Running with -j {}".format(GetOption('num_jobs')))

TOOLCHAIN, DEBUGGER, QEMU = check_requirements()

if TOOLCHAIN == None:
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

kernel = env.Command(
    KERNEL, ELF, '{}objcopy {} -O binary {}'.format(TOOLCHAIN, ELF, KERNEL))

boot = env.Command("boot/{}".format(KERNEL), KERNEL, Copy(
    "$TARGET", "$SOURCE"))

env.Default(boot)

env.Alias("all", [boot, "example/app.elf"])
env.Alias("hal", boot)

if 'example' in COMMAND_LINE_TARGETS or 'all' in COMMAND_LINE_TARGETS:
    SConscript("example/SConstruct")

if 'debug' in COMMAND_LINE_TARGETS:
    if QEMU == None:
        print()
        print("ERROR: missing emulator")
        Exit(1)

    print("Starting gdb server...")
    PhonyTargets(
        env=env,
        target='debug',
        depends=boot,
        action=
        "{} -M raspi3 -kernel boot/{} -drive file=test.dd,if=sd,format=raw -serial stdio -s -S"
        .format(QEMU, KERNEL))

elif 'run' in COMMAND_LINE_TARGETS:
    if QEMU == None:
        print()
        print("ERROR: missing emulator")
        Exit(1)

    print("running emulator...")
    PhonyTargets(
        env=env,
        target='run',
        depends=boot,
        action=
        "{} -M raspi3 -kernel boot/{} -drive file=test.dd,if=sd,format=raw -serial stdio"
        .format(QEMU, KERNEL))

if not os.path.exists(BUILD):
    print("Creating build folder...")
    Mkdir(BUILD)
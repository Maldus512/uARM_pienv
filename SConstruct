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

AddOption(
    '--toolchain-path',
    dest='toolchain',
    type='string',
    nargs=1,
    action='store',
    help='Alternative toolchain path')

ELF = 'output.elf'
HAL = 'mal.elf'
LDSCRIPT = 'mal.ld'
KERNEL = 'kernel8.img'
APP = GetOption('app')
TOOLCHAIN = None
DEBUGGER = None
QEMU = None
BUILD = "build"
FLAGS = [
    '-Wall', '-ffreestanding', '-nostdlib', '-nostartfiles', '-O0', '-g',
    '-fPIE', '-ffixed-x27', '-ffixed-x28', '-march=armv8-a',
    '-mtune=cortex-a53'
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
The '--toolchain-path' option can provide an alternative (cross) compiler to build the kernel
""",
    append=False)

num_cpu = multiprocessing.cpu_count()
SetOption('num_jobs', num_cpu)
print("Running with -j {}".format(GetOption('num_jobs')))

TOOLCHAIN, DEBUGGER, QEMU = check_requirements()

alt_toolchain = GetOption('toolchain')
if alt_toolchain != None:
    TOOLCHAIN = alt_toolchain

if TOOLCHAIN == None:
    Exit(1)

externalEnvironment = {}
if 'PATH' in os.environ.keys():
    externalEnvironment['PATH'] = os.environ['PATH']
if 'DISPLAY' in os.environ.keys():
    externalEnvironment['DISPLAY'] = os.environ['DISPLAY']

env_options = {
    "CC":
    "{}gcc".format(TOOLCHAIN),
    "LINK":
    "{}ld".format(TOOLCHAIN),
    "ENV":
    externalEnvironment,
    "CPPPATH": ['include', 'include/maldos'],
    "ASFLAGS":
    FLAGS,
    "CCFLAGS":
    FLAGS,
    "LINKFLAGS": [
        '-nostdlib',
        '-nostartfiles',
        '-r',
        '-T{}'.format(LDSCRIPT),  #'-pie',
        '-o{}'.format(HAL)
    ],
}

sources = Glob("{}/*.S".format(BUILD))
sources += Glob("{}/*.c".format(BUILD))
sources += Glob("{}/emulated/*.c".format(BUILD))
sources += Glob("{}/hal/*.c".format(BUILD))
sources += ["res/font.bin"]

env = Environment(**env_options)

env.Program(HAL, sources)

env_options['LINKFLAGS'] = [
    '-nostdlib', '-nostartfiles', '-T{}'.format(LDSCRIPT), '-o{}'.format(ELF)
]

if APP:
    env.Command(
        ELF, HAL, '{}ld -nostdlib -nostartfiles -pie -T{} -o{} {} {}'.format(
            TOOLCHAIN, LDSCRIPT, ELF, HAL, APP))
else:
    env.Command(
        ELF, HAL, '{}ld -nostdlib -nostartfiles -pie -T{} -o{} {}'.format(
            TOOLCHAIN, LDSCRIPT, ELF, HAL))

kernel = env.Command(
    KERNEL, ELF, '{}objcopy {} -O binary {}'.format(TOOLCHAIN, ELF, KERNEL))

boot = env.Command("boot/{}".format(KERNEL), KERNEL, Copy(
    "$TARGET", "$SOURCE"))

env.Default(boot)

env.Alias("all", [boot, "example/app.elf"])
env.Alias("hal", boot)

PhonyTargets(env=env, target='install', depends=boot, action=
    "cp -r include/maldos /usr/local/include && mkdir -p /usr/local/share/maldos/ && cp {} /usr/local/share/maldos".format(HAL))
PhonyTargets(env=env, target='remove', depends=[], action=
    "rm -r /usr/local/include/maldos/*.h && rm -r /usr/local/share/maldos/{}".format(HAL))

if 'example' in COMMAND_LINE_TARGETS or 'all' in COMMAND_LINE_TARGETS:
    Export('alt_toolchain')
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
        "{} -M raspi3 -kernel boot/{} -drive file=test.dd,if=sd,format=raw -serial stdio -serial vc -s -S"
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
        "{} -M raspi3 -kernel boot/{} -drive file=test.dd,if=sd,format=raw -serial stdio -serial vc"
        .format(QEMU, KERNEL))

if not os.path.exists(BUILD):
    print("Creating build folder...")
    Mkdir(BUILD)

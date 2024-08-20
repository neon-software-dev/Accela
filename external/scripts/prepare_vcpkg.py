import os
import subprocess


def prepare_vcpkg(args):
    if args.no_local_vcpkg:
        print("Skipping vcpkg - configured not to use it")
        return

    if os.path.isdir('vcpkg'):
        print("Skipping vcpkg - already exists")
        return

    print("[Preparing vcpkg]")

    subprocess.run(['git', 'clone', "https://github.com/microsoft/vcpkg.git"])
    os.chdir('vcpkg')

    if os.name == 'nt':
        os.system('bootstrap-vcpkg.bat --disableMetrics')
    else:
        os.system('./bootstrap-vcpkg.sh --disableMetrics')

    print("[Prepared vcpkg]")

import os
import argparse

from prepare_physx import prepare_physx
from prepare_openal import prepare_openal
from prepare_vcpkg import prepare_vcpkg

parser = argparse.ArgumentParser(description='Fetches and makes available non-package-management Accela dependencies')

# Physx Arguments
parser.add_argument('--physx-preset',
                    choices=['linux', 'linux-aarch64', 'vc16win64', 'vc17win64'],
                    help='PhysX build preset to use')

# vcpkg arguments
parser.add_argument('--no-local-vcpkg', action='store_true', help='Disables local vcpkg installation')

args = parser.parse_args()

# Create and switch to External directory 
external_dir = os.getcwd()

print("Preparing dependencies...")

install_dir = external_dir + "/build"
install_dir_debug = install_dir + "/debug"
install_dir_release = install_dir + "/release"

#######
# PhysX
os.chdir(external_dir)
prepare_physx(args)

#######
# OpenAL
os.chdir(external_dir)
prepare_openal(args, install_dir_debug, install_dir_release)

#######
# vcpkg
os.chdir(external_dir)
prepare_vcpkg(args)

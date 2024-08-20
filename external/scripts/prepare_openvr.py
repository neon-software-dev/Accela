import os
import subprocess
import tarfile
import urllib.request

openvr_archive = 'openvr-2.5.1.tar.gz'  # Name of the OpenVR source archive
openvr_extract_dir = 'openvr-2.5.1'  # Name of the dir that's extracted from the OpenVR source archive


def build_openvr_linux(install_dir_debug, install_dir_release):
    build_dir = os.getcwd()

    os.mkdir("debug")
    os.chdir("debug")
    os.system("cmake -DBUILD_SHARED=OFF -DCMAKE_BUILD_TYPE=Debug -DCMAKE_INSTALL_PREFIX=%s ../../" % install_dir_debug)
    os.system("make -j $(nproc)")
    os.system("make install")

    os.chdir(build_dir)

    os.mkdir("release")
    os.chdir("release")
    os.system("cmake -DBUILD_SHARED=OFF -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=%s ../../" % install_dir_release)
    os.system("make -j $(nproc)")
    os.system("make install")


def build_openvr_windows(install_dir_debug, install_dir_release):
    build_dir = os.getcwd()

    os.mkdir("debug")
    os.chdir("debug")
    os.system("cmake -DBUILD_SHARED=OFF -DCMAKE_BUILD_TYPE=Debug -DCMAKE_INSTALL_PREFIX=%s ../../" % install_dir_debug)
    subprocess.run(['msbuild', 'OpenVRSDK.sln', "/p:Configuration=Debug"])
    subprocess.run(['msbuild', 'INSTALL.vcxproj', "/p:Configuration=Debug"])

    os.chdir(build_dir)

    os.mkdir("release")
    os.chdir("release")
    os.system("cmake -DBUILD_SHARED=OFF -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=%s ../../" % install_dir_release)
    subprocess.run(['msbuild', 'OpenVRSDK.sln', "/p:Configuration=Release"])
    subprocess.run(['msbuild', 'INSTALL.vcxproj', "/p:Configuration=Release"])


def prepare_openvr(args, install_dir_debug, install_dir_release):
    if os.path.isdir(openvr_extract_dir):
        print("Skipping OpenVR - already exists")
        return

    print("[Preparing openvr]")

    if not os.path.isfile(openvr_archive):
        print('- OpenVR archive doesn\'t exist, downloading it')

        # Download the 2.5.1 release source archive from GitHub
        urllib.request.urlretrieve(
            "https://github.com/ValveSoftware/openvr/archive/refs/tags/v2.5.1.tar.gz", openvr_archive)

    # Extract the OpenVR source
    with tarfile.open(openvr_archive) as f:
        f.extractall()

    # Build OpenVR
    os.chdir(openvr_extract_dir)
    os.mkdir("build")
    os.chdir("build")

    if os.name == 'nt':
        build_openvr_windows(install_dir_debug, install_dir_release)
    else:
        build_openvr_linux(install_dir_debug, install_dir_release)

    print("[Prepared openvr]")

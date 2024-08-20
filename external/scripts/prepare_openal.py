import os
import subprocess
import tarfile
import urllib.request

openal_archive = 'openal-soft-1.23.1.tar.bz2'  # Name of the OpenAL source archive
openal_extract_dir = 'openal-soft-1.23.1'  # Name of the dir that's extracted from the OpenAL source archive

alsoft_options = ("-DBUILD_SHARED_LIBS=ON -DALSOFT_UTILS=OFF -DALSOFT_NO_CONFIG_UTIL=ON -DALSOFT_EXAMPLES=OFF "
                  "-DALSOFT_TESTS=OFF -DALSOFT_INSTALL_EXAMPLES=OFF -DALSOFT_INSTALL_UTILS=OFF")


def build_openal_linux(install_dir_debug, install_dir_release):
    build_dir = os.getcwd()

    os.mkdir("debug")
    os.chdir("debug")
    os.system("cmake -DCMAKE_BUILD_TYPE=Debug %s -DCMAKE_INSTALL_PREFIX=%s ../../"
              % (alsoft_options, install_dir_debug))
    os.system("make -j $(nproc)")
    os.system("make install")

    os.chdir(build_dir)

    os.mkdir("release")
    os.chdir("release")
    os.system("cmake -DCMAKE_BUILD_TYPE=Release %s -DCMAKE_INSTALL_PREFIX=%s ../../" %
              (alsoft_options, install_dir_release))
    os.system("make -j $(nproc)")
    os.system("make install")


def build_openal_windows(install_dir_debug, install_dir_release):
    build_dir = os.getcwd()

    os.mkdir("debug")
    os.chdir("debug")
    os.system("cmake -DCMAKE_BUILD_TYPE=Debug %s -DCMAKE_INSTALL_PREFIX=%s ../../"
              % (alsoft_options, install_dir_debug))
    subprocess.run(['msbuild', 'OpenAL.sln', "/p:Configuration=Debug"])
    subprocess.run(['msbuild', 'INSTALL.vcxproj', "/p:Configuration=Debug"])

    os.chdir(build_dir)

    os.mkdir("release")
    os.chdir("release")
    os.system("cmake -DCMAKE_BUILD_TYPE=Release %s -DCMAKE_INSTALL_PREFIX=%s ../../"
              % (alsoft_options, install_dir_release))
    subprocess.run(['msbuild', 'OpenAL.sln', "/p:Configuration=Release"])
    subprocess.run(['msbuild', 'INSTALL.vcxproj', "/p:Configuration=Release"])


def prepare_openal(args, install_dir_debug, install_dir_release):
    if os.path.isdir(openal_extract_dir):
        print("Skipping OpenAL - already exists")
        return

    print("[Preparing openal-soft]")

    if not os.path.isfile(openal_archive):
        print('- OpenAL archive doesn\'t exist, downloading it')

        # Download the 2.5.1 release source archive from GitHub
        urllib.request.urlretrieve(
            "https://github.com/kcat/openal-soft/releases/download/1.23.1/openal-soft-1.23.1.tar.bz2", openal_archive)

    # Extract the OpenAL source
    with tarfile.open(openal_archive) as f:
        f.extractall()

    # Build OpenAL
    os.chdir(openal_extract_dir)
    os.chdir("build")

    if os.name == 'nt':
        build_openal_windows(install_dir_debug, install_dir_release)
    else:
        build_openal_linux(install_dir_debug, install_dir_release)

    print("[Prepared openvr]")

import os
import shutil
import subprocess


def prepare_manifest_vcpkg():
    if os.path.isdir('vcpkg_manifest'):
        print("Skipping manifest vcpkg - already exists")
        return

    subprocess.run(['git', 'clone', "https://github.com/microsoft/vcpkg.git"])
    os.rename('vcpkg', 'vcpkg_manifest')
    os.chdir('vcpkg_manifest')

    if os.name == 'nt':
        os.system('bootstrap-vcpkg.bat -disableMetrics')
    else:
        os.system('./bootstrap-vcpkg.sh --disableMetrics')

    print("[Prepared manifest vcpkg]")
    

# Note: Not using this right now, just relying on community triplet existing
def create_custom_triplet():
    print('[Creating custom triplet]')
    os.mkdir("custom-triplets")
    shutil.copyfile('triplets/x64-linux.cmake', 'custom-triplets/x64-linux-dynamic.cmake')
    
    # Read the contents of the triplet file
    fpath = 'custom-triplets/x64-linux-dynamic.cmake'
    f = open(fpath, 'r')
    lines = f.readlines()
    f.close()

    newlines = []

    # Update the static linkage config to be dynamic
    for line in lines:
        newlines.append(line.replace("set(VCPKG_LIBRARY_LINKAGE static)", "set(VCPKG_LIBRARY_LINKAGE dynamic)"))

    # Overwrite the triplet file with the modified lines
    f = open(fpath, 'w')
    for line in newlines:
        f.write("{}".format(line))
    f.close()
    
    
def install_dep(dep, linuxDynamic):
    depStr = dep
    
    # If we're on linux and the dependency should be linux dynamic, use the custom triplet
    if os.name != 'nt' and linuxDynamic:
    	depStr = dep + ':x64-linux-dynamic'

    print('[Installing: %s]' % (depStr))
    
    if os.name == 'nt':
        os.system('vcpkg install %s' % (depStr))
    else:
        os.system('./vcpkg install %s' % (depStr))


def prepare_manual_vcpkg():
    if os.path.isdir('vcpkg_manual'):
        print("Skipping manual vcpkg - already exists")
        return

    subprocess.run(['git', 'clone', "https://github.com/microsoft/vcpkg.git"])
    os.rename('vcpkg', 'vcpkg_manual')
    os.chdir('vcpkg_manual')

    if os.name == 'nt':
        os.system('bootstrap-vcpkg.bat -disableMetrics')
    else:
        os.system('./bootstrap-vcpkg.sh --disableMetrics')
        
    # Install manual dependencies
    install_dep('openal-soft', True)
    install_dep('ffmpeg[core,avcodec,avdevice,avformat,avfilter,ass,swresample,swscale,nvcodec]', True)

    print("[Prepared manual vcpkg]")


def prepare_vcpkg(args):
    external_dir = os.getcwd()

    print("[Preparing manifest vcpkg]")
    prepare_manifest_vcpkg()
    
    print("[Preparing manual vcpkg]")
    os.chdir(external_dir)
    prepare_manual_vcpkg()
          


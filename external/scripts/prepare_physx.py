import os
import subprocess
import tarfile
import urllib.request

physx_archive = 'PhysX-5.4.1.tar.gz'  # Name of the PhysX source archive
physx_extract_dir = 'PhysX-106.0-physx-5.4.1'  # Name of the dir that's extracted from the PhysX source archive


##################################################
# Modifies PhysX's custom "preset" xml files to 
# have the proper build settings for each platform
##################################################

def line_contains_config(line, config):
    return any(config in s for s in line.split())


def configure_physx_preset(fpath, config_map):
    print("- Configuring preset: " + fpath)

    # Read the contents of the preset file
    f = open(fpath, 'r')
    lines = f.readlines()
    f.close()

    newlines = []

    # Look for lines with the configurations we're interested in and set their value to the correct value
    for line in lines:
        found_key = False
        for key in list(config_map.keys()):
            if line_contains_config(line, key):
                val = config_map[key]
                newlines.append(line.replace("value=\"% s\"" % (str(not val)), "value=\"% s\"" % (str(val))))
                found_key = True
                break
        if not found_key:
            newlines.append(line)

    # Overwrite the preset file with the modified lines
    f = open(fpath, 'w')
    for line in newlines:
        f.write("{}".format(line))
    f.close()


#######################################
# Downloads, builds, and installs PhysX
#######################################

def build_physx_linux(physx_preset):
    physx_dir = os.getcwd()

    ####
    # As of PhysX 5.4.1, on Linux, the build has warnings on recent compilers, so manually patch
    # their CMakeLists file to not treat warnings as errors, so their build can succeed
    print('- Disabling errors on warnings')
    os.system('sed -i \'s/\\s*-Werror//\' ./source/compiler/cmake/linux/CMakeLists.txt')

    ####
    # Configure PhysX presets
    print("- Configuring project presets")

    presets_rel_dir = './buildtools/presets/public/'

    for presetFileName in os.listdir('./buildtools/presets/public/'):
        configure_physx_preset(presets_rel_dir + presetFileName,
                               {"PX_GENERATE_STATIC_LIBRARIES": True, "PX_BUILDSNIPPETS": False})

    ####
    # Generate project files
    print('- Generating projects')
    preset_str = physx_preset if physx_preset is not None else ""
    os.system('./generate_projects.sh %s' % preset_str)

    """
  On Linux, generating projects creates separate compiler projects for each build variant: debug, checked, profile, 
  and release. We can directly make and install each generated project.
  """

    ####
    # Build the project
    print('- Building PhysX')

    for compilerDir in os.listdir('./compiler'):
        # Ignore the "public" directory that PhysX creates during project generation step
        if compilerDir == 'public':
            continue
        # For every other directory, cd into it and build and install it
        print('- Building PhysX: ' + compilerDir)
        os.chdir(physx_dir + '/compiler/' + compilerDir)
        os.system('make -j $(nproc)')
        os.system('make install')


def build_physx_windows(physx_preset):
    physx_dir = os.getcwd()

    #####
    # Run the configure/build/install flow for each variant, contrary to linux (see below)
    for variant in ['Debug', 'Release']:
        print("- Configuring project presets for " + variant)

        os.chdir(physx_dir)

        is_debug = variant == 'Debug'
        presets_rel_dir = './buildtools/presets/public/'

        for presetFileName in os.listdir(presets_rel_dir):
            configure_physx_preset(presets_rel_dir + presetFileName,
                                   {"PX_GENERATE_STATIC_LIBRARIES": True, "PX_BUILDSNIPPETS": False,
                                    "NV_USE_STATIC_WINCRT": False, "NV_USE_DEBUG_WINCRT": is_debug})

        ####
        # Generate project files for the variant
        print('- Generating projects')
        preset_str = physx_preset if physx_preset is not None else ""
        os.system('generate_projects.bat %s' % preset_str)

        """
        On Windows, generating projects creates a single compiler project which can then be set to build as either debug 
        or release at build time. However, the project needs separate CRT configuration for debug vs. release builds, so 
        we need to generate the project twice, once for debug, and once for release, building and installing each. (As 
        opposed to Linux, where generating projects creates separate compiler projects for each variant.)
        """

        ####
        # Build the project
        print('- Building PhysX')

        for compilerDir in os.listdir('./compiler'):
            # Ignore the "public" directory that PhysX creates during project generation step
            if compilerDir == 'public':
                continue

            # For every other directory, cd into it and build and install it
            os.chdir(physx_dir + '/compiler/' + compilerDir)
            subprocess.run(['msbuild', 'PhysXSDK.sln', "/p:Configuration=" + variant])
            subprocess.run(['msbuild', 'INSTALL.vcxproj', "/p:Configuration=" + variant])


def prepare_physx(args):
    physx_dir_exists = os.path.isdir(physx_extract_dir)

    if physx_dir_exists:
        print("Skipping PhysX - already exists")
        return

    print("[Preparing PhysX]")

    ####
    # Retrieve PhysX source

    if not os.path.isfile(physx_archive):
        print('- PhysX archive doesn\'t exist, downloading it')

        # Download the 5.4.1 release source archive from GitHub
        urllib.request.urlretrieve(
            "https://github.com/NVIDIA-Omniverse/PhysX/archive/refs/tags/106.0-physx-5.4.1.tar.gz", physx_archive)

    # Extract the PhysX source
    with tarfile.open(physx_archive) as f:
        f.extractall()

    os.chdir(physx_extract_dir + '/physx')

    ####
    # Build PhysX

    if os.name == 'nt':
        build_physx_windows(args.physx_preset)
    else:
        build_physx_linux(args.physx_preset)

    print("[Prepared PhysX]")

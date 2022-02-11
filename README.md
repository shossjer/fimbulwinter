# Fimbulwinter

Yet another game engine. Supported platforms are Windows and
Linux. Written in C++14.

## Building

If your project is using CMake it is recommended to add Fimbulwinter
to it with a call to `add_subdirectory`. That way you can easily setup
the parameters and flags you want and Fimbulwinter (and its
dependencies) will just inherit everything.

You can, of course, also build Fimbulwinter on its own.

### Instructions

#### Linux

It is recommended that you make a separate directory for your
build. Run cmake from there and build like you normally do:

```
cd build
cmake ..
make
```

#### Windows - Visual Studio

Run CMake from within the developer command prompt. This helps CMake
know the location of your system libraries. It is recommended that you
make a separate directory for your build.

After generating the build files you do not need to go through the
step of running CMake from the developer command prompt again.

### Options

* `CMAKE_BUILD_TYPE` (only for single configuration builds)

  Either the empty string, or the name of something for which there
  exists matching `CMAKE_*_FLAGS_name` variables,
  e.g. `CMAKE_CXX_FLAGS_name`.

* `CMAKE_CONFIGURATION_TYPES` (only for multi configuration builds)

  A semicolon separated list of names for which there exists matching
  `CMAKE_*_FLAGS_name` variables, e.g. `CMAKE_CXX_FLAGS_name`.

* `BUILD_TESTS`

  `ON` if tests should be built, which is the default if Catch (see
  [Dependencies](#dependencies)) is available. `OFF` otherwise.

* `BUILD_BENCHMARKS`

  Default is `OFF` but can be turned `ON` if Catch (see
  [Dependencies](#dependencies)) is available.

* `RUN_TESTS`

  `ON` or `OFF` depending on whether tests should be run as part of
  the build process, see [Testing](#testing). Really useful when
  developing Fimbulwinter. Its default value is `ON`.

* `AUDIO_USE_FMOD`

* `FILE_USE_INOTIFY`

* `FILE_USE_KERNEL32`

* `GRAPHICS_USE_OPENGL`

* `INPUT_HAS_USER32_HID`

* `TEXT_USE_FREETYPE`

* `TEXT_USE_USER32`

* `TEXT_USE_X11`

* `THREAD_USE_KERNEL32`

* `THREAD_USE_PTHREAD`

* `WINDOW_USE_USER32`

* `WINDOW_USE_X11`

* `PROFILING_COZ` (only for Linux)

  Turns Coz (see [Tools](#tools)) profiling `ON` or `OFF`, see
  #[Profiling](#profiling).

### Dependencies

Libraries that are _needed_ for building and running a derivative of
Fimbulwinter are collectively named "dependencies". These dependencies
can either be built as part of the configure process or searched for
in the sea that is the system libraries. It is recommended to build
them to avoid version mismatch, and will be by default.

All dependencies are collected in the `dep` directory and can be built
in a release and debug configuration. Exactly what these
configurations are can be specified by setting valid configurations to
the `DEP_RELEASE_CONFIG` and `DEP_DEBUG_CONFIG` variables. Their
default values are `Release` and `` (which corresponds to disabling
it). The release configuration is the normal case and the debug
configuration will only be considered for build configurations that
are listed in the `DEBUG_CONFIGURATION` global property.

Not all dependencies support explicit debug builds. For instance, this
is the case for header only libraries (which already implicitly
support debug builds).

Not all dependencies are hard requirements to build a meaningful
library of Fimbulwinter. After all, one of its core principles is to
be modular.

* Catch - https://github.com/catchorg/Catch2

* FreeType

* libpng

* nlohmann/json - https://github.com/nlohmann/json

* zlib

And some additional system things (to be updated).

## Benchmarking

(Micro) benchmarks are turned off by default but can be turned on via the
cmake option `BUILD_BENCHMARKS` but only if Catch is available.

### Compiler Explorer

As a way to help with benchmarking, Fimbulwinter can be configured to
setup a local modified version of the Compiler Explorer (see
https://github.com/compiler-explorer/compiler-explorer). This is
controlled by the CMake option `ENABLE_COMPILER_EXPLORER`, which is
`OFF` by default, and only available with CMake versions 3.14 and
above.

The modifications are meant to facilitate looking up project
files, and a few other things that the otherwise amazing online
version of Compiler Explorer cannot do, but are otherwise
unnecessary. This tool, although originally added to help with
benchmarking, is completely orthogonal to the benchmarking flags and
can thus be enabled without them.

#### Setup

1) Set `ENABLE_COMPILER_EXPLORER` to `ON` and configure your
   project. There should now be a directory
   `/path/to/fimbulwinter/tools/compiler-explorer`.

2) Configure your project a second time in order to generate some meta
   data about your project that Compiler Explorer will tap into. There
   should be a `.cmake` directory in your build directory after this
   step, and a nested `reply` directory.

3) Running Compiler Explorer requires Node.js (see https://nodejs.org)
   which if you are not a script-kiddie might sound scary or even
   humiliating, but this is where you need to come together and just
   install it because Compiler Explorer is worth it, seriously.

4) (WINDOWS ONLY) If you want to run Compiler Explorer from WSL, and
   you want to access `cl.exe`, you need to manually modify
   `path/to/fimbulwinter/tools/compiler-explorer/etc/config/c++.defaults.properties`
   to something like this:

   ```
   # Default settings for C++
   compilers=&gcc:&clang:&cl19
   [...]
   group.cl19.compilers=clx86:clx64
   group.cl19.compilerType=wsl-vc
   group.cl19.includeFlag=/I
   group.cl19.versionFlag=/?
   group.cl19.versionRe=^Microsoft \(R\) C/C\+\+.*$
   group.cl19.demanglerClassFile=./demangler-win32
   group.cl19.supportsBinary=false
   # group.cl19.options=/IC:/Program Files (x86)/Microsoft Visual Studio/2019/Community/VC/Tools/MSVC/14.25.28610/include /IC:/Program Files (x86)/Windows Kits/10/Include/10.0.18362.0/ucrt /IC:/Program Files (x86)/Windows Kits/10/Include/10.0.18362.0/um /IC:/Program Files (x86)/Windows Kits/10/Include/10.0.18362.0/shared <-- DOES NOT WORK
   compiler.clx86.exe=/mnt/c/Program Files (x86)/Microsoft Visual Studio/2019/Community/VC/Tools/MSVC/14.25.28610/bin/Hostx64/x86/cl.exe
   compiler.clx86.name=msvc 2019 x86
   compiler.clx86.demangler=/mnt/c/Program Files (x86)/Microsoft Visual Studio/2019/Community/VC/Tools/MSVC/14.25.28610/bin/Hostx64/x86/undname.exe
   compiler.clx64.exe=/mnt/c/Program Files (x86)/Microsoft Visual Studio/2019/Community/VC/Tools/MSVC/14.25.28610/bin/Hostx64/x64/cl.exe
   compiler.clx64.name=msvc 2019 x64
   compiler.clx64.demangler=/mnt/c/Program Files (x86)/Microsoft Visual Studio/2019/Community/VC/Tools/MSVC/14.25.28610/bin/Hostx64/x64/undname.exe
   [...]
   ```

5) Start your favorite console and execute:

   ```
   cd /path/to/fimbulwinter/tools/compiler-explorer
   make EXTRA_ARGS='--language c++'
   ```

   If everything works as intended, you will eventually be rewarded
   with a link to a local web service.

   Browsing project files can be done via the Save/Load button.

### Tools

Libraries that are not strictly _needed_ for running a derivative of
Fimbulwinter, and other things that can be considered helpful, are
collectively named "tools". These tools can be built as part of the
configure process but are turned off by default.

All tools are collected in the `tools` directory and can only be built
in a release configuration, as determined by the variable
`TOOLS_RELEASE_CONFIG`. It is `Release` by default.

* Compiler Explorer - https://github.com/compiler-explorer/compiler-explorer

* Coz: Causal Profiling - https://github.com/plasma-umass/coz

## Testing

Tests will always be built (there currently is no option to turn them
off) and the resulting executables will be labeled with `test` at the
end. Running the tests is simply a matter of running the
executables. The build process does this for you but can be turned off
via the `RUN_TESTS` option.

## Profiling

Profiling is disabled by default but can be turned on if Coz (see
[Tools](#tools)) can be found. This is controlled via the CMake option
`PROFILING_COZ`. When Coz profiling is enabled you need to run your
executable similar to this:

```
coz run --- /path/to/your/executable [args...]
```

or, if you built Coz as part of the configure process, simply execute

```
/path/to/fimbulwinter/tools/coz/run /path/to/your/executable [args...]
```

Make use of https://plasma-umass.org/coz/ to make sense of the
profiling being made. More about Coz can be read at their site.

Building Coz requires `pkg-config`, and `rst2man` (see
`python-docutils`) to be installed on the system.

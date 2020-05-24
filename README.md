# Fimbulwinter

Yet another game engine. Supported platforms are Windows and
Linux. Written in C++14.

## Build Instructions

If your project is using CMake it is recommended to add Fimbulwinter
to it with a call to `add_subdirectory`. That way you can easily setup
the parameters and flags you want and Fimbulwinter (and its
dependencies) will just inherit everything.

You can, of course, also build Fimbulwinter on its own.

### Linux

It is recommended that you make a separate directory for your
build. Run cmake from there and build like you normally do:

```
cd build
cmake ..
make
```

### Windows - Visual Studio

Run CMake from within the developer command prompt. This helps CMake
know the location of your system libraries. It is recommended that you
make a separate directory for your build.

After generating the build files you do not need to go through the
step of running CMake from the developer command prompt again.

## Build Options

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

## Testing

Tests will always be built (there currently is no option to turn them
off) and the resulting executables will be labeled with `test` at the
end. Running the tests is simply a matter of running the
executables. The build process does this for you but can be turned off
via the `RUN_TESTS` option.

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

* `RUN_TESTS`

  `ON` or `OFF` depending on whether tests should be run as part of
  the build process. Really useful when developing Fimbulwinter. Its
  default value is `ON`.

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

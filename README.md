# VCS

A control application for Datapath Vision capture cards on Linux.

![VCS 3.0](./screenshot1.png)

![VCS 3.0](./screenshot2.png)

![VCS 3.0](./screenshot3.png)

## Key features

- Unlimited video presets for analog capture
- Several scaling modes and image filters
- Variable refresh rate output
- Free and open source

## Supported capture hardware

Any model of Datapath capture card supported by the Datapath Vision driver for Linux should be compatible, although the card's full set of capabilities may not be exposed.

## Manuals

- [User's manual](./docs/user/README.md)

## Building

![](./cat1.jpg)

Open [vcs.pro](vcs.pro) in Qt Creator, or run `$ qmake && make -j` in the repo's root. You'll need to meet the [dependencies](#dependencies), and may need to modify some of the `INCLUDEPATH` and `LIBS` entries in [vcs.pro](vcs.pro) to match your system.

### Dependencies

The VCS codebase depends on the following libraries and frameworks:

1. Qt 5
2. OpenCV 4
3. For [non-release builds](#release-build-vs-debug-build), AddressSanitizer and UndefinedBehaviorSanitizer, which should come pre-installed with your compiler*

\* To suppress Sanitizer warnings in external libraries, create an `asan-suppress.txt` file in the build directory and populate it according to [this guide](https://github.com/google/sanitizers/wiki/AddressSanitizerLeakSanitizer#suppressions).

I use the following toolchains when developing VCS:

| OS                 | Compiler           | Qt   | OpenCV | Capture card           |
| ------------------ | ------------------ | ---- | ------ | ---------------------- |
| Ubuntu 20.04 HWE   | GCC 9.4 (64-bit)   | 5.12 | 4.2.0  | Datapath VisionRGB-E1S |

### Release build vs. debug build

The default configuration in [vcs.pro](vcs.pro) produces a debug build, which has &ndash; among other things &ndash; more copious run-time bounds-checking of memory accesses. The run-time debugging features are expected to reduce performance to some extent, but can help reveal programming errors.

Defining `VCS_RELEASE_BUILD` globally will produce a release build, with fewer debugging checks in performance-critical sections of the program. Simply uncomment `DEFINES += VCS_RELEASE_BUILD` at the top of [vcs.pro](vcs.pro) and do a full rebuild.

To confirm whether the program is running in release or debug mode, navigate to Control panel &rarr; About VCS.

### The capture backend

A capture backend is an implementation in VCS providing support for a particular capture source, be it a hardware device (e.g. a Datapath Vision capture card) or something else.

One &ndash; and only one &ndash; capture backend must be active in any build of VCS. To select which, include the corresponding identifier in the `DEFINES` variable in [vcs.pro](vcs.pro).

The following capture backends are available to choose from:

| Backend identifier          | Purpose                                                                                                                       |
| --------------------------- | ----------------------------------------------------------------------------------------------------------------------------- |
| CAPTURE_BACKEND_VISION_V4L  | Supports the Datapath Vision range of capture cards in Linux. Requires installation of the Datapath Vision driver.            |
| CAPTURE_BACKEND_VIRTUAL     | Provides a virtual capture device that generates a test image.                                                                |
| CAPTURE_BACKEND_DOSBOX_MMAP | Captures DOSBox's frame buffer. Requires [patching](./src/capture/dosbox_mmap/dosbox-0.74.3-linux-for-vcs-mmap.patch) DOSBox. |

# VCS

A control application for Datapath Vision capture cards on Linux.

![VCS 3.0](./screenshot1.png)

![VCS 3.0](./screenshot2.webp)

## Key features

- Unlimited video presets for analog capture
- Several scaling modes and image filters
- Variable refresh rate output
- Free and open source

## Supported capture hardware

Any model of Datapath capture card supported by the Datapath Vision driver for Linux should be compatible, although the card's full set of capabilities may not be exposed.

## Manuals

- [User's manual](https://www.tarpeeksihyvaesoft.com/vcs/docs/user/) ([source](./docs/user/))
- [Developer's manual](https://www.tarpeeksihyvaesoft.com/vcs/docs/developer/) ([source](./docs/developer/))

## Building

![](./cat1.jpg)

Open [vcs.pro](vcs.pro) in Qt Creator, or run `$ qmake && make -j` in the repo's root. You'll need to meet the [dependencies](#dependencies), and may need to modify some of the `INCLUDEPATH` entries in [vcs.pro](vcs.pro) to match your system.

### Dependencies

The VCS codebase depends on the following libraries and frameworks:

1. Qt
2. OpenCV*
3. If compiling for X11, libx11-dev
4. The Datapath capture API, as distributed with their Linux capture drivers**
5. Non-release builds*** are configured to use AddressSanitizer and UndefinedBehaviorSanitizer, which should come pre-installed with your compiler on common platforms. For these builds, you'll need to create a file called **asan-suppress.txt** in the directory of the VCS executable, and either leave it blank, or populate it according to [this guide](https://github.com/google/sanitizers/wiki/AddressSanitizerLeakSanitizer#suppressions) to have AddressSanitizer ignore leaks in external code (e.g. third-party libraries).

\* I use OpenCV 3.2.0, but later versions may also be compatible. See [this issue](https://github.com/leikareipa/vcs/issues/30) for details.\
\** Only needed if compiling for Datapath hardware, which is the default option. See [The capture backend](#the-capture-backend) for details.\
\*** See [Release build vs. debug build](#release-build-vs-debug-build).

I use the following toolchains when developing VCS:

| OS                 | Compiler           | Qt   | OpenCV | Capture card           |
| ------------------ | ------------------ | ---- | ------ | ---------------------- |
| Ubuntu 20.04 HWE   | GCC 9.4 (64-bit)   | 5.12 | 3.2.0  | Datapath VisionRGB-E1S |

### Release build vs. debug build

The default configuration in [vcs.pro](vcs.pro) produces a debug build, which has &ndash; among other things &ndash; more copious run-time bounds-checking of memory accesses. The run-time debugging features are expected to reduce performance to some extent, but can help reveal programming errors.

Defining `VCS_RELEASE_BUILD` globally will produce a release build, with fewer debugging checks in performance-critical sections of the program. Simply uncomment `DEFINES += VCS_RELEASE_BUILD` at the top of [vcs.pro](vcs.pro) and do a full rebuild.

To confirm whether the program is running in release or debug mode, navigate to Control panel &rarr; About VCS.

### The capture backend

A capture backend in VCS is an implementation providing support for a particular capture device. For example, Datapath's VisionRGB capture cards are supported in VCS on Linux by the [Vision capture backend](./src/capture/vision_v4l/).

One (and only one) capture backend must be active in any build of VCS. To select which one it is, set its identifier in the `DEFINES` variable in [vcs.pro](vcs.pro).

The following capture backends are available:

| Identifier                  | Explanation |
| --------------------------- | ----------- |
| CAPTURE_BACKEND_VISION_V4L  | Supports the Datapath VisionRGB range of capture cards on Linux. Requires [Datapath's Linux Vision driver](https://www.datapathsoftware.com/vision-capture-card-downloads/vision-drivers/vision-drivers-1) to be installed on the system (and re-installed whenever the kernel is updated). |
| CAPTURE_BACKEND_VIRTUAL     | Provides a device-independent capture source that generates a test image. Useful for debugging, and doesn't require a capture device or its drivers to be installed. |
| CAPTURE_BACKEND_DOSBOX_MMAP | Allows capturing DOSBox's frame buffer on Linux. Intended for debugging. See [this blog post](https://www.tarpeeksihyvaesoft.com/blog/capturing-dosboxs-frame-buffer-via-shared-memory/) for details.|

# VCS

A third-party capture tool for Datapath's VisionRGB range of capture cards. Greatly improves the hardware's suitability for capturing dynamic VGA signals (e.g. of retro PCs) compared to Datapath's bundled capture software.

VCS interfaces with compatible capture device to display the capture output in a window on your desktop. Additionally, you can apply filters, scalers, anti-tearing, and various other adjustments to the output before it's displayed.

![VCS 2.4](./images/vcs-2.4-with-dialogs.png)\
*A screenshot of VCS 2.4 showing the capture window (in the background) and some of the control dialogs.*

## Features

- Capture functionality tailored for the Datapath VisionRGB series of capture cards, and especially for the VisionRGB-PRO
- Run-time image filtering (blur, crop, decimate, denoise, etc.)
- Frame scaling with nearest, linear, area, cubic, and Lanczos sampling
- Host-side triple buffering to reconstruct torn frames
- On-screen display customizable with HTML/CSS
- Output in software and OpenGL (with variable refresh rate)
- Support for Windows (XP and later) and Linux (experimental)
- Virtual machine friendly: minimal reliance on GPU features

## Supported capture hardware

VCS is compatible with at least the following Datapath capture cards:
- VisionRGB-PRO1
- VisionRGB-PRO2
- VisionRGB-E1
- VisionRGB-E2
- VisionRGB-E1S
- VisionRGB-E2S
- VisionRGB-X2

The VisionAV range of cards should also work, albeit without their audio capture functionality.

## Manuals

- [Developer's manual](https://www.tarpeeksihyvaesoft.com/vcs/docs/developer-manual/)
- User's manual
    - [2.5.1](https://www.tarpeeksihyvaesoft.com/vcs/docs/user-manual/)
    - [2.5.0](https://www.tarpeeksihyvaesoft.com/vcs/docs/user-manual/2.5.0/)
    - [2.4.0](https://github.com/leikareipa/vcs/blob/v2.4.0/README.md#users-manual)

## Building

Run `$ qmake && make` in the repo's root; or open [vcs.pro](vcs.pro) in Qt Creator. (See also dependencies, below.)

I've been compiling VCS using GCC 5...9 on Linux and MinGW 5.3 on Windows. My Qt has been version 5.5...5.12 on Linux and 5.7 on Windows. Sticking with these tools should give you the least number of compatibility issues.

### Dependencies

The VCS codebase depends on the following external libraries and frameworks:

1. Qt
2. OpenCV
3. Datapath capture API (RGBEasy on Windows, Vision/Video4Linux on Linux)

It's possible to build VCS with just Qt and none of the other dependencies by modifying certain flags in [vcs.pro](vcs.pro) (see the sections below for details), although this will generally produce a very feature-stripped version of the program.

#### Qt

VCS uses [Qt](https://www.qt.io/) for its GUI and certain other functionality. Qt >= 5.7 or newer should satisfy the requirements.

Non-GUI code interacts with the GUI through a wrapper interface ([src/display/display.h](src/display/display.h), instantiated for Qt in [src/display/qt/d_main.cpp](src/display/qt/d_main.cpp)). If you wanted to implement the GUI with something other than Qt, you could do that by creating a new wrapper that implements this interface.

**Note:** There's currently some bleeding of Qt functionality into non-GUI regions of the codebase, which you would need to deal with also if you wanted to fully excise Qt.

#### OpenCV

VCS uses the [OpenCV](https://opencv.org/) 3.2.0 library for image filtering/scaling and video recording. For Windows, the binary distribution of VCS includes a pre-compiled DLL compatible with MinGW 5.3. For Linux, you can get the OpenCV 3.2.0 source code [here](https://github.com/opencv/opencv/tree/3.2.0) and follow the build instructions [here](https://docs.opencv.org/3.2.0/d7/d9f/tutorial_linux_install.html) (maybe also see [this](https://stackoverflow.com/questions/46884682/error-in-building-opencv-with-ffmpeg) in case of build errors).

The dependency on OpenCV can be removed by undefining `USE_OPENCV` in [vcs.pro](vcs.pro). If undefined, most forms of image filtering and scaling will be unavailable, video recording won't be possible, etc.

#### Datapath RGBEasy

On Windows, VCS uses the Datapath RGBEasy 1.0* API to interface with your VisionRGB capture device. The drivers for your Datapath capture card should include the required libraries, though you may need to adjust the paths to them in [vcs.pro](vcs.pro).

\*As distributed with the VisionRGB-PRO driver package v8.1.2.

To remove VCS's the dependency on RGBEasy, replace `CAPTURE_DEVICE_RGBEASY` with `CAPTURE_DEVICE_VIRTUAL` in [vcs.pro](vcs.pro). This will also disable capturing, but will let you run the program without the Datapath drivers/dependencies installed.

#### Datapath Vision

On Linux, VCS uses the Datapath Vision driver to interface with Datapath capture devices. You'll need to download and install the kernel module driver from Datapath's website (and re-install it every time your kernel updates).

To remove VCS's the dependency on the Datapath Vision driver, replace `CAPTURE_DEVICE_VISION_V4L` with `CAPTURE_DEVICE_VIRTUAL` in [vcs.pro](vcs.pro). This will also disable capturing, but will let you run the program without the drivers installed.

## Program flow

VCS is a mostly single-threaded application whose event loop is synchronized to the capture device's rate of output. In general, VCS's main loop polls the capture device until a capture event (e.g. new frame) occurs, then processes the event, and returns to the polling loop.

![](./images/diagrams/code-flow.png)

 The above diagram shows a general step-by-step view of VCS's program flow. `Capture` communicates with the capture device (which will typically be running in its own thread), receiving frame data and setting hardware-side capture parameters. `Main` polls `Capture` to receive information about capture events. When it receives a new frame from `Capture`, `Main` sends the frame's data to `Scale` for scaling, which then forwards it to `Filter` for image filtering, which in turn sends it to `Record` to be appended into a video file (if video recording is enabled) and then to `Display` to be rendered onto VCS's output window.

| System  | Corresponding source code          |
| ------- | ---------------------------------- |
| Main    | [src/main.cpp](./src/main.cpp)     |
| Capture | [src/capture/*](./src/capture/)    |
| Scale   | [src/scaler/*](./src/scaler/)      |
| Filter  | [src/filter/*](./src/filter/)      |
| Record  | [src/record/*](./src/record/)      |
| Display | [src/display/*](./src/display/)    |

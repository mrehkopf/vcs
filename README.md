# VCS
A third-party capture tool for Datapath's VisionRGB range of capture cards. Greatly improves the hardware's suitability for capturing dynamic VGA signals (e.g. of retro PCs) compared to Datapath's bundled capture software.

VCS interfaces with compatible capture hardware to display the capture output in a window on your desktop. Additionally, you can apply filters, scalers, anti-tearing, and various other adjustments to the output before it's displayed. A more complete list of VCS's features is given below.

You can find the pre-built binary distribution of VCS for Windows on [Tarpeeksi Hyvae Soft's website](https://www.tarpeeksihyvaesoft.com/soft/).

### Features
- Real-time customizable image filters: blur, crop, decimate, denoise, and more
- Scaling with nearest, linear, area, cubic, and Lanczos sampling
- Active triple buffering to reconstruct torn frames
- Custom overlays with HTML/CSS formatting
- Video recording
- Virtual machine friendly (low reliance on GPU features)
- Output in software or OpenGL
- Supports Windows (XP and later), Linux

### Hardware support
VCS is compatible with at least the following Datapath capture cards:
- VisionRGB-PRO1
- VisionRGB-PRO2
- VisionRGB-E1
- VisionRGB-E2
- VisionRGB-E1S
- VisionRGB-E2S
- VisionRGB-X2

The VisionAV range of cards should also work, albeit without their audio capture functionality.

# User's manual
Contents:
- [Setting up](#setting-up)
- [Output window](#output-window)
    - Interacting with the output window
        - Magnifying glass
        - Dragging
        - Borderless mode
        - Fullscreen mode
        - Resizing
        - Scaling with the mouse wheel
        - Title bar
        - Menu bar
- [Dialogs](#dialogs)
    - Video & color dialog
    - Alias resolutions dialog
    - Record dialog
    - Input resolution dialog
    - Output resolution dialog
    - Anti-tear dialog
    - Filter graph dialog
- [Mouse and keyboard shortcuts](#mouse-and-keyboard-shortcuts)
- [Command-line arguments](#command-line-arguments)

## Setting up
Assuming you've installed the drivers for your capture hardware, as well as unpacked the binary distribution of VCS (linked to, above) into a folder, getting VCS going is simply a matter of running its `vcs.exe` executable.

When you run the executable, two windows will open: a console window, in which notifications about VCS's status will appear during operation; and the [output window](#output-window), in which captured frames are displayed.

- Note: You can launch `vcs.exe` with command-line parameters to automate certain start-up tasks. A list of the command-line options is given in the [command-line](#command-line-arguments) section.

When running VCS for the first time, the first thing you may want to do is adjust the capture video parameters, like phase, color balance, and so on. These can be set up via the [video & color dialog](#video-&-color-dialog).

## Output window
The central point of the VCS user interface is the output window, where captured frames are displayed as they arrive from the capture hardware.

![](images/screenshots/v2.0.0/output-window.png)\
*The output window - showing a Windows 98 desktop being captured on Linux.*

### Interacting with the output window
#### Magnifying glass
If you press and hold the right mouse button over the output window, the portion of the image under the cursor will be magnified.

- Note: The magnifying glass is not available with the OpenGL renderer.

#### Dragging
You can drag the output window by left-clicking anywhere on the window.

#### Borderless mode
You can double-click inside the output window to toggle its border on/off.

- Note: When the border is toggled off, the window will also snap to the top left corner of the screen.

#### Fullscreen mode
Although you can emulate a fullscreen mode by turning off the output window's border and scaling the window to the size of the the display area, there is also a true fullscreen mode available. You can toggle it with the F11 shortcut key.

For the fullscreen mode to work optimally, you may first need to resize the output window to match the resolution of your screen (see the [output resolution dialog](#output-resolution-dialog)).

#### Resizing
The output window cannot be resized directly. Instead, you can use the [output resolution dialog](#output-resolution-dialog) to adjust the window's size.

#### Scaling with the mouse wheel
By scrolling the mouse wheel over the output window, you can scale the size of the window up and down.

- Note: Mouse wheel scaling is not available while recording video.

This is a shortcut for the `relative scale` setting in the [output resolution dialog](#output-resolution-dialog).

#### Title bar
The output window's title bar displays information about VCS's current state, like the input and output resolutions.

The title bar's text may contain the following elements, in order of appearance from left to right:
- `{!}`: Shown if captured frames are being dropped (e.g. due to VCS lacking sufficient CPU power to keep up with the capture hardware's output rate).
- `VCS`: The program's name!
- `R` `F` `O` `A`: Indicators for whether certain functionality is enabled. Not shown if the corresponding functionality is disabled.
    - `R` &rarr; Recording to video (see [Record dialog](#record-dialog)).
    - `F` &rarr; Filtering enabled (see [Filter graph dialog](#filter-graph-dialog)).
    - `O` &rarr; Overlay enabled (see [Overlay dialog](#overlay-dialog)).
    - `A` &rarr; Anti-tearing enabled (see [Anti-tear dialog](#anti-tear-dialog)).
- `640 x 480`: The current input (capture) resolution.
- `scaled to 704 x 528`: The current output resolution.
- `(~110%)`: An estimate of the percentage by which the input resolution has been scaled for output.

#### Menu bar
The output window contains an auto-hiding menu bar, from which you can access the various controls and dialogs of VCS.

The menu bar will be displayed when you hover the mouse cursor over the output window; and hidden after a few seconds of mouse inactivity.

The menu bar is divided into four categories: `File`, `Input`, `Output`, and `About`. The following is a list of the options available.

##### Menu bar: File
`File` &rarr; `Exit`\
Exit VCS.

##### Menu bar: Window
`Window` &rarr; `Renderer`\
Set the type of rendering VCS uses to draw captured frames onto the [output window](#output-window).

- &rarr; `Software`: Most compatible option. Rendering is done on the CPU.
- &rarr; `OpenGL`: Rendering is done on the GPU using OpenGL.

When choosing the renderer, be mindful of the following:
- The OpenGL renderer might not work in Windows XP.
- The output window's magnifying glass feature is not available with the OpenGL renderer.

`Window` &rarr; `Position`\
Snap the [output window](#output-window) into a pre-set position on the screen.

`Window` &rarr; `Custom title...`\
Give the [output window](#output-window) a custom title. All of VCS's normal title bar information - except the frame drop indicator - will be replaced with the text you set. You could, for instance, set the title to "Tomb Raider" if you're capturing footage of Tomb Raider. Setting an empty text will restore VCS's default title bar.

`Window` &rarr; `Show border`\
Set whether the [output window](#output-window) should be drawn with a border.

`Window` &rarr; `Fullscreen`\
Place the [output window](#output-window) into fullscreen mode. For best results, you should first scale the window to the resolution of your screen before making it fullscreen.

Note: The fullscreen feature may not work properly on certain distributions of Linux.

##### Menu bar: Input
`Input` &rarr; `Channel`\
Set the hardware capture channel. Depending on the capabilities of your capture hardware, channels #1 through #2 are available.

`Input` &rarr; `Color depth`\
Set the color depth with which frames are captured. This is a hardware-level setting: the capture hardware will convert each frame to this color depth before uploading it to system memory - thus lower color depths consume less bandwidth. Prior to display, VCS will convert the frames to the color depth of the [output window](#output-window).
 
`Input` &rarr; `Video...`\
Open the [video & color](#video-&-color-dialog) dialog.

`Input` &rarr; `Aliases...`\
Open the [alias resolutions](#alias-resolutions-dialog) dialog.

`Input` &rarr; `Resolution...`\
Open the [input resolution](#input-resolution-dialog) dialog.

##### Menu bar: Output
`Output` &rarr; `Aspect ratio`\
Set the aspect ratio to display captured frames in. Letterboxing will be used to achieve the desired ratio.

- &rarr; `Native`: Display frames in the full size of the [output window](#output-window), without letterboxing.
- &rarr; `Traditional 4:3`: Use 4:3 aspect ratio for resolutions that historically might have been meant to be displayed as such. These include 720 x 400, 640 x 400, and 320 x 200.
- &rarr; `Always 4:3`: Display all frames in 4:3 aspect ratio.

`Output` &rarr; `Upscaler`\
Set the scaler to be used when frames are upscaled to fit the [output window](#output-window).

`Output` &rarr; `Downscaler`\
Set the scaler to be used when frames are downscaled to fit the [output window](#output-window).

`Output` &rarr; `Record...`\
Open the [record](#record-dialog) dialog.

`Output` &rarr; `Overlay...`\
Open the [overlay](#overlay-dialog) dialog.

`Output` &rarr; `Anti-tear...`\
Open the [anti-tear](#anti-tear-dialog) dialog.

`Output` &rarr; `Resolution...`\
Open the [output resolution](#output-resolution-dialog) dialog.

`Output` &rarr; `Filter graph`\
Open the [filter graph](#filter-graph-dialog) dialog.

##### Menu bar: Help
`Help` &rarr; `About...`\
Display information about VCS and the capture hardware.

## Dialogs
The VCS user interface includes a number of dialogs, with which you can adjust the program's operational parameters.

Contents:
- [Video presets dialog](#video-presets-dialog)
- [Alias resolutions dialog](#alias-resolutions-dialog)
- [Record dialog](#record-dialog)
- [Input resolution dialog](#input-resolution-dialog)
- [Output resolution dialog](#output-resolution-dialog)
- [Anti-tear dialog](#anti-tear-dialog)
- [Filter graph dialog](#filter-graph-dialog)

### Video presets dialog
To access: Ctrl+V or [Menu bar](#menu-bar) &rarr; `Input` &rarr; `Video...`

The video presets dialog lets you to modify the capture hardware's video signal parameters.

![](images/screenshots/v2.0.0/video-presets-dialog.png)\
*The video presets dialog - showing controls for adjusting the capture hardware's video signal parameters.*

The parameter values you input will take effect when all of the "Activates when" conditions are met. For instance, if you've set the preset's activation resolution to 800 x 600 and the refresh rate to 59.695 Hz, the video parameters will apply when the capture resolution is 800 x 600 and the capture refresh rate is 59.695 Hz. Or, for instance, you might only assign the preset with a shortcut activation, in which case its parameters will be applied when you activate the shortcut key, but not otherwise.

To add or delete presets, select the corresponding option from the dialog's "Video presets" menu.

If you want your settings to persist after you exit VCS, remember to save them first! This can be done via `File` &rarr; `Save settings as...`.

Saved settings can be restored via `File` &rarr; `Load settings...`.

### Alias resolutions dialog
To access: [Menu bar](#menu-bar) &rarr; `Input` &rarr; `Aliases...`

With the alias resolutions dialog, you can tell VCS to automatically override certain input resolutions.

![](images/screenshots/v2.0.0/alias-dialog.png)\
*The alias resolutions dialog - showing controls for creating and managing aliases.*

For instance, if you find that your capture hardware is incorrectly initializing 640 x 400 as 512 x 488 (or something of that sort), you can define 640 x 400 as an alias of 512 x 488. Now, whenever the capture hardware sets the input resolution to 512 x 488, VCS will instruct it to change it to 640 x 400.

### Record dialog
To access: Ctrl+R or [Menu bar](#menu-bar) &rarr; `Output` &rarr; `Record...`

The record dialog gives you the option to stream captured frames into a video file.

![](images/screenshots/v2.0.0/record-dialog.png)\
*The record dialog - showing controls for recording captured frames into a video file.*

The recording functionality will write frames as they appear in the [output window](#output-window) into a video file. But make note of the following:

- Audio will not be recorded.
- The video will be recorded in the H.264 format using an x264 codec.
- The video resolution will be that of the current output size (see the [output resolution dialog](#output-resolution-dialog)).
- The output size cannot be changed while recording; frames will be scaled to fit the current size.
- The [overlay](#overlay-dialog) will not be recorded.
- Encoder parameters influencing image quality (e.g. CRF) cannot be altered in the Linux version of VCS - this is a limitation of OpenCV. You can, however, modify and recompile the OpenCV code with higher-quality default options (see e.g. [here](https://www.researchgate.net/post/Is_it_possible_to_set_the_lossfree_option_for_the_X264_codec_in_OpenCV)).

To make use of VCS's recording functionality on Windows, you will need to install the 32-bit version of the [x264vfw](https://sourceforge.net/projects/x264vfw/files/x264vfw/44_2851bm_44825/) codec and run its configurator at least once, so that its settings are added into the Windows registry for VCS to find.

#### Recording settings
**Frame rate.** The video's nominal playback rate. Typically, you will want to match this to the capture source's refresh rate, so that e.g. a 60 Hz capture signal is recorded with a frame rate of 60.

**Linear sampling.** Whether VCS is allowed to duplicate and/or skip frames to match the captured frame rate with the video's nominal playback rate. If linear sampling is disabled, captured frames will be inserted into the video as they are received, and are never duplicated or skipped to maintain time-coherency. Disabling linear sampling may result in smoother-looking playback when the capture frame rate is uneven; but enabling it will help prevent time compression in these cases. If you are planning to append the video with an audio track you recorded at the same time, you will most likely want to enable linear sampling or the video may not keep in sync with the audio.

- Note: While the capture hardware reports 'no signal', no frames will be recorded, regardless of whether linear sampling is enabled.

**Video container.** The file format in which the video is saved. On Windows, the AVI format is used.

**Video codec.** The encoder with which to create the video. On Windows, the 32-bit version of x264vfw is used.

**Additional x264 arguments.** You can provide the encoder with custom command-line parameters via this field.

For best image quality regardless of performance and/or file size, set `profile` to "High 4:4:4", `pixel format` to "RGB", `CRF` to 1, and `preset` to "ultrafast". To maintain high image quality but reduce the file size, you can set `preset` to "veryfast" or "faster", and increase `CRF` to 10&ndash;15. For more tips and tricks, you can look up documentation specific to the x264 encoder.

### Input resolution dialog
To access: Ctrl+I or [Menu bar](#menu-bar) &rarr; `Input` &rarr; `Resolution...`

The input resolution dialog lets you override the capture hardware's current capture resolution.

![](images/screenshots/v1.6.2/input-resolution-dialog.png)\
*The input resolution dialog - showing controls for adjusting the capture resolution.*

Normally, the capture hardware will automatically set the capture resolution to match that of the input signal. But sometimes the result is sub-optimal, and you may want to manually override it.

The dialog's buttons will tell the capture hardware to set a particular input resolution regardless of what the hardware thinks is the correct resolution for the input signal.

- Note: The `other...` button lets you specify a custom resolution, in case the pre-set ones are not suitable.

You can change a button's assigned resolution by clicking on it while holding down the Alt key.

### Output resolution dialog
To access: Ctrl+O or [Menu bar](#menu-bar) &rarr; `Output` &rarr; `Resolution...`

The output resolution dialog lets you resize the output window. This also resizes the frames being displayed in the window.

![](images/screenshots/v1.6.2/output-resolution-dialog.png)\
*The output resolution dialog - showing controls for adjusting the size of the output window.*

Normally, the size of the [output window](#output-window) will match the capture resolution, but you can use this dialog to scale the window up or down.

- Note: The output resolution controls are not available while recording video (see the [record dialog](#record-dialog)).

#### Settings

**Override.** Lock the size of the output window, so that changes to the capture resolution do not resize it. Frames will be scaled up or down as needed to match this resolution.

**Relative scale.** Scale the size of the output window up or down by a percentage of its base size. The base size is either the capture resolution, or, if enabled, the override resolution.

### Overlay dialog
To access: Ctrl+L or [Menu bar](#menu-bar) &rarr; `Output` &rarr; `Overlay...`

The overlay dialog lets you define a message to be overlaid on the [output window](#output-window).

![](images/screenshots/v2.0.0/overlay-dialog.png)\
*The overlay dialog - showing controls for overlaying a message on the output window.*

You can combine normal text with pre-set VCS variables and HTML/CSS formatting to create a message to be shown over the output window.

- Note: The overlay will not be included in videos recorded using VCS's built-in recording functionality (see the [record dialog](#record-dialog)).

### Anti-tear dialog
To access: Ctrl+A or [Menu bar](#menu-bar) &rarr; `Output` &rarr; `Anti-tear...`

The anti-tear dialog provides functionality to remove tearing from captured frames.

![](images/screenshots/v2.0.0/anti-tear-dialog.png)\
*The anti-tear dialog - showing controls for adjusting the parameters of VCS's anti-tear engine.*

Under some circumstances, such as when the captured source does not sync its rendering with the refresh rate, you may find that the captured frames contain horizontal tearing. VCS comes with an anti-tear engine to help mitigate this problem.

- Note: The anti-tearing is not 100% perfect, in that not absolutely all tears are eliminated. But it should provide a considerable improvement nonetheless.

Anti-tearing can be considered an experimental feature of VCS. It works quite well in many cases, but can fail in others, and may be a performance hog. The default settings should work well enough in most cases, although you will likely need to adjust the range offsets (see below).

#### Settings

**Range offsets.** Set the vertical range inside which the anti-tearing operates. Static content, like a game's UI bar at the top or bottom of the screen, can prevent the anti-tearing from working, and you should set this range so as to exclude such content. You can enable `visualization` to see the range represented with horizontal lines in the [output window](#output-window).

**Visualization.** Draw certain anti-tearing-related markers in the output window.

**Threshold.** Set the maximum amount by which pixel color values are allowed to change between two frames without being considered new data. The less noise there is in the capture, the lower you can set this value.

**Domain width.** Set the size of the sampling window. A lower value reduces CPU usage, but may be less able to detect subtle tearing.

**Step size.** Set the number of pixels by which to slide the sampling window at a time. A higher value reduces CPU usage, but may be less able to detect tearing.

**Matches req'd.** Set how many times on a row of pixels the sums of the sampling window need to exceed the threshold for that row of pixels to be considered new data.

### Filter graph dialog
To access: Ctrl+F or [Menu bar](#menu-bar) &rarr; `Output` &rarr; `Filter graph...`

The filter graph dialog allows you to create chains of image filters to be applied to captured frames prior to display in the [output window](#output-window).

![](images/screenshots/v2.0.0/filter-graph-dialog.png)\
*The filter graph dialog - showing controls for creating and modifying filter chains.*

The filter graph is made up of nodes that can be connected together in a chain. These nodes come in three varieties: `input gate`, `output gate`, and `filter`.

The input and output gates determine the resolutions for which the connected filters will be applied. For instance, if you set an input gate's width and height to 640 and 480, and the width and height of an output gate to 1920 and 1080, any filters you connect between these two nodes will be applied when the size of the output window is 1920 x 1080 and the original resolution of the frames (i.e. the capture resolution) is 640 x 480. You can also use the value 0 for a gate's width and/or height to allow VCS to match any value to that dimension: an input gate with a width and height of 0, for instance, will apply the connected filters to frames of all capture resolutions, provided that they also meet the resolution specified for the output gate. A filter graph can have multiple chains of these input-filter-output combos, and VCS will select the most suitable one (or none) given the current capture and output resolutions.

- Note: When deciding which of multiple filter chains to use, VCS will prefer more specific chains to more general ones. If you have e.g. an input gate whose width and height are 0, and another input gate whose width and height are 640 and 480, the latter will be used when the capture resolution is exactly 640 x 480, and the former otherwise. Likewise, if your input gates are 0 x 0 and 640 x 0, the former will be applied for capture resolutions of *any* x *any*, except for 640 x *any*, where the latter chain will apply - except if you also have a third input gate of 640 x 480, in which case that will be used when the capture resolution is exactly 640 x 480.

To connect two nodes, click and drag with the left mouse button from one node's output edge (square) to another's input edge (circle), or vice versa. A node can be connected to as many other nodes as you like. To disconnect a node from another, right-click on the node's output edge, and select the other node from the list that pops up. To remove a node itself from the graph, right-click on the node and select to remove it. To add nodes to the graph, select `Add` from the dialog's menu bar.

## Mouse and keyboard shortcuts
You can make use of the following mouse and keyboard shortcuts:
```
Double-click
VCS's output window ..... Toggle window border on/off.

Middle-click
output window ........... Open the overlay editor.

Left-press and drag
output window ........... Move the window (same as dragging by its title bar).

Right-press
output window ........... Magnify this portion of the output window.

Mouse wheel
output window ........... Scale the output window up/down.

F11 ..................... Toggle fullscreen mode on/off.

Ctrl + A ................ Open the anti-tear dialog.

Ctrl + F ................ Open the filter graph dialog.

Ctrl + V ................ Open the video settings dialog.

Ctrl + I ................ Open the input resolution dialog.

Ctrl + O ................ Open the output resolution dialog.

Ctrl + R ................ Open the record dialog.

Ctrl + L ................ Open the overlay dialog.

Ctrl + Shift + key ...... Toggle the corresponding dialog's functionality on/off;
                          e.g. Ctrl + Shift + R to turn recording on/off.

Ctrl + 1 to 9 ........... Shortcuts for the input resolution buttons on the
                          control panel's Input tab.
```

## Command-line arguments
Optionally, you can pass one or more of following command-line arguments to VCS:
```
-v <path + filename> .... Load video presets from the given file on start-up.
                          Video preset files typically have the .vcs-video
                          suffix.

-f <path + filename> .... Load a custom filter graph from the given file on
                          start-up. Filter graph files typically have the .vcs-
                          filter-graph suffix.

-a <path + filename> .... Load alias resolutions from the given file on start-
                          up. Alias resolution files typically have the .vcs-
                          alias suffix.

-i <input channel> ...... Start capture on the given input channel (1...n). By
                          default, channel #1 will be used.
```

For instance, if you had capture parameters stored in the file `params.vcsm`, and you wanted capture to start on input channel #2 when you run VCS, you might launch VCS like so:
```
vcs.exe -m "params.vcsm" -i 2
```

# Developer's manual

## Building
**On Linux:** Do `qmake && make` at the repo's root, or open [vcs.pro](vcs.pro) in Qt Creator.

**On Windows:** Same as for Linux.

While developing VCS, I've been compiling it with GCC 5-9 on Linux and MinGW 5.3 on Windows, and my Qt has been version 5.5-5.9 on Linux and 5.7 on Windows. If you're building VCS, sticking with these tools should guarantee the least number of compatibility issues.

### Build dependencies
**Qt.** VCS uses [Qt](https://www.qt.io/) for its GUI and certain other functionality. Qt of version 5.5 or newer should satisfy VCS's requirements. The binary distribution of VCS for Windows includes the required DLLs.
- Non-GUI code interacts with the GUI through a wrapper interface ([src/display/display.h](src/display/display.h), instantiated for Qt in [src/display/qt/d_main.cpp](src/display/qt/d_main.cpp)). If you wanted to implement the GUI with something other than Qt, you could do so by creating a new wrapper that implements this interface.
    - There is, however, currently some bleeding of Qt functionality into non-GUI regions of the codebase, which you would need to deal with also if you wanted to fully excise Qt. Namely, in the units [src/record/record.cpp](src/record/record.cpp), [src/common/disk.cpp](src/common/disk.cpp), and [src/common/csv.h](src/common/csv.h).

**OpenCV.** VCS makes use of the [OpenCV](https://opencv.org/) 3.2.0 library for image filtering and scaling, and for video recording. The binary distribution of VCS for Windows includes a pre-compiled DLL of OpenCV 3.2.0 compatible with MinGW 5.3.
- The dependency on OpenCV can be removed by undefining `USE_OPENCV` in [vcs.pro](vcs.pro). If undefined, most forms of image filtering and scaling will be unavailable, and video recording will not be possible.

**RGBEasy.** On Windows, VCS uses Datapath's RGBEasy API to interface with the capture hardware. The drivers for your Datapath capture card should include and have installed the required libraries, though you may need to adjust the paths to them in [vcs.pro](vcs.pro).
- If you want to remove VCS's the dependency on RGBEasy, replace `CAPTURE_API_RGBEASY` with `CAPTURE_API_VIRTUAL` in [vcs.pro](vcs.pro). This will also disable capturing, but will let you run the program without the Datapath drivers installed.

**Video4Linux.** On Linux, VCS uses Datapath's Video4Linux driver to interface with the capture hardware. For this to work, you should install the latest Datapath Vision Linux driver (may not support kernel 5+).
- If you want to remove VCS's the dependency on the Video4Linux driver, replace `CAPTURE_API_VIDEO4LINUX` with `CAPTURE_API_VIRTUAL` in [vcs.pro](vcs.pro). This will also disable capturing, but will let you run the program without the Datapath drivers installed.

## Code organization
VCS is largely a single-threaded application whose event loop is synchronized to the capture devices's rate of operation. VCS's main loop polls the capture hardware (which may run in a separate thread) until a capture event (e.g. new frame) occurs, then processes the event, and returns to the polling loop.

```
.--> MAIN <--> CAPTURE <-- [Capture device]
|     |
|     v
|    SCALE
|     |
|     v 
|    FILTER
|     |
|     +--> RECORD
|     |
|     v
`--- DISPLAY
```

In the above diagram, MAIN polls CAPTURE, which returns information to MAIN about capture events. When it receives a new frame from CAPTURE, MAIN sends the frame data to SCALE for scaling, to FILTER for image filtering, and finally to DISPLAY for the scaled and filtered frame be rendered on screen - and optionally to RECORD for recording the frame into a video file.

The modules marked in uppercase in the above diagram correspond to source code files roughly like so:

| Module  | Source                             |
| ------- | ---------------------------------- |
| Main    | [src/main.cpp](src/main.cpp)       |
| Capture | [src/capture/](src/capture/)       |
| Scale   | [src/scaler/](src/scaler/)         |
| Filter  | [src/filter/](src/filter/)         |
| Record  | [src/record/](src/record/)         |
| Display | [src/display/](src/display/)       |

## Guide: Adding a frame filter

Frame filters modify captured frames' pixel data before the frames are displayed on-screen. With these filters, the user can add real-time effects like blur, crop, or rotation - or something more exotic like a frame rate counter, which keeps track of pixel changes across frames to count how many unique frames per second are being captured.

Coding a custom filter for VCS involves three steps: (1) [creating a function that applies the filter to incoming frames' pixels](#creating-the-filter-function), (2) [defining a GUI widget with which the user can interact with the filter](#defining-the-filters-gui-widget), and (3) [making VCS aware of the filter's existence](#making-vcs-aware-of-the-filter). The steps are described in more detail, below.

### Creating the filter function
Each filter must have a function that applies the filter's processing (blurring, sharpening, or whatever you have in mind) to captured pixel data. These functions are defined in [src/filter/filter_funcs.cpp](src/filter/filter_funcs.cpp) and declared in [src/filter/filter_funcs.h](src/filter/filter_funcs.h).

For example, a simple filter function - let's call it `filter_func_solid_fill()` - that makes all pixels in the frame a certain color could be defined in [src/filter/filter_funcs.cpp](src/filter/filter_funcs.cpp) like so:

```cpp
void filter_func_solid_fill(FILTER_FUNC_PARAMS)
{
    VALIDATE_FILTER_INPUT

    const u8 red = params[filter_widget_solid_fill_s::OFFS_RED];
    const u8 green = params[filter_widget_solid_fill_s::OFFS_GREEN];
    const u8 blue = params[filter_widget_solid_fill_s::OFFS_BLUE];

    #if USE_OPENCV
        cv::Mat output = cv::Mat(r->h, r->w, CV_8UC4, pixels);
        output = cv::Scalar(blue, green, red, 255);
    #else
        for (unsigned y = 0; y < r->h; y++)
        {
            for (unsigned x = 0; x < r->w; x++)
            {
                const unsigned pixelIdx = ((x + y * r->w) * 4);

                pixels[pixelIdx + 0] = blue;
                pixels[pixelIdx + 1] = green;
                pixels[pixelIdx + 2] = red;
                pixels[pixelIdx + 3] = 255;
            }
        }
    #endif

    return;
}
```

The function's `FILTER_FUNC_PARAMS` parameter list is a #define which expands to:

```cpp
u8 *const pixels,
const resolution_s *const r,
const u8 *const params
```
Here, `pixels` is a one-dimensional array of 32-bit color values (8 bits each for BGRA), `params` is an array providing the filter's parameter values (e.g. radius for a blur filter), and `r` is the resolution of the pixel buffer represented by `pixels`.

What `filter_func_solid_fill()` is doing, then, is iterating over all of the frame's pixels to modify their individual color values. It does this using OpenCV if VCS has been compiled with OpenCV support; and via a normal loop otherwise. You don't have to use OpenCV in a filter function, but if you do, encase any OpenCV methods, parameters, and such in an `#if USE_OPENCV` preprocessor conditional, so that the code compiles even when OpenCV is disabled.

Don't forget to declare the filter function in [src/filter/filter_funcs.h](src/filter/filter_funcs.h):

```cpp
void filter_func_solid_fill(FILTER_FUNC_PARAMS);
```

If you're wondering where the magic `filter_widget_solid_fill_s::` values in `filter_func_solid_fill()` come from, read on! (Hint: they come from the filter's GUI widget and so are ultimately set by the user.)

### Defining the filter's GUI widget
Each filter must be accompanied by a GUI widget that provides the user a way to interact with the widget in the [Filter graph dialog](#filter-graph-dialog).

Filter widgets are defined in [src/display/qt/widgets/filter_widgets.cpp](src/display/qt/widgets/filter_widgets.cpp) and declared in [src/display/qt/widgets/filter_widgets.h](src/display/qt/widgets/filter_widgets.h). Each widget is a struct subclassing `filter_widget_s`.

The following is what might be the corresponding widget definition for our sample `filter_func_solid_fill()` filter function. The widget provides the user with controls to define which color the filter function should fill the frame's pixels with.

```cpp
struct filter_widget_solid_fill_s : public filter_widget_s
{
    // Byte offsets in the filter's parameter array of the individual
    // parameters. Each parameter in this filter takes up one byte; those
    // parameters being the fill color's red, green, and blue values.
    // The red value is thus in this->parameterArray[0], the green value
    // in this->parameterArray[1], etc.
    enum data_offset_e { OFFS_RED = 0, OFFS_GREEN = 1, OFFS_BLUE = 2 };

    filter_widget_solid_fill_s(u8 *const parameterArray,
                               const u8 *const initialParameterValues) :
        filter_widget_s(filter_type_enum_e::solid_fill,
                        parameterArray,
                        initialParameterValues)
    {
        if (!initialParameterValues) this->reset_parameter_data();
        create_widget();
        return;
    }

    // Give the filter's parameters default starting values.
    void reset_parameter_data(void) override
    {
        memset(this->parameterArray, 0, FILTER_PARAMETER_ARRAY_LENGTH);

        // Default to a light blue color.
        this->parameterArray[OFFS_RED] = 0;
        this->parameterArray[OFFS_GREEN] = 150;
        this->parameterArray[OFFS_BLUE] = 255;

        return;
    }

private:
    Q_OBJECT
    
    void create_widget(void) override
    {
        // Create a frame that will group together the widget's
        // contents.
        QFrame *frame = new QFrame();
        frame->setMinimumWidth(this->minWidth);

        // Create a set of UI controls for the user to adjust the
        // filter's parameters.
        QLabel *labelRed = new QLabel("Red:", frame);
        QSpinBox *spinRed = new QSpinBox(frame);
        spinRed->setRange(0, 255);
        spinRed->setValue(this->parameterArray[OFFS_RED]);

        QLabel *labelGreen = new QLabel("Green:", frame);
        QSpinBox *spinGreen = new QSpinBox(frame);
        spinGreen->setRange(0, 255);
        spinGreen->setValue(this->parameterArray[OFFS_GREEN]);

        QLabel *labelBlue = new QLabel("Blue:", frame);
        QSpinBox *spinBlue = new QSpinBox(frame);
        spinBlue->setRange(0, 255);
        spinBlue->setValue(this->parameterArray[OFFS_BLUE]);

        QFormLayout *l = new QFormLayout(frame);
        l->addRow(labelRed, spinRed);
        l->addRow(labelGreen, spinGreen);
        l->addRow(labelBlue, spinBlue);

        // Make the filter's parameter values update as the user
        // operates the controls. Changes will be reflected in the
        // captured frames in real-time.
        {
            connect(spinRed, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
            [this](const int newValue)
            {
                this->parameterArray[OFFS_RED] = newValue;
            });

            connect(spinGreen, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
            [this](const int newValue)
            {
                this->parameterArray[OFFS_GREEN] = newValue;
            });

            connect(spinBlue, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
            [this](const int newValue)
            {
                this->parameterArray[OFFS_BLUE] = newValue;
            });
        }

        frame->adjustSize();
        this->widget = frame;

        return;
    }
};
```

You can look around [src/display/qt/widgets/filter_widgets.h](src/display/qt/widgets/filter_widgets.h) for more examples of how VCS's filters are implementing their GUI widgets.

### Making VCS aware of the filter
Once the filter function and GUI widget have been created, all that's left to do is to let VCS know that the filter exists - which will also make the filter available to the user in the [Filter graph dialog](#filter-graph-dialog).

To make VCS recognize our sample solid fill filter, we'll need to append [src/filter/filter.cpp](src/filter/filter.cpp) and [src/filter/filter.h](src/filter/filter.h) with the following:

```cpp
// In filter.h:

// Find the filter_type_enum_e enumerator, and add an entry
// for this filter (we'll call it 'solid_fill').
enum class filter_type_enum_e
{
    ...
    solid_fill
};
```

```cpp
// In filter.cpp:

// Find the KNOWN_FILTER_TYPES map, and add the filter there. Note: you'll
// need to generate a unique UUID (e.g. 03847778-bb9c-4e8c-96d5-0c10335c4f34)
// for the filter.
static const std::unordered_map ... KNOWN_FILTER_TYPES =
{
    ...
    {"UUID", {"Solid fill", filter_type_enum_e::solid_fill, filter_func_solid_fill}}
};

// Find the filter_c::create_gui_widget() function, and connect
// the filter function to its filter widget.
filter_widget_s *filter_c::create_gui_widget(...)
{
    ...
    switch (this->metaData.type)
    {
        case filter_type_enum_e::solid_fill: return new filter_widget_solid_fill_s(arguments);
    }
    ...
}
```

Now if you compile and run VCS, the "Solid fill" filter should be available as an option in the [Filter graph dialog](#filter-graph-dialog)!

# Project status
VCS is currently in post-1.0, having come out of beta in 2018. Development is sporadic.

### System requirements
You are encouraged to have a fast CPU, since most of VCS's operations are performed on the CPU. The GPU is of less importance, and even fairly old ones will likely work. VCS uses roughly 1 GB of RAM, and so your system should have at least that much free &ndash; preferably twice as much or more.

**Performance.** On my Intel Xeon E3-1230 v3, VCS performs more than adequately. The table below shows that an input of 640 x 480 can be scaled to 1920 x 1440 at about 300&ndash;400 frames per second, depending on the interpolation used.

| 640 x 480    | Nearest | Linear | Area | Cubic | Lanczos |
|:------------ |:-------:|:------:|:----:|:-----:|:-------:|
| 2x upscaled  | 1100    | 480    | 480  | 280   | 100     |
| 3x upscaled  | 460     | 340    | 340  | 180   | 50      |

Drawing frames onto the [output window](#output-window) using software rendering is likewise sufficiently fast, as shown in the following table. An input of 640 x 480 can be upscaled by 2x and drawn on screen at roughly 340 frames per second when using nearest-neighbor interpolation.

| 640 x 480       | 1x<br>Nearest | 2x<br>Nearest | 3x<br>Nearest |
|:--------------- |:-------------:|:-------------:|:-------------:|
| With display    | 1360          | 340           | 150           |
| Without display | 1910          | 1100          | 510           |

Padding (i.e. aspect ratio correction) can incur a performance penalty with some of the scalers. The following table shows the frame rates associated with scaling a 640 x 480 input into 1920 x 1080 with and without padding to 4:3.

| 480p to 1080p | Nearest | Linear | Area | Cubic | Lanczos |
|:------------- |:-------:|:------:|:----:|:-----:|:-------:|
| Padded / 4:3  | 390     | 270    | 270  | 200   | 80      |
| No padding    | 820     | 370    | 370  | 210   | 70      |

# Authors and credits
The primary author of VCS is the one-man Tarpeeksi Hyvae Soft (see on [GitHub](https://github.com/leikareipa) and the [Web](https://www.tarpeeksihyvaesoft.com)).

VCS uses [Qt](https://www.qt.io/) for its UI, [OpenCV](https://opencv.org/) for image filtering, and [Datapath](https://www.datapath.co.uk/)'s RGBEasy API for interfacing with the capture hardware.

# VCS
A third-party capture tool for Datapath's VisionRGB range of capture cards. Greatly improves the hardware's suitability for dynamic VGA capture (e.g. of retro PCs) compared to Datapath's own capture software. Is free and open-source.

VCS interfaces with compatible capture hardware to display the capture output in a window on your desktop. Additionally, you can apply filters, scalers, anti-tearing, and various other adjustments to the output before it's displayed. A more complete list of VCS's features is given, below.

You can find a binary distribution of VCS on [Tarpeeksi Hyvae Soft's website](http://tarpeeksihyvaesoft.com/soft/).

### Features
- On-the-fly frame filtering: blur, crop, flip, decimate, rotate, sharpen...
- Several frame scalers: nearest, linear, area, cubic, and Lanczos
- Video recording with H.264 encoding
- Temporal image denoising
- Anti-tearing
- Custom overlays with HTML formatting
- Minimal reliance on GPU features &ndash; ideal for virtual machines
- Unique frame count &ndash; an FPS counter for DOS games!
- Supports Windows XP to Windows 10, compiles on Linux

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

In general, if you know that your card supports Datapath's RGBEasy API, it should be able to function with VCS.

# How to use VCS
_Coming_

## Setting up
Assuming you've installed the drivers for your capture hardware, and unpacked the binary distribution of VCS (linked to, above) into a folder, getting VCS going is simply a matter of running its `vcs.exe` executable.

When you run the executable, three windows open: a console window, in which notifications about VCS's status will apear during operation; the [output window](#the-output-window), in which captured frames will be displayed; and the [control panel](#the-control-panel), from which you can control aspects of VCS's operation.

- Note: You can launch `vcs.exe` with certain command-line parameters to automate tasks like loading up settings files. A list of the available command-line parameters is given in the [Command-line](#command-line) subsection.

One of the first things you'll probably want to do after getting VCS up and running is to adjust the video parameters, like phase, image positioning, color balance, and so on, to get the output from the capture hardware to look as it should. You can get all that done through VCS, and the sections below will tell you how. (If you're in a hurry, you can [jump straight to the part that shows you where to adjust the video settings](#input-tab).)

## The output window
The central feature of VCS is the output window, where the captured frame data is displayed in real-time as it arrives from the capture hardware.

The output window has been kept free of unnecessary clutter &ndash; there are no visible controls, only a couple of bits of convenient information in the window's title bar. This allows you to concentrate fully on the capture output.

![](images/screenshots/v1.2.6/output-window.png)

### Hidden functionality in the output window
To tell you a secret, there _is_ some hidden functionality to the output window, despite it having no visible controls per se.

#### Magnifying glass
If you press and hold the right mouse button over the output window, a portion of the output image under the cursor will be magnified.

The magnifying glass can be useful if, for instance, you're adjusting the capture's video parameters, like phase, and want to inspect the output for any smaller artefacts remaining.

![](images/screenshots/v1.2.6/output-window-nonmagnified-small.png)
![](images/screenshots/v1.2.6/output-window-magnified-small.png)

#### Borderless mode
You can double-click inside the output window to toggle the output window's border on and off.

By scaling the output size (via the control panel's [output tab](#output-tab)) to match the size of your display, you can emulate a fullscreen mode, where the borderless output window fills the entire display.

When the border is toggled off, the window is automatically snapped to the top left corner of the screen.

While borderless, you can drag the window by grabbing anywhere on it with the left mouse button.

![](images/screenshots/v1.2.6/output-window-small.png)
![](images/screenshots/v1.2.6/output-window-borderless-small.png)

#### Fullscreen mode
Although you can emulate a fullscreen mode by toggling the output window's border off and scaling the window to fit the display, as described above, there is also a 'true' fullscreen mode available. You can toggle it on and off with the F11 shortcut key.

The fullscreen mode may be of most benefit when using the OpenGL renderer (toggleable on the control panel's [output tab](#output-tab)), as it may allow your display driver to use adaptive sync &ndash; if available on your system &ndash; to match your display's refresh rate with that of the capture source.

#### Scaling with the mouse wheel
By scrolling the mouse wheel over the output window, you can scale the size of the window up and down. This is a shortcut for the `Relative scale` option in the control panel's [output tab](#output-tab).

Mouse wheel scaling is not available while recording video, or when it has been specifically disabled via the control panel's [output tab](#output-tab).

#### Quick access to the overlay editor
You can click inside the capture window with the middle mouse button to open the overlay editor.

## The control panel
The control panel is the heart of VCS. With it, you can control the various aspects of how the capture hardware conducts its operation, how VCS displays the captured frames, and so on.

The controls and information in the control panel are divided thematically into four tabs: `Input`, `Output`, `Log`, and `About`. Below, you'll find descriptions of each tab and the functionality they provide.

### Input tab
The `Input` tab lets you view and control the parameters related to the capture hardware's current input channel.

If your capture hardware has multiple input channels, you can switch between them using the drop-down selector at the top of the `Input` tab. (The screenshot was taken with capture disabled, so the selector is showing no text; but normally you'd see it displaying "Channel #1" or the like.)

![](images/screenshots/v1.3.1/control-panel-input.png)

**Signal.** The type of input signal currently being received by the capture hardware.

**Video mode.** The resolution and refresh rate of the input signal currently being received by the capture hardware.

**Frame skip.** Skip every *n*th frame. This is a hardware-level setting: the skipping is done by the capture card, and the intermediate frames are never uploaded to the system or received by VCS.

**Color depth.** Set the color depth with which the captured frames are displayed. This is a hardware-level setting: the capture card will convert each frame to this color depth before uploading it to the system; so lower color depths consume less system bandwidth. VCS will convert the frames back to the full color depth of its output window for display, although any fidelity lost in a previous conversion will remain.

**Alias resolutions.** Define alias resolutions. An alias resolution is a resolution that you want to force the capture hardware into when it proposes another resolution. For instance, if you know that the capture source's resolution is 512 x 384, but the capture hardware detects it as 511 x 304, you can assign 511 x 304 as an alias of 512 x 384. After that, every time the capture hardware sets its input resolution to 511 x 304, VCS will tell it to use 512 x 384, instead.

**Adjust video & color.** Adjust various capture parameters, like color balance, phase, horizontal position, etc. The settings are specific to the current input resolution, and will be recalled automatically by VCS each time this resolution is entered. If you have defined no settings for the current input resolution, VCS will use its default ones. Remember to save any custom settings before you exit VCS, if you want to keep them. The settings are hardware-level and will be enforced by the capture hardware.

**Force input resolution.** Tell the capture hardware to adopt a particular input resolution. If the capture source's resolution doesn't match the capture hardware's input resolution, the captured frames will likely not display correctly in VCS. If you click on a button while holding down the control key, you can change the resolution assigned to that button. The `Other...` button lets you specify an arbitrary resolution.

### Output tab
The `Output` tab lets you view and control the parameters related to VCS's output of captured frames.

![](images/screenshots/v1.3.3/control-panel-output.png)

**Resolution.** The current output resolution. This will be the size of the output window.

**Original.** The current input resolution.

**Latency.** If the capture card sends VCS a new frame before VCS has finished processing the previous one, the new frame will be ignored, and this will display "Dropping frames". Otherwise, all frames sent by the capture card are being processed and displayed by VCS in a timely manner, and this shows "No problem".

**Frame rate.** The number of frames passing through the capture pipeline 
per second. The pipeline consists of the following stages: a frame being received by VCS from the capture card, the frame being scaled and filtered, and the frame being drawn on VCS's output window.

**Renderer.** Set the type of rendering VCS uses to draw captured frames onto the [output window](#the-output-window). The _software_ renderer should be the most compatible option. The _OpenGL_ renderer may offer e.g. compatibility with adaptive synchronization technologies.
- The _OpenGL_ renderer may not work in Windows XP.
- The [magnifying glass](#magnifying-glass) is not available when using the _OpenGL_ renderer.
- The overlay (see below) is not available when using the _OpenGL_ renderer.

**Overlay.** Create a message to be overlaid on VCS's output window during capture. You can enter custom text, choose from several variables that update in real-time, and style the message with HTML. The overlay will only be shown while a signal is being received from the capture hardware.

![](images/screenshots/v1.2.6/overlay-dialog.png)

**Anti-tear.** Enable automatic removal of image tears from captured frames. Tearing can result, for instance, when the capture source is displaying a non-v-synced application: capturing DOS games often results in torn frames, as does capturing games in general whose FPS is less than or more than the refresh rate. The anti-tearing will not work in all cases &ndash; for instance, when the capture source's application is redrawing its screen at a rate higher than the refresh rate, e.g. at more than 60 FPS. You can find more information about anti-tearing in the [Anti-tearing](#anti-tearing) subsection.

![](images/screenshots/v1.2.6/anti-tear-dialog.png)

**Output size.** Set a custom output resolution, i.e. the resolution to which all captured frames will be scaled prior to being displayed in the [output window](#the-output-window).

**Relative scale.** Scale captured frames by a percentage relative to the resolution set in the `Output size` field.

**Maintain aspect ratio when scaling.** When scaling captured frames, pad the resulting image with black borders if necessary to conserve the frame's aspect ratio. If disabled, frames will be stretched to fit the output size.

**Mouse wheel scales output window.** Set whether scrolling the mouse wheel over the [output window](#the-output-window) scales the size of the window up or down. This works by modifying the `Relative scale` value, which in turns affects the size of the output window.

**Upscaler.** Set the type of scaling to be used when the output resolution is larger than the input resolution. Any relevant custom filtering (see below) will override this setting.

**Downscaler.** Set the type of scaling to be used when the output resolution is smaller than the input resolution. Any relevant custom filtering (see below) will override this setting.

**Custom filtering.** Create sets of image filters to be applied to incoming frames. You can find more information about custom filtering in the [Custom filters](#custom-filters) subsection.

### Record tab
On the `Record` tab, you can tell VCS record captured frames &ndash; as they are displayed on its [output window](#the-output-window) &ndash; into a video.

**Note!**
- The recorder will record video only. Audio will not be recorded.
- The resolution of the video will be that of the current output size, settable via the control panel's [output tab](#output-tab).
- The output size cannot be changed while recording video. All frames will be scaled &ndash; with padding as needed to maintain their aspect ratios &ndash; to fit the video's resolution.
- The overlay will not be recorded.

![](images/screenshots/v1.3.2/control-panel-record.png)

The built-in recording functionality in VCS outputs videos in the H.264 format using the x264 codec, which is capable of producing a very good (virtually lossless) image quality at reasonable file sizes.

Before being able to record video with VCS, you need to install the [x264vfw](https://sourceforge.net/projects/x264vfw/files/x264vfw/44_2851bm_44825/) codec, and run its configurator at least once, so that its settings are added into the Windows registry for VCS to find.

**Frame rate.** The video's nominal playback rate. Typically, you will want to match this to the capture source's refresh rate, so that e.g. a 60 Hz capture signal is recorded with a frame rate of 60.

**Linear sampling.** Whether VCS is allowed to duplicate and/or skip frames to match the captured frame rate with the video's nominal playback rate. If linear sampling is disabled, captured frames will be inserted into the video as they are received, and are never duplicated or skipped by the video recorder. Disabling linear sampling may result in smoother-looking playback when the capture frame rate is stable; but enabling it will help prevent time compression if the input frame rate is uneven. If you are planning to append the video with an audio track you recorded at the same time, you will most likely want to enable linear sampling when recording the video, or it may not be able to keep in sync with the audio.
- While the capture hardware is reporting 'no signal', no frames will be inserted into the video, regardless of whether linear sampling is enabled.

**Video container.** The file format in which the video is saved. On Windows, the AVI format is used.

**Video codec.** The encoder with which to create the video. On Windows, the 32-bit version of x264vfw is used.

**Options specific to H.264 encoding.** Settings like `Profile`, `Pixel format`, and `Preset` are available to you for customizing the encoding process. Further descriptions of what these settings do can be found in sources specific to H.264. In brief, if you want the best image quality, you can set `Profile` to "High 4:4:4", `Pixel format` to "RGB", and `CRF` to 1. To maintain high image quality but reduce the file size, you can set `Preset` to "Veryfast" or "Faster" instead of "Superfast" or "Ultrafast", and increase `CRF` from 1 to 10&ndash;15.

**Video.** As the recording runs, you will receive real-time information about its status, here.
- `Resolution` The video's resolution and nominal playback rate.
- `Input FPS` An estimate of the number of frames being inserted per every second of video.
- `Size on disk` How many megabytes the video file is taking up, at present.
- `Duration` The video's duration, so far; in hours, minutes, and seconds.

### Log tab
The `Log` tab displays the various internal mssages generated by VCS during its operation.

![](images/screenshots/v1.3.1/control-panel-log.png)

You can selectively show or hide messages based on their category: _info_ messages are general notifications of events; _debug_ messages contain information that's non-essential to the user but of possible interest to the developer; and _N.B._ (nota bene) messages are errors, warnings, and other such messages of importance. You can also disable logging altogether.

The messages shown in the `Log` tab are also displayed in VCS's console window.

### About tab
The `About` tab provides information about VCS, like its current version. It also shows information about the capture hardware it's currently interfaced with.

## Custom filters
_Coming_

## Alias resolutions
_Coming_

## Anti-tearing
Under some circumstances, like when capturing DOS games, you may find that the captured frames contain tearing artefacts. This is most likely caused by the capture hardware having sampled an incompletely drawn frame from the source signal &ndash; for instance, due to the source not syncing its rendering to its refresh rate. A tear, then, results as the visible edge between the incompletely drawn new frame, and the still partially visible previous frame.

VCS comes with some facilities for reducing tearing artefacts. You can enable anti-tearing from the control panel's `Output` tab, by marking the `Anti-tear` check-box.

![](images/screenshots/v1.3.3/control-panel-output.png)

Anti-tearing in VCS works by accumulating the incoming frame data from the capture hardware into an off-screen frame buffer, and displaying the buffer's contents in the output window only once all of the frame's data has been accumulated.

Noise inherent in analog capture causes some uncertainty, however, about which parts of an incoming frame are the new data to be accumulated, and which parts differ from the previous frame only due to irrelevant noise.

The accuracy with which the anti-tearing system can tell apart noise from legit changes between frames has a strong impact on the extent to which the system can remove tears. Currently, the system attempts this by sliding a horizontal sampling window along two adjacent frames' pixels, and comparing the sums of the pixel values within that window. If the sums differ by more than a given threshold, the entire row of pixels is condered new and accumulated into the frame buffer.

You can view and adjust the relevant parameters of this operation by clicking the `Settings...` button next to the `Anti-tear` check-box on the control panel's `Output` tab. Depending on what you're capturing, you may find that the default values work well enough; but in other cases, you may have better results by trying different values.

![](images/screenshots/v1.2.6/anti-tear-dialog.png)

**Range offsets.** Set the vertical range inside which the anti-tearing accumulates frame data. Static content, like a game's UI bars at the top or bottom of the screen, can completely throw off the system, and need to be excluded from consideration. The anti-tearing will ignore the first _up_ pixel rows and the last _down_ pixel rows in each frame. You can enable `Visualization` to see the values of _up_ and _down_ as corresponding vertical lines in the output window, allowing you to more easily align them with the content.

**Visualization.** Draw certain anti-tearing-related markers in the output window.

**Threshold.** Set the maximum amount by which pixel color values are allowed to change between two frames without being considered new data. The less noise there is in the capture, the lower you can set this value; and vice versa.

**Domain width.** Set the size of the sampling window. A lower value reduces CPU usage, but may be less able to detect subtle tearing.

**Step size.** Set the number of pixels by which to slide the sampling window at a time. A higher value reduces CPU usage, but may be less able to detect tearing.

**Matches req'd.** Set how many times on a row of pixels the sums of the sampling window need to exceed the threshold for that row of pixels to be considered new data.

**Update direction.** _n/a_

In general, anti-tearing is an experimental feature in VCS. It works quite well in many cases, but can fail in others, and may be a performance hog unless you have a fast CPU.

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

F1 ...................... Force the capture's input resolution to 640 x 400.

F2 ...................... Force the capture's input resolution to 720 x 400.

F11 ..................... Toggle fullscreen mode on/off.

Ctrl+A .................. Open the anti-tear dialog.

Ctrl+F .................. Open the filter sets dialog.

Ctrl+V .................. Open the video settings dialog.
```

## Command-line
Optionally, you can pass one or more of following command-line arguments to VCS:
```
-m <path + filename> .... Load capture parameters from the given file on start-
                          up. Capture parameter files typically have the .vcsm
                          suffix.

-f <path + filename> .... Load custom filter sets from the given file on start-
                          up. Filter set files typically have the .vcsf suffix.

-a <path + filename> .... Load alias resolutions from the given file on start-
                          up. Alias resolution files typically have the .vcsa
                          suffix.

-i <input channel> ...... Start capture on the given input channel (1...n). By
                          default, channel #1 will be used.
```

For instance, if you had capture parameters stored in the file `params.vcsm`, and you wanted capture to start on input channel #2 when you run VCS, you might launch VCS like so:
```
vcs.exe -m "params.vcsm" -i 2
```

# Building
**On Linux:** Do `qmake && make` at the repo's root, or open [vcs.pro](vcs.pro) in Qt Creator. **Note:** By default, VCS's capture functionality is disabled on Linux, unless you edit [vcs.pro](vcs.pro) to remove `DEFINES -= USE_RGBEASY_API` from the Linux-specific build options. I don't have a Linux-compatible capture card, so I'm not able to test capturing with VCS natively in Linux, which is why this functionality is disabled by default.

**On Windows:** The build process should be much the same as described for Linux, above; except that capture functionality will be enabled, by default.

While developing VCS, I've been compiling it with GCC 5.4 on Linux and MinGW 5.3 on Windows, and my Qt has been version 5.5 on Linux and 5.7 on Windows. If you're building VCS, sticking with these tools should guarantee the least number of compatibility issues.

### Dependencies
**Qt.** VCS uses [Qt](https://www.qt.io/) for its GUI and certain other functionality. Qt of version 5.5 or newer should satisfy VCS's requirements. The binary distribution of VCS for Windows includes the required DLLs.
- Non-GUI code in VCS interacts with the Qt GUI through a wrapper ([src/display/qt/d_main.cpp](src/display/qt/d_main.cpp)). In theory, if you wanted to use a GUI framework other than Qt, you could do so by editing the wrapper, and implementing responses to its functions in your other framework. However, there is also some Qt bleed into non-GUI areas, e.g. as use of the QString class across much of the codebase.

**OpenCV.** VCS makes use of the [OpenCV](https://opencv.org/) 3.2.0 library for image filtering and scaling, and for video recording. The binary distribution of VCS for Windows includes a pre-compiled DLL of OpenCV 3.2.0 compatible with MinGW 5.3.
- The dependency on OpenCV can be broken by undefining `USE_OPENCV` in [vcs.pro](vcs.pro). If undefined, most forms of image filtering and scaling will be unavailable, and video recording will not be possible.

**RGBEasy.** VCS uses Datapath's RGBEasy API to interface with the capture hardware. The drivers for your Datapath capture card should include and have installed the required libraries.
- The dependency on RGBEasy can be broken by undefining `USE_RGBEASY_API` in [vcs.pro](vcs.pro). If undefined, VCS will not attempt to interact with the capture hardware in any way.

# Code organization
**Modules.** The following table lists the four main modules of VCS:

| Module  | Source                             | Responsibility                        |
| ------- | ---------------------------------- | ------------------------------------- |
| Capture | [src/capture/](src/capture/)       | Interaction with the capture hardware |
| Scaler  | [src/scaler/](src/scaler/)         | Frame scaling                         |
| Filter  | [src/filter/](src/filter/)         | Frame filtering                       |
| Record  | [src/record/](src/record/)         | Record capture output into video      |
| Display | [src/display/qt/](src/display/qt/) | Graphical user interface              |

**Main loop and program flow.** VCS has been written in a procedural style. As such, you can easily identify &ndash; and follow &ndash; the program's main loop, which is located in `main()` in [src/main.cpp](src/main.cpp).

- The main loop first asks the `Capture` module to poll for any new capture events, like a new captured frame.
- Once a new frame has been received, it is directed into the `Filter` module for any pre-scaling filtering.
- The filtered frame will then be passed to the `Scaler` module, where it's scaled to match the user-specified output size.
- The scaled frame will be fed into the `Filter` module again for any post-scaling filtering.
- Finally, the frame is sent to the `Display` module for displaying on screen.

**Qt's event loop.** The loop in which Qt processes GUI-related events is spun manually (by `update_gui_state()` in [src/display/qt/d_window.cpp](src/display/qt/d_window.cpp)) each time a new frame has been received from the capture hardware. This is done to match the rate of screen updates on the output to that of the input capture source.

## How-to

**Adding a frame filter.** Frame filters allow captured frames' pixel data to be modified in particular ways before being displayed on screen. A filter might, for instance, apply blur, cropping, or rotation to each frame. Adding a new filter to VCS is fairly easy, and the process is described below.

A filter consists of a `filter function`, and a `filter dialog`. The filter function applies the filter to the frame's pixel data, and the filter dialog is a GUI element that lets the user specify values for any parameters the filter might take &ndash; like radius for a blur filter.

The filter function is to be defined in [src/filter/filter.cpp](src/filter/filter.cpp). There, you will also find all of VCS's other filters. Looking at that source file, you can see the following pattern:
```
// Pre-declare the filter function.
static void filter_func_NAME(FILTER_FUNC_PARAMS);

// Add the filter to VCS's list of known filters.
static const std::map ... FILTERS =
    ...
    {"DISPLAY_NAME", {filter_func_NAME, []{ static filter_dlg_NAME_s f; return &f;}()}},
    ...

// Define the filter function.
static void filter_func_NAME(FILTER_FUNC_PARAMS)
{
    VALIDATE_FILTER_INPUT

    // Perform the filtering.
    ...
}
```

The filter function's parameter list macro `FILTER_FUNC_PARAMS` expands to
```
u8 *const pixels, const resolution_s *const r, const u8 *const params
```
where `pixels` is a one-dimensional array of 32-bit color values (8 bits each for BGRA), `params` is an array providing the filter's parameter values, and `r` is the frame's resolution, giving the size of the `pixels` array.

The following simple filter function uses OpenCV to set each pixel in the frame to a light blue color. (Note that you should encase any code that uses OpenCV in a `#if USE_OPENCV` directive.)
```
static void filter_func_blue(FILTER_FUNC_PARAMS)
{
    VALIDATE_FILTER_INPUT

    #if USE_OPENCV
        cv::Mat output = cv::Mat(r->h, r->w, CV_8UC4, pixels);
        output = cv::Scalar(255, 150, 0, 255);
    #endif
}
```

You can have a look at the existing filter functions in [src/filter/filter.cpp](src/filter/filter.cpp) to get a feel for how the filter function can be operated.

The filter dialog is to be defined in [src/display/qt/df_filters.h](src/display/qt/df_filters.h), where you can also find the dialogs of VCS's other filters. Each dialog is defined as a separate struct inheriting from the dialog base `filter_dlc_s`. A filter can have no more or less than one dialog.

The following is a minimal filter dialog:

```
struct filter_dlg_NAME_s : public filter_dlg_s
{
    filter_dlg_NAME_s() :
        filter_dlg_s("DISPLAY_NAME") {}

    // Offsets in the paramData array of the various parameters' values.
    enum data_offset_e { OFFS_X = 0 };

    // Initialize paramData to its default values for this filter.
    void insert_default_params(u8 *const paramData) const override
    {
        memset(paramData, 0, FILTER_DATA_LENGTH);
        paramData[OFFS_X] = 15;
    }

    void poll_user_for_params(u8 *const paramData, QWidget *const parent = nullptr) const override
    {
        QDialog d(parent, QDialog().windowFlags() & ~Qt::WindowContextHelpButtonHint);
        d.setWindowTitle(filterName + " Filter");
        d.setMinimumWidth(dlgMinWidth);

        // Add the dialog's widgets. We'll let the user specify
        // a value in the range 0..255 for a parameter called X.
        {
            QLabel xLabel("X:");

            QSpinBox xSpin;
            xSpin.setRange(0, 255);
            xSpin.setValue(paramData[OFFS_X]);

            QFormLayout layout;
            layout.addRow(&xLabel, &xSpin);
            d.setLayout(&layout);
        }

        // Display the dialog (blocking).
        d.exec();

        // Update paramData with values provided by the user.
        paramData[OFFS_X] = xSpin.value();
    }
}
```

When the user asks for access to the filter's parameters, VCS will call `poll_user_for_params()`. The function constructs and displays a dialog containing GUI elements with which the user can modify the filter's parameters. Once the user has finished doing so, the function saves the parameter values into the `paramData` byte array, which will later be sent to the filter function as its `params` array.

# Project status
VCS is currently in post-1.0, having come out of beta in 2018. Development is sporadic.

### System requirements
You are encouraged to have a fast CPU, since most of VCS's operations are performed on the CPU. The GPU is of less importance, and even fairly old ones will likely work. VCS uses roughly 1 GB of RAM, and so your system should have at least that much free &ndash; preferably twice as much or more.

**Performance.** On my Intel Xeon E3-1230 v3, VCS performs more than adequately. The table below shows that an input of 640 x 480 can be scaled to 1920 x 1440 at about 300&ndash;400 frames per second, depending on the interpolation used.

| 640 x 480    | Nearest | Linear | Area | Cubic | Lanczos |
|:------------ |:-------:|:------:|:----:|:-----:|:-------:|
| 2x upscaled  | 1100    | 480    | 480  | 280   | 100     |
| 3x upscaled  | 460     | 340    | 340  | 180   | 50      |

Drawing frames onto the [output window](#the-output-window) using software rendering is likewise sufficiently fast, as shown in the following table. An input of 640 x 480 can be upscaled by 2x and drawn on screen at roughly 340 frames per second when using nearest-neighbor interpolation.

| 640 x 480       | 1x<br>Nearest | 2x<br>Nearest | 3x<br>Nearest |
|:--------------- |:-------------:|:-------------:|:-------------:|
| With display    | 1360          | 340           | 150           |
| Without display | 1910          | 1100          | 510           |

Padding (i.e. aspect ratio correction) can incur a performance penalty with some of the scalers. The following table shows the frame rates associated with scaling a 640 x 480 input into 1920 x 1080 with and without padding to 4:3.

| 480p to 1080p | Nearest | Linear | Area | Cubic | Lanczos |
|:------------- |:-------:|:------:|:----:|:-----:|:-------:|
| Padded / 4:3  | 390     | 270    | 270  | 200   | 80      |
| No padding    | 820     | 370    | 370  | 210   | 70      |

### A word about VCS's GUI
I run my capture card, the VisionRGB-PRO2, through a virtual machine, since the Linux I'm on isn't natively supported by the card. In the VM, performance-wise, I find that a Windows XP guest works best, so I use that.

For that reason, the VCS GUI has been tweaked for use with Windows XP, and looks somewhat old-fashioned and disjoint on newer versions of Windows. I'm aware of this, but unfortunately it can't be helped in the current circumstance, since I want to be able to run VCS properly in my own use.

# Authors and credits
The primary author of VCS is the one-man Tarpeeksi Hyvae Soft (see on [GitHub](https://github.com/leikareipa) and the [Web](http://www.tarpeeksihyvaesoft.com)).

VCS uses [Qt](https://www.qt.io/) for its UI, [OpenCV](https://opencv.org/) for image filtering, and [Datapath](https://www.datapath.co.uk/)'s RGBEasy API for interfacing with the capture hardware.

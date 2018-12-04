# VCS
A personal project to improve on Datapath's default capture software, VCS has come to include a host of features useful in capturing footage of old '90s games, in particular; although capture for other purposes may likewise benefit from the features of VCS. Find out more in the readme file included with the binary distribution of VCS, available from [Tarpeeksi Hyvae Soft's website](http://tarpeeksihyvaesoft.com/soft)

This repo contains the full source code to VCS; compilable on Windows and Linux (with Qt). Note that the repo tracks my local one with low granularity only.

### Building
To build on Linux, do ```qmake && make```, or load the .pro file in Qt Creator. Note that by default, capture functionality will be disabled on Linux, unless you change it in the .pro file. I'm not aware that Datapath's capture API works in Linux, nor do I have compatible hardware to test with, hence this affair.

Building on Windows should be much the same, except the build will by default have capture functionality enabled.

I use GCC and MinGW, respectively, to build VCS. These toolsets will likely give you the easiest time when compiling VCS yourself. Qt-wise, version 5.5 should suffice.

##### OpenCV
VCS uses the OpenCV library for certain functionality. For proper operation, you'll need to have OpenCV present&mdash;see the .pro file on how to include it. The binary distribution of VCS (see link, above) includes pre-built .dlls for MinGW.

##### Datapath RGBEASY API
VCS interfaces with Datapath's RGBEASY API to interact with the capture hardware, and so needs access to the RGBEASY library. If you've installed the drivers for your compatible Datapath capture card, you should have this library on your computer. See the VCS .pro file on how to include it.

### Code flow
VCS consists of four main parts: ```capture```, ```filter```, ```scaler```, and ```display```. The code for these can be found in the likewise-named .cpp files (the source for ```display``` resides under ```src/qt/```).

Marshaled by the main loop in ```src/main.cpp```, the typical flow of execution in the program is as follows. The capture hardware will be polled by ```capture``` for any new capture events. Once ```capture``` receives a new frame of data from the hardware, it will direct it into ```filter``` for any pre-scaling filtering, like denoising, blurring, etc. From there, the frame passes on to ```scaler```, which re-sizes it to the size of the display. The frame continues again through ```filter``` for any post-scaling filtering; and finally, the frame is handed to ```display``` for displaying it on screen. In the meantime, any user&ndash;GUI interaction is also handled by ```display```.

In the current implementation, Qt's event loop in ```display``` is spun manually, relying on the capture hardware to set the pacing of screen updates for drawing new frames. I use VCS in a virtual machine (KVM; Windows guest, Linux host), so can't say whether notable screen-tearing or jerkiness is introduced by this process under Windows-native operation. It seems to work well enough under a VM. In cases where it doesn't, you may want to look into letting Qt spin its own loop, or implement some other way of syncing screen updates with the refresh rate&mdash;like displaying the frames on a QOpenGLWidget surface that's set to block for vsync.

![A screenshot of VCS](http://tarpeeksihyvaesoft.com/soft/img/vcs_a.png)

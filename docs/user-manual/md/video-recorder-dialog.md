# Video recorder dialog

This dialog can be accessed with <key-combo>Ctrl + R</key-combo> or <menu-path>Context > Output > Dialogs > Video recorder...</menu-path>.

<dokki-warning>
    On Linux, the video encoder's quality settings can't be modified through VCS &ndash; you'd need to re-compile OpenCV with your desired encoder settings, instead.
</dokki-warning>

The video recorder gives you the option to stream captured frames into a video file.

To use the video recorder in Windows, you'll need to install the 32-bit version of the [x264vfw](https://sourceforge.net/projects/x264vfw/files/x264vfw/44_2851bm_44825/) codec and run its configurator at least once, so that its settings are added into the Windows registry for VCS to find.

The recorder will write frames as they appear in the [output window](#output-window) into a video file, with the following caveats:

- Audio won't be recorded.
- Frames will be inserted into the video at the rate of capture; the recorder doesn't try to maintain any particular frame rate (e.g. by duplicating or dropping frames). For example, if your capture source is 57.5 Hz, one minute of video will have 57.5 * 60 frames, and if that video is played back at 60 FPS, it will appear slightly sped up.
- If VCS drops any frames during recording (e.g. due to insufficient system performance), the video's playback will be non-linear. So if you're recording a separate audio file and are planning to sync it with the video, you want there to be no frames dropped while recording the video.
- The video will be recorded in the H.264 format using an x264 codec.
- The video resolution will be that of the current output size (see the [Output resolution](#output-resolution-dialog) dialog).
- The output size can't be changed while recording; all frames will be scaled automatically to fit the current size.
- The [overlay](#overlay-dialog) won't be recorded.
- Encoder parameters influencing image quality (e.g. CRF) can't be customized in the Linux version of VCS. This is a limitation of OpenCV. You can, however, modify and recompile the OpenCV code with higher-quality default options (see e.g. [here](https://www.researchgate.net/post/Is_it_possible_to_set_the_lossfree_option_for_the_X264_codec_in_OpenCV)).


## Settings explained

<dokki-table headerless>
    <template #table>
        <tr>
            <th>Setting<sup>*</sup></th>
            <th>Description</th>
        </tr>
        <tr>
            <td>Nominal FPS</td>
            <td>
                The video's suggested playback rate. This setting doesn't affect the recording rate, only the rate at which the video might be played back by your video player. The actual recording FPS will be determined by the capture source's refresh rate.
            </td>
        </tr>
        <tr>
            <td>Video container</td>
            <td>
                The file format used for storing the video on disk.
            </td>
        </tr>
        <tr>
            <td>Video codec</td>
            <td>
                The codec used for encoding the video. Currently, only <em>x264</em> is supported.
            </td>
        </tr>
        <tr>
            <td>Arguments</td>
            <td>
                You can enter any additional command-line arguments for the video codec.
            </td>
        </tr>
        <tr>
            <td colspan="2">
                <sup>*</sup>Only some of the available settings are included in this list; others depend on the choice of video codec and are as per standard for that codec (e.g. CRF for x264).
            </td>
        </tr>
    </template>
</dokki-table>

## Settings for the highest video quality

The following video recorder settings should result in the highest possible output video quality.

<dokki-table headerless>
    <template #table>
        <tr>
            <th>Setting</th>
            <th>Value</th>
        </tr>
        <tr>
            <td>Video codec</td>
            <td>x264</td>
        </tr>
        <tr>
            <td>Profile</td>
            <td>High 4:4:4</td>
        </tr>
        <tr>
            <td>Pixel format</td>
            <td>RGB</td>
        </tr>
        <tr>
            <td>CRF</td>
            <td>0<sup>*</sup></td>
        </tr>
        <tr>
            <td colspan="2">
                <sup>*</sup>Increasing the CRF value to 10&ndash;14 will considerably reduce the size of the resulting video file while maintaining high image quality.
            </td>
        </tr>
    </template>
</dokki-table>

## Tips for performance

If the recorder dialog indicates that frames are being dropped while recording, your system may be limited in CPU or disk performance. Here are a few tips that may help you mitigate those issues.

Since VCS is largely a single-threaded program, dragging or otherwise interacting with its GUI items (e.g. dialog windows) during recording may cause transient frame drops even if your system is otherwise capable.  

During recording, frames are saved to disk in batches. If you get a bunch of frames droppede very couple of seconds but are fine otherwise, insufficient disk performance may be the reason. Optimizing the recorder's settings for a smaller file size should help, as would recording at a lower resolution (i.e. with a smaller-sized [output window](#output-window).

If recording fails and you receive an error in the VCS console window to the tune of "x264 [error]: malloc of size ░ failed", you may be running into a memory fragmentation issue (see [this issue report on GitHub](https://github.com/leikareipa/vcs/issues/21)). VCS is a 32-bit program that, while recording video, shares its couple-gigabyte memory space with the x264 encoder, which may result in x264 being unable to allocate a large-enough contiguous block of memory for its own operation. You may be able to fix this issue either by selecting a <em>Preset</em> option closer to "Ultrafast", reducing the recording resolution (i.e. the size of the output window), or decreasing the size of VCS's pre-allocated memory cache (see the <em>-m</em> [command-line option](#command-line-options)).

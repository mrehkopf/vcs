# Anti-tear dialog

This dialog can be accessed with <key-combo>Ctrl + A</key-combo> or <menu-path>Context > Output > Dialogs > Anti-tear...</menu-path>.

> A screenshot of the anti-tear dialog
![{image:489x502}](https://github.com/leikareipa/vcs/raw/master/images/screenshots/v2.4.0/anti-tear-dialog.png)

The anti-tear dialog provides functionality to remove tearing from captured frames.

Under some circumstances, like when the captured source doesn't sync its rendering with the refresh rate, captured frames can contain tearing. VCS's anti-tearer helps mitigate this issue.

Anti-tearing should be considered an experimental feature of VCS. It works well in some cases and not that well in others. It'll completely fail to work if the captured source redraws the screen at a rate higher than the capture's refresh rate &ndash; e.g. a game running at 100 FPS with a refresh of 60 Hz.

## Settings explained

<dokki-table headerless>
    <template #table>
        <tr>
            <th>Setting</th>
            <th>Description</th>
        </tr>
        <tr>
            <td>Scan start</td>
            <td>
                Set where the anti-tearer begins scanning each frame for tears. Static screen-wide content like a game's UI bar can prevent the anti-tearing from working, so you should set this value so that such content is excluded. You can choose to visualize the scan range to help you set it up.
            </td>
        </tr>
        <tr>
            <td>Scan end</td>
            <td>
                Same as <strong>Scan start</strong> but for where the scanning should end. This is an offset from the bottom of the screen up, so e.g. a value of 5 at a resolution of 640 &times; 480 would mean the scanning ends at pixel row 475.
            </td>
        </tr>
        <tr>
            <td>Scan direction</td>
            <td>
                If the captured source redraws its screen from bottom to top, set the scan direction to <em>Down</em>. Otherwise, use the <em>Up</em> setting. Using the wrong direction will fully prevent the anti-tearing from working (it may correctly detect tears but won't be able to remove them).
            </td>
        </tr>
        <tr>
            <td>Scan hint</td>
            <td>
                If the captured source redraws its screen at a rate higher than half of its refresh rate but lower than the full refresh rate (e.g. 35 FPS at 60 Hz), you may (or might not) have better results and/or performance by setting this option to <em>Look for one tear per frame</em>. Otherwise, use the <em>Look for multiple tears per frame</em> setting.
            </td>
        </tr>
        <tr>
            <td>Visualization</td>
            <td>
                Draw certain anti-tearing-related markers in the <a href="#output-window">output window</a>.
            </td>
        </tr>
        <tr>
            <td>Threshold</td>
            <td>
                The anti-tearer compares adjacent frames to find which parts of the new frame may be torn (where pixels from the previous frame are still visible). This setting controls the amount by which a pixel's color values are allowed to change between frames without the pixel being considered new (given inherent noise in analog pixels). In an ideal situation where there's no noise in the captured signal, you can set this to 0 or close to it. Otherwise, the value should be high enough to exclude capture noise.
            </td>
        </tr>
        <tr>
            <td>Window size</td>
            <td>
                When scanning frames for tears, the anti-tearer will average together a bunch of horizontal pixels' color values to reduce the negative effect of analog noise. This setting controls the pixel size of the sampling window. Lower values will result in better performance but possibly worse tear detection.
            </td>
        </tr>
        <tr>
            <td>Step size</td>
            <td>
                The number of pixels to skip horizontally when scanning for tears. Higher values will improve performance but may cause a failure to detect subtler tears.
            </td>
        </tr>
        <tr>
            <td>Matches req'd</td>
            <td>
                Set how many times the sampling window must find a pixel's color values to have exceeded the detection threshold for a horizontal row of pixels to be considered new relative to the previous frame. Higher values should reduce the chance of false positives but may also cause a failure to detect subtler tears.
            </td>
        </tr>
    </template>
</dokki-table>

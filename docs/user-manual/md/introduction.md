# Introduction

VCS is a third-party capture tool by [Tarpeeksi Hyvae Soft](https://www.tarpeeksihyvaesoft.com) for Datapath's VisionRGB range on capture hardware; developed especially for capturing retro PCs and consoles, whose dynamic, non-standard video signals aren't well supported by Datapath's default capture software.

VCS is [available on GitHub](https://github.com/leikareipa/vcs). You can also <ths-inline-feedback-button> send direct feedback</ths-inline-feedback-button>.

> A screenshot of VCS 2.4, showing the capture window and some of the control dialogs
![{image:1189x878}](https://www.tarpeeksihyvaesoft.com/soft/img/vcs/vcs-2.4-with-dialogs.png)


## Program features

- Capture functionality tailored for the Datapath VisionRGB series
- Supports both modern and legacy VisionRGB hardware
- Runs on Windows (XP and later) and Linux (experimental)
- Unlimited video presets, allowing different capture settings for different resolutions and/or refresh rates
- Customizable frame scaling and filtering
- Anti-tearing to reduce/eliminate tears in captured frames
- On-screen display with HTML/CSS formatting
- Variable refresh rate output
- Minimal reliance on GPU features, runs well in virtual machines

## Supported capture hardware

<dokki-table headerless>
    <template #table>
        <tr>
            <th>Vendor</th>
            <th>Model</th>
        </tr>
        <tr>
            <td>Datapath</td>
            <td>VisionRGB-PRO1</td>
        </tr>
        <tr>
            <td>Datapath</td>
            <td>VisionRGB-PRO2</td>
        </tr>
        <tr>
            <td>Datapath</td>
            <td>VisionRGB-E1</td>
        </tr>
        <tr>
            <td>Datapath</td>
            <td>VisionRGB-E2</td>
        </tr>
        <tr>
            <td>Datapath</td>
            <td>VisionRGB-E1S</td>
        </tr>
        <tr>
            <td>Datapath</td>
            <td>VisionRGB-E2S</td>
        </tr>
        <tr>
            <td>Datapath</td>
            <td>VisionRGB-X2</td>
        </tr>
    </template>
</dokki-table>

## System requirements

<dokki-table headerless>
    <template #table>
        <tr>
            <th>OS</th>
            <td class="with-inline-table">
                <dokki-table>
                    <template #table>
                        <tr>
                            <th>Platform</th>
                            <th>Required version</th>
                        </tr>
                        <tr>
                            <td>Windows</td>
                            <td>XP or later</td>
                        </tr>
                        <tr>
                            <td>Linux</td>
                            <td>Kernel 5</td>
                        </tr>
                    </template>
                </dokki-table>
            </td>
        </tr>
        <tr>
            <th>CPU</th>
            <td class="with-inline-table">
                <dokki-table>
                    <template #table>
                        <tr>
                            <th>Capture resolution</th>
                            <th>Required performance level<sup>*</sup></th>
                        </tr>
                        <tr>
                            <td>VGA</td>
                            <td>Intel Sandy Bridge</td>
                        </tr>
                        <tr>
                            <td>1080p, 30 FPS</td>
                            <td>Intel Haswell</td>
                        </tr>
                        <tr>
                            <td>1080p, 60 FPS</td>
                            <td>Intel Coffee Lake</td>
                        </tr>
                        <tr>
                            <td colspan="2">
                                <sup>*</sup>Estimated.
                            </td>
                        </tr>
                    </template>
                </dokki-table>
            </td>
        </tr>
        <tr>
            <th>GPU</th>
            <td>
                For hardware rendering (optional), an OpenGL 1.2-compatible video card
            </td>
        </tr>
        <tr>
            <th>RAM</th>
            <td>1 GB</td>
        </tr>
        <tr>
            <th>Other</th>
            <td>
                <ul>
                    <li>The Datapath capture card driver must be installed</li>
                    <li>On Linux, OpenCV 3.2.0 and Qt 5 must be installed</li>
                </ul>
            </td>
        </tr>
    </template>
</dokki-table>

# Introduction to VCS

VCS is an [open-source](https://github.com/leikareipa/vcs) Linux video capture application for Datapath capture cards, with a feature-set targeted especially at retro enthusiasts.

> A screenshot of VCS 2.8 showing the capture window (in the background) and some of the control options.
![{image}{headerless}{no-border-rounding}](../img/vcs-2.8.png)

## Key program features

- Various usability improvements over Datapath's bundled capture application, enabling high-quality capture of dynamic signals
- Unlimited video presets with programmable activation
- Several scaling modes and image filters
- Variable refresh rate output
- Free and open source, with a modular implementation easily extendable to non-Datapath capture devices

## Supported capture hardware

Any model of Datapath capture card supported by the Datapath Vision driver for Linux should be compatible with VCS; although the card's full set of capabilities may not be exposed.

## System requirements

<dokki-table headerless>
    <table>
        <tr>
            <th>OS</th>
            <td>Linux</td>
        </tr>
        <tr>
            <th>CPU</th>
            <td>
                <dokki-table headerless>
                    <table>
                        <tr>
                            <th>Capture resolution</th>
                            <th>Recommended performance level</th>
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
                    </table>
                </dokki-table>
            </td>
        </tr>
        <tr>
            <th>GPU</th>
            <td>
                For hardware rendering (optional), an OpenGL 1.2 compatible graphics card
            </td>
        </tr>
        <tr>
            <th>RAM</th>
            <td>1 GB</td>
        </tr>
    </table>
</dokki-table>

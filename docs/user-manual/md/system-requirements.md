# System requirements

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
                            <th>Required performance level</th>
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
                    </template>
                </dokki-table>
            </td>
        </tr>
        <tr>
            <th>GPU</th>
            <td>
                For OpenGL rendering (optional), an OpenGL 1.2-compatible video card
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
                    <li>On Linux, the legacy OpenCV 3.2.0 library must be installed</li>
                </ul>
            </td>
        </tr>
    </template>
</dokki-table>

# Output resolution dialog

This dialog can be accessed with <key-combo>Ctrl + O</key-combo> or <menu-path>Context > Output > Dialogs > Resolution...</menu-path>.

The output resolution dialog lets you resize the [output window](#output-window). This also resizes the frames being displayed in the window.

## Settings explained

<dokki-table headerless>
    <template #table>
        <tr>
            <th>Setting</th>
            <th>Description</th>
        </tr>
        <tr>
            <td>Resolution</td>
            <td>
                Lock the size of the output window so that changes to the capture resolution don't affect the output
                resolution. Frames will be scaled up or down as needed to match this resolution.
            </td>
        </tr>
        <tr>
            <td>Relative scale</td>
            <td>
                Scale the size of the output window up or down by a percentage of its base size. The base size is
                either the capture resolution, or, if enabled, the locked output resolution.
            </td>
        </tr>
    </template>
</dokki-table>

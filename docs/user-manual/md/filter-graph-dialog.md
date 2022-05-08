# Filter graph dialog

This dialog can be accessed with <key-combo>Ctrl + F</key-combo> or <menu-path>Context > Output > Dialogs > Filter graph...</menu-path>.

> A screenshot of the filter graph dialog
![{image:1187x556}](https://github.com/leikareipa/vcs/raw/master/images/screenshots/v2.4.0/filter-graph-dialog.png)

The filter graph dialog lets you to create chains of image filters to be applied to captured frames prior to display in the [output window](#output-window).

The filter graph is made up of nodes that can be connected together in a chain. These nodes come in three varieties: *input gate*, *output gate*, and *filter*.

The input and output gates determine the resolutions for which the connected filters will be applied. For instance, if you set an input gate's width and height to 640 and 480, and the width and height of an output gate to 1920 and 1080, any filters you connect between these two nodes will be applied when the size of the output window is 1920 &times; 1080 and the original resolution of the frames (i.e. the capture resolution) is 640 &times; 480. You can also use the value 0 for a gate's width and/or height to allow VCS to match any value to that dimension: an input gate with a width and height of 0, for instance, will apply the connected filters to frames of all capture resolutions, provided that they also meet the resolution specified for the output gate. A filter graph can have multiple chains of these input-filter-output combos, and VCS will select the most suitable one (or none) given the current capture and output resolutions.

<dokki-tip>
    <p>
        When deciding which of multiple filter chains to use, VCS will prefer more specific chains to more general ones.
    </p>
    <p>
        If you have e.g. an input gate whose width and height are 0, and another input gate whose width and height are 640 and 480, the latter will be used when the capture resolution is exactly 640 &times; 480, and the former otherwise.
    </p>
    <p>
        Likewise, if your input gates are 0 &times; 0 and 640 &times; 0, the former will be applied for capture resolutions of <i>any</i> &times; <i>any</i>, except for 640 &times; <i>any</i>, where the latter chain will apply &ndash; except if you also have a third input gate of 640 &times; 480, in which case that will be used when the capture resolution is exactly 640 &times; 480.
    </p>
</dokki-tip>

To connect two nodes, click and drag with the left mouse button from one node's output edge (square) to another's input edge (circle), or vice versa. A node can be connected to as many other nodes as you like. To disconnect a node from another, right-click on the node's output edge, and select the other node from the list that pops up. To remove a node itself from the graph, right-click on the node and select to remove it. To add nodes to the graph, right-click on the graph's background to bring up the node menu, or select <menu-path>Filters</menu-path> from the dialog's menu bar.

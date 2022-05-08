# Video presets dialog

This dialog can be accessed with <key-combo>Ctrl + V</key-combo>or <menu-path>Context > Input > Dialogs > Video presets...</menu-path>.

> A screenshot of the video presets dialog
![{image:513x656}](https://github.com/leikareipa/vcs/raw/master/images/screenshots/v2.4.0/video-presets-dialog.png)

The video presets dialog lets you to modify the capture devices's video signal parameters.

A given video preset's parameters will be applied when all of its "Activates with" conditions are met. For instance, if you've defined a preset's activation resolution as 800 &times; 600 and have disabled the other activating conditions, the preset's parameters will be applied when the capture video mode is 800 &times; 600.

To add or delete a preset, click the + or - buttons next to the preset selector at the top of the dialog. Clicking the + button while holding the Alt key will create a new preset with the current preset's settings.

If you want your changes to the video presets to persist after you exit VCS, remember to save them first! This can be done via <menu-path>File > Save as&hellip;</menu-path>. Saved settings can be restored via <menu-path>File > Open&hellip;</menu-path>. Any saved settings that're open when VCS exits will be reloaded automatically when you run VCS again.

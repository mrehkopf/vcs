# Main Page

## Introduction
Welcome to VCS's technical documentation! These documents aim to provide architectural and code-level information about VCS to developers.

VCS is a handy application by Tarpeeksi Hyvae Soft for controlling certain models of Datapath's Vision range of video capture cards. (Neither Tarpeeksi Hyvae Soft nor VCS are associated with Datapath nor are there any endorsements.)

You can find VCS's source code repository on GitHub here: https://www.github.com/leikareipa/vcs/. The repository includes end-user documentation.

## Layout of documentation

In most cases, the thematic organizational unit of code in VCS is the source file (e.g. .cpp); for which the corresponding header file (e.g. .h) provides a public interface accessible to other units. This is organization is roughly equivalent to classes: the source file represents the class, and static global variables and functions inside the source file are its private members.

With that in mind, perhaps the most useful way of accessing this documentation is on the file level, where for each documented header file there's an overview of its interface followed by documentation of each of the interface's functions. You can find an index of the documented interfaces (header files) [here](./files.html), or by clicking on "Files" in the upper left corner.

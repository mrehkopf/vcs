# VCS Developer Docs
VCS comes with work-in-progress documentation in the Doxygen format.

Pre-built docs are hosted [here](https://www.tarpeeksihyvaesoft.com/vcs/devdocs/).

To build the documentation yourself:

1. Run `$ doxygen doxygen.cfg`. This will produce Doxygen's documentation for VCS in XML format (placed under the `xml` subdirectory)
2. Convert the XML into HTML using [vcs-doxy-theme](https://github.com/leikareipa/vcs-doxy-theme)

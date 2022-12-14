# VCS developer docs
VCS comes with work-in-progress developer documentation in the Doxygen format.

To build the documentation:

1. Run `$ doxygen doxygen.cfg` from this directory. It'll build the docs in XML format and put them in an  `xml` folder under this directory.
2. Convert the XML into HTML using [vcs-doxy-theme](https://github.com/leikareipa/vcs-doxy-theme).

Pre-built docs are hosted [here](https://www.tarpeeksihyvaesoft.com/vcs/docs/developer-manual/), but aren't guaranteed to be up-to-date.

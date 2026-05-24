# Organizer

## Overview

Organizer is a Qt lightweight personal information manager.

It is maintained by the LXQt project but can be used independently from this
desktop environment.

![](organizer-070.png)

### Compiling source code

Compiling from source provides the latest version.  

```
cmake -B build -S .
cmake --build build
cmake --install build
```

### Dependencies

Runtime dependencies are [liblxqt](https://github.com/lxqt/liblxqt), qtmultimedia and qtsvg.
Additional build dependencies are CMake, [lxqt-build-tools](https://github.com/lxqt/lxqt-build-tools), qt6-tools and optionally Git to pull latest VCS checkouts.

## Versioning

[SemVer](http://semver.org/) is used for versioning. The version number has the form 0.0.0 representing major, minor and bug fix changes. Currently at 0.7.4.

## Roadmap

```
Custom calendar (completed)
Add, remove and update appointments (completed)
Add, remove and update contacts (completed)
Reminders (completed)
Calendar themes (light and dark themes completed)
Enhanced features
``` 

### Translation (Weblate)

Translations can be done in [LXQt-Weblate](https://translate.lxqt-project.org/projects/lxqt-desktop/organizer/).


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

### Prerequisites

You need the following dependencies to build and run Organizer on Debian, Ubuntu and its derivatives.

```
build-essential
cmake
lxqt2-build-tools
liblxqt2-dev
qt6-base-dev
qt6-svg-dev
qt6-tools-dev
qt6-tools-dev-tools
qt6-l10n-tools
qt6-multimedia-dev
```

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


Audex
=====

Audex is an audio grabber tool for CD-ROM drives built with KDE Frameworks.

### Prerequisites

* KF5Cddb
* libcdio
* libcdio-paranoia

### Prerequisites (optional)

Native supported encoder binaries:

* FLAC (flac files)
* LAME (mp3 files)
* Opus (opus files)
* Ogg Vorbis (ogg files)
* FAAC (mp4/aac files)

Beyond these you can define custom profiles for nearly any encoding binary support.

### Build and install

```
cmake -DCMAKE_VERBOSE_MAKEFILE=ON -DCMAKE_BUILD_TYPE=Debug -DCMAKE_INSTALL_PREFIX=/usr -B build/
cmake --build build/
```

For buildung and installation in one step:

```
(sudo) cmake --build build/ --target install
```

To uninstall either use classical way:

```
(sudo) make uninstall
```

Or remove all files listed in `install_manifest.txt` in your `build/` directory manually:

```
(sudo) xargs rm < build/install_manifest.txt
```

### Further information

These files make Audex libcdio/libcdio-paranoia dependent:

* utils/cddaparanoia.h
* utils/cddaparanoia.cpp


Audex
=====

Audex is an audio grabber tool for CD-ROM drives built with KDE Frameworks.

### Prerequisites

* KF5Cddb
* cdparanoia

### Prerequisites (optional)

* FLAC (flac files)
* LAME (mp3 files)
* Opus (opus files)
* Ogg Vorbis (ogg files)
* FAAC (mp4/aac files)

### Compile and install
```
mkdir build
cd build
cmake -DCMAKE_VERBOSE_MAKEFILE=ON -DCMAKE_BUILD_TYPE=Debug -DCMAKE_INSTALL_PREFIX=/usr ..
make
(sudo) make install
```

### Further information

These files make Audex linux dependent:

* utils/pid.h
* utils/pid.cpp

These files make Audex cdda paranoia dependent:

* utils/cddaparanoia.h
* utils/cddaparanoia.cpp


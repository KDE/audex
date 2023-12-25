Audex
=====

Audex is an audio grabber tool for CD-ROM drives built with KDE Frameworks.

*Note: There is an active branch "kf5" containing a Qt5/KF5 supported version 0.96.X.*

### Prerequisites

* libkcddb
* cdparanoia

### Prerequisites (optional)

* FLAC (flac files)
* LAME (mp3 files)
* Opus (opus files)
* Ogg Vorbis (ogg files)
* FAAC (mp4/aac files)

### Compile
```
mkdir build
cd build
cmake -DCMAKE_VERBOSE_MAKEFILE=ON -DCMAKE_BUILD_TYPE=Debug -DQT_NO_FIND_QMLSC=TRUE -DCMAKE_INSTALL_PREFIX=/usr ..
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

### TODO
* Cover auto search
* Add device selector widget
* Check for bugs in KF6 porting effort

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
* eyeD3 (mp3 tagging)

### Compile
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

### TODO
* New cover search API (not Bing with dev key requirement)
* Add device selector widget
* Check for bugs in KF5 porting effort


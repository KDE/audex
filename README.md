Audex
=====

** WARNING! This is a not yet runnable and highly experimental branch with complete new ripping technologies. **

Audex is an audio grabber tool for CD-ROM drives built with KDE Frameworks.

### Some of the new features

* No dependencies to cdio/cdparanoia anymore (new custom ripping engine)
* CD-Text support
* Full offset correction with AccurateRip database reading support (http://www.accuraterip.com/driveoffsets.htm)
* AccurateRip support (http://www.accuraterip.com/)
* ReplayGain tag support

### Features

* Native encoder binary support for FLAC, LAME (MP3), Opus, Ogg Vorbis and FAAC (MP4/AAC)
* Custom encoder binary support
* CDDB/MusicBrainz support

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


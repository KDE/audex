# Audex

**Audex** is an advanced audio ripping tool for CD-ROM drives, built using KDE Frameworks.

## Features

- Native support for encoder binaries: FLAC, LAME (MP3), Opus, Ogg Vorbis, and FAAC (MP4/AAC)
- Custom encoder binary support
- CDDB and MusicBrainz integration
- CD-Text support
- Album cover support
- Sample offset correction
- Single-file ripping with cue sheet generation

## Requirements

- Qt6 / KDE Frameworks 6 (KF6)
- `libkcddb`
- `libcdio-paranoia`

## Optional Requirements

For native encoder support, the following binaries are required:

- **FLAC** – for `.flac` files  
- **LAME** – for `.mp3` files  
- **Opus** – for `.opus` files  
- **Ogg Vorbis** – for `.ogg` files  
- **FAAC** – for `.mp4` / `.aac` files  

Additionally, you can define custom profiles to support virtually any encoding binary.

## Building and Installing

To configure and build Audex:

```bash
cmake -DCMAKE_VERBOSE_MAKEFILE=ON -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr -B build/
cmake --build build/
```

To build and install in one step:

```bash
(sudo) cmake --build build/ --target install
```

To uninstall, use the traditional method:

```bash
(sudo) make uninstall
```

Or manually remove all installed files listed in the `install_manifest.txt`:

```bash
(sudo) xargs rm < build/install_manifest.txt
```

## Roadmap / TODO

- Automatic cover art search
- ReplayGain support
- Accurate ripping with automatic offset detection

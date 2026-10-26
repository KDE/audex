# Audex

Audex is an audio CD ripper for Linux built with Qt 6 and KDE Frameworks 6. It features a direct Linux SG_IO/MMC extraction engine, AccurateRip and CUETools database (CTDB) verification, and automatic image repair.

---

## Table of Contents
- [Key Features](#key-features)
- [Requirements](#requirements)
- [Build and Install](#build-and-install)
- [Quick Start](#quick-start)
- [Extraction & Verification Modes](#extraction--verification-modes)
- [Advanced Features](#advanced-features)
  - [HDCD Detection](#hdcd-detection)
  - [CD+G Graphics](#cdg-graphics)
  - [Pre-emphasis](#pre-emphasis)
  - [CTDB Repair](#ctdb-repair)
- [CLI Tool (`audex-cli`)](#cli-tool-audex-cli)
- [Verifying a Rip](#verifying-a-rip)
- [Changes from Audex 0.x](#changes-from-audex-0x)
- [Known Limitations](#known-limitations)
- [Developer Notes](#developer-notes)
- [Roadmap](#roadmap)
- [Author's Note](#authors-note)
- [Authors & Acknowledgments](#authors--acknowledgments)

---

## Key Features

### Extraction Engine
- **Secure & Normal Modes:** Multi-pass sector comparison with drive cache defeat and multi-round error recovery.
- **Verification:** Integrated AccurateRip (v1/v2) and CUETools Database (CTDB) verification.
- **Hardware Integration:** Per-device read offset correction, overread into lead-in/lead-out, and optional C2 error pointer support.
- **Disc Layout Support:** Supports Hidden Track One Audio (HTOA / Track 0), Enhanced CD / CD-Extra, Mixed Mode CDs, CD-Text, and Q sub-channel scanning.
- **Pregap Handling:** Pregaps can be placed at the end of the preceding track or at the start of their own track.

### Encoding & Tagging
- **Parallel Encoding:** Built-in asynchronous encoder pipeline with temporary `.part` file writing to prevent incomplete files.
- **Native Encoder Plugins:** Built-in support for FLAC (libFLAC), MP3 (LAME with Xing header), Opus (libopusenc), and uncompressed WAVE.
- **Custom Encoders:** Configurable CLI encoder integration via STDIN (e.g., `fdkaac`, `ffmpeg` ALAC) with presets in `encoderpresets.json`.
- **Comprehensive Tagging:** TagLib-powered tagging across all formats (ID3v2.4, Vorbis comments, MP4 atoms) with MusicBrainz IDs and embedded cover art.

### Metadata & Output
- **Multi-Source Metadata:** MusicBrainz disc ID lookups, CD-Text, and gnudb (CDDB) support with Cover Art Archive integration.
- **Metadata Caching:** Lookups are cached locally in `~/.cache/audex/online` using full TOC requests to avoid disc ID collisions.
- **Output Profiles:** Flexible profiles control track splitting or full-disc image output, file/folder naming, cue sheets, M3U playlists, log files, and post-rip automation scripts.

---

## Requirements

Audex runs exclusively on Linux and requires kernel SG_IO interface access (`/dev/sr*`).

### Build Dependencies
- **Build System & Compiler:** CMake ≥ 3.16 and a C++20 compiler (GCC or Clang).
- **Frameworks:** Qt ≥ 6.6 (Core, Gui, Widgets, Concurrent, Network, DBus) and KDE Frameworks 6.
- **Tagging:** TagLib ≥ 1.12 (TagLib 2.x required for cover art embedding across all formats).

### Encoders

WAVE is built into Audex. All other formats come as plugins. A plugin that needs a library is built only if CMake finds that library; if the plugin is missing later on, its profiles are disabled but keep their settings.

| Format | Library | Debian / Ubuntu | Fedora | Arch |
| --- | --- | --- | --- | --- |
| WAVE | built in | – | – | – |
| AIFF | none (always built) | – | – | – |
| FLAC | libFLAC | `libflac-dev` | `flac-devel` | `flac` |
| MP3 | LAME | `libmp3lame-dev` | `lame-devel` | `lame` |
| Opus | libopusenc | `libopusenc-dev` | `libopusenc-devel` | `libopusenc` |

All of them also work in a sandbox (Flatpak, Snap).

### Custom Encoders

Other formats (such as Ogg Vorbis, AAC, ALAC, WavPack, or Monkey's Audio) rely on custom encoders—command-line tools that accept WAVE input via stdout/stdin. Because sandboxed Audex instances cannot execute external binaries, custom encoding is unsupported. It is recommend ripping to FLAC first and converting the files post-rip outside the sandbox. FLAC preserves the lossless audio, metadata tags and embedded cover art.

### Drive Access
The user account requires read/write access to the optical device node (typically via the `cdrom` or `optical` group, or systemd-logind seat ACLs). Write access is required for commands like `SET CD SPEED`.

Audex holds an advisory lock on the device node during extraction.

---

## Build and Install

```sh
cmake -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo -DCMAKE_INSTALL_PREFIX=/usr
cmake --build build
sudo cmake --install build
```

To uninstall:
```sh
sudo xargs rm < build/install_manifest.txt
```

### Sandboxed Packages
When compiled with `-DAUDEX_SANDBOX=ON` (e.g., for Flatpak), custom encoders and post-rip commands are disabled. Profiles configured with custom encoders automatically fall back to WAVE output.

### Running from Build Directory
Point Audex to freshly built plugins before running without installation:
```sh
AUDEX_ENCODER_PLUGIN_PATH=$PWD/build/plugins/audex_encoders ./build/bin/audex
```

Search order for encoder plugins:
1. `$AUDEX_ENCODER_PLUGIN_PATH`
2. `<application dir>/plugins/audex_encoders`
3. `<Qt plugin paths>/audex_encoders`
4. `${KDE_INSTALL_PLUGINDIR}/audex_encoders`

---

## Quick Start

1. **Configure Read Offset:** Open *Settings -> Configure Audex... -> Device settings*, select your drive, and click **Fetch from AccurateRip**.
2. **Review Extraction Defaults:** Keep *Secure mode* and *Cache defeat* enabled. Enable *C2 error pointers* only if supported reliably by your drive.
3. **Select an Output Profile:** Choose or create a profile defining file formats, naming schemes, cue sheets, and playlists.
4. **Insert CD & Rip:** Audex reads TOC/CD-Text, fetches metadata from MusicBrainz, and presents track details. Click **Rip**.
5. **Check Rip Log:** Ensure tracks report `Copy OK` and AccurateRip status shows `accurately ripped`.

---

## Extraction & Verification Modes

Three primary settings control disc extraction:
- **Secure mode:** Configured per drive under *Device settings*.
- **AccurateRip / CTDB Verification:** Configured under *General settings*.

| | Verification Off | Verification On |
| --- | --- | --- |
| **Secure Mode Off** | Fast single pass. | Fast single pass, verified post-rip. |
| **Secure Mode On** | Secure dual pass with byte comparison. | Single pass per track if confirmed by database; falls back to secure dual pass if unconfirmed. |

### Secure Mode Off
Each sector is read once. Re-reads occur only on hardware read errors or C2 error flags up to *Max. read retries*.

```text
  disc ──► read once ──────────────────────────► encode ──► file
               │
               └─ read error ─► re-read sector (up to max retries)
```

### Secure Mode On, Verification Off
Discs are read in 512-sector windows. Every window is read twice with drive cache defeat performed in between.
- **Matching passes:** Kept immediately.
- **Differing / Unreadable sectors:** Trigger recovery (up to 5 rounds of 16 reads). A sector is accepted when 8 reads match within a round. Unrecoverable sectors are output as best-copy or silence and logged as suspicious positions.
- **C2 Enabled:** Replaces the second pass. Sectors without C2 errors are accepted on the first read.

```text
  window of 512 sectors
    ┌─ read 1 ─┐
    │          ├─► compare ─── equal ─────────────────────► keep
    └─ read 2 ─┘      │
     (cache defeat)   └─ different or unreadable
                           │
                           ▼
                      recovery: up to 5 rounds of 16 reads
                           │
                           ├─ 8 reads agree in a round ──► keep
                           └─ no agreement ──► best copy / silence (suspicious position)
```

### Secure Mode On, Verification On
Tracks are initially read once and checked against AccurateRip and CTDB checksums:
- **Confirmed track:** Accepted after a single pass (AccurateRip confidence ≥ 2 or CTDB confidence ≥ 2).
- **Unconfirmed track:** Discarded and re-extracted in full secure dual-pass mode.

```text
  look up disc in AccurateRip and CTDB
                           │
  track 1 ─► read once ─► confirmed by database? ── yes ─► keep (1 read)
  track 2 ─► read once ─► confirmed by database? ── no ──┐
  track 3 ─► read once ─► confirmed by database? ── yes ─► keep (1 read)
                                                           │
                           ┌───────────────────────────────┘
                           ▼
  track 2 ─► discard first read ─► re-read track in secure mode
```

**Full-Disc Images:** Single-pass audio is written directly to the image while unconfirmed tracks are securely extracted to temporary files. Post-rip, Audex rewrites the image, replacing unconfirmed sections with secure extractions.

### Verification Details
AccurateRip (v1/v2) and CTDB checksums are computed on-the-fly during extraction without requiring extra disc passes.

```text
  before rip ──► query AccurateRip and CTDB
  during rip ──► compute AccurateRip v1/v2 and CTDB CRCs during extraction
  after rip  ──► compare checksums, generate rip log, trigger CTDB repair if enabled
```

---

## Advanced Features

### HDCD Detection
Audex detects High Definition Compatible Digital (HDCD) encoded audio during initial disc scan and tags tracks (`HDCD=1`) or log entries accordingly. Bit-perfect rips preserve HDCD control codes in the LSBs.

To decode HDCD files post-rip via FFmpeg:
```sh
ffmpeg -i track01.flac -af hdcd -sample_fmt s32 -c:a flac track01_hdcd.flac
```

### CD+G Graphics
Karaoke CD+G graphic sub-channels (300 pack commands/sec) are automatically detected and saved as `.cdg` files alongside audio tracks.
- **Sub-channel Recovery:** Sub-channels are read twice with error-matching recovery, auto-deinterleaving, and Q-channel offset correction.

To render `.cdg` graphics with audio into a video via FFmpeg:
```sh
ffmpeg -i "01 - Artist - Title.cdg" -i "01 - Artist - Title.flac" \
       -map 0:v -map 1:a \
       -vf "scale=1200:864:flags=neighbor,fps=25" \
       -c:v libx264 -pix_fmt yuv420p -c:a copy -shortest \
       "01 - Artist - Title.mkv"
```

### Pre-emphasis
Discs with pre-emphasis mastering are flagged in the GUI, cue sheets (`FLAGS PRE`), and file tags (`PRE_EMPHASIS=1`). Audio data remains unaltered to preserve bit-exact verification.

To apply de-emphasis post-rip via FFmpeg:
```sh
ffmpeg -i track01.flac -af aemphasis=type=cd -c:a flac track01_deemph.flac
```

### CTDB Repair
For full-disc image rips, Audex can repair corrupted audio using CTDB Reed-Solomon parity data.
1. If CTDB verification fails post-rip, Audex downloads parity recovery data (~300 KB max).
2. The image is corrected in-memory and rewritten only if the resulting CRC matches the database.
3. Up to 8 corrupted 16-bit values per 10-sector column (~1 second of continuous damage) can be restored.

---

## CLI Tool (`audex-cli`)

`audex-cli` provides headless extraction and verification without KDE dependencies (requires Qt 6 only).

### Basic Commands
```sh
audex-cli info                 # Display drive and disc TOC information
audex-cli offset               # Measure and save read offset
audex-cli rip -o ~/music/disc  # Rip full disc to WAV + cue + log
audex-cli verify <log>         # Verify an existing rip log
```

### Read Offset Measurement
`audex-cli offset` measures hardware read offset against AccurateRip entries and saves the result per drive model in `~/.config/audex-cli.conf`:
```text
HL-DT-ST BD-RE WH16NS60 = +6  # discs: <MusicBrainz IDs disc>
```

### CLI Ripping Options
`audex-cli rip` outputs WAV files (`01.wav`, `02.wav`), `disc.cue`, and `disc.log` using secure dual-pass mode and AccurateRip verification.

```sh
audex-cli rip -o ~/music/disc               # Rip all tracks
audex-cli rip -o ~/music/disc -t 1,2,5-7,9  # Rip specific tracks (0 = HTOA)
audex-cli rip -o ~/music/disc --offset +6   # Override drive offset
```

Key options: `-d/--device <node>`, `-t/--tracks <list>`, `-o/--output <dir>`, `--fast`, `--c2`, `--overread`, `--speed <x>`, `--pregaps-with-track`, `--cdg`, `--overwrite`, `-q/--quiet`, `--no-accuraterip`.

### CLI Log Verification
Re-verify existing logs and audio files offline or online:
```sh
audex-cli verify "Artist - Album.log"
audex-cli verify --offline --quiet */*.log
audex-cli verify --json "Artist - Album.log"
```

Exit Codes:
- `0`: All files intact and verified against database.
- `1`: Files modified or database contradicts rip.
- `2`: Files intact but unconfirmed (e.g., unknown pressing).
- `3`: Verification failed (missing audio files, network error).
- `64`: Invalid usage or missing read offset.

---

## Verifying a Rip

Verify existing lossless rips (FLAC, WAV) at any time via **Audex -> Check Rip...**.
- **Integrity Check:** Verifies audio file CRCs against values stored in the `.log` file.
- **Database Re-check:** Performs fresh queries against AccurateRip and CTDB to confirm previously unknown pressings.
- **Relocation Friendly:** Looks in the log's recorded path first, then falls back to the current log directory.

---

## Changes from Audex 0.x

- **No cdparanoia Dependency:** Replaced entirely by the native Linux SG_IO extraction engine.
- **Built-in Encoding:** Internal thread-managed FLAC, MP3, and Opus encoders replace external binaries.
- **Custom Encoder Pipeline:** Legacy Ogg Vorbis and AAC profiles use custom STDIN commands.
- **Direct Metadata Integration:** MusicBrainz, CD-Text, and gnudb (CDDB) are built-in (removed `libkcddb` dependency).
- **Per-Device Settings:** Multi-drive configuration with individual offsets and parameters.

---

## Known Limitations

- Operating system limited to Linux (SG_IO interface).
- Drives lacking "accurate stream" support (pre-2000 CD-ROMs) experience sample jitter after seeks.
- SCSI drives requiring vendor-specific non-MMC commands are unsupported.
- CTDB repair is currently restricted to full-disc image profiles.

---

## Developer Notes

### Test Suite
The extraction engine is verified using an automated test framework in `tests/` operating against a simulated drive (`sim/`).

```sh
cmake --build build --target audex_tests
ctest --test-dir build --output-on-failure
```

Direct test execution:
```sh
./build/bin/audex_tests              # Run all tests
./build/bin/audex_tests offset c2    # Filter by keyword
```

### Source Code Layout

| Directory | Responsibilities |
| --- | --- |
| `core/` | Engine core: TOC parsing, read plan, `RipEngine`, CRC32, AccurateRip, CTDB, CD-Text, MMC parsers, rip report generation. |
| `device/` | Linux SG_IO device transport, MMC command layer (`MmcSectorReader`). |
| `sim/` | Simulated optical drive for unit tests. |
| `encoding/` | Encoder/decoder interfaces, WAVE/custom encoders, threaded output pipelines, TagLib integration. |
| `plugins/` | Native encoder plugins (FLAC, MP3/LAME, Opus). |
| `metadata/`, `online/` | Data models (`CDInfo`), MusicBrainz/gnudb importers, Cover Art Archive, AccurateRip/CTDB clients. |
| `utils/` | Worker threads, CTDB image repair, Solid drive detection (`DiscController`), profile request builders, cue/playlist generators. |
| `models/`, `widgets/`, `dialogs/` | KF6 graphical user interface. |

### Translation Bridge and Localization Tests

Audex uses two translation mechanisms: the KDE GUI uses `i18n()` and `i18nc()` from KLocalizedString, while the extraction engine and native encoder plugins use Qt's `QCoreApplication::translate()` to avoid dependencies on KDE Frameworks.

The `EngineTranslator` (`utils/enginetranslator.cpp`) bridges these mechanisms. It is installed at application startup and forwards translations from recognized Qt contexts (`Audex::...` and the native encoder contexts) to the KDE gettext domain `audex`. Contextual translations and Qt placeholders such as `%1` are preserved. Unrelated Qt translation contexts remain untouched.

#### Integration Testing

The integration test (`tests/test_i18n.cpp`) verifies that GUI, engine and encoder messages use the same translations. It covers:

- German and French translations.
- English fallback when no translation is available.
- Runtime language switching between German, French and English.
- Contextual translations and placeholder substitution.
- Isolation of unrelated Qt translation contexts.

The test uses dedicated `.po` catalogs in `tests/i18n/`, compiled into `.mo` files by CMake using GNU gettext. These fixtures are independent of installed system translations, ensuring reproducible results.

Build and run the test with:

```sh
cmake --build build --target audex_i18n_tests
ctest --test-dir build -R '^audex_i18n_tests$' --output-on-failure
```

**Note:** The integration test validates the translation bridge, not the completeness of the production catalogs in `po/`. New engine and encoder messages must still be included in the regular translation extraction and localization process.

### Platform Support
The extraction engine builds as a static library (`audexengine`) depending on Qt Core, Gui, Network, and TagLib. Non-Linux builds compile SCSI device access as a stub while maintaining full simulation test coverage (e.g., on FreeBSD).

What is planned: To enable full FreeBSD support, probably only a CAM transport (camlib/pass(4)) implementing the device/scsidevice.h interface is needed. Contributions are more than welcome.

---

## Roadmap

- MusicBrainz disc submission.
- Native FreeBSD CAM transport (`camlib`/`pass(4)`) support.
- CTDB repair enhancements using suspicious sector locations.

---

## Author's Note

The release of Audex 27.04 fulfills a long-term vision for Audex. It combines a completely modern, custom extraction engine developed over 10 years with the refined Audex GUI.

- **Homepage:** <https://apps.kde.org/audex>
- **Bug Tracker:** <https://bugs.kde.org/enter_bug.cgi?product=audex>
- **License:** GPL-3.0-or-later

---

## Authors & Acknowledgments

- **Maintainer & Main Developer:** Marco Nelles, with contributions from Craig Drummond, Elson, and others. Special thanks to credativ GmbH.
- **CTDB Verification & Repair:** Based on CUETools by Gregory S. Chudov (<https://github.com/gchudov/cuetools.net>).
- **HDCD Detection:** Ported from FFmpeg's `af_hdcd` filter by Chris Moeller.
- **CD-R/RW ATIP Lookup:** Based on `libburn` by Thomas Schmitt (<https://libburnia-project.org>).

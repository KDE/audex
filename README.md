# Audex

Audex is an audio CD ripper for Linux built with Qt 6 and KDE Frameworks 6.

The extraction engine talks to the drive directly through Linux SG_IO/MMC
commands. It rips securely and the key points are (more details below):

- read offset correction
- cache defeat
- optional C2 error pointers
- AccurateRip verification
- CUETools database (CTDB) support: Verification and repair (of image rips)

---

## Key points

### Extraction engine

- **Secure mode.** The disc is read in windows of 512 sectors. Each window is
  read twice, with the drive cache defeated in between, and the two copies are
  compared byte by byte. Sectors that differ or that fail to read are going
  through error recovery. Recovery makes up to 5 rounds of 16 reads each.
  A sector is accepted as soon as 8 reads within one round are identical.
  With AccurateRip or CTDB verification turned on, a track that a database
  confirms after the first read is kept without the second read (see _Read
  modes_).
- **Normal mode.** Used when secure mode is not enabled. Each sector is read
  once. Only sectors that report a read error are retried.
- **C2 error pointers** are optional, if the drive supports them. A copy is
  trusted only if the drive flags no C2 errors. In secure mode this replaces
  the second pass.
- **Read offset correction** for any sample offset (AccurateRip convention).
  The _Fetch from AccurateRip_ button in the device settings looks up your
  drive's offset in the AccurateRip drive database.
- **Overread** into the lead-in and lead-out, if the drive can do it. If it
  can't, Audex falls back to silence automatically.
- **Cache defeat.** Before every re-read, Audex reads a sector far away on the
  disc, so the drive has to read the sector again and clear the cache.
- **Suspicious positions.** If a sector cannot be recovered, the best copy (or
  silence) is used, and the position is reported with its time in the track.
- **AccurateRip.** The v1 and v2 checksums are computed while ripping and
  compared with the AccurateRip database afterwards. This step is optional.
- **CUETools database (CTDB).** As a second, independent verification the
  rip is looked up in the CTDB, which covers discs AccurateRip does not
  know. Other pressings of the disc are recognized from the CTDB alone
  (offsets up to ±5879 samples). The log also tells you whether repair data
  is available for the disc.
- **CTDB repair** (optional for image rips only). If the CTDB contradicts the
  rip, Audex downloads the recovery data of the disc and repairs the image,
  see the chapter below.
- **Rip log** in a well-known style. It contains the drive, the settings, the
  TOC, for each track the peak level, CRC32, CRC32 without null samples,
  the AccurateRip v1/v2 checksums, suspicious positions and statistics.
- **Disc layouts.** Hidden track one audio (HTOA, ripped as track 0),
  Enhanced CD / CD-Extra and Mixed Mode CDs are supported. Audex reads the
  full TOC and falls back to the formatted TOC if needed.
- **CD-Text.** Audex reads all blocks and languages, supports ISO 8859-1 and
  MS-JIS, checks the CRC of every pack and takes UPC/EAN and ISRC codes from
  CD-Text.
- **Q sub-channel.** An optional scan detects pre-gaps and further indexes
  (they are written to the cue sheet) and reads ISRC and media catalog
  numbers where CD-Text does not provide them.
- **Pregaps in track files.** A track's pregap goes to the end of the
  previous track (the usual way) or to the start of its own track. AccurateRip
  and the CTDB check the tracks as the TOC defines them either way. Images
  always contain the whole disc.
- **Per-drive settings.** Multiple drives are supported. Read mode, offset,
  C2, cache defeat, overread and read speeds (including a separate speed for
  error recovery) are stored per each device.
- **HDCD detection.** When a disc is inserted, and for every track while
  ripping (rip log, optional tag). For decoding see the chapter below.
- **CD+G.** Karaoke graphics in the sub-channel are detected and can be
  saved as `.cdg` files next to the audio files, see the chapter below.
- **Pre-emphasis.** Flagged tracks are marked, tagged and flagged in the cue
  sheet. For de-emphasis see the chapter below.
- **CD-R.** Detects CD-R media.

### Encoding and tagging

- **Built-in encoding.** Audex encodes while it rips: every file gets its own
  worker thread, and ripping pauses if the encoders fall behind. Files are
  written as `name.part.ext` and renamed only when they are complete so an
  interrupted rip never leaves a file that looks finished.
- **Native encoder plugins**, loaded at run time:
  - **FLAC** (libFLAC, with verification and reserved padding for tags;
    FLAC files can also be read back for the CTDB repair)
  - **MP3** (LAME, VBR or CBR, with a LAME/Xing header for gapless playback)
  - **Opus** (libopusenc)
- **WAVE** is built in.
- **Custom encoders.** Any command-line encoder that reads WAVE from standard
  input works. `$o` in the command stands for the output file, for example:
  - `fdkaac --silent -m 5 -o $o -`
  - `ffmpeg -loglevel error -f wav -i - -c:a alac $o`

  The custom encoder widget offers some presets. The presets
  come from `encoderpresets.json` (installed to the application data folder);
  add your own to `~/.local/share/audex/encoderpresets.json`.

- **Tagging** with TagLib the same way for every format: ID3v2.4 for MP3,
  Vorbis comments for FLAC/Opus, and MP4 atoms for M4A. The tags include
  the MusicBrainz IDs and optionally an embedded cover.

### Metadata and output

- **Metadata** from MusicBrainz (disc ID lookup with a choice between
  releases) or from CD-Text. Cover art comes from the Cover Art Archive.
- **Profiles** control:
  - the output: one file per track, or the whole disc as one image file
    with a cue sheet (FLAC or WAVE)
  - file and folder naming schemes
  - playlists (M3U)
  - cover files and the rip log
  - a custom command to run after the rip

---

## Requirements

Audex runs on Linux only. The engine accesses the drive through the
kernel's SG_IO interface (`/dev/sr*`).

### Build dependencies

- CMake ≥ 3.16 and a C++20 compiler (GCC or Clang)
- Qt ≥ 6.5: Core, Gui, Widgets, Concurrent, Network, DBus
- KDE Frameworks 6
- TagLib ≥ 1.12 (TagLib 2.x works as well)

### Optional: native encoder plugins

Each plugin is built only if CMake finds its library. Without any of them,
only WAVE and custom encoders are available. When configuring CMake prints
the plugins it is going to build:
`Encoder plugins to build: FLAC=... MP3=... OPUS=...`.

Profiles whose plugin is missing stay in the list, but are disabled. The
main window names the missing plugins, the settings are kept, and the
profiles work as soon as the plugin is installed.

| Plugin | Library    | Debian / Ubuntu  | Fedora             | Arch         |
| ------ | ---------- | ---------------- | ------------------ | ------------ |
| FLAC   | libFLAC    | `libflac-dev`    | `flac-devel`       | `flac`       |
| MP3    | LAME       | `libmp3lame-dev` | `lame-devel`       | `lame`       |
| Opus   | libopusenc | `libopusenc-dev` | `libopusenc-devel` | `libopusenc` |

### Drive access

Your user needs read and write access to the drive's device node.
Distributions usually grant it through the `cdrom` or `optical` group or
through systemd-logind's seat ACLs. Write access is needed for commands such
as SET CD SPEED. Without it Audex cannot change the read speed.

---

## Build and install

```sh
cmake -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo -DCMAKE_INSTALL_PREFIX=/usr
cmake --build build
sudo cmake --install build
```

To uninstall:

```sh
sudo xargs rm < build/install_manifest.txt
```

### Running from the build directory

The encoder plugins are loaded at run time. Audex looks for them in these
places, in this order:

1. `$AUDEX_ENCODER_PLUGIN_PATH` (colon-separated list of folders)
2. `<application dir>/plugins/audex_encoders`
3. `<Qt plugin paths>/audex_encoders`
4. the install location, `${KDE_INSTALL_PLUGINDIR}/audex_encoders` (for
   example `/usr/lib/x86_64-linux-gnu/qt6/plugins/audex_encoders`)

To try a build without installing it point Audex to the freshly built plugins:

```sh
AUDEX_ENCODER_PLUGIN_PATH=$PWD/build/plugins/audex_encoders ./build/bin/audex
```

On startup Audex prints the search folders and the plugins it loaded in the
terminal.

---

## First steps

1. **Set the read offset.** Open _Settings → Configure Audex..._, go to the
   _Device settings_ page, select the drive and click _Fetch from
   AccurateRip_. Without the correct offset AccurateRip cannot confirm your rips.
1. **Keep secure mode and cache defeat turned on** (the defaults). Turn on C2
   only if you know your drive reports C2 errors reliably.
1. **Choose or create a profile.** A profile sets the output (one file per
   track or a disc image), the format, the naming scheme and the extra files
   you want (playlist, cue sheet, log, ...).
1. **Insert a CD.** Audex reads the TOC and the CD-Text, looks up the disc on
   MusicBrainz and shows the metadata. Check it and then click _Rip_.
1. **Read the result in the rip log.** Every track should say _Copy OK_, and
   the AccurateRip section should say _accurately ripped_.
1. **Optional: CTDB repair.** For image profiles, turn on _Repair with the
   CUETools database_ on the _Image_ tab of the profile.

---

## Read modes

Three settings decide how Audex reads a disc:

- _Secure mode_, per drive on the _Device settings_ page,
- _Verify ripped tracks with AccurateRip_ and _Verify ripped tracks with the
  CUETools database_, on the _General settings_ page. Below, "verification
  on" means at least one of the two.

|                     | verification off            | verification on                                               |
| ------------------- | --------------------------- | ------------------------------------------------------------- |
| **Secure mode off** | fast: one read              | fast: one read, verified afterwards                           |
| **Secure mode on**  | secure: two reads, compared | one read per track, secure only where no database confirms it |

### Secure mode off

Every sector is read once. Only sectors the drive reports as unreadable (or
with C2 errors, if C2 is turned on) are read again, up to _Max. read
retries_ times.

```text
  disc ──► read once ──────────────────────────► encode ──► file
               │
               └─ read error ─► read the sector again (max. read retries)
```

With verification turned on, the rip is checked against the databases
afterwards (see _Verification_ below). A track that does not match is only
reported, not read again.

### Secure mode on, verification off

The disc is read in windows of 512 sectors. Every window is read twice. The
drive cache defeated in between and the two copies are compared byte by
byte.

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
                           ├─ 8 reads in a round agree ────► keep
                           └─ no agreement ─► best copy, reported as
                                              suspicious position
```

With C2 turned on, the drive's C2 check replaces the second read. A sector
without C2 errors is kept after the first read.

Secure mode detects errors that change between reads. A drive that returns
the same wrong data every time (for example by interpolating a scratch)
passes the comparison. Only AccurateRip or the CTDB can catch this.

### Secure mode on, verification on

This is the fastest way to a proven rip. Every track is read once first and
its checksum is compared with the databases right away:

```text
  look up the disc in AccurateRip and the CTDB, recognize the pressing
                           │
  track 1 ─► read once ─► confirmed by a database? ── yes ─► keep (1 read)
  track 2 ─► read once ─► confirmed by a database? ── no ──┐
  track 3 ─► read once ─► confirmed by a database? ── yes ─► keep (1 read)
                                                           │
                           ┌───────────────────────────────┘
                           ▼
  track 2 ─► discard the first read, read the whole track
             again in secure mode (as above)
```

A track is kept after one read if the read left no suspicious positions and
one of the databases confirms it:

- **AccurateRip:** the checksum matches at least two submissions.
- **CTDB:** the track checksum matches entries with a total confidence of at
  least two. It is compared at the read offset and, if AccurateRip found
  another pressing, at its offset. Old CTDB entries without track checksums
  cannot confirm single tracks.

Audex falls back to plain secure mode for all tracks in these cases:

- Neither database can confirm tracks of this disc: AccurateRip does not
  know it (or its pressing) and the CTDB has no entry with track checksums.
- The profile creates a disc image. The image is one continuous file, so a
  single track cannot be discarded and written again.

**Disc images** work the same way. The image is one continuous file, so an
unconfirmed track cannot simply be discarded: its first read stays in the
image for the moment, and its secure extraction goes into a temporary file
next to it. After the rip Audex reads the image back and writes it again
with the secure extractions in place of the first reads:

```text
  image ─► tracks 1, 2, 3 read once, written into the image
           track 2 not confirmed ─► read again securely ─► temporary file
                           │
  after the rip   read the image back, replace track 2, write it again
                  (same encoder, settings and tags; the cue sheet is unchanged)
```

If only the CTDB knows the disc and your copy is a pressing with a different
offset, no track matches and every track is read twice. The result is still
correct; it just takes longer.

### Verification

Both databases are looked up before the rip. The checksums for both are
computed while reading, so verification needs no extra reads.

```text
  before the rip   AccurateRip: look up, recognize the pressing
                   CTDB: look up
                          │
  rip              every read computes AccurateRip v1/v2 and the CTDB CRC
                   (secure mode: keep confirmed tracks, see above)
                          │
  after the rip    AccurateRip: compare ──────────────► rip log
                   CTDB: compare, also other pressings
                   up to ±5879 samples ───────────────► rip log
                   image profile with repair: CTDB contradicts
                   the rip ─► repair (see CTDB repair)
                          │
  result           a warning for each database that contradicts the rip
```

The two databases are independent. The CTDB knows many discs that
AccurateRip does not have. A rip counts as confirmed by a database if its
tracks match there. It gets a warning if a database knows the disc but a
track does not match.

---

## HDCD

HDCD (High Definition Compatible Digital) was an audio encoding technology
that delivered 20-bit sound quality on standard 16-bit audio CDs. The
extra audio information was hidden within the lowest bits of the standard
audio signal using high-frequency control codes. To search for HDCD
encoded comapct audio discs, have a look at [Discogs](https://www.discogs.com/de/search?page=1&format_description_exact=HDCD&format_name_exact=CD).

Audex detects HDCD encoded discs and marks them with a small HDCD in the
main window. Detection needs only a few seconds of audio and runs in the
background when a disc is inserted.

While ripping, Audex checks every track completely: the rip log names the
HDCD encoded tracks (with peak extend and transient filter), and with _Tag
for HDCD encoded tracks_ (under _Tagging_, default `HDCD`) they get a tag
with the value `1`, so that they can be found and decoded later. An image
gets the tag if one of its tracks is HDCD encoded. Both need _Detect HDCD
encoded discs_.

Audex does not decode HDCD. A bit-perfect rip (which the AccurateRip
verification confirms) preserves the complete HDCD information in the LSBs.
Decoding can happen at any later point without touching the disc again,
e.g. with FFmpeg:

```sh
ffmpeg -i track01.flac -af hdcd -sample_fmt s32 -c:a flac track01_hdcd.flac
```

---

## CD+G

CD+G discs (mostly karaoke) carry next to the audio graphics in the R-W
sub-channels, e.g. lyrics, pictures, color changes in 300 small drawing
commands ("packs") per second in sync with the music.

**Detection.** With _Detect CD+G graphics_ (under _Reading the CD_ on the
_General settings_ page) Audex reads a few seconds of the sub-channel when
a disc is inserted and marks CD+G discs with a black CD+G badge in the main
window. Drives on which the drive test found no raw sub-channel are not
asked.

**Saving.** With Save CD+G graphics as .cdg files (under Ripping and
verification) the sub-channel is read after the audio and saved next to the
audio files under the same name: "01 - Artist - Title.flac" gets
"01 - Artist - Title.cdg". An image gets one .cdg file for the whole disc.
The setting depends on the detection: only discs it found with graphics
are read, the others cost no time. A disc it has not checked yet (a rip
started right after inserting it) is checked by the rip first.

The sub-channel has no checksums like AccurateRip and drives can not correct
it. Audex therefore reads it twice and reads sectors whose two reads differ
again until two reads agree. On the disc the packs are scrambled and
interleaved; most drives deliver them that way, a few de-interleave them.
Audex recognizes which and writes the packs in the order players expect.
Many drives also deliver the sub-channel a few sectors off; the Q channel in
the same data tells by how much and Audex corrects it. The drive test
(_R-W sub-channel (CD+G)_) shows whether a drive delivers the raw
sub-channel and by how many sectors it is off.

**Playing.** VLC, mpv play a `.cdg` file together with the audio file of
the same name. FFmpeg turns both into a video that plays everywhere
(the graphics are 300×216 pixels, here scaled up four times), e.g.:

```sh
ffmpeg -i "01 - Artist - Title.cdg" -i "01 - Artist - Title.flac" \
       -map 0:v -map 1:a \
       -vf "scale=1200:864:flags=neighbor,fps=25" \
       -c:v libx264 -pix_fmt yuv420p -c:a copy -shortest \
       "01 - Artist - Title.mkv"
```

---

## Pre-emphasis

Some CDs, mostly from the 1980s, were mastered with pre-emphasis: the treble
was raised before the recording and has to be lowered again on playback.
The disc flags this per track in the TOC. Most CD players de-emphasize on
their own. You can find a list of audio cd releases mastered with
preemphasis [at this page](<https://www.studio-nibble.com/cd/index.php?title=Pre-emphasis_(release_list)>).

Audex marks such discs with a purple PRE-EMPHASIS badge in the main window
(the tooltip names the tracks) and shows the flag in the _Edit Data_
dialog. The cue sheet of an image carries `FLAGS PRE` for every flagged
track. Ripped files get a tag, by default `PRE_EMPHASIS=1`; the name is set
on the _General settings_ page; an empty name writes no tag. An image is
tagged only if all of its tracks are flagged. Note, that WAVE files cannot
be tagged.

Audex does not de-emphasize. The rip stays bit-perfect and can be verified
with AccurateRip and the CTDB. De-emphasis can be done afterwards, e.g. with
FFmpeg:

```sh
ffmpeg -i track01.flac -af aemphasis=type=cd -c:a flac track01_deemph.flac
```

The filter works in floating point, the result is written as 24-bit FLAC.

---

## CTDB repair

The CUETools database (CTDB) does not only store checksums. For most discs
it also stores recovery data: Reed-Solomon parity over the whole audio of
the disc. With it, a damaged rip can be restored to the exact audio of the
database entry, even where the drive delivered wrong data consistently and
secure ripping could not notice it.

Audex uses the recovery data for image rips (profiles with the output
_Whole disc as one image file_). Turn it on with _Repair with the CUETools
database if differences are found_ on the _Image_ tab of the profile. This
also turns on the CTDB verification for its rips.

How it works:

1. The rip runs as usual. Afterwards the image is verified against the CTDB.
   If the database confirms the rip, nothing else happens: nothing is
   downloaded, read or written.
2. If the CTDB contradicts the rip, Audex reads the image back and compares
   it with the database entries. Other pressings of the disc are handled as
   well.
3. The recovery data of a matching entry is downloaded (at most 368 KB) and
   the image is corrected.
4. The image is written again with the same encoder, settings and tags. It
   replaces the original only if the result matches the CRC of the database
   entry. Optionally the unrepaired image is kept as
   `<name>.unrepaired.<suffix>` (_Keep the unrepaired image file_).

The rip log gets a section _CUETools database (CTDB) repair_ with the entry
used, the number of corrected values and their positions (disc time). Audex
then reads the repaired image back and verifies it again: the track CRCs and
AccurateRip checksums in the log are those of the repaired image, each
repaired track names its CRC from before, and AccurateRip and the CTDB are
checked once more.

How much can be repaired: the audio is split into rows of 10 sectors, and up
to 8 wrong 16 bit values per column of this grid can be corrected. That is
about one second of damaged audio in one piece (about half of it for older
database entries with less recovery data). Damage in several places adds up
if the places are a multiple of 10 sectors apart.

Repairing takes a few seconds for a whole CD: about four seconds for the
recovery calculation, plus reading the image back and writing it again.

The recovery data and the way it is computed come from CUETools by Gregory
S. Chudov. Audex implements it on its own and was checked against the
original CUETools code, so it uses the same data as CUETools does.

---

## Changes compared to Audex 0.x

- **cdparanoia is no longer used or needed.** All reading is done by the new
  engine.
- **Encoding happens inside Audex.** The external `lame`, `flac` and `opusenc`
  binaries are no longer needed. The native plugins replace them.
- **Ogg Vorbis and AAC (FAAC) profiles are no longer built in.** Use a custom
  command instead, for example `oggenc -Q -q 6 -o $o -`.
- **Custom commands receive the audio as WAVE on standard input.**
- **Metadata comes from MusicBrainz or CD-Text.** CDDB/gnudb is no longer
  supported.
- **Multiple device support and device settings are stored per device.**

---

## Known limitations

- Linux only (SG_IO).
- Drives without "accurate stream" (mostly CD-ROM drives from before about
  the year 2000) miss the position by a few samples after every seek. Audex
  does not realign such reads: secure rips flag almost every sector, fast
  rips are shifted. The drive test finds such drives, and Audex warns
  clearly.
- Drives are read with the MMC command READ CD only; old SCSI drives that
  need vendor-specific read commands are not supported.
- The CTDB repair works for image rips only and needs the whole disc. It
  does not yet use the positions of suspicious sectors, which would allow
  repairing more damage.
- After a CTDB repair the track CRCs and the AccurateRip results in the rip
  log still refer to the image before the repair.

---

## Some developer notes

### Testing

The extraction engine is covered by an automated test suite in `tests/`.
Everything runs against the simulated drive, so no optical drive and no
test CD is needed.

```sh
cmake --build build --target audex_tests
ctest --test-dir build --output-on-failure
```

You can also run the test binary directly. With arguments, only the tests
whose name contains one of them run:

```sh
./build/bin/audex_tests              # all tests
./build/bin/audex_tests offset c2    # only matching tests
```

#### What is tested

- **Read plan and offset correction**, including negative offsets and
  offsets larger than one sector (`makeReadPlan()`, `floorDiv()`)
- **`correctedLbaForDriveSector()`**, the mapping used to report error
  positions in disc time
- **Cache defeat positioning**: distinct far reads that keep their distance,
  alternate between in front of and behind the range, stay inside the audio
  area, and report (`fits`) when a short disc forces them closer together
- **A clean secure rip with cache and offset**: byte-exact output, cache
  hits on the second pass, cache defeats counted
- **Recovery of a noisy sector**: the two passes mismatch, the majority
  vote resolves the sector, no suspicious position remains
- **A permanently unreadable sector**: zero-filled in the output, reported
  as a suspicious position, everything around it byte-exact (in both secure
  and fast mode)
- **Consistently wrong data** (drive interpolation): the two passes agree,
  so secure reading cannot detect it, this limitation is pinned by an
  explicit test; only AccurateRip/CTDB verification catches such errors
- **Fast mode recovery**: a transient read error is retried transparently
- **C2 error pointers**: a flagged sector is re-read, a clean copy is
  trusted immediately; data that stays flagged ends up suspicious
- **Overread**: lead-in and lead-out supply the offset margin; a drive
  without overread capability falls back to silence (padded sectors)
- **Track splitting**: one contiguous read is split at exactly the track
  boundary
- **verifyFirst**: an unconfirmed track is discarded and re-extracted
  securely; a confirmed track is kept after a single read
- **Unstable overread**: lead-out data that differs between reads falls
  back to silence instead of going through error recovery
- **Q sub-channel, short reads**: the ISRC is still found when reads stop
  at Q sectors the drive cannot read
- **Q sub-channel**: pre-gaps and indexes are found also with drives that
  deliver the Q frames shifted by some sectors
- **CTDB pressings**: the CTDB CRC at every offset (sliding CRC) finds other
  pressings without AccurateRip, also when single tracks differ
- **CTDB track confirmation**: a single track is confirmed at the read
  offset and at the offset of a pressing found via AccurateRip, not at
  arbitrary offsets; damaged tracks and old entries without track checksums
  confirm nothing
- **CTDB recovery data** against reference values of the original CUETools
  code: syndromes and CRCs at several offsets, the old parity format, offset
  search, corrections for three pressings and the repair limits
- **Image repair**: a damaged WAVE image is restored byte-exact (also for
  another pressing and with the original kept); an image with too much
  damage or without differences is left alone; an image read back has the
  checksums of its extraction (CRCs, AccurateRip also at other offsets,
  CTDB margins), a changed sample changes its track only
- **Tracks read again in an image**: the engine delivers the whole image
  in the first pass and exactly the track's audio on the secure re-read;
  the re-read tracks replace their first read in a WAVE image byte-exact,
  audio beyond the end of the image is refused
- **CD+G**: interleave and scramble of the R-W packs, detection, the
  extraction with the drive's sub-channel shift, with drives that do and do
  not de-interleave, with read errors (read again until two reads agree), a
  disc without graphics and a drive without the raw sub-channel; the drive
  test measures the shift
- **Accurate stream**: the drive test finds a drive whose reads are off
  after a seek
- **Pregaps with their own track**: the files move, the AccurateRip and
  CTDB checksums stay those of the TOC ranges (also for other pressings and
  for a single track); a file whose pregap another track's checksums did
  not confirm is read again
- **Medium**: the ATIP of a CD-R and a CD-RW, the manufacturer from the
  lead-in start, no ATIP for a pressed CD, the current profile of GET
  CONFIGURATION, the CD-R write bit of the capabilities page
- **Sense data**: the information field (e.g. the block of a recovered
  error) in fixed and descriptor format

#### Writing tests

The suite uses its own tiny framework (`tests/test_framework.h`), not
Qt Test. A test is one macro - no list to maintain:

```cpp
AUDEX_TEST("secure mode recovers a noisy sector")
{
    // ...
    AUDEX_CHECK(t, result.completed);
    AUDEX_EQUAL(t, result.segments.size(), 1);
    AUDEX_EQUAL_DATA(t, capture.segments.at(0), expected);
}
```

`AUDEX_EQUAL_DATA` compares two `QByteArray` and reports the first
differing byte (and the sector it belongs to).

The tests are deterministic: Defects of the simulated drive either fire
with probability 1.0 or follow a scripted pattern
(`Defect{DefectKind::Noisy, 1.0, c2, {true, true, false, ...}}` - the
n-th physical read of the sector is affected iff `pattern[n % size]`).
No test depends on a particular random sequence, so a green run is
reproducible on every machine. The helpers in `test_ripengine.cpp`
(`runRip()`, `expected()`, `expectedWithSilentSector()`) build the drive,
run the engine and produce the reference bytes a correct extraction must
deliver (`expectedAudio()` in `sim/`).

### Source layout

| Folder                            | Contents                                                                                                                                                                        |
| --------------------------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `core/`                           | Engine without platform or KDE dependencies: TOC, read plan, `RipEngine`, checksums (CRC32, AccurateRip), CTDB recovery data, disc IDs, CD-Text and MMC parsers, rip report     |
| `device/`                         | Linux SG_IO device, MMC command layer, `MmcSectorReader`                                                                                                                        |
| `sim/`                            | Simulated drive for tests and demos                                                                                                                                             |
| `encoding/`                       | Encoder and decoder interface and registry, WAVE and custom encoders, threaded `EncodedOutputs`, `TagWriter` (TagLib)                                                           |
| `plugins/`                        | Native encoder plugins: FLAC, MP3 (LAME), Opus                                                                                                                                  |
| `metadata/`, `online/`            | Disc model (`CDInfo`), metadata fields, CD-Text and MusicBrainz import, Cover Art Archive, AccurateRip and CTDB clients                                                         |
| `utils/`                          | Rip job (worker thread), CTDB image repair, drive handling via Solid (`DiscController`), profile → request translation (`RipRequestBuilder`), cue sheets, playlists, hash lists |
| `models/`, `widgets/`, `dialogs/` | KF6 user interface                                                                                                                                                              |

The engine is built as a static library (`audexengine`) that depends only on
Qt Core, Gui and Network plus TagLib. The KDE application links against it.

### Platform Support

Audex is developed and tested on Linux. The only platform-specific code is the
SCSI transport (device/scsidevice.cpp), which relies on Linux SG_IO. All other
components are platform-independent Qt code.
On non-Linux platforms (mainly FreeBSD) the transport compiles to a stub that
returns a clean runtime error ("Direct disc access is not supported...").
The CI test suite runs fully on FreeBSD using the simulated drive without
physical hardware.

What is planned: To enable full FreeBSD support, probably only a CAM transport
(camlib/pass(4)) implementing the device/scsidevice.h interface is needed.
Contributions are more than welcome.

---

## Roadmap

- MusicBrainz submission
- FreeBSD support
- CTDB repair using the positions of suspicious sectors (more repairable damage)

---

## Some personal words to Audex 27.04 with complete new engine

The new release with Audex 27.04 comes close to the vision I have had for
Audex since the beginning.
Audex stands for AUDio EXtractor and was always intended to be a tool that
offers the best ripping technologies some day, just like the top players on
other operating systems. Unfortunately, no other useful options besides
cdparanoia, which is limited in different ways, have ever existed.

The new Audex now is the symbiosis of a completely newly written extraction
engine, which I have been developing from time to time over the last 10 years
within a minimal, never-published prototype application, and the well-known
GUI of Audex.
Due to personal reasons the new extraction engine never reached production
level, but over the last year I was able to finalize it with moderate use
of AI. This helped me a lot with the routine work, refactoring the old GUI
code, speeding up testing and eliminating a few bugs in the engine.

The user interface is well-known with profiles, naming schemes, metadata
editing and so on.

- Homepage: <https://apps.kde.org/audex>
- Bug tracker: <https://bugs.kde.org/enter_bug.cgi?product=audex>
- License: GPL-3.0-or-later

---

## Authors

Marco Nelles (maintainer, main developer), with contributions from Craig
Drummond, Elson and others. Special thanks to credativ GmbH.

The CTDB verification and repair follow CUETools by Gregory S. Chudov
(<https://github.com/gchudov/cuetools.net>, GPL-2.0-or-later), who also
runs the CUETools database.

The HDCD detection is ported from FFmpeg's af_hdcd filter by Chris Moeller
(https://ffmpeg.org, BSD-3-Clause).

The CD-R/RW manufacturer codes (lead-in start of the ATIP) and their lookup
are taken from libburn by Thomas Schmitt (<https://libburnia-project.org>,
GPL-2.0-or-later).

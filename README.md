# Windows FFmpeg prebuilt libraries

This branch preserves the x64 LGPL shared FFmpeg build used by Starfish.
Fetch the submodule at its pinned commit; no FFmpeg compilation or GNU build
tools are needed on the consuming Windows machine or CI runner.

The original build is BtbN's
`n8.1.3-14-g330caae0c1-20261006`, licensed under LGPL-3.0-or-later.
`manifest.json` records the original archive checksums and source revisions.
`BUILD_CONFIGURATION.txt` records the configuration reported by the binary.

The `x64/` prefix contains development headers, import libraries, export
definitions, and runtime DLLs for `avcodec`, `avformat`, `avutil`, `swscale`,
and `swresample`. Executables, unrelated FFmpeg libraries, and MinGW import
archives are omitted. x86 binaries are not included in this revision.

Ship the five DLLs from `x64/bin/` together. All packaged libraries and source
archives are listed in `SHA256SUMS`. The DLLs are upstream build artifacts;
Starfish selects software decoders and does not need their optional hardware
decoder integrations.

MSVC consumers should generate import libraries from the supplied `.def`
files with the matching MSVC `lib.exe`. This avoids the import-library
compatibility issue with Release `/OPT:REF` described in
https://ffmpeg.org/platform.html#Linking-to-FFmpeg-with-Microsoft-Visual-C_002b_002b.
For example, from an x64 Native Tools prompt:

```bat
lib.exe /nologo /machine:x64 /def:x64\lib\avcodec-62.def /out:avcodec.lib
```

The original LGPL text is retained in `LICENSE.txt`. Corresponding FFmpeg
source and the upstream build recipes are retained under `sources/`.

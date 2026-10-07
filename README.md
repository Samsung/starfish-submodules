# Windows FFmpeg prebuilt libraries

This branch preserves x86 and x64 LGPL shared FFmpeg builds used by
Starfish. Fetch the submodule at its pinned commit; no FFmpeg compilation
or GNU build tools are needed on the consuming Windows machine or CI runner.

| Prefix | Provider | FFmpeg build | Compiler |
| --- | --- | --- | --- |
| `x86/` | System233/ffmpeg-msvc-prebuilt | 8.1.3 | MSVC |
| `x64/` | BtbN/FFmpeg-Builds | n8.1.3-14-g330caae0c1-20261006 | MinGW GCC |

Both builds are licensed under LGPL-3.0-or-later. `manifest.json` records
provenance, archive checksums, and source revisions separately for each
architecture. `BUILD_CONFIGURATION.txt` and `BUILD_CONFIGURATION_X86.txt`
record the configurations reported by the respective binaries.

Each architecture prefix contains its own development headers, import
libraries, and runtime DLLs for `avcodec`, `avformat`, `avutil`, `swscale`,
and `swresample`. Executables and unrelated FFmpeg libraries are omitted.
Ship the five DLLs from the matching `bin/` directory together. All package
files are listed in `SHA256SUMS`.

The x86 import libraries are native MSVC libraries and can be linked
directly, including with Release `/OPT:REF`. A 32-bit MSVC executable was
linked against all five libraries and run on Windows; H.264 and AAC
software decoder lookup was also verified.

For x64, MSVC consumers should generate import libraries from the supplied
`.def` files with x64 MSVC `lib.exe`. This avoids the import-library
compatibility issue with Release `/OPT:REF` described in
https://ffmpeg.org/platform.html#Linking-to-FFmpeg-with-Microsoft-Visual-C_002b_002b.
For example, from an x64 Native Tools prompt:

```bat
lib.exe /nologo /machine:x64 /def:x64\lib\avcodec-62.def /out:avcodec.lib
```

The original LGPL texts are retained in `LICENSE.txt` for x64 and
`x86/LICENSE.txt` for x86. Corresponding FFmpeg source and upstream build
recipes are retained under `sources/`.

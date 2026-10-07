# Source provenance

`ffmpeg-330caae0c1.tar.gz` contains FFmpeg revision
`330caae0c1acccd2222edc52a05940c574561ce5`, which matches the version reported
by the packaged DLLs. FFmpeg is not patched by the BtbN build script.

`ffmpeg-builds-20261006.tar.gz` contains BtbN's build scripts at
`autobuild-2026-10-06-13-06`. Its dependency scripts record upstream source
locations, revisions, configuration, and patches for the third-party
libraries included in the upstream build. The build-script license is
separate from the LGPL license of FFmpeg.

The exact FFmpeg configuration is recorded in `../BUILD_CONFIGURATION.txt`.
The download URLs and checksums for both source archives are recorded in
`../manifest.json` and `../SHA256SUMS`.

The x86 package uses FFmpeg release `n8.1.3`, source revision
`1041abdc962f4cc4f394aa8de9dc5236c0c3b9e7`, retained in
`ffmpeg-n8.1.3.tar.gz`. Its MSVC build recipes and port patches are retained
in `ffmpeg-msvc-prebuilt-8.1.3.tar.gz` at the provider's `ffmpeg-8.1.3` tag.
The x86 binary configuration is recorded in `../BUILD_CONFIGURATION_X86.txt`.
The manifest records each architecture's sources and hashes independently.

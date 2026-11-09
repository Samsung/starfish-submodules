# Starfish pixel tests

The image-compared tests of the Starfish (LWE) `internal_test` suite: a page
renders, and its screenshot is compared with an expected PNG, either by the page
itself (`testImgDiff`) or by the test driver. They are Starfish's own tests, kept
out of the main tree because of their size. Starfish mounts this repository at
`tests/pixel`, and its lists (`tool/reftest/cairo/internal*.res`,
`tool/pixel_test/svg.res`) name the pages by that path.

| Here | What it is |
|---|---|
| `css/{position,position2,display,short-hand,zindex,zindex_img,background}`, `click` | Mostly pages that step through style combinations with `css/framework.js`, with one expected image per combination |
| `css/*`, `svg`, `canvas`, ... | Other pixel tests, by the category they test |

Tests that judge themselves, crash tests and the fixtures they share stay in
Starfish's `tests/` tree. Update this repository together with the Starfish
change that needs it, then commit the new submodule pointer in Starfish.

<p align="right">
  <a href="README.zh_CN.md">简体中文</a> · <strong>English</strong>
</p>

# Assets

This directory stores reusable fonts, images, music, and sound effects, organized by asset type.

Keep each asset in the matching subdirectory and document its destination, naming, integration method, and source/license. Do not mix binary assets with Markdown documentation.

## Fonts

Store reusable font files and generated font sources in `fonts/`.

- Use descriptive names that include the family, weight, size, and format when relevant.
- Document the source, license, character range, conversion command, and expected destination.
- Check Flash and internal-RAM impact before adding a font; the ESP32-C3 has no PSRAM.
- Do not commit fonts whose license does not permit redistribution.

### USELESS POETRY

`fonts/PoetrySubset-Regular.otf` is a renamed modified SC-face subset of
[Noto Sans CJK](https://github.com/notofonts/noto-cjk), from Debian's
`NotoSansCJK-Regular.ttc` face 2 (Google copyright, SIL OFL 1.1;
`fonts/OFL.txt`). `fonts/poetry_font_16.c` is the Flash-resident LVGL font:
16 px, 4 bpp plain/uncompressed, 20 px line height, 5 px baseline, no kerning.
`fonts/poetry_symbols.txt` contains printable ASCII and all corpus/UI/error
characters; external arbitrary text is not accepted. Every label selects this
font, and startup checks actual descriptors plus a known-missing negative case.
`fonts/poetry_assets.json` records source/output hashes.

Rebuild with Pillow 12.3.0, fontTools 4.61.1, CairoSVG 2.8.2 and the official
`lv_font_conv` 1.5.3, installed under ignored `work/`:

```sh
npm install --prefix work/font-tools lv_font_conv@1.5.3
python tools/generate_poetry_assets.py --font /path/to/NotoSansCJK-Regular.ttc
python tools/generate_poetry_assets.py --check
```

The script verifies source coverage and invokes `--size 16 --bpp 4 --no-compress
--no-kerning --format lvgl`. Run static inventory/provenance and actual LVGL tests
from [the application guide](../docs/development/useless-poetry.md) after edits.
Physical glyph appearance still needs on-device acceptance.

## Images

Store reusable source images and generated display assets in `images/`.

| File | Dimensions and format | Use and source |
| --- | --- | --- |
| [`images/home.jpg`](images/home.jpg) | 3840 × 2160, JPEG | Product hero image embedded in both project README files to foreground AI Passport and its open, maker-oriented identity. |
| [`images/readme-hardware-specs.png`](images/readme-hardware-specs.png) | 2172 × 724, PNG RGBA | Optional technical infographic retained as a reference asset; it is no longer used as the homepage hero. Generated for this repository with the built-in image generation tool on 2026-09-17; the six labels and values were checked against the documented hardware contract. |
| [`images/logo-wordmark.png`](images/logo-wordmark.png) | 1648 × 336, PNG RGBA | Transparent black wordmark extracted from the repository's original `images/logo.png`; embedded in both project README files for light backgrounds. |
| [`images/logo-wordmark-dark.png`](images/logo-wordmark-dark.png) | 1648 × 336, PNG RGBA | White version of the extracted wordmark, used by the README `<picture>` element when GitHub is in dark mode. |

- Use descriptive names and document dimensions, pixel format, conversion steps, and destination.
- Prefer formats suitable for the 240 × 320 RGB565 display and account for Flash and internal RAM.
- Preserve editable sources where licensing permits, and record the source and license.
- Never commit device QR secrets, credentials, or personal data in images.

Original MIT-licensed `poetry_otter.svg`, `poetry_foot.svg` and `poetry_ripple.svg`
produce `poetry_assets.c`: 154 x 88, 24 x 22 and 106 x 26 RGB565+A8 layers,
50,520 bytes in Flash. CairoSVG rasterizes at 3x; Pillow downsamples with Lanczos,
then concatenates RGB565 and A8 planes in LVGL order. Firmware moves Flash-backed
objects without a full-screen RAM canvas or frame sequence.

## Music and sound effects

Store reusable music and sound-effect sources in `music/`.

- Document the source, license, sample rate, bit depth, channels, conversion command, and destination.
- Prefer 16 kHz, 16-bit mono PCM when it matches the current BSP audio path.
- Check Flash and internal-RAM cost before embedding audio; stream or chunk long recordings.
- Do not commit media without redistribution permission.

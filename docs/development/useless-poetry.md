[简体中文](useless-poetry.zh_CN.md) · **English**

# USELESS POETRY

An independent offline ESP-IDF 5.5.3 application for AI Passport: ESP32-C3,
8 MB Flash, no PSRAM, 240 x 320 portrait display. Development is isolated on
`feature/useless-poetry`. The BSP, original demo sources and default partitions
are retained. The build selects `poetry_app.c` instead of the test-menu entry.
No radio, audio, microphone or cloud service starts; no API key is required.

## Experience and controls

Four manually selected personalities have independent original corpus banks:
ABSURD (everyday objects abandoning their duties), MELANCHOLY (gentle melancholy
with a comic landing), CORPORATE (office rituals), COSMIC (space and domestic
needs). Their visual motifs are crooked ink dashes on warm paper, blue-gray rain,
green ruled office paper, and constellations on dark blue. All labels use the
16 px application CJK font. Each poem has 2–4 lines.

| Screen | UP short | DOWN short | OK short |
| --- | --- | --- | --- |
| Home | Previous personality | Next personality | Generate |
| Poem | Previous personality/home | Next personality/home | Toggle favorite |
| History | Previous record | Next record | Toggle favorite |
| Basket | Previous favorite | Next favorite | Remove favorite |
| Waiting | Consumed | Consumed | Consumed |

Outside waiting, hold UP for history, DOWN for the basket and OK for home.
Lists clamp at either end; personalities wrap. Holds begin at 500 ms via the BSP's
per-button configuration. Releasing a hold does not trigger a short action.
ADC polling is 10 ms with three debounce ticks (30 ms). The adapter consumes
debounced PRESS/LONG/RELEASE, so rapid taps are handled individually without the
BSP's double-click classification dropping an action. The worker clock also
grades holds because the component's repeat-tap state omits LONG events.
A press begun during waiting
is consumed even if released after the poem appears. The shared timer callback
only enqueues timestamped events. On queue overflow, wait for all keys to release;
boot-held keys are also guarded. Backlight dims to 10% after 30 seconds idle and
turns off after 60 seconds; the first press while dark only wakes the display.
There is no automatic deep sleep. Battery read failure displays `--%`.

## Generation, rarity and animation

Each of 16 personality/rarity banks contains four openings, two optional middle
lines and four endings around one coherent motif. Six variant bits produce 64
combinations per bank, 1024 overall. This is curated offline composition, not an
on-device language model. Repeats are possible; the finite corpus permits testing
every string and layout.

| Rarity | Probability | Ticket range | Content/display |
| --- | --- | --- | --- |
| Common nonsense | 65% | 0–64 | Everyday absurdities; personality accent |
| Quality nonsense | 25% | 65–89 | Lyrical motifs; explicit rarity label |
| Philosophical accident | 9% | 90–98 | Existence/evidence/absence; ochre label |
| Universe-class nonsense | 1% | 99 | Cosmic domestic catastrophes; gold/ochre label |

Rejection sampling of `esp_random()` removes modulo bias. The rarity ticket is
independent of the verse variant. Radios stay off; randomness is for entertainment,
with no cryptographic claim. The percentages describe independent draws, not a
quota or a guarantee of a rare poem within 100 attempts.

The wait scene is installed while holding the BSP LVGL lock, then timed by
`esp_timer_get_time()` for 2000 ms. It cannot finish before the deadline. The
25 ms application loop and panel scheduling add physical tolerance; measure
2.00–2.15 seconds from first visible wait frame to poem on the actual board.
There is exactly one visible text label, the specified Chinese extraction
sentence; no spinner, bar or extra computation copy. Reveal the poem before
writing history, with Flash operations outside the LVGL lock.

Original MIT-licensed SVG art is rasterized into three Flash-resident RGB565+A8
layers (50,520 bytes). The supine otter's body, hands and iced tea stay fixed.
One hind foot moves vertically by two pixels either way over a 1600 ms cycle;
the other foot stays fixed. The entire body drifts six pixels during the wait,
accompanied by slight translucent ripple motion. No full-screen canvas, sprite
frame sequence or animation timer is allocated. A persistent LVGL object tree
avoids page churn; one application task owns model state and UI calls use the
BSP LVGL lock.

## Persistence contract

Newest-first history holds 32 poems; the basket independently holds 24 favorites.
History eviction never removes favorites. A full basket refuses additions without
overwriting existing entries. Empty/full/error states are explicit. Favorites
are confirmed after storage succeeds; failed writes restore the previous
in-memory favorite state. Unsaved history remains readable in RAM with a warning.

Partitions remain NVS `0x9000/0x6000`, PHY `0xF000/0x1000`, factory
`0x10000/0x7F0000`. Only namespace `useless_poetry`, key `records_v1`, is used.
The 696-byte snapshot includes magic, corpus version, next ID, selected persona,
counts, 12-byte little-endian records and CRC32. Records store ID, variant, persona
and rarity; **corpus v1 is immutable** so persisted poems reconstruct identically.
History and favorites are serialized together. NVS blob replacement/commit provides
the platform's power-loss behavior, which still needs hardware acceptance.
Unknown version, CRC errors, invalid counts/IDs and conflicting records disable
writes while retaining the original blob. No error recovery erases NVS. Future
corpus edits need an explicit migration/version strategy.

## Reproduce and validate

Activate the existing ESP-IDF 5.5.3 environment, then run:

```sh
./tools/validate.sh --static
./tools/validate.sh --firmware
./tools/validate.sh
```

Static tests cover all combinations, rarity boundaries, exact waiting deadlines,
input grading, list limits, capacity, eviction, serialization roundtrip,
single-byte corruption at every snapshot position, ID overflow and resource
provenance. The asset inventory/hash gate needs only standard-library Python.
NVS fault injection covers initialization, opening, oversized/corrupt reads,
write/commit failures, handle reopening and restart recovery without an erase API.
The firmware gate builds tracked defaults in isolation and verifies partition MD5,
offsets, bounds, merged bytes and matching ELF in a content-addressed bundle.

After dependency resolution, test the actual pinned LVGL 9.5.0 renderer:

```sh
cmake -S tests/poetry_lvgl -B work/lvgl-test -G Ninja
cmake --build work/lvgl-test -j 6
mkdir -p work/preview
work/lvgl-test/test_poetry_ui work/preview
```

This uses the firmware's 24 KiB pool and single 40-line RGB565 draw buffer.
It verifies glyph descriptors (including a known-missing negative case), widget
fonts, all poem text boxes, one-label waiting, 80 animation frames, empty/error
screens and 300 page transitions. Optional PPM screenshots come from the actual
application UI and BSP rounding mask, not a browser mockup. Startup logs record
internal free heap and largest block at startup, with heap and task-stack readings
every 30 seconds; actual board values remain unverified.
See [assets](../../assets/README.md) for sources, licenses and regeneration.

## Install only after approval

Use verified `build/FoloToy-AI-Passport-full.bin`, or the identical delivery file
`USELESS-POETRY-full.bin`, with the official AI Passport custom-firmware tool.
Select ESP32-C3 / 8 MB and write the complete image at `0x0`; never write the
app-only binary at `0x0`. Verify SHA-256 against the delivery report. Tool screens
vary by version; follow its documented custom-firmware flow.

A merged flash may reset NVS, history and favorites. To preserve them during a
compatible update, check the unchanged partition layout and use the **matching
verified bundle's** application image at `0x10000`. Do not use a different rebuild
or full-chip erase. A local alternative after explicit approval is:

```sh
python -m esptool --chip esp32c3 --port PORT --baud 460800 \
  write_flash --flash_size 8MB 0x0 USELESS-POETRY-full.bin
```

No flash, merge to main, release or official catalog publication is part of this
implementation. The PR remains for review pending user confirmation and hardware
acceptance.

## Physical acceptance checklist

- [ ] Record board revision, image SHA-256 and real port; obtain flash approval.
- [ ] Boot directly into the new home; battery reads a value or `--%`.
- [ ] Inspect all four palettes/motifs, Chinese/ASCII/digits and longest hints.
- [ ] Check 2/3/4-line poems and punctuation without missing/clipped glyphs.
- [ ] Generate 20 times and measure visible wait duration (2.00–2.15 s).
- [ ] Observe fixed arms/drink, one slow foot, horizontal drift and slight ripples.
- [ ] Wait scene has only the specified Chinese sentence, with no spinner/bar.
- [ ] Test rapid taps, each hold/release and a held key spanning the deadline.
- [ ] Test empty lists, clamped navigation, favorite add/remove and full basket.
- [ ] Generate over 32 poems; oldest history drops but its favorite survives.
- [ ] Restart/power-cycle; persona, order, exact poems and favorites survive.
- [ ] Interrupt power while saving; boot yields a valid old/new snapshot.
- [ ] Check rare labels/colors with controlled diagnostic tickets, then restore
      the release build (no user-facing probability override).
- [ ] Check 30 s dim, 60 s blank and a wake press with no poem/favorite action.
- [ ] Record free heap/largest block; run 30 minutes without resets/heap decline.
- [ ] Add device photos/results to the PR; physical rendering, timing, ADC,
      power consumption and NVS durability stay unverified until observed.

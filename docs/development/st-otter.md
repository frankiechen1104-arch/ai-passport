<p align="right"><a href="st-otter.zh_CN.md">简体中文</a> · <strong>English</strong></p>

# ST Otter firmware

This offline application starts from main on `feature/st-otter-pet`. It replaces
the demo entry point with a fresh 240 × 320 LVGL interface. The BSP and original
demo sources remain available as references; they are not the application UI.
The USELESS POETRY branch is outside this change.

## Controls and behavior

UP/DOWN clicks cycle the highlighted choice. OK click confirms; OK long returns
home without confirming a purchase. The home choices are feed, play, bath,
sleep, work, travel, wardrobe and collection. Work uses OK **PRESS** timestamps
for low-latency hits, not delayed clicks. Holding a key does not repeat strikes.

A conveyor round lasts exactly 60,000 monotonic milliseconds. The first brick
reaches the target at 1,400 ms; subsequent bricks arrive every 1,100 ms. The hit
window is ±160 ms. A hit consumes that brick; early presses and missed bricks
break the combo. Reward is 5 + hits + 2 × min(best combo, 20), preserving the
starter economy. Leaving early forfeits rewards; starting consumes 10 energy.
Gameplay rejects a new round below 10 energy. All other prices, unlock milestones
and stat changes follow the uploaded starter tables. These are implementation
defaults from that package, not additional user requirements.

The eight destination illustrations and six outfit sprites are independently
selectable. Clothes are previewed before purchase and retained in NVS. Every
outfit retains the complete blue fish head hood, extra-round belly and tiny
closed mouth. The bucket hat is an accessory to the hood. Collection records
visited destinations; visiting a locked destination requires cumulative joy,
which is separate from current mood. A pet with any stat under 20 sulks but
never dies.

## Time and storage

There is no new Wi-Fi, cloud dependency or RTC wiring. Cold power-off duration
is unknown and therefore earns no sleep or stat change. Only measured powered
session time counts. Sleep remains a pet state while the display stays powered;
it restores 12 energy per full powered hour, capped at eight hours. Calling this
pet sleep does not imply hardware deep sleep.

A version-2, 64-byte little-endian wire record includes checksum and range checks,
owned outfits, destination stamps, economy, fractional active hours and sleep
progress. It has no C-structure padding dependency. Meaningful actions queue a
save after 1.5 seconds; adjacent actions coalesce. Session time is checkpointed
every five minutes. A failed save retries after ten seconds and is visible in
the UI. Sudden power loss can lose the most recent unsaved action or five-minute
time fraction. NVS initialization errors never trigger a global erase. A corrupt
or unsupported save starts a new in-memory pet with an explicit warning; a later
successful action save replaces the application's record only.

## Assets and reproducibility

Sources and mapped images are in `assets/images/st_otter/`. Convert reviewed
atlases with `python3 tools/prepare_st_art.py` (Pillow). The RGB565 backgrounds
and RGB565+A8 sprites are mapped directly from Flash; they do not allocate
full-screen decoding buffers. Fonts use a 16 px, 4-bpp, uncompressed Noto Sans
CJK SC subset; run `python3 tools/prepare_st_font.py` with Pillow and fontTools.
The character inventory is generated from application source and verified in
host tests and at LVGL startup. See the assets index for provenance and license.

Run `./tools/validate.sh` in ESP-IDF 5.5.3. This includes the ST economy,
persistence corruption, sleep timing, conveyor boundary and resource tests,
in addition to the existing repository suite and merged-image validation.

## Installation and physical acceptance

The verified merged image is `build/FoloToy-AI-Passport-full.bin`, installed
at **0x0** on ESP32-C3 with 8 MB Flash. It includes the bootloader, partition
table and ST application. The partition layout stays unchanged. Merged flashing
can reset NVS because it includes padding over the data region. Segmented flashing
with matching images is needed to preserve compatible existing NVS data.
The `st-otter-v0.1` pre-release publishes the previously verified merged image
without rebuilding it; its embedded version remains `a06235e-dirty`. Release
attachments bind that binary to the committed source using per-file SHA-256
records. See the [release](https://github.com/frankiechen1104-arch/ai-passport/releases/tag/st-otter-v0.1).
No device flashing or main merge is performed.

Builds and host screenshots are not device acceptance. Pending physical checks:
Chinese text and artwork on ST7789, RGB565 colors, three-key latency, a full
60-second round, purchase/unlock navigation, NVS persistence after power cycling,
long-press back, battery availability and long-running heap stability.

#!/usr/bin/env python3
"""Dependency-free inventory, provenance and bitmap-layout regression checks."""
import hashlib
import importlib.util
import json
from pathlib import Path
import re
import unittest

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location("poetry_assets", ROOT / "tools/generate_poetry_assets.py")
generator = importlib.util.module_from_spec(spec)
spec.loader.exec_module(generator)

class PoetryAssetsTest(unittest.TestCase):
    def test_every_actual_string_is_in_inventory(self):
        chars = (ROOT / "assets/fonts/poetry_symbols.txt").read_text()
        self.assertEqual(chars, generator.inventory())
        self.assertIn("正在提取无意义", "".join(re.findall(r'"([^"\n]*)"', (ROOT / "main/poetry_ui.c").read_text())))
        self.assertNotIn("龘", chars)  # Required negative coverage case.
        self.assertEqual(len(chars), len(set(chars)))

    def test_assets_match_recorded_sources(self):
        manifest = json.loads((ROOT / "assets/fonts/poetry_assets.json").read_text())
        self.assertEqual(manifest["corpus_version"], 1)
        for path, expected in manifest["sha256"].items():
            self.assertEqual(hashlib.sha256((ROOT / path).read_bytes()).hexdigest(), expected, path)

    def test_layer_planes_and_small_ram_design(self):
        source = (ROOT / "assets/images/poetry_assets.c").read_text()
        expected = {"otter": (154, 88), "foot": (24, 22), "ripple": (106, 26)}
        total = 0
        for name, (w, h) in expected.items():
            data = re.search(rf'{name}_pixels\[\] = \{{(.*?)\}};', source, re.S).group(1)
            values = bytes(int(x, 16) for x in re.findall(r'0x([0-9a-f]{2})', data))
            self.assertEqual(len(values), w * h * 3)
            alpha = values[w*h*2:]
            self.assertIn(0, alpha)
            self.assertTrue(any(0 < a < 255 for a in alpha))
            total += len(values)
        self.assertLess(total, 52 * 1024)
        ui = (ROOT / "main/poetry_ui.c").read_text()
        animation = ui[ui.index("void poetry_ui_animate"):ui.index("void poetry_ui_render")]
        self.assertNotIn("lv_image_set_src", animation)  # Hands/drink pixels do not animate.
        self.assertNotIn("lv_spinner", ui)
        self.assertNotIn("lv_bar", ui)

    def test_font_is_flash_resident_plain_4bpp(self):
        source = (ROOT / "assets/fonts/poetry_font_16.c").read_text()
        self.assertTrue(re.search(r'static\s+(?:LV_ATTRIBUTE_LARGE_CONST\s+)?const uint8_t glyph_bitmap', source), "font bitmap must be static const")
        self.assertIn(".bitmap_format = 0", source)
        self.assertIn(".bpp = 4", source)
        self.assertIn(".line_height = 20", source)
        self.assertIn(".base_line = 5", source)
        self.assertNotIn(".type = LV_FONT_FMT_TXT_CMAP_FORMAT0_FULL", source)

if __name__ == "__main__":
    unittest.main()

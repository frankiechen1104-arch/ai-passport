"""Dependency-free inventory and mapped-resource format checks."""
from pathlib import Path
import json, re, unittest
ROOT=Path(__file__).resolve().parents[1]
class Assets(unittest.TestCase):
    def test_font_inventory(self):
        points=set(json.loads((ROOT/'assets/fonts/st_font_inventory.json').read_text()))
        text=''.join(p.read_text() for p in (ROOT/'main').glob('st_*.c'))+(ROOT/'main/main.c').read_text()
        self.assertTrue({ord(c) for c in text if ord(c)>127}.issubset(points))
        font=(ROOT/'assets/fonts/st_font_16.c').read_text()
        self.assertIn('LV_FONT_FMT_TXT_CMAP_SPARSE_TINY',font)
        self.assertIn('st_font_16', (ROOT/'main/st_ui.c').read_text())
    def test_image_payloads(self):
        directory=ROOT/'assets/images/st_otter'
        for i in range(8):self.assertEqual((directory/f'background_{i}.raw').stat().st_size,240*320*2)
        for i in range(6):self.assertEqual((directory/f'outfit_{i}.raw').stat().st_size,128*164*3)
        self.assertEqual(len(re.findall(r'LV_IMAGE_HEADER_MAGIC',(directory/'st_art.c').read_text())),14)
if __name__=='__main__':unittest.main()

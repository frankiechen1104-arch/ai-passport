#!/usr/bin/env python3
"""Assemble actual headless LVGL frames into a reviewable animation/contact sheet."""
from pathlib import Path
import argparse
from PIL import Image,ImageDraw,ImageFont
parser=argparse.ArgumentParser();parser.add_argument('frames',type=Path);args=parser.parse_args()
root=Path(__file__).resolve().parents[1];out=root/'assets/images/st_otter/animation'
font=ImageFont.truetype(str(root/'assets/fonts/NotoSansCJKsc-Regular.otf'),16)
scenes=[(0,0,'待机：呼吸、眨眼、挥爪',100),(0,1,'喂食：接鱼、闭唇进食',36),(0,2,'洗澡：搓肚皮、抖水泡',36),(0,3,'玩耍：举爪、小跳跃',36),(0,4,'睡觉：闭眼呼吸',36)]
scenes += [(o,5,'换装：六套服装都有动作',18) for o in range(6)]
scenes += [(0,6,'旅行：场景淡入淡出',36),(0,7,'搬砖：蓄力、拍砖命中',10),(0,8,'搬砖：失误反馈',10),(0,9,'结算：举爪庆祝',36)]
frames=[]
for outfit,action,title,count in scenes:
 for i in range(count):
  source=args.frames/f'anim_{outfit}_{action}_{i:03d}.ppm'
  canvas=Image.new('RGB',(240,346),(11,25,36));canvas.paste(Image.open(source),(0,26))
  ImageDraw.Draw(canvas).text((8,3),title,font=font,fill=(238,246,255))
  frames.append(canvas)
palette_board=Image.new('RGB',(240*6,346))
for i,index in enumerate([0,110,150,210,330,400]):palette_board.paste(frames[index],(i*240,0))
palette=palette_board.quantize(colors=192)
frames=[im.quantize(palette=palette,dither=Image.Dither.NONE) for im in frames]
frames[0].save(out/'preview.gif',save_all=True,append_images=frames[1:],duration=80,loop=0,optimize=True,disposal=1)
contact=Image.new('RGB',(720,692),(11,25,36))
for i,index in enumerate([65,111,150,210,390,460]):
 contact.paste(frames[min(index,len(frames)-1)].convert('RGB'),((i%3)*240,(i//3)*346))
contact.save(out/'preview.png')
print('Actual LVGL animation preview:',len(frames),'frames;', (out/'preview.gif').stat().st_size,'bytes')

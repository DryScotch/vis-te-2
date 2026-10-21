import math
from pathlib import Path
from PIL import Image
base=Path(__file__).resolve().parents[1]/'assets'/'scenes'
seqs={'blue_halo/base':96,'dual_sub/near':32,'dual_sub/far':32,'dual_sub/glow':32,'ribbon_wave/back':64,'ribbon_wave/front':64,'liquid_pixel/field_back':64,'liquid_pixel/field_front':64,'pulse_heart/body':48,'pulse_heart/aura':48}
for path,n in seqs.items():
 d=base/path
 for group in range(math.ceil(n/8)):
  sheet=Image.new('RGBA',(1280,360),(0,0,0,0))
  for j in range(8):
   frame=group*8+j
   if frame>=n:break
   with Image.open(d/f'frame_{frame:03d}.png') as im:
    sheet.paste(im.convert('RGBA'),((j%4)*320,(j//4)*180))
  sheet.save(d/f'sheet_{group:03d}.png',optimize=True)
print('packed 68 4x2 sheets')

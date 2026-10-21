#!/usr/bin/env python3
"""Minimal stdlib-only preflight check (runs in VitaSDK GitHub container)."""
from pathlib import Path
import struct
root=Path(__file__).resolve().parents[1]
sequences={
 'blue_halo/base':96,
 'dual_sub/near':32,'dual_sub/far':32,'dual_sub/glow':32,
 'ribbon_wave/back':64,'ribbon_wave/front':64,
 'liquid_pixel/field_back':64,'liquid_pixel/field_front':64,
 'pulse_heart/body':48,'pulse_heart/aura':48,
}
base=root/'assets'/'scenes'
for folder,n in sequences.items():
 p=base/folder
 names=[p/f'frame_{i:03d}.png' for i in range(n)]
 # GitHub build-only upload deliberately excludes full-resolution authoring frames.
 # Full editable source pack carries them and will validate all numbering + sizing.
 available=[t for t in names if t.exists()]
 if available and len(available)!=n:
  missing=[t.name for t in names if not t.is_file()]
  raise SystemExit(f'Incomplete source frames for {folder}: {missing[:8]}')
 for t in available:
  with t.open('rb') as fh: header=fh.read(24)
  if header[:8]!=b'\x89PNG\r\n\x1a\n' or struct.unpack('>II',header[16:24])!=(320,180):
   raise SystemExit(f'Invalid source PNG dimensions: {t}')
 for i in range(n//8):
  sheet=p/f'sheet_{i:03d}.png'
  if not sheet.is_file():raise SystemExit('Missing atlas: '+str(sheet))
  with sheet.open('rb') as fh: header=fh.read(24)
  if header[:8]!=b'\x89PNG\r\n\x1a\n' or struct.unpack('>II',header[16:24])!=(1280,360):
   raise SystemExit('Invalid atlas dimensions: '+str(sheet))
 print(f'OK {folder} {n} frames and {n//8} atlases')
for t in (base/'dual_sub/cabinet.png',base/'ribbon_wave/background.png'):
 if not t.is_file():raise SystemExit('Missing '+str(t))
print('PASS: all 68 sheets + static scene art validated; optional source frames checked if included')

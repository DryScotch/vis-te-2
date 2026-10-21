# Vita Motion Lab — v1.0 Alpha (GitHub VPK project)

**This is a complete, experimental PS Vita application source package** intended for your GitHub Actions → VPK → VitaShell workflow, not a design document or precompiled VPK.

**Status:** 5-scene VitaSDK app, generated scene assets, UI, audio analysis, local file player and build automation are implemented. **Not compiled with the actual VitaSDK and not installed/tested on a PS Vita.** A local Clang syntax check with API stubs passed, and asset checks passed. Expect possible GitHub compilation/API issues and report the Actions log so they can be fixed. **Keep your previous working v0.7 VPK installed.** This uses a **new title ID `VMLA10001`**, so it should install as a separate bubble rather than overwrite the previous app.

## What is actually implemented

- Five art-directed 320×180 3D-*style* (offline 2.5D/sprite-based) hero modes:
  - **Blue Halo:** 96 selected frames extracted from your 7.17-second reference video (15 FPS, 4:3 geometry preserved within 16:9 canvas).
  - **Dual Sub Reactor:** one rigid cabinet asset, **32 independent shape/depth positions for each of the two subwoofer cones**, and 32 discrete glow frames. It uses an angled, stylized interpretation of your supplied dual-sub photo.
  - **Ribbon Wave:** 64 actual back-ribbon frames and 64 independent front-ribbon frames.
  - **Liquid Pixel:** 64 back-field and 64 front-field frames with coherent, animated metaball shapes.
  - **Pulse Heart:** 48 anatomical heart reference frames (forward/reverse loop) and 48 aura frames.
- **544 authored animation frames** (10 sequences) packed into **68 transparent/opaque sprite-sheet textures** (8 frames per 1280×360 atlas). Runtime only loads current sheets into a 2-slot cache for each layer and reuses them for 8 animation frames at a time. The **full editable source download** also contains the 544 separate PNG frame files. The smaller **GitHub upload build download** omits those redundant source frames, keeping only the runtime sprite sheets and static layers so you can upload it more easily; both compile to the same runtime scene assets.
- Mic Mode: Vita microphone live input (256-frame buffer at 16kHz), 1024-sample FFT/band envelopes for bass/mids/highs; no recording.
- Music Player: MP3/WAV/FLAC decode-only miniaudio, stereo output at 48kHz, 2048-sample FFT for audio reactivity, thread-based playback, pause/resume, wave snapshot.
- Loop Mode: offline animation + simulated ambient pulse.
- Main menu, scene browser, music browser, source selection, color palette page, settings, about screen, and minimal HUD; 12 color schemes affecting UI and reactive accents; saved settings in `ux0:data/VitaMotionLab/settings.ini`.
- Nearest-neighbor rendering of pixel art on a **960×544 Vita display** (draw at 960×540 with 2-pixel top and bottom black letterbox).
- Audio changes **local scene elements**, never stretches the entire scene.

**Honest visual limitations:** The 3D appearance is achieved via layered offline artwork and finite cone-depth images, *not real-time 3D models or per-pixel shaders*. The subwoofer movement is a frame-based approximation, not a physical speaker simulation; purple and other palettes do not recolor the underlying blue/reference video art. The video-derived Heart uses a ping-pong loop. The 8 secondary v0.7 scenes are intentionally **not in this v1.0 Alpha**, per the five-hero production specification. There is still quality tuning to do on real OLED hardware.

## Upload and compile through GitHub

1. Download the **GitHub upload (build-only) ZIP** and **extract** it; do **not** upload the ZIP as one file to your repository. The optional larger **full editable source ZIP** contains extra source frames you do not need for compiling.
2. Create a new repository (or a new branch of `vita-te`, preserving v0.7). If using an existing repository, **replace the project root contents** rather than merging unrelated v0.7 source files. Upload all extracted directories, including the hidden **`.github/`** directory. The top level must contain `CMakeLists.txt`, `src/`, `assets/`, `.github/workflows/build-vpk.yml`, `tools/` and this README.
3. GitHub → repository → **Actions** → **Build PS Vita VPK** → **Run workflow** (on the uploaded branch). Pushing to `main` also builds automatically.
4. If green, open the run → **Artifacts** → download `Vita-Motion-Lab-v1-Alpha-VPK`; extract it to obtain `VitaMotionLabAlpha.vpk`.
5. Transfer the `.vpk` to `ux0:data/` using VitaShell FTP (press SELECT) and FileZilla; then in VitaShell select the VPK and press X to install.
6. Create `ux0:data/CabinMusic/` via VitaShell or FileZilla and copy MP3/WAV/FLAC files there if testing Music Player mode.
7. Start **Vita Motion Lab Alpha**; choose Loop Mode to test visuals before Mic/Player. Press **Circle** to return to menu, or Circle on main menu to quit.

If Actions fails, the **first red error** in the compile step is what matters. Copy it into the chat for a targeted patch. A successful VPK build alone still does not prove device compatibility.

## Controls

| Where | Buttons |
|---|---|
| Main menu | D-pad select; X open; Start launch selected scene; Triangle quick palette; Circle exit |
| Source picker | Up/Down source, X select, Circle back |
| Scene browser | Up/Down scene, X choose, Start launch, Circle back |
| Music browser | Up/Down track, X play file, Circle back |
| Palette grid | D-pad navigate, X apply, Circle back |
| Settings | Up/Down setting, Left/Right change, X apply/reset, Circle save/back |
| Visualizer | Left/Right or L/R scenes; Up/Down gain; X pause; Triangle palette; Square overlay; Start pin overlay; Select toggle demo autoplay; Circle menu |

## Rebuild assets after edits

The runtime uses `sheet_NNN.png` atlas images, not the individual `frame_NNN.png` files. To edit individual frames, get the **full editable source ZIP**, update 320×180 PNGs, and run `python3 tools/pack_atlases.py` (requires Pillow) to regenerate 4×2, 1280×360 sheets. Build-only ZIP already contains the final packed sheets.

`python3 tools/check_assets.py` checks the expected atlas counts, contiguous indices and dimensions; it also checks all 544 source frames when those optional source images are present. The GitHub build workflow runs it before compiling. Standard VitaSDK links are in `CMakeLists.txt`; CI retrieves miniaudio 0.11.23 at build time. Build flags `-Os -ffunction-sections -fdata-sections -Wl,--gc-sections` are retained to avoid previous ELF metadata overlap problems.

## First device test checklist

1. Loop Mode launch all five scenes, watch each through 2 full cycles. Scene textures must load without hangs/crashes.
2. Check Blue Halo shape and tempo against reference; note any jump at the loop seam.
3. Dual Sub Reactor: cabinet and mounting geometry must not move when cones pulse. In mic mode, test bass-heavy music in the car while keeping volume safe.
4. Confirm all 12 palettes are legible and only reactive accents change color.
5. Confirm Music Player successfully decodes one MP3, WAV and FLAC; pause/unpause and return to menu.
6. Confirm settings survive restart, Vita remains responsive and memory use doesn't grow across scene changes.

## Known alpha risks

- No VitaSDK toolchain in the generation environment: **no real CMake/Vita ELF link or VPK hardware verification has been performed.** Local syntax checking is not equivalent.
- Sprite-sheet changes still require PNG decoding when crossing an 8-frame group; test for frame hitching and potential GPU allocation stalls.
- Mic availability varies by environment and headphone/Bluetooth routing. Loop Mode works without mic input.
- File browser supports one folder, up to 80 tracks. No embedded cover art, playlists, or gapless playback.
- No 3D GPU mesh engine; this is a layered pre-rendered 3D-style animation app.

## Folder layout

```
CMakeLists.txt
.github/workflows/build-vpk.yml
src/main.c
src/music.c / music.h / decoder.c
src/audio_analysis.c / audio_analysis.h
src/scene_renderer.c / scene_renderer.h
assets/scenes/blue_halo/base/{sheet_NNN.png [and optional frame_NNN.png]}
assets/scenes/dual_sub/{cabinet.png,near/,far/,glow/}
assets/scenes/ribbon_wave/{background.png,back/,front/}
assets/scenes/liquid_pixel/{field_back/,field_front/}
assets/scenes/pulse_heart/{body/,aura/}
tools/{check_assets.py,pack_atlases.py}
docs/PRODUCTION_SPEC.md
```

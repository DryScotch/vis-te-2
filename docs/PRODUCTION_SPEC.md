# Vita Motion Lab v1.0 Alpha — Scene & Asset Production Specification

**Baseline:** existing v0.7 prototype (Mic Mode, MP3/WAV/FLAC Music Player, Loop Mode, 12 accent schemes, GitHub VitaSDK builder). **Target device:** PlayStation Vita PCH-1000 OLED. **Status:** authoritative design target, not implemented or Vita-tested.

## 1. Product scope and non-goals

Make a polished standalone animation-loop and audio visualizer: select a scene, use live built-in mic for car-cabin audio, visualize decoded local music, or run the scene freely in Loop Mode. This alpha ships five fully art-directed scenes and a complete, coherent navigation/settings system. Eight existing secondary scenes remain in the current stable app while the alpha is developed; they are not represented as finished v1 assets.

The look is **low-resolution blue/black dithered Y2K motion design**: crisp pixel silhouettes, 4–8 levels of blue, controlled highlights, black negative space, preserved perspective and *real movement of scene elements*. No sinusoidal warping of finished screenshots, no global zoom on bass, no random shimmer that shifts the underlying geometry. Audio response must alter the correct subject: cone travel, ring radius, ribbon amplitude, etc.

For practical Vita performance the *3D styling is rendered offline*, then played as sprite layers/state frames; this is not a promise of real-time polygonal 3D. The program remains responsive and controls real-time compositing and audio coupling.

## 2. Screen and image rules

- Vita output: **960×544**; preferred scene canvas **320×180**, drawn at exactly 3× nearest-neighbor to **960×540**, with two black pixels top and bottom. UI is rendered at native 960×544 on an overlay, never burned into scene images.
- Scene composition safe area: important objects within 20 px horizontally / 12 px vertically on the 320×180 canvas; small details should occupy at least 2–3 source pixels.
- Main master palette: `#010308` black, `#071126` darkest navy, `#123259` navy, `#1555A3` dark blue, `#2688E9` blue, `#66C7FF` electric cyan, `#B8ECFF` glint, `#EFFBFF` extreme highlight. Highlights may use a subtle glow image layer, not generalized antialias blur.
- Apply **ordered 8×8 Bayer dithering in world/screen coordinates**, after offline rendering, with noise-free deep blacks. Anchor dither matrix to fixed scene coordinates so it does not crawl during animation. Quantize luminance into intentionally limited levels; no accidental banding from image resizing.
- Export **PNG RGBA** (transparency for independent sprites); preserve hard pixels, no bilinear filtering at runtime. Matte/silhouette assets can be indexed/packed if supported; otherwise PNG textures are loaded on demand.
- Frame indices zero-padded to three digits. Animation time is measured in seconds; never tie animation progression directly to Vita display frame count.

## 3. Scene inventory and detailed animation contracts

### Scene 01 — Blue Halo (hero)
**Anchor:** user's uploaded 7.17-second blue-ring reference video. The ring's node positions, bright electric-blue/black contrast, elliptical shape, and motion rhythm should come from the source clip, not a generically generated orbit.

**Composition:** dark full-frame background, centered roughly at `(160,90)` in scene pixels. Preserve source geometry without stretching its 4:3 clip to 16:9: crop black or pillarbox, but never squash the ring.

**Offline layers:** reference-derived background frame loop; optional separately extracted bright-core node mask; separate controlled highlight/stars mask. **Playback:** target 96 selected frames, approximately 15 fps, 6.4 s first candidate segment, with visually matched first/last frames; preserve full reference timing instead if a seamless shorter segment cannot be found. Do not reverse, ping-pong, or time-warp the reference just to hide a seam. Store a scene-specific frame duration in metadata, rather than assuming all scenes use 32 frames.

**Audio:** bass 0–1 maps to 0–4% **node/ring local expansion**, not full canvas zoom. Mid raises core brightness by at most 25%. High produces at most 10 brief particles per second. In Loop Mode play authored source timing and optional gentle simulated pulse.

**Acceptance:** a muted/no-audio playback should closely resemble the actual supplied clip; no stretching of the ring, no abrupt seam, no background flicker.

### Scene 02 — Dual Sub Reactor (hero)
**Anchor:** user's photo of a dual subwoofer enclosure from approximately a one-third perspective and the approved coarse electric-blue dither example. **Cabinet geometry stays fully rigid.** Two physically same-sized subwoofers appear different due to perspective: larger near woofer, smaller farther woofer, not two randomly different speaker types.

**Offline assets:** `cabinet.png` = the static 320×180 cabinet with dark side/top planes, bolt locations, enclosure edge, masks, shadows and holes; `cone_near_000..031.png` and `cone_far_000..031.png` = transparent sprite states for independent inward/outward cone displacement *projected along each speaker axis*; `glow_000..015.png` = optional independent lighting states; `foreground.png` = rigid bezel/screw and rim occlusion to mask the cone edge convincingly.

**Cone states:** 32 equally indexed excursion states from maximum inward (`0`) to maximum outward (`31`), with true geometry/normal and center-cap highlight changes; not 32 resized copies of the entire cabinet. Use continuous audio-driven index selection, with interpolation or smoothing of signal. Distinct near/far parallax offsets remain under 2 source pixels.

**Bass response:** normalized bass envelope drives spring target `x_target = 0.85*bass` and damped travel `x'' + 2ζω x' + ω²(x−target)=0`. Initial tuning ω≈24 rad/s, ζ≈0.55, timestep capped at 1/60 s. Near and far speakers both react to mono microphone input; far speaker can be delayed 25–45 ms for a plausible cabinet feel, not arbitrarily disconnected. Idle movement ≤1 cone state; transient hits give clear extension and natural return. Optional meters use reactive accents without obscuring the enclosure.

**Acceptance:** screen capture of first/last full excursion shows the cabinet, screws, rims and outer silhouette in identical positions. Only the cone depth/highlight changes. Low bass should visibly pump both cones; voice/high-frequency noise should not make them jump wildly.

### Scene 03 — Ribbon Wave (hero)
**Anchor:** user's purple layered translucent ribbon reference, translated to electric-blue dither and optionally recolored via palette.

**Layers:** `background.png` static black; `ribbon_back_000..063.png`, `ribbon_mid_000..063.png`, `ribbon_front_000..063.png` with distinct parallax phases; `edge_glow_000..063.png` optional highlight strip. **64 authored frames at 24 fps** ≈2.67 s loop. Each ribbon is *separately animated*; hold the backdrop rigid. The spline geometry, highlights and occlusion should be rendered together per layer.

**Audio:** bass expands wave amplitude up to 25% within *ribbon geometry*; mids increase shimmer on front layer; treble produces small edge sparkles. Avoid global lens wobble. Seam error measured at phase wrap.

**Acceptance:** fluid continuous progression, distinct front/mid/back separation, no visual jump at loop boundary.

### Scene 04 — Liquid Pixel (hero)
**Anchor:** user's white/cyan fluid blob and LED/halftone reference. Keep organic morphing, not random pixels floating independently.

**Layers:** `field_back_000..063.png` slow organic underfield, `field_front_000..063.png` distinct blob motion, `highlights_000..063.png` moving specular accents; fixed `dither_mask.png` or ordered dithering applied offline. **64 authored frames @24 fps**. Blobs must have coherent topology across adjacent frames; contrast must stay clear against black. Shaderless 2.5D allowed.

**Audio:** bass changes blob area locally ±8%; mids alter flow speed within 0.75–1.5×; treble changes highlight frequency, not all pixel positions. Smooth the audio to prevent flicker.

**Acceptance:** moving forms remain visually identifiable across frames, no full-frame shifting dither grid, stable silhouette readability.

### Scene 05 — Pulse Heart (hero)
**Anchor:** user's isolated cyan luminous heart visual. Stylized central art object on black; no implied medical accuracy required.

**Layers:** static scene black; `heart_000..047.png` 48 pose/lighting frames; `core_000..047.png` independently animated internal glow; `aura_000..023.png` controlled halo; optional tiny radial details. **48 authored frames @24 fps** =2 s idle/breathing cycle. Heart grows/shrinks through 3D/object animation only, not global canvas scaling.

**Audio:** bass envelope triggers 4–10% heart-object expansion with fast attack and softer return; transients can produce a secondary pulse; mids modulate core lighting; highs glint small details. Loop Mode remains elegant without beat input.

**Acceptance:** center remains stable; clean heartbeat/breathing rhythm, no clipping near highlights or warping of background.

## 4. Secondary scenes — planned v1.1, not included as finished alpha

| Mode | Required movement | Geometry that must stay stable |
|---|---|---|
| Glass Spectrum | individually varying extruded bar levels and glints | glass housing/perspective |
| Ribbon Scope | authored ribbon trail following signal; fade history | background panel |
| Scope Glass | beam sweep synced to PCM, phosphor decay | bezel/grid |
| 3D EQ Towers | independent tower height/extrusion | floor and vanishing point |
| LED Floor | tiles light with frequency mapping, forward sweep | perspective floor |
| Reflection Bars | bars pulse with independent floor reflections | camera and floor plane |
| 3D Tunnel | new depth planes advance toward camera | rigid ring shape and edges |
| Speaker Classic | centered single cone travels along normal | cabinet and mounting |

These eight can remain available in a **Legacy** section if users install over an existing version, but they must be labeled as legacy—not silently passed off as completed new 3D assets.

## 5. Color system (12 colorways)

Use `config/palettes.json` as the source of truth, each with `black`, `shadow`, `mid`, `primary`, `highlight`. The baseline scene remains 8-level blue/black; for palette-aware scenes the asset loader may perform a **cached palette LUT conversion at scene-load time**, not per displayed frame. The renderer can also tint transparent glow/meter layers without touching core art. The Blue Halo reference-derived footage defaults to its original blue and presents alternate palette accents as optional; do not make orange/green clones unless an accurate LUT art pass exists. Persist scene palette, not merely global UI color, when supported. UI must always maintain readable contrast with minimum available pixel height.

## 6. Audio engine redesign

**Current v0.7 is not an adequate final bass analyzer:** a 256-sample window at 16 kHz covers only 16 ms and offers poor low-frequency separation. Keep existing working capture as the input layer, then add a separate analysis buffer and rate-specific DSP.

- Mic: input 16 kHz mono via `sceAudioIn*`, ring buffer of at least 2048 samples, 1024-sample Hann window (64 ms) with 256-sample hop (16 ms). Use calibrated bands: bass roughly 40–180 Hz, mids 180–2000 Hz, highs 2–7 kHz. The 16-kHz microphone source cannot represent energy much above 8 kHz.
- Music: 48 kHz decoded PCM, 2048-sample window (42.7 ms), 512-sample hop (10.7 ms), resample/normalize as necessary; 40–180 Hz bass, 180–3000 mids, 3–14 kHz highs. Use a real radix-2 FFT or equivalent tested spectral estimator; ensure sample rate corresponds to actual decoder output.
- Build **master RMS**, normalized bass/mids/highs, attack/release envelopes, transient/onset metric and simple silence gate. Attack target 20–40 ms for bass, release 180–300 ms; tune with actual car audio. Never multiply every visual by the same RMS.
- UI update target 60 Hz; scene playback commonly 15/24/30 fps independent of drawing; ensure event-driven input and queued audio data do not block frame drawing.
- Thread safety: music worker writes completed analysis blocks into a double buffer or synchronized ring; renderer reads consistent snapshots. Do not read a mutable 256-sample array from a decoding thread without synchronization (current v0.7 has this risk).
- Input errors and end-of-track must display an actionable message and return safely. Mic privacy: no network operations, recording, or persistent saved microphone audio.

Scene control values normalized 0..1: `bass`, `mids`, `highs`, `rms`, `transient`, plus `idle_time` and `playback_time`. Loop Mode uses authored timelines, optionally synthesized mellow envelopes clearly labeled as demo pulse.

## 7. UI, screen routing, and controls

**States:** `MAIN`, `SOURCE_PICKER`, `SCENE_BROWSER`, `MUSIC_BROWSER`, `PALETTES`, `SETTINGS`, `ABOUT`, `PLAYING`, `PAUSE_OVERLAY`. Selected source (`MIC`, `FILE`, `LOOP`) and selected scene persist in settings. Keep the tested `Circle = main menu` behavior from v0.2 onwards for visualizer playback.

**Main menu (960×544):** title at top left, slim source/palette indicator top right, center list with thumbnail preview on right. Items: Start Visualizer, Source, Visualizers, Music Browser, Color Schemes, Settings, About. Footer shows controls (Circle Back, Cross Select), a small version marker and no fake audio indicator.

**Scene browser:** five hero cards first with actual thumbnail/description; optionally eight legacy entries under an explicit Legacy divider. Selection previews a still frame, not loading an entire animation stack yet.

**Playback:** scene fills 960×540 pixel-perfect, status overlay in native resolution (scene name, source, palette, track filename for File mode, sensitivity only for Mic). Overlay hidden by default after 4 seconds with no input; Square toggles pinned visibility. While playing: Left/Right change scenes; Up/Down adjust mic gain or scene intensity depending on source; Triangle cycles schemes; Cross pauses or resumes animation + file playback as appropriate; Circle returns to MAIN and releases mic/worker resources; Start opens options; Select toggles demo autoplay. Avoid using the same input to both change the scene and menu item in a single frame.

**Menus:** D-pad navigates, Cross confirms, Circle backs one level, Start launches selected scene only when safe; keep actions discoverable. Music browser filters valid MP3/WAV/FLAC; display empty-folder instructions for `ux0:data/CabinMusic/`; sorting stable, filenames cropped rather than overflowing.

**Settings:** per-source gain, show overlay, speed {0.5, 1, 1.5}, dither strength {low, medium, high} (applied to palette-aware offline-derived assets), CRT scanlines on/off, performance {quality, balanced, fast}, autoplay {off, 30s, 60s, 120s}, remember last mode, reset to defaults. Settings at `ux0:data/VitaMotionLab/settings.ini` with atomic write and safe defaults if absent/corrupted.

## 8. Asset manifest and folder plan

```
VitaMotionLab/
  CMakeLists.txt                # checked-in VitaSDK-safe flags and Sce stubs
  .github/workflows/build-vpk.yml
  src/
    main.c                     # initialization, state transition dispatcher
    scene_renderer.c/.h        # animated layers and texture cache
    scene_manifest.c/.h        # metadata loader + fallbacks
    audio_capture.c/.h         # mic acquisition
    audio_analysis.c/.h        # FFT, envelopes, transients
    music.c/.h                 # existing playback, hardened
    decoder.c                  # miniaudio decode-only wrapper
    ui.c/.h                    # native overlay/menu rendering
    settings.c/.h              # persistent preferences
  assets/
    shared/{ui,font,previews,...}
    scenes/
      blue_halo/{scene.json,base/frame_000.png,...}
      dual_sub/{scene.json,cabinet.png,foreground.png,near/frame_000.png,...,far/...}
      ribbon_wave/{scene.json,back/...mid/...front/...}
      liquid_pixel/{scene.json,field_back/...field_front/...highlights/...}
      pulse_heart/{scene.json,heart/...core/...aura/...}
  tests/
    frame_differences.py       # detects unintended whole-frame movement
    continuity.py              # loop seam quality
    manifest_check.py          # missing/incorrect asset naming or dimensions
    audio_response.py          # synthetic bass/voice/noise traces
```

**Naming:** `frame_000.png`, increment without gaps to `frame_(count-1).png`. Manifest required fields: `id`, `title`, `art_w`, `art_h`, `fps`, `duration_s`, `loops`, `layers`, `reactivity`, `palette_policy`. Layer metadata includes `type`, `path`, `frame_count`, `frame_fps`, `alpha`, `blend`, `load_group`, and `reactive_target`. IDs are stable and unique. Manifests may be parsed at build time into a C table if a JSON parser would add unnecessary Vita binary size.

## 9. Vita memory, packaging and build constraints

- **Do not load hundreds of 320×180 RGBA PNGs concurrently.** One decoded 320×180 RGBA texture is 225 KiB before allocation overhead; 96 Blue Halo frames alone would be ~21 MiB decoded. Hold only a 2–4-frame cache per active layer plus current static layers; stream on scene transition, prefetch when possible, free old scene completely.
- Suggested **scene texture memory budget < 12 MiB**, target application steady-state RAM well under device limits; these are *targets*, to be measured on a real PCH-1000.
- Alpha performance targets: consistent 30 fps scene rendering on hardware, menu responsiveness at 60 Hz where possible, no audible playback discontinuities, switching hero scenes without running out of memory. Asset frame rate may differ from display refresh.
- Pack assets via `vita_create_vpk` with explicit relative paths. Validate in CI that the VPK contains representative frames and manifest filenames, *and* has the correct build/version tag, not just a green compilation checkmark.
- Keep VitaSDK's successful `-Os -ffunction-sections -fdata-sections` and `-Wl,--gc-sections` flags and `MA_NO_DEVICE_IO`, `MA_NO_RUNTIME_LINKING` decode-only setup. Keep correct `SceIofilemgr_stub` link name and existing working import libraries. Pin decoder dependency to a known-tested version/hash; avoid silent `latest` drift for production builds.
- Development path: continue using the user's successful GitHub Actions VPK build and VitaShell FTP install. Give each alpha an unambiguous on-screen version string, retain same Vita title ID only if deliberately replacing installed builds.

## 10. Acceptance criteria before claiming alpha completion

**Visual:** 5 hero scenes each play at correct scene frame rate, screenshots match approved art, fixed elements stationary, frame-loop seam invisible in regular playback, cone silhouette moves independently in at least two distinct depths, no universal warped/zoomed frames. Compare screenshots and short video captured on Vita to approved source references.

**Audio:** mic reacts to bass-heavy car audio without false triggering on quiet speech; FILE playback decodes a sample each of MP3/WAV/FLAC, no blocking UI, pause resumes, end-of-track behaves correctly; LOOP works offline and silent. Source switching releases input/output cleanly.

**Palette:** all 12 distinct schemes selectable, readable UI, correctly scoped tint policies; no Pioneer Blue/blue-gold accidental duplicate. Settings restore after restart.

**Build:** CI checks manifest, image dimensions, frame counts, VPK content list and toolchain output; VPK installs and launches on physical Vita 1000; navigate scenes for 10 minutes without crash, visible excessive frame stalls or accumulating texture memory.

**Release gate:** do not label v1.0 Alpha "fully working" until all acceptance checks pass on device. Until then report exact tested parts and remaining unknowns.

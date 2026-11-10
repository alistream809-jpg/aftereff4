# AFTER TRUE - Feature verification checklist (v0.1.0, Milestone 1)

Legend
* **TESTED-UNIT** - covered by automated unit tests that were compiled and run (28 tests, all passing).
* **BUILT-UNVERIFIED** - code written, but never compiled/run by the author: Qt UI and FFmpeg code can only be compiled on
  Windows (GitHub Actions). Treat as untested until you have run it. Expect first-build compile errors.
* **NOT IMPLEMENTED** - does not exist yet. The application does not pretend otherwise.

## Foundation
| Feature | Status |
|---|---|
| JSON engine, timecode, IDs, atomic file writing | TESTED-UNIT |
| Keyframe animation: linear, hold, Bezier / Easy Ease evaluation | TESTED-UNIT |
| Project model (project > footage, compositions > layers > properties) | TESTED-UNIT |
| `.atrue` save / load, lossless round-trip, rejects bad or newer files | TESTED-UNIT |
| Compositor: multi-layer stacking, position/scale/rotation/anchor/opacity, keyframed values, in/out trim, solo/visibility, preview divisor, bilinear sampling | TESTED-UNIT (CPU, pixel-checked) |
| Windows build via GitHub Actions + installer script | BUILT-UNVERIFIED (never run) |

## Media & timeline (needs Windows build to verify)
| Feature | Status |
|---|---|
| Import video/images/audio via FFmpeg (probe, decode, seek) | BUILT-UNVERIFIED |
| Drag & drop import, multi-file import, thumbnails, relink missing media | BUILT-UNVERIFIED |
| Composition viewer showing the combined result of all layers; preview resolution; playback | BUILT-UNVERIFIED |
| Timeline: ruler, playhead, zoom/scroll, move/trim layers, V/S/L switches, duplicate, split, reorder, rename, delete | BUILT-UNVERIFIED |
| Properties panel with keyframing (stopwatch), Linear / Easy Ease / Hold | BUILT-UNVERIFIED |
| Solid layers | BUILT-UNVERIFIED |
| Project save/open/recent, autosave + crash recovery, unsaved-changes prompt | BUILT-UNVERIFIED |
| Dockable/floating/resizable panels, saved window layout, keyboard shortcuts, context menu | BUILT-UNVERIFIED |
| Export current frame as PNG (full resolution) | BUILT-UNVERIFIED |

## Known limitations of what exists
* Compositing is **CPU-only** (multi-threaded). 4K sources at full preview quality will be slow; use Half/Quarter preview.
* Decoding runs on the UI thread; no frame cache beyond the last frame per layer.
* Phone videos with a rotation flag are not auto-rotated. AV1 video is not supported by the bundled FFmpeg build. Animated WebP/GIF are shown as the decoder provides.
* Audio files can be imported and are listed, but **cannot be played or added to compositions yet**.
* Keyframes cannot yet be moved/deleted individually in the UI (only toggling Animate off removes all of a property's keys).
* Skew, volume properties: not implemented. No undo/redo.

## NOT IMPLEMENTED (everything else requested in the specification)
* **Rendering/export:** MP4/MOV/image-sequence export, H.264/H.265/ProRes, bitrate/audio settings, render queue, background rendering.
* **Effects:** all 60 effects (glow, colour, blur, glitch, stylize, LUTs...), effect stacking, Effect Controls, Effects & Presets panel.
* **Layers:** text, shape, null, adjustment, camera layers; parenting; grouping; blend modes; masks; track mattes; markers; work area.
* **Animation:** Graph Editor, speed graph, motion paths, keyframe copy/paste/selection, animation presets.
* **Transitions:** all transition types and the transition builder.
* **AI:** Auto Shake/Zoom/Motion, captions/transcription, background removal and segmentation, tracking, scene detection,
  denoise, frame interpolation, voice isolation, audio leveling/sync. (Planned models: see docs/ARCHITECTURE.md.)
* **Audio:** waveforms, playback, mixing.
* **Performance:** GPU acceleration, proxies, RAM-aware cache, performance monitor.
* **Platform:** plugin manager, multiple workspaces beyond saved layout, relink-folder import, project import/merge.
* **Signed installer** (Windows will show an "unknown publisher" warning).

## Milestone plan
M2 text/solid/shape + blend modes + MP4 export (H.264) + audio playback | M3 effects registry + first effects + masks + graph editor |
M4 more effects, transitions, Auto Shake/Zoom, audio | M5 ONNX AI (background removal, captions) | M6 GPU, proxies, cache, render queue.

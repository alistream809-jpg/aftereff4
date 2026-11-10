# Architecture

```
src/core     time (Rational), Frame (RGBA8), JSON, ids, timecode, file utils, IFrameSource   [no dependencies]
src/anim     AnimatedProperty: keyframes, linear/hold/bezier evaluation, JSON            -> core
src/project  Project > Footage / Composition > Layer > 8 animated properties; .atrue I/O  -> anim, core
src/comp     renderComposition(): the ONE function that turns (project, time) into pixels -> project
src/media    FFmpeg: probe, VideoReader (seek+decode), MediaManager (IFrameSource)        -> project, FFmpeg
src/ui       Qt: Document (shared state + edit API), panels, MainWindow                   -> media, comp, Qt
src/app      main()
src/effects  (planned)   src/ai (planned)   src/render (planned)   src/plugin (planned)
tests        unit tests for core/anim/project/comp (no Qt/FFmpeg needed)
```

## Rules
* **One render path.** The viewer, PNG export and (later) video export all call `renderComposition`. Export will therefore match preview.
* `comp` never includes FFmpeg or Qt: it asks an `IFrameSource` for pictures. Tests use a fake source; the app uses `MediaManager`.
* Layer 0 in `Composition::layers` is the **top** layer (After Effects convention). Keyframe times are **layer-local** (time minus `startTime`), so moving a layer moves its animation.
* `Document` is the only place that mutates the project; it emits signals; panels never edit data directly.

## Planned (not built)
* GPU compositing: OpenGL 4.5 float16 framebuffers; the CPU compositor stays as the reference / test oracle.
* Effects: `IEffect` (parameter schema + shader pass) in a registry; effect chains stored per layer in the project file.
* Export: FFmpeg encode (H.264 via libx264 requires GPL build or a licence decision; H.265/ProRes similarly) + audio mixdown.
* AI via ONNX Runtime (DirectML): Robust Video Matting (**GPL-3.0 - check before shipping**) or MODNet (Apache-2.0) for matting;
  Whisper (MIT) for captions; RNNoise/Demucs for audio; RIFE (MIT) for interpolation. Auto Shake/Zoom are procedural keyframe generators.

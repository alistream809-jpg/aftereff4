# Third-party components

| Component | Licence | How used |
|---|---|---|
| Qt 6 (Widgets) | LGPL-3.0 | Dynamically linked DLLs shipped next to the EXE. You may replace them. |
| FFmpeg (libavcodec/format/util/swscale/swresample) | LGPL-2.1+ | Built by vcpkg **without** GPL/nonfree options, dynamically linked. |
| Microsoft Visual C++ runtime | Redistributable | App-local copy for clean PCs. |
| Inno Setup | Inno Setup licence | Build-time only (creates the installer). |

Important: some codecs (e.g. libx264 for H.264 export, planned in M2) are GPL. Using them changes the licensing of distributed binaries.
That decision is deferred and will be documented before export ships. Patent licensing for codecs such as H.264/H.265 is the distributor's responsibility.

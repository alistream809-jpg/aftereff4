# `.atrue` project format (version 1)

UTF-8 JSON. Written atomically (temp file + rename). A file with `version` greater than the application supports is refused with a clear message.

```json
{
  "format": "AFTERTRUE", "version": 1, "name": "My film", "activeComp": "<comp id>",
  "footage": [ { "id": "...", "name": "clip.mp4", "path": "C:\\Videos\\clip.mp4", "kind": "video|image|audio",
                 "width": 1920, "height": 1080, "fps": 29.97, "duration": 12.5, "hasAudio": true } ],
  "compositions": [ {
      "id": "...", "name": "Comp 1", "width": 1920, "height": 1080,
      "fps": { "num": 30000, "den": 1001 }, "duration": 10, "background": [0, 0, 0],
      "layers": [ {                       // index 0 = top layer
          "id": "...", "name": "clip.mp4", "type": "footage|solid",
          "footage": "<footage id>",      // footage layers
          "solid": { "color": [255,0,0], "width": 1920, "height": 1080 },   // solid layers
          "start": 0, "in": 0, "out": 5, "visible": true, "locked": false, "solo": false,
          "props": { "posX": { "value": 960, "keys": [ { "t": 0, "v": 0, "i": "linear|hold|bezier", "b": [0.42,0,0.58,1] } ] },
                     "anchorX","anchorY","posY","scaleX","scaleY","rotation","opacity" }
      } ] } ]
}
```
Media is referenced by absolute path (not embedded). Missing files are flagged in the Project panel and can be relinked.
Keyframe `t` is layer-local seconds; scale and opacity are percentages; rotation is degrees clockwise.

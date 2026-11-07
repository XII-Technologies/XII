# Diffuse GI capture sample

Build the `DiffuseGI` target in a renderer-enabled configuration. Transform the authored render pipeline assets before running the sample. The executable uses the compiled Scene and Editor assets and their shared `SceneScreenSpaceIrradiance` workload.

```powershell
DiffuseGI.exe -output D:/XII/Workspace/DiffuseGI -pipeline Scene -frames 24
DiffuseGI.exe -output D:/XII/Workspace/DiffuseGI -pipeline Editor -frames 24 -diagnostics
DiffuseGI.exe -output D:/XII/Workspace/DiffuseGI/Sky -pipeline Scene -frames 24 -sky-only
```

Each run produces a pair of PNGs at a fixed exposure. The first uses the standard authored pipeline with diffuse transport disabled. The second enables the ray request for the same camera and materials, with screen-space fallback when ray pipelines are unavailable. The room has neutral objects, red and green walls, and an emissive ceiling panel. `-sky-only` turns off the panel and uses a brighter blue sky to isolate sky bounce.

The Editor run requires a build with Editor support. `-diagnostics` captures all five diffuse GI views exposed in the viewport. Raw, Reconstructed, and Previous Frame show the irradiance signals. Reconstruction Delta displays the absolute RGB difference between raw and reconstructed irradiance. Frame Delta compares current and previous reconstructed irradiance at the same screen pixel, so camera motion can also show up. The history views are black until history is available.

Use `-no-ray` to capture the non-ray path on a ray-capable device. Add `-screen-only` to render the screen-space comparison in a separate output directory. The capture log identifies frames that scheduled the directional probe update.

On ray-capable hardware, use `-paths 1` to make the fresh-path budget explicit. Compare a second capture in a separate directory with `-paths 1 -no-reuse`. The reuse path traces additional reconnection rays. The capture log identifies when the authored ray pass reads the previous reservoir grid.

For a first-frame comparison, add `-frames 1` to both captures. The reuse capture then reads the current spatial grid before any temporal history exists.

Use `-local-lights -no-ray` to capture exact clustered lighting, raw reservoirs, and reconstructed reservoirs in a 30-point-light room. All three captures use the same camera, materials, and fixed exposure. `-frames 1` shows spatial filtering before temporal history is available.

Use `-offscreen-emitter` to capture the camera-based and world-geometry probe updates with an emissive panel outside the frame. This mode disables hardware ray tracing and removes the sky light. Add `-block-emitter` to place an opaque cover beneath the panel and compare the shadowed bounce. The capture log identifies which probe update ran.

Add `-dense-emitter` to replace the offscreen panel with a downward-facing, 4,608-triangle surface. This stresses meshlet rejection while keeping the emitter outside the camera view.

Use `-offscreen-light` to replace emission with four short-range point lights illuminating the panel. The camera and world captures use the same scene and exposure with hardware rays disabled. Add `-block-emitter` to cover the lights and compare against the same covered scene with `-no-light-shadows` to isolate secondary visibility.

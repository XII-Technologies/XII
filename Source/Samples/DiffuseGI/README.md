# Diffuse GI capture sample

Build the `DiffuseGI` target in a renderer-enabled configuration. Transform the authored render pipeline assets before running the sample. The executable uses the compiled Scene and Editor assets and their shared `SceneScreenSpaceIrradiance` workload.

```powershell
DiffuseGI.exe -output D:/XII/Workspace/DiffuseGI -pipeline Scene -frames 24
DiffuseGI.exe -output D:/XII/Workspace/DiffuseGI -pipeline Editor -frames 24 -diagnostics
DiffuseGI.exe -output D:/XII/Workspace/DiffuseGI/Sky -pipeline Scene -frames 24 -sky-only
```

Each run produces a pair of PNGs at a fixed exposure. The first uses the standard authored pipeline with diffuse transport disabled. The second enables the ray request for the same camera and materials, with screen-space fallback when ray pipelines are unavailable. The room has neutral objects, red and green walls, and an emissive ceiling panel. `-sky-only` turns off the panel and uses a brighter blue sky to isolate sky bounce.

The Editor run requires a build with Editor support. `-diagnostics` captures raw irradiance, reconstructed irradiance, and the previous reconstructed frame through the same view modes exposed in the viewport. The previous-frame view is black until history is available.

Use `-no-ray` to capture the non-ray path on a ray-capable device. Add `-screen-only` to render the screen-space comparison in a separate output directory. The capture log identifies frames that scheduled the directional probe update.

On ray-capable hardware, use `-paths 1` to make the fresh-path budget explicit. Compare a second capture in a separate directory with `-paths 1 -no-reuse`. The reuse path traces additional reconnection rays. The capture log identifies when the authored ray pass reads the previous reservoir grid.

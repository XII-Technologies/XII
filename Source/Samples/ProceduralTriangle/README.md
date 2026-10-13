# Procedural Triangle

The scene is synthesized by a pixel shader operating on one fullscreen triangle, with no geometry buffers, loaded models, or textures.

## Visual Architecture

- Seven hollow, counter-rotating **3D triangular alloy frames** with independent extrusion depths, phases, and colour assignments.
- A five-/six-stage **tetrahedral iterated-function-system fractal**, surrounding a slowly tumbling faceted diamond.
- Nine independently orbiting and spinning octahedral crystals.
- Two luminous gyroscopic orbital bands.
- Analytic metal/crystal material shading with Fresnel, two specular key lights, and optional three-sample ambient occlusion.
- A procedurally synthesized nebula, pinprick star field, animated atmosphere, cinematic filmic tonemapping, grain, vignette, and optional light reticle.
- A dark infinite illuminated grid floor with **secondary SDF reflection rays** for the quality tiers that support them.
- Four colour palettes: Gilded Teal, Neon Amethyst, Solar Ember, Polar Glass.

The scene does **not** use hardware ray-tracing acceleration: raymarching occurs
in the fragment shader and should run on standard graphics-capable devices.
There is no texture-based HDR bloom or full volumetric light simulation. Glow is
approximated by analytic emissive lighting and a light-accumulation term.

## Controls

| Key       | Effect |
| ---       | --- |
| `Space`   | Pause/resume animated simulation |
| `C`       | Cycle four cinematic colour palettes |
| `Q`       | Cycle Low / Medium / Ultra quality |
| `H`       | Toggle translucent technical reticle |
| `A` / `D` | Move camera orbit left / right |
| `W` / `S` | Tilt camera up / down |
| `R`       | Reset camera |
| `Escape`  | Quit |

Quality tiers: Low = 44 primary steps, no reflection; Medium = 68 primary
steps / 25 reflection steps; Ultra = 104 primary steps / 42 reflection steps,
plus three ambient-occlusion samples. The inner fractal increases detail on
Ultra. Quality does not change scene resolution.

## Design notes

The fullscreen triangle is still drawn using `cmd.Draw({3})`. The vertex shader
constructs an oversized triangle with `SV_VertexID`; `SV_POSITION` is used in
the pixel shader to derive pixel-perfect screen coordinates.

`xiiProceduralTriangleConstants` is expanded from 16 to 48 bytes (three
16-byte rows) and written via the existing mapped render-graph buffer. CPU
input only controls time, palette, raymarch budget, exposure, reticle and camera;
geometry is entirely shader-side.

The output render target is still RGBA8 sRGB. The shader therefore applies
filmic tonemapping *in linear light* and lets the render target perform the
linear-to-sRGB encoding: there is no extra manual `pow(color, 1/2.2)`.

# Renderer validation and remaining work

The default path is now Scene plus Editor. See [SceneRenderer.md](SceneRenderer.md)
for the implemented contract, ownership, consolidation decisions and validation.
The historical migration inventory is retained in
[NativePassInventory.md](NativePassInventory.md); its old asset counts and sample
orchestration descriptions do not describe the current defaults.

## Remaining acceptance work

- Complete a full interactive Editor session: ordinary imported scene, lights,
  cameras, materials, selection/picking, gizmos, previews, profile editing,
  asset transformation and hot reload. Engine/Editor GPU tests cover these paths
  selectively, not every desktop interaction. Native viewport validation and
  the grid/handle architecture are recorded in
  [EditorViewportVisualization.md](EditorViewportVisualization.md).
- Exercise D3D12 and other supported platforms, stereo cameras and representative
  large worlds. Current end-to-end output evidence is from the configured Windows
  Vulkan backend. Budget checks are explicit; no cross-backend certification or
  production frame-time/memory baseline is claimed.
- Extend material/shader coverage beyond the canonical GPU surface representation.
  Validate ordinary imported textured assets and supported custom permutations in
  representative projects before broad material compatibility claims.
- Add transparent/particle rasterization, local/virtual/ray shadows, environment convolution, reflections,
  GI and temporal reconstruction only through complete, independently observable
  rendering paths. Existing operation classes and focused tests do not establish
  complete production implementations. Atmospheric sky/aerial perspective now
  execute only with a selected world atmosphere. Conventional directional cascades
  execute only for the selected shadow-casting sun; the remaining paths are not enabled.
- Establish representative CPU/GPU timing, queue/submission and memory baselines.
  Optimize from those measurements, including the repeated Editor query work.
  Do not add subgraphs or authored assets without a concrete reusable workload.

Preserve world-authoritative extraction, per-instance mutable state, explicit
resource interfaces, graph/GAL allocation and synchronization, and observable
resource/build failures. The SDK asset library must remain intentional; adding an
operation or passing a compiler test is not rendering acceptance.

## Feature audit and delivery order

The first extension is spatial screen-space ambient visibility: actual Scene
depth and normals produce a filtered R8 mask consumed by diffuse illumination.
See `SceneRenderer.md` for topology, controls and resource contracts. Temporal AO
and hardware ray-traced AO remain distinct acceptance work.

The following order follows concrete producer dependencies. Each group requires
its own focused PR and production GPU comparisons before its status changes.

| Group | Existing operations | Missing production contract |
| --- | --- | --- |
| Ambient visibility | AO generation and spatial/temporal filtering | Temporal use needs real previous-camera matrices, motion and disocclusion acceptance; canonical spatial path is integrated |
| Atmosphere and environment | World atmosphere constants, transmittance, multiple scattering and application | Canonical sky/aerial path integrated with GPU Scene/Editor acceptance; environment convolution, clouds, atmospheric shadows and content caching remain |
| Directional shadows | World-selected cascades, existing GPU caster culling/depth raster and PCF visibility | Canonical directional atlas/lighting path integrated with Scene/Editor GPU comparisons; local/virtual/ray shadows, cascade transitions, animated/cutout acceptance and performance remain |
| Temporal reconstruction | TAA, buffer/texture history, G-buffer velocity | Previous-camera state, consistent jittered visibility/raster, valid motion including animation, depth/disocclusion rejection and independent-view resets |
| Reflections | Screen-space trace/temporal filter, BRDF/environment specular and confidence composition | Scene depth pyramids and radiance producers, explicit miss/environment response, moving geometry and final glossy-surface acceptance |
| Transparency and refraction | Weighted blended resolve and screen refraction trace | Transparent candidate/coverage contracts, actual accumulation/revealage raster producers, lighting, depth and format-compatible HDR composition |
| Particles | Explicit particle-buffer integration | World extraction, renderable particle resources, billboards/materials and visible transparent composition; simulation is not rendering |
| Global illumination | DDGI and sparse-voxel scheduling, upload, environment seed and gather; screen irradiance | Geometry-dependent transport and visibility, real radiance updates, bounded residency and final indirect-light acceptance; environment seeding is not scene GI |

The historical native-pass inventory must not be used to close any of these
production gaps. Sensor and optional hardware-ray pipelines stay separate from
the ordinary Scene path. Bloom already has a producer and tone-mapping consumer
in Scene; adding another node is not a missing-feature implementation. Its
quality, thresholds and view controls need effect-specific acceptance alongside
future world effects.

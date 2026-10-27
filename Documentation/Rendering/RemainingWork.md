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
  representative projects before broad material compatibility claims. Canonical
  opaque unlit emission and packed deferred flags now have production GPU acceptance;
  layered sheen/clearcoat/anisotropy evaluation remains a separate material workload.
- Add volumetric/rough refraction, lit/atmospheric and advanced particle renderers, local/virtual/ray shadows, environment convolution, reflection history/probes,
  offscreen/multibounce GI and temporal upsampling only through complete, independently observable
  rendering paths. Existing operation classes and focused tests do not establish
  complete production implementations. Atmospheric sky/aerial perspective now
  execute only with a selected world atmosphere. Conventional directional cascades
  execute only for the selected shadow-casting sun. Render-resolution TAA and spatial screen reflections are integrated
  as authored opt-ins. Transparent world meshes now render through a separately authored
  workload, gated by configuration and eligible content. Thin-slab screen refraction now uses explicit transparent materials' IOR, thickness, tint and transmission, with an immutable opaque background. Unlit ballistic billboards append to the same layers. Single-bounce screen-space diffuse transport now adds visible direct/emissive radiance through an authored opt-in; offscreen/probe/voxel GI remains separate work.
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
| Temporal reconstruction | TAA, buffer/texture history, G-buffer velocity and explicit temporal camera preparation | Opt-in native-resolution TAA integrated with rigid/bone motion, disocclusion, Scene/Editor GPU output and independent-view resets; temporal upsampling, morph/higher-quality reactive reconstruction and quality/performance remain |
| Reflections | Screen-space trace/temporal filter, BRDF/environment specular and confidence composition | Opt-in spatial SSR integrated with real depth/radiance mip producers, explicit zero-contribution misses and material-sensitive final output; environment/probe response, reflection history, TAA reactive-mask quality and representative glossy transport/performance remain |
| Transparency and refraction | Canonical forward transparent raster with dielectric slab tracing, weighted alpha/additive/modulation resolve and separate legacy UV refraction | World-mesh blending and thin-slab refraction are integrated with Scene/Editor GPU output, fragment-local lights/shadows/atmosphere and conservative temporal trust; opaque-transmission routing, rough/volume/multiple-interface transport, transparent ray targets/shadow casting, motion-aware AA, skinning/IES-specific acceptance and representative performance remain |
| Particles | World birth upload, GPU ballistic trajectories, billboard append and separate explicit Euler integration | Unlit ballistic point/sphere/volume-box burst/continuous billboards are integrated through Scene/Editor with materials, soft depth, shared transparency and temporal trust; lit/atmospheric, stretched/mesh/ribbon/volume renderers, graph/force/collision/event/sort/molecular execution and representative performance remain |
| Global illumination | Three-operation screen irradiance workload; separate DDGI and sparse-voxel scheduling, upload, environment seed and gather | Opt-in geometry-dependent single-bounce screen-space transport has Scene GPU response, visibility, independent-view and temporal rejection acceptance. Offscreen/probe/voxel transport, multibounce updates, denoising, bounded residency and representative performance remain; environment seeding is not scene GI |
| Bloom and exposure | Existing soft-knee tent bloom, log-luminance histogram, adaptation and tone mapping | Production Scene/Editor pixel acceptance now covers conditional bloom, threshold/knee/radius, fractional percentile metering, adaptation, resets and explicit malformed-input failures. Multiscale bloom, manual-mode histogram elimination, arbitrary retained exposure replay, HDR monitor calibration and representative performance remain |

The historical native-pass inventory must not be used to close any of these
production gaps. Sensor and optional hardware-ray pipelines stay separate from
the ordinary Scene path. Bloom already has a producer and tone-mapping consumer
in Scene; production GPU acceptance now establishes that existing workload's
threshold, knee, radius and view controls. Wider filtering and display quality
remain effect-specific work rather than reasons to add disconnected pass nodes.

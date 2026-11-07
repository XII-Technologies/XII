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
- Add volumetric/rough refraction, lit/atmospheric and advanced particle renderers, point/area/virtual/ray shadows, dynamic reflection captures,
  importance-sampled/denoised GI and temporal upsampling only through complete, independently observable
  rendering paths. Existing operation classes and focused tests do not establish
  complete production implementations. Atmospheric sky/aerial perspective now
  execute only with a selected world atmosphere. Conventional directional cascades
  execute only for the selected shadow-casting sun. Render-resolution TAA and spatial/temporal screen reflections are integrated
  as authored opt-ins. Transparent world meshes now render through a separately authored
  workload, gated by configuration and eligible content. Thin-slab screen refraction now uses explicit transparent materials' IOR, thickness, tint and transmission, with an immutable opaque background. Unlit ballistic billboards append to the same layers. Single-bounce screen-space diffuse transport now adds visible direct/emissive radiance through an authored opt-in; bounded offscreen emissive ray paths now extend the same opt-in; probe/voxel GI and production sampling/denoising remain separate work.
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
See `SceneRenderer.md` for topology, controls and resource contracts. Opt-in temporal AO now uses the explicit camera and motion producers; hardware
ray-traced AO and representative temporal quality remain distinct acceptance work.

The following order follows concrete producer dependencies. Each group requires
its own focused PR and production GPU comparisons before its status changes.

| Group | Existing operations | Missing production contract |
| --- | --- | --- |
| Ambient visibility | AO generation and spatial/temporal filtering | Canonical spatial and opt-in temporal paths are integrated with Scene/Editor camera-motion and disocclusion acceptance; combined jittered TAA/AO has production Scene/Editor correctness acceptance; hardware-ray visibility, animation-specific quality, stereo and representative quality/performance remain |
| Atmosphere and environment | World atmosphere constants, transmittance, multiple scattering and application | Canonical sky/aerial path integrated with GPU Scene/Editor acceptance; physical sky capture/convolution and canonical immutable LUT reuse are integrated; clouds, atmospheric shadows, higher-quality integration and representative performance remain |
| Directional shadows | World-selected cascades, existing GPU caster culling/depth raster and PCF visibility | Canonical directional atlas/lighting path integrated with Scene/Editor GPU comparisons; point/area/virtual/ray shadows, animated/directional-cutout acceptance and representative performance remain; configurable tent PCF, overlapping cascade transitions and optional bounded angular-source PCSS now execute through the shared opaque/transparent sampler |
| Spotlight shadows | Existing local projection planner and reusable caster/atlas operations, plus one native authoritative spotlight preparation operation | Conventional one-to-four-light perspective atlases now affect opaque and transparent Scene/Editor output. GPU readbacks verify all four tiles, per-light sums, off-camera and alpha-masked casters, shadow fading, temporal disappearance, independent worlds/views, resize/reload and retained-world destruction. Point/area/virtual/ray paths, animated spotlight-specific acceptance, stereo, soft penumbrae and representative performance remain |
| Temporal reconstruction | TAA, buffer/texture history, G-buffer velocity and explicit temporal camera preparation | Opt-in native-resolution TAA integrated with rigid/bone motion, disocclusion, Scene/Editor GPU output and independent-view resets; temporal upsampling, morph/higher-quality reactive reconstruction and quality/performance remain |
| Reflections | Screen-space trace/temporal filter, BRDF/environment specular and confidence composition | Opt-in spatial and temporal SSR integrated with real depth/radiance mip producers, explicit zero-contribution misses, bounded immutable histories and material-sensitive final output; resource-owned GGX convolution, diffuse irradiance, local probe fallback and forward transparent probe response are now integrated. Authored global sky fallback is integrated; dynamic geometry captures, reflected-hit motion, TAA reactive-mask quality and representative glossy transport/performance remain |
| Transparency and refraction | Canonical forward transparent raster with dielectric slab tracing, weighted alpha/additive/modulation resolve and separate legacy UV refraction | World-mesh blending and thin-slab refraction are integrated with Scene/Editor GPU output, fragment-local lights/shadows/atmosphere and conservative temporal trust; opaque-transmission routing, rough/volume/multiple-interface transport, transparent ray targets/shadow casting, motion-aware AA, skinning/IES-specific acceptance and representative performance remain |
| Particles | World birth upload, GPU ballistic trajectories, billboard append and separate explicit Euler integration | Unlit ballistic point/sphere/volume-box burst/continuous billboards are integrated through Scene/Editor with materials, soft depth, shared transparency and temporal trust; lit/atmospheric, stretched/mesh/ribbon/volume renderers, graph/force/collision/event/sort/molecular execution and representative performance remain |
| Global illumination | Screen irradiance and bounded emissive/analytic ray paths selected through one authored workload; separate DDGI and sparse-voxel operations | Opt-in geometry-dependent single-bounce screen-space transport has full shared Scene/Editor GPU response, visibility, independent-world and temporal rejection acceptance. Offscreen emissive diffuse paths with 1-4 hits, ray-disabled screen fallback, enclosure energy and Scene/Editor lifetime/output acceptance are integrated. All seven analytic emitter types now illuminate secondary hits with bounded view-eligible sampling, IES response and ray visibility, verified through Scene/Editor and a numerical enclosure reference. GPU power-weighted analytic proposals with a uniform support component now reduce unequal-light error at equal sample cost; multiblock and dim-light cases pass through Scene/Editor. Canonical TLAS-derived emissive proposal extraction and hierarchical aliases now have bounded GPU readback acceptance, including deformation and owner retirement; Scene/Editor next-event consumption is still outstanding. Geometry-aware analytic and emissive/environment importance sampling, reservoirs, probe/voxel transport, denoising, bounded residency and representative performance remain; environment seeding is not scene GI |
| Bloom and exposure | Existing soft-knee tent bloom, log-luminance histogram, adaptation and tone mapping | Production Scene/Editor pixel acceptance now covers conditional multiscale bloom with bounded graph stages and uniform-field energy, threshold/knee/radius, fractional percentile metering, adaptation, resets and explicit malformed-input failures. Manual mode omits metering/adaptation and retains independent upload snapshots; arbitrary retained automatic exposure replay, HDR monitor calibration and representative performance remain |

The historical native-pass inventory must not be used to close any of these
production gaps. Sensor and optional hardware-ray pipelines stay separate from
the ordinary Scene path. Bloom already has a producer and tone-mapping consumer
in Scene; production GPU acceptance now establishes that existing workload's
threshold, knee, radius and view controls. Wider filtering and display quality
remain effect-specific work rather than reasons to add disconnected pass nodes.

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
- Add transparent/particle rasterization, shadows, environment/sky, reflections,
  GI and temporal reconstruction only through complete, independently observable
  rendering paths. Existing operation classes and focused tests do not establish
  complete production implementations. None is implicitly enabled in Scene.
- Establish representative CPU/GPU timing, queue/submission and memory baselines.
  Optimize from those measurements, including the repeated Editor query work.
  Do not add subgraphs or authored assets without a concrete reusable workload.

Preserve world-authoritative extraction, per-instance mutable state, explicit
resource interfaces, graph/GAL allocation and synchronization, and observable
resource/build failures. The SDK asset library must remain intentional; adding an
operation or passing a compiler test is not rendering acceptance.

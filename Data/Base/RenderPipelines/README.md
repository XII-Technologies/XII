# Pipeline asset library

The `.xiiRenderPipelineAsset` files are ordinary editable render pipeline documents
with persistent document/node/connection identities and node layout. The adjacent
`.xiiBinRenderPipeline` files are packaged, backend-neutral graph resources emitted
by the existing `xiiRenderPipelineContext` asset compiler.

| Asset | Supported use |
| --- | --- |
| Deferred | Existing complete scene rendering stack. |
| EditorScene | Scene rendering with mesh picking, selection and shape icons. |
| OpaqueComposition | Opaque lighting, screen-space reflection trace and temporal reuse, and hybrid reflection composition. |
| ScreenEffects | Diffuse screen-space GI, refraction, atmosphere composition and sensor output. |
| DebugVisualization | Surface queries, channel visualization and debug overlays. |
| TexturePreview | Texture inspection through the existing reflected preview pass. |
| TextureCubePreview | Cube texture inspection with independently authored pass configuration. |

Project assets use the normal AssetCurator transformation/cache and resource GUID
references. Direct paths to the adjacent compiled files select the packaged SDK
snapshot; SDK packaging must update that snapshot when its source changes. Editing
a project pipeline and selecting its asset GUID uses the ordinary cache/hot-reload
path. Runtime code must select a resource explicitly rather than construct a graph
when loading fails.

`EditorDocuments/PipelineAssetLibrary` loads these source documents through the
production document/node manager, reconstructs their Qt nodes and connections,
exports them through the production asset compiler, loads the result through
ResourceManager, and checks byte-for-byte reproduction and repeated export.

The scene assets currently contain the existing reflected composite stages. The
ongoing migration replaces those stages with focused passes and reusable subgraph
assets; this library checkpoint does not claim that decomposition is complete.

The source documents now own persistent reflected pin configurations. Their
compiled snapshots preserve source pin identities and resource constraints; these
settings are lowered into immutable execution metadata rather than document
objects in each runtime pass. The production library regression checks every
pin's backing document identity and reproduces the packaged bytes.

Each source also owns reflected pipeline settings. Scene and texture preview assets
declare single-sample view outputs; reusable subgraphs leave the view-output contract
unconstrained and specify requirements on individual resource pins. Scene assets and
the three operation subgraphs require compute support. Output format remains
unconstrained so compatible view targets can choose their format.
These contracts are validated before pass construction and do not choose topology.

OpaqueComposition is authored from independent typed-member-pin operations. It
preserves the former interface, including optional supplied reflection signals,
through explicit selection nodes. Camera/frame and lighting-result connections are
ordinary graph edges. SSR radiance and identity history belong to each temporal
pass occurrence and view. The optional PlanarReflectionMap output is an empty
half-resolution image from a generic clear operation; this is not an implemented
secondary-view planar reflection renderer. Four former hair/water/SSS/eye
placeholders had no initialized pipelines or executable work and are removed.

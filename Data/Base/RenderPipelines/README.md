# Pipeline asset library

The `.xiiRenderPipelineAsset` files are ordinary editable render pipeline documents
with persistent document/node/connection identities and node layout. The adjacent
`.xiiBinRenderPipeline` files are packaged, backend-neutral graph resources emitted
by the existing `xiiRenderPipelineContext` asset compiler.

| Asset | Supported use |
| --- | --- |
| TransparencyResolve | Weighted accumulation/revealage composition over supplied HDR radiance; optional cleared targets. |
| OpaqueComposition | Nested surface-radiance composition, screen-space reflection trace and temporal reuse, and hybrid reflection composition. |
| RadianceAdd | Independently weighted addition of two supplied radiance images. |
| ConfidenceRadiance | Nonnegative radiance weighted by a signal alpha confidence. |
| VolumetricComposite | Integrated scattering/opacity applied to supplied scene radiance. |
| SurfaceRadianceComposition | Editable addition of supplied direct, indirect, emissive and confidence-weighted near GI, followed by explicit fog. |
| WorldSurfaceLighting | World-derived directional and clustered local lighting contributions over explicit surface inputs. |
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

The removed Deferred and EditorScene assets are not available. Complete scene graphs
still require native rebuilding; this library does not claim those techniques are
fully migrated. Select only assets that implement the required operations.

The source documents now own persistent reflected pin configurations. Their
compiled snapshots preserve source pin identities and resource constraints; these
settings are lowered into immutable execution metadata rather than document
objects in each runtime pass. The production library regression checks every
pin's backing document identity and reproduces the packaged bytes.

Each source owns reflected pipeline settings. Texture previews declare single-sample
view outputs; reusable subgraphs leave the view-output contract unconstrained and
specify requirements on individual resource pins. Device capabilities are declared
by each supported operation. Output format can remain unconstrained so compatible
view targets choose their format.
These contracts are validated before pass construction and do not choose topology.

OpaqueComposition references SurfaceRadianceComposition as an ordinary nested
definition. Addition, confidence weighting and fog are independent typed-member-pin
operations. Intermediate radiance uses RGBA32Float so sums above half-float range
remain valid before attenuation; the fog result is RGBA16Float. Optional supplied
reflection signals use explicit selection nodes. Camera/frame and lighting-result
connections are ordinary graph edges. SSR radiance and identity history belong to
each temporal pass occurrence and view. The unused PlanarReflectionMap selection,
clear and boundary nodes are removed. No secondary-view planar renderer is implied.

TransparencyResolve is an eight-node authored composition: accumulation and
revealage clears, optional supplied-target selection, weighted resolve, optional
texture/buffer forwarding and a particle integration-parameter upload. Its default
cleared targets preserve HDR radiance. Actual transparent geometry must produce
accumulation/revealage through other operations; the removed translucent draw
branch never initialized a graphics pipeline and was not a functioning renderer.
The resource requests no scene extraction or GPU scene services. It does not
allocate particle state. ParticleIntegrate is a separate reflected operation over
explicitly supplied structured state and constants; particle components/runtimes
retain their existing ownership. Integration changes external state and therefore
remains live even without a same-frame rendering consumer.

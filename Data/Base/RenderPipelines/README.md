# Default render pipelines

| Resource | Use |
| --- | --- |
| Scene | World meshes, materials, depth, basic directional/local lighting and presentation. |
| Editor | Scene rendering plus selection, picking, visualization, icons and debug overlays before presentation. |

The source documents are ordinary editable pipeline assets. Compiled resources
are emitted into the ignored asset cache and checked for deterministic reproduction.
Scene is independent; Editor references Scene through explicit interfaces.

Fixed texture and cubemap previews use C++ resource descriptors. Specialized sensors,
offline rendering and user-authored/programmatic pipelines remain supported by the
same compiler and runtime. The retired SDK fragments are not required to construct them.

See [the renderer contract and validation boundary](../../../Documentation/Rendering/SceneRenderer.md).

Compiled `.xiiBinRenderPipeline` files are generated output and must never be
committed. The Editor transforms the authored sources into its normal
`Data/Base/AssetCache/Common/RenderPipelines` cache; profile defaults and the Editor
subgraph reference use the source asset GUIDs through the existing lookup table.
For standalone GPU tests/sample preparation, run `EditorFrameworkTest -noGui -run
-close -all -filter RendererAssets -outputDir <project-local-test-output>`. This
exports both sources with the production compiler into the ignored cache, checks
stable exports and contracts, and is the CTest fixture for the GPU suites. The
standalone sample mounts that cache; a missing transformation remains a loading
failure rather than generating a substitute renderer. Core CPU tests require no
transformed assets or Editor dependency.

# Pipeline asset library

The `.xiiRenderPipelineAsset` files are ordinary editable render pipeline documents
with persistent document/node/connection identities and node layout. The adjacent
`.xiiBinRenderPipeline` files are packaged, backend-neutral graph resources emitted
by the existing `xiiRenderPipelineContext` asset compiler.

| Asset | Supported use |
| --- | --- |
| Deferred | Existing complete scene rendering stack. |
| EditorScene | Scene rendering with mesh picking, selection and shape icons. |
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

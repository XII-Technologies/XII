# Model importing

Use **Animated Mesh, Skeleton, Materials and All Animation Clips** in the asset import dialog for a skinned FBX, glTF or GLB. This creates a mesh with its default skeleton, a skeleton with its preview mesh, and one animation asset per source clip. Each clip selects its source animation and uses the generated mesh for preview. Individual mesh, skeleton and animation import modes remain available.

The complete mode uses source-space coordinates consistently across the mesh, skeleton and clips. Standalone static mesh and skeleton import settings retain their coordinate presets.

## Geometry quality

**RecalculateTangents** uses MikkTSpace. Face corners split at tangent discontinuities before full-attribute welding, cache optimization and vertex-fetch optimization. Static meshes also receive overdraw optimization. Skin indices and weights participate in welding, and simplification protects bone-palette boundaries.

**GeneratedLODs** adds up to four LODs to the normal native mesh resource. Each targets half the triangles of its predecessor, subject to the simplification error limit and attribute constraints. Generated LODs use the existing sections, material slots, meshlet metadata and runtime LOD selection. Zero retains only the imported mesh. Existing simplification controls still determine the base mesh quality.

Meshes retain two UV sets and one vertex-color set. Extra sets produce warnings. A UV mapping shared by all textures of a material is baked into the section's UV0 and its tangent basis is regenerated when required.

## Materials, textures and reimport

Material slots determine color space and texture usage. Unassigned slots create native PBR materials and texture documents beneath `<mesh>_data`. Embedded images have stable extraction paths and update when their source bytes change, even if materials already have assigned resources. A background transform requests foreground import when extraction needs to write files.

External paths resolve relative to the model after separator normalization and path cleaning. If the exact path is absent, resolution checks the model folder and adjacent `textures`, `Textures`, `maps` and `Maps` folders. Ambiguous matches are reported instead of choosing an arbitrary file.

A missing external image still receives a texture document with its intended source dependency. Restore the file or edit **Input1** in that document. Optional texture conversion failures produce warnings and retain the material's scalar/color properties so geometry remains inspectable.

Texture assets' filtering, addressing, mip limits and LOD bias now reach native material sampling. Scene's color, normal, packed material, occlusion, emissive and transmission inputs use the sampler belonging to their texture. Depth/alpha coverage, surface picking and emissive mesh-light extraction use the same settings. Ray-tracing material records also retain sampler indices for their explicit-LOD consumers.

Identical GAL samplers share one descriptor across textures. Reloads and material changes publish new indices; retired descriptors remain alive until their last-use frame completes. Explicit descriptor-only material bindings can provide a sampler index through `SetBindlessIndex`, or leave it invalid to retain the consumer's legacy sampler. Runtime sampler arrays use a separate 256-entry API capacity rather than the 4096-entry texture table; exhaustion is diagnosed and uses the legacy fallback.

Repeated complete imports reuse existing documents. Authored material assignments, texture settings, skeleton settings and clip settings are not overwritten. Existing generated documents can be reused before curator indexing completes. Duplicate clip names receive stable source-index suffixes; generated clip filenames also include the complete label's hash.

## Preview behavior

Mesh previews wait for final resources before committing their initial camera bounds. Relative preview cameras and thumbnail cameras scale clipping planes to the asset. Scene thumbnails use fixed exposure rather than adapting against their black background. Shared mesh-buffer previews inherit the buffer's meshlet ranges. Animation timeline scrubbing clamps to the last pose even for looping clips; the **Looping** property controls runtime playback. Skeleton visualization overlays the preview mesh and can render while that optional mesh is still loading.

## Current representation limits

Native materials still expose one texture per slot and one common UV mapping. Independent mappings and additional texture layers are diagnosed; unrepresentable mappings retain scalar material properties.

Morph animation and root-motion extraction are not implemented by this change. Native morph storage and root-motion metadata exist, but importing animated morph tracks and producing usable root-motion playback require additional animation/runtime work. Existing explicit errors for these unsupported conversion paths remain.

## Verification

Windows/MSVC Debug builds and the existing Vulkan engine-process suite cover the importer, native resources, skinning, previews and LOD integration. The optional `-importModel <absolute model>` argument exercises real models through `ModelImportWorkflow/CompleteModel`, `EditorViews/AnimationPreview` and `EditorViews/MeshPreviewEditorPipeline` without adding downloaded models to the repository. Khronos Fox and Damaged Helmet were used during implementation.

Desktop interaction and additional platforms/backends require separate acceptance checks. A successful export alone does not establish that a thumbnail or viewport contains visible geometry.

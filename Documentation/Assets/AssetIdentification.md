# Asset identification and resolution

An authored asset has one persistent document GUID. Sub-assets have their own GUIDs and belong to an owning document. File locations are lookup inputs, not asset identity. Moving a document does not change its GUID; copying it to create a different asset requires a distinct GUID.

## Reference forms

| Form | Meaning |
| --- | --- |
| `{ 12345678-abcd-4321-1234-123456789abc }` | XII GUID spelling. Hexadecimal digits are validated; runtime IDs use the spelling produced by `xiiConversionUtils::ToString`. |
| `Materials/Steel.xiiMaterialAsset` | Path relative to a configured data directory. It must have exactly one match. |
| `FeatureLab/Materials/Steel.xiiMaterialAsset` | Editor data-directory-parent-relative path. The directory name qualifies the location, provided that name is unique among configured directories. |
| `:project/Materials/Steel.xiiMaterialAsset` | Rooted virtual filesystem path. The root selects the data directory. |
| `D:/Project/Materials/Steel.xiiMaterialAsset` | Absolute source path inside a configured data directory. |
| `Meshes/Scene.xiiMeshAsset\|MeshName` | Named sub-asset. Resolution returns the sub-asset GUID; transformation operates on its owning document. |
| `AssetCache/Default/Materials/Steel.xiiBinMaterial` | Compiled output location. It is not the source asset identity and can change with profile/output tags. |
| `Native/Materials/Missing` | Resource identifier owned by runtime code. A string is not an authored asset merely because it resembles a path. |

Path lookup cleans separators and `.`/`..` components. Editor file lookup uses the existing file model's comparison policy and never searches outside a selected data directory through relative traversal. Path case is preserved. Runtime table keys use exact spelling; use authored casing for portable references. A registered GUID is the durable choice for serialized asset references.

Ambiguous paths fail. The diagnostic names conflicting locations and asks for a GUID, absolute path, or rooted path. Explicit exhaustive Editor lookup permits a **unique** path suffix for authoring/import suggestions; it does not choose by hash-table order, shortest path, or an arbitrary first match.

## Editor entry points

`xiiAssetCurator::ResolveAsset` accepts GUIDs and source paths and returns a persistent GUID with `xiiStatus` diagnostics. Its output is invalid on failure. `FindSubAsset` provides the same resolution with a lifetime-protected lookup and an empty result on failure. `GetSubAsset(xiiUuid)` remains the typed identity lookup.

`TransformAsset(xiiStringView, ...)` accepts the same forms. GUID overloads also accept sub-asset IDs and transform the owning document. `xiiAssetProcessor::QueueTransformAssetReferences` resolves a batch before queuing it. Typed queues normalize sub-assets to owner documents before saving and reserving work. Invalid references fail rather than silently disappearing from the request.

`xiiFileSystemModel::ResolveFilePath` resolves physical file locations with `xiiStatus` diagnostics; it does not interpret GUIDs. `FindFile` wraps it for callers needing only success/failure and file status. The Editor's path conversion helper, asset property widgets, document opening, importer suggestions, dependency inspection, and processor dependency discovery use these shared semantics. Asset property widgets resolve path-backed assets for type checking, thumbnail display, and circular-dependency checks, and commit selected assets as GUIDs.

Existing serialized references are not rewritten as part of scanning or lookup. Existing valid GUIDs and paths remain readable. Renaming does not preserve an obsolete path as an alias: GUID references follow identity, while references to an old location must be updated.

## Runtime entry points

The existing per-data-directory, per-profile `.xiiAidlt` table contains two record kinds:

```text
{ asset-guid };Default/Materials/Steel.xiiBinMaterial
Materials/Steel.xiiMaterialAsset;{ asset-guid }
```

The example abbreviates GUID text; actual records use the full XII spelling. There is no second identifier registry. Source-path records point to the GUID in the same table used for compiled output redirection. Sub-asset path records include `|Name`.

`xiiFileSystem::ResolveAssetIdentifier` resolves registered source paths to GUIDs, normalizes GUID spelling, and preserves unregistered resource identifiers verbatim. It fails on path ambiguity and clears the output. Runtime source-path resolution requires the project's asset tables to be mounted and loaded. It does not discover or transform source documents at runtime.

`xiiResourceManager::LoadResource`, typed/typeless existing-resource lookup, asynchronous resource creation lookup, and resource-handle deserialization share canonical ID resolution. Loading an asset by GUID and its registered source path therefore returns the same resource handle, including during concurrent calls. Resource serialization writes that canonical ID. Ordinary parameter changes and resource acquisition do not create an extra asset registry.

Raw file reads remain file reads: `xiiFileReader::Open("Materials/Steel.xiiMaterialAsset")` opens authored data. Only resource APIs resolve authored source paths to compiled resources. Direct compiled file IDs and application-created resource IDs retain their existing independent meanings. Asset GUID redirection continues to locate compiled data through the normal filesystem loader. Type override lookup uses the same mount precedence as file loading.

## Transformation and invalidation

Asset dependencies expressed as GUIDs or source paths resolve to their owners for hashing and scheduling. Both incorporate the same transitive asset dependency hashes instead of hashing a referenced document as an unrelated raw file. Raw source/compiler inputs retain content-based file hashing. Transform, thumbnail, and package dependency categories keep their distinct effects.

Reverse tracking includes normalized candidate locations for relative dependencies that do not exist yet. Creating, deleting, or adding an ambiguous second match invalidates consumers. GUID dependencies are rebound when source assets move, disappear, or return. Transitive consumers are invalidated, and circular references are rejected before transformation.

Direct document transforms share the background scheduler's asset reservation. The check and reservation happen together under the curator mutex; scoped release covers failure paths. Existing background request snapshots, dependency reservations, import application, and profile-specific hash evaluation continue to govern subprocess work.

Asset tables rebuild aliases from current curator state, invalidate cached profiles on asset changes, and discard borrowed profile pointers before those profiles are destroyed. A move removes obsolete table entries and schedules resource reload through the unchanged GUID. Reload messages follow successful asset-table publication; a failed publication keeps the table dirty and does not announce a reload against stale mappings.

## Regression coverage

- Foundation `IO/AssetIdentifiers`: GUID spelling, normalized relative/rooted/absolute/parent-relative paths, ambiguity, CRLF and unterminated final table records, table reload, and source/compiled file separation.
- ToolsFoundation `FileSystem/ReferenceResolution`: indexed file normalization, conflicting data directories, rooted disambiguation, missing files, and traversal rejection.
- EditorFramework `AssetResolution/EditorAndRuntime`: authored document/sub-asset resolution, dependency-first transformation, transitive path invalidation, missing input recovery, source relocation, canonical production surface handles, concurrent loading, resource serialization, and hot reload.
- Existing Editor asset processor/profile/background suites cover subprocess snapshots, profile changes, imports, cancellation, and overlapping requests. Existing dependency tracking/resave suites cover missing GUIDs, package/thumbnail dependencies, same-timestamp writes, and cycles.

Shader permutation caches and importer-specific dependency declarations remain owned by their existing compilers/importers. A compiler input must be declared as a dependency to participate in the asset curator's content hashes; asset-reference resolution does not infer dependencies from arbitrary strings or parse source languages.

## Local validation

Windows/MSVC Debug builds passed for `Editor`, `EditorFrameworkTest`, `CoreTest`, `FoundationTest`, and `ToolsFoundationTest`. The new Editor/runtime workflow passed, as did Foundation IO and communication tests, ToolsFoundation filesystem tests, Core resource-manager tests, and the existing Editor processor/profile, background transformation, background import, rescan/shutdown, and full startup/document integration suites. Full Editor integration includes dependent asset exports, profile evaluation, same-timestamp source edits, cycle checks, and real scene/pipeline documents.

The initial regression runs exposed rooted-path traversal, a competing save-triggered transformation, and Windows canceled-connect handling; their corrections passed subsequent runs. The new standalone Editor fixture also required normal Qt event processing during startup/teardown. Formatting and diff whitespace checks passed. Other operating systems, release configurations, archive-backed source-path tables, and exhaustive project-wide asset transformation were not exercised locally.

# Reflected settings and runtime CVars

Settings are structured configuration. CVars are lightweight runtime controls and diagnostics. Both use existing XII reflection, DDL, events, plugin lifecycle and editor infrastructure; there is no additional type registry or registration macro.

## Ownership and discovery

`Foundation/Configuration/Settings.h` provides `xiiSettings`, derived from `xiiReflectedClass`. It has no Qt, Core, filesystem path policy, singleton or editor dependency. Constructors and reflected setters must only update object state. Owners use `Create()` to apply explicit reflected member defaults over constructor defaults, restore with `ReadFromDDL()`, validate, commit with `Apply()`, and subscribe to `m_SettingsChangedEvent`. Loading and validation copies never notify; committing identical values never notifies. Call `NotifyChanged()` after validating direct programmatic edits.

`Validate()` checks finite scalar floating-point values, member clamp attributes, enum values and bitfields. Override it, calling the base implementation, for cross-property invariants, nested structures, collections or subsystem-specific constraints. Return `xiiResult`; do not throw. Runtime consumers should react to committed changes, not poll every property every frame. Reflected accessors must not initialize subsystems or send IPC while a temporary object is being restored.

The base does not select a global owner. Existing specializations retain their responsibilities:

| Context | Owner and storage |
| --- | --- |
| Application / user preferences | `xiiPreferences` editor adapter; application preferences folder, existing `.pref` DDL |
| Project user preferences | Same adapter; current project's user-specific preferences folder, existing `.pref` DDL |
| Document user preferences | Same adapter; current project plus document UUID folder, existing `.pref` DDL |
| Shared project / target configuration | Existing project configuration and `xiiPlatformProfile` owners; profile DDL and runtime chunk export remain unchanged |
| Runtime application user configuration | Application-owned `xiiGameApplicationSettings`; `:appdata/RuntimeConfigs/Presentation.ddl` |
| Scene / world configuration | Existing `xiiSettingsComponent` and ECS ownership; intentionally independent of global settings |

Project preferences have always been user-specific; the common base does not silently turn them into shared project configuration. Shared settings keep their existing project serialization owner. Platform/runtime metadata does not make an object an editor singleton or a platform-profile child: derive from `xiiProfileConfigData` for profile ownership and implement its existing runtime chunk methods.

`xiiPreferences` and `xiiProfileConfigData` now specialize the common base. Their reflected class names, property names, preference filenames, public query methods and runtime formats remain stable. Rebuild dependent modules/plugins after this C++ ABI change. `xiiPlatformProfile::AddMissingConfigs()` excludes abstract/non-allocatable types; live profiles release plugin-owned configs after plugin subsystem shutdown, before DLL unload. Re-run `AddMissingConfigs()` after loading new profile plugins as before. Profile config pointers are borrowed and expire on profile clear or owning plugin unload.

The surrounding configuration mechanisms retain their existing owners and formats:

- Application filesystem/plugin DDL and `xiiWorldModuleConfig` describe startup mounting, plugin dependencies and world-module implementation selection. They remain ordered bootstrap commands, not editor user singletons.
- Input mapping DDL remains a collection of actions applied through `xiiGameAppInputConfig`. Window creation descriptions remain caller-owned startup descriptors.
- `xiiConfigFileResource` remains a resource-managed, preprocessed `.xiiConfig` asset with include/override semantics. Its resource lifetime and authored source precedence must not become a second writable preference file.
- Pipeline settings, display-output configuration and view-render settings remain authored per-resource/per-view values. Their existing reflection, graph migration, export hashes and IPC serialization retain their separate contracts. Render-pipeline and texture profile configs acquire the common base through `xiiProfileConfigData`.
- Qt docking/window geometry, shortcuts and custom compiler/code-editor property widgets retain their existing owners. The common settings dialog reuses those property widgets; it does not move Qt layout state into Foundation or make project-generation settings global runtime state.

## Declaring an editor-visible settings type

Declare a concrete reflected type in its owning module. No change to the settings dialog is needed:

```cpp
class XII_MYMODULE_DLL xiiMySettings : public xiiSettings
{
  XII_ADD_DYNAMIC_REFLECTION(xiiMySettings, xiiSettings);
public:
  xiiUInt32 m_uiIterations = 8;
};

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiMySettings, 1, xiiRTTIDefaultAllocator<xiiMySettings>)
{
  XII_BEGIN_ATTRIBUTES
  {
    new xiiSettingsScopeAttribute(xiiSettingsScope::Project),
    new xiiCategoryAttribute("Simulation/Solver"),
  }
  XII_END_ATTRIBUTES;
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Iterations", m_uiIterations)->AddAttributes(
      new xiiDefaultValueAttribute(8), new xiiClampValueAttribute(1, 64)),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
```

Use the owning DLL macro and direct headers in real code. The example requires `Foundation/Configuration/Settings.h`. In editor code, access the borrowed instance through `xiiPreferences::QuerySettings<xiiMySettings>()`; document settings also pass the document. Runtime code supplies its own appropriate owner and must not depend on this editor adapter. Existing preference subclasses may continue using `QueryPreferences<T>()` and `m_ChangedEvent`; both the legacy and common events fire at the same commit boundary. Do not subscribe to both for the same operation.

`GatherTypes(scope, ...)` uses `xiiRTTI::ForEachDerivedType` with `ExcludeNotConcrete` and reads metadata before any allocation. `GatherSettings()` creates only application settings, current project settings when a project is open, and document settings for the supplied context. It includes previously queried legacy preferences. Add scope metadata to legacy plugin types to discover them before their first query. A legacy constructor's domain must match its metadata. An unannotated, never-queried legacy type cannot be safely classified without constructing it. Optional editor metadata `xiiDocumentSettingsAttribute("xiiGameObjectDocument")` restricts a document setting to that reflected document type and its descendants before construction; an unavailable context type is skipped. Existing scene, asset-view and game-object-context preferences declare their actual document contexts. Use `xiiTitleAttribute` or the existing translated RTTI type name for the navigation label; persistence identities are not shown as labels.

New direct subclasses use `Settings_<RTTI type name>.pref` in the selected preferences folder. Legacy preference subclasses retain their existing unique filenames. Keep RTTI names stable; use existing graph patches and explicit file migration for renames. Invalid filenames, conflicting persistence identities and mismatched domains are rejected. The same document settings type may have independent instances for multiple document UUIDs. Do not retain borrowed pointers past document/project close or plugin unload. Discover again after plugins change; do not cache RTTI pointers across unload.

## Editing, validation and application

The Settings dialog groups actual types by Application, Project and current Document, then their reflected category paths. Search matches scope, category, type identity and visible property names. It reuses the existing Qt property grid, supported structured editors, property groups, default/range attributes and translated property tooltips. Platform profiles keep their existing profile UI, ownership and export operations.

- Opening copies reflected data into a temporary document. Editing and Reset Category affect those copies only. Reset preserves hidden bookkeeping properties.
- Apply validates all category candidates, saves changed categories, then commits and notifies changed live instances. OK performs Apply and closes only on success. Cancel discards edits since the last successful Apply.
- A validation or save error keeps the dialog open and leaves live instances unchanged. Files are saved individually using existing deferred writers; an I/O failure after another category was saved can leave some files updated. This is not a multi-file transaction. Retry after correcting the error.
- Types/properties marked with `xiiSettingsRestartAttribute` show an explicit restart/reinitialization notice. The owning subsystem decides the safe application point; the dialog does not restart systems. Unmarked settings use the existing consumer's immediate commit behavior.
- An empty context is valid. Project/document categories are not fabricated. Plugin changes or destruction of owned settings synchronously release the temporary document and close the dialog before RTTI becomes invalid.

`xiiEditorPreferencesUser` applies global menu/translation state and sends the existing `xiiGlobalSettingsMsgToEngine` only at the commit notification, never while validating a clone. Engine-process consumers continue to receive serialized IPC messages; no editor pointers cross the process boundary.

Persisted DDL is graph-patched using existing facilities. Missing properties retain initialized values and removed properties are ignored. Scene preferences advance to version 2: legacy camera speeds 25..30 are clamped to the movement/public-setter range 0..24 through a graph patch; the intended constructor default 9 now matches its reflected default. Other legacy fields are preserved. Missing roots, incompatible root types, invalid ranges and invalid scalar values fail transactionally. Unreadable/invalid preference data is diagnosed and protected from automatic overwrite for the lifetime of that instance. Repair or move the original file, then reload its owner: restart the editor for application settings, or reopen the project/document for other scopes. This preserves the evidence instead of silently replacing user configuration with defaults. Settings file writes and runtime settings writes report failures.

## CVar audit (2026-10-29)

The audit covered the complete first-party source and sample/template tree, CVar implementation and tests, console/autocomplete, script accessors, telemetry/Inspector/editor IPC controls, command-line loading, per-plugin config files, plugin initialization and shutdown. There were 13 production CVar definitions and 28 test definitions before this change. Test fixtures are controls for persistence, defaults, delayed sync and plugin load/unload; they are not dead production configuration. The Core lifetime regression adds two scoped test definitions, and the frame warning adds one production definition.

No production CVar was proven unused across its supported development configurations. The `VisualScript.MaxNodeExecutions` registration is now excluded from Shipping, matching its existing development-only consumer; it previously had no consumer there. A CVar being used only through console/Inspector/scripts is not sufficient evidence to remove it. Accordingly there are no fabricated removals or unproven name changes. Removed supporting code includes the script CVar pointer cache and its accumulating event registrations: every script lookup previously added another handler, cached misses stayed stale, and cached plugin pointers could outlive their DLL. Direct case-insensitive lookup now observes creation, replacement and unload without a second registry.

| CVar | Classification | Consumer, defaults, flags and decision |
| --- | --- | --- |
| `App.VSync` | Migrate, compatibility control retained | Persistent presentation policy moves to reflected runtime settings. Existing public CVar symbol, console/script name, boolean default `true` and Save flag remain for compatibility/fallback. Changes write through to the application settings owner and output targets. New outputs also receive the committed value. |
| `App.ShowFrameStats` | Keep | `RenderFps()` overlay and input toggle; `false`, Save. Development diagnostics with a useful remembered preference. |
| `Debug.TextScale` | Keep | Debug renderer glyph layout, upload and public text-scale API; `1`, Save. Runtime readability/calibration control, not a competing render-profile setting. |
| `Streaming.TextureLoadDelay` | Improve | Real resource-loader fault injection; `0` seconds, now transient. Ignore non-finite/negative inputs and cap a single injected delay at 60 seconds. Saved artificial delays should not unexpectedly slow future sessions. Old keys remain harmless, ignored by normal save-flag-aware loading; explicit command-line/ignore-save loading still works. |
| `Graphics.DynamicResolution.TargetFrameTimeMs` | Keep | GPU timestamp driven PID controller; `16` ms, transient. Target clamped at the use site; does not duplicate authored base render scale. |
| `Graphics.DynamicResolution.MinimumRenderScale` | Keep | PID bound relative to authored scale; `1`, transient, native resolution until explicitly lowered. |
| `Graphics.DynamicResolution.MaximumRenderScale` | Keep | PID upper bound; `1`, transient. Existing use-site bounds retained. |
| `Spatial.Queries.CachingThreshold` | Improve | Spatial query cache heuristic; `100`, transient, minimum `1` enforced at use site. Remove captured-this subscription when each spatial system dies, preventing a callback into a destroyed cache mutex. |
| `Profiling.DiscardThresholdMS` | Keep | CPU/GPU profiling scope filtering and public threshold setter; `0.1` ms, transient. |
| `DirectoryWatcher.ForceNonNTFS` | Keep | Windows directory watcher fallback-path testing; `false`, transient. Concrete platform-specific diagnostic. |
| `Texture.PenalizeDXConversions` | Keep | Conversion-path selection during DirectX texture converter initialization; `false`, RequiresRestart. Persisted startup experiment with real fallback/driver diagnostic use; restart/delayed-sync semantics preserved. |
| `VisualScript.MaxNodeExecutions` | Improve | Development-only script invocation execution budget; `100000`, transient. Registration now uses the same build guard as the consumer. Values below `1` now mean `1`, rather than negative values converting into enormous unsigned budgets. |
| `Editor.Scene.UseCullingCamera` | Keep | Engine-plugin scene view selects the real CullingOnly camera; `false`, transient. Editor-process CVar IPC remains unchanged. |
| `App.FrameTimeWarningMS` | New | Actual measured wall-clock frame time in `Run_FinishFrame()`; `0` disables, transient. Finite positive thresholds log frame hitches without changing clock smoothing or simulation stepping. |

The 28 existing fixtures are: `testCVar{Int,Float,Double,Bool,String}` (5), `test1_{Int,Float,Double,Bool,String}` and corresponding `*2` fixtures (10), `test2_{Int,Float,Double,Bool,String,Inited}` (6), and `TestPlugin{1,2}{InitCount,UninitCount,Reloaded}` plus `TestPlugin2FoundDependencies` (7). All are retained for named lookups, state restoration or lifecycle assertions. The added scoped `SettingsTest.Dynamic` fixtures exercise replacement and destruction through the public script accessors.

### VSync migration and precedence

After existing CVar startup loading, `xiiGameApplication` loads `:appdata/RuntimeConfigs/Presentation.ddl`. If it does not exist, its default comes from the existing loaded `App.VSync` value. The resulting reflected configuration is saved. The old key remains a compatibility mirror, so a failed new-file write does not destroy the legacy fallback. Once the reflected file exists, legacy saved CVar values cannot override it. An explicit `-App.VSync` command-line override wins over both, and runtime console/script writes commit through the settings owner. The compatibility CVar is not independently consumed as presentation state anymore.

`-no-file-cvars` also disables the presentation file read/write. An invalid reflected file is preserved; the legacy/default value and explicit command-line overrides remain usable. Failed writes are reported and the compatibility mirror remains available for a later migration retry. There is no arbitrary JSON format, no preference-to-CVar mirroring framework and no blanket conversion of runtime diagnostics into persistent settings.

## Verification

New tests cover reflected discovery before queries, explicit/constructor defaults, non-allocatable types, missing/removed DDL properties, invalid values and roots, transactional loads, change notification counts, plugin type load/unload, per-document instances, project isolation, settings UI search/reset/Apply/Cancel, context invalidation, saved CVar migration and reflected-file precedence. Core regressions cover script CVar lookup lifetime and spatial subscription teardown. Existing CVar persistence/restart tests and platform export failure tests remain applicable. Build and executed-test results are recorded in the pull request; do not infer platform coverage from compilation alone.

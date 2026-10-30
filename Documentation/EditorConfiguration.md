# Editor configuration

The editor has three configuration entry points, in this order:

```
Edit > Configuration > Project Settings
Edit > Configuration > Editor Preferences
Edit > Configuration > Plugins
```

They navigate one modeless window. Switching sections retains pending edits. Project Settings contains shared project configuration; Editor Preferences contains user settings, including explicitly labeled current-project and document user scopes. Plugins edits the current project's bundle requirements. Without a project, user preferences and searchable plugin availability remain accessible; plugin requirements are read-only.

## Ownership and pages

| Configuration | Location in the shared window | Existing owner / format |
| --- | --- | --- |
| Reflected project settings from modules/plugins | Project Settings, RTTI category path | `xiiEditorSettings` adapter, project `Editor/Settings/Settings_<RTTI name>.pref` |
| Platform, graphics, simulation and asset profiles | Project Settings / Platform / Platform and Asset Profiles | `xiiAssetCurator` and `xiiPlatformProfile`, existing profile DDL/runtime exports |
| Input sets, actions and slots | Project Settings / Input / Input Actions | Existing `RuntimeConfigs/InputConfig.ddl` |
| Asset/resource search roots | Project Settings / Assets / Data Directories | Existing filesystem config, `RuntimeConfigs/DataDirectories.ddl`; reload project after changing |
| Tags | Project Settings / General / Tags | Existing tools tag registry and `RuntimeConfigs/Tags.ddl` |
| Runtime window defaults and explicit Player user override | Project Settings / Platform / Window Configuration | Existing project and per-user `RuntimeConfigs/Window.ddl` paths, separately labeled inside the page |
| Editor behavior, viewports, lighting and snapping | Editor Preferences / Editor / General | Existing `xiiEditorPreferencesUser` reflected properties and `General.pref` |
| Keyboard bindings | Editor Preferences / Interface / Keyboard Shortcuts | Existing action descriptors and shortcut persistence |
| C++ toolchain user preferences | Editor Preferences / Editor / C++ Projects | Existing reflected `xiiCppProject` properties and preference identity |
| Project/document user preferences | Editor Preferences / Project (This User), or Document / document path | Existing per-user project/document preference folders |
| Plugins | Plugins | Existing bundle discovery and project `Editor/PluginSelection.ddl` |

Categories appear only for real pages or editable reflected types. Physics/rendering options appear when their profile/config types are supplied by loaded modules; there are no placeholder physics or rendering pages. Export, asset import/editing, C++ project generation/build/open commands, runtime profile switching and quick snapping presets remain operational workflows. They do not duplicate configuration editors.

The former preferences dialog, independent plugin selection dialog, snap settings dialog and unused settings-tab form are removed. Input, tags, data directory, window, profile and shortcut dialogs are now embedded `QWidget` pages. The empty document-window host remains responsible for the editor's workspace/menu lifetime. Legacy command identifiers remain registered for saved keyboard bindings but navigate the common window; they have no duplicate menu entries. `Gizmo.SnapSettings` retains its End shortcut and opens reflected editor preferences.

## Reflected extensions and compatibility

Use `xiiSettings` and RTTI scope/category/title metadata as described in [Settings.md](Settings.md). A direct project-scoped settings type is shared configuration. For user settings scoped to an application, project or document, derive from `xiiEditorSettings`, declare matching scope metadata, and retain a stable constructor persistence name. The adapter discovers eligible types using Foundation's RTTI discovery, and the window displays them through the existing property grid. No window category list or registration macro is needed for a new reflected type.

The old `xiiPreferences` base, query wrappers, separate change event and Preferences source directory are removed. All callers now use `EditorFramework/Settings/EditorSettings.h`, `xiiEditorSettings::QuerySettings<T>()`, `xiiSettings::m_SettingsChangedEvent` and `NotifyChanged()`. Concrete preference RTTI names, property identifiers, `.pref` filenames, graph patches, Qt bootstrap splash state and IPC messages remain stable. External C++ plugins must migrate these source APIs and rebuild.

The initial unified adapter put direct project-scoped settings in a user folder. When no shared file exists, the new adapter restores that previous file and saves subsequent commits to the project's shared settings folder. The old file is retained. A shared file always wins. Actual user preference subclasses keep their existing user-specific paths and never participate in that migration. Invalid reflected settings files remain protected from automatic overwrite.

## Commit and lifecycle behavior

Reflected editing and Reset Category modify detached data. Embedded pages also stage their edits. Apply commits changed pages, OK applies and closes on success, and Cancel discards pending edits since the last Apply. Revert Page reconstructs a specialized page from its current committed data; shortcut Reset operates on the selected shortcut. Opening or navigating pages does not save settings. Window configuration no longer creates a file on open, and shortcut assignment no longer changes global actions before Apply.

Plugin metadata shows bundle identity, description, editor/engine/runtime module lists, dependencies, required/missing status, categories and requested activation state. Category and text filtering do not change requirements. Required dependencies cannot be disabled while needed; missing dependencies and exclusive-feature conflicts prevent commit. Cyclic metadata cannot recurse indefinitely. A checked entry means requested by this project or required by another bundle, not proof that the DLL is currently loaded. Apply writes the existing selection format and marks editor restart required. The copy option concerns runtime DLLs and only enables reload behavior already supported by the plugin architecture. The window never loads or unloads DLLs on a checkbox change.

On project transitions, current document invalidation or plugin changes, the window discards drafts and synchronously releases widgets, action pointers and reflected objects before their owners disappear. The next invocation discovers the new context. An already open window keeps its explicitly labeled document context until that context closes; switching editor document tabs does not silently replace its draft. Reading another project's plugin selection resets both activation and runtime-copy state for unmentioned bundles; availability-only browsing clears project requirements in its local snapshot.

Persistence remains per page/file, not a multi-file transaction. An I/O failure leaves the window open and reports an error. Earlier successful page saves remain committed; Cancel does not undo Apply or those successful saves. Profile commits retain their existing transactional file/export behavior. The window records editor restart and project reload requirements without prompting or performing those actions. Active profile changes retain the curator's normal engine-process application behavior.

## Verification

`EditorFrameworkTest` covers the real action-map hierarchy and command routing, modeless-window reuse, reflected discovery/default/reset/validation, user/shared scope separation, legacy project file restoration, Apply/Cancel, input page persistence, staged shortcuts, plugin selection persistence and filtering/dependency validation, and project/plugin/document invalidation. Existing native editor integration continues exercising profile authoring/export, scenes and editor-to-engine IPC. UI captures are generated in the test output for visual inspection.

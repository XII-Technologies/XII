# XII Editor workspace

The editor uses Qt Widgets, GuiFoundation action maps and Qt Advanced Docking
System (ADS). The redesign evolves those shared controls so document plugins
inherit the same behavior. Document formats, action identifiers, shortcuts,
undo/redo and renderer integration remain owned by their existing modules.

## Design

Use the splash screen's black shell, neutral gray surfaces, white text and
light-blue (#6da9f7) accent. Component category colors occupy a narrow header
marker rather than tinting the complete property group.
Reserve the accent for selection, focus and active context. Use the system UI
font and logical pixel metrics so controls follow screen scaling and text size.
Keep the viewport dominant, with editing commands near their context. Search,
keyboard access and labeled menus complement icons. Keep disabled commands
visible where they explain available operations.

GuiFoundation owns palettes, control metrics, search, command presentation,
docking and property controls. EditorFramework owns project context, content
navigation, viewport tools and shared object workflows. Plugins retain their
document-specific actions and adapters. Do not introduce a second command or
selection registry.

## Review sequence

Each layer is a separate PR based on the preceding layer. Merge from the bottom
of the stack, retargeting the next PR after each merge.

1. Shared palette, scalable control metrics, flat dock tabs and inline search.
2. Workspace shell and contextual command discovery using live action proxies.
3. Readable viewport controls and contextual navigation help.
4. Scene Outliner navigation and selection feedback.
5. Inspector grouping and editing navigation.
6. Content navigation, search and quick access to the Asset Browser.
7. Workspace layout controls using ADS persistence.
8. Diagnostic navigation and explicit log-follow behavior.

## Acceptance

Build Editor and EditorFrameworkTest with dependent plugins. Exercise search and
commands, document switching, viewport menus, hierarchy selection, property
editing and undo, asset filtering, layout persistence and log navigation. Inspect
native Qt captures at 100%, 150% and 200% scaling, including a compact window.
Preserve existing layout identifiers. Check startup restoration and engine IPC
with the existing native integration tests. Record platform/backend gaps in each
PR; a build alone is insufficient evidence of usable editor workflows.

## Workspace interactions

- **Ctrl+Shift+P** searches live commands for the focused document and viewport.
  Disabled commands remain visible. Hidden commands and commands from unrelated
  documents are excluded. Entries use existing shortcuts, enabled states and
  action callbacks, including plugin actions.
- Viewport menus display the current perspective and shading choice. Secondary
  display and device controls, camera speed, grid, selection outlines and
  visualizers live under **View Options**, keeping the document toolbar focused
  on authoring and Play.
- The title area contains the active document's live menus, project context,
  command search and workspace controls. Windows uses Qt's expanded client area
  with accessible Qt caption buttons and platform move/resize handling. Other
  platforms retain their normal window decoration. Detached documents retain
  local menus. Compact windows use icons for Commands and Workspaces, with the
  same tooltips and keyboard access. Application panels are in Workspaces.
- Document panels can be resized below the former 300-pixel minimum. Outliner
  navigation and selection feedback share one footer. Inspector keeps a separate
  selection-context row for multiple objects and an empty-selection hint; a
  single object's existing Name field supplies its identity.
- **Outliner** search preserves document selection. Clearing search restores the
  expansion state from before the search. **Reveal**, **Frame** and **Collapse**
  provide direct hierarchy navigation. Ctrl+F focuses search within the panel.
- **Inspector** search navigates existing property controls and opens their
  sections. Enter/Down advances; Up goes back. **Sections** expands or collapses
  all groups. Section headers support keyboard focus and Space/Enter toggling.
- **Ctrl+Space** opens the existing Asset Browser and focuses search. In a drawer,
  repeating it from the focused browser collapses the drawer. New workspaces
  start with content in the bottom ADS auto-hide bar; existing saved layouts
  retain their placement. The normal ADS pin button docks it permanently.
  Breadcrumbs, Back/Forward (Alt+Left/Right), Parent, list/thumbnail controls and
  thumbnail size share the existing asset model and filters.
- **Workspaces** saves, restores and removes named layouts in the current
  project's preferences. Names support Unicode and separators. Layouts include
  the shell and existing document layout groups; they do not close documents or
  open saved documents. Safe mode prevents named layout restoration. Failed
  restores recover the previous layout.
- Log controls expose severity counts, quick filters and **Follow**. Manual
  scrolling pauses following, and new messages preserve selected rows. Search
  and copy continue to use the original log model and callbacks.

The organization takes cues from Epic's [Editor interface](https://dev.epicgames.com/documentation/en-us/unreal-engine/unreal-editor-interface)
and [viewport toolbar](https://dev.epicgames.com/documentation/en-us/unreal-engine/viewport-toolbar).
XII retains its own palette, document model, action maps and ADS workspaces.

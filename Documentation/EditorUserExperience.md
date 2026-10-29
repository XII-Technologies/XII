# XII Editor workspace

The editor uses Qt Widgets, GuiFoundation action maps and Qt Advanced Docking
System (ADS). The redesign evolves those shared controls so document plugins
inherit the same behavior. Document formats, action identifiers, shortcuts,
undo/redo and renderer integration remain owned by their existing modules.

## Design

Use graphite surfaces, readable neutral text and a restrained petrol accent.
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

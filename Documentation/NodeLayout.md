# Node layout and alignment

The shared node editor provides intentional formatting for render pipelines, visual shaders,
animation graphs, state machines and other `xiiQtNodeScene` consumers. Opening, saving or
creating nodes never automatically rearranges a graph.

Right-click a node or connection to find **Format Graph**, **Format Selection**, **Align Nodes**
and **Distribute Nodes**. The canvas node-search menu also includes these commands below its
search widget. Space and connection-drag node creation retain their existing search workflow.
Specialized actions, including Expand Subgraph and Set as Initial State, remain available.

| Operation | Shortcut | Scope |
| --- | --- | --- |
| Format Graph | Ctrl+Shift+L | Entire graph; frame the result in the node view |
| Format Selection | Ctrl+L | Selected nodes only; requires at least two |
| Align Left / Center / Right | Context menu | Visible horizontal bounds of selected nodes |
| Align Top / Middle / Bottom | Context menu | Visible vertical bounds of selected nodes |
| Distribute Horizontally / Vertically | Context menu | At least three nodes; equal visible gaps |

Shortcuts apply inside the node view. Alignment requires two nodes. Distribution preserves
the outer bounds when space permits; overlapping or tightly packed selections expand enough
to provide a minimum gap. Formatting a selection uses nearby external connections as placement
anchors and searches for the nearest clear placement while leaving other nodes fixed. Distant
anchors are ignored so repeatedly formatting does not gradually pull a group across the canvas.

Each operation validates movement through `CanMoveNode`, then uses `xiiMoveNodeCommand` in one
transaction. Failure cancels the transaction. Undo/redo and ordinary document metadata saving
preserve positions. An unchanged layout creates no history entry.

## Implementation and validation

`xiiNodeLayout` separates placement computation from the graphics scene and document mutation.
Nodes are ordered by persistent UUID; connections use stable endpoint/pin ordering. Strongly
connected components provide cycle-safe layering, followed by longest-path ranks and backward
rank tightening to keep short branches near consumers. Feedback members share a column and
attract one another during pin-aware placement. Cyclic edges retain their authored direction.

Alternating barycentric sweeps reduce crossings and align connected pins. Interval compaction
resolves overlaps using rendered sizes, and shelf packing separates disconnected components.
Spacing follows the existing visual scale: 120 units between columns, 60 vertically and 120
between components. The algorithm is a deterministic layout heuristic, not a global crossing
optimum. Existing wire drawing remains in charge of connection paths; this feature does not
introduce serialized routing points or guarantee that every long wire avoids every node.

Run `EditorFrameworkTest -noGui -run -close -noSave -all -filter NodeLayout -sdkRoot <sdk>
-outputDir <output>` after building EditorFrameworkTest. Coverage includes variable sizes,
branches/merges, crossing reduction, pin alignment, disconnected components, feedback, a
240-node dense graph, deterministic repetition, selection boundaries, all manual operations,
movement rejection, shortcut actions, undo/redo, position persistence and the authored Scene
pipeline. The tests write `NodeLayout-Before.png`, `NodeLayout-After.png` and
`NodeLayout-Scene.png` using actual Qt node items for visual inspection. The existing
EditorDocuments tests cover pin selection, connection dragging, dynamic pins, deletion and
copy/paste workflows.

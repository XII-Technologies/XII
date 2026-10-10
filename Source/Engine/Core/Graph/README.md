# Core graph

`xiiGraph` owns directed multigraph topology. Parallel edges, self-loops, cycles
and disconnected vertices are supported. Domain objects, pins, types and edge
categories remain consumer data. The facility has no rendering, editor or Qt
dependencies.

## Repository integration and representation

Core publicly depends on Foundation (and Texture). This facility uses Foundation
containers, generational IDs, allocators, delegates, events, streams, reflection
and scoped locks. Existing `xiiAbstractObjectGraph` represents reflected object
properties/assets; `xiiDGMLGraph` exports visualization descriptions; Utilities'
`xiiPathSearch` generates domain-specific search states. None provides this
mutable, domain-independent multigraph topology.

Two `xiiIdTable`s provide slot allocation and O(1) lookup. Dense handle arrays
provide O(V) and O(E) enumeration without scanning vacant slots. Each edge belongs
to an intrusive doubly linked outgoing list and incoming list; vertices store
list heads and degrees. This avoids per-vertex adjacency allocations and allows
O(1) unlinking. Indexed adjacency trades contiguous neighbour scanning for
predictable mutation cost. No graph elements require polymorphic engine objects.

The recursive CMake source discovery integrates these files into Core/CoreTest.
Reconfigure CMake after adding files; no extra dependencies are required.

## Public API and ownership

```cpp
#include <Core/Graph/GraphAlgorithms.h>
#include <Core/Graph/GraphData.h>

xiiGraphWithData<MyPass*, MyConnectionMetadata> topology;
auto& graph = topology.m_Graph;
XII_SUCCEED_OR_RETURN(graph.Reserve(128, 256));
xiiGraphVertexHandle producer, consumer;
xiiGraphEdgeHandle connection;
XII_SUCCEED_OR_RETURN(graph.TryAddVertex(producer));
XII_SUCCEED_OR_RETURN(graph.TryAddVertex(consumer));
XII_SUCCEED_OR_RETURN(graph.TryAddEdge(producer, consumer, connection));
XII_SUCCEED_OR_RETURN(topology.m_Data.SetVertexData(producer, pProducerPass));

for (auto it = graph.GetOutgoingEdges(producer); it.IsValid(); ++it)
{
  const auto neighbour = graph.GetTarget(it.GetHandle());
  // Inspect the edge or neighbour's consumer data without adjacency copies.
}

xiiGraphAlgorithmWorkspace scratch;
xiiDynamicArray<xiiGraphVertexHandle> executionOrder;
XII_SUCCEED_OR_RETURN(xiiGraphAlgorithms::TopologicalSort(graph, executionOrder, &scratch));
```

Use the plain graph with external handle-keyed containers when the consumer
already owns its objects. Both handle types support Foundation hash tables and
ordered maps. `xiiGraphData<VertexData, EdgeData>` optionally attaches typed
property maps to an existing graph; `xiiGraphWithData` composes a graph and its
maps. Properties can hold values, raw pointers or engine smart pointers.
Raw pointers retain consumer ownership; smart pointers follow their own ownership
rules. Missing properties are permitted. Attached maps automatically destroy
removed properties and preserve them across compaction through remapping.
The graph must outlive attached maps. Property pointers are transient across
changes to their map. Independently maintained external maps require explicit
cleanup/remapping, or an event subscription.

`TryAddVertex`, `TryAddEdge`, `TryRemoveVertex`, `TryRemoveEdge`, `TryClear`,
`Reserve`, `SetLimits`, and `ReconnectEdge` report failures with `xiiResult`.
Convenience `Add*` forms return invalid handles; `Remove*` returns false;
`Clear` ignores failure. Use the Try forms where failures need to propagate.
Reconnection preserves the edge identity and changes both adjacency lists.
Invalid reconnection leaves the original connectivity unchanged.

A connection validator can reject incompatible pins, parallel edges or cycles
according to domain policy. Its existing-edge argument distinguishes insertion
from reconnection. Validators are read-only and must not mutate either input
graph. For topology replacement, the destination validator examines the complete
replacement graph with existing-edge handles. Setting a validator does not
retroactively validate existing edges.

Synchronous events cover additions, before/after removals, reconnection and
before/after topology replacement. Before-removal handles are valid;
after-removal handles are stale. Reentrant mutation of emitting graphs is
rejected. Reset events expose an optional remapping valid only during that
notification. Listener registration follows Foundation event rules.

## Identity, replacement and invalidation

Each 16-byte typed handle combines `xiiGenericId<32, 31>` and a 64-bit graph
identity. Identities are allocated lazily, atomically and never recycled within
the process. Exhaustion returns `XII_FAILURE`; there is no exception or fatal
termination path in the graph. The unused high local-ID bit avoids Foundation's
64-bit mask-shift limitation.

Handles survive unrelated mutation and buffer growth. Removing an element
increments its slot generation. At the maximum generation, a slot retires rather
than wrapping; it never appears in counts, enumeration, adjacency or algorithms.
`Clear` retains buffers and generation history. A graph recreated at the same
address rejects the previous graph's handles. Invalid, stale and foreign handles
are safely rejected; queries return zero, empty views/cursors or invalid handles.
`FindEdge` finds the newest surviving matching edge. Enumerate matching outgoing
edges to obtain all parallel connections.

`CopyFrom` explicitly copies topology and returns optional old-to-new handle
maps. `MoveFrom` transfers topology and source handles; the source becomes empty
and rejects them. Destination handles are invalidated. Limits and validation of
the destination apply; rejected replacements preserve both graphs and output
maps. The move constructor is available outside callbacks. Fallible copying uses
`CopyFrom` rather than a C++ copy constructor. Listeners, validators and attached
payload maps belong to graph objects and are not transferred with topology.
Allocations transferred by move retain their original allocator, which must
remain alive. Future destination allocations use the transferred buffers'
allocator until storage is rebuilt.

`Compact` rebuilds live topology and returns a complete handle remapping. All
old handles become stale; attached typed property maps follow the new handles.
External maps must apply the returned mapping. This explicitly trades identity
replacement for reclaiming high-water storage and retired slots. Compacting an
empty graph releases all topology storage. Copy/compaction costs O(V + E) and
requires temporary storage for old/new topology and maps.

All views, dense indices and adjacency cursors must be reacquired after any
mutation, including `Reserve`. No record references are exposed. Dense indices
are transient, not stable keys. Dense enumeration uses swap removal; adjacency
is newest first. A fixed sequence of mutations produces deterministic orders,
although new graph lifetimes have different handle identities.

Plain graphs allow concurrent const queries when allocator/output/callback
requirements are met. Mutation requires exclusive synchronization. The optional
`xiiSynchronizedGraph::Read()`/`Write()` returns Foundation scoped locked access;
both use an exclusive mutex. Keep views and cursors inside that scope. Handles
may escape, but must be revalidated when next accessed. Graph destruction,
lock ordering and attached payload synchronization remain owner responsibilities.

## Algorithms and filtered topology

`xiiGraphAlgorithms` provides iterative algorithms over public connectivity.
No persistent traversal state or duplicate topology is stored. Optional reusable
workspaces and outputs retain allocations. A workspace must not be shared by
overlapping calls. Callbacks must not mutate topology; edge filters/weights must
remain consistent throughout a call. Empty filters select all edges. Filters can
select enabled edges, typed connections, data flow or execution flow without
constructing another graph. Filtered components still include isolated vertices.

| Algorithm | Time | Auxiliary workspace |
| --- | --- | --- |
| BFS / DFS / reachability | O(V + E) | O(V) |
| Topological sort / cycle detection | O(V + E) | O(V) |
| Strong / weak components | O(V + E) | O(V) |
| Unweighted shortest path (BFS) | O(V + E) | O(V) |
| Nonnegative lowest-cost path (indexed-heap Dijkstra) | O((V + E) log V) | O(V) |
| Maximum flow (Edmonds-Karp) | O(V + E + V E squared) | O(V + E) |
| Minimum spanning forest (Kruskal, direction ignored) | O(V + E log E) | O(V + E) |

Bounds include state initialization and assume O(1) callbacks. No traversal
uses recursive C++ calls. Single-source traversal initializes O(V) state and
visits only reachable successors; visitors return false to stop successfully.
Invalid starts or empty visitors fail. A valid vertex reaches itself through a
zero-length path. BFS visits by nondecreasing distance; DFS visits in preorder.

Kahn sorting counts parallel edges independently, includes disconnected
components and returns failure with empty output on cycles, including loops.
Ready vertices use FIFO order seeded by dense enumeration. Cycle detection shares
the sorting kernel. Iterative Kosaraju shares the DFS walker, then uses existing
incoming connectivity, without constructing a reversed graph. Weak components
traverse both directions. Component output is flat vertices plus offsets.

Paths return vertices, edges and cost. Failure clears output. Dijkstra requires
finite nonnegative examined weights; it does not validate unreachable/unexamined
edges. Maximum flow requires distinct source/target and finite nonnegative
capacities; parallel/opposing edges are independent, with reverse residual arcs
provided by incoming edges. Output lists every graph edge and its flow; excluded
edges have zero flow. Kruskal accepts finite weights, including negative weights;
empty weights mean unit weights. Loops are ignored, parallel edges compete and
isolated vertices count as components. Weight/flow arithmetic uses double and
inherits floating-point precision; nonfinite accumulated results fail explicitly.

## Allocation and operation complexity

| Operation | Time |
| --- | --- |
| Vertex / edge insertion | Amortized O(1), O(capacity) on growth |
| Validity / endpoints / degree / dense index | O(1) |
| Edge removal / reconnection | O(1), excluding domain callbacks |
| Vertex removal | O(in-degree + out-degree) |
| Enumeration | O(V) / O(E) |
| Incoming / outgoing adjacency | O(incident edges), no allocations |
| Find one connection | O(out-degree) |
| Clear | O(V + E), retains storage |
| Copy / compact / snapshot capture or apply | Expected O(V + E) |
| Move | O(1) transfer plus destination destruction and O(E) validation if installed |
| Destruction | O(vertex slot capacity + edge slot capacity) |

Callbacks, optional property destruction and allocator work add their own costs.
An empty graph allocates nothing. First use allocates one storage descriptor;
two ID-table buffers and two dense arrays grow through the chosen allocator.
There are no per-element topology allocations. `Reserve` avoids subsequent growth
within reserved capacities. On an ABI with 8-byte alignment of 64-bit integers,
vertex/edge records are 32/56 bytes; ID-table keys and dense handles add 24 bytes
per live element, approximately 56/80 bytes before unused capacity/container
metadata. Retired slots retain their records but have no dense handles. Events
allocate only when subscribed; typed maps and algorithm buffers are optional.
Allocator lifetimes must cover their graph, maps, workspaces and output buffers.
Foundation delegate captures larger than inline storage may allocate.

`SetLimits` bounds occupied slots (including retired slots). The maximum
`0xFFFFFFC0` respects Foundation's 32-bit capacity/alignment arithmetic; available
memory normally imposes a much smaller bound. Identity exhaustion, declared
capacity limits, invalid operations, domain rejection, stream failures and
storage-descriptor allocation failure return `xiiResult`. Foundation container
buffer growth follows its existing allocation-success contract. General
recoverable buffer OOM would require fallible allocation support in Foundation
containers; it is not implemented by silently replacing engine containers here.

## Serialization and reflection

`xiiGraphSnapshot` is an optional reflected, versioned, endian-independent
portable topology representation. It stores vertex count and each edge's endpoint
indices, preserving isolated vertices, loops and parallel edges. Runtime handle
bits, allocator details and algorithm scratch are excluded. `Capture` follows
dense enumeration; `Apply` returns index-to-runtime handles for domain payloads.
Apply and Load are transactional; malformed data, unsupported versions,
truncation, limits or destination validation failures leave outputs unchanged.
Load is bounded by configurable counts (default one million vertices and four
million edges) and reads validated endpoint pairs incrementally.

Graph reflection exposes read-only counts; snapshot reflection exposes topology
properties. Consumers can use snapshots within their domain asset format, or
serialize their own stable asset IDs and remap on load. Arbitrary typed payload
serialization requires a consumer codec; the graph does not invent a universal
asset format or reflect every possible payload type.

## File organization and future consumers

Public files separate graph/handles, typed data, events, remapping, snapshots,
synchronization, algorithm callback types, algorithm interfaces and result types.
Implementation files separate lifetime/storage, mutation, connectivity, identity,
serialization, reflection, traversal, topological sorting, components, paths,
flow and spanning forests. Tests separate structure, facilities, algorithms,
optimization and randomized stress.

Render pipelines can associate existing pass pointers with vertices and
resource/pin metadata with edges, validate compatible connections, filter edge
categories, sort scheduling dependencies and use SCCs for diagnostics. Material,
script, animation, behavior and procedural systems can use the same topology
with their own metadata and validation. Ownership of passes, resource barriers,
execution semantics, pin schemas and editors remain in those consumers. No
additional speculative node graph abstraction is required.

## Verification

The five Graph test groups cover structure, stale/foreign handles, actual slot
reuse, identity exhaustion, capacity limits, growth, loops, parallel edges,
reconnection, high-degree removal, clearing and graph recreation at the same
address. Facilities tests cover validation, events/reentrancy, copy/move,
compaction and owned payload preservation, refused metadata allocation, stream
failures/truncation/bounds, reflection and four concurrent scoped writers.

Basic algorithm tests include exact traversal order, early stopping, disconnected
components, DAGs, cycles and a 30,000-vertex chain/cycle. Stress tests compare 8,000
seeded mutation steps (including reconnection) against independent edge records,
checking all structural invariants after every step. Eighty generated graphs
compare connectivity/components against independent transitive closure.
Fifty generated multigraphs compare Dijkstra against Floyd-Warshall, maximum flow
against exhaustive minimum cuts and spanning forests against independent Prim.
Tests also validate flow capacities/conservation, filtered edge categories,
invalid weights and allocation-free repeated algorithm execution after warm-up.

Verified with the VS2026 x64 Debug preset: Core and CoreTest build successfully;
all 20 Core subtests pass, including all five Graph groups. The graph source and
tests contain no `<exception>`, `throw` or `std::terminate` use.

```powershell
cmake --preset vs2026x64
cmake --build Workspace/vs2026x64 --config Debug --target CoreTest -- /m:1
Output/Bin/WinVs2026Debug64/CoreTest.exe -nosave -nogui -all -filter Graph
Output/Bin/WinVs2026Debug64/CoreTest.exe -nosave -nogui -all
```

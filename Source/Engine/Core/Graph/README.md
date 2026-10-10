# Core graph

`xiiGraph` owns the topology of a directed multigraph. Vertices are lightweight
structural records, not engine objects. Parallel edges, self-loops, cycles and
disconnected vertices are supported. Domain validation belongs to the owner.

## Repository integration and design decisions

Core already publicly depends on Foundation (and Texture). These files use only
Core's export macro and Foundation containers, IDs, allocators, delegates and
atomic integers. They introduce no renderer, editor or Qt dependencies.

The existing facilities solve different problems:

- `xiiAbstractObjectGraph` stores reflected object properties and asset identities
  for serialization and patching.
- `xiiDGMLGraph` builds visualization/export descriptions.
- Utilities' `xiiPathSearch` provides domain-generated pathfinding states.
- Render graphs and editor/visual-script graphs carry consumer-specific semantics.
- World uses stable handles and composition; resource handles additionally manage
  ownership. The topology does not need reference counting or polymorphic nodes.

`xiiIdTable` supplies slot allocation, generation maintenance and constant-time
lookup. Its iterator scans vacant slots, so two dense handle arrays separately
enumerate live vertices and edges. A dense array alone would need an additional
identity mapping; adjacency arrays per vertex would allocate for individual
vertices and require scans or back-index bookkeeping for removal. Instead each
edge participates in two intrusive doubly linked lists, with heads in its source
and target vertices. This costs four local IDs per edge, permits O(1) unlinking
from both lists, and needs no per-vertex allocation. Adjacency trades contiguous
scanning for indexed list traversal and predictable mutation costs.

The build system discovers Core and CoreTest sources recursively. Reconfigure
CMake after adding these files; no target dependencies or source lists change.

## Usage and payload ownership

```cpp
#include <Core/Graph/GraphAlgorithms.h>
#include <Foundation/Containers/HashTable.h>

xiiGraph graph;
const auto producer = graph.AddVertex();
const auto consumer = graph.AddVertex();
const auto connection = graph.AddEdge(producer, consumer);

// Consumer-owned objects retain their existing ownership and lifetime rules.
xiiHashTable<xiiGraphVertexHandle, MyPass*> passes;
passes.Insert(producer, pProducerPass);
passes.Insert(consumer, pConsumerPass);

for (auto it = graph.GetOutgoingEdges(producer); it.IsValid(); ++it)
{
  const auto neighbour = graph.GetTarget(it.GetHandle());
  // Inspect the connection or neighbour's consumer-owned payload.
}

xiiGraphAlgorithmWorkspace scratch;
xiiDynamicArray<xiiGraphVertexHandle> executionOrder;
if (xiiGraphAlgorithms::TopologicalSort(graph, executionOrder, &scratch).Succeeded())
{
  // Sources precede targets for every connection.
}
```

Both handle types support hash tables and ordered maps. External payload maps
must be maintained by their owner: removing a vertex removes incident topology,
but does not erase entries from external maps or destroy referenced objects.
An owner that needs incident edge payload cleanup should inspect incoming and
outgoing edges before removal (a self-loop occurs in both lists).

For transient dense arrays, `GetVertexIndex()` indexes `GetVertices()`. Rebuild
such associations after mutation; that index is not a stable identifier.

Pin IDs, connection categories, types, and domain metadata can live in edge
payloads. Domain validation can reject connections before `AddEdge()`. No
`xiiNodeGraph` is needed until concrete consumers establish a shared contract.

## Identity, invalid operations and ordering

Each 16-byte handle combines a Foundation `xiiGenericId<32, 31>` with a 64-bit
graph lifetime identity. Vertex and edge handles are distinct types. Graph
identities are allocated atomically once per graph, never recycled during the
process lifetime. Even a graph constructed at a previous graph's address rejects
the old handles. Exhausting the graph identity space terminates the process.
The unused high local-ID bit avoids Foundation's 64-bit mask-shift limitation.

Element storage can relocate; handles survive unrelated mutations and growth.
Removal increments the slot generation through `xiiIdTable`. On removing an
element at generation `0x7FFFFFFF`, its slot remains occupied but inactive rather
than wrapping. Retired records never appear in enumeration, adjacency, counts
or algorithms, and are rejected by `IsValid()`. Retirement happens only after
over two billion lifetimes of a slot. `Clear()` removes live elements individually
rather than resetting ID tables, preserving generations and retired slots.
Graphs cannot be copied or moved; cloning would require explicit handle remapping.
Handle internals are implementation details, not portable or persistent IDs.

Invalid, stale and foreign handles are safely rejected. `AddEdge()` returns an
invalid handle, removals return `false`, degree queries return zero, adjacency
queries return an empty cursor, and endpoint queries return invalid handles.
`GetVertexIndex()` returns `xiiInvalidIndex`. `FindEdge()` finds the newest surviving
edge between valid endpoints, or returns an invalid handle. Parallel connections
can all be found by filtering the source's outgoing edges by target.

All views, indices and adjacency cursors must be reacquired after **any** mutation,
including `Reserve()`. No record references or pointers are exposed. Dense lists
use swap removal, so their order is unspecified after removal. Adjacency is newest
first. The same sequence of mutations yields the same enumeration and algorithm
orders; handles themselves differ between graph lifetimes.

Concurrent const operations are allowed when allocators, scratch and output
containers are suitable. Mutation requires external exclusive synchronization.
Do not mutate during an algorithm or visitor. A workspace must not be shared by
overlapping or reentrant algorithm calls.

## Complexity and allocation

Here V and E count **live** elements.

| Operation | Time |
| --- | --- |
| Insert vertex / edge | Amortized O(1); O(capacity) when ID storage grows |
| Validate / lookup / endpoints / degree | O(1) |
| Remove edge | O(1) |
| Remove vertex | O(in-degree + out-degree), counting a loop only once for removal |
| Enumerate vertices / edges | O(V) / O(E) |
| Incoming / outgoing adjacency | O(in-degree) / O(out-degree), no allocations |
| Find one connection | O(out-degree) |
| Clear | O(V + E), retains buffers and generations |
| Destruction | O(vertex slot capacity + edge slot capacity), including retired slots |
| BFS / DFS / reachability | O(V + E), including O(V) initialization |
| Topological sort / cycle detection | O(V + E) |
| Strong / weak components | O(V + E) |

The graph has four growing contiguous buffers: two ID tables and two dense handle
arrays, all using the supplied allocator. Empty construction allocates nothing.
`Reserve()` preallocates for construction; allocator lifetime must exceed graph
lifetime. No nodes or edges allocate individually. Capacity follows Foundation's
growth and alignment policies, and memory follows the high-water mark, not only
the current live count. On an ABI with 8-byte alignment for 64-bit integers, the
vertex record is 32 bytes and the edge record 56 bytes; adding an 8-byte ID-table
key and a 16-byte dense handle gives approximately 56 bytes per vertex and 80
bytes per edge before unused capacity and container overhead. Retired slots keep
their ID-table storage but have no dense handles.

All algorithms use O(V) scratch, with no recursive C++ calls. The optional
workspace and output containers retain capacity for repeated execution. Single
source traversal visits only reachable successors but initializes O(V) state.
Visitors return `false` to stop successfully. Invalid starts or empty visitors
fail. Reachability includes a zero-length path from a valid vertex to itself.

Topological sorting uses Kahn's algorithm and handles parallel edge indegrees
individually. Empty graphs succeed; cycles fail with empty output. Ready vertices
use FIFO order seeded by the current dense vertex order. Cycle detection shares
the same kernel and needs no result array. Strong components use Kosaraju with
the same explicit DFS walker as depth-first traversal, followed by traversal of
existing incoming edges; no reversed graph is constructed. Weak components visit
both incoming and outgoing edges. Component output is one flat vertex array and
an offset array, with no per-component allocations.

## Serialization, events and limitations

There is no generic asset format, reflection registration, automatic mutation
event stream, payload storage, synchronization, or exception/OOM recovery. These
are consumer policies. Container capacity is limited by Foundation's 32-bit
capacity management and available memory. Capacity is not compacted, because
discarding vacant-slot generation history would weaken stale-handle rejection.
Shortest paths, flow algorithms and spanning trees are intentionally outside
this initial facility.

A consumer should serialize its own stable asset IDs and edge endpoint IDs,
then remap them to new runtime handles on load. Writing each edge separately
preserves parallel edges and self-loops. Algorithm scratch and runtime handle
bits must not be serialized. Foundation's existing reflection/stream/object-graph
facilities remain available for the consumer's domain format and events.

A future render-pipeline owner could map vertex handles to existing pass objects
and edge handles to resource/pin metadata, validate compatible connections, then
use topological sorting for scheduling and SCCs for diagnostics. Resource lifetime,
barriers, pass ownership and renderer-specific validation remain in that owner.
No renderer adoption or editor changes are part of this implementation.

## Verification

`CoreTest/Graph` covers structural operations, invalid and foreign handles,
actual slot reuse and generation changes, growth, loops, parallel edges, high
degree removal, clearing, graph recreation at the same address, traversal order,
early stopping, DAG ordering, cycles, disconnected components, and a
30,000-vertex chain/cycle without recursion.

Stress coverage compares 8,000 seeded mutation steps with independent edge
records, checking validity, endpoints, degrees, exact adjacency membership and
enumeration after every step. Eighty generated graphs (empty connectivity,
sparse, dense and complete; both arbitrary directed graphs and DAGs) compare
reachability, cycle detection, SCC and weak-component equivalence against an
independent Floyd-Warshall reference. BFS additionally checks distance ordering.
Explicitly tracked allocators verify reserved construction, allocation-free
adjacency, repeated algorithm reuse after warm-up, and balanced destruction.

```powershell
cmake --preset vs2026x64
cmake --build Workspace/vs2026x64 --config Debug --target CoreTest -- /m:1
Output/Bin/WinVs2026Debug64/CoreTest.exe -nosave -nogui -all -filter Graph
Output/Bin/WinVs2026Debug64/CoreTest.exe -nosave -nogui -all
```

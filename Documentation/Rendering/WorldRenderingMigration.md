# World-authoritative rendering migration

## Migration map

| Legacy responsibility | Final owner / replacement |
| --- | --- |
| Render objects, parents, local/global transforms, activation and bounds | Existing `xiiWorld`, `xiiGameObject`, render components and world spatial system. Delete the renderer hierarchy and its mutation API. |
| Mesh/material assignments, sections, forced LOD and animation pose | Existing mesh, LOD and skinned components and resource handles. Extraction packets carry immutable frame inputs. |
| Compact instance records, GPU identity, previous transforms and upload intervals | Focused GPU instance buffer owned by the existing render world module. Only extraction updates it; no scene manipulation API or independent hierarchy. |
| Geometry and material GPU addresses | Existing device residency services, acquired from resource handles. No per-scene asset index tables. |
| Camera culling, LOD hysteresis, exposure and image history | Views / graph occurrences; visibility consumes world instance buffers and a per-view candidate stream. |
| Picking subsets | Candidate indices into the same world extraction; validate authoritative object/component handles on readback. Remove picking-owned scene copies. |
| BLAS allocations | Shared device geometry resource cache. |
| TLAS transforms and membership | Derived world extraction and world-scoped rotating TLAS resources. Remove the global ray-tracing instance registry. |
| CPU render-side spatial hierarchy | Delete; use the world spatial system and existing GPU culling. |

## Implementation sequence

1. Introduce extraction-only GPU packing and verify its lifecycle/temporal contracts.
2. Migrate visibility, shadows, views, picking, editor tests and the sample to world data.
3. Delete database, hierarchy, object types, registrations and obsolete tests.
4. Consolidate ray tracing into shared BLAS residency plus world-owned TLAS frames.
5. Share extraction/uploads across views and verify demand, synchronization and incremental performance.

Each phase must build before committing. The final verification record will include
repository-wide deletion searches, affected builds, CPU/GPU/editor tests and sample
execution. No database objects are serialized as assets: old reflected descriptor
types are runtime-only; pipeline asset/resource versions remain unchanged unless
an authored reflected property requires an explicit conversion.

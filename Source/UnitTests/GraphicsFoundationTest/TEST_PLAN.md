# GraphicsFoundation Test Plan

## Goals

- Exercise every public behavior owned by `GraphicsFoundation`.
- Run backend-neutral contracts once and device contracts against every graphics implementation compiled into the test executable.
- When `-renderer <name>` is supplied, run device contracts only against that implementation.
- Keep tests deterministic, headless, validation-clean, and independent of frame timing or adapter performance.
- Gate optional feature tests on the device capability table and report the skipped capability rather than treating unsupported hardware as a failure.

## Backend Matrix

| Area | Vulkan | D3D12 | Backend independent |
| --- | --- | --- | --- |
| Factory, device, queue, resource/state creation | Yes | Yes | No |
| Command recording, mapping, copies, queries, fences | Yes | Yes | No |
| Swap chain/window presentation | Yes | Yes | No; requires a real window and is excluded by `-nogui` |
| Ray tracing and bindless resources | If supported | If supported | No |
| Descriptors, hashes, format/mip math, serialization | N/A | N/A | Yes |

## Feature Coverage

### Declarations and utilities

- `GraphicsTypes`: enum invariants, depth/stencil/sRGB/typeless classification, linear/sRGB conversion, primitive vertex/index counts, filter classification, and range reset/count semantics.
- `Descriptors`: default values, equality, hashing, and mutation sensitivity for device, resource, pipeline, draw/dispatch, barrier, and copy descriptors.
- `DescriptorHash`: equal descriptors produce equal hashes; every semantically relevant field changes equality and, where practical, the hash.
- `GraphicsUtilities`: swap-chain usage conversion, valid pipeline resource flags, and the default sampler contract.
- `TextureUtilities`: format metadata, component/block/plane properties, mip dimensions/count/properties, row/slice pitches, staging offsets and total size, copy layout, default view formats/descriptors, zero initialization, and row/depth-stride copies.
- `DeviceUtilities`: vendor mapping and descriptor construction, followed by device-backed vertex/index/constant/staging buffer creation and mapped updates.

### Device and command submission

- Factory selection, implementation metadata, adapter/device properties, feature negotiation, queue enumeration, and stable debug names.
- Command list lifecycle: begin/end, reset/reuse, debug groups, render-pass state, resource barriers, buffer/texture update and copy operations, mapping, draw/dispatch validation, and queue submission.
- Queue and fence behavior: monotonic fence values, wait/signal, idle waits, and completed-value observation.
- Swap-chain descriptor and resize/fullscreen contracts when GUI execution is enabled.

### Resources

- Buffers: immutable/default/dynamic/staging usage, structured/raw/typed descriptors, initial data, default views, explicit SRV/UAV views, mapped writes, copies, and bounds-sensitive metadata.
- Textures: 1D/2D/3D/cube/array descriptors, mip chains, initial data, default/explicit RTV/DSV/SRV/UAV views, subresource copies, updates, mapping/readback, and multisample capability gates.
- Samplers: default and non-default address/filter/comparison descriptions and retained descriptor state.
- Render passes and framebuffers: attachment/subpass/dependency descriptions, compatibility, retained references, and descriptor hashing.
- Queries: timestamp, occlusion, binary occlusion, pipeline statistics, and duration paths gated by advertised features.
- Bottom- and top-level acceleration structures: descriptors, geometry/instance metadata, scratch/build/update flows, and retained object state when ray tracing is supported.
- Bindless resource/table objects: allocation, updates, invalidation, and reuse when bindless resources are supported.

### Shader and pipeline state

- Shader bytecode: construction, ownership, equality/hash, serialization, deserialization, resource reflection, and vertex input metadata.
- Input layouts: element descriptors, stride/offset semantics, retained shader reference, and device object creation.
- Shaders: stage descriptors, stage combinations, retained bytecode, resource metadata, creation failure for invalid combinations, and debug names.
- Blend, rasterizer, and depth/stencil states: default contracts, representative non-default fields, equality/hash sensitivity, and device object creation.
- Pipeline resource signatures: resources, immutable samplers, binding indices, compatibility, and static variable lookup.
- Graphics/compute/ray-tracing/tile pipeline descriptors and objects, with optional stages gated by capabilities.

### Shader compiler

- Text sectionizer and parser: sections, comments, line mapping, permutations, resource declarations, render state, malformed input, and diagnostics.
- Permutation generator: Cartesian products, fixed variables, exclusions, deterministic ordering, and empty inputs.
- Stage/permutation binaries: versioned round trips, dependency data, hashes, corruption/version rejection, and deterministic serialization.
- Compiler/manager integration: implementation selection, cache keys, cache reuse/invalidation, and compilation diagnostics using small test shaders.

### Tools

- `DynamicBuffer`: element allocation/deallocation, growth, dirty-range upload, version changes, and object reuse.
- `MapHelper`: typed mapping, move/lifetime behavior, requested map flags, flush, and automatic unmap.
- `VertexBufferPool`: allocation, reuse, growth, upload, capacity, and deallocation invariants.
- `ImageCapture` and `TextureReadback`: capture scheduling, pending counts, row-pitch normalization, format metadata, completion, and deterministic pixel round trips.
- `ScopedDebugGroup`: balanced begin/end behavior on all exits.
- Empty compatibility headers (`DurationQueryHelper`, `DynamicTextureArray`, `DynamicTextureAtlas`, `ScopedQueryHelper`, `ShaderMacroHelper`, `StreamingBuffer`, and `TextureUploader`) receive include/compile-smoke coverage until they expose behavior.

## Execution and Quality Gates

1. Build `GraphicsFoundationTest` in Debug after each focused test family.
2. Run backend-independent tests first for fast feedback.
3. Run `GraphicsFoundationTest.exe -nosave -nogui -all`; without `-renderer`, both compiled backends must initialize and execute the same device contracts.
4. Run explicit `-renderer Vulkan` and `-renderer D3D12` selections to verify filtering.
5. Treat validation-layer errors, leaked references, nondeterministic waits, and backend-specific assertion differences as test failures.
6. Keep each commit buildable and limited to one coherent test family or harness change.

## Implementation Status

The feature matrix above is implemented by 35 subtests in nine test groups. Public headers that currently contain declarations but no runtime behavior are included by dedicated compile-smoke translation units so that future API additions cannot silently escape the test target.

The backend harness builds each renderer selected by the build configuration. With no `-renderer` argument it registers every compiled implementation; `-renderer <name>` restricts registration to the named implementation and rejects names that were not compiled into the executable. Every device-backed subtest iterates that registered list, while pure descriptor, serialization, parsing, hashing, and math contracts execute once.

Current host verification (2026-09-26):

- Visual Studio 2026 Debug x64, default backend selection: all 35 subtests passed with `-nosave -nogui -all`.
- Visual Studio 2026 Debug x64, explicit Vulkan selection: all 35 subtests passed with `-nosave -nogui -all -renderer Vulkan`.
- The active preset compiles Vulkan and its SPIR-V shader compiler. D3D12 remains covered by the conditional harness and the same backend-neutral fixtures, but cannot execute until that renderer is enabled in a build preset.
- The requested Visual Studio 2022 executable was not available on this host and its CMake generator is not installed; the equivalent installed Visual Studio 2026 configuration was used for build and execution.
- Headless execution validates swap-chain descriptors and ownership contracts but deliberately excludes native window presentation. Optional bindless, query, ray-tracing, and advanced pipeline paths execute only when the selected adapter advertises the corresponding feature.

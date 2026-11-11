# Subsystem Render Settings

A render configuration owns concrete reflected settings objects. Each object stores strongly typed values and a mask of explicitly authored property paths. Zero and false are valid authored values. Clearing a mask entry inherits the preceding scope. The mask uses reflected paths such as `Probes/ProbeSpacing`, and the values stay in their owning C++ fields.

## Selection and precedence

The view first selects a pipeline resource using the existing scene, camera, or explicit view resource selection rules. The platform profile supplies the resource reference when the caller has no explicit selection. The selected pipeline establishes the supported subsystem types and their initial values from its authored passes. The renderer then overlays the project/platform profile, the selected pipeline's explicit Render Defaults, the world settings component, the selected camera component, and the viewport. Later scopes win independently for each property. Selecting another resource starts resolution from that resource's passes and defaults.

The Render Settings dialog is available from viewport rendering controls and from the Render Configuration property in the platform profile, Render World Settings component, and Camera inspector. Pipeline assets expose Render Defaults through their settings object. Search filters nested property names and categories. Enable Override to edit a value and clear it to inherit. The viewport previews valid edits and displays the engine's last accepted values, their source, internal resolution, and configuration errors. Cancel restores the previous viewport settings. Inspector edits commit as a document transaction.

## Runtime publication

GraphicsCore discovers settings through RTTI. A subsystem contributes a concrete `xiiRenderSubsystemSettings` type with normal reflection metadata. `xiiRenderSettingsBindingAttribute` opts a property into an authored pass type and property path. A type can also declare availability for view controls. Pass contracts, pins, and internal implementation properties continue to live on the pipeline and passes.

A pass change is validated against its authored defaults before publication. An invalid request leaves the last accepted instance active and reports the cause. Changes to an existing pass keep its private runtime state. History and resource changes are identified by binding metadata, and structural GI, shadow, and reflection changes use the existing variant cache. DDGI update budget and hysteresis change the schedule in place. Changing dimensions, spacing, or relocation rebuilds the scheduler's probe volume. Graph passes consume typed fields and record the data needed by their GPU callbacks.

Device memory admission and shared residency use the active platform's Render Device Profile configuration. Vulkan samples its memory policy at BeginFrame. Capacity fields in the residency description require reinitialization when residency is active. Live mip, upload, and cache policies use the existing shared manager. Presentation and VSync remain window or swapchain policy.

## Migration inventory

| Previous source | Authoritative owner | Runtime consumer |
| --- | --- | --- |
| `Graphics.DynamicResolution.TargetFrameTimeMs`, `MinimumRenderScale`, `MaximumRenderScale` | Resolution settings per view | View PID and render extent calculation. Old CVars were transient. |
| Flat `RenderScale` override | Resolution settings per view | View render extent and dynamic resolution |
| Flat GI enum | Global Illumination technique override | Existing authored GI branch selection |
| Flat DDGI overrides and scheduler/gather defaults | DDGI settings | Probe schedule, atlas, trace range and gather |
| Flat shadow/SSR policies and pass quality defaults | Shadow and Reflection settings | Existing optional branches, cascade and spot rendering, screen space trace and temporal resolve |
| Flat `BloomStrength`, legacy root `DisplayOutput` parameters, display pass defaults | Post Processing and Output settings | Bloom, luminance histogram, exposure, tone mapping, color grading and presentation |
| AO, atmosphere, clustered lighting, geometry and temporal AA pass defaults | Respective reflected subsystem settings | The passes that author and consume those values |
| `Streaming.TextureUploadMiB`, `TextureMipBias`, `TextureMinimumMip`, `TextureCacheMiB`, `GeometryCacheMiB` | Platform residency streaming settings | Shared GPU residency manager |
| `Streaming.GpuBudgetMiB`, `GpuHeadroomMiB`, `GpuPressureHighPercent`, `GpuPressureLowPercent` | Platform device memory settings | Vulkan allocation admission and pressure tracking |
| `Streaming.DumpGpuAllocations`, `Streaming.TextureLoadDelay`, debug and profiling CVars | Existing diagnostics and fault injection | Allocation dumps, resource loader, and debug tools |

Existing serialized render configurations and object graphs migrate legacy fields into explicit property overrides. Device streaming uses the active platform profile and subsystem defaults. Old CVar files and standalone render-scale preferences are retired.

The pipeline inventory lists the full prior audit and classifies low-level controls. [Rendering settings inventory](RenderSettingsInventory.md)

XII currently exposes the algorithms present in the selected pipeline. The spatial upscaler, volumetric composition inputs, and other low-level graph contracts retain their authored configuration. There is no DLSS, FSR, ray-traced reflection, or volumetric fog generation control. HDR output modes require an appropriate output target and presentation configuration.

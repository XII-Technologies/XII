# Dear ImGui integration

`xiiImguiSubsystem` replaces the deleted singleton. It owns generation-checked contexts through XII startup/shutdown. CPU contexts can be used after core startup without a graphics device. High-level shutdown destroys every context; core shutdown releases the registry. No application, world, view, input manager, or extraction event implicitly creates or advances a UI frame.

## Constructing a frame

All ImGui calls run on XII's main thread, including calls made through this plugin. The installed library uses a thread-local current context, but its allocator and other library services are shared; independent contexts are not permission to build widgets concurrently. The integration preserves the library's allocator functions rather than replacing an allocator underneath unrelated ImGui users.

```cpp
xiiImguiContextHandle context;
XII_SUCCEED_OR_RETURN(xiiImguiSubsystem::CreateContext(context));
xiiSharedPtr<const xiiImguiFrame> snapshot;
{
  xiiImguiScopedContext active(context);
  xiiImguiFrameDescription frame;
  frame.m_vDisplaySize = {720, 480};
  frame.m_vFramebufferScale = {1, 1};
  frame.m_DeltaTime = delta;
  XII_SUCCEED_OR_RETURN(active.BeginFrame(frame));
  ImGui::Begin("Tool");
  ImGui::Button("Action");
  ImGui::End();
  XII_SUCCEED_OR_RETURN(active.EndFrame(snapshot));
}
```

Scopes restore the previous activation and prevent context destruction while borrowed. Nested/duplicate frames and invalid handles fail explicitly. An unfinished frame can be cancelled, or is ended when its context is destroyed. A zero display size and a frame without widgets are valid. A failed snapshot ends the frame and returns no snapshot; the context can be reused.

## Native pipeline composition

`xiiImguiRenderPass` is reflected under `Rendering/GUI`. It has a required 2D single-sample `Color` texture input/output and an optional typed `UI` data input (`xiiImguiFrameData`). Missing or empty UI forwards the color binding without allocating GPU resources or emitting a draw pass.

Expose the pass's `UI` input as an authored pipeline root interface. The host supplies its finalized snapshot during graph setup using the existing external-input mechanism:

```cpp
xiiRenderPipelineBinding ui;
ui.m_pFrameData = XII_DEFAULT_NEW(xiiImguiFrameData, graph.GetSetupSerial(), snapshot);
const xiiRenderPipelineInputBinding inputs[] = {
  {graph, pipeline->FindInterface("Color", true), color},
  {graph, pipeline->FindInterface("UI", true), ui}
};
XII_SUCCEED_OR_RETURN(instance.Build(view, graph, blackboard, {}, inputs));
```

Use the view's custom graph builder when supplying external inputs to an authored pipeline. Own its pipeline instance alongside the host view, synchronize selected resources at frame boundaries, and release the callback before its host dies. The sample shows a complete compiled pipeline and explicit graph execution. There is no global view/context map: bind the same snapshot to multiple related views, or bind independent contexts' snapshots to independent targets. Recreate the setup-local wrapper for each graph setup; old wrappers cannot be retagged.

Connect the overlay after SDR color grading and upscaling, before `xiiPresentPass`. Input must be display-linear SDR in RGBA16Float/RGBA32Float, or an sRGB render target. `ColorGradingPass` already creates a render-target-capable output. Packed ImGui vertex colors are decoded from sRGB in the vertex shader; sRGB user images are decoded by GAL sampling. Straight alpha is blended in linear light. The UI pass performs no tone mapping or presentation transfer function. Presentation applies the final output transform exactly once. HDR10, scRGB, HLG, display-encoded non-sRGB targets, multisampling, depth targets, texture arrays, and stereo are unsupported. A view explicitly configured for HDR is rejected.

The graph declares the color as read/write to retain scene contents and establish ordering. Textures are imported and read as shader resources. Vertex/index/uniform buffers come from the existing frame-safe graph resource pool, with geometric capacities for reuse. Preparation maps/uploads geometry and restores the declared GAL states before the graph-managed render pass begins. Execute uses XII shaders, pipeline caching, resource binding, viewport/scissor state, and indexed GAL draws with both index and vertex offsets. The pass uses the live view's viewport, or the whole explicit target for a viewless graph, and scales logical UI coordinates into it. It never owns or acquires a swapchain.

## Input and textures

Route events only from the UI's host. Subscribe to the source device's `GetInputSlotEvents()` and forward them through `xiiImguiInput::InputSlot`; subscribe to `GetTextInputEvents()` and forward each codepoint through `Text`. Remove both subscriptions before destroying either the host or context. `UpdateFromDevice` samples focus, absolute position, and current digital state before `BeginFrame`; wheel transitions come from events. This preserves clicks and key transitions occurring entirely between frames. Qt/editor hosts can use the explicit mouse/key/modifier/text/focus methods instead. Capture queries and input disabling belong to each scoped context and never suppress other application input.

Core's SDL dispatch routes events to the owning native window, continues pumping after the original window closes, updates device focus, and publishes complete UTF-8 text as Unicode scalar events. ImGui's XII configuration uses 32-bit character storage. Font coverage still depends on the fonts loaded by the host.

Register full 2D single-sample GAL shader-resource textures with `RegisterTexture`, then pass the returned integer token through `ImTextureRef`. Tokens are context-local in access and never reused during the registry lifetime. They are not native pointers. Unknown, unregistered, and foreign-context tokens fail snapshot creation. Unregistering a texture prevents new references; already finalized frames retain their resources. Color images should use sRGB formats; masks and already-linear data should use linear formats.

ImGui 1.92 texture create/update/destroy requests are handled at finalization. Font changes copy a complete immutable CPU texture version; the renderer lazily creates its GAL image. Old frames retain their old versions, so atlas changes never overwrite images still used by queued work. This favors safety over incremental atlas-upload bandwidth. Font pixels can be uploaded again for a recreated device; user textures must be registered again. Transparent recovery from device loss is not certified.

Snapshots deeply copy draw vertices, indices, clip rectangles, offsets, and strong texture references. They retain no ImDrawData, ImGui context, view, world, or producer pointers. Graph-owned data and GAL deferred destruction keep resources alive through submission. Arbitrary ImDrawCmd callbacks are rejected because their borrowed state cannot safely cross deferred execution; the reset-render-state marker is supported. No vendor-specific ImGui renderer is used.

Before unloading the plugin, stop producers, remove host input subscriptions, finish GPU frames, destroy graphs/pass instances, release snapshots, and then shut down contexts/services. Plugin reload requires this normal host teardown. Reusing live callbacks across DLL unload is unsupported.

## Demonstration and validation

Build with `XII_3RDPARTY_IMGUI_SUPPORT=ON` and a supported renderer:

```powershell
cmake --build Workspace/vs2026x64 --config Dev --target DearImgui DearImguiTest DearImguiGraphicsTest -- /m:1 /nologo /v:minimal
Output/Bin/WinVs2026Dev64/DearImgui.exe -renderer Vulkan
Output/Bin/WinVs2026Dev64/DearImguiTest.exe -nogui -all -close
Output/Bin/WinVs2026Dev64/DearImguiGraphicsTest.exe -nogui -all -close -renderer Vulkan
```

The sample derives directly from `xiiApplication`: two windows, two worlds, two independent contexts, and an offscreen view replaying the first snapshot with a viewport offset. It includes text entry, a slider, a button/counter, resize handling, independent window closure, and empty-frame submission. `-frames 12` provides a bounded smoke run.

The CPU suite covers ownership, stale handles, scoped activation, frame-state failures, minimized/empty frames, immutable snapshot lifetime, shared-view bindings, input isolation/disable, quick clicks, Unicode bursts, dynamic font versions, invalid textures, and rejected callbacks. The Vulkan suite reads pixels for alpha blending, clipping, viewport offsets, user textures, and font glyphs, using multiple draw lists and more than 65,536 vertices. It destroys the originating context before graph setup and recreates the destination view between outstanding frames. Its SDL test covers source-local key/text/focus/resize events and event pumping after primary-window destruction.

Windows/MSVC Dev builds and both suites passed on Vulkan. A 12-frame sample run passed; its rendered windows and independent button state were inspected interactively. D3D12, Linux/macOS, static linking, mixed-DPI monitors, device-loss recovery, plugin hot reload, and integration into an authored Editor project remain unverified. The bundle loads reflection into the Editor and engine processes; the sample is the exercised host integration.

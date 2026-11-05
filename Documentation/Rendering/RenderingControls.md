# Rendering controls

Viewport **View Options > Rendering** contains Show FPS, render scale, VSync,
the engine application frame limit, and a link to the Editor scene pipeline.
Existing view render modes continue to provide rendering diagnostics.

| Control | Owner and persistence | Behavior |
| --- | --- | --- |
| Show FPS | Individual view overlay preference; saved with document viewport preferences | Half-second wall-clock averages for FPS and milliseconds. Also displays internal scene dimensions. Off by default. |
| Render scale | Individual view rendering setting; saved with document viewport preferences | 10–100%, default 100%. The menu reports the last rendered internal resolution, including dynamic resolution. Camera projection and native output dimensions remain independent. |
| VSync | Viewport window/swapchain presentation preference; saved with its viewport layout preferences | Selects supported GAL VSync or Immediate presentation at the next presentation boundary. New/recreated swapchains use the same preference. |
| Frame limit | Application-wide `App.FrameLimit` saved CVar | 0 is unlimited; 1–1000 caps engine application frames. Invalid integers are clamped at the frame boundary. VSync or existing editor focused/unfocused redraw preferences can impose a lower rate. |
| Quality | Authored render pipeline and referenced shared pipeline assets | Edit existing reflected pass properties through the pipeline editor; changes affect all consumers of that asset. |

Scene scaling reuses `xiiView::SetRenderScale` and the renderer's existing
view-relative texture sizing, upscaling, target recreation and temporal-history
invalidation. UI and debug text retain output resolution. Integer pixel extents
round up independently. Existing dynamic-resolution CVars still control adaptive
feedback on top of each view's independently configured baseline; no new global
scale override is introduced.

The CVar panel console accepts explicit editor viewport targets:

```lua
View_List()
View_ShowFPS(7, true)
View_RenderScale(7, 0.75)
View_VSync(7, false)
App.FrameLimit = 60
App.FrameLimit = 0
```

`View_List()` logs current viewport IDs, document context and settings. IDs identify
live widgets and change when layouts are recreated. Issue each targeted view
command separately in the Editor console. These commands and the viewport menu
write the same preferences in the Editor process; normal redraw IPC applies scene
changes under the world's write guard. Non-finite/out-of-range console scales are
rejected. Application CVars continue to execute in the engine process.

Runtime views expose `SetShowFrameStats` and `SetRenderScale`; camera components
provide authored Show FPS and render-scale properties. The existing F5 shortcut
toggles the main view and its owning camera preference. The former application-wide
`App.ShowFrameStats` control has been removed. Runtime VSync continues to use the
existing `App.VSync` control and `RuntimeConfigs/Presentation.ddl` authority.

In a render pipeline asset window, the **Quality** menu selects existing shadow,
anti-aliasing and post-processing pass properties. **Shared pipeline assets** opens
referenced assets, including Scene from Editor and the shadow subgraphs from Scene.
For anti-aliasing changes, keep temporal-camera jitter and temporal resolve enabled
or disabled together. Shadow cascade/atlas settings, temporal blending and bloom,
exposure, tone-mapping and color-grading properties retain their existing renderer
contracts, undo/redo, asset persistence and resource reload path.

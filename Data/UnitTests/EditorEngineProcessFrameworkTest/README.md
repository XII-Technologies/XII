# Editor engine-process integration tests

The tests use generated textures rather than external fixtures. Run
`EditorEngineProcessFrameworkTest.exe -nogui -all -autoexit -renderer Vulkan`
and repeat with each compiled backend. They verify IPC camera enums, preview
light ownership, camera framing, and synchronized thumbnail pixels and scaling.
Embedded Qt window presentation and remote activation still require separate
integration verification.

## View scheduling

The view lifecycle test updates a paused world with independently enabled views.
A custom render-graph pass clears a real offscreen target, and texture readback
verifies its RGBA output. Disabled views execute no pass and retain their camera,
viewport, target, and selected pipeline resource.
Registered document tests verify redraw expiration, window/view teardown, and
closing shared-world documents before their owning document.

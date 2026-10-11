# Editor engine-process integration tests

The tests use generated textures rather than external fixtures. Run
`EditorEngineProcessFrameworkTest.exe -nogui -all -autoexit -renderer Vulkan`
and repeat with each compiled backend. They verify IPC camera enums, preview
light ownership, camera framing, and synchronized thumbnail pixels and scaling.
Embedded Qt window presentation and remote activation still require separate
integration verification.

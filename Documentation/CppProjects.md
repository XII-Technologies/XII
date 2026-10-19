# Project C++ plugins

Project code is compiled against the installed SDK without writing project binaries into that installation.

- `Source/<Name>Plugin`: authored plugin sources and bundle metadata.
- `Source/Build/<generator>`: generated CMake build files and intermediate compiler output.
- `Output/Bin/<configuration>`: plugin, symbols, and standalone project application.
- `Output/Lib/<configuration>`: import and static libraries.

Configurations use the SDK names (`Debug`, `Dev`, `Shipping`). The compiler and configuration must match the SDK. Ignore the project `Output` and `Source/Build` directories in version control.

The external-project CMake setup imports SDK targets first, preserving their installed locations, then selects project-local output. Existing generated source trees that call `xii_configure_external_project()` adopt this layout on regeneration; their authored sources are preserved. The Editor recognizes caches from the previous layout and regenerates them before building. Custom CMake overrides that redirect output elsewhere must be updated; a successful build must produce the plugin in the expected project directory.

Plugin configuration stores the project binary directory separately from the plugin name. The engine process and Player resolve it through `>project`; dependency identity and plugin events continue to use the name. An explicitly configured project directory never falls back to a same-name DLL in the Editor installation. Windows hot-reload copies also remain in the project output directory and are removed on unload or failed native loading.

Standalone project applications stage the SDK's exported shared libraries and runtime plugins into the project output folder. This copies dependencies out of the SDK, never project code into it. A development-only `xiiSdkRoot.txt` beside the application points to SDK data, including when the project is on another drive; it is excluded from export.

Project export gathers selected project binaries and package dependencies into the exported `Bin` directory. It writes a portable plugin configuration without project output paths or hot-reload copies, and excludes the development `Output` tree from project data. Missing project binaries fail export instead of silently packaging an older SDK copy.

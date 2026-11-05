# Player and managed play sessions

## Editor workflow

Use **Export and Run** (`Ctrl+R`) to export the current scene and launch one to sixteen instances. The existing C++ build, asset transformation and scene export steps still run first. A failed process creation stops any siblings already created for that launch. A runtime failure in one instance is reported independently and does not stop another project or session.

The dialog supports named configurations, project/application overrides, a startup scene override, roles, connection parameters, common argument tokens and one additional command line per instance. **Save Configuration** stores named configurations in `RuntimeConfigs/Player.ddl`. **Use Current Scene as Main** edits the existing project manifest's `DefaultScene`; save the configuration to persist it. There is no second main-scene setting. Selecting a saved configuration uses its startup policy; the current-scene checkbox is a temporary override. A project override requires disabling the current-scene checkbox, and does not inherit the active project's window/profile selection.

**Tools > Player Sessions** (`Ctrl+Shift+R`) reopens the nonmodal session window. Closing this window hides it. The editor application owns its process records across document/project switches. Select a row to inspect its bounded output tail, stop that instance, stop its session, request a scene transition, or cancel loading. Stop All affects only the processes owned by this editor.

The session window distinguishes process creation from application readiness. Instances report initialization, resource/scene loading, activation and running states over a private XII pipe/protocol. Missing status, startup timeout, unexpected exits, exit codes and scene failures remain attributable to the instance. Scene loading failures keep a previously running scene available and can recover through another transition request. A clean application exit is terminal even if it was initiated by its own window rather than the editor.

## Ownership and isolation

- Each launch has a random session identifier; each instance has a distinct identifier and default window title. Optional window/title overrides remain explicit.
- The editor keeps an owning `xiiProcess` for each child. It never finds or controls processes by executable name. Only that instance's private IPC endpoint receives its commands.
- Stop requests are sent through XII IPC. After five seconds, an unresponsive child is terminated through its owned process handle. Editor shutdown stops all owned instances and joins them within a shared bounded shutdown window. A connected Player also exits if its editor connection disappears. Applications are responsible for descendants that they create themselves.
- Managed `xiiGameApplication` processes mount private `appdata`, `bin` and writable `shadercache` roots under the application user directory, project-path hash and instance identifier. A settings lock prevents duplicate writers. Their process working directory and `>temp` are private temporary directories. Temporary directories and port leases are released after exit; preferences, logs and other private application data remain available for diagnostics and do not become the next session's preferences.
- Existing SDK shader permutations are available through a lower-priority read-only mount. Newly compiled permutations go to the private writable cache.
- Standalone Player normally persists preferences in its project-specific user directory. The first process holds its preferences lock. Concurrent standalone processes receive separate private directories; they do not write the first process's preferences. Standalone temporary directories are removed on shutdown.
- Player writes `Player.html` in its `:appdata` directory. The editor also captures stdout/stderr separately for each instance and bounds the displayed history. Managed applications can use their existing log writers and writable mounts.

Applications must use the configured XII writable mounts and special directories for managed isolation. Application-owned absolute file paths, independently chosen networking ports and externally spawned services remain the application's responsibility.

## Main scene, assets and packaging

The existing `xiiProject` manifest's `DefaultScene` is the project main scene, shared by editor startup and Player startup. Store a normalized project-relative source scene path. Scene overrides and runtime transition requests can also use data-directory paths, transformed scene paths or asset GUIDs supported by the filesystem redirection table.

Player uses the normal asset profile, filesystem configuration, runtime plugins, asset redirection tables and exported binary scenes. Source scene paths redirect to their transformed `AssetCache/Common` outputs. A missing/untransformed scene reports its resolved path and an export/package action. `-scene` does not rewrite the manifest. `-preload` selects a collection; `Player.ddl` can supply a project startup collection. Existing adjacent collection discovery remains supported.

A packaged Player can discover `../Data/project` relative to its executable. Exported per-scene Windows launch scripts anchor both executable and project paths at the script directory. Project overrides resolve against the containing project; application overrides resolve against the engine application directory. Player's relative `-project` first resolves against the executable directory, then the SDK. Named launch files resolve against the project. None of these require a particular process working directory.

Named configurations apply to the current process when selected with `-launch`; the editor owns their instance count and process orchestration. Project applications deriving from `xiiGameApplication` inherit managed IPC, private mounts, readiness reporting, background pacing and main-view settings. Applications overriding core lifecycle hooks must preserve the base lifecycle; a custom staged startup calls `InitializeManagedSession()` after core startup. Arbitrary executables that do not implement the XII application/session contract cannot publish application readiness or honor graceful stop and isolation flags.

## Command-line options

```text
xiiPlayer -project "D:/Projects/MyGame" -launch "Two clients"
xiiPlayer -project "D:/Projects/MyGame" -scene "Scenes/Test.xiiScene"
xiiPlayer -project "D:/Projects/MyGame" -window-mode window -width 1280 -height 720 -App.VSync false -App.FrameLimit 60 -render-scale 0.75 -show-fps true
```

| Option | Meaning |
| --- | --- |
| `-project <directory>` | Project location. Editor launches supply an absolute directory. |
| `-launch <name>` | Named configuration in `RuntimeConfigs/Player.ddl`, applied to this process. |
| `-launch-file <path>` | Optional alternate configuration file; relative to the project. |
| `-scene <reference>` | Temporary main-scene override. |
| `-preload <reference>` | Optional startup collection. |
| `-profile <name>`, `-renderer <name>` | Existing asset profile and renderer selection. |
| `-wnd <file>` | Existing explicit window description override. |
| `-window-mode window\|borderless\|fullscreen` | Main-window mode override. |
| `-width`, `-height`, `-display` | Resolution and display; display `-1` selects the primary display. |
| `-window-x`, `-window-y`, `-window-title` | Explicit placement/title overrides. |
| `-App.VSync <bool>`, `-App.FrameLimit <0..1000>` | Presentation and application-wide pacing; zero FPS limit means unlimited foreground rendering. |
| `-background-fps <1..1000>` | Inactive/minimized main-window cap; default 30. |
| `-render-scale <0.1..1>`, `-show-fps <bool>` | Main view/camera resolution scale and native-resolution FPS/frame-time overlay. |
| `-volume <0..1>` | Master volume through the existing sound interface, when present. |
| `-role`, `-connect`, `-port` | Application networking metadata. Player does not create a gameplay networking stack. |
| `-TelemetryPort <port>` | Existing telemetry endpoint. Editor launch configuration assigns this per instance. |
| `-no-file-cvars` | Existing opt-out from saved CVar and presentation preference files. |

Player-owned numeric/window inputs are validated before editor spawning and again at Player startup. Other arguments are passed as tokens directly to `xiiProcess`, without a shell. Per-instance command lines use the existing XII command-line splitter. Managed identity, project, IPC, temporary-directory, role and port arguments are reserved and cannot be replaced by additional arguments.

`standalone`, `server`, `client` and `server-clients` are supported launch roles. In `server-clients`, instance zero is the server and the other instances connect using the configured host and base game port. For `N` instances, ordinary per-instance game ports are `base + index`, and telemetry ports are `base + N + index`. Automatic blocks are derived from the session identifier and probe subsequent blocks when leased. Exclusive leases coordinate cooperating editors; the operating system still decides whether an application can bind its actual gameplay sockets. Telemetry bind failures are reported rather than silently claiming a server exists. Standalone Player leases its telemetry port and chooses another endpoint if the default is occupied, unless explicitly overridden.

## Settings precedence and runtime interaction

The precedence, from lowest to highest, is:

1. Engine defaults and project `RuntimeConfigs/Window.ddl` / `Presentation.ddl`.
2. The current process's persisted user preferences.
3. Named launch/common arguments and launch window placement.
4. Per-instance overrides.
5. Explicit standalone command-line overrides.

The command-line utility uses the first occurrence of an option. Editor assembly puts per-instance tokens before common tokens and automatic placement. Standalone named configuration expansion appends defaults after explicit arguments. `-wnd` selects an entire window description; explicit window-mode/size/display/position options then override its fields. A temporary current-scene selection takes the startup-scene field's place for that launch; per-instance scene arguments can still override it.

**F2** opens the Player settings draft. Arrow keys select/edit; Enter applies; Backspace restores project defaults; F2/Escape closes. The interface scrolls its rows for small windows. Window mode, size and display changes preview on the existing native window, retaining its output target. Enter confirms and saves; Escape, focus loss or a fifteen-second timeout restores the previous display. An unavailable previous monitor falls back to a resizable primary-display window. Failed display changes attempt restoration and retain the previous saved preferences.

Other controls cover VSync, foreground/background frame limits, main-view render scale, FPS/frame time, master volume, background muting and cursor capture. Main-view controls update the active main camera as well as its view, leaving other views and authored assets unchanged. Audio controls use `xiiSoundInterface`; projects without a sound implementation retain the preference without audio output. F1 opens the existing console for advanced renderer/plugin CVars; restart-marked CVars retain their existing delayed/restart semantics. Console presentation/frame-limit changes and the reflected presentation settings stay synchronized.

Startup resolution does not rewrite `Presentation.ddl`. Explicit runtime settings commits persist to the current user/instance root. Player restores ephemeral presentation/frame-limit launch overrides before CVar shutdown persistence when they were not committed at runtime. Other plugin CVars retain their own persistence contracts. Editor viewport preferences remain owned by the editor rendering controls and are not replaced by Player main-view settings.

## Loading and scene transitions

Player creates its native window before project/plugin/device initialization and pumps OS/IPC work between thread-affine startup stages. These initialization stages are indeterminate. Plugin initialization, filesystem/configuration setup and graphics-device creation remain on the main thread and may still block within an individual stage.

Scene loading uses `xiiSceneLoadUtility`, existing resource preloading and `xiiTaskSystem`:

- A file-access task owns scene IO and world-description deserialization. It creates no world, renderer or components and makes no callback into its former owner.
- A custom world-reader type-lookup callback retains its existing caller-thread affinity; that description read stays on the caller thread. Do not change the callback while readers are active.
- Collection completion is measured; scene IO is indeterminate. Incremental instantiation has its own measured phase and one-millisecond step budget. Individual component initialization and activation hooks can exceed that budget.
- The main thread creates the incoming world with simulation disabled and performs incremental component/world initialization. Cancellation retires the reader until completion, cancels the instantiation context and releases the incoming world. Shutdown joins outstanding readers before plugin unloading.
- `xiiGameState::TransitionToScene(scene, collection)` keeps the current world usable while preparing the next one. A newer scene/collection request replaces the pending request; an empty request returns failure without replacing the active scene. A completed request is committed during `ProcessInput`, before world update tasks start.
- `OnBeforeSceneActivation(world, error)` validates the incoming world with write access before committing. Rejection destroys it and retains/restores the outgoing world. The old world stays alive until its view is detached and world-change hooks have completed. Application/game-state services remain outside scene ownership.
- `CancelBackgroundSceneLoading()` keeps/resumes the previous scene where available. `LoadScene()` retains its explicit loading-screen behavior. Custom presentation uses `CreateLoadingScreenWorld`, measured progress/phase queries and the existing completion/failure/cancellation hooks.
- Loading callbacks cannot start another transition reentrantly; enqueue that request for the next frame. Normal sequential/overlapping requests from the runtime API are supported.

## Verification scope

Focused Windows/MSVC Debug checks cover real Vulkan Players launched through the production editor session controller, simultaneous projects, individual/session stop, repeated startup cancellation, main-scene startup, transitions, load failure/recovery, transition cancellation, unexpected early exits and bounded termination of a nonresponsive child. Loader checks cover incremental ownership transfer, IO/instantiation cancellation and malformed/truncated scene failure. Settings checks cover layer precedence, unchanged startup preference files, independent user roots, actual native resize and invalid-display recovery; the existing measured application frame-limit check is retained.

Interactive F2 navigation and timed confirmation have not been acceptance-tested. Native API checks do not replace that interaction check. Other operating systems, D3D12, Shipping, mixed-DPI/multi-monitor transitions, a complete exported-package smoke run and arbitrary third-party application overrides remain unverified. Filesystem reads and application/component initialization cannot be forcibly interrupted in place; managed processes retain the bounded stop fallback.

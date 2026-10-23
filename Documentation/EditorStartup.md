# Editor Startup Presentation

`EditorFramework/GUI/EditorSplashScreen` is a lightweight Qt Widgets startup panel. It uses the existing XII wordmark, converted from the repository's Colonna MT logo artwork into SVG paths so Linux and macOS do not need that font installed. System UI fonts provide native typography; Qt paints text, vectors, and a standard progress bar at the current device pixel ratio. No GPU context, image generator, animation framework, downloaded asset, or extra runtime dependency is involved.

The panel uses a restrained dark brand palette in both light and dark desktops. Its logical composition is 720 × 412, scaled down to fit small available screen geometries. It starts on the screen under the pointer, with the primary screen as fallback. Window flags identify it as a splash, request no focus, and omit always-on-top behavior; `WA_QuitOnClose` is disabled. After the main container is shown, its Qt window becomes the splash's transient parent; a single raise above that active container preserves the startup presentation without repeatedly raising over other applications. It does not create or replace a taskbar window. The application title, SDK version, build configuration/platform/bitness, and existing CMake-generated revision identify the executable being started.

## Actual Startup Stages

The existing `StartupEditor` sequence and project-selection precedence remain authoritative. Stage calls are adjacent to the work they describe:

- Prepare the workspace and initialize editor subsystems, document types, actions, and asset services.
- Prepare application/user directories and load preferences, shortcuts, tool settings, and recent projects.
- Create existing Editor panels and discover plugin bundle metadata.
- Open/create the explicit or recent project, load its plugin DLLs and configuration, register asset types, initialize the existing rendering process, and index assets.
- Restore project documents and transition to the Editor, project dashboard, or an existing startup dialog.

There is no overall startup percentage or minimum display duration. Most stages have unknown cost and use Qt's indeterminate progress mode. The resolved plugin load order has a real item count; only that stage displays measured completed/total work. Counts represent items, not elapsed time. Advancing to another stage restores indeterminate mode. The existing general `xiiProgress` dialog is attached after the splash closes, avoiding competing startup progress dialogs; taskbar/tray subscriptions retain their existing sources.

## Responsiveness and Ownership

Initialization stays on its existing GUI thread and in its existing order. Painting a status never implicitly processes arbitrary commands. Qt events are serviced at the initial expose and between plugin operations, after each prior operation has returned. The render-process startup wait listens to actual IPC/channel and child-process events; the asset-index wait uses an empty dependent task group and its completion callback to wake a stack-owned Qt event loop. Neither adds polling, artificial sleeps, or another startup scheduler. Editing input is excluded from these nested waits, which also preserve the original unbounded wait contract until response, disconnection, or a startup quit request.

Individual synchronous subsystem/plugin callbacks, process launch, and final asset metadata work still run synchronously. They must return before another Qt event boundary; moving these operations to worker threads would require a separate thread-safety change and is outside this presentation feature.

Only GUI-thread code owns or paints the splash. IPC I/O callbacks enqueue receiver-bound invocations; synchronized subscription teardown precedes destruction of the local event loop. Asset initialization is joined before its task-dependent loop is destroyed. No borrowed worker-owned text or QWidget is accessed from another thread.

The splash is closed immediately at the end of startup/project opening, before dashboard selection, on project-open failure, and at shutdown. It does not use `QSplashScreen::finish`'s expose wait. Showing a dialog hides the splash so a question or failure cannot be obscured. The usual project-open diagnostics remain in place; an actual rendering-process startup failure during interactive startup now explicitly directs the user to the Editor log. Click-to-dismiss hides presentation while initialization continues.

Native close/quit requests received during startup are recorded once. An IPC startup wait wakes and returns through its normal process cleanup path. Already running asset initialization is joined before teardown. Once the initialization scope returns, `RunEditor` skips `exec` for a pending quit and ordinary `ShutdownEditor` owns cleanup. Startup never destroys partially initialized documents/plugins from a nested event loop. A fresh startup resets that request.

The `-NoSplash` option and existing `EditorPreferences/ShowSplashScreen` setting remain honored. Headless, background, and normal unit-test startup do not create a splash. Safe mode, explicit `-project`/`-newproject`, document arguments, recent-project policy, and the existing project-switch process behavior retain their established meanings.

## Verification and Acceptance

The `EditorSplash` suite renders the actual production widget under light/dark application palettes at 1×, 1.5×, 2×, and 3× image device pixel ratios. PNGs are written under the test output's `SplashPreviews` directory for visual inspection. It also covers actual versus unknown work, count bounds, full accessible Unicode details, taskbar/focus flags, available-screen placement, dismissal/modal diagnostic handoff, suppression options, startup/shutdown with a quit before the main event loop, real pipe/protocol connection and project-ready response delivery, child-process disconnection, and a quit during an event-driven IPC wait. Run the suite twice with the same output directory to exercise fresh processes before and after saving the real startup shortcut assignments; the Editor's plugin lifecycle is not designed for repeated `StartupEditor` calls in one process. The shortcut fixture and splash-policy settings are isolated from the user's preferences.

Regression suites are `EditorJumpList` (including startup actions), `EditorTaskbar`, `EditorSystemTray`, `EditorNotifications`, and `CodeGen`. The implementation uses existing Qt Widgets/SVG dependencies; the test target explicitly links Qt Svg for the public widget header.

Manual acceptance is still required on Windows, Linux, and macOS with real project/rendering plugins: cold and warm project opening, available multi-monitor arrangements, live movement/hot-plug between different scale factors, light/dark desktop themes, a slow plugin, a failed project or rendering process, native OS quit/close during startup, and taskbar/Dock behavior. Rendered image-scale tests do not substitute for those platform checks.

The Windows validation workspace can use the existing `XII_OUTPUT_DIRECTORY_DLL` CMake option to isolate binaries when a live engine or asset processor holds the normal output directory open. That is a local build choice, not a new repository option or deployment requirement.

Qt references: [splash lifecycle and event processing](https://doc.qt.io/qt-6.11/qsplashscreen.html), [event-loop input exclusions](https://doc.qt.io/qt-6.11/qeventloop.html), and [high-DPI logical coordinates](https://doc.qt.io/qt-6.11/highdpi.html).

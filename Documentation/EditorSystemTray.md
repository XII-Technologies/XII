# Editor System Tray

`EditorFramework/GUI/EditorSystemTray` presents XII Editor in the operating system's notification area using `QSystemTrayIcon` and `QMenu`. It uses the existing Qt application/window icon, including its SVG resource, without creating or owning a native window or `HICON`.

The Editor creates the controller for interactive sessions and destroys it at the start of `ShutdownEditor`, before project teardown or progress destruction can process UI events. Headless/background processors and normal unit-test startup do not create tray entries. Builds of Qt without the system-tray feature compile out the controller. No setting, background process, polling timer, close interception, or minimize-to-tray behavior is added. Qt's existing quit-on-last-window behavior stays in place.

## Commands

- **Show XII Editor**, single-click, and double-click restore the main container while preserving its maximized state. An active modal dialog receives activation when appropriate. Qt's `raise()`/`activateWindow()` follow the platform's foreground restrictions; an attention request is the fallback.
- **Recent Projects** reads the existing project history when the context menu opens, retaining its order and filtering missing, inaccessible, and non-project files. Labels escape menu mnemonics, while action data retains the full Unicode project path. Selection uses the existing queued `OpenProject` path, including its normal project switching and separate-process behavior.
- **Open Project...** opens the existing project dashboard.
- **Exit XII Editor** requests the normal container close path. Existing unsaved-document prompts, close vetoes, layout saving, project teardown, and application exit remain authoritative. Cancelling close leaves the tray entry available.

Project-changing commands and Exit are disabled during foreground progress, external tools/builds, or a modal dialog. Queued commands check those conditions again before running. Show remains available to bring the active operation or dialog forward. Background asset processing does not disable normal project-close handling.

## Status

The tooltip is a concise activity view, independent of taskbar progress colors and diagnostic overlays:

- Existing `xiiProgress` events supply active import/transform labels and cancellation state. Producer text is copied before GUI-thread dispatch; queued callbacks never borrow a progress range.
- The existing paired tool lifecycle signals carry matching display labels. Shader compiler invocations report **Compiling shaders**, C++ builds report **Building C++ project**, and other external tools use their actual tool name. Concurrent operations are counted by label and presented in deterministic order. Completion and failure both remove the activity label; taskbar diagnostics retain their existing failure policy.
- Existing asset-processor state events report whether background asset processing is enabled or finishing. Enabled processing is not presented as a measured active job or percentage. Processor event subscription/teardown is synchronized with worker broadcasts, and tray updates are queued to the Qt GUI thread.

The tooltip is bounded to Windows' 127 UTF-16 code-unit limit without splitting surrogate pairs. Unchanged text is not resent. Individual log messages do not produce tray badges or notifications.

## Platform Behavior

The controller requests visibility even when a notification area is currently unavailable. Qt registers the entry if support appears later and handles Explorer/taskbar recreation. This is also compatible with supported Linux tray implementations and macOS; an unavailable tray does not change Editor startup or window behavior. Activation gestures follow Qt's desktop capabilities; the Show command remains available where clicks open the menu instead (such as macOS tray icons with menus). No native Shell restart hook or retry timer is added. See the [Qt system tray contract](https://doc.qt.io/qt-6.11/qsystemtrayicon.html).

Qt renders the existing `QIcon` at the platform's required sizes. Window/application icon, palette, theme, style, and device-pixel-ratio events refresh the icon through Qt. Normal application quit hides the entry; controller destruction explicitly hides it, detaches the owned menu, and unsubscribes from status sources. Queued receiver callbacks are discarded with the controller. Destroying the borrowed main window hides the entry and makes activation harmless.

## Verification

Build `Editor` and `EditorFrameworkTest`, then run:

```text
EditorFrameworkTest.exe -noGui -filter EditorSystemTray -outputDir <test-output-folder>
EditorFrameworkTest.exe -noGui -filter EditorTaskbar -outputDir <test-output-folder>
EditorFrameworkTest.exe -noGui -filter EditorJumpList -outputDir <test-output-folder>
EditorFrameworkTest.exe -noGui -filter CodeGen -outputDir <test-output-folder>
```

Tray tests cover overlapping operations, real tool-start failure, import/cancellation labels, bounded Unicode tooltips, current project history, activation, preserved maximization, close vetoes, modal/progress guards, worker notifications, concurrent subscription teardown, icon changes, native-window lifetime, actual Editor shutdown, and normal exit from Qt's event loop with the controller alive. They can also run with Qt's offscreen platform to exercise unavailable-tray behavior. Tests create only temporary tray entries and do not alter personal recent-project files.

Manual visual acceptance should check tray placement, theme/high-DPI rendering, tooltip appearance during real shader compilation/import/build operations, activation from another application, recent-project switching, unsaved-document cancellation, normal exit, and restoration after restarting Explorer. Platform API success and requested visibility do not certify every desktop environment's visual behavior.

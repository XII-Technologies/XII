# Editor Windows Notifications

The interactive Windows Editor uses native `Windows.UI.Notifications` desktop toasts for operation outcomes. The controller lives in `EditorFramework/Platform/Windows` beside the taskbar and Jump List integrations. Linux/macOS and headless, background-processor, and unit-test startup retain their existing behavior.

## Deployment and Identity

The repository currently produces an unpackaged Win32 Qt executable. Its embedded manifest requests `asInvoker` with `uiAccess=false`. It has no Editor MSIX manifest, binary installer, Windows App SDK dependency, or runtime bootstrap. The release workflow packages source snapshots. The Editor already establishes the process AppUserModelID `XII.Technologies.Editor` before Qt creates windows; notifications reuse that identity without changing taskbar grouping or Jump Lists.

The native WinRT desktop API is supplied by Windows and the Windows SDK (`RuntimeObject.lib`); no Windows App SDK installation or NuGet runtime is needed. Calls use HRESULT-based WRL interfaces and balanced GUI-thread `RoInitialize`/`RoUninitialize` alongside the existing taskbar COM apartment.

Windows requires a Start menu shortcut with the matching AppUserModelID for this desktop API. At the first eligible notification, the controller creates or updates the current user's `Programs/XII Editor.lnk`, pointing to the actual running executable with its embedded icon. This supports a developer checkout or extracted build without administrator installation. It does not overwrite a same-named shortcut belonging to another application. A future binary installer should own this shortcut instead; removing a checkout currently requires removing its shortcut manually. Launching a different build refreshes the owned shortcut's executable and icon on its first notification attempt.

Elevated processes are deliberately excluded before shortcut registration or notifier creation. Notification failures, unavailable shell services, unwritable shortcut folders, native Shell executable/icon path limits, and disabled Windows notifications remain optional presentation failures; they never fail an operation, interrupt startup, or prevent shutdown. Unicode executable paths and spaces use explicit wide Shell interfaces; executable paths beyond the native Shell's supported length may disable toast registration.

## Live-Process Activation

The supported legacy desktop `ToastText02` template provides native toast UI and a click action returning to the live Editor. This is intentionally a session notification, not a persistent Action Center workflow. It does not register a COM local server, protocol handler, cold-start command line, extra process, or global single-instance policy. There are no extra buttons requiring persistent COM activation. XII's project switching already launches another Editor when needed, so notification clicks must not launch an executable or replay project arguments.

The native `Activated` callback queues the shared `xiiQtEditorApp::ShowEditorWindow` path also used by tray commands. Qt restores the borrowed container, preserves maximization, targets an active modal dialog, and requests attention if foreground activation is restricted. Native callbacks never inspect a `QPointer` or access GUI objects directly. A mutex-protected reference-counted callback gate serializes enqueueing with destruction; receiver-bound Qt functors and generation checks discard late or superseded callbacks.

The controller retains at most one toast. Activation, dismissal, delivery failure, foreground acknowledgement, a new tool batch, project close, window destruction, and shutdown revoke or clear it. Queued asset/long-operation outcomes carry a project generation so a project switch cannot display an old result. Toasts are not replayed after Explorer restarts: they describe transient outcomes, whereas the existing taskbar and tray integrations retain responsibility for restoring their persistent status.

## Event and Attention Rules

| Event | Notification rule while Editor is unfocused |
| --- | --- |
| Shader compilation or C++ project build | Failure, or successful completion after at least 10 seconds |
| Explicit asset-import batch | Imported/failed totals; any failure, or successful completion after at least 10 seconds |
| Other external tool | Failure after at least 10 seconds |
| Background asset request | Final processing failure; successful transformations and `NeedsImport` are silent |
| Engine long operation | Failed result after at least 10 seconds; explicit cancellation and success are silent |
| Logs, minor warnings, progress ticks | Silent |

Tool start/finish signals are paired by their existing operation label. Concurrent instances of the same label form one batch, keeping its earliest start and any failure until the last instance finishes. An unmatched or duplicate finish is ignored. Independent labels remain independent.

Foreground detection uses Windows' actual foreground process, including other Editor containers and modal dialogs; it still works when an external tool blocks Qt's event loop. Toasts are silent, check `SHQueryUserNotificationState`, and require `IToastNotifier::Setting` to be enabled. Windows controls Focus Assist/Do Not Disturb and delivery. The controller never changes notification preferences or uses an attention/alarm scenario to bypass them.

At most one delivery attempt is made per 30 seconds per Editor process. Bursts, disabled deliveries, and failed native attempts are dropped rather than queued for later. Failed WinRT interfaces are released and reacquired on the next eligible event, without polling. Titles and messages are bounded and XML-escaped without splitting UTF-16 surrogate pairs. Application attribution/icon comes from the matching executable shortcut; no separate icon cache is maintained.

Actual outcomes come from existing execution boundaries: paired tool/build signals, the import dialog's selected-generator batch, the processor's final result state, and the long-operation controller's received result message. Import save errors now propagate into the batch result; long-operation events report cancellation separately and ignore duplicate result messages. No notification polling or log subscription is introduced.

## Verification

`EditorNotifications` covers attention thresholds, overlapping tools and failure aggregation, duplicate finishes, native XML/toast construction, Unicode content, per-user shortcut properties in an isolated temporary directory, invalid/conflicting shortcut failures, GUI-thread activation, superseded callbacks, borrowed-window destruction, late callbacks, actual Editor shutdown, and real controller result/cancellation delivery through its IPC event boundary. Native content tests construct a toast without displaying it or installing a production Start menu shortcut.

Regression suites: `EditorSystemTray`, `EditorTaskbar`, `EditorJumpList`, and `CodeGen`. Build `EditorFrameworkTest` and `Editor` with the normal unity configuration. No notification controller is automatically created in test startup; lifecycle tests attach it explicitly.

Manual acceptance on a normal, unelevated Editor remains necessary:

1. Start from a developer/extracted build without a shortcut. Run a shader compilation or C++ build lasting over 10 seconds and switch away before completion. Check the native toast title, text, application attribution, and executable icon.
2. Click it while the Editor is minimized, maximized, or displaying a modal dialog. Confirm restoration targets that live process and no new process is created.
3. Exercise failed shader builds, mixed import batches, asset processor crashes, and canceled/failed long operations. Verify details remain available in the existing Editor diagnostics and repeated background failures are limited.
4. Repeat while focused, with Windows notifications disabled, with Do Not Disturb/Focus Assist, under elevation, and with an unwritable or conflicting shortcut. Normal editing and shutdown must continue without notification UI errors.
5. Close/switch projects or quit with a toast/callback pending. Confirm no stale activation or notification remains. Restart Explorer and verify subsequent notifications plus the existing taskbar, Jump List, and tray behavior.

## Native API References

- [Desktop toast shortcut and AppUserModelID requirement](https://learn.microsoft.com/en-us/windows/win32/shell/enable-desktop-toast-with-appusermodelid)
- [Desktop native toast delivery and live activation callbacks](https://learn.microsoft.com/en-us/windows/win32/shell/quickstart-sending-desktop-toast)
- [WRL desktop notifications, legacy templates, and persistent COM activation](https://learn.microsoft.com/en-us/windows/apps/design/shell/tiles-and-notifications/send-local-toast-desktop-cpp-wrl)
- [Windows notification-state query](https://learn.microsoft.com/en-us/windows/win32/api/shellapi/nf-shellapi-shqueryusernotificationstate)
- [Windows App SDK notification limitations](https://learn.microsoft.com/en-us/windows/apps/design/shell/tiles-and-notifications/toast-notifications-overview)

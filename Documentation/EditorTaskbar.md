# Windows Editor Taskbar Integration

`EditorFramework/Platform/Windows` owns native taskbar presentation. One `xiiWindowsTaskbar` serves the main Qt container window. It is created before that window is shown, observes the Editor's existing progress and status sources, and is destroyed before the window and progress source. Headless editor processors do not create a taskbar controller. Linux and macOS do not compile the native implementation or link the Windows COM libraries.

## Operation Sources

| Source | Taskbar behavior |
| --- | --- |
| Editor `xiiProgress` | Starts indeterminate; uses reported completion values once available. Cancellation stays yellow while the operation unwinds, then clears. Existing nested progress ranges remain one foreground operation. |
| File-to-asset import | Counts selected input files in the import dialog and reports each completed import. Individual generators use a nested range while creating and saving documents. The import dialog's controls are disabled while the existing progress UI processes events. |
| `ExecuteTool` | Indeterminate while the real child process runs. This includes the material asset's shader compiler invocation. |
| C++ project `CompileSolution` | Indeterminate for the existing CMake build and output validation. Every early return reports completion or failure through the same tool lifecycle signals. |
| Asset curator and processor | Uses the existing up-to-date/total asset counts. Unknown asset states are indeterminate; pending transforms/thumbnails are yellow when processing is stopped or stopping. Transform and dependency failures are red. |
| Long-operation controller | Uses the reported fractions of running engine operations under the controller's mutex. Completed and removed operations no longer contribute. |

Shader work performed inside an editor processor contributes through the parent Editor's asset queue; this feature does not add a separate shader progress protocol. Tools and builds do not invent percentages or parse arbitrary compiler output into estimates. Existing progress dialogs, process execution, cancellation, and logging remain in place.

## Concurrent Work and Notifications

All sources feed one presentation state. Progress priority is **error, paused, indeterminate, normal, inactive**. Available measured fractions have equal weight, and work with unknown progress keeps the display indeterminate unless an error or pause takes priority. Only the controller writes to `ITaskbarList3`, so concurrent operations cannot overwrite each other's native taskbar state. Tool lifecycle signals and copied progress/status snapshots are delivered to the GUI thread; periodic asset and long-operation snapshots are collected there as well.

Overlays use the existing Qt warning/error SVG resources, rendered at 16 logical pixels with the window's device pixel ratio. Qt creates an owned `HICON`, which remains valid throughout `SetOverlayIcon` and is destroyed immediately after that call, on success and failure. The taskbar keeps its own copy. No success badge is displayed.

Badge conditions are deliberately narrower than log history. Current assets needing manual import produce a warning; transform failures, missing dependencies, and cycles produce an error. The existing restart/reload reason sets produce a warning through `RequiredActionsChanged`, emitted where the Editor updates those conditions. Live asset conditions reuse the progress controller's existing curator snapshots; there is no additional overlay timer or polling mechanism. Resolving asset issues or clearing required actions automatically removes their badges.

A failed tool batch produces an error badge until the user acknowledges it using the existing status-bar gesture, starts a new tool batch, or closes its project. A concurrent tool's success cannot erase another tool's failure. Error takes precedence over warning: clearing a failure reveals any remaining asset/configuration warning. Arbitrary global text, unread log counts, and individual warning/error log entries do not create persistent badges. Clearing unrelated log text does not acknowledge a failed operation, and historical messages cannot revive a badge after a successful retry.

Closing a project clears its tool failures, including failures reported through local shader logs instead of the global log panel. Reload reasons clear through the existing project-close path; an Editor restart requirement remains until that condition is resolved. Inactive means no outstanding work or tool failure; it clears the progress bar. An actionable warning can retain an overlay independently of progress. `SetProgressValue` is called only for reported numeric values, before `SetProgressState`, so Windows cannot accidentally replace yellow/red/indeterminate with green. Unchanged overlays are not resent on percentage updates. See Microsoft's documentation for [progress state transitions](https://learn.microsoft.com/en-us/windows/win32/api/shobjidl_core/nf-shobjidl_core-itaskbarlist3-setprogressstate) and [overlay restoration](https://learn.microsoft.com/en-us/windows/win32/api/shobjidl_core/nf-shobjidl_core-itaskbarlist3-setoverlayicon).

## Native Lifetime and Recovery

The controller borrows Qt's existing `HWND` through `internalWinId()` and observes native handle changes without changing window ownership, styles, or the window procedure. It waits for `TaskbarButtonCreated` before publishing state. `TaskbarCreated` invalidates the shell interface; the subsequent button notification recreates it and reapplies the latest state. A recreated Qt native window receives the same treatment.

COM initialization and all interface calls/releases stay on the GUI thread. Successful `CoInitializeEx` calls, including `S_FALSE`, have matching `CoUninitialize` calls. An existing incompatible apartment is not uninitialized by the controller. Native messages dispatched during a COM call defer interface resets until the call returns. Destruction removes observers and native event filters, clears presentation when possible, and drops queued Qt callbacks. COM or taskbar failures retain the desired state for a later retry and do not fail the underlying operation.

## Jump Lists

The Editor executable establishes the version-independent AppUserModelID `XII.Technologies.Editor` before constructing Qt or showing UI. The destination list and Shell links use the same identity. The existing Qt window icon, progress, and overlay controller retain their ownership and behavior. Shortcuts distributed for this executable should use this identity as well; shortcuts pinned before explicit identity was introduced may need to be pinned again. See Microsoft's [application identity guidance](https://learn.microsoft.com/en-us/windows/win32/shell/appids).

The native `ICustomDestinationList` builder uses `IObjectCollection` and Unicode `IShellLinkW` objects. Its **Recent Projects** category follows the existing `xiiRecentFilesList` order, excluding missing files, directories, non-project entries, inaccessible files, duplicates, and destinations removed through the Windows UI. The Shell's available destination slots determine how many valid entries are submitted. Existing project history loads and changes trigger updates; document saves, progress ticks, and unchanged project-save events do not rebuild the list. Unit tests, background/headless processors, and `-noRecent` sessions do not publish the Editor's list.

Windows clears its removed-destination records after a successful commit. Removed project paths are therefore pruned from XII's authoritative recent-project history after commit and persisted through the existing history file, preserving all remaining entries' order. Opening a project again puts it back through normal history insertion. Failed transactions abort and retain history for a later update. Privacy settings that reject custom categories still allow the common task entries to be committed. See the Shell's [removal contract](https://learn.microsoft.com/en-us/windows/win32/api/shobjidl_core/nf-shobjidl_core-icustomdestinationlist-beginlist) and [privacy behavior](https://learn.microsoft.com/en-us/windows/win32/api/shobjidl_core/nf-shobjidl_core-icustomdestinationlist-appendcategory).

Recent-project links target the actual Editor executable and use the existing `-project` startup argument. Windows argument quoting preserves spaces, Unicode, quotes, and trailing backslashes without `MAX_PATH` argument buffers. The existing OS command-line parser supplies UTF-8 paths to the normal project-opening flow. Links inherit the executable directory as their working directory. XII currently uses separate Editor processes for project lifetimes; Jump List activation follows that behavior and adds no single-instance protocol.

The native Tasks section contains **Create New Project** (`-createProject`) and **Open Project** (`-openProject`). These startup requests select the existing queued project-creation dialog and project dashboard respectively, taking precedence over automatic last-project loading. Explicit `-newproject <path>` or `-project <path>` requests retain precedence over those dialogs. Cancelling a task's dialog leaves a normal empty Editor window. These options do not disable subsequent history updates.

Jump List updates reuse the taskbar controller's GUI-thread COM apartment and serialize reentrant history requests. Every native interface and property value is released locally; successful commits persist in the Shell across Explorer restarts, independently of the window's progress/overlay replay. Native failures produce a diagnostic and preserve normal Editor operation. No new history store, timer, configuration preference, or window procedure is introduced.

## Verification

Build `Editor` and `EditorFrameworkTest`, then run the focused suite:

```powershell
EditorFrameworkTest.exe -noGui -filter EditorTaskbar -outputDir <test-output-folder>
EditorFrameworkTest.exe -noGui -filter EditorJumpList -outputDir <test-output-folder>
ToolsFoundationTest.exe -noGui -filter RecentFilesList -outputDir <test-output-folder>
```

The tests cover arbitration, value/state ordering, actionable overlay priority and fallback, ignored log history, successful retries, Qt resource rendering, native icon validity during the call and destruction afterward (including failed calls), actual successful/failed child processes, concurrent worker-thread notifications, cancellation, COM failure recovery, native handle recreation, badge replay after shell notifications, reentrant shell notifications, and shutdown with queued updates. A native shell smoke test checks actual COM calls against a Qt window when Explorer is present. It needs access to shell COM; a restricted sandbox can deny that access even while Explorer is running. The smoke test is skipped when Explorer is absent. Shell restart tests send the registered messages to the controller without restarting the user's Explorer process.

For visual acceptance, use real Editor operations: transform/import assets, compile a material shader, and build a project's C++ plugin. Verify overlap, processor pause/resume, failed builds, resolved asset issues, and status acknowledgement. Verify state restoration after an actual Explorer restart and Qt native window recreation. Native API success alone does not certify taskbar appearance, high-contrast behavior, or every taskbar grouping setting.

Jump List tests round-trip Shell-link arguments through Windows and the engine parser, exercise Unicode and long project paths, preserve history order and removals across commits/reload, inject transaction failures/privacy restrictions, and verify the actual creation/dashboard dialogs through startup. A native transaction smoke test uses a temporary test identity and deletes its list afterward. For visual acceptance, right-click the Editor's taskbar icon, open a recent project and both common tasks, remove a project and trigger a history update/restart, then reopen it explicitly. Verify current and newly pinned shortcuts, cancellation, and launch paths containing spaces and Unicode without disturbing existing progress/overlay presentation.

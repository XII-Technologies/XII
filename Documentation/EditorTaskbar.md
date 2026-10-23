# Windows Editor Taskbar Integration

`EditorFramework` owns one `xiiWindowsTaskbar` for the main Qt container window. It is created before that window is shown, observes the Editor's existing progress and status sources, and is destroyed before the window and progress source. Headless editor processors do not create a taskbar controller. Linux and macOS do not compile the native implementation or link the Windows COM libraries.

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

Overlays use Windows' shared warning and error icons, with error taking precedence. They reflect the Editor's existing permanent/global status messages and current asset conditions. Assets needing manual import produce a warning; transform failures, missing dependencies, and cycles produce an error. Live asset conditions are checked every 200 ms and disappear automatically when resolved. Log notifications clear when the existing status message is cleared or acknowledged. A failed tool batch remains red until acknowledged or another tool batch starts; a successful concurrent tool does not erase another tool's failure. Independent unresolved conditions continue to take precedence.

Closing a project clears its tool failures, including failures reported through local shader logs instead of the global log panel. Status overlays continue to follow their existing UI messages; closing a project does not acknowledge independent application conditions. Inactive means no outstanding work or tool failure; it clears the progress bar. A warning or unread log notification can retain an overlay independently of progress. `SetProgressValue` is called only for reported numeric values, before `SetProgressState`, so Windows cannot accidentally replace yellow/red/indeterminate with green. Unchanged overlays are not resent on percentage updates. See Microsoft's documentation for [progress state transitions](https://learn.microsoft.com/en-us/windows/win32/api/shobjidl_core/nf-shobjidl_core-itaskbarlist3-setprogressstate) and [overlay restoration](https://learn.microsoft.com/en-us/windows/win32/api/shobjidl_core/nf-shobjidl_core-itaskbarlist3-setoverlayicon).

## Native Lifetime and Recovery

The controller borrows Qt's existing `HWND` through `internalWinId()` and observes native handle changes without changing window ownership, styles, or the window procedure. It waits for `TaskbarButtonCreated` before publishing state. `TaskbarCreated` invalidates the shell interface; the subsequent button notification recreates it and reapplies the latest state. A recreated Qt native window receives the same treatment.

COM initialization and all interface calls/releases stay on the GUI thread. Successful `CoInitializeEx` calls, including `S_FALSE`, have matching `CoUninitialize` calls. An existing incompatible apartment is not uninitialized by the controller. Native messages dispatched during a COM call defer interface resets until the call returns. Destruction removes observers and native event filters, clears presentation when possible, and drops queued Qt callbacks. COM or taskbar failures retain the desired state for a later retry and do not fail the underlying operation.

## Verification

Build `Editor` and `EditorFrameworkTest`, then run the focused suite:

```powershell
EditorFrameworkTest.exe -noGui -filter EditorTaskbar -outputDir <test-output-folder>
```

The tests cover arbitration, value/state ordering, overlays and clearing, actual successful/failed child processes, concurrent worker-thread notifications, cancellation, COM failure recovery, native handle recreation, shell notification replay, reentrant shell notifications, and shutdown with queued updates. A native shell smoke test checks actual COM calls against a Qt window when Explorer is present. It needs access to shell COM; a restricted sandbox can deny that access even while Explorer is running. The smoke test is skipped when Explorer is absent. Shell restart tests send the registered messages to the controller without restarting the user's Explorer process.

For visual acceptance, use real Editor operations: transform/import assets, compile a material shader, and build a project's C++ plugin. Verify overlap, processor pause/resume, failed builds, resolved asset issues, and status acknowledgement. Verify state restoration after an actual Explorer restart and Qt native window recreation. Native API success alone does not certify taskbar appearance, high-contrast behavior, or every taskbar grouping setting.

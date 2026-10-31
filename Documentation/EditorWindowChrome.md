# Editor window chrome

The Windows Editor has one integrated custom title bar containing the live
document menu, project label, Commands, Workspaces and accessible caption buttons.
Windows retains the native system menu, window capabilities, resize frame and
move/restore/snap operations. Empty regions and mouse-transparent labels are
caption drag surfaces. Menus, buttons, disabled controls and other child widgets
do not become caption regions when they ignore a mouse event.

## Investigation

- The former `CustomizeWindowHint` flags omitted `WindowMinimizeButtonHint`,
  `WindowMaximizeButtonHint` and `WindowCloseButtonHint`. Qt consequently did not
  give Windows the normal minimize/maximize capabilities. A Qt button calling
  `showMaximized()` does not restore those native window capabilities.
- `ExpandedClientAreaHint` removes the normal nonclient/client separation. In
  the installed Qt 6.11 Windows implementation, expanded-area hit testing also
  delivers synchronous widget mouse events while deciding caption ownership.
  Interactive controls now remain ordinary client widgets. `WM_NCCALCSIZE`
  extends only the top client edge over the native caption, retaining native
  side/bottom borders and the maximized work area. Standard Qt window flags
  retain minimize/maximize/close capabilities without mutating the native styles
  behind Qt's frame calculations or painting a second caption.
- Actual Windows GUI checking found native-caption dragging worked while the
  embedded strip's `QWindow::startSystemMove()` did not move the window. The
  Windows-only `WM_NCHITTEST` fallback therefore returns `HTCAPTION` exclusively
  for the strip's noninteractive regions. Windows performs the entire move,
  restore and snap operation. There is no application-controlled move loop.
- The native message supplies signed physical screen coordinates.
  `ScreenToClient()` removes the window's physical origin before converting the
  client offset with that window's current Qt device pixel ratio. This avoids
  primary-monitor assumptions and applying a single scale to global coordinates.
  The top resize strip uses `GetSystemMetricsForDpi()` for the current HWND;
  remaining native resize borders retain `DefWindowProc` hit testing.
- The expanding embedded menu consumed potential caption space. Its preferred
  width now leaves surplus space to the dedicated caption spacer.
- `resizeEvent()` changed Commands/Workspaces presentation at 1100 pixels,
  changing size hints during layout. Buttons now retain text beside their icons.
  A regular horizontal layout keeps caption controls in the row instead of
  sending them into a toolbar overflow menu. The menu bar retains its own menu
  overflow behavior. Font metrics and logical-pixel style metrics scale through
  Qt. Fixed caption-button/icon sizes do not depend on maximize/restore glyphs.
- `UpdateDocumentMenu()` already skipped identical source/action lists. Ordinary
  unchanged focus was therefore not proof of repeated menu reconstruction.
  Focus notifications are now restricted to document context entry, temporary
  focus loss and title-control interaction retain that context, and plugin menu
  changes apply individual insertions/removals rather than clearing every action.
  Document removal explicitly resynchronizes the menu.
- `xiiQtDarkEditorStyle` has no hover-dependent size metrics. Its toolbar spacing,
  control height rules and ADS stylesheet dimensions are unchanged. Regression
  coverage checks size hints and geometry with hover, focus, press and popups.

The custom row has a 32-logical-pixel minimum height, growing with larger fonts,
and retains the upper native resize strip. It uses a charcoal surface, flat menu/command hover backgrounds,
fixed 46-pixel caption hit widths and 16-pixel SVG glyphs. Hover rules change
paint only, with common border/padding in all states. Project names are elided
at paint time, retaining the full accessible label and tooltip without changing
text or size hints during resizing.

Qt implementation references: [native flags and expanded-area hit testing](https://github.com/qt/qtbase/blob/v6.11.0/src/plugins/platforms/windows/qwindowswindow.cpp),
[system movement API](https://doc.qt.io/qt-6/qwindow.html#startSystemMove).

## Regression coverage

`EditorWindowChrome` exercises caption/control exclusion, double-click
maximize/restore and fullscreen exclusion, native Windows capabilities,
all eight resize hit regions on each connected screen, client/caption hit
testing in normal and maximized states, native maximize/restore commands,
document switching/closing, transient focus and incremental plugin menus.
It also checks menu/toolbar geometry and size hints over 800, 960, 1099, 1100,
1101 and 1280 logical-pixel window widths, hover/focus transitions and popup use.

Run it with the native Windows Qt plugin and repeat with `QT_SCALE_FACTOR=1.5`
and `2`. `EditorContainerShutdown` covers late focus and deferred destruction;
`EditorIntegration` exercises the real scene workspace, document/plugin menus,
shortcuts, compact layouts, ADS and Vulkan rendering.

Programmatic maximize/restore and hit-test success do not prove Windows Snap.
GUI acceptance must drag the custom caption spacer,
drag to the screen top, side and corner, pull a maximized window down, use the
custom maximize/restore buttons, double-click the caption, resize
the frame and interact with menus and toolbar controls. Cross-monitor mixed-DPI
dragging requires an actual corresponding monitor configuration.

The initial Windows implementation passed the chrome suites at 100/150/200%,
shutdown/focus tests and Windows/Vulkan EditorIntegration. Subsequent custom
caption/layout refinements were built, with additional suite runs stopped at
the user's request. GUI acceptance confirmed custom maximize/restore and
pulling the maximized custom caption down to restore. The final appearance,
top/side/corner Snap and mixed-DPI dragging require further GUI acceptance;
Computer Use was stopped by the user. Earlier drag-to-top success on a separate
native-caption prototype is not acceptance evidence for this final version.

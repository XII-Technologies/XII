#pragma once

#include <Core/CoreDLL.h>
#include <Foundation/Math/Rect.h>
#include <Foundation/Math/Size.h>
#include <Foundation/Math/Vec2.h>
#include <Foundation/Strings/String.h>
#include <Foundation/System/PlatformFeatures.h>
#include <Foundation/Types/UniquePtr.h>

class xiiOpenDdlWriter;
class xiiOpenDdlReader;
class xiiOpenDdlReaderElement;

// Include the proper Input implementation to use
#if XII_ENABLED(XII_PLATFORM_WINDOWS_DESKTOP)
#  include <Core/System/Implementation/Win/InputDevice_win32.h>
#elif XII_ENABLED(XII_PLATFORM_WINDOWS_UWP)
#  include <Core/System/Implementation/uwp/InputDevice_uwp.h>
#elif XII_ENABLED(XII_PLATFORM_ANDROID)
#  include <Core/System/Implementation/Android/InputDevice_android.h>
#else
#  include <Core/System/Implementation/null/InputDevice_null.h>
#endif

#if XII_ENABLED(XII_PLATFORM_WINDOWS_DESKTOP)

#  include <Foundation/Basics/Platform/Win/MinWindows.h>
using xiiWindowHandle         = xiiMinWindows::HWND;
using xiiWindowInternalHandle = xiiWindowHandle;
#  define INVALID_WINDOW_HANDLE_VALUE (xiiWindowHandle)(0)

#elif XII_ENABLED(XII_PLATFORM_WINDOWS_UWP)

using xiiWindowHandle         = IUnknown*;
using xiiWindowInternalHandle = xiiWindowHandle;
#  define INVALID_WINDOW_HANDLE_VALUE nullptr

#elif XII_ENABLED(XII_PLATFORM_ANDROID)

extern "C"
{
  using ANativeWindow = struct ANativeWindow;
}

using xiiWindowHandle         = ANativeWindow*;
using xiiWindowInternalHandle = xiiWindowHandle;
#  define INVALID_WINDOW_HANDLE_VALUE nullptr

#else

using xiiWindowHandle         = void*;
using xiiWindowInternalHandle = xiiWindowHandle;
#  define INVALID_WINDOW_HANDLE_VALUE nullptr

#endif

#ifndef INVALID_INTERNAL_WINDOW_HANDLE_VALUE
#  define INVALID_INTERNAL_WINDOW_HANDLE_VALUE INVALID_WINDOW_HANDLE_VALUE
#endif

/// \brief Base class of all window classes that have a client area and a native window handle.
class XII_CORE_DLL xiiWindowBase
{
public:
  virtual ~xiiWindowBase() = default;

  virtual xiiSizeU32      GetClientAreaSize() const     = 0;
  virtual xiiWindowHandle GetNativeWindowHandle() const = 0;

  /// \brief Whether the window is a fullscreen window
  /// or should be one - some platforms may enforce this via the GALSwapchain)
  ///
  /// If bOnlyProperFullscreenMode, the caller accepts borderless windows that cover the entire screen as "fullscreen".
  virtual bool IsFullscreenWindow(bool bOnlyProperFullscreenMode = false) const = 0;

  virtual void ProcessWindowMessages() = 0;

  virtual void AddReference()    = 0;
  virtual void RemoveReference() = 0;
};

/// \brief Determines how the position and resolution for a window are picked
struct XII_CORE_DLL xiiWindowMode
{
  using StorageType = xiiUInt8;

  enum Enum
  {
    WindowFixedResolution,                ///< The resolution and size are what the user picked and will not be changed. The window will not be resizable.
    WindowResizable,                      ///< The resolution and size are what the user picked and will not be changed. Allows window resizing by the user.
    FullscreenBorderlessNativeResolution, ///< A borderless window, the position and resolution are taken from the monitor on which the
                                          ///< window shall appear.
    FullscreenFixedResolution,            ///< A fullscreen window using the user provided resolution. Tries to change the monitor resolution
                                          ///< accordingly.

    Default = WindowFixedResolution
  };

  /// \brief Returns whether the window covers an entire monitor. This includes borderless windows and proper fullscreen modes.
  static constexpr bool IsFullscreen(Enum e) { return e == FullscreenBorderlessNativeResolution || e == FullscreenFixedResolution; }
};

/// \brief Parameters for creating a window, such as position and resolution
struct XII_CORE_DLL xiiWindowCreationDesc
{
  /// \brief Adjusts the position and size members, depending on the current value of m_WindowMode and m_iMonitor.
  ///
  /// For windowed mode, this does nothing.
  /// For fullscreen modes, the window position is taken from the given monitor.
  /// For borderless fullscreen mode, the window resolution is also taken from the given monitor.
  ///
  /// This function can only fail if xiiScreen::EnumerateScreens fails to enumerate the available screens.
  xiiResult AdjustWindowSizeAndPosition();

  /// Serializes the configuration to DDL.
  void SaveToDDL(xiiOpenDdlWriter& ref_writer);

  /// Serializes the configuration to DDL.
  xiiResult SaveToDDL(xiiStringView sFile);

  /// Deserializes the configuration from DDL.
  void LoadFromDDL(const xiiOpenDdlReaderElement* pParentElement);

  /// Deserializes the configuration from DDL.
  xiiResult LoadFromDDL(xiiStringView sFile);


  /// The window title to be displayed.
  xiiString m_Title = "XII";

  /// Defines how the window size is determined.
  xiiEnum<xiiWindowMode> m_WindowMode;

  /// The monitor index is as given by xiiScreen::EnumerateScreens.
  /// -1 as the index means to pick the primary monitor.
  xiiInt8 m_iMonitor = -1;

  /// The virtual position of the window. Determines on which monitor the window ends up.
  xiiVec2I32 m_Position = xiiVec2I32(0x80000000, 0x80000000); // Magic number on windows that positions the window at a 'good default position'

  /// The pixel resolution of the window.
  xiiSizeU32 m_Resolution = xiiSizeU32(1280u, 720u);

  /// The number of the window. This is mostly used for setting up the input system, which then reports
  /// different mouse positions for each window.
  xiiUInt8 m_uiWindowNumber = 0;

  /// Whether the mouse cursor should be trapped inside the window or not.
  /// \see xiiStandardInputDevice::SetClipMouseCursor
  bool m_bClipMouseCursor = true;

  /// Whether the mouse cursor should be visible or not.
  /// \see xiiStandardInputDevice::SetShowMouseCursor
  bool m_bShowMouseCursor = false;

  /// Whether the window is activated and focussed on Initialize()
  bool m_bSetForegroundOnInit = true;
};

/// \brief A simple abstraction for platform specific window creation.
///
/// Will handle basic message looping. Notable events can be listened to by overriding the corresponding callbacks.
/// You should call ProcessWindowMessages every frame to keep the window responsive.
/// Input messages will not be forwarded automatically. You can do so by overriding the OnWindowMessage function.
class XII_CORE_DLL xiiWindow : public xiiWindowBase
{
public:
  /// \brief Creates empty window instance with standard settings
  ///
  /// You need to call Initialize to actually create a window.
  /// \see xiiWindow::Initialize
  xiiWindow();

  /// \brief Destroys the window if not already done.
  virtual ~xiiWindow();

  /// \brief Returns the currently active description struct.
  inline const xiiWindowCreationDesc& GetCreationDescription() const { return m_CreationDescription; }

  /// \brief Returns the size of the client area / ie. the window resolution.
  virtual xiiSizeU32 GetClientAreaSize() const override { return m_CreationDescription.m_Resolution; }

  /// \brief Returns the platform specific window handle.
  virtual xiiWindowHandle GetNativeWindowHandle() const override;

  /// \brief Returns whether the window covers an entire monitor.
  ///
  /// If bOnlyProperFullscreenMode == false, this includes borderless windows.
  virtual bool IsFullscreenWindow(bool bOnlyProperFullscreenMode = false) const override
  {
    if (bOnlyProperFullscreenMode)
      return m_CreationDescription.m_WindowMode == xiiWindowMode::FullscreenFixedResolution;

    return xiiWindowMode::IsFullscreen(m_CreationDescription.m_WindowMode);
  }

  virtual void AddReference() override { m_iReferenceCount.Increment(); }
  virtual void RemoveReference() override { m_iReferenceCount.Decrement(); }


  /// \brief Runs the platform specific message pump.
  ///
  /// You should call ProcessWindowMessages every frame to keep the window responsive.
  virtual void ProcessWindowMessages() override;

  /// \brief Creates a new platform specific window with the current settings
  ///
  /// Will automatically call xiiWindow::Destroy if window is already initialized.
  ///
  /// \see xiiWindow::Destroy, xiiWindow::Initialize
  xiiResult Initialize();

  /// \brief Creates a new platform specific window with the given settings.
  ///
  /// Will automatically call xiiWindow::Destroy if window is already initialized.
  ///
  /// \param creationDescription
  ///   Struct with various settings for window creation. Will be saved internally for later lookup.
  ///
  /// \see xiiWindow::Destroy, xiiWindow::Initialize
  xiiResult Initialize(const xiiWindowCreationDesc& creationDescription)
  {
    m_CreationDescription = creationDescription;
    return Initialize();
  }

  /// \brief Gets if the window is up and running.
  inline bool IsInitialized() const { return m_bInitialized; }

  /// \brief Destroys the window.
  xiiResult Destroy();

  /// \brief Tries to resize the window.
  /// Override OnResize to get the actual new window size.
  xiiResult Resize(const xiiSizeU32& newWindowSize);

  /// \brief Called on window resize messages.
  ///
  /// \param newWindowSize
  ///   New window size in pixel.
  /// \see OnWindowMessage
  virtual void OnResize(const xiiSizeU32& newWindowSize);

  /// \brief Called when the window position is changed. Not possible on all OSes.
  virtual void OnWindowMove(const xiiInt32 iNewPosX, const xiiInt32 iNewPosY) {}

  /// \brief Called when the window gets focus or loses focus.
  virtual void OnFocus(bool bHasFocus) {}

  /// \brief Called when the close button of the window is clicked. Does nothing by default.
  virtual void OnClickClose() {}


#if XII_ENABLED(XII_PLATFORM_WINDOWS)
  /// \brief Called on any window message.
  ///
  /// You can use this function for example to dispatch the message to another system.
  ///
  /// \remarks
  ///   Will be called <i>after</i> the On[...] callbacks!
  ///
  /// \see OnResizeMessage
  virtual void OnWindowMessage(xiiMinWindows::HWND pWnd, xiiMinWindows::UINT msg, xiiMinWindows::WPARAM wparam, xiiMinWindows::LPARAM lparam);

#elif XII_ENABLED(XII_PLATFORM_OSX)

#elif XII_ENABLED(XII_PLATFORM_LINUX)

#elif XII_ENABLED(XII_PLATFORM_ANDROID)

#else
#  error "Missing code for xiiWindow on this platform!"
#endif

  /// \brief Returns the input device that is attached to this window and typically provides mouse / keyboard input.
  xiiStandardInputDevice* GetInputDevice() const { return m_pInputDevice.Borrow(); }

  /// \brief Returns a number that can be used as a window number in xiiWindowCreationDesc
  ///
  /// This number just increments when a xiiWindow is created. It starts at zero.
  static xiiUInt8 GetNextUnusedWindowNumber();

protected:
  /// Description at creation time. xiiWindow will not update this in any method other than Initialize.
  /// \remarks That means that messages like Resize will also have no effect on this variable.
  xiiWindowCreationDesc m_CreationDescription;

private:
#if XII_ENABLED(XII_PLATFORM_ANDROID)
  static void     CommandCallback(android_app* pAndroidApp, xiiInt32 iCommand);
  static xiiInt32 InputEventCallback(android_app* pAndroidApp, AInputEvent* pInputEvent);
#endif

private:
  bool m_bInitialized = false;

  xiiUniquePtr<xiiStandardInputDevice> m_pInputDevice;

  mutable xiiWindowInternalHandle m_hWindowHandle = xiiWindowInternalHandle();

  /// Increased when a xiiWindow is created, to be able to get a free window index easily
  static xiiUInt8    s_uiNextUnusedWindowNumber;
  xiiAtomicInteger32 m_iReferenceCount = 0;
};

#pragma once

#include <Core/CoreDLL.h>

#include <Foundation/Configuration/Singleton.h>
#include <Foundation/Containers/IdTable.h>
#include <Foundation/Types/Delegate.h>
#include <Foundation/Types/Id.h>
#include <Foundation/Types/UniquePtr.h>

class xiiWindowBase;
class xiiWindowOutputTargetBase;

using xiiRegisteredWindowHandleData = xiiGenericId<16, 16>;

/// \brief Handle type for windows registered with the xiiWindowManager.
///
/// Default-constructed handles are invalid and can be checked with IsInvalidated().
/// This handle type is separate from native platform window handles (xiiWindowHandle).
class xiiRegisteredWindowHandle
{
  XII_DECLARE_HANDLE_TYPE(xiiRegisteredWindowHandle, xiiRegisteredWindowHandleData);
};

/// \brief Callback function type called when a registered window is destroyed.
using xiiWindowDestroyFunc = xiiDelegate<void(xiiRegisteredWindowHandle)>;

/// \brief Manages registered windows and their associated data.
///
/// The WindowManager provides a centralized system for managing windows throughout their lifetime.
/// Windows are registered with unique handles and can have associated output targets and destruction callbacks.
class XII_CORE_DLL xiiWindowManager final
{
  XII_DECLARE_SINGLETON(xiiWindowManager);

public:
  xiiWindowManager();
  ~xiiWindowManager();

  /// \brief Processes window messages for all registered windows.
  ///
  /// This should be called regularly (typically once per frame) to handle platform-specific window events.
  void Update();

  /// \brief Registers a new window with the manager.
  ///
  /// \param sName Human-readable name for the window (for debugging).
  /// \param pCreatedBy Pointer identifying the creator (used for bulk operations).
  /// \param pWindow The window implementation to register.
  ///
  /// \return Handle to the registered window.
  ///
  /// The returned handle remains valid until the window is explicitly closed.
  /// The pCreatedBy parameter allows closing all windows created by a specific object.
  xiiRegisteredWindowHandle Register(xiiStringView sName, const void* pCreatedBy, xiiUniquePtr<xiiWindowBase>&& pWindow);

  /// \brief Retrieves handles for all registered windows.
  ///
  /// \param out_WindowIDs Array to fill with window handles.
  /// \param pCreatedBy Optional filter to only return windows created by this object.
  void GetRegistered(xiiDynamicArray<xiiRegisteredWindowHandle>& out_windowHandles, const void* pCreatedBy = nullptr);

  /// \brief Checks if a window handle is valid and refers to an existing window.
  ///
  /// Invalid handles can occur if the window was closed or if using a default-constructed handle.
  bool IsValid(xiiRegisteredWindowHandle hWindow) const;

  /// \brief Gets the name of a registered window.
  xiiStringView GetName(xiiRegisteredWindowHandle hWindow) const;

  /// \brief Gets the window implementation for a registered window.
  xiiWindowBase* GetWindow(xiiRegisteredWindowHandle hWindow) const;

  /// \brief Sets a callback to be invoked when the window is destroyed.
  ///
  /// The callback receives the window handle as parameter. Only one callback can be set per window, setting a new callback replaces the previous one.
  void SetDestroyCallback(xiiRegisteredWindowHandle hWindow, xiiWindowDestroyFunc onDestroyCallback);

  /// \brief Associates an output target with a registered window.
  ///
  /// Output targets are destroyed before the window to ensure proper cleanup order.
  /// Setting a new output target replaces any existing one.
  void SetOutputTarget(xiiRegisteredWindowHandle hWindow, xiiUniquePtr<xiiWindowOutputTargetBase>&& pOutputTarget);

  /// \brief Gets the output target associated with a window.
  xiiWindowOutputTargetBase* GetOutputTarget(xiiRegisteredWindowHandle hWindow) const;

  /// \brief Closes and unregisters a specific window.
  ///
  /// This first calls any registered destroy callback, then destroys the output target, then the window.
  /// The handle becomes invalid after this call.
  void Close(xiiRegisteredWindowHandle hWindow);

  /// \brief Closes all windows created by a specific object.
  ///
  /// \param pCreatedBy Identifier of the creator, or nullptr to close all windows
  ///
  /// This is useful for cleanup when an object that created multiple windows is destroyed.
  void CloseAll(const void* pCreatedBy);

private:
  struct Data
  {
    xiiString                               m_sName;
    const void*                             m_pCreatedBy = nullptr;
    xiiUniquePtr<xiiWindowBase>             m_pWindow;
    xiiUniquePtr<xiiWindowOutputTargetBase> m_pOutputTarget;
    xiiWindowDestroyFunc                    m_OnDestroy;
  };

  xiiIdTable<xiiRegisteredWindowHandleData, xiiUniquePtr<Data>> m_Data;
};

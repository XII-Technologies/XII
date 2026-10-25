/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <DearImguiPlugin/DearImguiPluginDLL.h>
#include <Foundation/Math/Vec2.h>
#include <Foundation/Time/Time.h>
#include <Foundation/Types/Id.h>
#include <Foundation/Types/SharedPtr.h>
#include <Imgui/imgui.h>

using xiiImguiContextId = xiiGenericId<32, 31>;

class xiiImguiContextHandle
{
  XII_DECLARE_HANDLE_TYPE(xiiImguiContextHandle, xiiImguiContextId);
};

class xiiImguiFrame;
class xiiImguiContext;

struct xiiImguiFrameDescription
{
  xiiVec2 m_vDisplaySize      = xiiVec2::MakeZero(); ///< Logical pixels, independent of the rendering target's viewport offset.
  xiiVec2 m_vFramebufferScale = xiiVec2(1.0f);
  xiiTime m_DeltaTime         = xiiTime::MakeFromSeconds(1.0 / 60.0);
};

struct xiiImguiInputCapture
{
  bool m_bMouse    = false;
  bool m_bKeyboard = false;
  bool m_bText     = false;
};

/// All ImGui access, including raw ImGui calls, is restricted to the engine main thread.
/// A valid scope pins the context against destruction and restores the previous activation.
/// Scopes may nest, but widgets must only be built inside a successfully begun frame.
class XII_DEARIMGUIPLUGIN_DLL xiiImguiScopedContext
{
  XII_DISALLOW_COPY_AND_ASSIGN(xiiImguiScopedContext);

public:
  explicit xiiImguiScopedContext(xiiImguiContextHandle hContext);
  ~xiiImguiScopedContext();
  bool      IsValid() const { return m_pContext != nullptr; }
  xiiResult BeginFrame(const xiiImguiFrameDescription& description);
  /// Finalizes once and copies all draw data and texture references. On failure the frame is still ended.
  xiiResult            EndFrame(xiiSharedPtr<const xiiImguiFrame>& out_frame);
  xiiResult            CancelFrame();
  void                 SetInputEnabled(bool bEnabled);
  xiiImguiInputCapture GetInputCapture() const;

private:
  friend class xiiImguiInput;
  xiiImguiContext* m_pContext  = nullptr;
  ImGuiContext*    m_pPrevious = nullptr;
};

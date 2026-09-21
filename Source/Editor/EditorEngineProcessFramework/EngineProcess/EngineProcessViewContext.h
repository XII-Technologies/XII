/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Core/Graphics/Camera.h>
#include <Core/ResourceManager/ResourceHandle.h>
#include <Core/System/Window.h>
#include <EditorEngineProcessFramework/EditorEngineProcessFrameworkDLL.h>
#include <GraphicsCore/Pipeline/Declarations.h>

class xiiEngineProcessDocumentContext;
class xiiEditorEngineDocumentMsg;
class xiiViewRedrawMsgToEngine;
class xiiEditorEngineViewMsg;

using xiiRenderPipelineResourceHandle = xiiTypedResourceHandle<class xiiRenderPipelineResource>;

/// Represents the window inside the editor process, into which the engine process renders
class XII_EDITORENGINEPROCESSFRAMEWORK_DLL xiiEditorProcessViewWindow : public xiiWindowBase
{
public:
  xiiEditorProcessViewWindow()
  {
    m_hWindow  = INVALID_WINDOW_HANDLE_VALUE;
    m_uiWidth  = 0;
    m_uiHeight = 0;
  }

  ~xiiEditorProcessViewWindow();

  xiiResult UpdateWindow(xiiWindowHandle hParentWindow, xiiUInt16 uiWidth, xiiUInt16 uiHeight);

  // Inherited via xiiWindowBase
  virtual xiiSizeU32      GetClientAreaSize() const override { return xiiSizeU32(m_uiWidth, m_uiHeight); }
  virtual xiiWindowHandle GetNativeWindowHandle() const override { return m_hWindow; }
  virtual void            ProcessWindowMessages() override {}
  virtual bool            IsFullscreenWindow(bool bOnlyProperFullscreenMode = false) const override { return false; }
  virtual bool            IsVisible() const override { return true; }
  virtual void            AddReference() override { m_iReferenceCount.Increment(); }
  virtual void            RemoveReference() override { m_iReferenceCount.Decrement(); }

  xiiUInt16 m_uiWidth;
  xiiUInt16 m_uiHeight;

private:
  xiiWindowHandle    m_hWindow;
  xiiAtomicInteger32 m_iReferenceCount = 0;
};

/// Represents a view context in the engine process that is used to render a view for an editor document. It is responsible for creating a view, setting up the render target and handling view messages from the editor process.
class XII_EDITORENGINEPROCESSFRAMEWORK_DLL xiiEngineProcessViewContext
{
public:
  xiiEngineProcessViewContext(xiiEngineProcessDocumentContext* pContext);
  virtual ~xiiEngineProcessViewContext();

  void SetViewID(xiiUInt32 uiId);

  xiiEngineProcessDocumentContext* GetDocumentContext() const { return m_pDocumentContext; }

  virtual void HandleViewMessage(const xiiEditorEngineViewMsg* pMsg);
  virtual void SetupRenderTarget(xiiSharedPtr<xiiGALSwapChain> pSwapChain, const xiiRenderTargets* pRenderTargets, xiiUInt16 uiWidth, xiiUInt16 uiHeight);
  virtual void Redraw(bool bRenderEditorGizmos);

  /// Focuses camera on the given object
  static bool FocusCameraOnObject(xiiCamera& inout_camera, const xiiBoundingBoxSphere& objectBounds, float fFov, const xiiVec3& vViewDir);

  xiiViewHandle GetViewHandle() const { return m_hView; }

protected:
  void SendViewMessage(xiiEditorEngineViewMsg* pViewMsg);
  void HandleWindowUpdate(xiiWindowHandle hWnd, xiiUInt16 uiWidth, xiiUInt16 uiHeight);
  void OnSwapChainChanged(xiiSharedPtr<xiiGALSwapChain> pSwapChain, xiiSizeU32 size);

  virtual void SetCamera(const xiiViewRedrawMsgToEngine* pMsg);

  /// Create the actual view.
  virtual xiiViewHandle CreateView() = 0;

protected:
  xiiCamera     m_Camera;
  xiiViewHandle m_hView;
  xiiUInt32     m_uiViewID;

private:
  xiiRegisteredWindowHandle        m_hEditorWindow;
  xiiEngineProcessDocumentContext* m_pDocumentContext;
};

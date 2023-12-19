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
class xiiGALRenderTargetSetup;
class xiiActor;
struct xiiGALRenderTargets;

using xiiRenderPipelineResourceHandle = xiiTypedResourceHandle<class xiiRenderPipelineResource>;

/// \brief Represents the window inside the editor process, into which the engine process renders
class XII_EDITORENGINEPROCESSFRAMEWORK_DLL xiiEditorProcessViewWindow : public xiiWindowBase
{
public:
  xiiEditorProcessViewWindow()
  {
    m_hWnd     = INVALID_WINDOW_HANDLE_VALUE;
    m_uiWidth  = 0;
    m_uiHeight = 0;
  }

  ~xiiEditorProcessViewWindow();

  xiiResult UpdateWindow(xiiWindowHandle hParentWindow, xiiUInt16 uiWidth, xiiUInt16 uiHeight);

  // Inherited via xiiWindowBase
  virtual xiiSizeU32      GetClientAreaSize() const override { return xiiSizeU32(m_uiWidth, m_uiHeight); }
  virtual xiiWindowHandle GetNativeWindowHandle() const override { return m_hWnd; }
  virtual void            ProcessWindowMessages() override {}
  virtual bool            IsFullscreenWindow(bool bOnlyProperFullscreenMode = false) const override { return false; }
  virtual void            AddReference() override { m_iReferenceCount.Increment(); }
  virtual void            RemoveReference() override { m_iReferenceCount.Decrement(); }


  xiiUInt16 m_uiWidth;
  xiiUInt16 m_uiHeight;

private:
  xiiWindowHandle    m_hWnd;
  xiiAtomicInteger32 m_iReferenceCount = 0;
};

/// \brief Represents the view/window on the engine process side, holds all data necessary for rendering
class XII_EDITORENGINEPROCESSFRAMEWORK_DLL xiiEngineProcessViewContext
{
public:
  xiiEngineProcessViewContext(xiiEngineProcessDocumentContext* pContext);
  virtual ~xiiEngineProcessViewContext();

  void SetViewID(xiiUInt32 uiId);

  xiiEngineProcessDocumentContext* GetDocumentContext() const { return m_pDocumentContext; }

  virtual void HandleViewMessage(const xiiEditorEngineViewMsg* pMsg);
  virtual void SetupRenderTarget(xiiGALSwapChainHandle hSwapChain, const xiiGALRenderTargets* pRenderTargets, xiiUInt16 uiWidth, xiiUInt16 uiHeight);
  virtual void Redraw(bool bRenderEditorGizmos);

  /// \brief Focuses camera on the given object
  static bool FocusCameraOnObject(xiiCamera& inout_camera, const xiiBoundingBoxSphere& objectBounds, float fFov, const xiiVec3& vViewDir);

  xiiViewHandle GetViewHandle() const { return m_hView; }

  void DrawSimpleGrid() const;

protected:
  void SendViewMessage(xiiEditorEngineViewMsg* pViewMsg);
  void HandleWindowUpdate(xiiWindowHandle hWnd, xiiUInt16 uiWidth, xiiUInt16 uiHeight);
  void OnSwapChainChanged(xiiGALSwapChainHandle hSwapChain, xiiSizeU32 size);

  virtual void SetCamera(const xiiViewRedrawMsgToEngine* pMsg);

  /// \brief Returns the handle to the default render pipeline.
  virtual xiiRenderPipelineResourceHandle CreateDefaultRenderPipeline();

  /// \brief Returns the handle to the debug render pipeline.
  virtual xiiRenderPipelineResourceHandle CreateDebugRenderPipeline();

  /// \brief Create the actual view.
  virtual xiiViewHandle CreateView() = 0;

private:
  xiiEngineProcessDocumentContext* m_pDocumentContext;
  xiiActor*                        m_pEditorWndActor = nullptr;

protected:
  xiiCamera     m_Camera;
  xiiViewHandle m_hView;
  xiiUInt32     m_uiViewID;
};

/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <EditorEngineProcessFramework/EditorEngineProcessFrameworkDLL.h>

#include <Core/Graphics/Camera.h>
#include <Core/System/Window.h>
#include <Core/System/WindowManager.h>
#include <EditorEngineProcessFramework/Rendering/EditorGrid.h>
#include <GraphicsCore/Pipeline/Declarations.h>
#include <GraphicsCore/Pipeline/RenderPipelineProfileConfig.h>

/// Engine-owned view purposes; only Main uses the asset profile's pipeline selection.
enum class xiiEditorRenderPipelineUsage
{
  Main,
  EditorScene,
  TexturePreview,
  CubeTexturePreview
};

class xiiEngineProcessDocumentContext;
class xiiEditorEngineDocumentMsg;
class xiiViewRedrawMsgToEngine;
class xiiViewPickingResultMsgToEditor;
class xiiEditorEngineViewMsg;
class xiiView;
class xiiGALSwapChain;
class xiiGALTextureView;

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
  virtual xiiSizeU32         GetClientAreaSize() const override { return xiiSizeU32(m_uiWidth, m_uiHeight); }
  virtual xiiWindowHandle    GetNativeWindowHandle() const override { return m_hWindow; }
  virtual void               ProcessWindowMessages() override {}
  virtual bool               IsFullscreenWindow(bool bOnlyProperFullscreenMode = false) const override { return false; }
  virtual bool               IsVisible() const override { return true; }
  virtual void               AddReference() override { m_iReferenceCount.Increment(); }
  virtual void               RemoveReference() override { m_iReferenceCount.Decrement(); }
  void                       SetOutputTarget(xiiUniquePtr<xiiWindowOutputTargetBase>&& pOutputTarget) override { m_pOutputTarget = std::move(pOutputTarget); }
  xiiWindowOutputTargetBase* GetOutputTarget() const override { return m_pOutputTarget.Borrow(); }

  xiiUInt16 m_uiWidth;
  xiiUInt16 m_uiHeight;

private:
  xiiWindowHandle                         m_hWindow;
  xiiAtomicInteger32                      m_iReferenceCount = 0;
  xiiUniquePtr<xiiWindowOutputTargetBase> m_pOutputTarget;
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
  virtual void SetupRenderTarget(xiiSharedPtr<xiiGALSwapChain> pSwapChain, xiiGALTextureView* pRenderTargetView, xiiUInt16 uiWidth, xiiUInt16 uiHeight);
  virtual void Redraw(bool bRenderEditorGizmos);

  /// Consumes this frame's redraw request without discarding camera or pipeline state.
  virtual void SuspendRendering();

  xiiRegisteredWindowHandle GetWindowHandle() const { return m_hEditorWindow; }

  /// Focuses camera on the given object
  static bool FocusCameraOnObject(xiiCamera& inout_camera, const xiiBoundingBoxSphere& objectBounds, float fFov, const xiiVec3& vViewDir);

  xiiViewHandle GetViewHandle() const { return m_hView; }

  /// The profile is application-owned and must outlive the context. An explicit resource
  /// reference overrides its policy without modifying the shared resource definition.
  void SetRenderPipelineProfile(const xiiPlatformProfile* profile) { m_pPipelineProfile = profile; }
  void SetRenderPipelineResourceReference(xiiStringView resource) { m_sPipelineReference = resource; }

protected:
  /// Resolves the view through its owning document world. The caller must hold the world's access marker
  /// for the entire use of the returned non-owning pointer.
  xiiView*  GetView() const;
  xiiResult ResolveRenderPipeline(xiiEditorRenderPipelineUsage usage, xiiRenderPipelineResourceHandle& out_pipeline);
  void      UpdateRenderPipeline(xiiView& view);
  /// Submits reusable grid geometry to this view's regular debug-render passes.
  xiiResult DrawGrid(float fSpacing, const xiiTransform& transform = xiiTransform::MakeIdentity(), bool bGlobal = true);
  /// Picks synchronized handles at a window pixel center, accounting for viewport offsets.
  /// Clears all hit fields on failure; document and view routing fields remain unchanged.
  /// Gizmos do not report a surface normal. The caller sends or combines the result.
  xiiResult PickGizmoAtPixel(xiiUInt32 x, xiiUInt32 y, xiiViewPickingResultMsgToEditor& out_result) const;
  void      SendViewMessage(xiiEditorEngineViewMsg* pViewMsg);
  void      HandleWindowUpdate(xiiWindowHandle hWnd, xiiUInt16 uiWidth, xiiUInt16 uiHeight);
  void      OnSwapChainChanged(xiiSharedPtr<xiiGALSwapChain> pSwapChain, xiiSizeU32 size);

  virtual void SetCamera(const xiiViewRedrawMsgToEngine* pMsg);

  /// Create the actual view.
  virtual xiiViewHandle CreateView() = 0;

protected:
  xiiCamera                 m_Camera;
  xiiViewHandle             m_hView;
  xiiUInt32                 m_uiViewID;
  xiiRegisteredWindowHandle m_hEditorWindow;

private:
  xiiEngineProcessDocumentContext* m_pDocumentContext;
  xiiEditorGrid                    m_Grid;
  const xiiPlatformProfile*        m_pPipelineProfile = nullptr;
  xiiString                        m_sPipelineReference;
  xiiEditorRenderPipelineUsage     m_PipelineUsage           = xiiEditorRenderPipelineUsage::Main;
  bool                             m_bPipelinePolicySelected = false;
};

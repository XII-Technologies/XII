/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once
#include <EditorFramework/EditorFrameworkDLL.h>

#include <EditorEngineProcessFramework/EngineProcess/EngineProcessMessages.h>
#include <Foundation/Types/Delegate.h>
#include <GraphicsFoundation/Device/Device.h>
#include <GraphicsFoundation/Device/SwapChain.h>
#include <QWidget>

/// Qt owns the native window and input. GAL owns every graphics resource and operation.
class XII_EDITORFRAMEWORK_DLL xiiQtSharedTextureWidget : public QWidget
{
public:
  explicit xiiQtSharedTextureWidget(QWidget* pParent);
  ~xiiQtSharedTextureWidget();

  void                                         SetVSyncEnabled(bool bEnabled) { m_bVSync = bEnabled; }
  bool                                         SupportsSharedTextures();
  void                                         SetFrame(const xiiViewFrameMsgToEditor& frame);
  xiiDelegate<void(xiiUInt8, xiiUInt64, bool)> m_OnFrameConsumed;

protected:
  void          paintEvent(QPaintEvent*) override;
  QPaintEngine* paintEngine() const override { return nullptr; }
  bool          eventFilter(QObject* object, QEvent* event) override;
  bool          event(QEvent* event) override;

private:
  xiiResult EnsureDevice();
  xiiResult EnsureSwapChain();
  xiiResult PresentTexture();
  void      DestroySwapChain();

  static xiiSharedPtr<xiiGALDevice> s_pDevice;
  static xiiUInt32                  s_uiWidgetCount;
  xiiSharedPtr<xiiGALSwapChain>     m_pSwapChain;
  struct ImportedSlot
  {
    xiiSharedPtr<xiiGALTexture> m_pTexture;
    xiiUInt64                   m_uiTextureID = 0;
  };
  ImportedSlot                m_ImportedSlots[XII_MAX_VIEWPORT_FRAME_SLOTS];
  xiiSharedPtr<xiiGALTexture> m_pPresentationTexture;
  bool                        m_bVSync           = true;
  bool                        m_bSharingFailed   = false;
  bool                        m_bCreatingSurface = false;
};

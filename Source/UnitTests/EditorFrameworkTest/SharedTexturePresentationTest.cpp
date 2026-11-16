/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorFrameworkTest/EditorFrameworkTestPCH.h>

#include <EditorEngineProcessFramework/EngineProcess/EngineProcessMessages.h>
#include <EditorFramework/DocumentWindow/SharedTextureWidget.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Types/ScopeExit.h>
#include <GraphicsFoundation/CommandEncoder/CommandList.h>
#include <GraphicsFoundation/CommandEncoder/CommandQueue.h>
#include <GraphicsFoundation/Device/DeviceFactory.h>
#include <QApplication>
#include <QCoreApplication>
#include <QElapsedTimer>
#include <QImage>
#include <QLabel>
#include <QPixmap>
#include <QScreen>
#include <QThread>
#include <QVBoxLayout>
#include <QWidget>
#include <QWindow>
#include <Qt>
#include <TestFramework/Utilities/TestLogInterface.h>
#include <ads/AutoHideDockContainer.h>
#include <ads/DockManager.h>
#include <ads/DockWidget.h>

XII_CREATE_SIMPLE_TEST_GROUP(EditorDockViewportStacking);
XII_CREATE_SIMPLE_TEST(EditorDockViewportStacking, AutoHideAboveNativeViewport)
{
#if XII_DISABLED(XII_PLATFORM_LINUX)
  return;
#endif

  auto*            pEditor      = xiiQtEditorApp::GetSingleton();
  xiiStringBuilder sPreferences = xiiTestFramework::GetInstance()->GetAbsOutputPath();
  sPreferences.AppendPath("Preferences");
  XII_TEST_RESULT(xiiOSFile::CreateDirectoryStructure(sPreferences));
  pEditor->StartupEditor(xiiQtEditorApp::StartupFlags::UnitTest | xiiQtEditorApp::StartupFlags::NoRecent | xiiQtEditorApp::StartupFlags::SafeMode, sPreferences);
  XII_SCOPE_EXIT(pEditor->ShutdownEditor());

  XII_TEST_BOOL(QCoreApplication::testAttribute(Qt::AA_DontCreateNativeWidgetSiblings));
  const auto previousFlags = ads::CDockManager::autoHideConfigFlags();
  ads::CDockManager::setAutoHideConfigFlags(previousFlags | ads::CDockManager::AutoHideFeatureEnabled);
  XII_SCOPE_EXIT(ads::CDockManager::setAutoHideConfigFlags(previousFlags));

  QWidget window;
  window.resize(640, 480);
  auto* pLayout  = new QVBoxLayout(&window);
  auto* pManager = new ads::CDockManager(&window);
  pLayout->addWidget(pManager);
  auto* pCentral = new ads::CDockWidget(pManager, "Viewport");
  auto* pContent = new QWidget;
  pCentral->setWidget(pContent);
  pManager->setCentralWidget(pCentral);
  auto* pContentLayout = new QVBoxLayout(pContent);
  auto* pViewport      = new xiiQtSharedTextureWidget(pContent);
  pContentLayout->addWidget(pViewport);
  auto* pPanel = new ads::CDockWidget(pManager, "Auto-hide panel");
  pPanel->setWidget(new QLabel(QStringLiteral("Panel content")));
  auto* pDrawer = pManager->addAutoHideDockWidget(ads::SideBarRight, pPanel);

  window.show();
  QApplication::processEvents();
  XII_TEST_BOOL(pViewport->winId() != 0);
  XII_TEST_BOOL(pManager->internalWinId() == 0);

  xiiViewFrameMsgToEditor frame;
  frame.m_uiFrameID = 1U;
  frame.m_uiWidth   = 2U;
  frame.m_uiHeight  = 2U;
  frame.m_Pixels.SetCount(16U, 255U);
  bool bConsumed               = false;
  pViewport->m_OnFrameConsumed = [&](xiiUInt8, xiiUInt64, bool) {
    bConsumed = true;
  };
  pViewport->SetVSyncEnabled(false);
  pViewport->SetFrame(frame);
  XII_TEST_BOOL(bConsumed);
  QElapsedTimer timer;
  timer.start();
  while (timer.elapsed() < 50)
  {
    QApplication::processEvents();
    QThread::msleep(2);
  }

  for (xiiUInt32 i = 0U; i < 2U; ++i)
  {
    pDrawer->collapseView(false);
    QApplication::processEvents();
    XII_TEST_BOOL(pDrawer->isVisible());
    XII_TEST_BOOL(pDrawer->internalWinId() != 0);
    XII_TEST_BOOL(pManager->internalWinId() == 0);
    if (XII_TEST_BOOL(pDrawer->windowHandle() != nullptr))
    {
      XII_TEST_BOOL(pDrawer->windowHandle()->parent() == pViewport->windowHandle()->parent());
    }
    pDrawer->collapseView(true);
    QApplication::processEvents();
    XII_TEST_BOOL(!pDrawer->isVisible());
    bConsumed = false;
    ++frame.m_uiFrameID;
    pViewport->SetFrame(frame);
    XII_TEST_BOOL(bConsumed);
  }

  QWidget reparentHost;
  reparentHost.resize(320, 240);
  reparentHost.show();
  pViewport->setParent(&reparentHost);
  pViewport->resize(320, 240);
  pViewport->show();
  QApplication::processEvents();
  bConsumed = false;
  ++frame.m_uiFrameID;
  pViewport->SetFrame(frame);
  XII_TEST_BOOL(bConsumed);

  pViewport->setParent(pContent);
  pContentLayout->addWidget(pViewport);
  pViewport->show();
  QApplication::processEvents();
  bConsumed = false;
  ++frame.m_uiFrameID;
  pViewport->SetFrame(frame);
  XII_TEST_BOOL(bConsumed);
  pViewport->m_OnFrameConsumed.Invalidate();
}

XII_CREATE_SIMPLE_TEST_GROUP(EditorViewportPresentation);
XII_CREATE_SIMPLE_TEST(EditorViewportPresentation, NativeChildIsolation)
{
  QWidget window;
  window.resize(400, 300);
  QWidget panel(&window);
  panel.setGeometry(20, 30, 300, 200);
  xiiQtSharedTextureWidget viewport(&panel);
  viewport.setGeometry(10, 15, 160, 120);

  XII_TEST_BOOL(viewport.testAttribute(Qt::WA_DontCreateNativeAncestors));
  XII_TEST_BOOL(viewport.testAttribute(Qt::WA_TransparentForMouseEvents));
  XII_TEST_BOOL(!panel.testAttribute(Qt::WA_NativeWindow));
  window.show();
  XII_TEST_BOOL(viewport.winId() != 0);
  QApplication::processEvents();
  XII_TEST_BOOL(viewport.testAttribute(Qt::WA_NativeWindow));
  XII_TEST_BOOL(!panel.testAttribute(Qt::WA_NativeWindow));
  XII_TEST_BOOL(panel.windowHandle() == nullptr);
  XII_TEST_BOOL(viewport.window() == &window);
  XII_TEST_BOOL(panel.childAt(viewport.geometry().center()) == nullptr);
}

XII_CREATE_SIMPLE_TEST(EditorViewportPresentation, SharedTexture)
{
  const auto platform = QGuiApplication::platformName();
  if (platform != "windows" && platform != "xcb" && !platform.startsWith("wayland"))
  {
    return;
  }

  // XWayland and Wayland cannot capture a Vulkan window through QScreen.
  const bool       bCanCapturePixels = platform == "windows" || (platform == "xcb" && qgetenv("XDG_SESSION_TYPE") != "wayland");
  auto*            editor            = xiiQtEditorApp::GetSingleton();
  xiiStringBuilder preferences       = xiiTestFramework::GetInstance()->GetAbsOutputPath();
  preferences.AppendPath("Preferences");
  XII_TEST_RESULT(xiiOSFile::CreateDirectoryStructure(preferences));
  editor->StartupEditor(xiiQtEditorApp::StartupFlags::UnitTest | xiiQtEditorApp::StartupFlags::NoRecent | xiiQtEditorApp::StartupFlags::SafeMode, preferences);
  XII_SCOPE_EXIT(editor->ShutdownEditor());
  xiiGALDeviceCreationDescription deviceDescription;
  deviceDescription.m_GraphicsDeviceType                 = xiiGALGraphicsDeviceType::Vulkan;
  deviceDescription.m_ValidationLevel                    = xiiGALDeviceValidationLevel::Standard;
  deviceDescription.m_DeviceFeatures.m_NativeFence       = xiiGALDeviceFeatureState::Optional;
  deviceDescription.m_DeviceFeatures.m_ExternalMemory    = xiiGALDeviceFeatureState::Optional;
  deviceDescription.m_DeviceFeatures.m_ExternalSemaphore = xiiGALDeviceFeatureState::Optional;
  auto producer                                          = xiiGALDeviceFactory::CreateDevice("Vulkan", xiiFoundation::GetDefaultAllocator(), deviceDescription);
  if (!XII_TEST_BOOL(producer != nullptr) || !XII_TEST_RESULT(producer->Initialize()))
    return;
  xiiUniquePtr<xiiQtSharedTextureWidget> widgets[2];
  for (xiiUInt32 i = 0; i < 2; ++i)
  {
    widgets[i] = XII_DEFAULT_NEW(xiiQtSharedTextureWidget, nullptr);
    widgets[i]->setGeometry(80 + i * 240, 80, 128, 96);
    widgets[i]->show();
  }
  QApplication::processEvents();
  const auto checkPresented = [&](xiiQtSharedTextureWidget& widget, xiiUInt64 uiFrameID, QColor expected) {
    if (bCanCapturePixels)
    {
      widget.raise();
      widget.activateWindow();
    }
    QElapsedTimer timer;
    timer.start();
    QImage image;
    while (timer.elapsed() < 10000)
    {
      QApplication::processEvents();
      widget.repaint();
      if (widget.GetLastSubmittedFrameID() == uiFrameID)
      {
        if (!bCanCapturePixels)
        {
          return true;
        }
        const auto position = widget.mapToGlobal(QPoint(0, 0));
        image               = widget.screen()->grabWindow(0, position.x(), position.y(), widget.width(), widget.height()).toImage();
        if (!image.isNull() && image.pixelColor(image.width() / 2, image.height() / 2) == expected)
        {
          return true;
        }
      }
      QThread::msleep(5);
    }
    if (!image.isNull())
      xiiLog::Info("Presented pixel: {}, {}, {}; expected: {}, {}, {}.", image.pixelColor(image.width() / 2, image.height() / 2).red(), image.pixelColor(image.width() / 2, image.height() / 2).green(), image.pixelColor(image.width() / 2, image.height() / 2).blue(), expected.red(), expected.green(), expected.blue());
    return false;
  };
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Readback fallback uploads and presents through GAL")
  {
    xiiViewFrameMsgToEditor frame;
    frame.m_uiFrameID = 1U;
    frame.m_uiWidth   = 2;
    frame.m_uiHeight  = 2;
    frame.m_Pixels.SetCount(16);
    for (xiiUInt32 i = 0; i < 4; ++i)
    {
      frame.m_Pixels[i * 4]     = 255;
      frame.m_Pixels[i * 4 + 1] = 0;
      frame.m_Pixels[i * 4 + 2] = 0;
      frame.m_Pixels[i * 4 + 3] = 255;
    }
    bool bConsumed                = false;
    widgets[0]->m_OnFrameConsumed = [&](xiiUInt8, xiiUInt64, bool bShared) {
      XII_TEST_BOOL(!bShared);
      bConsumed = true;
    };
    widgets[0]->SetVSyncEnabled(false);
    widgets[0]->SetFrame(frame);
    XII_TEST_BOOL(bConsumed);
    XII_TEST_BOOL(checkPresented(*widgets[0], frame.m_uiFrameID, QColor(Qt::red)));

    bConsumed = false;
    ++frame.m_uiFrameID;
    for (xiiUInt32 uiPixel = 0; uiPixel < 4; ++uiPixel)
    {
      frame.m_Pixels[uiPixel * 4]     = 0;
      frame.m_Pixels[uiPixel * 4 + 2] = 255;
    }
    widgets[0]->SetVSyncEnabled(true);
    widgets[0]->SetFrame(frame);
    XII_TEST_BOOL(bConsumed);
    XII_TEST_BOOL(checkPresented(*widgets[0], frame.m_uiFrameID, QColor(Qt::blue)));
    widgets[0]->m_OnFrameConsumed.Invalidate();
  }
  if (!widgets[0]->SupportsSharedTextures() || producer->GetFeatures().m_NativeFence != xiiGALDeviceFeatureState::Enabled || producer->GetFeatures().m_ExternalMemory != xiiGALDeviceFeatureState::Enabled || producer->GetFeatures().m_ExternalSemaphore != xiiGALDeviceFeatureState::Enabled)
    return;
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Two viewports return several independent allocations per batch")
  {
    xiiGALTextureCreationDescription description;
    description.m_Type                   = xiiGALResourceDimension::Texture2D;
    description.m_Size                   = {64, 48};
    description.m_Format                 = xiiGALResourceFormat::RGBA8UNormalizedSRGB;
    description.m_BindFlags              = xiiGALBindFlags::RenderTarget | xiiGALBindFlags::ShaderResource;
    description.m_ExternalMemory.m_Flags = xiiGALExternalMemoryFlags::BinarySemaphore;
    xiiSharedPtr<xiiGALTexture> pTextures[2][XII_MAX_VIEWPORT_FRAME_SLOTS];
    for (xiiUInt8 uiBatch = 0; uiBatch < 3; ++uiBatch)
    {
      // Replace the last slot on the final batch, preserving the identities of the others.
      if (uiBatch == 2)
      {
        description.m_Size = {80, 60};
      }
      xiiUInt32       uiReturned[2]  = {};
      const xiiUInt64 uiFirstFrameID = 3U + uiBatch * XII_MAX_VIEWPORT_FRAME_SLOTS;
      for (xiiUInt8 uiView = 0; uiView < 2; ++uiView)
      {
        widgets[uiView]->m_OnFrameConsumed = [&, uiView](xiiUInt8 uiSlot, xiiUInt64 uiFrameID, bool bShared) {
          XII_TEST_INT(uiFrameID, uiFirstFrameID + uiSlot);
          XII_TEST_BOOL(bShared);
          XII_TEST_BOOL((uiReturned[uiView] & XII_BIT(uiSlot)) == 0);
          uiReturned[uiView] |= XII_BIT(uiSlot);
        };
        widgets[uiView]->SetVSyncEnabled(uiView == 0);
      }

      auto  pCommand = producer->CreateCommandList({.m_QueueFlags = xiiGALCommandQueueFlags::Graphics});
      auto* pQueue   = producer->GetCommandQueue();
      for (xiiUInt8 uiSlot = 0; uiSlot < XII_MAX_VIEWPORT_FRAME_SLOTS; ++uiSlot)
      {
        for (xiiUInt8 uiView = 0; uiView < 2; ++uiView)
        {
          auto&      pTexture = pTextures[uiView][uiSlot];
          const bool bReplace = uiBatch == 2 && uiSlot == XII_MAX_VIEWPORT_FRAME_SLOTS - 1;
          if (uiBatch == 0 || bReplace)
          {
            pTexture = producer->CreateTexture(description, nullptr, xiiGALExternalMemoryKind::Exportable);
            if (!XII_TEST_BOOL(pTexture != nullptr))
            {
              return;
            }
          }

          const auto color = (uiBatch + uiView + uiSlot) % 2 != 0 ? xiiColor::Blue : xiiColor::Red;
          pCommand->Begin();
          pCommand->ClearRenderTargetView(pTexture->GetDefaultView(xiiGALTextureViewType::RenderTarget), color);
          XII_TEST_RESULT(pTexture->ReleaseExternalMemory(pCommand));
          pCommand->End();
          pQueue->WaitForFenceValue(pQueue->Submit(pCommand));

          xiiViewFrameMsgToEditor message;
          message.m_uiSlotIndex = uiSlot;
          message.m_uiFrameID   = uiFirstFrameID + uiSlot;
          message.m_uiTextureID = bReplace ? 4U : uiSlot + 1U;
          message.m_uiWidth     = static_cast<xiiUInt16>(pTexture->GetDescription().m_Size.width);
          message.m_uiHeight    = static_cast<xiiUInt16>(pTexture->GetDescription().m_Size.height);
          message.m_Memory      = pTexture->GetExternalMemoryDescription();
          if (uiBatch != 0 && !bReplace)
          {
            // A cached allocation can be consumed without reopening its native handles.
            message.m_Memory.m_uiNativeHandle = 0;
          }
          widgets[uiView]->SetFrame(message);
        }
      }

      QElapsedTimer timer;
      timer.start();
      const xiiUInt32 uiAllSlots = XII_BIT(XII_MAX_VIEWPORT_FRAME_SLOTS) - 1U;
      while ((uiReturned[0] != uiAllSlots || uiReturned[1] != uiAllSlots) && timer.elapsed() < 5000)
      {
        QApplication::processEvents();
        QThread::msleep(2);
      }

      for (xiiUInt8 uiView = 0; uiView < 2; ++uiView)
      {
        widgets[uiView]->m_OnFrameConsumed.Invalidate();
        if (!XII_TEST_INT(uiReturned[uiView], uiAllSlots))
        {
          return;
        }
        widgets[uiView]->resize(128 + uiBatch * 8, 96 + uiBatch * 4);
        const auto color = (uiBatch + uiView) % 2 != 0 ? QColor(Qt::blue) : QColor(Qt::red);
        XII_TEST_BOOL(checkPresented(*widgets[uiView], uiFirstFrameID + XII_MAX_VIEWPORT_FRAME_SLOTS - 1, color));
        for (auto& pTexture : pTextures[uiView])
        {
          pCommand->Begin();
          XII_TEST_RESULT(pTexture->AcquireExternalMemory(pCommand));
          pCommand->End();
          pQueue->WaitForFenceValue(pQueue->Submit(pCommand));
        }
      }
    }

    {
      xiiViewFrameMsgToEditor rejected;
      rejected.m_uiFrameID           = 12;
      rejected.m_uiTextureID         = 5;
      rejected.m_uiWidth             = 64;
      rejected.m_uiHeight            = 48;
      rejected.m_Memory              = pTextures[0][0]->GetExternalMemoryDescription();
      rejected.m_Memory.m_DriverUUID = {};
      bool bRejected                 = false;
      widgets[0]->m_OnFrameConsumed  = [&](xiiUInt8, xiiUInt64 uiFrameID, bool bShared) {
        if (uiFrameID == rejected.m_uiFrameID)
        {
          bRejected = !bShared;
        }
      };
      {
        xiiTestLogInterface   log;
        xiiTestLogSystemScope scope(&log, true);
        log.ExpectMessage("External Vulkan memory requires the exporting physical device and driver.", xiiLogMsgType::ErrorMsg);
        log.ExpectMessage("GAL viewport texture import unavailable", xiiLogMsgType::WarningMsg);
        widgets[0]->SetFrame(rejected);
      }
      XII_TEST_BOOL(bRejected);
      XII_TEST_BOOL(!widgets[0]->SupportsSharedTextures());

      xiiViewFrameMsgToEditor fallback;
      fallback.m_uiFrameID = 13;
      fallback.m_uiWidth   = 2;
      fallback.m_uiHeight  = 2;
      fallback.m_Pixels.SetCount(16, 255);
      widgets[0]->SetFrame(fallback);
      XII_TEST_BOOL(checkPresented(*widgets[0], fallback.m_uiFrameID, QColor(Qt::white)));
      widgets[0]->m_OnFrameConsumed.Invalidate();
    }
  }
}

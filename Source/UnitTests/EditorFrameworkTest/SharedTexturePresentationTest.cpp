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
#include <QElapsedTimer>
#include <QImage>
#include <QPixmap>
#include <QScreen>
#include <QThread>

XII_CREATE_SIMPLE_TEST_GROUP(EditorViewportPresentation);
XII_CREATE_SIMPLE_TEST(EditorViewportPresentation, SharedTexture)
{
  if (QGuiApplication::platformName() != "windows" && QGuiApplication::platformName() != "xcb")
    return; // These native platforms permit capturing the presented window for pixel verification.
  auto*            editor      = xiiQtEditorApp::GetSingleton();
  xiiStringBuilder preferences = xiiTestFramework::GetInstance()->GetAbsOutputPath();
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
  const auto checkPresented = [&](xiiQtSharedTextureWidget& widget, QColor expected) {
    widget.raise();
    widget.activateWindow();
    QElapsedTimer timer;
    timer.start();
    QImage image;
    while (timer.elapsed() < 10000)
    {
      QApplication::processEvents();
      widget.repaint();
      const auto position = widget.mapToGlobal(QPoint(0, 0));
      image               = widget.screen()->grabWindow(0, position.x(), position.y(), widget.width(), widget.height()).toImage();
      if (!image.isNull() && image.pixelColor(image.width() / 2, image.height() / 2) == expected)
        return true;
      QThread::msleep(5);
    }
    if (!image.isNull())
      xiiLog::Info("Presented pixel: {}, {}, {}; expected: {}, {}, {}.", image.pixelColor(image.width() / 2, image.height() / 2).red(), image.pixelColor(image.width() / 2, image.height() / 2).green(), image.pixelColor(image.width() / 2, image.height() / 2).blue(), expected.red(), expected.green(), expected.blue());
    return false;
  };
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Readback fallback uploads and presents through GAL")
  {
    xiiViewFrameMsgToEditor frame;
    frame.m_uiWidth  = 2;
    frame.m_uiHeight = 2;
    frame.m_Pixels.SetCount(16);
    for (xiiUInt32 i = 0; i < 4; ++i)
    {
      frame.m_Pixels[i * 4]     = 255;
      frame.m_Pixels[i * 4 + 1] = 0;
      frame.m_Pixels[i * 4 + 2] = 0;
      frame.m_Pixels[i * 4 + 3] = 255;
    }
    bool consumed                 = false;
    widgets[0]->m_OnFrameConsumed = [&](xiiUInt64, bool shared) {
      XII_TEST_BOOL(!shared);
      consumed = true;
    };
    widgets[0]->SetVSyncEnabled(false);
    widgets[0]->SetFrame(frame);
    widgets[0]->SetVSyncEnabled(true);
    widgets[0]->SetFrame(frame);
    XII_TEST_BOOL(consumed);
    XII_TEST_BOOL(checkPresented(*widgets[0], QColor(Qt::red)));
    widgets[0]->m_OnFrameConsumed.Invalidate();
  }
  if (!widgets[0]->SupportsSharedTextures() || producer->GetFeatures().m_NativeFence != xiiGALDeviceFeatureState::Enabled || producer->GetFeatures().m_ExternalMemory != xiiGALDeviceFeatureState::Enabled || producer->GetFeatures().m_ExternalSemaphore != xiiGALDeviceFeatureState::Enabled)
    return;
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Two viewports import, present, resize and return independent textures")
  {
    xiiGALTextureCreationDescription description;
    description.m_Type                   = xiiGALResourceDimension::Texture2D;
    description.m_Size                   = {64, 48};
    description.m_Format                 = xiiGALResourceFormat::RGBA8UNormalizedSRGB;
    description.m_BindFlags              = xiiGALBindFlags::RenderTarget | xiiGALBindFlags::ShaderResource;
    description.m_ExternalMemory.m_Flags = xiiGALExternalMemoryFlags::BinarySemaphore;
    xiiSharedPtr<xiiGALTexture> textures[2];
    for (auto& texture : textures)
    {
      texture = producer->CreateTexture(description, nullptr, xiiGALExternalMemoryKind::Exportable);
      if (!XII_TEST_BOOL(texture != nullptr))
        return;
    }
    auto command = producer->CreateCommandList({.m_QueueFlags = xiiGALCommandQueueFlags::Graphics});
    for (xiiUInt32 frame = 0; frame < 6; ++frame)
      for (xiiUInt32 i = 0; i < 2; ++i)
      {
        command->Begin();
        command->ClearRenderTargetView(textures[i]->GetDefaultView(xiiGALTextureViewType::RenderTarget), (frame + i) % 2 ? xiiColor::Blue : xiiColor::Red);
        XII_TEST_RESULT(textures[i]->ReleaseExternalMemory(command));
        command->End();
        producer->GetCommandQueue()->Submit(command);
        xiiViewFrameMsgToEditor message;
        message.m_uiFrameID           = frame + 1;
        message.m_uiTextureID         = 1;
        message.m_uiWidth             = 64;
        message.m_uiHeight            = 48;
        message.m_Memory              = textures[i]->GetExternalMemoryDescription();
        bool accepted                 = false;
        widgets[i]->m_OnFrameConsumed = [&](xiiUInt64 id, bool shared) {
          XII_TEST_INT(id, frame + 1);
          accepted = shared;
        };
        widgets[i]->SetFrame(message);
        if (!XII_TEST_BOOL(accepted))
          return;
        XII_TEST_BOOL(checkPresented(*widgets[i], (frame + i) % 2 ? QColor(Qt::blue) : QColor(Qt::red)));
        command->Begin();
        XII_TEST_RESULT(textures[i]->AcquireExternalMemory(command));
        command->End();
        producer->GetCommandQueue()->WaitForFenceValue(producer->GetCommandQueue()->Submit(command));
        widgets[i]->resize(128 + frame * 8, 96 + frame * 4);
        widgets[i]->m_OnFrameConsumed.Invalidate();
      }
    producer->WaitIdle();
  }
}

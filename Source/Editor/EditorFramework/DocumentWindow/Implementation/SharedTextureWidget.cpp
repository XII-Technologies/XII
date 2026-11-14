/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorEngineProcessFramework/EngineProcess/EngineProcessMessages.h>
#include <EditorFramework/DocumentWindow/SharedTextureWidget.h>
#include <Foundation/Types/ScopeExit.h>
#include <GraphicsFoundation/CommandEncoder/CommandList.h>
#include <GraphicsFoundation/CommandEncoder/CommandQueue.h>
#include <GraphicsFoundation/Device/DeviceFactory.h>
#include <QGuiApplication>
#include <QPlatformSurfaceEvent>
#include <QWindow>
#if XII_ENABLED(XII_PLATFORM_LINUX)
#  include <QtGui/qguiapplication_platform.h>
#endif

xiiSharedPtr<xiiGALDevice> xiiQtSharedTextureWidget::s_pDevice;
xiiUInt32                  xiiQtSharedTextureWidget::s_uiWidgetCount = 0;

xiiQtSharedTextureWidget::xiiQtSharedTextureWidget(QWidget* parent) : QWidget(parent)
{
  ++s_uiWidgetCount;
  setAttribute(Qt::WA_DontCreateNativeAncestors);
  setAttribute(Qt::WA_NativeWindow);
  setAttribute(Qt::WA_TransparentForMouseEvents);
  setAttribute(Qt::WA_PaintOnScreen);
  setAttribute(Qt::WA_NoSystemBackground);
  setAutoFillBackground(false);
  setMouseTracking(true);
  setMinimumSize(64, 64);
}

xiiQtSharedTextureWidget::~xiiQtSharedTextureWidget()
{
  if (windowHandle())
    windowHandle()->removeEventFilter(this);
  DestroySwapChain();
  m_pImportedTexture.Clear();
  m_pPresentationTexture.Clear();
  if (--s_uiWidgetCount == 0 && s_pDevice)
  {
    s_pDevice->WaitIdle();
    s_pDevice.Clear();
  }
}

xiiResult xiiQtSharedTextureWidget::EnsureDevice()
{
  if (s_pDevice)
    return XII_SUCCESS;
  xiiGALDeviceCreationDescription description;
  description.m_GraphicsDeviceType                 = xiiGALGraphicsDeviceType::Vulkan;
  description.m_DeviceFeatures.m_NativeFence       = xiiGALDeviceFeatureState::Optional;
  description.m_DeviceFeatures.m_ExternalMemory    = xiiGALDeviceFeatureState::Optional;
  description.m_DeviceFeatures.m_ExternalSemaphore = xiiGALDeviceFeatureState::Optional;
#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
  description.m_ValidationLevel = xiiGALDeviceValidationLevel::Standard;
#endif
  auto device = xiiGALDeviceFactory::CreateDevice("Vulkan", xiiFoundation::GetDefaultAllocator(), description);
  if (!device || device->Initialize().Failed())
    return XII_FAILURE;
  s_pDevice = device;
  return XII_SUCCESS;
}

bool xiiQtSharedTextureWidget::SupportsSharedTextures()
{
  return !m_bSharingFailed && EnsureDevice().Succeeded() &&
    s_pDevice->GetFeatures().m_NativeFence == xiiGALDeviceFeatureState::Enabled &&
    s_pDevice->GetFeatures().m_ExternalMemory == xiiGALDeviceFeatureState::Enabled &&
    s_pDevice->GetFeatures().m_ExternalSemaphore == xiiGALDeviceFeatureState::Enabled;
}

xiiResult xiiQtSharedTextureWidget::EnsureSwapChain()
{
  XII_SUCCEED_OR_RETURN(EnsureDevice());
  if (!isVisible() || width() <= 0 || height() <= 0)
    return XII_FAILURE;
  const xiiSizeU32 size(static_cast<xiiUInt32>(qRound(width() * devicePixelRatioF())), static_cast<xiiUInt32>(qRound(height() * devicePixelRatioF())));
  if (m_pSwapChain)
  {
    m_pSwapChain->SetPresentMode(m_bVSync ? xiiGALPresentMode::VSync : xiiGALPresentMode::Immediate);
    if (m_pSwapChain->GetCurrentSize() != size)
      return m_pSwapChain->Resize(size);
    return XII_SUCCESS;
  }
  m_bCreatingSurface = true;
  const auto id      = winId();
  m_bCreatingSurface = false;
  if (!id || !windowHandle())
    return XII_FAILURE;
  windowHandle()->installEventFilter(this);
  xiiGALSwapChainCreationDescription description;
  description.m_SurfaceSize                   = size;
  description.m_NativeWindow.m_uiWindowHandle = static_cast<xiiUInt64>(id);
#if XII_ENABLED(XII_PLATFORM_WINDOWS)
  description.m_NativeWindow.m_Type = xiiGALNativeWindowDescription::Type::Win32;
#elif XII_ENABLED(XII_PLATFORM_LINUX)
  if (QGuiApplication::platformName() == "xcb")
  {
#  if QT_CONFIG(xcb)
    auto* native = qGuiApp->nativeInterface<QNativeInterface::QX11Application>();
    if (!native)
      return XII_FAILURE;
    description.m_NativeWindow.m_Type     = xiiGALNativeWindowDescription::Type::X11;
    description.m_NativeWindow.m_pDisplay = native->display();
#  else
    return XII_FAILURE;
#  endif
  }
  else if (QGuiApplication::platformName().startsWith("wayland"))
  {
#  if QT_CONFIG(wayland)
    auto* native = qGuiApp->nativeInterface<QNativeInterface::QWaylandApplication>();
    if (!native)
      return XII_FAILURE;
    description.m_NativeWindow.m_Type     = xiiGALNativeWindowDescription::Type::Wayland;
    description.m_NativeWindow.m_pDisplay = native->display();
    // The owning view handles input. Keep pointer grabs on the Qt widget window
    // instead of transferring them to the presentation-only Wayland subsurface.
    windowHandle()->setFlag(Qt::WindowTransparentForInput);
#  else
    return XII_FAILURE;
#  endif
  }
  else
    return XII_FAILURE;
#elif XII_ENABLED(XII_PLATFORM_OSX)
  description.m_NativeWindow.m_Type = xiiGALNativeWindowDescription::Type::Cocoa;
#else
  return XII_FAILURE;
#endif
  m_pSwapChain = s_pDevice->CreateSwapChain(description);
  if (m_pSwapChain)
    m_pSwapChain->SetPresentMode(m_bVSync ? xiiGALPresentMode::VSync : xiiGALPresentMode::Immediate);
  return m_pSwapChain ? XII_SUCCESS : XII_FAILURE;
}

void xiiQtSharedTextureWidget::SetFrame(const xiiViewFrameMsgToEditor& frame)
{
  const bool shared   = frame.m_Memory.m_Type.IsSet(xiiGALExternalMemoryKind::Exportable);
  bool       accepted = false;
  if (frame.m_uiWidth == 0 || frame.m_uiHeight == 0 || EnsureDevice().Failed())
  {
    m_OnFrameConsumed(frame.m_uiFrameID, false);
    return;
  }
  s_pDevice->BeginFrame();
  {
    XII_SCOPE_EXIT(s_pDevice->EndFrame());
    xiiGALTextureCreationDescription description;
    description.m_Type      = xiiGALResourceDimension::Texture2D;
    description.m_Size      = {frame.m_uiWidth, frame.m_uiHeight};
    description.m_Format    = xiiGALResourceFormat::RGBA8UNormalizedSRGB;
    description.m_BindFlags = xiiGALBindFlags::RenderTarget | xiiGALBindFlags::ShaderResource;
    if (shared)
    {
      if (m_uiTextureID != frame.m_uiTextureID || !m_pImportedTexture)
      {
        m_pImportedTexture.Clear();
        description.m_ExternalMemory = frame.m_Memory;
        m_pImportedTexture           = s_pDevice->CreateTexture(description, nullptr, xiiGALExternalMemoryKind::Imported);
        description.m_ExternalMemory = {};
        m_uiTextureID                = frame.m_uiTextureID;
      }
      if (m_pImportedTexture)
      {
        if (!m_pPresentationTexture || m_pPresentationTexture->GetDescription().m_Size != description.m_Size)
          m_pPresentationTexture = s_pDevice->CreateTexture(description);
        auto command = s_pDevice->CreateCommandList({.m_QueueFlags = xiiGALCommandQueueFlags::Graphics | xiiGALCommandQueueFlags::Transfer});
        if (command && m_pPresentationTexture)
        {
          command->Begin();
          const auto acquired = m_pImportedTexture->AcquireExternalMemory(command);
          if (acquired.Succeeded())
            command->CopyTexture(m_pImportedTexture, m_pPresentationTexture);
          const auto released = acquired.Succeeded() ? m_pImportedTexture->ReleaseExternalMemory(command) : XII_FAILURE;
          command->End();
          if (released.Succeeded())
          {
            s_pDevice->GetCommandQueue()->Submit(command);
            accepted = true;
          }
        }
      }
      if (!accepted)
      {
        m_bSharingFailed = true;
        xiiLog::Warning("GAL viewport texture import unavailable; using offscreen readback presentation.");
      }
    }
    else if (xiiUInt64(frame.m_uiWidth) * frame.m_uiHeight * 4 == frame.m_Pixels.GetCount())
    {
      xiiGALTextureSubResourceData data(frame.m_Pixels.GetArrayPtr().ToByteArray(), frame.m_uiWidth * 4, xiiUInt64(frame.m_uiWidth) * frame.m_uiHeight * 4);
      xiiGALTextureData            initialData(xiiMakeArrayPtr(&data, 1));
      m_pPresentationTexture = s_pDevice->CreateTexture(description, &initialData);
    }
    // The presentation texture is private to this process. Repaints never access
    // the imported image after its release semaphore has been submitted.
    if (m_pPresentationTexture && PresentTexture().Failed() && isVisible())
      xiiLog::Error("Could not present the editor viewport through GAL.");
  }
  m_OnFrameConsumed(frame.m_uiFrameID, accepted);
}

xiiResult xiiQtSharedTextureWidget::PresentTexture()
{
  XII_SUCCEED_OR_RETURN(EnsureSwapChain());
  auto target = m_pSwapChain->GetBackBufferTexture();
  if (!target || !m_pPresentationTexture)
    return XII_FAILURE;
  auto command = s_pDevice->CreateCommandList({.m_QueueFlags = xiiGALCommandQueueFlags::Graphics | xiiGALCommandQueueFlags::Transfer});
  if (!command)
    return XII_FAILURE;
  command->Begin();
  const auto result = command->BlitTexture(m_pPresentationTexture, target);
  command->End();
  XII_SUCCEED_OR_RETURN(result);
  s_pDevice->GetCommandQueue()->Submit(command);
  m_pSwapChain->Present();
  return XII_SUCCESS;
}

void xiiQtSharedTextureWidget::paintEvent(QPaintEvent*)
{
  if (!s_pDevice || !m_pPresentationTexture)
    return;
  s_pDevice->BeginFrame();
  PresentTexture().IgnoreResult();
  s_pDevice->EndFrame();
}

void xiiQtSharedTextureWidget::DestroySwapChain()
{
  if (s_pDevice)
    s_pDevice->WaitIdle();
  m_pSwapChain.Clear();
}

bool xiiQtSharedTextureWidget::eventFilter(QObject* object, QEvent* event)
{
  if (event->type() == QEvent::PlatformSurface && static_cast<QPlatformSurfaceEvent*>(event)->surfaceEventType() == QPlatformSurfaceEvent::SurfaceAboutToBeDestroyed)
    DestroySwapChain();
  return QWidget::eventFilter(object, event);
}

bool xiiQtSharedTextureWidget::event(QEvent* event)
{
  if (event->type() == QEvent::WinIdChange && !m_bCreatingSurface)
    DestroySwapChain();
  return QWidget::event(event);
}

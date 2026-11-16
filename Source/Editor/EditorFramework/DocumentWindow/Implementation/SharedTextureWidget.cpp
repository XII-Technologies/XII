/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorEngineProcessFramework/EngineProcess/EngineProcessMessages.h>
#include <EditorFramework/DocumentWindow/SharedTextureWidget.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/Types/ScopeExit.h>
#include <GraphicsFoundation/CommandEncoder/CommandList.h>
#include <GraphicsFoundation/CommandEncoder/CommandQueue.h>
#include <GraphicsFoundation/Device/DeviceFactory.h>
#include <QGuiApplication>
#include <QPlatformSurfaceEvent>
#include <QWidget>
#include <QWindow>
#include <Qt>
#if XII_ENABLED(XII_PLATFORM_LINUX)
#  include <QtGui/qguiapplication_platform.h>
#endif

xiiSharedPtr<xiiGALDevice> xiiQtSharedTextureWidget::s_pDevice;
xiiUInt32                  xiiQtSharedTextureWidget::s_uiWidgetCount = 0;

xiiQtSharedTextureWidget::xiiQtSharedTextureWidget(QWidget* parent) : QWidget(parent)
{
  ++s_uiWidgetCount;
  m_CompletionTimer.setSingleShot(true);
  m_CompletionTimer.setInterval(2);
  connect(&m_CompletionTimer, &QTimer::timeout, this, [this] {
    CollectCompletedFrames();
    if (m_bPresentRetry && isVisible())
    {
      update();
    }
    ScheduleCompletionCheck();
  });
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
  for (auto& slot : m_ImportedSlots)
  {
    slot.m_pTexture.Clear();
  }
  for (auto& slot : m_PresentationSlots)
  {
    slot.m_pTexture.Clear();
  }
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
  description.m_SurfaceSize      = size;
  description.m_bDeferredAcquire = true;
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
  // QWidget caches its WId. Wayland can replace the underlying surface when
  // the native window's parent changes without updating that cached value.
  description.m_NativeWindow.m_uiWindowHandle = static_cast<xiiUInt64>(windowHandle()->winId());
  m_pSwapChain                                = s_pDevice->CreateSwapChain(description);
  if (m_pSwapChain)
    m_pSwapChain->SetPresentMode(m_bVSync ? xiiGALPresentMode::VSync : xiiGALPresentMode::Immediate);
  return m_pSwapChain ? XII_SUCCESS : XII_FAILURE;
}

void xiiQtSharedTextureWidget::SetFrame(const xiiViewFrameMsgToEditor& frame)
{
  XII_PROFILE_SCOPE("Viewport: Receive frame");
  const bool bShared = frame.m_Memory.m_Type.IsSet(xiiGALExternalMemoryKind::Exportable);
  if (frame.m_uiSlotIndex >= XII_MAX_VIEWPORT_FRAME_SLOTS || frame.m_uiWidth == 0 || frame.m_uiHeight == 0 ||
      (!bShared && xiiUInt64(frame.m_uiWidth) * frame.m_uiHeight * 4U != frame.m_Pixels.GetCount()) || EnsureDevice().Failed())
  {
    m_OnFrameConsumed(frame.m_uiSlotIndex, frame.m_uiFrameID, false);
    return;
  }

  CollectCompletedFrames();
  auto& imported = m_ImportedSlots[frame.m_uiSlotIndex];
  if (imported.m_uiFrameID >= frame.m_uiFrameID || imported.m_uiReleaseFence != 0)
  {
    // The producer can only publish after we acknowledge the previous frame in this slot.
    return;
  }
  imported.m_uiFrameID = frame.m_uiFrameID;

  s_pDevice->BeginFrame();
  XII_SCOPE_EXIT(s_pDevice->EndFrame());
  xiiGALTextureCreationDescription description;
  description.m_Type      = xiiGALResourceDimension::Texture2D;
  description.m_Size      = {frame.m_uiWidth, frame.m_uiHeight};
  description.m_Format    = xiiGALResourceFormat::RGBA8UNormalizedSRGB;
  description.m_BindFlags = xiiGALBindFlags::RenderTarget | xiiGALBindFlags::ShaderResource;

  if (bShared && imported.m_uiTextureID != frame.m_uiTextureID)
  {
    XII_PROFILE_SCOPE("Viewport: Import texture allocation");
    description.m_ExternalMemory = frame.m_Memory;
    imported.m_pTexture          = s_pDevice->CreateTexture(description, nullptr, xiiGALExternalMemoryKind::Imported);
    description.m_ExternalMemory = {};
    imported.m_uiTextureID       = frame.m_uiTextureID;
  }
  else if (!bShared)
  {
    imported.m_pTexture.Clear();
    imported.m_uiTextureID = 0;
  }

  PresentationSlot* pDestination = nullptr;
  if (frame.m_uiFrameID > m_uiLatestFrameID && isVisible())
  {
    for (xiiUInt8 uiIndex = 0; uiIndex < PresentationSlotCount; ++uiIndex)
    {
      auto& slot = m_PresentationSlots[uiIndex];
      if (uiIndex == m_uiLatestSlot || slot.m_uiCopyFence != 0)
      {
        continue;
      }
      if (!slot.m_pTexture || slot.m_pTexture->GetDescription().m_Size != description.m_Size)
      {
        slot.m_pTexture = s_pDevice->CreateTexture(description);
      }
      if (slot.m_pTexture)
      {
        pDestination = &slot;
      }
      break;
    }
  }

  auto*     pQueue      = s_pDevice->GetCommandQueue();
  xiiUInt64 uiCopyFence = 0;
  if (bShared && imported.m_pTexture)
  {
    auto pCommand = s_pDevice->CreateCommandList({.m_QueueFlags = xiiGALCommandQueueFlags::Graphics | xiiGALCommandQueueFlags::Transfer});
    if (pCommand)
    {
      XII_PROFILE_SCOPE("Viewport: Submit external texture copy");
      pCommand->Begin();
      const auto acquired = imported.m_pTexture->AcquireExternalMemory(pCommand);
      if (acquired.Succeeded() && pDestination)
      {
        pCommand->CopyTexture(imported.m_pTexture, pDestination->m_pTexture);
      }
      const auto released = acquired.Succeeded() ? imported.m_pTexture->ReleaseExternalMemory(pCommand) : XII_FAILURE;
      pCommand->End();
      if (released.Succeeded())
      {
        uiCopyFence               = pQueue->Submit(pCommand);
        imported.m_uiReleaseFence = uiCopyFence;
      }
    }
  }
  else if (!bShared && pDestination)
  {
    auto pCommand = s_pDevice->CreateCommandList({.m_QueueFlags = xiiGALCommandQueueFlags::Graphics | xiiGALCommandQueueFlags::Transfer});
    if (pCommand)
    {
      XII_PROFILE_SCOPE("Viewport: Submit pixel upload");
      const xiiGALTextureSubResourceData data(frame.m_Pixels.GetArrayPtr().ToByteArray(), frame.m_uiWidth * 4U, frame.m_Pixels.GetCount());
      const auto                         box = xiiBoundingBoxU32::MakeFromMinMax(xiiVec3U32::MakeZero(), {frame.m_uiWidth, frame.m_uiHeight, 1U});
      pCommand->Begin();
      pCommand->UpdateTexture(pDestination->m_pTexture, {}, box, data);
      pCommand->End();
      uiCopyFence = pQueue->Submit(pCommand);
    }
  }

  if (uiCopyFence != 0 && pDestination)
  {
    pDestination->m_uiFrameID   = frame.m_uiFrameID;
    pDestination->m_uiCopyFence = uiCopyFence;
  }
  else
  {
    XII_PROFILE_SCOPE("Viewport: Drop presentation frame");
  }

  if (!bShared || imported.m_uiReleaseFence == 0)
  {
    if (bShared && !m_bSharingFailed)
    {
      m_bSharingFailed = true;
      xiiLog::Warning("GAL viewport texture import unavailable; using offscreen readback presentation.");
    }
    m_OnFrameConsumed(frame.m_uiSlotIndex, frame.m_uiFrameID, false);
  }
  ScheduleCompletionCheck();
}

void xiiQtSharedTextureWidget::CollectCompletedFrames()
{
  XII_PROFILE_SCOPE("Viewport: Collect completed copies");
  const xiiUInt64 uiCompleted = s_pDevice->GetCommandQueue()->GetCompletedFenceValue();
  for (xiiUInt8 uiIndex = 0; uiIndex < XII_MAX_VIEWPORT_FRAME_SLOTS; ++uiIndex)
  {
    auto& slot = m_ImportedSlots[uiIndex];
    if (slot.m_uiReleaseFence != 0 && slot.m_uiReleaseFence <= uiCompleted)
    {
      slot.m_uiReleaseFence = 0;
      m_OnFrameConsumed(uiIndex, slot.m_uiFrameID, true);
    }
  }
  const xiiUInt64 uiPreviousFrameID = m_uiLatestFrameID;
  for (xiiUInt8 uiIndex = 0; uiIndex < PresentationSlotCount; ++uiIndex)
  {
    auto& slot = m_PresentationSlots[uiIndex];
    if (slot.m_uiCopyFence != 0 && slot.m_uiCopyFence <= uiCompleted)
    {
      slot.m_uiCopyFence = 0;
      if (slot.m_uiFrameID > m_uiLatestFrameID)
      {
        m_uiLatestFrameID = slot.m_uiFrameID;
        m_uiLatestSlot    = uiIndex;
      }
    }
  }
  if (m_uiLatestFrameID != uiPreviousFrameID)
  {
    update();
  }
}

void xiiQtSharedTextureWidget::ScheduleCompletionCheck()
{
  if (m_CompletionTimer.isActive())
  {
    return;
  }
  const bool bPendingRelease = std::ranges::any_of(m_ImportedSlots, [](const ImportedSlot& slot) { return slot.m_uiReleaseFence != 0; });
  const bool bPendingCopy    = std::ranges::any_of(m_PresentationSlots, [](const PresentationSlot& slot) { return slot.m_uiCopyFence != 0; });
  if (bPendingRelease || bPendingCopy || (m_bPresentRetry && isVisible()))
  {
    m_CompletionTimer.start();
  }
}

xiiResult xiiQtSharedTextureWidget::PresentTexture()
{
  XII_PROFILE_SCOPE("Viewport: Present latest completed frame");
  m_bPresentRetry = false;
  XII_SUCCEED_OR_RETURN(EnsureSwapChain());
  xiiSharedPtr<xiiGALTexture> pTarget;
  XII_SUCCEED_OR_RETURN(m_pSwapChain->TryAcquireBackBuffer(pTarget));
  if (!pTarget)
  {
    m_bPresentRetry = true;
    ScheduleCompletionCheck();
    return XII_SUCCESS;
  }
  auto command = s_pDevice->CreateCommandList({.m_QueueFlags = xiiGALCommandQueueFlags::Graphics | xiiGALCommandQueueFlags::Transfer});
  if (!command)
    return XII_FAILURE;
  command->Begin();
  const auto result = command->BlitTexture(m_PresentationSlots[m_uiLatestSlot].m_pTexture, pTarget);
  command->End();
  XII_SUCCEED_OR_RETURN(result);
  s_pDevice->GetCommandQueue()->Submit(command);
  m_pSwapChain->Present();
  return XII_SUCCESS;
}

void xiiQtSharedTextureWidget::paintEvent(QPaintEvent*)
{
  if (!s_pDevice || m_uiLatestSlot >= PresentationSlotCount)
    return;
  s_pDevice->BeginFrame();
  PresentTexture().IgnoreResult();
  s_pDevice->EndFrame();
}

void xiiQtSharedTextureWidget::DestroySwapChain()
{
  m_pSwapChain.Clear();
}

bool xiiQtSharedTextureWidget::eventFilter(QObject* object, QEvent* event)
{
  if (event->type() == QEvent::ParentWindowAboutToChange ||
      (event->type() == QEvent::PlatformSurface && static_cast<QPlatformSurfaceEvent*>(event)->surfaceEventType() == QPlatformSurfaceEvent::SurfaceAboutToBeDestroyed))
  {
    DestroySwapChain();
  }
  return QWidget::eventFilter(object, event);
}

bool xiiQtSharedTextureWidget::event(QEvent* event)
{
  if (event->type() == QEvent::WinIdChange && !m_bCreatingSurface)
    DestroySwapChain();
  return QWidget::event(event);
}

/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <SharedPluginScene/SharedPluginScenePCH.h>

#include <GraphicsFoundation/CommandEncoder/CommandList.h>
#include <SharedPluginScene/Scene/Implementation/ScenePickingReadback.h>

void xiiScenePickingReadback::Invalidate()
{
  ++m_uiEpoch;
  m_bValid = false;
  m_Pixels.Clear();
  m_Completed.m_pTarget = nullptr;
  for (auto& slot : m_Slots)
    slot.m_Snapshot.m_pTarget = nullptr;
}

xiiResult xiiScenePickingReadback::Poll(xiiUInt32& out_uiCompletedSlot)
{
  out_uiCompletedSlot = xiiInvalidIndex;
  if (!m_pReadback)
    return XII_SUCCESS;
  while (m_pReadback->HasCompleted())
  {
    const auto capture = m_pReadback->GetCompleted();
    if (!capture || capture.m_uiTextureID >= XII_ARRAY_SIZE(m_Slots))
      return XII_FAILURE;
    auto& slot   = m_Slots[capture.m_uiTextureID];
    slot.m_State = Slot::Free;
    if (slot.m_Snapshot.m_uiEpoch != m_uiEpoch)
    {
      m_pReadback->RecycleStagingTexture(capture.m_pStagingTexture);
      slot.m_Snapshot.m_pTarget = nullptr;
      continue;
    }
    if (!m_pMapCommands)
      m_pMapCommands = xiiGALDevice::GetDefaultDevice()->CreateCommandList({.m_QueueFlags = xiiGALCommandQueueFlags::Graphics});
    if (!m_pMapCommands)
    {
      m_pReadback->RecycleStagingTexture(capture.m_pStagingTexture);
      Invalidate();
      return XII_FAILURE;
    }
    m_pMapCommands->Reset();
    m_pMapCommands->Begin();
    xiiGALMappedTextureSubresource mapped;
    xiiGALTextureMipLevelData      mip;
    auto                           result = m_pMapCommands->MapTextureSubresource(capture.m_pStagingTexture, mip, xiiGALMapType::Read, xiiGALMapFlags::DoNotWait, nullptr, mapped);
    if (result.Succeeded())
    {
      const xiiUInt64 rowWords = static_cast<xiiUInt64>(capture.m_uiWidth) * m_uiWordsPerPixel;
      const xiiUInt64 words    = rowWords * capture.m_uiHeight;
      if (mapped.m_pData && capture.m_uiBytesPerPixel == sizeof(xiiUInt32) * m_uiWordsPerPixel &&
          rowWords * sizeof(xiiUInt32) <= mapped.m_uiStride && words <= xiiMath::MaxValue<xiiUInt32>())
      {
        m_Pixels.SetCountUninitialized(static_cast<xiiUInt32>(words));
        for (xiiUInt32 y = 0; y < capture.m_uiHeight; ++y)
          xiiMemoryUtils::Copy(m_Pixels.GetData() + y * rowWords,
                               xiiMemoryUtils::AddByteOffset(static_cast<const xiiUInt32*>(mapped.m_pData), y * mapped.m_uiStride), static_cast<xiiUInt32>(rowWords));
      }
      else
        result = XII_FAILURE;
      if (m_pMapCommands->UnmapTextureSubresource(capture.m_pStagingTexture, mip).Failed())
        result = XII_FAILURE;
    }
    m_pMapCommands->End(); // Host mapping only; no submission or GPU wait.
    m_pReadback->RecycleStagingTexture(capture.m_pStagingTexture);
    if (result.Failed())
    {
      Invalidate();
      out_uiCompletedSlot = xiiInvalidIndex;
      return XII_FAILURE;
    }
    m_Completed               = slot.m_Snapshot;
    slot.m_Snapshot.m_pTarget = nullptr;
    m_bValid                  = true;
    out_uiCompletedSlot       = capture.m_uiTextureID;
  }
  return XII_SUCCESS;
}

xiiResult xiiScenePickingReadback::AddCapture(xiiRenderPipelinePassContext& context, xiiRenderGraphTextureHandle texture, xiiUInt32& out_uiReservedSlot)
{
  out_uiReservedSlot = xiiInvalidIndex;
  // Graphs which never executed release their reservations at the next CPU frame boundary.
  for (auto& slot : m_Slots)
    if (slot.m_State == Slot::Reserved && slot.m_uiSetupFrameIndex != context.m_Graph.GetFrameIndex())
      slot.m_State = Slot::Free;
  const auto* description = context.m_Graph.GetTextureDescription(texture);
  if (!context.m_pView || !context.m_pView->IsValid() || context.m_pView->GetCamera()->GetCameraMode() == xiiCameraMode::None ||
      !description || description->m_Type != xiiGALResourceDimension::Texture2D || description->m_uiSampleCount != 1 ||
      (description->m_Format != xiiGALResourceFormat::R32UInt && description->m_Format != xiiGALResourceFormat::RGBA32UInt))
    return XII_FAILURE;
  const xiiUInt32 wordsPerPixel = description->m_Format == xiiGALResourceFormat::R32UInt ? 1 : 4;
  const xiiUInt64 pixelCount    = static_cast<xiiUInt64>(description->m_Size.width) * description->m_Size.height;
  if (pixelCount == 0 || pixelCount > xiiMath::MaxValue<xiiUInt32>() / wordsPerPixel)
    return XII_FAILURE;
  if (!m_pReadback || m_Size != description->m_Size || m_uiWordsPerPixel != wordsPerPixel)
  {
    Invalidate();
    for (const auto& slot : m_Slots)
      if (slot.m_State == Slot::Pending)
        return XII_SUCCESS;
    m_pReadback       = XII_DEFAULT_NEW(xiiGALTextureReadback, xiiGALDevice::GetDefaultDevice());
    m_Size            = description->m_Size;
    m_uiWordsPerPixel = wordsPerPixel;
  }
  xiiUInt32 index = 0;
  for (; index < XII_ARRAY_SIZE(m_Slots); ++index)
    if (m_Slots[index].m_State == Slot::Free)
      break;
  if (index == XII_ARRAY_SIZE(m_Slots))
    return XII_SUCCESS;
  auto& slot                       = m_Slots[index];
  slot.m_State                     = Slot::Reserved;
  slot.m_uiSetupFrameIndex         = context.m_Graph.GetFrameIndex();
  slot.m_Snapshot.m_pWorld         = context.m_pView->GetWorld();
  slot.m_Snapshot.m_hView          = context.m_pView->GetHandle();
  auto* target                     = context.m_pView->GetRenderTargetView();
  slot.m_Snapshot.m_pTarget        = target ? xiiSharedPtr<xiiGALTextureView>(target, target->GetDevice()->GetAllocator()) : nullptr;
  slot.m_Snapshot.m_pSwapChain     = context.m_pView->GetSwapChain();
  slot.m_Snapshot.m_ViewProjection = context.m_pView->GetViewProjectionMatrix(xiiCameraEye::Left);
  slot.m_Snapshot.m_Viewport       = context.m_pView->GetViewport();
  slot.m_Snapshot.m_uiEpoch        = m_uiEpoch;
  struct Data
  {
    xiiRenderGraphTextureHandle m_Texture;
  };
  xiiStringBuilder name;
  name.SetFormat("{}: Read picking {}", context.m_sPassName, index);
  context.m_Graph.AddPass<Data>(name, xiiGALCommandQueueFlags::Graphics, [&](Data& data, xiiRenderGraphBuilder& builder) { data.m_Texture = builder.ReadTexture(texture, xiiGALResourceStateFlags::CopySource); }, [this, index](const Data& data, xiiRenderGraphPassContext& execution) {
      auto& captureSlot = m_Slots[index];
      const auto result = m_pReadback->Enqueue(&execution.GetCommandList(), {.m_pTexture = execution.GetTexture(data.m_Texture), .m_uiTextureID = index});
      captureSlot.m_State = result.Succeeded() ? Slot::Pending : Slot::Free;
      if (result.Failed())
      {
        Invalidate();
        xiiLog::Error("Failed to enqueue scene picking readback.");
      } }, true);
  out_uiReservedSlot = index;
  return XII_SUCCESS;
}

bool xiiScenePickingReadback::Matches(const xiiView& view) const
{
  return m_bValid && view.IsValid() && view.GetCamera()->GetCameraMode() != xiiCameraMode::None && view.GetCameraUsageHint() == xiiCameraUsageHint::EditorView &&
    view.GetWorld() == m_Completed.m_pWorld && view.GetHandle() == m_Completed.m_hView && view.GetViewport() == m_Completed.m_Viewport &&
    view.GetRenderTargetView() == m_Completed.m_pTarget.Borrow() && view.GetSwapChain() == m_Completed.m_pSwapChain &&
    view.GetViewProjectionMatrix(xiiCameraEye::Left).IsIdentical(m_Completed.m_ViewProjection);
}

/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <SharedPluginScene/SharedPluginScenePCH.h>

#include <Core/Graphics/Camera.h>
#include <Core/World/World.h>
#include <Foundation/Containers/HashSet.h>
#include <GraphicsCore/Pipeline/View.h>
#include <GraphicsFoundation/CommandEncoder/CommandList.h>
#include <SharedPluginScene/Scene/Implementation/SceneShapeIconPicking.h>

void xiiSceneShapeIconPicking::Invalidate()
{
  ++m_uiEpoch;
  m_bValid = false;
  m_Pixels.Clear();
  m_Hits.Clear();
  m_Completed.m_pTarget = nullptr;
  for (auto& slot : m_Slots)
    slot.m_Snapshot.m_pTarget = nullptr;
}

xiiResult xiiSceneShapeIconPicking::Poll()
{
  if (!m_pReadback)
    return XII_SUCCESS;
  while (m_pReadback->HasCompleted())
  {
    const auto capture = m_pReadback->GetCompleted();
    if (!capture || capture.m_uiTextureID >= XII_ARRAY_SIZE(m_Slots))
      return XII_FAILURE;
    auto& slot   = m_Slots[capture.m_uiTextureID];
    slot.m_State = Slot::Free;
    // Invalidated captures are drained/recycled without mapping or publishing stale IDs.
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
      if (mapped.m_pData && capture.m_uiBytesPerPixel == sizeof(xiiUInt32) && mapped.m_uiStride >= capture.m_uiWidth * sizeof(xiiUInt32))
      {
        m_Pixels.SetCountUninitialized(capture.m_uiWidth * capture.m_uiHeight);
        for (xiiUInt32 y = 0; y < capture.m_uiHeight; ++y)
          xiiMemoryUtils::Copy(m_Pixels.GetData() + y * capture.m_uiWidth,
                               xiiMemoryUtils::AddByteOffset(static_cast<const xiiUInt32*>(mapped.m_pData), y * mapped.m_uiStride), capture.m_uiWidth);
      }
      else
        result = XII_FAILURE;
      if (m_pMapCommands->UnmapTextureSubresource(capture.m_pStagingTexture, mip).Failed())
        result = XII_FAILURE;
    }
    m_pMapCommands->End(); // Host mapping only; no submission or GPU wait is needed.
    m_pReadback->RecycleStagingTexture(capture.m_pStagingTexture);
    if (result.Failed())
    {
      Invalidate();
      return XII_FAILURE;
    }
    m_Hits.Swap(slot.m_Hits);
    m_Completed               = slot.m_Snapshot;
    slot.m_Snapshot.m_pTarget = nullptr;
    m_bValid                  = true;
  }
  return XII_SUCCESS;
}

xiiResult xiiSceneShapeIconPicking::AddCapture(xiiRenderPipelinePassContext& context, xiiRenderGraphTextureHandle texture, xiiArrayPtr<const xiiSceneShapeIconPass::Icon> icons)
{
  // A reservation whose graph did not execute is safe to reuse at the next CPU frame boundary.
  for (auto& slot : m_Slots)
    if (slot.m_State == Slot::Reserved)
      slot.m_State = Slot::Free;
  XII_SUCCEED_OR_RETURN(Poll());
  const auto* description = context.m_Graph.GetTextureDescription(texture);
  if (!context.m_pView || !description || description->m_Format != xiiGALResourceFormat::R32UInt)
    return XII_FAILURE;
  if (!m_pReadback || m_Size != description->m_Size)
  {
    Invalidate();
    for (const auto& slot : m_Slots)
      if (slot.m_State == Slot::Pending)
        return XII_SUCCESS; // Drain old-size captures without blocking.
    m_pReadback = XII_DEFAULT_NEW(xiiGALTextureReadback, xiiGALDevice::GetDefaultDevice());
    m_Size      = description->m_Size;
  }
  xiiUInt32 index = 0;
  for (; index < XII_ARRAY_SIZE(m_Slots); ++index)
    if (m_Slots[index].m_State == Slot::Free)
      break;
  if (index == XII_ARRAY_SIZE(m_Slots))
    return XII_SUCCESS; // Backpressure: keep the latest completed image, never grow the queue.
  auto& slot                       = m_Slots[index];
  slot.m_State                     = Slot::Reserved;
  slot.m_Snapshot.m_pWorld         = context.m_pView->GetWorld();
  slot.m_Snapshot.m_hView          = context.m_pView->GetHandle();
  auto* target                     = context.m_pView->GetRenderTargetView();
  slot.m_Snapshot.m_pTarget        = target ? xiiSharedPtr<xiiGALTextureView>(target, target->GetDevice()->GetAllocator()) : nullptr;
  slot.m_Snapshot.m_pSwapChain     = context.m_pView->GetSwapChain();
  slot.m_Snapshot.m_ViewProjection = context.m_pView->GetViewProjectionMatrix(xiiCameraEye::Left);
  slot.m_Snapshot.m_Viewport       = context.m_pView->GetViewport();
  slot.m_Snapshot.m_uiEpoch        = m_uiEpoch;
  slot.m_Hits.Clear();
  slot.m_Hits.Reserve(icons.GetCount());
  for (const auto& icon : icons)
    slot.m_Hits.PushBack({icon.m_hObject, icon.m_hComponent, icon.m_vWorldPosition});
  struct Data
  {
    xiiRenderGraphTextureHandle m_Texture;
  };
  xiiStringBuilder name(context.m_sPassName, ": Read picking");
  context.m_Graph.AddPass<Data>(name, xiiGALCommandQueueFlags::Graphics, [&](Data& data, xiiRenderGraphBuilder& builder) { data.m_Texture = builder.ReadTexture(texture, xiiGALResourceStateFlags::CopySource); }, [this, index](const Data& data, xiiRenderGraphPassContext& execution) {
    auto& captureSlot = m_Slots[index];
    const auto result = m_pReadback->Enqueue(&execution.GetCommandList(), {.m_pTexture = execution.GetTexture(data.m_Texture), .m_uiTextureID = index});
    captureSlot.m_State = result.Succeeded() ? Slot::Pending : Slot::Free;
    if (result.Failed())
    {
      Invalidate();
      xiiLog::Error("Failed to enqueue scene shape icon picking readback.");
    } }, true);
  return XII_SUCCESS;
}

bool xiiSceneShapeIconPicking::Matches(const xiiView& view) const
{
  return m_bValid && view.IsValid() && view.GetCamera()->GetCameraMode() != xiiCameraMode::None && view.GetCameraUsageHint() == xiiCameraUsageHint::EditorView &&
    view.GetWorld() == m_Completed.m_pWorld && view.GetHandle() == m_Completed.m_hView && view.GetViewport() == m_Completed.m_Viewport &&
    view.GetRenderTargetView() == m_Completed.m_pTarget.Borrow() && view.GetSwapChain() == m_Completed.m_pSwapChain &&
    view.GetViewProjectionMatrix(xiiCameraEye::Left).IsIdentical(m_Completed.m_ViewProjection);
}

bool xiiSceneShapeIconPicking::Resolve(const xiiView& view, xiiUInt32 index, xiiSceneShapeIconPass::PickingResult& out_hit) const
{
  if (index == 0 || index > m_Hits.GetCount())
    return false;
  const auto&          hit       = m_Hits[index - 1];
  const xiiComponent*  component = nullptr;
  const xiiGameObject* object    = nullptr;
  if (!view.GetWorld()->TryGetObject(hit.m_hObject, object) || !view.GetWorld()->TryGetComponent(hit.m_hComponent, component) ||
      component->GetOwner() != object || !component->IsActive() || !object->IsActive() || object->IsShapeIconHidden() ||
      !object->GetGlobalPosition().IsIdentical(hit.m_vWorldPosition))
    return false;
  const auto& tags = object->GetTags();
  if (tags.IsAnySet(view.m_ExcludeTags) || (!view.m_IncludeTags.IsEmpty() && !tags.IsAnySet(view.m_IncludeTags)) ||
      tags.IsSetByName(xiiStringView("Editor")) || tags.IsSetByName(xiiStringView("EditorHidden")))
    return false;
  out_hit = hit;
  return true;
}

xiiResult xiiSceneShapeIconPicking::Pick(const xiiView& view, xiiUInt32 x, xiiUInt32 y, xiiSceneShapeIconPass::PickingResult& out_hit)
{
  out_hit = {};
  XII_SUCCEED_OR_RETURN(Poll());
  if (!Matches(view) || x >= m_Size.width || y >= m_Size.height || !view.GetViewport().Contains(xiiVec2(static_cast<float>(x) + 0.5f, static_cast<float>(y) + 0.5f)))
    return XII_FAILURE;
  Resolve(view, m_Pixels[y * m_Size.width + x], out_hit);
  return XII_SUCCESS;
}

xiiResult xiiSceneShapeIconPicking::Marquee(const xiiView& view, const xiiRectU32& rectangle, xiiDynamicArray<xiiSceneShapeIconPass::PickingResult>& out_hits)
{
  out_hits.Clear();
  XII_SUCCEED_OR_RETURN(Poll());
  if (!Matches(view) || rectangle.x > m_Size.width || rectangle.y > m_Size.height || rectangle.width > m_Size.width - rectangle.x || rectangle.height > m_Size.height - rectangle.y)
    return XII_FAILURE;
  xiiHashSet<xiiGameObjectHandle> seen;
  for (xiiUInt32 y = rectangle.y; y < rectangle.y + rectangle.height; ++y)
    for (xiiUInt32 x = rectangle.x; x < rectangle.x + rectangle.width; ++x)
    {
      if (!view.GetViewport().Contains(xiiVec2(static_cast<float>(x) + 0.5f, static_cast<float>(y) + 0.5f)))
        continue;
      xiiSceneShapeIconPass::PickingResult hit;
      if (Resolve(view, m_Pixels[y * m_Size.width + x], hit) && !seen.Insert(hit.m_hObject))
        out_hits.PushBack(hit);
    }
  return XII_SUCCESS;
}

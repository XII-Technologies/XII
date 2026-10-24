/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <SharedPluginScene/SharedPluginScenePCH.h>

#include <Core/Graphics/Camera.h>
#include <Core/World/World.h>
#include <Foundation/Containers/HashSet.h>
#include <GraphicsCore/Pipeline/View.h>
#include <SharedPluginScene/Scene/Implementation/SceneShapeIconPicking.h>

void xiiSceneShapeIconPicking::Invalidate()
{
  m_Readback.Invalidate();
  m_Hits.Clear();
}

xiiResult xiiSceneShapeIconPicking::Poll()
{
  xiiUInt32 slot;
  XII_SUCCEED_OR_RETURN(m_Readback.Poll(slot));
  if (slot != xiiInvalidIndex)
    m_Hits.Swap(m_SlotHits[slot]);
  return XII_SUCCESS;
}

xiiResult xiiSceneShapeIconPicking::AddCapture(xiiRenderPipelinePassContext& context, xiiRenderGraphTextureHandle texture, xiiArrayPtr<const xiiSceneShapeIconPass::Icon> icons)
{
  XII_SUCCEED_OR_RETURN(Poll());
  xiiUInt32 slot;
  XII_SUCCEED_OR_RETURN(m_Readback.AddCapture(context, texture, slot));
  if (slot == xiiInvalidIndex)
    return XII_SUCCESS;
  auto& hits = m_SlotHits[slot];
  hits.Clear();
  hits.Reserve(icons.GetCount());
  for (const auto& icon : icons)
    hits.PushBack({icon.m_hObject, icon.m_hComponent, icon.m_vWorldPosition});
  return XII_SUCCESS;
}

bool xiiSceneShapeIconPicking::Resolve(const xiiView& view, xiiUInt32 index, xiiSceneShapeIconPass::PickingResult& out_hit) const
{
  if (index == 0 || index > m_Hits.GetCount())
    return false;
  const auto&          hit       = m_Hits[index - 1];
  const xiiComponent*  component = nullptr;
  const xiiGameObject* object    = nullptr;
  if (!view.GetWorld()->TryGetObject(hit.m_hObject, object) || !view.GetWorld()->TryGetComponent(hit.m_hComponent, component) || component->GetOwner() != object || !component->IsActive() || !object->IsActive() || object->IsShapeIconHidden() || !object->GetGlobalPosition().IsIdentical(hit.m_vWorldPosition))
    return false;
  const auto& tags = object->GetTags();
  if (tags.IsAnySet(view.m_ExcludeTags) || (!view.m_IncludeTags.IsEmpty() && !tags.IsAnySet(view.m_IncludeTags)) || tags.IsSetByName(xiiStringView("Editor")) || tags.IsSetByName(xiiStringView("EditorHidden")))
    return false;
  out_hit = hit;
  return true;
}

xiiResult xiiSceneShapeIconPicking::Pick(const xiiView& view, xiiUInt32 x, xiiUInt32 y, xiiSceneShapeIconPass::PickingResult& out_hit)
{
  out_hit = {};
  XII_SUCCEED_OR_RETURN(Poll());
  if (!m_Readback.Matches(view) || !view.GetViewport().Contains(xiiVec2(static_cast<float>(x) + 0.5f, static_cast<float>(y) + 0.5f)))
    return XII_FAILURE;
  const auto& viewport = view.GetViewport();
  const auto  size     = m_Readback.GetSize();
  const auto  px       = static_cast<xiiUInt32>((float(x) + 0.5f - viewport.x) * size.width / viewport.width);
  const auto  py       = static_cast<xiiUInt32>((float(y) + 0.5f - viewport.y) * size.height / viewport.height);
  if (px >= size.width || py >= size.height)
    return XII_FAILURE;
  Resolve(view, m_Readback.GetPixels()[py * size.width + px], out_hit);
  return XII_SUCCESS;
}

xiiResult xiiSceneShapeIconPicking::Marquee(const xiiView& view, const xiiRectU32& rectangle, xiiDynamicArray<xiiSceneShapeIconPass::PickingResult>& out_hits)
{
  out_hits.Clear();
  XII_SUCCEED_OR_RETURN(Poll());
  const auto& viewport = view.GetViewport();
  const auto  size     = m_Readback.GetSize();
  if (!m_Readback.Matches(view) || rectangle.x < viewport.x || rectangle.y < viewport.y ||
      double(rectangle.x) + rectangle.width > double(viewport.x) + viewport.width || double(rectangle.y) + rectangle.height > double(viewport.y) + viewport.height)
    return XII_FAILURE;
  xiiHashSet<xiiGameObjectHandle> seen;
  for (xiiUInt32 y = rectangle.y; y < rectangle.y + rectangle.height; ++y)
    for (xiiUInt32 x = rectangle.x; x < rectangle.x + rectangle.width; ++x)
    {
      if (!view.GetViewport().Contains(xiiVec2(static_cast<float>(x) + 0.5f, static_cast<float>(y) + 0.5f)))
        continue;
      xiiSceneShapeIconPass::PickingResult hit;
      const auto                           px = static_cast<xiiUInt32>((float(x) + 0.5f - viewport.x) * size.width / viewport.width);
      const auto                           py = static_cast<xiiUInt32>((float(y) + 0.5f - viewport.y) * size.height / viewport.height);
      if (px >= size.width || py >= size.height)
        continue;
      if (Resolve(view, m_Readback.GetPixels()[py * size.width + px], hit) && !seen.Insert(hit.m_hObject))
        out_hits.PushBack(hit);
    }
  return XII_SUCCESS;
}

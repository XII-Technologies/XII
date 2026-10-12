/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <SharedPluginScene/SharedPluginScenePCH.h>

#include <bit>

#include <SharedPluginScene/Scene/Implementation/SceneMeshPickingState.h>

xiiResult xiiSceneMeshPickingState::Pick(const xiiView& view, xiiUInt32 x, xiiUInt32 y, xiiSceneMeshPickingPass::PickingResult& out_hit)
{
  out_hit = {};
  XII_SUCCEED_OR_RETURN(Poll());
  xiiUInt32 index;
  if (!m_Readback.Matches(view) || m_Readback.GetWordsPerPixel() != 4 || !Pixel(view, x, y, index))
    return XII_FAILURE;
  const auto pixels = m_Readback.GetPixels();
  const auto id     = pixels[index * 4];
  if (id == 0)
    return XII_SUCCESS;
  const Owner* owner = nullptr;
  if (!Resolve(view, id, owner))
    return XII_FAILURE;
  const float depth = std::bit_cast<float>(pixels[index * 4 + 1]);
  const float nx    = std::bit_cast<float>(pixels[index * 4 + 2]) * 2.0f - 1.0f;
  const float ny    = std::bit_cast<float>(pixels[index * 4 + 3]) * 2.0f - 1.0f;
  if (!xiiMath::IsFinite(depth) || depth < 0 || depth > 1 || !xiiMath::IsFinite(nx) || !xiiMath::IsFinite(ny))
    return XII_FAILURE;
  xiiVec3     normal(nx, ny, 1.0f - xiiMath::Abs(nx) - xiiMath::Abs(ny));
  const float unfold = xiiMath::Clamp(-normal.z, 0.0f, 1.0f);
  normal.x += normal.x >= 0 ? -unfold : unfold;
  normal.y += normal.y >= 0 ? -unfold : unfold;
  XII_SUCCEED_OR_RETURN(normal.NormalizeIfNotZero());
  const auto                             size = m_Readback.GetSize();
  const float                            u    = (static_cast<float>(index % size.width) + 0.5f) / size.width;
  const float                            v    = (static_cast<float>(index / size.width) + 0.5f) / size.height;
  xiiSceneMeshPickingPass::PickingResult hit;
  XII_SUCCEED_OR_RETURN(xiiGraphicsUtils::ConvertScreenPosToWorldPos(view.GetData().m_InverseViewProjectionMatrix[0], xiiVec3(u, v, depth), hit.m_vWorldPosition, nullptr, xiiClipSpaceDepthRange::ZeroToOne));
  xiiVec3 direction;
  XII_SUCCEED_OR_RETURN(view.GetData().ComputePickingRay(u, v, hit.m_vRayStart, direction));
  if (!hit.m_vWorldPosition.IsValid())
    return XII_FAILURE;
  hit.m_Source       = owner->m_Source;
  hit.m_vWorldNormal = normal;
  out_hit            = hit;
  return XII_SUCCESS;
}

xiiResult xiiSceneMeshPickingState::Marquee(const xiiView& view, const xiiRectU32& rectangle, xiiDynamicArray<xiiGameObjectHandle>& out_objects)
{
  out_objects.Clear();
  XII_SUCCEED_OR_RETURN(Poll());
  if (!m_Readback.Matches(view) || m_Readback.GetWordsPerPixel() != 4 ||
      rectangle.width > xiiMath::MaxValue<xiiUInt32>() - rectangle.x || rectangle.height > xiiMath::MaxValue<xiiUInt32>() - rectangle.y)
    return XII_FAILURE;
  if (rectangle.width == 0 || rectangle.height == 0)
    return XII_SUCCESS;
  xiiUInt32 ignored;
  if (!Pixel(view, rectangle.x, rectangle.y, ignored) || !Pixel(view, rectangle.x + rectangle.width - 1, rectangle.y + rectangle.height - 1, ignored))
    return XII_FAILURE;
  xiiHashSet<xiiUInt32>           ids;
  xiiHashSet<xiiGameObjectHandle> objects;
  const auto                      pixels = m_Readback.GetPixels();
  for (xiiUInt32 y = rectangle.y; y < rectangle.y + rectangle.height; ++y)
    for (xiiUInt32 x = rectangle.x; x < rectangle.x + rectangle.width; ++x)
    {
      xiiUInt32 index;
      if (!Pixel(view, x, y, index))
        continue;
      const auto id = pixels[index * 4];
      if (id == 0 || ids.Insert(id))
        continue;
      const Owner* owner = nullptr;
      if (!Resolve(view, id, owner))
      {
        out_objects.Clear();
        return XII_FAILURE;
      }
      if (!objects.Insert(owner->m_Source.m_hOwnerObject))
        out_objects.PushBack(owner->m_Source.m_hOwnerObject);
    }
  return XII_SUCCESS;
}

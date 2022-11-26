#include <Core/CorePCH.h>

#include <Core/World/SpatialSystem.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiSpatialSystem, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiSpatialSystem::xiiSpatialSystem() :
  m_Allocator("Spatial System", xiiFoundation::GetDefaultAllocator())
{
}

xiiSpatialSystem::~xiiSpatialSystem() = default;

void xiiSpatialSystem::StartNewFrame()
{
  ++m_uiFrameCounter;
}

void xiiSpatialSystem::FindObjectsInSphere(const xiiBoundingSphere& sphere, const QueryParams& queryParams, xiiDynamicArray<xiiGameObject*>& out_Objects) const
{
  out_Objects.Clear();

  FindObjectsInSphere(
    sphere, queryParams,
    [&](xiiGameObject* pObject) {
      out_Objects.PushBack(pObject);

      return xiiVisitorExecution::Continue;
    });
}

void xiiSpatialSystem::FindObjectsInBox(const xiiBoundingBox& box, const QueryParams& queryParams, xiiDynamicArray<xiiGameObject*>& out_Objects) const
{
  out_Objects.Clear();

  FindObjectsInBox(
    box, queryParams,
    [&](xiiGameObject* pObject) {
      out_Objects.PushBack(pObject);

      return xiiVisitorExecution::Continue;
    });
}

#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
void xiiSpatialSystem::GetInternalStats(xiiStringBuilder& sb) const
{
  sb.Clear();
}
#endif

XII_STATICLINK_FILE(Core, Core_World_Implementation_SpatialSystem);

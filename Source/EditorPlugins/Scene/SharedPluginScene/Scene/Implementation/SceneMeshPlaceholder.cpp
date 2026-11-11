/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <SharedPluginScene/SharedPluginScenePCH.h>

#include <Core/World/World.h>
#include <Foundation/Math/Frustum.h>
#include <GraphicsCore/Debug/DebugRenderer.h>
#include <GraphicsCore/Meshes/MeshComponent.h>
#include <GraphicsCore/Meshes/MeshResource.h>
#include <GraphicsCore/Pipeline/View.h>
#include <SharedPluginScene/Scene/SceneMeshPlaceholder.h>

void xiiSceneMeshPlaceholder::Collect(const xiiView& view, xiiDynamicArray<xiiSceneMeshPlaceholder>& out_bounds)
{
  out_bounds.Clear();
  const auto* world = view.GetWorld();
  if (!world || view.GetCameraUsageHint() != xiiCameraUsageHint::EditorView)
    return;
  for (auto object = world->GetObjects(); object.IsValid(); ++object)
  {
    const auto& tags = object->GetTags();
    if (!object->IsActive() || tags.IsAnySet(view.m_ExcludeTags) || (!view.m_IncludeTags.IsEmpty() && !tags.IsAnySet(view.m_IncludeTags)))
      continue;
    for (const auto* component : object->GetComponents())
    {
      const auto* mesh = xiiDynamicCast<const xiiMeshComponentBase*>(component);
      if (!mesh || !mesh->IsActiveAndInitialized() || !mesh->GetMesh().IsValid() || mesh->GetVisibilityMask() == 0)
        continue;
      xiiResourceLock<xiiMeshResource> resource(mesh->GetMesh(), xiiResourceAcquireMode::PointerOnly);
      if (resource->IsReadyForRendering())
        continue;
      // A single render component's cached object bounds can survive a reload.
      // Bounds from unrelated components must not become the mesh's placeholder.
      auto bounds = object->GetComponents().GetCount() == 1 ? object->GetLocalBounds() : xiiBoundingBoxSphere::MakeInvalid();
      if (resource->GetLoadingState() == xiiResourceState::Loaded && resource->GetBounds().IsValid())
      {
        bounds = resource->GetBounds();
        if (const auto* instanced = xiiDynamicCast<const xiiInstancedMeshComponent*>(mesh))
        {
          const auto local = bounds;
          bounds           = xiiBoundingBoxSphere::MakeInvalid();
          for (const auto& transform : instanced->GetInstanceTransforms())
          {
            auto instance = local;
            instance.Transform(transform);
            bounds.ExpandToInclude(instance);
          }
          if (instanced->GetInstanceTransforms().IsEmpty())
            continue;
        }
      }
      if (!bounds.IsValid() || bounds.m_vBoxHalfExtents.IsZero())
        bounds = xiiBoundingBoxSphere::MakeFromBox(xiiBoundingBox::MakeFromCenterAndHalfExtents(xiiVec3::MakeZero(), xiiVec3(0.25f)));
      auto& placeholder         = out_bounds.ExpandAndGetRef();
      placeholder.m_hObject     = object->GetHandle();
      placeholder.m_hComponent  = mesh->GetHandle();
      placeholder.m_LocalBounds = bounds.GetBox();
      placeholder.m_Transform   = object->GetGlobalTransform();
    }
  }
}

void xiiSceneMeshPlaceholder::Draw(const xiiView& view)
{
  xiiDynamicArray<xiiSceneMeshPlaceholder> bounds;
  Collect(view, bounds);
  for (const auto& mesh : bounds)
    xiiDebugRenderer::DrawLineBox(view, mesh.m_LocalBounds, xiiColor::Orange, mesh.m_Transform);
}

xiiResult xiiSceneMeshPlaceholder::Pick(const xiiView& view, float u, float v, xiiSceneMeshPickingPass::PickingResult& out_hit)
{
  out_hit = {};
  xiiVec3 start, direction;
  if (!xiiMath::IsFinite(u) || !xiiMath::IsFinite(v) || u < 0 || u > 1 || v < 0 || v > 1 || view.ComputePickingRay(u, v, start, direction).Failed())
    return XII_FAILURE;
  xiiDynamicArray<xiiSceneMeshPlaceholder> bounds;
  Collect(view, bounds);
  float closest = xiiMath::MaxValue<float>();
  for (const auto& mesh : bounds)
  {
    auto inverse = mesh.m_Transform.GetAsMat4();
    if (inverse.Invert().Failed())
      continue;
    const auto localStart     = inverse.TransformPosition(start);
    const auto localDirection = inverse.TransformDirection(direction);
    float      distance       = 0;
    if (!mesh.m_LocalBounds.Contains(localStart) && !mesh.m_LocalBounds.GetRayIntersection(localStart, localDirection, &distance))
      continue;
    if (distance < 0 || distance >= closest)
      continue;
    const auto position = start + direction * distance;
    xiiVec3    screen;
    if (view.GetData().ComputeScreenSpacePos(position, screen).Failed() || screen.z < 0 || screen.z > 1)
      continue;
    closest                            = distance;
    out_hit.m_Source.m_hOwnerObject    = mesh.m_hObject;
    out_hit.m_Source.m_hOwnerComponent = mesh.m_hComponent;
    out_hit.m_vWorldPosition           = position;
    out_hit.m_vWorldNormal             = -direction;
    out_hit.m_vRayStart                = start;
  }
  return closest < xiiMath::MaxValue<float>() ? XII_SUCCESS : XII_FAILURE;
}

void xiiSceneMeshPlaceholder::PickObjectsInRect(const xiiView& view, const xiiRectU32& rectangle, xiiDynamicArray<xiiGameObjectHandle>& inout_objects)
{
  xiiDynamicArray<xiiSceneMeshPlaceholder> bounds;
  Collect(view, bounds);
  const auto& viewport = view.GetViewport();
  const float left     = xiiMath::Max(float(rectangle.x), viewport.x);
  const float top      = xiiMath::Max(float(rectangle.y), viewport.y);
  const float right    = xiiMath::Min(float(rectangle.x) + rectangle.width, viewport.x + viewport.width);
  const float bottom   = xiiMath::Min(float(rectangle.y) + rectangle.height, viewport.y + viewport.height);
  if (right <= left || bottom <= top)
    return;
  // Crop the existing projection to the marquee, then use the engine's frustum
  // test. This also handles bounds that cross the near plane or surround the view.
  auto crop          = xiiMat4::MakeIdentity();
  crop.Element(0, 0) = viewport.width / (right - left);
  crop.Element(1, 1) = viewport.height / (bottom - top);
  crop.Element(3, 0) = (viewport.width - (left + right - 2 * viewport.x)) / (right - left);
  crop.Element(3, 1) = ((top + bottom - 2 * viewport.y) - viewport.height) / (bottom - top);
  xiiFrustum selection;
  if (xiiFrustum::TryMakeFromMVP(selection, crop * view.GetViewProjectionMatrix(xiiCameraEye::Left), xiiClipSpaceDepthRange::ZeroToOne).Failed())
    return;
  for (const auto& mesh : bounds)
  {
    xiiVec3 corners[8];
    mesh.m_LocalBounds.GetCorners(corners);
    if (selection.GetObjectPosition(corners, 8, mesh.m_Transform.GetAsMat4()) != xiiVolumePosition::Outside)
      inout_objects.PushBack(mesh.m_hObject);
  }
}

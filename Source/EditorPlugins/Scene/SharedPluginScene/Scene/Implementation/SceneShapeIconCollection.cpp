/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <SharedPluginScene/SharedPluginScenePCH.h>

#include <Core/Graphics/Camera.h>
#include <Core/ResourceManager/ResourceManager.h>
#include <Core/World/World.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/Math/ColorScheme.h>
#include <GraphicsCore/Pipeline/View.h>
#include <SharedPluginScene/Scene/SceneShapeIconPass.h>

namespace
{
  struct TypeInfo
  {
    xiiTexture2DResourceHandle                     m_hTexture;
    const xiiTypedMemberProperty<xiiColor>*        m_pColor   = nullptr;
    const xiiTypedMemberProperty<xiiColorGammaUB>* m_pGamma   = nullptr;
    xiiColor                                       m_Fallback = xiiColor::White;
  };

  TypeInfo DescribeType(const xiiRTTI& type)
  {
    TypeInfo         info;
    xiiStringBuilder path("Editor/ShapeIcons/", type.GetTypeName(), ".dds");
    info.m_hTexture = xiiResourceManager::GetExistingResource<xiiTexture2DResource>(path);
    if (!info.m_hTexture.IsValid() && xiiFileSystem::ExistsFile(path))
      info.m_hTexture = xiiResourceManager::LoadResource<xiiTexture2DResource>(path);
    if (!info.m_hTexture.IsValid())
      return info;

    xiiHybridArray<const xiiAbstractProperty*, 32> properties;
    type.GetAllProperties(properties);
    for (const auto* property : properties)
    {
      if (property->GetCategory() != xiiPropertyCategory::Member)
        continue;
      if (!info.m_pGamma && property->GetSpecificType() == xiiGetStaticRTTI<xiiColorGammaUB>())
        info.m_pGamma = static_cast<const xiiTypedMemberProperty<xiiColorGammaUB>*>(property);
      if (!info.m_pColor && property->GetSpecificType() == xiiGetStaticRTTI<xiiColor>())
        info.m_pColor = static_cast<const xiiTypedMemberProperty<xiiColor>*>(property);
    }
    if (const auto* category = type.GetAttributeByType<xiiCategoryAttribute>())
      info.m_Fallback = xiiColorScheme::GetCategoryColor(category->GetCategory(), xiiColorScheme::CategoryColorUsage::ViewportIcon);
    return info;
  }
} // namespace

xiiResult xiiSceneShapeIconPass::Collect(const xiiView& view)
{
  m_Icons.Clear();
  const auto* camera   = view.GetCamera();
  const auto& viewport = view.GetViewport();
  if (!view.GetWorld() || !camera || camera->GetCameraMode() == xiiCameraMode::None || camera->IsStereoscopic() || !xiiMath::IsFinite(viewport.x) || !xiiMath::IsFinite(viewport.y) || !xiiMath::IsFinite(viewport.width) || !xiiMath::IsFinite(viewport.height) || viewport.width <= 0 || viewport.height <= 0 || !xiiMath::IsFinite(m_fSize) || m_fSize < 0 || !xiiMath::IsFinite(m_fMaxScreenSize) || m_fMaxScreenSize < 0)
    return XII_FAILURE;
  const float projectionScale = xiiMath::Abs(view.GetProjectionMatrix(xiiCameraEye::Left).Element(1, 1));
  if (!xiiMath::IsFinite(projectionScale) || projectionScale <= 0)
    return XII_FAILURE;
  if (m_fSize == 0 || m_fMaxScreenSize == 0)
    return XII_SUCCESS;

  // Reflection pointers live only during collection; plugin reloads never leave cached RTTI behind.
  // One type lookup / missing-file check per type per frame, including objects without render data.
  xiiHashTable<const xiiRTTI*, TypeInfo> types;
  for (auto it = view.GetWorld()->GetObjects(); it.IsValid(); ++it)
  {
    const xiiGameObject& object = *it;
    if (!object.IsActive() || object.IsShapeIconHidden() || object.GetTags().IsAnySet(view.m_ExcludeTags) || (!view.m_IncludeTags.IsEmpty() && !object.GetTags().IsAnySet(view.m_IncludeTags)) || object.GetTags().IsSetByName(xiiStringView("Editor")) || object.GetTags().IsSetByName(xiiStringView("EditorHidden")))
      continue;

    xiiVec3     screen;
    const float depth = (object.GetGlobalPosition() - camera->GetCenterPosition()).Dot(camera->GetCenterDirForwards());
    if ((!camera->IsOrthographic() && depth <= 0) || view.ComputeScreenSpacePos(object.GetGlobalPosition(), screen).Failed() || !screen.IsValid() || screen.z < 0 || screen.z > 1)
      continue;
    const float pixels = xiiMath::Min(m_fMaxScreenSize, m_fSize * projectionScale * viewport.height / (2.0f * (camera->IsOrthographic() ? 1.0f : depth)));
    if (!xiiMath::IsFinite(pixels) || pixels <= 0)
      continue;
    const xiiVec2 size(pixels / viewport.width, pixels / viewport.height);
    // Cull by the projected quad, so partly visible icons at viewport edges are retained.
    if (screen.x + size.x * 0.5f <= 0 || screen.x - size.x * 0.5f >= 1 || screen.y + size.y * 0.5f <= 0 || screen.y - size.y * 0.5f >= 1)
      continue;

    for (const auto* component : object.GetComponents())
    {
      if (!component->IsActive())
        continue;
      const auto* type = component->GetDynamicRTTI();
      TypeInfo*   info = nullptr;
      if (!types.TryGetValue(type, info))
      {
        types.Insert(type, DescribeType(*type));
        types.TryGetValue(type, info);
      }
      if (!info->m_hTexture.IsValid())
        continue;
      auto& icon             = m_Icons.ExpandAndGetRef();
      icon.m_hObject         = object.GetHandle();
      icon.m_hComponent      = component->GetHandle();
      icon.m_hTexture        = info->m_hTexture;
      icon.m_vWorldPosition  = object.GetGlobalPosition();
      icon.m_vScreenPosition = screen;
      icon.m_vScreenSize     = size;
      icon.m_Color           = info->m_pGamma ? xiiColor(info->m_pGamma->GetValue(component)) : info->m_pColor ? info->m_pColor->GetValue(component) :
                                                                                                                 info->m_Fallback;
      if (!icon.m_Color.IsValid())
        icon.m_Color = info->m_Fallback;
      icon.m_Color.a      = 1.0f;
      icon.m_fCameraDepth = depth;
      break; // One icon per object, using the first active component that actually has an icon.
    }
  }
  m_Icons.Sort([](const Icon& a, const Icon& b) {
    if (a.m_fCameraDepth != b.m_fCameraDepth)
      return a.m_fCameraDepth > b.m_fCameraDepth;
    return a.m_hObject.GetInternalID().m_Data < b.m_hObject.GetInternalID().m_Data;
  });
  return XII_SUCCESS;
}

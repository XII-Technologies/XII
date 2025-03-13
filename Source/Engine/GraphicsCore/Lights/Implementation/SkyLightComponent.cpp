#include <GraphicsCore/GraphicsCorePCH.h>

#include <Core/Messages/TransformChangedMessage.h>
#include <Core/Messages/UpdateLocalBoundsMessage.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <GraphicsCore/Lights/Implementation/ReflectionPool.h>
#include <GraphicsCore/Lights/SkyLightComponent.h>
#include <GraphicsCore/Pipeline/RenderData.h>
#include <GraphicsCore/Pipeline/View.h>
#include <GraphicsCore/RenderWorld/RenderWorld.h>
#include <GraphicsCore/Textures/TextureCubeResource.h>

namespace
{
  static xiiVariantArray GetDefaultTags()
  {
    xiiVariantArray value(xiiStaticAllocatorWrapper::GetAllocator());
    value.PushBack("SkyLight");
    return value;
  }
} // namespace

// clang-format off
XII_BEGIN_COMPONENT_TYPE(xiiSkyLightComponent, 3, xiiComponentMode::Static)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ENUM_ACCESSOR_PROPERTY("ReflectionProbeMode", xiiReflectionProbeMode, GetReflectionProbeMode, SetReflectionProbeMode)->AddAttributes(new xiiDefaultValueAttribute(xiiReflectionProbeMode::Dynamic), new xiiGroupAttribute("Capture Description")),
    XII_ACCESSOR_PROPERTY("CubeMap", GetCubeMapFile, SetCubeMapFile)->AddAttributes(new xiiAssetBrowserAttribute("CompatibleAsset_Texture_Cube")),
    XII_ACCESSOR_PROPERTY("Intensity", GetIntensity, SetIntensity)->AddAttributes(new xiiClampValueAttribute(0.0f, xiiVariant()), new xiiDefaultValueAttribute(0.4f)),
    XII_ACCESSOR_PROPERTY("Saturation", GetSaturation, SetSaturation)->AddAttributes(new xiiClampValueAttribute(0.0f, xiiVariant()), new xiiDefaultValueAttribute(0.3f)),
    XII_SET_ACCESSOR_PROPERTY("IncludeTags", GetIncludeTags, InsertIncludeTag, RemoveIncludeTag)->AddAttributes(new xiiTagSetWidgetAttribute("Default"), new xiiDefaultValueAttribute(GetDefaultTags())),
    XII_SET_ACCESSOR_PROPERTY("ExcludeTags", GetExcludeTags, InsertExcludeTag, RemoveExcludeTag)->AddAttributes(new xiiTagSetWidgetAttribute("Default")),
    XII_ACCESSOR_PROPERTY("NearPlane", GetNearPlane, SetNearPlane)->AddAttributes(new xiiDefaultValueAttribute(0.0f), new xiiClampValueAttribute(0.0f, {}), new xiiMinValueTextAttribute("Auto")),
    XII_ACCESSOR_PROPERTY("FarPlane", GetFarPlane, SetFarPlane)->AddAttributes(new xiiDefaultValueAttribute(100.0f), new xiiClampValueAttribute(0.01f, 10000.0f)),
    XII_ACCESSOR_PROPERTY("ShowDebugInfo", GetShowDebugInfo, SetShowDebugInfo),
    XII_ACCESSOR_PROPERTY("ShowMipMaps", GetShowMipMaps, SetShowMipMaps),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_MESSAGEHANDLERS
  {
    XII_MESSAGE_HANDLER(xiiMsgUpdateLocalBounds, OnUpdateLocalBounds),
    XII_MESSAGE_HANDLER(xiiMsgExtractRenderData, OnMsgExtractRenderData),
    XII_MESSAGE_HANDLER(xiiMsgTransformChanged, OnTransformChanged),
  }
  XII_END_MESSAGEHANDLERS;
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("Lighting"),
  }
  XII_END_ATTRIBUTES;
}
XII_END_COMPONENT_TYPE
// clang-format on

xiiSkyLightComponent::xiiSkyLightComponent()
{
  m_Desc.m_uniqueID = xiiUuid::MakeUuid();
}

xiiSkyLightComponent::~xiiSkyLightComponent() = default;

void xiiSkyLightComponent::OnActivated()
{
  GetOwner()->EnableStaticTransformChangesNotifications();
  m_Id = xiiReflectionPool::RegisterSkyLight(GetWorld(), m_Desc, this);

  GetOwner()->UpdateLocalBounds();
}

void xiiSkyLightComponent::OnDeactivated()
{
  xiiReflectionPool::DeregisterSkyLight(GetWorld(), m_Id);
  m_Id.Invalidate();

  GetOwner()->UpdateLocalBounds();
}

void xiiSkyLightComponent::SetReflectionProbeMode(xiiEnum<xiiReflectionProbeMode> mode)
{
  m_Desc.m_Mode  = mode;
  m_bStatesDirty = true;
}

xiiEnum<xiiReflectionProbeMode> xiiSkyLightComponent::GetReflectionProbeMode() const
{
  return m_Desc.m_Mode;
}

void xiiSkyLightComponent::SetIntensity(float fIntensity)
{
  m_Desc.m_fIntensity = fIntensity;
  m_bStatesDirty      = true;
}

float xiiSkyLightComponent::GetIntensity() const
{
  return m_Desc.m_fIntensity;
}

void xiiSkyLightComponent::SetSaturation(float fSaturation)
{
  m_Desc.m_fSaturation = fSaturation;
  m_bStatesDirty       = true;
}

float xiiSkyLightComponent::GetSaturation() const
{
  return m_Desc.m_fSaturation;
}

const xiiTagSet& xiiSkyLightComponent::GetIncludeTags() const
{
  return m_Desc.m_IncludeTags;
}

void xiiSkyLightComponent::InsertIncludeTag(xiiStringView sTag)
{
  m_Desc.m_IncludeTags.SetByName(sTag);
  m_bStatesDirty = true;
}

void xiiSkyLightComponent::RemoveIncludeTag(xiiStringView sTag)
{
  m_Desc.m_IncludeTags.RemoveByName(sTag);
  m_bStatesDirty = true;
}

const xiiTagSet& xiiSkyLightComponent::GetExcludeTags() const
{
  return m_Desc.m_ExcludeTags;
}

void xiiSkyLightComponent::InsertExcludeTag(xiiStringView sTag)
{
  m_Desc.m_ExcludeTags.SetByName(sTag);
  m_bStatesDirty = true;
}

void xiiSkyLightComponent::RemoveExcludeTag(xiiStringView sTag)
{
  m_Desc.m_ExcludeTags.RemoveByName(sTag);
  m_bStatesDirty = true;
}

void xiiSkyLightComponent::SetShowDebugInfo(bool bShowDebugInfo)
{
  m_Desc.m_bShowDebugInfo = bShowDebugInfo;
  m_bStatesDirty          = true;
}

bool xiiSkyLightComponent::GetShowDebugInfo() const
{
  return m_Desc.m_bShowDebugInfo;
}

void xiiSkyLightComponent::SetShowMipMaps(bool bShowMipMaps)
{
  m_Desc.m_bShowMipMaps = bShowMipMaps;
  m_bStatesDirty        = true;
}

bool xiiSkyLightComponent::GetShowMipMaps() const
{
  return m_Desc.m_bShowMipMaps;
}


void xiiSkyLightComponent::SetCubeMapFile(xiiStringView sFile)
{
  xiiTextureCubeResourceHandle hCubeMap;
  if (!sFile.IsEmpty())
  {
    hCubeMap = xiiResourceManager::LoadResource<xiiTextureCubeResource>(sFile);
  }
  m_hCubeMap     = hCubeMap;
  m_bStatesDirty = true;
}

xiiStringView xiiSkyLightComponent::GetCubeMapFile() const
{
  return m_hCubeMap.GetResourceID();
}

void xiiSkyLightComponent::SetNearPlane(float fNearPlane)
{
  m_Desc.m_fNearPlane = fNearPlane;
  m_bStatesDirty      = true;
}

void xiiSkyLightComponent::SetFarPlane(float fFarPlane)
{
  m_Desc.m_fFarPlane = fFarPlane;
  m_bStatesDirty     = true;
}

void xiiSkyLightComponent::OnUpdateLocalBounds(xiiMsgUpdateLocalBounds& msg)
{
  msg.SetAlwaysVisible(GetOwner()->IsDynamic() ? xiiDefaultSpatialDataCategories::RenderDynamic : xiiDefaultSpatialDataCategories::RenderStatic);
}

void xiiSkyLightComponent::OnMsgExtractRenderData(xiiMsgExtractRenderData& msg) const
{
  // Don't trigger reflection rendering in shadow or other reflection views.
  if (msg.m_pView->GetCameraUsageHint() == xiiCameraUsageHint::Shadow || msg.m_pView->GetCameraUsageHint() == xiiCameraUsageHint::Reflection)
    return;

  if (m_bStatesDirty)
  {
    m_bStatesDirty = false;
    xiiReflectionPool::UpdateSkyLight(GetWorld(), m_Id, m_Desc, this);
  }

  xiiReflectionPool::ExtractReflectionProbe(this, msg, nullptr, GetWorld(), m_Id, xiiMath::MaxValue<float>());
}

void xiiSkyLightComponent::OnTransformChanged(xiiMsgTransformChanged& msg)
{
  m_bStatesDirty = true;
}

void xiiSkyLightComponent::SerializeComponent(xiiWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);

  xiiStreamWriter& s = inout_stream.GetStream();

  m_Desc.m_IncludeTags.Save(s);
  m_Desc.m_ExcludeTags.Save(s);
  s << m_Desc.m_Mode;
  s << m_Desc.m_bShowDebugInfo;
  s << m_Desc.m_fIntensity;
  s << m_Desc.m_fSaturation;
  s << m_hCubeMap;
  s << m_Desc.m_fNearPlane;
  s << m_Desc.m_fFarPlane;
}

void xiiSkyLightComponent::DeserializeComponent(xiiWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  const xiiUInt32  uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());
  xiiStreamReader& s         = inout_stream.GetStream();

  m_Desc.m_IncludeTags.Load(s, xiiTagRegistry::GetGlobalRegistry());
  m_Desc.m_ExcludeTags.Load(s, xiiTagRegistry::GetGlobalRegistry());
  s >> m_Desc.m_Mode;
  s >> m_Desc.m_bShowDebugInfo;
  s >> m_Desc.m_fIntensity;
  s >> m_Desc.m_fSaturation;
  if (uiVersion >= 2)
  {
    s >> m_hCubeMap;
  }
  if (uiVersion >= 3)
  {
    s >> m_Desc.m_fNearPlane;
    s >> m_Desc.m_fFarPlane;
  }
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Lights_Implementation_SkyLightComponent);

#include <RendererCore/RendererCorePCH.h>

#include <Core/Messages/TransformChangedMessage.h>
#include <Core/Messages/UpdateLocalBoundsMessage.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <RendererCore/Lights/Implementation/ReflectionPool.h>
#include <RendererCore/Lights/SkyLightComponent.h>
#include <RendererCore/Pipeline/RenderData.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderWorld/RenderWorld.h>
#include <RendererCore/Textures/TextureCubeResource.h>

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
    XII_ACCESSOR_PROPERTY("Intensity", GetIntensity, SetIntensity)->AddAttributes(new xiiClampValueAttribute(0.0f, xiiVariant()), new xiiDefaultValueAttribute(1.0f)),
    XII_ACCESSOR_PROPERTY("Saturation", GetSaturation, SetSaturation)->AddAttributes(new xiiClampValueAttribute(0.0f, xiiVariant()), new xiiDefaultValueAttribute(1.0f)),
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
    new xiiCategoryAttribute("Rendering/Lighting"),
  }
  XII_END_ATTRIBUTES;
}
XII_END_COMPONENT_TYPE
// clang-format on

xiiSkyLightComponent::xiiSkyLightComponent()
{
  m_Desc.m_uniqueID.CreateNewUuid();
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

void xiiSkyLightComponent::InsertIncludeTag(const char* szTag)
{
  m_Desc.m_IncludeTags.SetByName(szTag);
  m_bStatesDirty = true;
}

void xiiSkyLightComponent::RemoveIncludeTag(const char* szTag)
{
  m_Desc.m_IncludeTags.RemoveByName(szTag);
  m_bStatesDirty = true;
}

const xiiTagSet& xiiSkyLightComponent::GetExcludeTags() const
{
  return m_Desc.m_ExcludeTags;
}

void xiiSkyLightComponent::InsertExcludeTag(const char* szTag)
{
  m_Desc.m_ExcludeTags.SetByName(szTag);
  m_bStatesDirty = true;
}

void xiiSkyLightComponent::RemoveExcludeTag(const char* szTag)
{
  m_Desc.m_ExcludeTags.RemoveByName(szTag);
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


void xiiSkyLightComponent::SetCubeMapFile(const char* szFile)
{
  xiiTextureCubeResourceHandle hCubeMap;
  if (!xiiStringUtils::IsNullOrEmpty(szFile))
  {
    hCubeMap = xiiResourceManager::LoadResource<xiiTextureCubeResource>(szFile);
  }
  m_hCubeMap     = hCubeMap;
  m_bStatesDirty = true;
}

const char* xiiSkyLightComponent::GetCubeMapFile() const
{
  return m_hCubeMap.IsValid() ? m_hCubeMap.GetResourceID().GetData() : "";
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

void xiiSkyLightComponent::SerializeComponent(xiiWorldWriter& ref_stream) const
{
  SUPER::SerializeComponent(ref_stream);

  xiiStreamWriter& s = ref_stream.GetStream();

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

void xiiSkyLightComponent::DeserializeComponent(xiiWorldReader& ref_stream)
{
  SUPER::DeserializeComponent(ref_stream);
  const xiiUInt32  uiVersion = ref_stream.GetComponentTypeVersion(GetStaticRTTI());
  xiiStreamReader& s         = ref_stream.GetStream();

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

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

#include <Foundation/Serialization/GraphPatch.h>

class xiiSkyLightComponentPatch_2_3 : public xiiGraphPatch
{
public:
  xiiSkyLightComponentPatch_2_3() :
    xiiGraphPatch("xiiSkyLightComponent", 3)
  {
  }

  virtual void Patch(xiiGraphPatchContext& ref_context, xiiAbstractObjectGraph* pGraph, xiiAbstractObjectNode* pNode) const override
  {
    // Inline ReflectionData sub-object into the sky light itself.
    if (const xiiAbstractObjectNode::Property* pProp0 = pNode->FindProperty("ReflectionData"))
    {
      if (pProp0->m_Value.IsA<xiiUuid>())
      {
        if (xiiAbstractObjectNode* pSubNode = pGraph->GetNode(pProp0->m_Value.Get<xiiUuid>()))
        {
          for (auto pProp : pSubNode->GetProperties())
          {
            pNode->AddProperty(pProp.m_sPropertyName, pProp.m_Value);
          }
        }
      }
    }
  }
};

xiiSkyLightComponentPatch_2_3 g_xiiSkyLightComponentPatch_2_3;

XII_STATICLINK_FILE(RendererCore, RendererCore_Lights_Implementation_SkyLightComponent);

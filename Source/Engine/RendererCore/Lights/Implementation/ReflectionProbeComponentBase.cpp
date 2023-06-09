#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/Lights/ReflectionProbeComponentBase.h>

#include <Core/Graphics/Camera.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <RendererCore/Pipeline/RenderData.h>
#include <RendererCore/Pipeline/View.h>

namespace
{
  static xiiVariantArray GetDefaultExcludeTags()
  {
    xiiVariantArray value(xiiStaticAllocatorWrapper::GetAllocator());
    value.PushBack(xiiStringView("SkyLight"));
    return value;
  }
} // namespace


// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiReflectionProbeComponentBase, 2, xiiRTTINoAllocator)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ENUM_ACCESSOR_PROPERTY("ReflectionProbeMode", xiiReflectionProbeMode, GetReflectionProbeMode, SetReflectionProbeMode)->AddAttributes(new xiiDefaultValueAttribute(xiiReflectionProbeMode::Static), new xiiGroupAttribute("Capture Description")),
    XII_SET_ACCESSOR_PROPERTY("IncludeTags", GetIncludeTags, InsertIncludeTag, RemoveIncludeTag)->AddAttributes(new xiiTagSetWidgetAttribute("Default")),
    XII_SET_ACCESSOR_PROPERTY("ExcludeTags", GetExcludeTags, InsertExcludeTag, RemoveExcludeTag)->AddAttributes(new xiiTagSetWidgetAttribute("Default"), new xiiDefaultValueAttribute(GetDefaultExcludeTags())),
    XII_ACCESSOR_PROPERTY("NearPlane", GetNearPlane, SetNearPlane)->AddAttributes(new xiiDefaultValueAttribute(0.0f), new xiiClampValueAttribute(0.0f, {}), new xiiMinValueTextAttribute("Auto")),
    XII_ACCESSOR_PROPERTY("FarPlane", GetFarPlane, SetFarPlane)->AddAttributes(new xiiDefaultValueAttribute(100.0f), new xiiClampValueAttribute(0.01f, 10000.0f)),
    XII_ACCESSOR_PROPERTY("CaptureOffset", GetCaptureOffset, SetCaptureOffset),
    XII_ACCESSOR_PROPERTY("ShowDebugInfo", GetShowDebugInfo, SetShowDebugInfo),
    XII_ACCESSOR_PROPERTY("ShowMipMaps", GetShowMipMaps, SetShowMipMaps),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES
  {
    new xiiTransformManipulatorAttribute("CaptureOffset"),
  }
  XII_END_ATTRIBUTES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiReflectionProbeComponentBase::xiiReflectionProbeComponentBase()
{
  m_Desc.m_uniqueID.CreateNewUuid();
}

xiiReflectionProbeComponentBase::~xiiReflectionProbeComponentBase() = default;

void xiiReflectionProbeComponentBase::SetReflectionProbeMode(xiiEnum<xiiReflectionProbeMode> mode)
{
  m_Desc.m_Mode  = mode;
  m_bStatesDirty = true;
}

xiiEnum<xiiReflectionProbeMode> xiiReflectionProbeComponentBase::GetReflectionProbeMode() const
{
  return m_Desc.m_Mode;
}

const xiiTagSet& xiiReflectionProbeComponentBase::GetIncludeTags() const
{
  return m_Desc.m_IncludeTags;
}

void xiiReflectionProbeComponentBase::InsertIncludeTag(const char* szTag)
{
  m_Desc.m_IncludeTags.SetByName(szTag);
  m_bStatesDirty = true;
}

void xiiReflectionProbeComponentBase::RemoveIncludeTag(const char* szTag)
{
  m_Desc.m_IncludeTags.RemoveByName(szTag);
  m_bStatesDirty = true;
}


const xiiTagSet& xiiReflectionProbeComponentBase::GetExcludeTags() const
{
  return m_Desc.m_ExcludeTags;
}

void xiiReflectionProbeComponentBase::InsertExcludeTag(const char* szTag)
{
  m_Desc.m_ExcludeTags.SetByName(szTag);
  m_bStatesDirty = true;
}

void xiiReflectionProbeComponentBase::RemoveExcludeTag(const char* szTag)
{
  m_Desc.m_ExcludeTags.RemoveByName(szTag);
  m_bStatesDirty = true;
}

void xiiReflectionProbeComponentBase::SetNearPlane(float fNearPlane)
{
  m_Desc.m_fNearPlane = fNearPlane;
  m_bStatesDirty      = true;
}

void xiiReflectionProbeComponentBase::SetFarPlane(float fFarPlane)
{
  m_Desc.m_fFarPlane = fFarPlane;
  m_bStatesDirty     = true;
}

void xiiReflectionProbeComponentBase::SetCaptureOffset(const xiiVec3& vOffset)
{
  m_Desc.m_vCaptureOffset = vOffset;
  m_bStatesDirty          = true;
}

void xiiReflectionProbeComponentBase::SetShowDebugInfo(bool bShowDebugInfo)
{
  m_Desc.m_bShowDebugInfo = bShowDebugInfo;
  m_bStatesDirty          = true;
}

bool xiiReflectionProbeComponentBase::GetShowDebugInfo() const
{
  return m_Desc.m_bShowDebugInfo;
}

void xiiReflectionProbeComponentBase::SetShowMipMaps(bool bShowMipMaps)
{
  m_Desc.m_bShowMipMaps = bShowMipMaps;
  m_bStatesDirty        = true;
}

bool xiiReflectionProbeComponentBase::GetShowMipMaps() const
{
  return m_Desc.m_bShowMipMaps;
}

void xiiReflectionProbeComponentBase::SerializeComponent(xiiWorldWriter& ref_stream) const
{
  SUPER::SerializeComponent(ref_stream);

  xiiStreamWriter& s = ref_stream.GetStream();

  m_Desc.m_IncludeTags.Save(s);
  m_Desc.m_ExcludeTags.Save(s);
  s << m_Desc.m_Mode;
  s << m_Desc.m_bShowDebugInfo;
  s << m_Desc.m_uniqueID;
  s << m_Desc.m_fNearPlane;
  s << m_Desc.m_fFarPlane;
  s << m_Desc.m_vCaptureOffset;
}

void xiiReflectionProbeComponentBase::DeserializeComponent(xiiWorldReader& ref_stream)
{
  SUPER::DeserializeComponent(ref_stream);
  //const xiiUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  xiiStreamReader& s = ref_stream.GetStream();

  m_Desc.m_IncludeTags.Load(s, xiiTagRegistry::GetGlobalRegistry());
  m_Desc.m_ExcludeTags.Load(s, xiiTagRegistry::GetGlobalRegistry());
  s >> m_Desc.m_Mode;
  s >> m_Desc.m_bShowDebugInfo;
  s >> m_Desc.m_uniqueID;
  s >> m_Desc.m_fNearPlane;
  s >> m_Desc.m_fFarPlane;
  s >> m_Desc.m_vCaptureOffset;
}

float xiiReflectionProbeComponentBase::ComputePriority(xiiMsgExtractRenderData& msg, xiiReflectionProbeRenderData* pRenderData, float fVolume, const xiiVec3& vScale) const
{
  float       fPriority  = 0.0f;
  const float fLogVolume = xiiMath::Log2(1.0f + fVolume); // +1 to make sure it never goes negative.
  // This sorting is only by size to make sure the probes in a cluster are iterating from smallest to largest on the GPU. Which probes are actually used is determined below by the returned priority.
  pRenderData->m_uiSortingKey = xiiMath::FloatToInt(static_cast<float>(xiiMath::MaxValue<xiiUInt32>()) * fLogVolume / 40.0f);

  //#TODO This is a pretty poor distance / size based score.
  if (msg.m_pView)
  {
    if (auto pCamera = msg.m_pView->GetLodCamera())
    {
      float fDistance = (pCamera->GetPosition() - pRenderData->m_GlobalTransform.m_vPosition).GetLength();
      float fRadius   = (xiiMath::Abs(vScale.x) + xiiMath::Abs(vScale.y) + xiiMath::Abs(vScale.z)) / 3.0f;
      fPriority       = fRadius / fDistance;
    }
  }

#ifdef XII_SHOW_REFLECTION_PROBE_PRIORITIES
  xiiStringBuilder s;
  s.Format("{}, {}", pRenderData->m_uiSortingKey, fPriority);
  xiiDebugRenderer::Draw3DText(GetWorld(), s, pRenderData->m_GlobalTransform.m_vPosition, xiiColor::Wheat);
#endif
  return fPriority;
}

XII_STATICLINK_FILE(RendererCore, RendererCore_Lights_Implementation_ReflectionProbeComponentBase);

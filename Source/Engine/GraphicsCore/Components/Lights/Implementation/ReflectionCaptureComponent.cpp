/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GraphicsCore/Components/Lights/ReflectionCaptureComponent.h>
#include <GraphicsCore/Pipeline/MsgExtractRenderData.h>
#include <GraphicsCore/Pipeline/RenderWorldModule.h>

// clang-format off
XII_BEGIN_STATIC_REFLECTED_ENUM(xiiReflectionProbeInfluenceShape, 1)
  XII_ENUM_CONSTANT(xiiReflectionProbeInfluenceShape::Sphere),
  XII_ENUM_CONSTANT(xiiReflectionProbeInfluenceShape::Box)
XII_END_STATIC_REFLECTED_ENUM;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiReflectionCaptureRenderData, 1, xiiRTTIDefaultAllocator<xiiReflectionCaptureRenderData>)
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_COMPONENT_TYPE(xiiReflectionCaptureComponent, 1, xiiComponentMode::Static)
{
  XII_BEGIN_PROPERTIES
  {
    XII_RESOURCE_ACCESSOR_PROPERTY("ReflectionMap", GetReflectionMap, SetReflectionMap)->AddAttributes(new xiiAssetBrowserAttribute("CompatibleAsset_Texture_Cube", xiiDependencyFlags::Package)),
    XII_ENUM_ACCESSOR_PROPERTY("InfluenceShape", xiiReflectionProbeInfluenceShape, GetInfluenceShape, SetInfluenceShape),
    XII_ACCESSOR_PROPERTY("HalfExtents", GetHalfExtents, SetHalfExtents)->AddAttributes(new xiiDefaultValueAttribute(xiiVec3(5.0f)), new xiiClampValueAttribute(xiiVec3(0.01f), xiiVariant()), new xiiSuffixAttribute(" m")),
    XII_ACCESSOR_PROPERTY("SphereRadius", GetSphereRadius, SetSphereRadius)->AddAttributes(new xiiDefaultValueAttribute(5.0f), new xiiClampValueAttribute(0.01f, xiiVariant()), new xiiSuffixAttribute(" m")),
    XII_ACCESSOR_PROPERTY("BlendDistance", GetBlendDistance, SetBlendDistance)->AddAttributes(new xiiDefaultValueAttribute(1.0f), new xiiClampValueAttribute(0.0f, xiiVariant()), new xiiSuffixAttribute(" m")),
    XII_ACCESSOR_PROPERTY("Intensity", GetIntensity, SetIntensity)->AddAttributes(new xiiDefaultValueAttribute(1.0f), new xiiClampValueAttribute(0.0f, xiiVariant())),
    XII_ACCESSOR_PROPERTY("Saturation", GetSaturation, SetSaturation)->AddAttributes(new xiiDefaultValueAttribute(1.0f), new xiiClampValueAttribute(0.0f, xiiVariant())),
    XII_ACCESSOR_PROPERTY("Priority", GetPriority, SetPriority)->AddAttributes(new xiiDefaultValueAttribute(0)),
    XII_ACCESSOR_PROPERTY("ParallaxCorrected", GetParallaxCorrected, SetParallaxCorrected)->AddAttributes(new xiiDefaultValueAttribute(true)),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_MESSAGEHANDLERS
  {
    XII_MESSAGE_HANDLER(xiiMsgExtractRenderData, OnMsgExtractRenderData),
  }
  XII_END_MESSAGEHANDLERS;
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("RenderWorld/Lighting/Reflections"),
  }
  XII_END_ATTRIBUTES;
}
XII_END_COMPONENT_TYPE
// clang-format on

xiiReflectionCaptureComponent::xiiReflectionCaptureComponent()  = default;
xiiReflectionCaptureComponent::~xiiReflectionCaptureComponent() = default;

void xiiReflectionCaptureComponent::SerializeComponent(xiiWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  xiiStreamWriter& stream = inout_stream.GetStream();

  stream << m_hReflectionMap;
  stream << m_InfluenceShape;
  stream << m_vHalfExtents;
  stream << m_fSphereRadius;
  stream << m_fBlendDistance;
  stream << m_fIntensity;
  stream << m_fSaturation;
  stream << m_iPriority;
  stream << m_bParallaxCorrected;
}

void xiiReflectionCaptureComponent::DeserializeComponent(xiiWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  xiiStreamReader& stream = inout_stream.GetStream();

  stream >> m_hReflectionMap;
  stream >> m_InfluenceShape;
  stream >> m_vHalfExtents;
  stream >> m_fSphereRadius;
  stream >> m_fBlendDistance;
  stream >> m_fIntensity;
  stream >> m_fSaturation;
  stream >> m_iPriority;
  stream >> m_bParallaxCorrected;

  m_vHalfExtents   = m_vHalfExtents.CompMax(xiiVec3(0.01f));
  m_fSphereRadius  = xiiMath::Max(m_fSphereRadius, 0.01f);
  m_fBlendDistance = xiiMath::Max(m_fBlendDistance, 0.0f);
  m_fIntensity     = xiiMath::Max(m_fIntensity, 0.0f);
  m_fSaturation    = xiiMath::Max(m_fSaturation, 0.0f);
}

xiiResult xiiReflectionCaptureComponent::GetLocalBounds(xiiBoundingBoxSphere& out_bounds, bool& out_bAlwaysVisible, xiiMsgUpdateLocalBounds& ref_msg)
{
  XII_IGNORE_UNUSED(ref_msg);
  out_bAlwaysVisible = false;

  if (m_InfluenceShape == xiiReflectionProbeInfluenceShape::Sphere)
  {
    out_bounds = xiiBoundingSphere::MakeFromCenterAndRadius(xiiVec3::MakeZero(), m_fSphereRadius);
  }
  else
  {
    out_bounds = xiiBoundingBoxSphere::MakeFromCenterExtents(xiiVec3::MakeZero(), m_vHalfExtents, m_vHalfExtents.GetLength());
  }
  return XII_SUCCESS;
}

void xiiReflectionCaptureComponent::SetReflectionMap(const xiiTextureCubeResourceHandle& hReflectionMap)
{
  if (m_hReflectionMap == hReflectionMap)
    return;
  m_hReflectionMap = hReflectionMap;
  InvalidateCachedRenderData();
}

const xiiTextureCubeResourceHandle& xiiReflectionCaptureComponent::GetReflectionMap() const
{
  return m_hReflectionMap;
}

void xiiReflectionCaptureComponent::SetInfluenceShape(xiiEnum<xiiReflectionProbeInfluenceShape> shape)
{
  if (m_InfluenceShape == shape)
    return;
  m_InfluenceShape = shape;
  TriggerLocalBoundsUpdate();
  InvalidateCachedRenderData();
}

xiiEnum<xiiReflectionProbeInfluenceShape> xiiReflectionCaptureComponent::GetInfluenceShape() const
{
  return m_InfluenceShape;
}

void xiiReflectionCaptureComponent::SetHalfExtents(xiiVec3 vHalfExtents)
{
  vHalfExtents = vHalfExtents.CompMax(xiiVec3(0.01f));
  if (m_vHalfExtents == vHalfExtents)
    return;
  m_vHalfExtents = vHalfExtents;
  TriggerLocalBoundsUpdate();
  InvalidateCachedRenderData();
}

xiiVec3 xiiReflectionCaptureComponent::GetHalfExtents() const
{
  return m_vHalfExtents;
}

void xiiReflectionCaptureComponent::SetSphereRadius(float fRadius)
{
  fRadius = xiiMath::Max(fRadius, 0.01f);
  if (m_fSphereRadius == fRadius)
    return;
  m_fSphereRadius = fRadius;
  TriggerLocalBoundsUpdate();
  InvalidateCachedRenderData();
}

float xiiReflectionCaptureComponent::GetSphereRadius() const
{
  return m_fSphereRadius;
}

void xiiReflectionCaptureComponent::SetBlendDistance(float fDistance)
{
  fDistance = xiiMath::Max(fDistance, 0.0f);
  if (m_fBlendDistance == fDistance)
    return;
  m_fBlendDistance = fDistance;
  InvalidateCachedRenderData();
}

float xiiReflectionCaptureComponent::GetBlendDistance() const
{
  return m_fBlendDistance;
}

void xiiReflectionCaptureComponent::SetIntensity(float fIntensity)
{
  fIntensity = xiiMath::Max(fIntensity, 0.0f);
  if (m_fIntensity == fIntensity)
    return;
  m_fIntensity = fIntensity;
  InvalidateCachedRenderData();
}

float xiiReflectionCaptureComponent::GetIntensity() const
{
  return m_fIntensity;
}

void xiiReflectionCaptureComponent::SetSaturation(float fSaturation)
{
  fSaturation = xiiMath::Max(fSaturation, 0.0f);
  if (m_fSaturation == fSaturation)
    return;
  m_fSaturation = fSaturation;
  InvalidateCachedRenderData();
}

float xiiReflectionCaptureComponent::GetSaturation() const
{
  return m_fSaturation;
}

void xiiReflectionCaptureComponent::SetPriority(xiiInt32 iPriority)
{
  if (m_iPriority == iPriority)
    return;
  m_iPriority = iPriority;
  InvalidateCachedRenderData();
}

xiiInt32 xiiReflectionCaptureComponent::GetPriority() const
{
  return m_iPriority;
}

void xiiReflectionCaptureComponent::SetParallaxCorrected(bool bEnabled)
{
  if (m_bParallaxCorrected == bEnabled)
    return;
  m_bParallaxCorrected = bEnabled;
  InvalidateCachedRenderData();
}

bool xiiReflectionCaptureComponent::GetParallaxCorrected() const
{
  return m_bParallaxCorrected;
}

void xiiReflectionCaptureComponent::OnMsgExtractRenderData(xiiMsgExtractRenderData& ref_msg) const
{
  if (ref_msg.m_pView == nullptr || ref_msg.m_pExtractedRenderData == nullptr || !m_hReflectionMap.IsValid() || m_fIntensity <= 0.0f)
    return;

  const xiiRenderWorldModule* pWorldModule = GetWorld()->GetModule<xiiRenderWorldModule>();
  if (pWorldModule == nullptr)
    return;

  xiiReflectionCaptureRenderData* pRenderData = pWorldModule->CreateRenderDataForThisFrame<xiiReflectionCaptureRenderData>(this);
  pRenderData->m_hReflectionMap               = m_hReflectionMap;
  pRenderData->m_GlobalTransform              = GetOwner()->GetGlobalTransform();
  pRenderData->m_vHalfExtents                 = m_vHalfExtents;
  pRenderData->m_InfluenceShape               = m_InfluenceShape;
  pRenderData->m_fSphereRadius                = m_fSphereRadius;
  pRenderData->m_fBlendDistance               = m_fBlendDistance;
  pRenderData->m_fIntensity                   = m_fIntensity;
  pRenderData->m_fSaturation                  = m_fSaturation;
  pRenderData->m_iPriority                    = m_iPriority;
  pRenderData->m_bParallaxCorrected           = m_bParallaxCorrected;
  pRenderData->m_uiSortingKey                 = GetUniqueIdForRendering();

  ref_msg.AddRenderData(pRenderData, xiiRenderData::Caching::IfStatic);
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Lights_Implementation_ReflectionCaptureComponent);

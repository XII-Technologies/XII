#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/Lights/BoxReflectionProbeComponent.h>

#include <../../Data/Base/Shaders/Common/LightData.h>
#include <Core/Messages/TransformChangedMessage.h>
#include <Core/Messages/UpdateLocalBoundsMessage.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <RendererCore/Lights/Implementation/ReflectionPool.h>
#include <RendererCore/Pipeline/View.h>

// clang-format off
XII_BEGIN_COMPONENT_TYPE(xiiBoxReflectionProbeComponent, 2, xiiComponentMode::Static)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ACCESSOR_PROPERTY("Extents", GetExtents, SetExtents)->AddAttributes(new xiiClampValueAttribute(xiiVec3(0.0f), {}), new xiiDefaultValueAttribute(xiiVec3(5.0f))),
    XII_ACCESSOR_PROPERTY("InfluenceScale", GetInfluenceScale, SetInfluenceScale)->AddAttributes(new xiiClampValueAttribute(xiiVec3(0.0f), xiiVec3(1.0f)), new xiiDefaultValueAttribute(xiiVec3(1.0f))),
    XII_ACCESSOR_PROPERTY("InfluenceShift", GetInfluenceShift, SetInfluenceShift)->AddAttributes(new xiiClampValueAttribute(xiiVec3(-1.0f), xiiVec3(1.0f)), new xiiDefaultValueAttribute(xiiVec3(0.0f))),
    XII_ACCESSOR_PROPERTY("PositiveFalloff", GetPositiveFalloff, SetPositiveFalloff)->AddAttributes(new xiiClampValueAttribute(xiiVec3(0.0f), xiiVec3(1.0f)), new xiiDefaultValueAttribute(xiiVec3(0.1f, 0.1f, 0.0f))),
    XII_ACCESSOR_PROPERTY("NegativeFalloff", GetNegativeFalloff, SetNegativeFalloff)->AddAttributes(new xiiClampValueAttribute(xiiVec3(0.0f), xiiVec3(1.0f)), new xiiDefaultValueAttribute(xiiVec3(0.1f, 0.1f, 0.0f))),
    XII_ACCESSOR_PROPERTY("BoxProjection", GetBoxProjection, SetBoxProjection)->AddAttributes(new xiiDefaultValueAttribute(true)),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_FUNCTIONS
  {
    XII_FUNCTION_PROPERTY(OnObjectCreated),
  }
  XII_END_FUNCTIONS;
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
    new xiiBoxVisualizerAttribute("Extents", 1.0f, xiiColorScheme::LightUI(xiiColorScheme::Blue)),
    new xiiBoxManipulatorAttribute("Extents", 1.0f, true),
    new xiiBoxReflectionProbeVisualizerAttribute("Extents", "InfluenceScale", "InfluenceShift"),
  }
  XII_END_ATTRIBUTES;
}
XII_END_COMPONENT_TYPE

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiBoxReflectionProbeVisualizerAttribute, 1, xiiRTTIDefaultAllocator<xiiBoxReflectionProbeVisualizerAttribute>)
{
  XII_BEGIN_FUNCTIONS
  {
    XII_CONSTRUCTOR_PROPERTY(const char*, const char*, const char*),
  }
  XII_END_FUNCTIONS;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiBoxReflectionProbeComponentManager::xiiBoxReflectionProbeComponentManager(xiiWorld* pWorld) :
  xiiComponentManager<xiiBoxReflectionProbeComponent, xiiBlockStorageType::Compact>(pWorld)
{
}

//////////////////////////////////////////////////////////////////////////

xiiBoxReflectionProbeComponent::xiiBoxReflectionProbeComponent()  = default;
xiiBoxReflectionProbeComponent::~xiiBoxReflectionProbeComponent() = default;

void xiiBoxReflectionProbeComponent::SetExtents(const xiiVec3& extents)
{
  m_vExtents = extents;
}

const xiiVec3& xiiBoxReflectionProbeComponent::GetInfluenceScale() const
{
  return m_vInfluenceScale;
}

void xiiBoxReflectionProbeComponent::SetInfluenceScale(const xiiVec3& vInfluenceScale)
{
  m_vInfluenceScale = vInfluenceScale;
}

const xiiVec3& xiiBoxReflectionProbeComponent::GetInfluenceShift() const
{
  return m_vInfluenceShift;
}

void xiiBoxReflectionProbeComponent::SetInfluenceShift(const xiiVec3& vInfluenceShift)
{
  m_vInfluenceShift = vInfluenceShift;
}

void xiiBoxReflectionProbeComponent::SetPositiveFalloff(const xiiVec3& vFalloff)
{
  // Does not affect cube generation so m_bStatesDirty is not set.
  m_vPositiveFalloff = vFalloff.CompClamp(xiiVec3(xiiMath::DefaultEpsilon<float>()), xiiVec3(1.0f));
}

void xiiBoxReflectionProbeComponent::SetNegativeFalloff(const xiiVec3& vFalloff)
{
  // Does not affect cube generation so m_bStatesDirty is not set.
  m_vNegativeFalloff = vFalloff.CompClamp(xiiVec3(xiiMath::DefaultEpsilon<float>()), xiiVec3(1.0f));
}

void xiiBoxReflectionProbeComponent::SetBoxProjection(bool bBoxProjection)
{
  m_bBoxProjection = bBoxProjection;
}

const xiiVec3& xiiBoxReflectionProbeComponent::GetExtents() const
{
  return m_vExtents;
}

void xiiBoxReflectionProbeComponent::OnActivated()
{
  GetOwner()->EnableStaticTransformChangesNotifications();
  m_Id = xiiReflectionPool::RegisterReflectionProbe(GetWorld(), m_Desc, this);
  GetOwner()->UpdateLocalBounds();
}

void xiiBoxReflectionProbeComponent::OnDeactivated()
{
  xiiReflectionPool::DeregisterReflectionProbe(GetWorld(), m_Id);
  m_Id.Invalidate();

  GetOwner()->UpdateLocalBounds();
}

void xiiBoxReflectionProbeComponent::OnObjectCreated(const xiiAbstractObjectNode& node)
{
  m_Desc.m_uniqueID = node.GetGuid();
}

void xiiBoxReflectionProbeComponent::OnUpdateLocalBounds(xiiMsgUpdateLocalBounds& msg)
{
  msg.SetAlwaysVisible(xiiDefaultSpatialDataCategories::RenderDynamic);
}

void xiiBoxReflectionProbeComponent::OnMsgExtractRenderData(xiiMsgExtractRenderData& msg) const
{
  // Don't trigger reflection rendering in shadow or other reflection views.
  if (msg.m_pView->GetCameraUsageHint() == xiiCameraUsageHint::Shadow || msg.m_pView->GetCameraUsageHint() == xiiCameraUsageHint::Reflection)
    return;

  if (m_bStatesDirty)
  {
    m_bStatesDirty = false;
    xiiReflectionPool::UpdateReflectionProbe(GetWorld(), m_Id, m_Desc, this);
  }

  auto pRenderData                = xiiCreateRenderDataForThisFrame<xiiReflectionProbeRenderData>(GetOwner());
  pRenderData->m_GlobalTransform  = GetOwner()->GetGlobalTransform();
  pRenderData->m_vProbePosition   = pRenderData->m_GlobalTransform * m_Desc.m_vCaptureOffset;
  pRenderData->m_vHalfExtents     = m_vExtents / 2.0f;
  pRenderData->m_vInfluenceScale  = m_vInfluenceScale;
  pRenderData->m_vInfluenceShift  = m_vInfluenceShift;
  pRenderData->m_vPositiveFalloff = m_vPositiveFalloff;
  pRenderData->m_vNegativeFalloff = m_vNegativeFalloff;
  pRenderData->m_Id               = m_Id;
  pRenderData->m_uiIndex          = 0;
  if (m_bBoxProjection)
    pRenderData->m_uiIndex |= REFLECTION_PROBE_IS_PROJECTED;

  const xiiVec3 vScale  = pRenderData->m_GlobalTransform.m_vScale.CompMul(m_vExtents);
  const float   fVolume = xiiMath::Abs(vScale.x * vScale.y * vScale.z);

  float fPriority = ComputePriority(msg, pRenderData, fVolume, vScale);
  xiiReflectionPool::ExtractReflectionProbe(this, msg, pRenderData, GetWorld(), m_Id, fPriority);
}

void xiiBoxReflectionProbeComponent::OnTransformChanged(xiiMsgTransformChanged& msg)
{
  m_bStatesDirty = true;
}

void xiiBoxReflectionProbeComponent::SerializeComponent(xiiWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);

  xiiStreamWriter& s = stream.GetStream();

  s << m_vExtents;
  s << m_vInfluenceScale;
  s << m_vInfluenceShift;
  s << m_vPositiveFalloff;
  s << m_vNegativeFalloff;
  s << m_bBoxProjection;
}

void xiiBoxReflectionProbeComponent::DeserializeComponent(xiiWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  const xiiUInt32  uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  xiiStreamReader& s         = stream.GetStream();

  s >> m_vExtents;
  s >> m_vInfluenceScale;
  s >> m_vInfluenceShift;
  s >> m_vPositiveFalloff;
  s >> m_vNegativeFalloff;
  if (uiVersion >= 2)
  {
    s >> m_bBoxProjection;
  }
}

//////////////////////////////////////////////////////////////////////////

xiiBoxReflectionProbeVisualizerAttribute::xiiBoxReflectionProbeVisualizerAttribute() :
  xiiVisualizerAttribute(nullptr)
{
}

xiiBoxReflectionProbeVisualizerAttribute::xiiBoxReflectionProbeVisualizerAttribute(const char* szExtentsProperty, const char* szInfluenceScaleProperty, const char* szInfluenceShiftProperty) :
  xiiVisualizerAttribute(szExtentsProperty, szInfluenceScaleProperty, szInfluenceShiftProperty)
{
}

XII_STATICLINK_FILE(RendererCore, RendererCore_Lights_Implementation_BoxReflectionProbeComponent);

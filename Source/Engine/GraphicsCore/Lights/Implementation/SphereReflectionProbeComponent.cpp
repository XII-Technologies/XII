#include <GraphicsCore/GraphicsCorePCH.h>

#include <GraphicsCore/Lights/SphereReflectionProbeComponent.h>

#include <../../Data/Base/Shaders/Common/LightData.h>
#include <Core/Messages/TransformChangedMessage.h>
#include <Core/Messages/UpdateLocalBoundsMessage.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <GraphicsCore/Lights/Implementation/ReflectionPool.h>
#include <GraphicsCore/Pipeline/View.h>

// clang-format off
XII_BEGIN_COMPONENT_TYPE(xiiSphereReflectionProbeComponent, 2, xiiComponentMode::Static)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ACCESSOR_PROPERTY("Radius", GetRadius, SetRadius)->AddAttributes(new xiiClampValueAttribute(0.0f, {}), new xiiDefaultValueAttribute(5.0f)),
    XII_ACCESSOR_PROPERTY("Falloff", GetFalloff, SetFalloff)->AddAttributes(new xiiClampValueAttribute(0.0f, 1.0f), new xiiDefaultValueAttribute(0.1f)),
    XII_ACCESSOR_PROPERTY("SphereProjection", GetSphereProjection, SetSphereProjection)->AddAttributes(new xiiDefaultValueAttribute(true)),
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
    new xiiSphereVisualizerAttribute("Radius", xiiColorScheme::LightUI(xiiColorScheme::Blue)),
    new xiiSphereManipulatorAttribute("Radius"),
  }
  XII_END_ATTRIBUTES;
}
XII_END_COMPONENT_TYPE
// clang-format on

xiiSphereReflectionProbeComponentManager::xiiSphereReflectionProbeComponentManager(xiiWorld* pWorld) :
  xiiComponentManager<xiiSphereReflectionProbeComponent, xiiBlockStorageType::Compact>(pWorld)
{
}

//////////////////////////////////////////////////////////////////////////

xiiSphereReflectionProbeComponent::xiiSphereReflectionProbeComponent()  = default;
xiiSphereReflectionProbeComponent::~xiiSphereReflectionProbeComponent() = default;

void xiiSphereReflectionProbeComponent::SetRadius(float fRadius)
{
  m_fRadius      = xiiMath::Max(fRadius, 0.0f);
  m_bStatesDirty = true;
}

float xiiSphereReflectionProbeComponent::GetRadius() const
{
  return m_fRadius;
}

void xiiSphereReflectionProbeComponent::SetFalloff(float fFalloff)
{
  m_fFalloff = xiiMath::Clamp(fFalloff, xiiMath::DefaultEpsilon<float>(), 1.0f);
}

void xiiSphereReflectionProbeComponent::SetSphereProjection(bool bSphereProjection)
{
  m_bSphereProjection = bSphereProjection;
}

void xiiSphereReflectionProbeComponent::OnActivated()
{
  GetOwner()->EnableStaticTransformChangesNotifications();
  m_Id = xiiReflectionPool::RegisterReflectionProbe(GetWorld(), m_Desc, this);
  GetOwner()->UpdateLocalBounds();
}

void xiiSphereReflectionProbeComponent::OnDeactivated()
{
  xiiReflectionPool::DeregisterReflectionProbe(GetWorld(), m_Id);
  m_Id.Invalidate();

  GetOwner()->UpdateLocalBounds();
}

void xiiSphereReflectionProbeComponent::OnObjectCreated(const xiiAbstractObjectNode& node)
{
  m_Desc.m_uniqueID = node.GetGuid();
}

void xiiSphereReflectionProbeComponent::OnUpdateLocalBounds(xiiMsgUpdateLocalBounds& msg)
{
  msg.SetAlwaysVisible(xiiDefaultSpatialDataCategories::RenderDynamic);
}

void xiiSphereReflectionProbeComponent::OnMsgExtractRenderData(xiiMsgExtractRenderData& msg) const
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
  pRenderData->m_vHalfExtents     = xiiVec3(m_fRadius);
  pRenderData->m_vInfluenceScale  = xiiVec3(1.0f);
  pRenderData->m_vInfluenceShift  = xiiVec3(0.0f);
  pRenderData->m_vPositiveFalloff = xiiVec3(m_fFalloff);
  pRenderData->m_vNegativeFalloff = xiiVec3(m_fFalloff);
  pRenderData->m_Id               = m_Id;
  pRenderData->m_uiIndex          = REFLECTION_PROBE_IS_SPHERE;
  if (m_bSphereProjection)
    pRenderData->m_uiIndex |= REFLECTION_PROBE_IS_PROJECTED;

  const xiiVec3   vScale           = pRenderData->m_GlobalTransform.m_vScale * m_fRadius;
  constexpr float fSphereConstant  = (4.0f / 3.0f) * xiiMath::Pi<float>();
  const float     fEllipsoidVolume = fSphereConstant * xiiMath::Abs(vScale.x * vScale.y * vScale.z);

  float fPriority = ComputePriority(msg, pRenderData, fEllipsoidVolume, vScale);
  xiiReflectionPool::ExtractReflectionProbe(this, msg, pRenderData, GetWorld(), m_Id, fPriority);
}

void xiiSphereReflectionProbeComponent::OnTransformChanged(xiiMsgTransformChanged& msg)
{
  m_bStatesDirty = true;
}

void xiiSphereReflectionProbeComponent::SerializeComponent(xiiWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);

  xiiStreamWriter& s = inout_stream.GetStream();

  s << m_fRadius;
  s << m_fFalloff;
  s << m_bSphereProjection;
}

void xiiSphereReflectionProbeComponent::DeserializeComponent(xiiWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  const xiiUInt32  uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());
  xiiStreamReader& s         = inout_stream.GetStream();

  s >> m_fRadius;
  s >> m_fFalloff;
  if (uiVersion >= 2)
  {
    s >> m_bSphereProjection;
  }
  else
  {
    m_bSphereProjection = false;
  }
}


//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

#include <Foundation/Serialization/GraphPatch.h>

class xiiSphereReflectionProbeComponent_1_2 : public xiiGraphPatch
{
public:
  xiiSphereReflectionProbeComponent_1_2() :
    xiiGraphPatch("xiiSphereReflectionProbeComponent", 2)
  {
  }

  virtual void Patch(xiiGraphPatchContext& ref_context, xiiAbstractObjectGraph* pGraph, xiiAbstractObjectNode* pNode) const override
  {
    pNode->AddProperty("SphereProjection", false);
  }
};

xiiSphereReflectionProbeComponent_1_2 g_xiiSphereReflectionProbeComponent_1_2;

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Lights_Implementation_SphereReflectionProbeComponent);

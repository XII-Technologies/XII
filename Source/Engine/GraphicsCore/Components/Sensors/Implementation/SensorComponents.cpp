#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GraphicsCore/Components/Sensors/SensorComponents.h>
#include <GraphicsCore/GraphicsCorePCH.h>
#include <GraphicsCore/Pipeline/MsgExtractRenderData.h>
#include <GraphicsCore/Pipeline/RenderWorldModule.h>

#define XII_S_REFLECT(T)                                             \
  XII_BEGIN_DYNAMIC_REFLECTED_TYPE(T, 1, xiiRTTIDefaultAllocator<T>) \
  XII_END_DYNAMIC_REFLECTED_TYPE
XII_S_REFLECT(xiiPhysicsDebugRenderData);
XII_S_REFLECT(xiiFieldVisualizerRenderData);
XII_S_REFLECT(xiiMolecularVisualizerRenderData);
XII_S_REFLECT(xiiParticleSimulationRenderData);
XII_S_REFLECT(xiiLiDARSensorRenderData);
XII_S_REFLECT(xiiDepthSensorRenderData);
XII_S_REFLECT(xiiThermalSensorRenderData);
XII_S_REFLECT(xiiSensorFusionRenderData);

// clang-format off
XII_BEGIN_COMPONENT_TYPE(xiiPhysicsDebugRenderComponent, 1, xiiComponentMode::Static)
{ XII_BEGIN_PROPERTIES { XII_ACCESSOR_PROPERTY("ShowColliders", GetShowColliders, SetShowColliders)->AddAttributes(new xiiDefaultValueAttribute(true)),
  XII_ACCESSOR_PROPERTY("ShowJoints", GetShowJoints, SetShowJoints)->AddAttributes(new xiiDefaultValueAttribute(true)),
  XII_ACCESSOR_PROPERTY("ShowContacts", GetShowContacts, SetShowContacts), }
  XII_END_PROPERTIES;
  XII_BEGIN_MESSAGEHANDLERS { XII_MESSAGE_HANDLER(xiiMsgExtractRenderData, OnMsgExtractRenderData), } XII_END_MESSAGEHANDLERS;
  XII_BEGIN_ATTRIBUTES { new xiiCategoryAttribute("Rendering/Debug"); } XII_END_ATTRIBUTES; } XII_END_COMPONENT_TYPE;

XII_BEGIN_COMPONENT_TYPE(xiiFieldVisualizerComponent, 1, xiiComponentMode::Static)
{ XII_BEGIN_ATTRIBUTES { new xiiCategoryAttribute("Rendering/Debug"); } XII_END_ATTRIBUTES;
  XII_BEGIN_MESSAGEHANDLERS { XII_MESSAGE_HANDLER(xiiMsgExtractRenderData, OnMsgExtractRenderData), } XII_END_MESSAGEHANDLERS; } XII_END_COMPONENT_TYPE;

XII_BEGIN_COMPONENT_TYPE(xiiMolecularVisualizerComponent, 1, xiiComponentMode::Static)
{ XII_BEGIN_ATTRIBUTES { new xiiCategoryAttribute("Rendering/Debug"); } XII_END_ATTRIBUTES;
  XII_BEGIN_MESSAGEHANDLERS { XII_MESSAGE_HANDLER(xiiMsgExtractRenderData, OnMsgExtractRenderData), } XII_END_MESSAGEHANDLERS; } XII_END_COMPONENT_TYPE;

XII_BEGIN_COMPONENT_TYPE(xiiParticleSimulationComponent, 1, xiiComponentMode::Dynamic)
{ XII_BEGIN_PROPERTIES { XII_ACCESSOR_PROPERTY("ParticleCount", GetParticleCount, SetParticleCount)->AddAttributes(new xiiDefaultValueAttribute(10000u)), } XII_END_PROPERTIES;
  XII_BEGIN_MESSAGEHANDLERS { XII_MESSAGE_HANDLER(xiiMsgExtractRenderData, OnMsgExtractRenderData), } XII_END_MESSAGEHANDLERS;
  XII_BEGIN_ATTRIBUTES { new xiiCategoryAttribute("Rendering/Simulation"); } XII_END_ATTRIBUTES; } XII_END_COMPONENT_TYPE;

XII_BEGIN_COMPONENT_TYPE(xiiLiDARSensorComponent, 1, xiiComponentMode::Dynamic)
{ XII_BEGIN_PROPERTIES { XII_ACCESSOR_PROPERTY("RayCount", GetRayCount, SetRayCount)->AddAttributes(new xiiDefaultValueAttribute(64u)),
  XII_ACCESSOR_PROPERTY("Range", GetRange, SetRange)->AddAttributes(new xiiDefaultValueAttribute(100.0f)), } XII_END_PROPERTIES;
  XII_BEGIN_MESSAGEHANDLERS { XII_MESSAGE_HANDLER(xiiMsgExtractRenderData, OnMsgExtractRenderData), } XII_END_MESSAGEHANDLERS;
  XII_BEGIN_ATTRIBUTES { new xiiCategoryAttribute("Rendering/Sensors"); } XII_END_ATTRIBUTES; } XII_END_COMPONENT_TYPE;

XII_BEGIN_COMPONENT_TYPE(xiiDepthSensorComponent, 1, xiiComponentMode::Dynamic)
{ XII_BEGIN_ATTRIBUTES { new xiiCategoryAttribute("Rendering/Sensors"); } XII_END_ATTRIBUTES;
  XII_BEGIN_MESSAGEHANDLERS { XII_MESSAGE_HANDLER(xiiMsgExtractRenderData, OnMsgExtractRenderData), } XII_END_MESSAGEHANDLERS; } XII_END_COMPONENT_TYPE;

XII_BEGIN_COMPONENT_TYPE(xiiThermalSensorComponent, 1, xiiComponentMode::Dynamic)
{ XII_BEGIN_ATTRIBUTES { new xiiCategoryAttribute("Rendering/Sensors"); } XII_END_ATTRIBUTES;
  XII_BEGIN_MESSAGEHANDLERS { XII_MESSAGE_HANDLER(xiiMsgExtractRenderData, OnMsgExtractRenderData), } XII_END_MESSAGEHANDLERS; } XII_END_COMPONENT_TYPE;

XII_BEGIN_COMPONENT_TYPE(xiiSensorFusionComponent, 1, xiiComponentMode::Dynamic)
{ XII_BEGIN_ATTRIBUTES { new xiiCategoryAttribute("Rendering/Sensors"); } XII_END_ATTRIBUTES;
  XII_BEGIN_MESSAGEHANDLERS { XII_MESSAGE_HANDLER(xiiMsgExtractRenderData, OnMsgExtractRenderData), } XII_END_MESSAGEHANDLERS; } XII_END_COMPONENT_TYPE;
// clang-format on

// Shared extract macro
#define XII_EXTRACT_SIMPLE(ClassName, RDType, ...)                               \
  void ClassName::OnMsgExtractRenderData(xiiMsgExtractRenderData& ref_msg) const \
  {                                                                              \
    if (!ref_msg.m_pView || !ref_msg.m_pExtractedRenderData) return;             \
    auto* pWM = GetWorld()->GetModule<xiiRenderWorldModule>();                   \
    if (!pWM) return;                                                            \
    auto* pRD              = pWM->CreateRenderDataForThisFrame<RDType>(this);    \
    pRD->m_GlobalTransform = GetOwner()->GetGlobalTransform();                   \
    pRD->m_GlobalBounds    = GetOwner()->GetGlobalBounds();                      \
    pRD->m_hOwnerObject    = GetOwner()->GetHandle();                            \
    pRD->m_hOwnerComponent = GetHandle();                                        \
    pRD->m_uiSortingKey    = GetUniqueIdForRendering();                          \
    __VA_ARGS__                                                                  \
    ref_msg.AddRenderData(pRD, xiiRenderData::Caching::Never);                   \
  }

#define XII_SENSOR_TRIVIAL_BOUNDS(ClassName)                                                          \
  xiiResult ClassName::GetLocalBounds(xiiBoundingBoxSphere& b, bool& bAV, xiiMsgUpdateLocalBounds& m) \
  {                                                                                                   \
    XII_IGNORE_UNUSED(b);                                                                             \
    XII_IGNORE_UNUSED(m);                                                                             \
    bAV = true;                                                                                       \
    return XII_SUCCESS;                                                                               \
  }

// xiiPhysicsDebugRenderComponent
xiiPhysicsDebugRenderComponent::xiiPhysicsDebugRenderComponent()  = default;
xiiPhysicsDebugRenderComponent::~xiiPhysicsDebugRenderComponent() = default;
XII_SENSOR_TRIVIAL_BOUNDS(xiiPhysicsDebugRenderComponent)
void xiiPhysicsDebugRenderComponent::SerializeComponent(xiiWorldWriter& s) const
{
  SUPER::SerializeComponent(s);
  s.GetStream() << m_bShowColliders << m_bShowJoints << m_bShowContacts;
}
void xiiPhysicsDebugRenderComponent::DeserializeComponent(xiiWorldReader& s)
{
  SUPER::DeserializeComponent(s);
  s.GetStream() >> m_bShowColliders >> m_bShowJoints >> m_bShowContacts;
}
void xiiPhysicsDebugRenderComponent::SetShowColliders(bool b) { m_bShowColliders = b; }
void xiiPhysicsDebugRenderComponent::SetShowJoints(bool b) { m_bShowJoints = b; }
void xiiPhysicsDebugRenderComponent::SetShowContacts(bool b) { m_bShowContacts = b; }
XII_EXTRACT_SIMPLE(xiiPhysicsDebugRenderComponent, xiiPhysicsDebugRenderData,
                   pRD->m_bShowColliders = m_bShowColliders;
                   pRD->m_bShowJoints = m_bShowJoints; pRD->m_bShowContacts = m_bShowContacts;)

// xiiFieldVisualizerComponent
xiiFieldVisualizerComponent::xiiFieldVisualizerComponent()  = default;
xiiFieldVisualizerComponent::~xiiFieldVisualizerComponent() = default;
XII_SENSOR_TRIVIAL_BOUNDS(xiiFieldVisualizerComponent)
void xiiFieldVisualizerComponent::SerializeComponent(xiiWorldWriter& s) const
{
  SUPER::SerializeComponent(s);
  s.GetStream() << m_uiFieldType << m_fScale;
}
void xiiFieldVisualizerComponent::DeserializeComponent(xiiWorldReader& s)
{
  SUPER::DeserializeComponent(s);
  s.GetStream() >> m_uiFieldType >> m_fScale;
}
XII_EXTRACT_SIMPLE(xiiFieldVisualizerComponent, xiiFieldVisualizerRenderData, pRD->m_uiFieldType = m_uiFieldType; pRD->m_fScale = m_fScale;)

// xiiMolecularVisualizerComponent
xiiMolecularVisualizerComponent::xiiMolecularVisualizerComponent()  = default;
xiiMolecularVisualizerComponent::~xiiMolecularVisualizerComponent() = default;
XII_SENSOR_TRIVIAL_BOUNDS(xiiMolecularVisualizerComponent)
void xiiMolecularVisualizerComponent::SerializeComponent(xiiWorldWriter& s) const
{
  SUPER::SerializeComponent(s);
  s.GetStream() << m_fAtomRadius << m_bShowBonds;
}
void xiiMolecularVisualizerComponent::DeserializeComponent(xiiWorldReader& s)
{
  SUPER::DeserializeComponent(s);
  s.GetStream() >> m_fAtomRadius >> m_bShowBonds;
}
XII_EXTRACT_SIMPLE(xiiMolecularVisualizerComponent, xiiMolecularVisualizerRenderData, pRD->m_fAtomRadius = m_fAtomRadius; pRD->m_bShowBonds = m_bShowBonds;)

// xiiParticleSimulationComponent
xiiParticleSimulationComponent::xiiParticleSimulationComponent()  = default;
xiiParticleSimulationComponent::~xiiParticleSimulationComponent() = default;
XII_SENSOR_TRIVIAL_BOUNDS(xiiParticleSimulationComponent)
void xiiParticleSimulationComponent::SerializeComponent(xiiWorldWriter& s) const
{
  SUPER::SerializeComponent(s);
  s.GetStream() << m_uiParticleCount << m_fTimeStep;
}
void xiiParticleSimulationComponent::DeserializeComponent(xiiWorldReader& s)
{
  SUPER::DeserializeComponent(s);
  s.GetStream() >> m_uiParticleCount >> m_fTimeStep;
}
void xiiParticleSimulationComponent::SetParticleCount(xiiUInt32 n) { m_uiParticleCount = xiiMath::Max(n, 1u); }
XII_EXTRACT_SIMPLE(xiiParticleSimulationComponent, xiiParticleSimulationRenderData, pRD->m_uiParticleCount = m_uiParticleCount; pRD->m_fTimeStep = m_fTimeStep;)

// xiiLiDARSensorComponent
xiiLiDARSensorComponent::xiiLiDARSensorComponent()  = default;
xiiLiDARSensorComponent::~xiiLiDARSensorComponent() = default;
XII_SENSOR_TRIVIAL_BOUNDS(xiiLiDARSensorComponent)
void xiiLiDARSensorComponent::SerializeComponent(xiiWorldWriter& s) const
{
  SUPER::SerializeComponent(s);
  s.GetStream() << m_uiRayCount << m_fRange << m_fHFOV;
}
void xiiLiDARSensorComponent::DeserializeComponent(xiiWorldReader& s)
{
  SUPER::DeserializeComponent(s);
  s.GetStream() >> m_uiRayCount >> m_fRange >> m_fHFOV;
}
void xiiLiDARSensorComponent::SetRayCount(xiiUInt32 n) { m_uiRayCount = xiiMath::Max(n, 1u); }
void xiiLiDARSensorComponent::SetRange(float f) { m_fRange = xiiMath::Max(f, 0.1f); }
XII_EXTRACT_SIMPLE(xiiLiDARSensorComponent, xiiLiDARSensorRenderData, pRD->m_uiRayCount = m_uiRayCount; pRD->m_fRange = m_fRange; pRD->m_fHFOV = m_fHFOV;)

// xiiDepthSensorComponent
xiiDepthSensorComponent::xiiDepthSensorComponent()  = default;
xiiDepthSensorComponent::~xiiDepthSensorComponent() = default;
XII_SENSOR_TRIVIAL_BOUNDS(xiiDepthSensorComponent)
void xiiDepthSensorComponent::SerializeComponent(xiiWorldWriter& s) const
{
  SUPER::SerializeComponent(s);
  s.GetStream() << m_uiWidth << m_uiHeight << m_fMinDepth << m_fMaxDepth;
}
void xiiDepthSensorComponent::DeserializeComponent(xiiWorldReader& s)
{
  SUPER::DeserializeComponent(s);
  s.GetStream() >> m_uiWidth >> m_uiHeight >> m_fMinDepth >> m_fMaxDepth;
}
XII_EXTRACT_SIMPLE(xiiDepthSensorComponent, xiiDepthSensorRenderData, pRD->m_uiWidth = m_uiWidth; pRD->m_uiHeight = m_uiHeight; pRD->m_fMinDepth = m_fMinDepth; pRD->m_fMaxDepth = m_fMaxDepth;)

// xiiThermalSensorComponent
xiiThermalSensorComponent::xiiThermalSensorComponent()  = default;
xiiThermalSensorComponent::~xiiThermalSensorComponent() = default;
XII_SENSOR_TRIVIAL_BOUNDS(xiiThermalSensorComponent)
void xiiThermalSensorComponent::SerializeComponent(xiiWorldWriter& s) const
{
  SUPER::SerializeComponent(s);
  s.GetStream() << m_fMinTemp << m_fMaxTemp << m_uiWidth << m_uiHeight;
}
void xiiThermalSensorComponent::DeserializeComponent(xiiWorldReader& s)
{
  SUPER::DeserializeComponent(s);
  s.GetStream() >> m_fMinTemp >> m_fMaxTemp >> m_uiWidth >> m_uiHeight;
}
XII_EXTRACT_SIMPLE(xiiThermalSensorComponent, xiiThermalSensorRenderData, pRD->m_fMinTemp = m_fMinTemp; pRD->m_fMaxTemp = m_fMaxTemp; pRD->m_uiWidth = m_uiWidth; pRD->m_uiHeight = m_uiHeight;)

// xiiSensorFusionComponent
xiiSensorFusionComponent::xiiSensorFusionComponent()  = default;
xiiSensorFusionComponent::~xiiSensorFusionComponent() = default;
XII_SENSOR_TRIVIAL_BOUNDS(xiiSensorFusionComponent)
void xiiSensorFusionComponent::SerializeComponent(xiiWorldWriter& s) const
{
  SUPER::SerializeComponent(s);
  s.GetStream() << m_uiActiveSensorMask;
}
void xiiSensorFusionComponent::DeserializeComponent(xiiWorldReader& s)
{
  SUPER::DeserializeComponent(s);
  s.GetStream() >> m_uiActiveSensorMask;
}
void xiiSensorFusionComponent::SetSensorMask(xiiUInt8 mask) { m_uiActiveSensorMask = mask; }
XII_EXTRACT_SIMPLE(xiiSensorFusionComponent, xiiSensorFusionRenderData, pRD->m_uiActiveSensorMask = m_uiActiveSensorMask;)

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Components_Sensors_Implementation_SensorComponents);

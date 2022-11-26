#include <RecastPlugin/RecastPluginPCH.h>

#include <Core/Interfaces/PhysicsWorldModule.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Foundation/Memory/FrameAllocator.h>
#include <RecastPlugin/Components/MarkPoiVisibleComponent.h>
#include <RecastPlugin/WorldModule/RecastWorldModule.h>

//////////////////////////////////////////////////////////////////////////

// clang-format off
XII_BEGIN_COMPONENT_TYPE(xiiRcMarkPoiVisibleComponent, 1, xiiComponentMode::Static)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Radius", m_fRadius)->AddAttributes(new xiiDefaultValueAttribute(20.0f)),
    XII_MEMBER_PROPERTY("CollisionLayer", m_uiCollisionLayer)->AddAttributes(new xiiDynamicEnumAttribute("PhysicsCollisionLayer")),
  }
  XII_END_PROPERTIES;
}
XII_END_COMPONENT_TYPE
// clang-format on

xiiRcMarkPoiVisibleComponent::xiiRcMarkPoiVisibleComponent()  = default;
xiiRcMarkPoiVisibleComponent::~xiiRcMarkPoiVisibleComponent() = default;

void xiiRcMarkPoiVisibleComponent::SerializeComponent(xiiWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);
  xiiStreamWriter& s = stream.GetStream();

  s << m_fRadius;
  s << m_uiCollisionLayer;
}

void xiiRcMarkPoiVisibleComponent::DeserializeComponent(xiiWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  // const xiiUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  xiiStreamReader& s = stream.GetStream();

  s >> m_fRadius;
  s >> m_uiCollisionLayer;
}

static const xiiInt32 g_iMaxPointsToCheckPerFrame = 30;

void xiiRcMarkPoiVisibleComponent::Update()
{
  if (!IsActiveAndSimulating() || m_pWorldModule == nullptr)
    return;

  if (m_pPhysicsModule == nullptr)
  {
    m_pPhysicsModule = GetWorld()->GetModule<xiiPhysicsWorldModuleInterface>();

    if (m_pPhysicsModule == nullptr)
      return;
  }

  const xiiVec3 vOwnPos = GetOwner()->GetGlobalPosition();

  auto pPoiGraph = m_pWorldModule->AccessNavMeshPointsOfInterestGraph();

  if (pPoiGraph == nullptr)
    return;

  const xiiUInt32 uiCheckTimeStamp        = pPoiGraph->GetCheckVisibilityTimeStamp();
  const xiiUInt32 uiTimeStampFullyVisible = uiCheckTimeStamp | 3U;
  const xiiUInt32 uiTimeStampTopVisible   = uiCheckTimeStamp | 2U;

  const xiiUInt32 uiSkipCheckTimeStamp = uiCheckTimeStamp - 10;

  auto& graph = pPoiGraph->GetGraph();
  auto& POIs  = graph.AccessPoints();

  xiiDynamicArray<xiiUInt32> points(xiiFrameAllocator::GetCurrentAllocator());
  graph.FindPointsOfInterest(vOwnPos, 20.0f, points);

  xiiInt32 iPointsToCheck = g_iMaxPointsToCheckPerFrame;

  for (xiiUInt32 i = 0; i < points.GetCount(); ++i)
  {
    ++m_uiLastFirstCheckedPoint;

    if (m_uiLastFirstCheckedPoint >= points.GetCount())
      m_uiLastFirstCheckedPoint = 0;

    auto& poi = POIs[points[m_uiLastFirstCheckedPoint]];

    if (poi.m_uiVisibleMarker >= uiSkipCheckTimeStamp)
      continue;

    if (--iPointsToCheck <= 0)
      break;

    const xiiVec3 vTargetBottom = poi.m_vFloorPosition + xiiVec3(0, 0, 0.5f);
    xiiVec3       vDirToBottom  = vTargetBottom - vOwnPos;
    const float   fRayLenBottom = vDirToBottom.GetLengthAndNormalize();

    xiiPhysicsCastResult hit;

    if (m_pPhysicsModule->Raycast(hit, vOwnPos, vDirToBottom, fRayLenBottom, xiiPhysicsQueryParameters(m_uiCollisionLayer, xiiPhysicsShapeType::Static)))
    {
      const xiiVec3 vTargetTop = poi.m_vFloorPosition + xiiVec3(0, 0, 1.0f);
      xiiVec3       vDirToTop  = vTargetTop - vOwnPos;
      const float   fRayLenTop = vDirToTop.GetLengthAndNormalize();

      --iPointsToCheck;

      xiiPhysicsCastResult hit2;
      if (m_pPhysicsModule->Raycast(hit2, vOwnPos, vDirToTop, fRayLenTop, xiiPhysicsQueryParameters(m_uiCollisionLayer, xiiPhysicsShapeType::Static)))
      {
        poi.m_uiVisibleMarker = uiCheckTimeStamp;
      }
      else
      {
        poi.m_uiVisibleMarker = uiTimeStampTopVisible;
      }
    }
    else
    {
      poi.m_uiVisibleMarker = uiTimeStampFullyVisible;
    }
  }
}

void xiiRcMarkPoiVisibleComponent::OnSimulationStarted()
{
  m_pWorldModule = GetWorld()->GetOrCreateModule<xiiRecastWorldModule>();
}

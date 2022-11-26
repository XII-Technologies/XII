#include <RecastPlugin/RecastPluginPCH.h>

#include <Core/Interfaces/PhysicsWorldModule.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GameEngine/AI/AgentSteeringComponent.h>
#include <Recast/DetourCrowd.h>
#include <RecastPlugin/Components/SoldierComponent.h>
#include <RecastPlugin/WorldModule/RecastWorldModule.h>
#include <RendererCore/Debug/DebugRenderer.h>

// clang-format off
XII_BEGIN_COMPONENT_TYPE(xiiSoldierComponent, 1, xiiComponentMode::Dynamic)
XII_END_COMPONENT_TYPE
// clang-format on

xiiSoldierComponent::xiiSoldierComponent()  = default;
xiiSoldierComponent::~xiiSoldierComponent() = default;

void xiiSoldierComponent::SerializeComponent(xiiWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);
  // xiiStreamWriter& s = stream.GetStream();
}

void xiiSoldierComponent::DeserializeComponent(xiiWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  // const xiiUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  // xiiStreamReader& s = stream.GetStream();
}

void xiiSoldierComponent::OnSimulationStarted()
{
  xiiAgentSteeringComponent* pSteering = nullptr;
  if (GetOwner()->TryGetComponentOfBaseType<xiiAgentSteeringComponent>(pSteering))
  {
    m_hSteeringComponent = pSteering->GetHandle();

    pSteering->m_SteeringEvents.AddEventHandler(xiiMakeDelegate(&xiiSoldierComponent::SteeringEventHandler, this));
  }

  m_State = State::Idle;
}

void xiiSoldierComponent::Deinitialize()
{
  xiiAgentSteeringComponent* pSteering;
  if (GetWorld()->TryGetComponent(m_hSteeringComponent, pSteering))
  {
    if (pSteering->m_SteeringEvents.HasEventHandler(xiiMakeDelegate(&xiiSoldierComponent::SteeringEventHandler, this)))
    {
      pSteering->m_SteeringEvents.RemoveEventHandler(xiiMakeDelegate(&xiiSoldierComponent::SteeringEventHandler, this));
    }
  }

  SUPER::Deinitialize();
}

void xiiSoldierComponent::Update()
{
  if (!IsActiveAndSimulating())
    return;

  if (m_State == State::Idle)
  {
    xiiRecastWorldModule* pRecastModule = GetWorld()->GetOrCreateModule<xiiRecastWorldModule>();

    if (pRecastModule == nullptr)
      return;

    xiiAgentSteeringComponent* pSteering = nullptr;
    if (!GetWorld()->TryGetComponent(m_hSteeringComponent, pSteering))
      return;

    const auto pPoiGraph = pRecastModule->GetNavMeshPointsOfInterestGraph();

    if (pPoiGraph == nullptr)
      return;


    xiiVec3 vNewTargetPos;
    bool    bFoundAny = false;

    {
      const auto& graph = pPoiGraph->GetGraph();

      const xiiUInt32 uiTimestamp = pPoiGraph->GetCheckVisibilityTimeStamp() - 10;

      const xiiVec3 vOwnPos = GetOwner()->GetGlobalPosition();

      xiiDynamicArray<xiiUInt32> points;
      graph.FindPointsOfInterest(vOwnPos, 10.0, points);

      float fBestDistance = 1000;

      for (xiiUInt32 i = 0; i < points.GetCount(); ++i)
      {
        const xiiUInt32 marker = graph.GetPoints()[points[i]].m_uiVisibleMarker;

        const bool bHalfVisible = (marker >= uiTimestamp) && ((marker & 3U) == 2);
        const bool bInvisible   = (marker < uiTimestamp) || ((marker & 3U) == 0);

        const xiiVec3 ptPos = graph.GetPoints()[points[i]].m_vFloorPosition;
        const float   fDist = (vOwnPos - ptPos).GetLength();

        if (bHalfVisible) // top visible, bottom invisible
        {
          if (fDist < fBestDistance)
          {
            bFoundAny     = true;
            fBestDistance = fDist;
            vNewTargetPos = ptPos;
          }
        }

        if (bInvisible)
        {
          if (fDist * 2 < fBestDistance)
          {
            bFoundAny     = true;
            fBestDistance = fDist * 2;
            vNewTargetPos = ptPos;
          }
        }
      }
    }

    if (bFoundAny)
    {
      pSteering->SetTargetPosition(vNewTargetPos);
      m_State = State::WaitingForPath;
    }
  }
}

void xiiSoldierComponent::SteeringEventHandler(const xiiAgentSteeringEvent& e)
{
  switch (e.m_Type)
  {
    case xiiAgentSteeringEvent::TargetReached:
    case xiiAgentSteeringEvent::TargetCleared:
    case xiiAgentSteeringEvent::ErrorInvalidTargetPosition:
    case xiiAgentSteeringEvent::ErrorNoPathToTarget:
    case xiiAgentSteeringEvent::WarningNoFullPathToTarget:
    {
      e.m_pComponent->ClearTargetPosition();
      m_State = State::Idle;
    }
    break;

    case xiiAgentSteeringEvent::PathToTargetFound:
    {
      m_State = State::Walking;
    }
    break;

    case xiiAgentSteeringEvent::ErrorOutsideNavArea:
    case xiiAgentSteeringEvent::ErrorSteeringFailed:
    {
      XII_ASSERT_DEV(m_State != State::ErrorState, "Multi-error state?");

      e.m_pComponent->ClearTargetPosition();

      m_State = State::ErrorState;
      xiiLog::Error("NPC is now in error state");
    }
    break;
  }
}

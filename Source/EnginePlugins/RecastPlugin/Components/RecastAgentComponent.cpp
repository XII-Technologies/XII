#include <RecastPlugin/RecastPluginPCH.h>

#include <Core/Interfaces/PhysicsWorldModule.h>
#include <Core/ResourceManager/ResourceManager.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GameEngine/Physics/CharacterControllerComponent.h>
#include <Recast/DetourCrowd.h>
#include <RecastPlugin/Components/RecastAgentComponent.h>
#include <RecastPlugin/Resources/RecastNavMeshResource.h>
#include <RecastPlugin/Utils/RcMath.h>
#include <RecastPlugin/WorldModule/RecastWorldModule.h>
#include <RendererCore/Debug/DebugRenderer.h>

//////////////////////////////////////////////////////////////////////////

// clang-format off
XII_BEGIN_COMPONENT_TYPE(xiiRcAgentComponent, 2, xiiComponentMode::Dynamic)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("WalkSpeed",m_fWalkSpeed)->AddAttributes(new xiiDefaultValueAttribute(4.0f)),
  }
  XII_END_PROPERTIES;
}
XII_END_COMPONENT_TYPE
// clang-format on

xiiRcAgentComponent::xiiRcAgentComponent() {}
xiiRcAgentComponent::~xiiRcAgentComponent() {}

void xiiRcAgentComponent::SerializeComponent(xiiWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);
  xiiStreamWriter& s = stream.GetStream();

  s << m_fWalkSpeed;
}

void xiiRcAgentComponent::DeserializeComponent(xiiWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  // const xiiUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  xiiStreamReader& s = stream.GetStream();

  s >> m_fWalkSpeed;
}

xiiResult xiiRcAgentComponent::InitializeRecast()
{
  if (m_bRecastInitialized)
    return XII_SUCCESS;

  const dtNavMesh* pNavMesh = GetWorld()->GetOrCreateModule<xiiRecastWorldModule>()->GetDetourNavMesh();
  if (pNavMesh == nullptr)
    return XII_FAILURE;

  m_bRecastInitialized = true;

  m_pQuery    = XII_DEFAULT_NEW(dtNavMeshQuery);
  m_pCorridor = XII_DEFAULT_NEW(dtPathCorridor);

  /// \todo Hard-coded limits
  m_pQuery->init(pNavMesh, 512);
  m_pCorridor->init(256);

  return XII_SUCCESS;
}

void xiiRcAgentComponent::UninitializeRecast()
{
  if (!m_bRecastInitialized)
    return;

  m_bRecastInitialized = false;
  m_pQuery.Clear();
  m_pCorridor.Clear();

  if (m_PathToTargetState != xiiAgentPathFindingState::HasNoTarget)
    SetTargetPosition(m_vTargetPosition);
  else
    ClearTargetPosition();
}

void xiiRcAgentComponent::ClearTargetPosition()
{
  m_iNumNextSteps  = 0;
  m_iFirstNextStep = 0;
  m_PathCorridor.Clear();
  m_vCurrentSteeringDirection.SetZero();

  if (m_PathToTargetState != xiiAgentPathFindingState::HasNoTarget)
  {
    m_PathToTargetState = xiiAgentPathFindingState::HasNoTarget;

    xiiAgentSteeringEvent e;
    e.m_pComponent = this;
    e.m_Type       = xiiAgentSteeringEvent::TargetCleared;

    m_SteeringEvents.Broadcast(e, 1);
  }
}

xiiAgentPathFindingState::Enum xiiRcAgentComponent::GetPathToTargetState() const
{
  return m_PathToTargetState;
}

void xiiRcAgentComponent::SetTargetPosition(const xiiVec3& vPos)
{
  ClearTargetPosition();

  m_vTargetPosition   = vPos;
  m_PathToTargetState = xiiAgentPathFindingState::HasTargetWaitingForPath;
}

xiiVec3 xiiRcAgentComponent::GetTargetPosition() const
{
  return m_vTargetPosition;
}

xiiResult xiiRcAgentComponent::FindNavMeshPolyAt(const xiiVec3& vPosition, dtPolyRef& out_PolyRef, xiiVec3* out_vAdjustedPosition /*= nullptr*/, float fPlaneEpsilon /*= 0.01f*/, float fHeightEpsilon /*= 1.0f*/) const
{
  xiiRcPos rcPos = vPosition;
  xiiVec3  vSize(fPlaneEpsilon, fHeightEpsilon, fPlaneEpsilon);

  xiiRcPos      resultPos;
  dtQueryFilter filter; /// \todo Hard-coded filter
  if (dtStatusFailed(m_pQuery->findNearestPoly(rcPos, &vSize.x, &m_QueryFilter, &out_PolyRef, resultPos)))
    return XII_FAILURE;

  if (!xiiMath::IsEqual(vPosition.x, resultPos.m_Pos[0], fPlaneEpsilon) || !xiiMath::IsEqual(vPosition.y, resultPos.m_Pos[2], fPlaneEpsilon) || !xiiMath::IsEqual(vPosition.z, resultPos.m_Pos[1], fHeightEpsilon))
    return XII_FAILURE;

  if (out_vAdjustedPosition != nullptr)
  {
    *out_vAdjustedPosition = resultPos;
  }

  return XII_SUCCESS;
}

xiiResult xiiRcAgentComponent::ComputePathCorridor(dtPolyRef startPoly, dtPolyRef endPoly, bool& bFoundPartialPath)
{
  bFoundPartialPath = false;

  xiiRcPos rcStart = m_vCurrentPositionOnNavmesh;
  xiiRcPos rcEnd   = m_vTargetPosition;

  xiiInt32 iPathCorridorLength = 0;

  // make enough room
  m_PathCorridor.SetCountUninitialized(256);
  if (dtStatusFailed(m_pQuery->findPath(startPoly, endPoly, rcStart, rcEnd, &m_QueryFilter, m_PathCorridor.GetData(), &iPathCorridorLength, (int)m_PathCorridor.GetCount())) || iPathCorridorLength <= 0)
  {
    m_PathCorridor.Clear();
    return XII_FAILURE;
  }

  // reduce to actual length
  m_PathCorridor.SetCountUninitialized(iPathCorridorLength);

  if (m_PathCorridor[iPathCorridorLength - 1] != endPoly)
  {
    // if this is the case, the target position cannot be reached, but we can walk close to it
    bFoundPartialPath = true;
  }

  m_pCorridor->reset(startPoly, rcStart);
  m_pCorridor->setCorridor(rcEnd, m_PathCorridor.GetData(), iPathCorridorLength);

  return XII_SUCCESS;
}

xiiResult xiiRcAgentComponent::ComputePathToTarget()
{
  const xiiVec3 vStartPos = GetOwner()->GetGlobalPosition();

  dtPolyRef startPoly;
  if (FindNavMeshPolyAt(vStartPos, startPoly, &m_vCurrentPositionOnNavmesh).Failed())
  {
    m_PathToTargetState = xiiAgentPathFindingState::HasTargetPathFindingFailed;

    xiiAgentSteeringEvent e;
    e.m_pComponent = this;
    e.m_Type       = xiiAgentSteeringEvent::ErrorOutsideNavArea;
    m_SteeringEvents.Broadcast(e);
    return XII_FAILURE;
  }

  dtPolyRef endPoly;
  if (FindNavMeshPolyAt(m_vTargetPosition, endPoly).Failed())
  {
    m_PathToTargetState = xiiAgentPathFindingState::HasTargetPathFindingFailed;

    xiiAgentSteeringEvent e;
    e.m_pComponent = this;
    e.m_Type       = xiiAgentSteeringEvent::ErrorInvalidTargetPosition;
    m_SteeringEvents.Broadcast(e);
    return XII_FAILURE;
  }

  /// \todo Optimize case when endPoly is same as previously ?

  bool bFoundPartialPath = false;
  if (ComputePathCorridor(startPoly, endPoly, bFoundPartialPath).Failed() || bFoundPartialPath)
  {
    m_PathToTargetState = xiiAgentPathFindingState::HasTargetPathFindingFailed;

    /// \todo For now a partial path is considered an error

    xiiAgentSteeringEvent e;
    e.m_pComponent = this;
    e.m_Type       = bFoundPartialPath ? xiiAgentSteeringEvent::WarningNoFullPathToTarget : xiiAgentSteeringEvent::ErrorNoPathToTarget;
    m_SteeringEvents.Broadcast(e);
    return XII_FAILURE;
  }

  m_PathToTargetState = xiiAgentPathFindingState::HasTargetAndValidPath;

  xiiAgentSteeringEvent e;
  e.m_pComponent = this;
  e.m_Type       = xiiAgentSteeringEvent::PathToTargetFound;
  m_SteeringEvents.Broadcast(e);
  return XII_SUCCESS;
}

bool xiiRcAgentComponent::HasReachedPosition(const xiiVec3& pos, float fMaxDistance) const
{
  xiiVec3 vTargetPos = pos;
  xiiVec3 vOwnPos    = GetOwner()->GetGlobalPosition();

  /// \todo The comment below may not always be true
  const float fCellHeight = 1.5f;

  // agent component is assumed to be located on the ground (independent of character height)
  // so max error is dependent on the navmesh resolution mostly (cell height)
  const float fHeightError = fCellHeight;

  if (!xiiMath::IsInRange(vTargetPos.z, vOwnPos.z - fHeightError, vOwnPos.z + fHeightError))
    return false;

  vTargetPos.z = 0;
  vOwnPos.z    = 0;

  return (vTargetPos - vOwnPos).GetLengthSquared() < xiiMath::Square(fMaxDistance);
}

bool xiiRcAgentComponent::HasReachedGoal(float fMaxDistance) const
{
  if (GetPathToTargetState() == xiiAgentPathFindingState::HasNoTarget)
    return true;

  return HasReachedPosition(m_vTargetPosition, fMaxDistance);
}

void xiiRcAgentComponent::PlanNextSteps()
{
  if (m_PathCorridor.IsEmpty())
    return;

  xiiUInt8  stepFlags[16];
  dtPolyRef stepPolys[16];

  m_iFirstNextStep = 0;
  m_iNumNextSteps  = m_pCorridor->findCorners(&m_vNextSteps[0].x, stepFlags, stepPolys, 4, m_pQuery.Borrow(), &m_QueryFilter);

  // convert from Recast convention (Y up) to XII (Z up)
  for (xiiInt32 i = 0; i < m_iNumNextSteps; ++i)
  {
    xiiMath::Swap(m_vNextSteps[i].y, m_vNextSteps[i].z);
  }
}

bool xiiRcAgentComponent::IsPositionVisible(const xiiVec3& pos) const
{
  xiiRcPos endPos = pos;

  dtRaycastHit hit;
  if (dtStatusFailed(m_pQuery->raycast(m_pCorridor->getFirstPoly(), m_pCorridor->getPos(), endPos, &m_QueryFilter, 0, &hit)))
    return false;

  // 'visible' if no hit was detected
  return (hit.t > 100000.0f);
}

void xiiRcAgentComponent::OnSimulationStarted()
{
  ClearTargetPosition();

  m_bRecastInitialized = false;

  xiiCharacterControllerComponent* pCC = nullptr;
  if (GetOwner()->TryGetComponentOfBaseType<xiiCharacterControllerComponent>(pCC))
  {
    m_hCharacterController = pCC->GetHandle();
  }
}

void xiiRcAgentComponent::ApplySteering(const xiiVec3& vDirection, float fSpeed)
{
  // compute new rotation
  {
    xiiQuat qDesiredNewRotation;
    qDesiredNewRotation.SetShortestRotation(xiiVec3(1, 0, 0), vDirection);

    /// \todo Pass through character controller
    GetOwner()->SetGlobalRotation(qDesiredNewRotation);
  }

  if (!m_hCharacterController.IsInvalidated())
  {
    xiiCharacterControllerComponent* pCharacter = nullptr;
    if (GetWorld()->TryGetComponent(m_hCharacterController, pCharacter))
    {
      // the character controller already applies time scaling
      const xiiVec3 vRelativeSpeed = (-GetOwner()->GetGlobalRotation() * vDirection) * fSpeed;

      xiiMsgMoveCharacterController msg;
      msg.m_fMoveForwards  = xiiMath::Max(0.0f, vRelativeSpeed.x);
      msg.m_fMoveBackwards = xiiMath::Max(0.0f, -vRelativeSpeed.x);
      msg.m_fStrafeLeft    = xiiMath::Max(0.0f, -vRelativeSpeed.y);
      msg.m_fStrafeRight   = xiiMath::Max(0.0f, vRelativeSpeed.y);

      pCharacter->MoveCharacter(msg);
    }
  }
  else
  {
    const float   fTimeDiff           = (float)GetWorld()->GetClock().GetTimeDiff().GetSeconds();
    const xiiVec3 vOwnerPos           = GetOwner()->GetGlobalPosition();
    const xiiVec3 vDesiredNewPosition = vOwnerPos + vDirection * fSpeed * fTimeDiff;

    GetOwner()->SetGlobalPosition(vDesiredNewPosition);
  }
}

void xiiRcAgentComponent::SyncSteeringWithReality()
{
  const xiiRcPos rcCurrentAgentPosition = GetOwner()->GetGlobalPosition();

  if (!m_pCorridor->movePosition(rcCurrentAgentPosition, m_pQuery.Borrow(), &m_QueryFilter))
  {
    xiiAgentSteeringEvent e;
    e.m_pComponent = this;
    e.m_Type       = xiiAgentSteeringEvent::ErrorSteeringFailed;
    m_SteeringEvents.Broadcast(e);
    ClearTargetPosition();
    return;
  }

  const xiiRcPos rcPosOnNavmesh = m_pCorridor->getPos();
  m_vCurrentPositionOnNavmesh   = rcPosOnNavmesh;

  /// \todo Check when these values diverge
}

void xiiRcAgentComponent::Update()
{
  // this can happen the first few frames
  if (InitializeRecast().Failed())
    return;

  // visualize various things
  {
    VisualizePathCorridorPosition();
    VisualizePathCorridor();
    VisualizeCurrentPath();
    VisualizeTargetPosition();
  }

  // target is set, but no path is computed yet
  if (GetPathToTargetState() == xiiAgentPathFindingState::HasTargetWaitingForPath)
  {
    if (ComputePathToTarget().Failed())
      return;

    PlanNextSteps();
  }

  // from here on down, everything has to do with following a valid path

  if (GetPathToTargetState() != xiiAgentPathFindingState::HasTargetAndValidPath)
    return;

  if (HasReachedGoal(1.0f))
  {
    xiiAgentSteeringEvent e;
    e.m_pComponent = this;
    e.m_Type       = xiiAgentSteeringEvent::TargetReached;
    m_SteeringEvents.Broadcast(e);

    ClearTargetPosition();
    return;
  }

  ComputeSteeringDirection(1.0f);

  if (m_vCurrentSteeringDirection.IsZero())
  {
    /// \todo This would be some sort of error
    xiiLog::Error("Steering Direction is zero.");
    ClearTargetPosition();
    return;
  }

  ApplySteering(m_vCurrentSteeringDirection, m_fWalkSpeed);

  SyncSteeringWithReality();
}

void xiiRcAgentComponent::ComputeSteeringDirection(float fMaxDistance)
{
  xiiVec3 vCurPos = GetOwner()->GetGlobalPosition();
  vCurPos.z       = 0;

  xiiInt32 iNextStep = 0;
  for (iNextStep = m_iNumNextSteps - 1; iNextStep >= m_iFirstNextStep; --iNextStep)
  {
    xiiVec3 step = m_vNextSteps[iNextStep];

    if (IsPositionVisible(step))
      break;
  }

  if (iNextStep < m_iFirstNextStep)
  {
    // xiiLog::Error("Next step not visible");
    iNextStep = m_iFirstNextStep;
  }

  if (iNextStep == m_iNumNextSteps - 1)
  {
    if (HasReachedPosition(m_vNextSteps[iNextStep], 1.0f))
    {
      PlanNextSteps();
      return; // reuse last steering direction
    }
  }

  m_iFirstNextStep = iNextStep;

  if (m_iFirstNextStep >= m_iNumNextSteps)
  {
    PlanNextSteps();
    return; // reuse last steering direction
  }


  xiiVec3 vDirection = m_vNextSteps[m_iFirstNextStep] - vCurPos;
  vDirection.z       = 0;
  vDirection.NormalizeIfNotZero(xiiVec3::ZeroVector()).IgnoreResult();

  m_vCurrentSteeringDirection = vDirection;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

void xiiRcAgentComponent::VisualizePathCorridorPosition()
{
  if (GetPathToTargetState() != xiiAgentPathFindingState::HasTargetAndValidPath)
    return;

  const float*  pos = m_pCorridor->getPos();
  const xiiVec3 vPos(pos[0], pos[2], pos[1]);

  xiiBoundingBox box;
  box.SetCenterAndHalfExtents(xiiVec3(0, 0, 1.0f), xiiVec3(0.3f, 0.3f, 1.0f));

  xiiTransform t;
  t.SetIdentity();
  t.m_vPosition = vPos;
  t.m_qRotation = GetOwner()->GetGlobalRotation();

  xiiDebugRenderer::DrawLineBox(GetWorld(), box, xiiColor::DarkGreen, t);
}

void xiiRcAgentComponent::VisualizePathCorridor()
{
  if (GetPathToTargetState() != xiiAgentPathFindingState::HasTargetAndValidPath)
    return;

  for (xiiUInt32 c = 0; c < m_PathCorridor.GetCount(); ++c)
  {
    dtPolyRef poly = m_PathCorridor[c];

    const dtMeshTile* pTile;
    const dtPoly*     pPoly;
    m_pQuery->getAttachedNavMesh()->getTileAndPolyByRef(poly, &pTile, &pPoly);

    xiiHybridArray<xiiDebugRenderer::Triangle, 32> tris;

    for (xiiUInt32 i = 2; i < pPoly->vertCount; ++i)
    {
      xiiRcPos rcPos[3];
      rcPos[0] = &(pTile->verts[pPoly->verts[0] * 3]);
      rcPos[1] = &(pTile->verts[pPoly->verts[i - 1] * 3]);
      rcPos[2] = &(pTile->verts[pPoly->verts[i] * 3]);

      auto& tri         = tris.ExpandAndGetRef();
      tri.m_position[0] = xiiVec3(rcPos[0]) + xiiVec3(0, 0, 0.1f);
      tri.m_position[1] = xiiVec3(rcPos[1]) + xiiVec3(0, 0, 0.1f);
      tri.m_position[2] = xiiVec3(rcPos[2]) + xiiVec3(0, 0, 0.1f);
    }

    xiiDebugRenderer::DrawSolidTriangles(GetWorld(), tris, xiiColor::OrangeRed.WithAlpha(0.4f));
  }
}

void xiiRcAgentComponent::VisualizeTargetPosition()
{
  if (GetPathToTargetState() == xiiAgentPathFindingState::HasNoTarget)
    return;

  xiiHybridArray<xiiDebugRenderer::Line, 16> lines;
  auto&                                      line = lines.ExpandAndGetRef();

  line.m_start = m_vTargetPosition - xiiVec3(0, 0, 0.5f);
  line.m_end   = m_vTargetPosition + xiiVec3(0, 0, 1.5f);

  xiiDebugRenderer::DrawLines(GetWorld(), lines, xiiColor::HotPink);
}

void xiiRcAgentComponent::VisualizeCurrentPath()
{
  if (GetPathToTargetState() != xiiAgentPathFindingState::HasTargetAndValidPath)
    return;

  xiiHybridArray<xiiDebugRenderer::Line, 16> lines;
  lines.Reserve(m_iNumNextSteps);

  xiiHybridArray<xiiDebugRenderer::Line, 16> steps;
  steps.Reserve(m_iNumNextSteps);

  /// \todo Hard-coded height offset
  xiiVec3 vPrev = GetOwner()->GetGlobalPosition() + xiiVec3(0, 0, 0.5f);
  for (xiiInt32 i = m_iFirstNextStep; i < m_iNumNextSteps; ++i)
  {
    auto& line   = lines.ExpandAndGetRef();
    line.m_start = vPrev;
    line.m_end   = m_vNextSteps[i] + xiiVec3(0, 0, 0.5f);
    vPrev        = line.m_end;

    auto& step   = steps.ExpandAndGetRef();
    step.m_start = m_vNextSteps[i];
    step.m_end   = m_vNextSteps[i] + xiiVec3(0, 0, 1.0f);
  }

  xiiDebugRenderer::DrawLines(GetWorld(), lines, xiiColor::DarkViolet);
  xiiDebugRenderer::DrawLines(GetWorld(), steps, xiiColor::LightYellow);
}


//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

xiiRcAgentComponentManager::xiiRcAgentComponentManager(xiiWorld* pWorld) :
  SUPER(pWorld)
{
}
xiiRcAgentComponentManager::~xiiRcAgentComponentManager() {}

void xiiRcAgentComponentManager::Initialize()
{
  SUPER::Initialize();

  // make sure this world module exists
  m_pWorldModule = GetWorld()->GetOrCreateModule<xiiRecastWorldModule>();

  m_pPhysicsInterface = GetWorld()->GetOrCreateModule<xiiPhysicsWorldModuleInterface>();

  auto desc = XII_CREATE_MODULE_UPDATE_FUNCTION_DESC(xiiRcAgentComponentManager::Update, this);

  RegisterUpdateFunction(desc);

  xiiResourceManager::GetResourceEvents().AddEventHandler(xiiMakeDelegate(&xiiRcAgentComponentManager::ResourceEventHandler, this));
}

void xiiRcAgentComponentManager::Deinitialize()
{
  xiiResourceManager::GetResourceEvents().RemoveEventHandler(xiiMakeDelegate(&xiiRcAgentComponentManager::ResourceEventHandler, this));

  SUPER::Deinitialize();
}

void xiiRcAgentComponentManager::ResourceEventHandler(const xiiResourceEvent& e)
{
  if (e.m_Type == xiiResourceEvent::Type::ResourceContentUnloading && e.m_pResource->GetDynamicRTTI()->IsDerivedFrom<xiiRecastNavMeshResource>())
  {
    for (auto it = this->m_ComponentStorage.GetIterator(); it.IsValid(); ++it)
    {
      // make sure no agent references previous navmeshes
      it->UninitializeRecast();
    }
  }
}

void xiiRcAgentComponentManager::Update(const xiiWorldModule::UpdateContext& context)
{
  for (auto it = this->m_ComponentStorage.GetIterator(context.m_uiFirstComponentIndex, context.m_uiComponentCount); it.IsValid(); ++it)
  {
    if (it->IsActiveAndSimulating())
      it->Update();
  }
}

#pragma once

#include <Core/World/Component.h>
#include <Core/World/World.h>
#include <GameEngine/AI/AgentSteeringComponent.h>
#include <Recast/DetourNavMeshQuery.h>
#include <Recast/DetourPathCorridor.h>
#include <RecastPlugin/Components/RecastNavMeshComponent.h>
#include <RecastPlugin/NavMeshBuilder/NavMeshBuilder.h>
#include <RecastPlugin/RecastPluginDLL.h>

class xiiRecastWorldModule;
class xiiPhysicsWorldModuleInterface;
struct xiiResourceEvent;

//////////////////////////////////////////////////////////////////////////

class XII_RECASTPLUGIN_DLL xiiRcAgentComponentManager : public xiiComponentManager<class xiiRcAgentComponent, xiiBlockStorageType::FreeList>
{
  typedef xiiComponentManager<class xiiRcAgentComponent, xiiBlockStorageType::FreeList> SUPER;

public:
  xiiRcAgentComponentManager(xiiWorld* pWorld);
  ~xiiRcAgentComponentManager();

  virtual void Initialize() override;
  virtual void Deinitialize() override;

  xiiRecastWorldModule* GetRecastWorldModule() const { return m_pWorldModule; }

private:
  void ResourceEventHandler(const xiiResourceEvent& e);
  void Update(const xiiWorldModule::UpdateContext& context);

  xiiPhysicsWorldModuleInterface* m_pPhysicsInterface = nullptr;
  xiiRecastWorldModule*           m_pWorldModule      = nullptr;
};

//////////////////////////////////////////////////////////////////////////

class XII_RECASTPLUGIN_DLL xiiRcAgentComponent : public xiiAgentSteeringComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiRcAgentComponent, xiiAgentSteeringComponent, xiiRcAgentComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // xiiComponent

protected:
  virtual void SerializeComponent(xiiWorldWriter& stream) const override;
  virtual void DeserializeComponent(xiiWorldReader& stream) override;

  //////////////////////////////////////////////////////////////////////////
  // xiiAgentSteeringComponent

public:
  xiiRcAgentComponent();
  ~xiiRcAgentComponent();

  virtual void                           SetTargetPosition(const xiiVec3& vPosition) override;
  virtual xiiVec3                        GetTargetPosition() const override;
  virtual void                           ClearTargetPosition() override;
  virtual xiiAgentPathFindingState::Enum GetPathToTargetState() const override;

  //////////////////////////////////////////////////////////////////////////
  // Helper Functions

public:
  xiiResult FindNavMeshPolyAt(const xiiVec3& vPosition, dtPolyRef& out_PolyRef, xiiVec3* out_vAdjustedPosition = nullptr, float fPlaneEpsilon = 0.01f, float fHeightEpsilon = 1.0f) const;
  bool      HasReachedPosition(const xiiVec3& pos, float fMaxDistance) const;
  bool      HasReachedGoal(float fMaxDistance) const;
  bool      IsPositionVisible(const xiiVec3& pos) const;

  //////////////////////////////////////////////////////////////////////////
  // Debug Visualization Functions

private:
  void VisualizePathCorridorPosition();
  void VisualizePathCorridor();
  void VisualizeCurrentPath();
  void VisualizeTargetPosition();

  //////////////////////////////////////////////////////////////////////////
  // Path Finding and Steering

private:
  xiiResult ComputePathToTarget();
  xiiResult ComputePathCorridor(dtPolyRef startPoly, dtPolyRef endPoly, bool& bFoundPartialPath);
  void      ComputeSteeringDirection(float fMaxDistance);
  void      ApplySteering(const xiiVec3& vDirection, float fSpeed);
  void      SyncSteeringWithReality();
  void      PlanNextSteps();

  xiiVec3                           m_vTargetPosition;
  xiiEnum<xiiAgentPathFindingState> m_PathToTargetState;
  xiiVec3                           m_vCurrentPositionOnNavmesh; /// \todo ??? keep update ?
  xiiUniquePtr<dtNavMeshQuery>      m_pQuery;                    // careful, dtNavMeshQuery is not moveble
  xiiUniquePtr<dtPathCorridor>      m_pCorridor;                 // careful, dtPathCorridor is not moveble
  dtQueryFilter                     m_QueryFilter;               /// \todo hard-coded filter
  xiiDynamicArray<dtPolyRef>        m_PathCorridor;
  // path following
  xiiInt32 m_iFirstNextStep = 0;
  xiiInt32 m_iNumNextSteps  = 0;
  xiiVec3  m_vNextSteps[16];
  xiiVec3  m_vCurrentSteeringDirection;


  //////////////////////////////////////////////////////////////////////////
  // Properties
public:
  float m_fWalkSpeed = 4.0f; // [ property ]
  /// \todo Expose and use
  // float m_fRadius = 0.2f;
  // float m_fHeight = 1.0f;


  //////////////////////////////////////////////////////////////////////////
  // Other
private:
  xiiResult    InitializeRecast();
  void         UninitializeRecast();
  virtual void OnSimulationStarted() override;
  void         Update();

  bool               m_bRecastInitialized = false;
  xiiComponentHandle m_hCharacterController;
};

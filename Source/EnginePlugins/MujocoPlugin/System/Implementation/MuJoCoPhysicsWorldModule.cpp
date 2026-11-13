/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <MujocoPlugin/MujocoPluginPCH.h>

#include <Core/World/World.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Math/Math.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/Strings/PathUtils.h>
#include <Foundation/Strings/StringBuilder.h>
#include <Foundation/Threading/AtomicInteger.h>
#include <MujocoPlugin/Components/MuJoCoActuatorComponent.h>
#include <MujocoPlugin/Components/MuJoCoBodyComponent.h>
#include <MujocoPlugin/Resources/Implementation/MuJoCoCompiledModel.h>
#include <MujocoPlugin/System/Implementation/MuJoCoSimulation.h>
#include <MujocoPlugin/System/MuJoCoPhysicsWorldModule.h>

#include <mujoco/mujoco.h>

#include <algorithm>
#include <memory>

XII_IMPLEMENT_WORLD_MODULE(xiiMuJoCoPhysicsWorldModule);

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiMuJoCoPhysicsWorldModule, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

namespace
{
  mjModel* CompileMjcf(const char* szPath)
  {
    // Parse and compile explicitly to keep the spec local. mj_loadXML retains it for mj_saveLastXML.
    char                                                    szError[2048] = {};
    const std::unique_ptr<mjSpec, decltype(&mj_deleteSpec)> pSpec(mj_parseXML(szPath, nullptr, szError, sizeof(szError)), mj_deleteSpec);
    if (!pSpec)
    {
      xiiLog::Error("MuJoCo could not parse '{}': {}", szPath, szError);
      return nullptr;
    }

    mjModel* pModel = mj_compile(pSpec.get(), nullptr);
    if (pModel == nullptr)
    {
      xiiLog::Error("MuJoCo could not compile '{}': {}", szPath, mjs_getError(pSpec.get()));
      return nullptr;
    }

    const int iWarningCount = mjs_numWarnings(pSpec.get());
    for (int i = 0; i < iWarningCount; ++i)
    {
      xiiLog::Warning("MuJoCo model '{}': {}", szPath, mjs_getWarning(pSpec.get(), i));
    }

    return pModel;
  }
} // namespace

xiiMuJoCoPhysicsWorldModule::xiiMuJoCoPhysicsWorldModule(xiiWorld* pWorld) :
  xiiWorldModule(pWorld)
{
}

xiiMuJoCoPhysicsWorldModule::~xiiMuJoCoPhysicsWorldModule() = default;

xiiResult xiiMuJoCoPhysicsWorldModule::LoadModel(xiiStringView sPath)
{
  if (m_bExecuting)
    return XII_FAILURE;
  if (!m_hModelOwner.IsInvalidated())
  {
    xiiLog::Error("A MuJoCo model component already controls world '{}'.", GetWorld()->GetName());
    return XII_FAILURE;
  }

  if (sPath.IsEmpty())
  {
    xiiLog::Error("A MuJoCo model path is required.");
    return XII_FAILURE;
  }

  xiiStringBuilder sExpandedPath;
  if (xiiFileSystem::ResolveSpecialDirectory(sPath, sExpandedPath).Failed())
  {
    xiiLog::Error("Could not resolve MuJoCo model path '{}'.", sPath);
    return XII_FAILURE;
  }

  xiiStringBuilder sNativePath;
  if (xiiPathUtils::IsAbsolutePath(sExpandedPath))
  {
    sNativePath = sExpandedPath;
    sNativePath.MakeCleanPath();
  }
  else if (xiiFileSystem::ResolvePath(sExpandedPath, &sNativePath, nullptr).Failed())
  {
    xiiLog::Error("Could not resolve MuJoCo model path '{}'.", sPath);
    return XII_FAILURE;
  }

  if (!xiiPathUtils::IsAbsolutePath(sNativePath) || !xiiOSFile::ExistsFile(sNativePath))
  {
    xiiLog::Error("MuJoCo model '{}' could not be resolved to a loose file.", sPath);
    return XII_FAILURE;
  }

  xiiMuJoCoCompiledModel::ModelPtr pModel(nullptr, mj_deleteModel);
  if (sNativePath.HasExtension("mjb"))
  {
    pModel.reset(mj_loadModel(sNativePath.GetData(), nullptr));
  }
  else if (sNativePath.HasExtension("xml") || sNativePath.HasExtension("mjcf"))
  {
    pModel.reset(CompileMjcf(sNativePath.GetData()));
    if (!pModel)
      return XII_FAILURE;
  }
  else
  {
    xiiLog::Error("Unsupported MuJoCo model extension in '{}'. Expected .xml, .mjcf, or .mjb.", sNativePath);
    return XII_FAILURE;
  }

  return InstallModel(xiiMuJoCoCompiledModel::Adopt(std::move(pModel), sNativePath), sNativePath);
}

xiiResult xiiMuJoCoPhysicsWorldModule::LoadModelFromBuffer(xiiArrayPtr<const xiiUInt8> modelBytes)
{
  if (m_bExecuting)
    return XII_FAILURE;
  if (!m_hModelOwner.IsInvalidated())
  {
    xiiLog::Error("A MuJoCo model component already controls world '{}'.", GetWorld()->GetName());
    return XII_FAILURE;
  }

  return InstallModel(xiiMuJoCoCompiledModel::LoadBinary(modelBytes, "MJB buffer"), "MJB buffer");
}

xiiResult xiiMuJoCoPhysicsWorldModule::InstallModel(xiiSharedPtr<xiiMuJoCoCompiledModel> pModel, xiiStringView sSource)
{
  if (!pModel || m_bExecuting)
    return XII_FAILURE;

  const mjModel* pNativeModel = pModel->GetModel();
  const mjtSize  iMaxCount    = xiiMath::MaxValue<xiiInt32>();
  if (pNativeModel->nbody > iMaxCount || pNativeModel->njnt > iMaxCount || pNativeModel->nactuator > iMaxCount ||
      pNativeModel->nq > iMaxCount || pNativeModel->nv > iMaxCount || pNativeModel->nu > iMaxCount ||
      pNativeModel->na > iMaxCount || pNativeModel->nout > iMaxCount || pNativeModel->nsensor > iMaxCount ||
      pNativeModel->nsensordata > iMaxCount || pNativeModel->ngeom > iMaxCount || pNativeModel->nsite > iMaxCount)
  {
    xiiLog::Error("MuJoCo model '{}' exceeds the supported state indexing range.", sSource);
    return XII_FAILURE;
  }

  xiiUniquePtr<NativeSimulation> pSimulation = XII_NEW(GetAllocator(), NativeSimulation);
  pSimulation->m_pModel                      = std::move(pModel);
  pSimulation->m_pData.reset(mj_makeData(pNativeModel));
  if (!pSimulation->m_pData)
  {
    xiiLog::Error("MuJoCo could not create simulation data for '{}'.", sSource);
    return XII_FAILURE;
  }

  // mj_makeData supplies native neutral controls, including identity quaternion controls.
  const auto uiControls = static_cast<xiiUInt32>(pNativeModel->nu);
  pSimulation->m_BodySceneBindings.SetCount(static_cast<xiiUInt32>(pNativeModel->nbody));
  pSimulation->m_PendingControls.SetCount(uiControls);
  pSimulation->m_ControlPending.SetCount(uiControls, 0);
  pSimulation->m_NeutralControls.SetCount(uiControls);
  for (xiiUInt32 i = 0; i < uiControls; ++i)
    pSimulation->m_NeutralControls[i] = pSimulation->m_pData->ctrl[i];

  // Evaluate the initial state before publishing the pair.
  m_bExecuting = true;
  mj_forward(pNativeModel, pSimulation->m_pData.get());

  static xiiAtomicIntegerU64 s_uiNextGeneration;
  m_uiModelGeneration = s_uiNextGeneration.Increment();
  m_pSimulation       = std::move(pSimulation);
  m_bExecuting        = false;
  m_AccumulatedTime   = xiiTime::MakeZero();
  for (auto& binding : m_BodyBindings)
    ResolveBody(binding);
  RebuildBodyBindingMap();
  ReportNativeWarnings();
  m_bSyncBodies = true;
  RebindActuators();
  return XII_SUCCESS;
}

void xiiMuJoCoPhysicsWorldModule::ResetSimulation()
{
  if (m_bExecuting)
    return;

  m_AccumulatedTime = xiiTime::MakeZero();
  if (m_pSimulation)
  {
    const mjModel* pModel = m_pSimulation->m_pModel->GetModel();
    mjData*        pData  = m_pSimulation->m_pData.get();
    m_bExecuting          = true;
    mj_resetData(pModel, pData);
    m_pSimulation->m_ControlPending.Clear();
    m_pSimulation->m_ControlPending.SetCount(static_cast<xiiUInt32>(pModel->nu), 0);
    m_pSimulation->m_BodyForces.Clear();
    m_pSimulation->m_bStateDirty            = false;
    m_pSimulation->m_bControlsPending       = false;
    m_pSimulation->m_bExternalForcesApplied = false;
    m_pSimulation->m_bObservationsDirty     = false;
    ++m_pSimulation->m_uiStateRevision;
    m_pSimulation->m_uiStepIndex     = 0;
    m_pSimulation->m_StepDiagnostics = {};
    std::fill_n(m_pSimulation->m_WarningsReported, mjNWARNING, false);
    mj_forward(pModel, pData);
    m_bExecuting  = false;
    m_bSyncBodies = true;
    ReportNativeWarnings();
    RebindActuators();
  }
}

xiiResult xiiMuJoCoPhysicsWorldModule::SetMaxCatchUpSteps(xiiUInt32 uiMaxCatchUpSteps)
{
  if (uiMaxCatchUpSteps == 0)
    return XII_FAILURE;

  m_uiMaxCatchUpSteps = uiMaxCatchUpSteps;
  return XII_SUCCESS;
}

void xiiMuJoCoPhysicsWorldModule::RequestModel(xiiComponentHandle hOwner, const xiiMuJoCoModelResourceHandle& hModel)
{
  if (m_bExecuting)
    return;
  if (m_hModelOwner != hOwner && (!m_hModelOwner.IsInvalidated() || m_pSimulation))
  {
    xiiLog::Error("World '{}' already has a MuJoCo model. Only one model may control a world.", GetWorld()->GetName());
    return;
  }

  m_hModelOwner     = hOwner;
  m_hRequestedModel = hModel;
  const xiiResourceLock<xiiMuJoCoModelResource> resource(m_hRequestedModel, xiiResourceAcquireMode::PointerOnly);
  m_pRequestedResource = resource.GetPointer();
  m_bAttemptedRequest  = false;

  if (resource->GetLoadingState() == xiiResourceState::Unloaded)
    xiiResourceManager::PreloadResource(m_hRequestedModel);
}

void xiiMuJoCoPhysicsWorldModule::ReleaseModel(xiiComponentHandle hOwner)
{
  if (m_bExecuting || m_hModelOwner != hOwner)
    return;

  m_pRequestedResource = nullptr;
  m_hRequestedModel.Invalidate();
  m_hModelOwner.Invalidate();
  m_bAttemptedRequest = false;
  ClearSimulation();
}

void xiiMuJoCoPhysicsWorldModule::UpdateRequestedModel()
{
  if (m_pRequestedResource == nullptr)
    return;

  const xiiUInt32 uiRevision = m_pRequestedResource->m_uiRevision;
  if (m_bAttemptedRequest && uiRevision == m_uiAttemptedRevision)
    return;

  const auto state = m_pRequestedResource->GetLoadingState();
  if (state == xiiResourceState::Unloaded)
    return;

  auto pModel = m_pRequestedResource->GetModel(m_uiAttemptedRevision);
  // Reload may have removed the content since the state read. A successful or failed load publishes a new revision.
  if (!pModel && m_pRequestedResource->GetLoadingState() != xiiResourceState::LoadedResourceMissing)
    return;

  m_bAttemptedRequest = true;
  if (pModel)
    InstallModel(std::move(pModel), m_hRequestedModel.GetResourceID()).IgnoreResult();
}

void xiiMuJoCoPhysicsWorldModule::ClearSimulation()
{
  m_uiModelGeneration = 0;
  m_bExecuting        = true;
  m_pSimulation.Clear();
  m_bExecuting      = false;
  m_AccumulatedTime = xiiTime::MakeZero();
  m_bSyncBodies     = false;
  for (auto& binding : m_BodyBindings)
    binding.m_iBodyId = -1;
  RebindActuators();
}

void xiiMuJoCoPhysicsWorldModule::RegisterBody(xiiComponentHandle hComponent, const xiiString& sBodyName)
{
  UnregisterBody(hComponent);
  if (sBodyName.IsEmpty())
    return;

  xiiMuJoCoBodyComponent* pComponent = nullptr;
  if (!GetWorld()->TryGetComponent(hComponent, pComponent))
    return;

  const auto hObject = pComponent->GetOwner()->GetHandle();
  for (const auto& binding : m_BodyBindings)
  {
    if (binding.m_hObject == hObject)
    {
      xiiLog::Warning("Object '{}' already has a MuJoCo body binding.", pComponent->GetOwner()->GetName());
      return;
    }
  }

  auto& binding        = m_BodyBindings.ExpandAndGetRef();
  binding.m_hComponent = hComponent;
  binding.m_hObject    = hObject;
  binding.m_sBodyName  = sBodyName;
  ResolveBody(binding);
  RebuildBodyBindingMap();
  m_bSyncBodies = true;
}

void xiiMuJoCoPhysicsWorldModule::UnregisterBody(xiiComponentHandle hComponent)
{
  for (xiiUInt32 i = 0; i < m_BodyBindings.GetCount(); ++i)
  {
    if (m_BodyBindings[i].m_hComponent == hComponent)
    {
      m_BodyBindings.RemoveAtAndSwap(i);
      RebuildBodyBindingMap();
      return;
    }
  }
}

void xiiMuJoCoPhysicsWorldModule::ResolveBody(BodyBinding& ref_binding) const
{
  ref_binding.m_iBodyId = -1;
  if (m_pSimulation)
  {
    ref_binding.m_iBodyId = mj_name2id(m_pSimulation->m_pModel->GetModel(), mjOBJ_BODY, ref_binding.m_sBodyName.GetData());
    if (ref_binding.m_iBodyId < 0)
      xiiLog::Warning("MuJoCo body '{}' was not found in the model for world '{}'.", ref_binding.m_sBodyName, GetWorld()->GetName());
  }
}

void xiiMuJoCoPhysicsWorldModule::SynchronizeBodies()
{
  if (!m_bSyncBodies || !m_pSimulation)
    return;

  m_bSyncBodies = false;
  for (auto& binding : m_BodyBindings)
  {
    binding.m_uiDepth = 0;
    if (binding.m_iBodyId < 0)
      continue;

    const xiiGameObject* pObject = nullptr;
    if (GetWorld()->TryGetObject(binding.m_hObject, pObject))
    {
      for (auto* pParent = pObject->GetParent(); pParent != nullptr; pParent = pParent->GetParent())
        ++binding.m_uiDepth;
    }
  }

  // SetGlobalTransform computes local space using the current parent pose.
  m_BodyBindings.Sort([](const BodyBinding& a, const BodyBinding& b) { return a.m_uiDepth < b.m_uiDepth; });
  const auto& data = *m_pSimulation->m_pData;
  for (const auto& binding : m_BodyBindings)
  {
    xiiGameObject* pObject = nullptr;
    if (binding.m_iBodyId < 0 || !GetWorld()->TryGetObject(binding.m_hObject, pObject))
      continue;

    const auto    uiBody    = static_cast<size_t>(binding.m_iBodyId);
    const mjtNum* pPosition = data.xpos + 3 * uiBody;
    const mjtNum* pRotation = data.xquat + 4 * uiBody;
    // Both use right-handed Z-up world coordinates. MuJoCo stores quaternion W first.
    const xiiVec3 vPosition(static_cast<float>(pPosition[0]), static_cast<float>(pPosition[1]), static_cast<float>(pPosition[2]));
    const xiiQuat qRotation(static_cast<float>(pRotation[1]), static_cast<float>(pRotation[2]), static_cast<float>(pRotation[3]), static_cast<float>(pRotation[0]));
    if (!vPosition.IsValid() || !qRotation.IsValid())
      continue;

    pObject->UpdateGlobalTransform();
    auto          transform = pObject->GetGlobalTransform();
    const xiiQuat qOpposite(-qRotation.x, -qRotation.y, -qRotation.z, -qRotation.w);
    if (transform.m_vPosition == vPosition && (transform.m_qRotation == qRotation || transform.m_qRotation == qOpposite))
      continue;

    transform.m_vPosition = vPosition;
    transform.m_qRotation = qRotation;
    pObject->SetGlobalTransform(transform);
  }
}

void xiiMuJoCoPhysicsWorldModule::Initialize()
{
  auto desc                        = XII_CREATE_MODULE_UPDATE_FUNCTION_DESC(xiiMuJoCoPhysicsWorldModule::Simulate, this);
  desc.m_Phase                     = xiiWorldUpdatePhase::PreAsync;
  desc.m_bOnlyUpdateWhenSimulating = true;
  desc.m_fPriority                 = -100000.0f;
  RegisterUpdateFunction(desc);
}

void xiiMuJoCoPhysicsWorldModule::Deinitialize()
{
  auto desc    = XII_CREATE_MODULE_UPDATE_FUNCTION_DESC(xiiMuJoCoPhysicsWorldModule::Simulate, this);
  desc.m_Phase = xiiWorldUpdatePhase::PreAsync;
  DeregisterUpdateFunction(desc);
  WorldClear();
}

void xiiMuJoCoPhysicsWorldModule::OnSimulationStarted()
{
  ResetSimulation();
}

void xiiMuJoCoPhysicsWorldModule::WorldClear()
{
  m_ActuatorComponents.Clear();
  m_BodyBindings.Clear();
  m_pRequestedResource = nullptr;
  m_hRequestedModel.Invalidate();
  m_hModelOwner.Invalidate();
  m_bAttemptedRequest = false;
  ClearSimulation();
}

void xiiMuJoCoPhysicsWorldModule::Simulate(const UpdateContext&)
{
  UpdateRequestedModel();
  if (!m_pSimulation)
    return;

  EvaluateState();
  m_pSimulation->m_StepDiagnostics.m_uiStepsLastUpdate      = 0;
  m_pSimulation->m_StepDiagnostics.m_UpdateSteppingWallTime = xiiTime::MakeZero();
  XII_SCOPE_EXIT(SynchronizeBodies());
  const xiiClock& clock         = GetWorld()->GetClock();
  const double    fDeltaSeconds = clock.GetTimeDiff().GetSeconds();
  if (clock.GetPaused() || !xiiMath::IsFinite(fDeltaSeconds) || fDeltaSeconds <= 0.0)
    return;

  const mjModel* pModel          = m_pSimulation->m_pModel->GetModel();
  const double   fStepSeconds    = pModel->opt.timestep;
  const double   fElapsedSeconds = m_AccumulatedTime.GetSeconds() + fDeltaSeconds;
  m_AccumulatedTime              = xiiTime::MakeFromSeconds(fElapsedSeconds);
  if (fElapsedSeconds < fStepSeconds)
    return;

  // Keep the fractional step across frames and discard whole steps beyond the catch-up limit.
  const double fElapsedSteps = xiiMath::Floor(fElapsedSeconds / fStepSeconds);
  const auto   uiSteps       = static_cast<xiiUInt32>(xiiMath::Min(fElapsedSteps, static_cast<double>(m_uiMaxCatchUpSteps)));
  const double fRemainder    = fElapsedSteps > m_uiMaxCatchUpSteps ? xiiMath::Mod(fElapsedSeconds, fStepSeconds) : xiiMath::Max(0.0, fElapsedSeconds - uiSteps * fStepSeconds);
  m_AccumulatedTime          = xiiTime::MakeFromSeconds(fRemainder);

  XII_PROFILE_SCOPE("MuJoCo Physics");
  mjData* pData = m_pSimulation->m_pData.get();
  for (xiiUInt32 uiStep = 0; uiStep < uiSteps; ++uiStep)
  {
    ApplyPendingControls();
    ApplyExternalForces();
    auto& diagnostics        = m_pSimulation->m_StepDiagnostics;
    diagnostics.m_fStartTime = pData->time;
    const xiiTime started    = xiiTime::Now();
    m_bExecuting             = true;
    mj_step(pModel, pData);
    m_bExecuting               = false;
    diagnostics.m_StepWallTime = xiiTime::Now() - started;
    diagnostics.m_UpdateSteppingWallTime += diagnostics.m_StepWallTime;
    ++diagnostics.m_uiStepsLastUpdate;
    diagnostics.m_fEndTime              = pData->time;
    diagnostics.m_uiStepIndex           = ++m_pSimulation->m_uiStepIndex;
    diagnostics.m_uiContactCount        = static_cast<xiiUInt32>(pData->ncon);
    diagnostics.m_uiConstraintCount     = static_cast<xiiUInt32>(pData->nefc);
    diagnostics.m_uiMaxSolverIterations = 0;
    for (int i = 0; i < mjNISLAND; ++i)
      diagnostics.m_uiMaxSolverIterations = xiiMath::Max(diagnostics.m_uiMaxSolverIterations, static_cast<xiiUInt32>(pData->solver_niter[i]));
    ++m_pSimulation->m_uiStateRevision;
    m_pSimulation->m_bObservationsDirty = true;
    ReportNativeWarnings();
    ClearExternalForces();
  }

  // Refresh the integrated body frames and spatial velocities without invoking control callbacks.
  mj_kinematics(pModel, pData);
  mj_comPos(pModel, pData);
  mj_comVel(pModel, pData);
  m_bSyncBodies = true;
}

XII_STATICLINK_FILE(MujocoPlugin, MujocoPlugin_System_Implementation_MuJoCoPhysicsWorldModule);

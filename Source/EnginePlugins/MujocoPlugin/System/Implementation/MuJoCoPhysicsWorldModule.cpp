/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <MujocoPlugin/MujocoPluginPCH.h>

#include <Core/World/World.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Math/Math.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/Strings/PathUtils.h>
#include <Foundation/Strings/StringBuilder.h>
#include <MujocoPlugin/System/MuJoCoPhysicsWorldModule.h>

#include <mujoco/mujoco.h>

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

struct xiiMuJoCoPhysicsWorldModule::NativeSimulation
{
  // Members are destroyed in reverse order so mjData is released first.
  std::unique_ptr<mjModel, decltype(&mj_deleteModel)> m_pModel{nullptr, mj_deleteModel};
  std::unique_ptr<mjData, decltype(&mj_deleteData)>   m_pData{nullptr, mj_deleteData};
};

xiiMuJoCoPhysicsWorldModule::xiiMuJoCoPhysicsWorldModule(xiiWorld* pWorld) :
  xiiWorldModule(pWorld)
{
}

xiiMuJoCoPhysicsWorldModule::~xiiMuJoCoPhysicsWorldModule() = default;

xiiResult xiiMuJoCoPhysicsWorldModule::LoadModel(xiiStringView sPath)
{
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

  xiiUniquePtr<NativeSimulation> pSimulation = XII_NEW(GetAllocator(), NativeSimulation);
  if (sNativePath.HasExtension("mjb"))
  {
    pSimulation->m_pModel.reset(mj_loadModel(sNativePath.GetData(), nullptr));
    if (!pSimulation->m_pModel)
    {
      xiiLog::Error("MuJoCo could not load binary model '{}'.", sNativePath);
      return XII_FAILURE;
    }
  }
  else if (sNativePath.HasExtension("xml") || sNativePath.HasExtension("mjcf"))
  {
    pSimulation->m_pModel.reset(CompileMjcf(sNativePath.GetData()));
    if (!pSimulation->m_pModel)
      return XII_FAILURE;
  }
  else
  {
    xiiLog::Error("Unsupported MuJoCo model extension in '{}'. Expected .xml, .mjcf, or .mjb.", sNativePath);
    return XII_FAILURE;
  }

  return InstallModel(std::move(pSimulation), sNativePath);
}

xiiResult xiiMuJoCoPhysicsWorldModule::LoadModelFromBuffer(xiiArrayPtr<const xiiUInt8> modelBytes)
{
  if (modelBytes.IsEmpty())
  {
    xiiLog::Error("The MuJoCo binary model buffer is empty.");
    return XII_FAILURE;
  }

  xiiUniquePtr<NativeSimulation> pSimulation = XII_NEW(GetAllocator(), NativeSimulation);
  pSimulation->m_pModel.reset(mj_loadModelBuffer(modelBytes.GetPtr(), modelBytes.GetCount()));
  if (!pSimulation->m_pModel)
  {
    xiiLog::Error("MuJoCo could not load the binary model buffer.");
    return XII_FAILURE;
  }

  return InstallModel(std::move(pSimulation), "MJB buffer");
}

xiiResult xiiMuJoCoPhysicsWorldModule::InstallModel(xiiUniquePtr<NativeSimulation> pSimulation, xiiStringView sSource)
{
  const double fStepSeconds = pSimulation->m_pModel->opt.timestep;
  if (!xiiMath::IsFinite(fStepSeconds) || fStepSeconds <= 0.0)
  {
    xiiLog::Error("MuJoCo model '{}' has an invalid timestep.", sSource);
    return XII_FAILURE;
  }

  pSimulation->m_pData.reset(mj_makeData(pSimulation->m_pModel.get()));
  if (!pSimulation->m_pData)
  {
    xiiLog::Error("MuJoCo could not create simulation data for '{}'.", sSource);
    return XII_FAILURE;
  }

  // mj_makeData resets to the model's initial state. Evaluate it before publishing the pair.
  mj_forward(pSimulation->m_pModel.get(), pSimulation->m_pData.get());
  m_pSimulation     = std::move(pSimulation);
  m_AccumulatedTime = xiiTime::MakeZero();
  return XII_SUCCESS;
}

void xiiMuJoCoPhysicsWorldModule::ResetSimulation()
{
  m_AccumulatedTime = xiiTime::MakeZero();
  if (m_pSimulation)
  {
    const mjModel* pModel = m_pSimulation->m_pModel.get();
    mjData*        pData  = m_pSimulation->m_pData.get();
    mj_resetData(pModel, pData);
    mj_forward(pModel, pData);
  }
}

xiiResult xiiMuJoCoPhysicsWorldModule::SetMaxCatchUpSteps(xiiUInt32 uiMaxCatchUpSteps)
{
  if (uiMaxCatchUpSteps == 0)
    return XII_FAILURE;

  m_uiMaxCatchUpSteps = uiMaxCatchUpSteps;
  return XII_SUCCESS;
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
  m_pSimulation.Clear();
  m_AccumulatedTime = xiiTime::MakeZero();
}

void xiiMuJoCoPhysicsWorldModule::Simulate(const UpdateContext&)
{
  if (!m_pSimulation)
    return;

  const xiiClock& clock         = GetWorld()->GetClock();
  const double    fDeltaSeconds = clock.GetTimeDiff().GetSeconds();
  if (clock.GetPaused() || !xiiMath::IsFinite(fDeltaSeconds) || fDeltaSeconds <= 0.0)
    return;

  const mjModel* pModel          = m_pSimulation->m_pModel.get();
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
    mj_step(pModel, pData);
  }
}

XII_STATICLINK_FILE(MujocoPlugin, MujocoPlugin_System_Implementation_MuJoCoPhysicsWorldModule);

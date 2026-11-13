/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <MujocoPlugin/MujocoPluginPCH.h>

#include <Core/World/World.h>
#include <Foundation/Algorithm/HashingUtils.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Math/Math.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/Strings/PathUtils.h>
#include <Foundation/Strings/StringBuilder.h>
#include <Foundation/Threading/AtomicInteger.h>
#include <MujocoPlugin/Components/MuJoCoActuatorComponent.h>
#include <MujocoPlugin/Components/MuJoCoBodyComponent.h>
#include <MujocoPlugin/Components/MuJoCoModelComponent.h>
#include <MujocoPlugin/Resources/Implementation/MuJoCoCompiledModel.h>
#include <MujocoPlugin/System/Implementation/MuJoCoSimulation.h>
#include <MujocoPlugin/System/MuJoCoPhysicsWorldModule.h>

#include <mujoco/mjxmacro.h>
#include <mujoco/mujoco.h>

#include <algorithm>
#include <memory>
#include <utility>

XII_IMPLEMENT_WORLD_MODULE(xiiMuJoCoPhysicsWorldModule);

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiMuJoCoPhysicsWorldModule, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

namespace
{
  class CompositionLog final : public xiiLogInterface
  {
  public:
    xiiLogInterface* m_pParent = xiiLog::GetThreadLocalLogSystem();
    xiiString        m_sError;
    void             HandleLogMessage(const xiiLoggingEventData& event) override
    {
      if (event.m_EventType == xiiLogMsgType::ErrorMsg && m_sError.IsEmpty())
        m_sError = event.m_sText;
      m_pParent->HandleLogMessage(event);
    }
  };
  using SpecPtr = std::unique_ptr<mjSpec, decltype([](mjSpec* pSpec) { mj_deleteSpec(pSpec); })>;

  // Every native path is resolved against the inputs captured by XII's asset transformer.
  class CompositionInputs
  {
  public:
    CompositionInputs()
    {
      mj_defaultVFS(&m_Vfs);
      mjp_defaultProvider(&m_Provider);
      m_Provider.data     = this;
      m_Provider.open     = Open;
      m_Provider.read     = Read;
      m_Provider.close    = Close;
      m_Provider.modified = Modified;
      m_bMounted          = mj_mountVFS(&m_Vfs, "", &m_Provider) == 0;
    }

    ~CompositionInputs() { mj_deleteVFS(&m_Vfs); }

    bool Add(xiiArrayPtr<const xiiMuJoCoCompiledModel::SourceFile> files)
    {
      for (const auto& file : files)
      {
        auto& input = m_Files[file.m_sPath];
        if (input.m_pFile == &file)
          continue;
        if (input.m_pFile != nullptr && input.m_pFile->m_Data != file.m_Data)
        {
          xiiLog::Error("MuJoCo models contain different snapshots of '{}'. Transform the affected assets again.", file.m_sPath);
          return false;
        }
        input.m_pFile  = &file;
        input.m_uiHash = xiiHashingUtils::xxHash64(file.m_Data.GetData(), file.m_Data.GetCount());
      }
      return true;
    }

    mjVFS m_Vfs{};
    bool  m_bMounted = false;

  private:
    struct Input
    {
      const xiiMuJoCoCompiledModel::SourceFile* m_pFile  = nullptr;
      xiiUInt64                                 m_uiHash = 0;
    };

    static int Open(mjResource* pResource)
    {
      const auto&      inputs = *static_cast<const CompositionInputs*>(pResource->provider->data);
      xiiStringBuilder path(pResource->name);
      path.MakeCleanPath();
      const auto it = inputs.m_Files.Find(path);
      if (!it.IsValid())
        return 0;
      pResource->data = const_cast<Input*>(&it.Value());
      xiiStringUtils::snprintf(pResource->timestamp, XII_ARRAY_SIZE(pResource->timestamp), "%llu", static_cast<unsigned long long>(it.Value().m_uiHash));
      return 1;
    }

    static int Read(mjResource* pResource, const void** out_pBuffer)
    {
      const auto& bytes = static_cast<const Input*>(pResource->data)->m_pFile->m_Data;
      *out_pBuffer      = bytes.GetData();
      return static_cast<int>(bytes.GetCount());
    }

    static void Close(mjResource* pResource) { pResource->data = nullptr; }
    static int  Modified(const mjResource* pResource, const char* szTimestamp)
    {
      return xiiStringUtils::IsEqual(pResource->timestamp, szTimestamp) ? 0 : 1;
    }

    mjpResourceProvider      m_Provider{};
    xiiMap<xiiString, Input> m_Files;
  };

  const char* FindOptionMismatch(const mjOption& reference, const mjOption& candidate)
  {
#define X(type, field, count)             \
  if (reference.field != candidate.field) \
    return #field;
#define XVEC(type, field, count)                                              \
  if (!std::equal(reference.field, reference.field + count, candidate.field)) \
    return #field;
    MJOPTION_FIELDS
#undef XVEC
#undef X
    return nullptr;
  }

  bool EqualLengthRangeOptions(const mjLROpt& a, const mjLROpt& b)
  {
    return a.mode == b.mode && a.useexisting == b.useexisting && a.uselimit == b.uselimit &&
      a.accel == b.accel && a.maxforce == b.maxforce && a.timeconst == b.timeconst && a.timestep == b.timestep &&
      a.inttotal == b.inttotal && a.interval == b.interval && a.tolrange == b.tolrange;
  }

  void PrepareSourceAssets(mjSpec* pSpec)
  {
    pSpec->compiler.usethread   = 0;
    pSpec->compiler.passthrough = mjPASSTHROUGH_NONE;
    for (const auto type : {mjOBJ_MESH, mjOBJ_HFIELD, mjOBJ_SKIN, mjOBJ_TEXTURE})
      for (auto* pElement = mjs_firstElement(pSpec, type); pElement; pElement = mjs_nextElement(pSpec, pElement))
      {
        if (auto* pCompiler = mjs_getCompiler(pElement))
        {
          pCompiler->usethread   = 0;
          pCompiler->passthrough = mjPASSTHROUGH_NONE;
        }

        // The attached spec uses the parent's strippath setting. Resolve that
        // option into each affected source asset before attaching it.
        const auto stripFile = [pSpec](mjString* pFile) {
          if (pSpec->strippath && *mjs_getString(pFile))
          {
            const xiiString name(xiiPathUtils::GetFileNameAndExtension(mjs_getString(pFile)));
            mjs_setString(pFile, name.GetData());
          }
        };
        switch (type)
        {
          case mjOBJ_MESH:
            stripFile(mjs_asMesh(pElement)->file);
            break;
          case mjOBJ_HFIELD:
            stripFile(mjs_asHField(pElement)->file);
            break;
          case mjOBJ_SKIN:
            stripFile(mjs_asSkin(pElement)->file);
            break;
          case mjOBJ_TEXTURE:
          {
            auto* pTexture  = mjs_asTexture(pElement);
            pTexture->embed = mjEMBED_TRUE;
            stripFile(pTexture->file);
            if (pSpec->strippath)
              for (int i = 0; i < 6; ++i)
                if (const char* szFile = mjs_getInStringVec(pTexture->cubefiles, i); szFile && *szFile)
                {
                  const xiiString name(xiiPathUtils::GetFileNameAndExtension(szFile));
                  mjs_setInStringVec(pTexture->cubefiles, i, name.GetData());
                }
            break;
          }
          default:
            break;
        }
      }
    pSpec->strippath = 0;
  }

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
  if (!m_ModelInstances.IsEmpty())
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
  if (!m_ModelInstances.IsEmpty())
  {
    xiiLog::Error("A MuJoCo model component already controls world '{}'.", GetWorld()->GetName());
    return XII_FAILURE;
  }

  return InstallModel(xiiMuJoCoCompiledModel::LoadBinary(modelBytes, "MJB buffer"), "MJB buffer");
}

xiiResult xiiMuJoCoPhysicsWorldModule::InstallModel(xiiSharedPtr<xiiMuJoCoCompiledModel> pModel, xiiStringView sSource, xiiArrayPtr<const InstalledInstance> instances)
{
  if (!pModel || m_bExecuting)
    return XII_FAILURE;

  const mjModel* pNativeModel = pModel->GetModel();
  const mjtSize  iMaxCount    = xiiMath::MaxValue<xiiInt32>();
  if (pNativeModel->nbody > iMaxCount || pNativeModel->njnt > iMaxCount || pNativeModel->nactuator > iMaxCount ||
      pNativeModel->nq > iMaxCount || pNativeModel->nv > iMaxCount || pNativeModel->nu > iMaxCount ||
      pNativeModel->na > iMaxCount || pNativeModel->nout > iMaxCount || pNativeModel->nsensor > iMaxCount ||
      pNativeModel->nsensordata > iMaxCount || pNativeModel->ngeom > iMaxCount || pNativeModel->nsite > iMaxCount || pNativeModel->nflex > iMaxCount)
  {
    xiiLog::Error("MuJoCo model '{}' exceeds the supported state indexing range.", sSource);
    return XII_FAILURE;
  }

  xiiUniquePtr<NativeSimulation> pSimulation = XII_NEW(GetAllocator(), NativeSimulation);
  pSimulation->m_pModel                      = std::move(pModel);
  pSimulation->m_Instances                   = instances;
  if (!instances.IsEmpty())
  {
    const auto mapOwners = [pNativeModel, instances](xiiDynamicArray<xiiComponentHandle>& ref_owners, mjtObj type, mjtSize iCount) {
      ref_owners.SetCount(static_cast<xiiUInt32>(iCount));
      if (instances.GetCount() == 1 && instances[0].m_sPrefix.IsEmpty())
      {
        for (auto& owner : ref_owners)
          owner = instances[0].m_hComponent;
        return;
      }
      for (xiiUInt32 i = 0; i < ref_owners.GetCount(); ++i)
      {
        const xiiStringView sName = mj_id2name(pNativeModel, type, static_cast<int>(i));
        for (const auto& instance : instances)
          if (sName.StartsWith(instance.m_sPrefix))
          {
            ref_owners[i] = instance.m_hComponent;
            break;
          }
      }
    };
    mapOwners(pSimulation->m_ElementOwners[0], mjOBJ_BODY, pNativeModel->nbody);
    mapOwners(pSimulation->m_ElementOwners[1], mjOBJ_JOINT, pNativeModel->njnt);
    mapOwners(pSimulation->m_ElementOwners[2], mjOBJ_ACTUATOR, pNativeModel->nactuator);
    mapOwners(pSimulation->m_ElementOwners[3], mjOBJ_SENSOR, pNativeModel->nsensor);
    mapOwners(pSimulation->m_ElementOwners[4], mjOBJ_GEOM, pNativeModel->ngeom);
    mapOwners(pSimulation->m_ElementOwners[5], mjOBJ_SITE, pNativeModel->nsite);
    mapOwners(pSimulation->m_FlexOwners, mjOBJ_FLEX, pNativeModel->nflex);
  }
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
  auto                       pPrevious = std::move(m_pSimulation);
  m_uiModelGeneration                  = s_uiNextGeneration.Increment();
  m_pSimulation                        = std::move(pSimulation);
  m_bExecuting                         = false;
  m_AccumulatedTime                    = xiiTime::MakeZero();
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

  m_AccumulatedTime      = xiiTime::MakeZero();
  m_bSingleStepRequested = false;
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

  xiiMuJoCoModelComponent* pComponent = nullptr;
  if (!GetWorld()->TryGetComponent(hOwner, pComponent))
    return;

  xiiUInt32  uiIndex   = 0;
  const auto uiOwnerId = hOwner.GetInternalID().m_InstanceIndex;
  while (uiIndex < m_ModelInstances.GetCount() && m_ModelInstances[uiIndex].m_hComponent.GetInternalID().m_InstanceIndex < uiOwnerId)
    ++uiIndex;
  if (uiIndex == m_ModelInstances.GetCount() || m_ModelInstances[uiIndex].m_hComponent != hOwner)
  {
    ModelInstance instance;
    instance.m_hComponent = hOwner;
    instance.m_hObject    = pComponent->GetOwner()->GetHandle();
    m_ModelInstances.InsertAt(uiIndex, std::move(instance));
  }

  auto& instance = m_ModelInstances[uiIndex];
  if (instance.m_hResource == hModel)
    return;
  instance.m_hResource = hModel;
  const xiiResourceLock<xiiMuJoCoModelResource> resource(hModel, xiiResourceAcquireMode::PointerOnly);
  instance.m_pResource = resource.GetPointer();
  m_bCompositionDirty  = true;
  if (resource->GetLoadingState() == xiiResourceState::Unloaded)
    xiiResourceManager::PreloadResource(hModel);
}

void xiiMuJoCoPhysicsWorldModule::ReleaseModel(xiiComponentHandle hOwner)
{
  if (m_bExecuting)
    return;

  for (xiiUInt32 i = 0; i < m_ModelInstances.GetCount(); ++i)
    if (m_ModelInstances[i].m_hComponent == hOwner)
    {
      m_ModelInstances.RemoveAtAndCopy(i);
      m_bCompositionDirty = true;
      return;
    }
}

void xiiMuJoCoPhysicsWorldModule::ReadPlacement(ModelInstance& ref_instance, bool bObserveOnly)
{
  xiiGameObject* pObject = nullptr;
  if (!GetWorld()->TryGetObject(ref_instance.m_hObject, pObject))
    return;
  pObject->UpdateGlobalTransform();
  const auto transform = pObject->GetGlobalTransform();
  if (!bObserveOnly && (!ref_instance.m_bObserved || !ref_instance.m_Observed.IsIdentical(transform)))
  {
    ref_instance.m_Placement = transform;
    m_bCompositionDirty      = true;
  }
  ref_instance.m_Observed  = transform;
  ref_instance.m_bObserved = true;
}

void xiiMuJoCoPhysicsWorldModule::UpdateRequestedModels()
{
  for (auto& instance : m_ModelInstances)
  {
    ReadPlacement(instance, false);
    m_bCompositionDirty |= instance.m_uiAttemptedRevision != instance.m_pResource->m_uiRevision;
  }
  if (!m_bCompositionDirty)
    return;

  // A pending or failed replacement can keep the old simulation only while every installed
  // component still participates. Removed components must stop contributing collision geometry.
  XII_SCOPE_EXIT(
    if (m_pSimulation) for (const auto& installed : m_pSimulation->m_Instances) {
      if (std::none_of(begin(m_ModelInstances), end(m_ModelInstances), [&installed](const ModelInstance& instance) { return instance.m_hComponent == installed.m_hComponent; }))
      {
        ClearSimulation();
        break;
      }
    });

  for (const auto& instance : m_ModelInstances)
    if (instance.m_pResource->GetLoadingState() == xiiResourceState::Unloaded)
    {
      m_sCompositionStatus = "Waiting for model resources.";
      return;
    }

  xiiHybridArray<xiiSharedPtr<xiiMuJoCoCompiledModel>, 8> models;
  xiiHybridArray<xiiUInt32, 8>                            revisions;
  for (const auto& instance : m_ModelInstances)
  {
    auto pModel = instance.m_pResource->GetModel(revisions.ExpandAndGetRef());
    if (!pModel && instance.m_pResource->GetLoadingState() != xiiResourceState::LoadedResourceMissing)
    {
      m_sCompositionStatus = "Waiting for compiled model resources.";
      return;
    }
    models.PushBack(std::move(pModel));
  }
  m_bCompositionDirty = false;
  m_bCompositionReady = false;
  CompositionLog    log;
  xiiLogSystemScope logScope(&log);
  m_sCompositionStatus = "No model instances registered.";
  XII_SCOPE_EXIT(if (!log.m_sError.IsEmpty()) m_sCompositionStatus = log.m_sError);
  if (m_ModelInstances.IsEmpty())
    return;

  xiiHybridArray<InstalledInstance, 8> installed;
  for (xiiUInt32 i = 0; i < m_ModelInstances.GetCount(); ++i)
    m_ModelInstances[i].m_uiAttemptedRevision = revisions[i];
  for (xiiUInt32 i = 0; i < m_ModelInstances.GetCount(); ++i)
  {
    const auto& instance  = m_ModelInstances[i];
    const auto& placement = instance.m_Placement;
    if (!models[i])
    {
      xiiLog::Error("MuJoCo world '{}' could not load model resource '{}'.", GetWorld()->GetName(), instance.m_hResource.GetResourceID());
      return;
    }
    const xiiGameObject* pObject = nullptr;
    if (!GetWorld()->TryGetObject(instance.m_hObject, pObject))
      return;
    if (!placement.IsValid() || !placement.m_vScale.IsEqual(xiiVec3(1.0f), xiiMath::SmallEpsilon<float>()))
    {
      xiiLog::Error("MuJoCo model '{}' on '{}' requires a valid, unscaled placement.", instance.m_hResource.GetResourceID(), pObject->GetName());
      return;
    }
    auto& entry        = installed.ExpandAndGetRef();
    entry.m_hComponent = instance.m_hComponent;
  }

  const auto& firstPlacement = m_ModelInstances[0].m_Placement;
  const auto  result         = models.GetCount() == 1 && firstPlacement.m_vPosition.IsZero() && firstPlacement.m_qRotation.IsEqualRotation(xiiQuat::MakeIdentity(), xiiMath::SmallEpsilon<float>()) ?
    InstallModel(std::move(models[0]), m_ModelInstances[0].m_hResource.GetResourceID(), installed) :
    ComposeModels(models, installed);
  m_bCompositionReady        = result.Succeeded();
  m_sCompositionStatus       = result.Succeeded() ? "Ready" : "Composition failed; the previous valid simulation is retained where safe.";
}

xiiResult xiiMuJoCoPhysicsWorldModule::ComposeModels(xiiArrayPtr<const xiiSharedPtr<xiiMuJoCoCompiledModel>> models, xiiArrayPtr<InstalledInstance> instances)
{
  XII_PROFILE_SCOPE("MuJoCo Model Composition");
  CompositionInputs inputs;
  if (!inputs.m_bMounted)
  {
    xiiLog::Error("MuJoCo could not mount model inputs for world '{}'.", GetWorld()->GetName());
    return XII_FAILURE;
  }
  const auto& options = models[0]->GetModel()->opt;
  for (xiiUInt32 i = 0; i < models.GetCount(); ++i)
  {
    const auto sResource = m_ModelInstances[i].m_hResource.GetResourceID();
    const auto files     = models[i]->GetSourceFiles();
    if (files.IsEmpty())
    {
      xiiLog::Error("MuJoCo resource '{}' has no source snapshot. Transform an MJCF or URDF asset to compose or place this model.", sResource);
      return XII_FAILURE;
    }
    if (const char* szField = FindOptionMismatch(options, models[i]->GetModel()->opt))
    {
      xiiLog::Error("MuJoCo resource '{}' has a conflicting '{}' option. Composed models must use matching world physics options.", sResource, szField);
      return XII_FAILURE;
    }
    if (!inputs.Add(files))
      return XII_FAILURE;
  }

  xiiMuJoCoCompiledModel::ModelPtr pModel(nullptr, mj_deleteModel);
  {
    m_bExecuting = true;
    XII_SCOPE_EXIT(m_bExecuting = false);
    // The input snapshots and their parsed specs are shared by repeated instances during this build.
    xiiMap<const xiiMuJoCoCompiledModel*, SpecPtr> sources;
    SpecPtr                                        parent(mj_makeSpec());
    if (!parent)
    {
      xiiLog::Error("MuJoCo could not create a model specification for world '{}'.", GetWorld()->GetName());
      return XII_FAILURE;
    }
    parent->option             = options;
    parent->compiler.usethread = 0;
    parent->compiler.conflict  = mjCONFLICT_ERROR;
    mjs_setDeepCopy(parent.get(), 1);
    auto* pWorldBody = mjs_findBody(parent.get(), "world");

    for (xiiUInt32 i = 0; i < instances.GetCount(); ++i)
    {
      auto&       instance  = instances[i];
      const auto& request   = m_ModelInstances[i];
      const auto  sResource = request.m_hResource.GetResourceID();
      auto        source    = sources.Find(models[i].Borrow());
      if (!source.IsValid())
      {
        char        error[4096] = {};
        const auto& root        = models[i]->GetSourceFiles()[0];
        SpecPtr     spec(mj_parseXML(root.m_sPath.GetData(), &inputs.m_Vfs, error, sizeof(error)));
        if (!spec)
        {
          xiiLog::Error("MuJoCo could not parse the source snapshot of '{}': {}", sResource, error);
          return XII_FAILURE;
        }
        if (spec->compiler.settotalmass > 0.0)
        {
          xiiLog::Error("MuJoCo resource '{}' uses settotalmass, which cannot be applied per instance.", sResource);
          return XII_FAILURE;
        }
        PrepareSourceAssets(spec.get());
        // These compiler operations apply to a complete spec. Apply them before attachment
        // so the importing model cannot restructure any other instance.
        if ((spec->compiler.discardvisual && mjs_discardVisual(spec.get(), &inputs.m_Vfs) < 0) ||
            (spec->compiler.fusestatic && mjs_fuseStatic(spec.get(), &inputs.m_Vfs) < 0))
        {
          xiiLog::Error("MuJoCo could not prepare '{}' for attachment: {}", sResource, mjs_getError(spec.get()));
          return XII_FAILURE;
        }
        // Unnamed elements need an attachment namespace for inspection and contact ownership.
        for (const auto type : {mjOBJ_BODY, mjOBJ_JOINT, mjOBJ_ACTUATOR, mjOBJ_SENSOR, mjOBJ_SITE, mjOBJ_GEOM, mjOBJ_FLEX})
        {
          xiiUInt32 uiGenerated = 0;
          for (auto* pElement = mjs_firstElement(spec.get(), type); pElement; pElement = mjs_nextElement(spec.get(), pElement))
            if ((type != mjOBJ_BODY || mjs_getParent(pElement)) && *mjs_getString(mjs_getName(pElement)) == '\0')
            {
              xiiStringBuilder sName;
              do
              {
                sName.SetFormat("__xii_unnamed_{}", uiGenerated++);
              } while (mjs_findElement(spec.get(), type, sName.GetData()));
              if (mjs_setName(pElement, sName.GetData()) != 0)
              {
                xiiLog::Error("MuJoCo could not name an unnamed element in '{}': {}", sResource, mjs_getError(spec.get()));
                return XII_FAILURE;
              }
            }
        }
        source = sources.Insert(models[i].Borrow(), std::move(spec));
      }

      auto* pSource = source.Value().get();
      if (i == 0)
      {
        parent->visual         = pSource->visual;
        parent->authored       = pSource->authored;
        parent->memory         = pSource->memory;
        parent->compiler.LRopt = pSource->compiler.LRopt;
      }
      else if (!EqualLengthRangeOptions(parent->compiler.LRopt, pSource->compiler.LRopt))
      {
        xiiLog::Error("MuJoCo resource '{}' has incompatible actuator length-range compiler options.", sResource);
        return XII_FAILURE;
      }
      // mjs_attach adds a world frame to the supplied spec. Keep the parsed spec pristine for reuse.
      SpecPtr child(mj_copySpec(pSource));
      if (!child)
      {
        xiiLog::Error("MuJoCo could not copy the specification of '{}'.", sResource);
        return XII_FAILURE;
      }
      auto*       pFrame    = mjs_addFrame(pWorldBody, nullptr);
      const auto& placement = request.m_Placement;
      const auto& position  = placement.m_vPosition;
      const auto& rotation  = placement.m_qRotation;
      pFrame->pos[0]        = position.x;
      pFrame->pos[1]        = position.y;
      pFrame->pos[2]        = position.z;
      pFrame->quat[0]       = rotation.w;
      pFrame->quat[1]       = rotation.x;
      pFrame->quat[2]       = rotation.y;
      pFrame->quat[3]       = rotation.z;
      const auto       id   = instance.m_hComponent.GetInternalID();
      xiiStringBuilder sPrefix;
      sPrefix.SetFormat("xii_{}_{}_", static_cast<xiiUInt32>(id.m_InstanceIndex), static_cast<xiiUInt32>(id.m_Generation));
      instance.m_sPrefix = sPrefix;
      if (!mjs_attach(pFrame->element, child->element, instance.m_sPrefix.GetData(), nullptr))
      {
        xiiLog::Error("MuJoCo could not attach '{}' in world '{}': {}", sResource, GetWorld()->GetName(), mjs_getError(parent.get()));
        return XII_FAILURE;
      }
    }

    pModel.reset(mj_compile(parent.get(), &inputs.m_Vfs));
    if (!pModel)
    {
      xiiLog::Error("MuJoCo could not compile the composition for world '{}': {}", GetWorld()->GetName(), mjs_getError(parent.get()));
      return XII_FAILURE;
    }
    if (const char* szField = FindOptionMismatch(options, pModel->opt))
    {
      xiiLog::Error("MuJoCo composition for world '{}' changed the effective '{}' option.", GetWorld()->GetName(), szField);
      return XII_FAILURE;
    }
    mjtSize iBodies = 1, iJoints = 0, iGeometries = 0, iActuators = 0, iSensors = 0, iPositions = 0, iVelocities = 0, iSites = 0, iFlexes = 0;
    mjtSize iContactPairs = 0, iExclusions = 0, iEqualities = 0, iTendons = 0;
    for (const auto& model : models)
    {
      const auto& native = *model->GetModel();
      iBodies += native.nbody - 1;
      iJoints += native.njnt;
      iGeometries += native.ngeom;
      iActuators += native.nactuator;
      iSensors += native.nsensor;
      iPositions += native.nq;
      iVelocities += native.nv;
      iSites += native.nsite;
      iFlexes += native.nflex;
      iContactPairs += native.npair;
      iExclusions += native.nexclude;
      iEqualities += native.neq;
      iTendons += native.ntendon;
    }
    if (pModel->nbody != iBodies || pModel->njnt != iJoints || pModel->ngeom != iGeometries || pModel->nactuator != iActuators ||
        pModel->nsensor != iSensors || pModel->nq != iPositions || pModel->nv != iVelocities || pModel->nsite != iSites || pModel->nflex != iFlexes ||
        pModel->npair != iContactPairs || pModel->nexclude != iExclusions || pModel->neq != iEqualities || pModel->ntendon != iTendons)
    {
      xiiLog::Error("MuJoCo composition for world '{}' did not preserve the input model elements or degrees of freedom.", GetWorld()->GetName());
      return XII_FAILURE;
    }
    for (int i = 0; i < mjs_numWarnings(parent.get()); ++i)
      xiiLog::Warning("MuJoCo composition for world '{}': {}", GetWorld()->GetName(), mjs_getWarning(parent.get(), i));
  }
  const auto sWorld = GetWorld()->GetName();
  return InstallModel(xiiMuJoCoCompiledModel::Adopt(std::move(pModel), sWorld), sWorld, instances);
}

void xiiMuJoCoPhysicsWorldModule::ClearSimulation()
{
  m_bCompositionReady    = false;
  m_bSingleStepRequested = false;
  m_uiModelGeneration    = 0;
  m_bExecuting           = true;
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
  ref_binding.m_iBodyId        = -1;
  const xiiGameObject* pObject = nullptr;
  if (m_pSimulation && GetWorld()->TryGetObject(ref_binding.m_hObject, pObject))
  {
    ref_binding.m_iBodyId = ResolveElementForObject(pObject, xiiMuJoCoElementType::Body, ref_binding.m_sBodyName).m_iIndex;
    if (ref_binding.m_iBodyId < 0)
      xiiLog::Warning("MuJoCo body '{}' was not found in the model for world '{}'.", ref_binding.m_sBodyName, GetWorld()->GetName());
  }
}

void xiiMuJoCoPhysicsWorldModule::SynchronizeBodies()
{
  if (!m_bSyncBodies || !m_pSimulation || m_bAuthoringPreview)
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
  const auto& data            = *m_pSimulation->m_pData;
  bool        bWroteTransform = false;
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
    bWroteTransform = true;
  }
  // Scene output can move an instance's own anchor or an ancestor. Keep the authored
  // placement used by composition, but recognize those writes as observed scene state.
  if (bWroteTransform)
    for (auto& instance : m_ModelInstances)
      ReadPlacement(instance, true);
}

void xiiMuJoCoPhysicsWorldModule::Initialize()
{
  auto desc        = XII_CREATE_MODULE_UPDATE_FUNCTION_DESC(xiiMuJoCoPhysicsWorldModule::Simulate, this);
  desc.m_Phase     = xiiWorldUpdatePhase::PreAsync;
  desc.m_fPriority = -100000.0f;
  RegisterUpdateFunction(desc);
  auto debug        = XII_CREATE_MODULE_UPDATE_FUNCTION_DESC(xiiMuJoCoPhysicsWorldModule::PrepareDebugDraw, this);
  debug.m_Phase     = xiiWorldUpdatePhase::PostAsync;
  debug.m_fPriority = -100000.0f;
  RegisterUpdateFunction(debug);
}

void xiiMuJoCoPhysicsWorldModule::Deinitialize()
{
  auto desc    = XII_CREATE_MODULE_UPDATE_FUNCTION_DESC(xiiMuJoCoPhysicsWorldModule::Simulate, this);
  desc.m_Phase = xiiWorldUpdatePhase::PreAsync;
  DeregisterUpdateFunction(desc);
  auto debug    = XII_CREATE_MODULE_UPDATE_FUNCTION_DESC(xiiMuJoCoPhysicsWorldModule::PrepareDebugDraw, this);
  debug.m_Phase = xiiWorldUpdatePhase::PostAsync;
  DeregisterUpdateFunction(debug);
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
  m_ModelInstances.Clear();
  m_bDebugRequested    = false;
  m_bCompositionDirty  = false;
  m_sCompositionStatus = "No model instances registered.";
  m_bScenePreview = m_bAuthoringPreview = m_bDeferComposition = m_bSingleStepRequested = false;
  ClearSimulation();
}

void xiiMuJoCoPhysicsWorldModule::Simulate(const UpdateContext&)
{
  if (m_bDeferComposition)
    return;
  const auto generation = m_uiModelGeneration;
  UpdateRequestedModels();
  const bool singleStep = std::exchange(m_bSingleStepRequested, false) && generation == m_uiModelGeneration && GetWorld()->GetClock().GetPaused();
  if (!m_pSimulation || m_bAuthoringPreview || !GetWorld()->GetWorldSimulationEnabled())
    return;

  EvaluateState();
  m_pSimulation->m_StepDiagnostics.m_uiStepsLastUpdate      = 0;
  m_pSimulation->m_StepDiagnostics.m_UpdateSteppingWallTime = xiiTime::MakeZero();
  XII_SCOPE_EXIT(SynchronizeBodies());
  const xiiClock& clock         = GetWorld()->GetClock();
  const double    fDeltaSeconds = clock.GetTimeDiff().GetSeconds();
  if (!singleStep && (clock.GetPaused() || !xiiMath::IsFinite(fDeltaSeconds) || fDeltaSeconds <= 0.0))
    return;

  const mjModel* pModel          = m_pSimulation->m_pModel->GetModel();
  const double   fStepSeconds    = pModel->opt.timestep;
  const double   fElapsedSeconds = singleStep ? fStepSeconds : m_AccumulatedTime.GetSeconds() + fDeltaSeconds;
  if (!singleStep)
    m_AccumulatedTime = xiiTime::MakeFromSeconds(fElapsedSeconds);
  if (fElapsedSeconds < fStepSeconds)
    return;

  // Keep the fractional step across frames and discard whole steps beyond the catch-up limit.
  const double fElapsedSteps = xiiMath::Floor(fElapsedSeconds / fStepSeconds);
  const auto   uiSteps       = static_cast<xiiUInt32>(xiiMath::Min(fElapsedSteps, static_cast<double>(m_uiMaxCatchUpSteps)));
  const double fRemainder    = fElapsedSteps > m_uiMaxCatchUpSteps ? xiiMath::Mod(fElapsedSeconds, fStepSeconds) : xiiMath::Max(0.0, fElapsedSeconds - uiSteps * fStepSeconds);
  if (!singleStep)
    m_AccumulatedTime = xiiTime::MakeFromSeconds(fRemainder);

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

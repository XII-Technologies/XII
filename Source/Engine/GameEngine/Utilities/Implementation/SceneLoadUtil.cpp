/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GameEngine/GameEnginePCH.h>

#include <Core/Collection/CollectionResource.h>
#include <Foundation/Threading/TaskSystem.h>
#include <Foundation/Utilities/AssetFileHeader.h>
#include <GameEngine/GameApplication/GameApplication.h>
#include <GameEngine/Utilities/SceneLoadUtil.h>

// preloading assets is considered to be the vast majority of scene loading
constexpr float fCollectionPreloadPiece = 0.9f;

// Owns all worker data. World creation, component deserialization and activation never run here.
class xiiSceneReadTask final : public xiiTask
{
public:
  xiiSceneReadTask(xiiStringView sPath) : m_sPath(sPath)
  {
    ConfigureTask("Read scene description", xiiTaskNesting::Never);
  }

  xiiString          m_sPath;
  xiiString          m_sError;
  xiiWorldReader     m_Reader;
  xiiAtomicInteger32 m_iCancelled = 0;

  void Execute() override
  {
    if (m_iCancelled != 0)
      return;

    xiiFileReader file;
    if (file.Open(m_sPath).Failed())
    {
      m_sError = "Cannot open the transformed scene. Export/transform it and include it in the package.";
      return;
    }

    // The shared header reader asserts on malformed envelopes. Validate external input first.
    xiiUInt8 envelope[9] = {};
    if (file.ReadBytes(envelope, sizeof(envelope)) != sizeof(envelope) ||
        xiiMemoryUtils::Compare(envelope, reinterpret_cast<const xiiUInt8*>("xiiAsset"), 8) != 0 || envelope[8] != 3)
    {
      m_sError = "Invalid asset envelope. Re-export the scene with this engine version.";
      return;
    }
    file.Close();
    if (file.Open(m_sPath).Failed())
    {
      m_sError = "The scene became unavailable while loading.";
      return;
    }

    xiiAssetFileHeader header;
    char               szSceneTag[16] = {};
    if (header.Read(file).Failed() || file.ReadBytes(szSceneTag, sizeof(szSceneTag)) != sizeof(szSceneTag) ||
        !xiiStringUtils::IsEqualN(szSceneTag, "[xiiBinaryScene]", 16))
    {
      m_sError = "Invalid binary scene header. Re-export the scene with this engine version.";
      return;
    }

    if (m_iCancelled == 0 && m_Reader.ReadWorldDescription(file).Failed())
      m_sError = "Cannot deserialize the scene description. Check the scene version and required plugins.";
  }
};

xiiSceneLoadUtility::xiiSceneLoadUtility() = default;

xiiSceneLoadUtility::~xiiSceneLoadUtility()
{
  Cancel();
  // Destruction is the final join boundary, before plugin unloading. Normal cancellation retains
  // this utility until IsReadTaskFinished(), so it never waits in a running application's frame.
  if (m_pReadTask)
    xiiTaskSystem::CancelTask(m_pReadTask, xiiOnTaskRunning::WaitTillFinished).IgnoreResult();
}

bool xiiSceneLoadUtility::IsReadTaskFinished() const
{
  return !m_pReadTask || m_pReadTask->IsTaskFinished();
}

void xiiSceneLoadUtility::Cancel()
{
  if (m_pReadTask)
    m_pReadTask->m_iCancelled = 1;
  if (m_pInstantiationContext)
  {
    XII_LOCK(m_pWorld->GetWriteMarker());
    m_pInstantiationContext->Cancel();
    m_pInstantiationContext.Clear();
  }
  m_pWorld.Clear();
  m_hPreloadCollection.Invalidate();
  if (m_LoadingState == LoadingState::Ongoing)
    m_LoadingState = LoadingState::Cancelled;
}

void xiiSceneLoadUtility::StartSceneLoading(xiiStringView sSceneFile, xiiStringView sPreloadCollectionFile)
{
  XII_ASSERT_DEV(m_LoadingState == LoadingState::NotStarted, "Can't reuse a xiiSceneLoadUtility.");

  XII_LOG_BLOCK("StartSceneLoading");

  m_LoadingState = LoadingState::Ongoing;
  m_sRequestedCollection = sPreloadCollectionFile;

  m_sRequestedFile                 = sSceneFile;
  xiiStringBuilder sFinalSceneFile = sSceneFile;

  if (sFinalSceneFile.IsEmpty())
  {
    LoadingFailed("No scene file specified.");
    return;
  }

  xiiLog::Info("Loading scene '{}'.", sSceneFile);

  if (sFinalSceneFile.IsAbsolutePath())
  {
    // this can fail if the scene is in a different data directory than the project directory
    // shouldn't stop us from loading it anyway
    sFinalSceneFile.MakeRelativeTo(xiiGameApplication::GetGameApplicationInstance()->GetAppProjectPath()).IgnoreResult();
  }

  if (sFinalSceneFile.HasExtension("xiiScene") || sFinalSceneFile.HasExtension("xiiPrefab"))
  {
    if (sFinalSceneFile.IsAbsolutePath())
    {
      if (xiiFileSystem::ResolvePath(sFinalSceneFile, nullptr, &sFinalSceneFile).Failed())
      {
        LoadingFailed(xiiFmt("Scene path is not located in any data directory: '{}'", sFinalSceneFile));
        return;
      }
    }

    // if this is a path to the non-transformed source file, redirect it to the transformed file in the asset cache
    sFinalSceneFile.Prepend("AssetCache/Common/");

    if (sFinalSceneFile.HasExtension("xiiScene"))
      sFinalSceneFile.ChangeFileExtension("xiiBinScene");
    else
      sFinalSceneFile.ChangeFileExtension("xiiBinPrefab");
  }

  if (sFinalSceneFile != sSceneFile)
  {
    xiiLog::Dev("Redirecting scene file from '{}' to '{}'", sSceneFile, sFinalSceneFile);
  }

  m_sRedirectedFile = sFinalSceneFile;

  if (!sPreloadCollectionFile.IsEmpty())
  {
    m_hPreloadCollection = xiiResourceManager::LoadResource<xiiCollectionResource>(xiiString(sPreloadCollectionFile));
  }
}

xiiUniquePtr<xiiWorld> xiiSceneLoadUtility::RetrieveLoadedScene()
{
  XII_ASSERT_DEV(m_LoadingState == LoadingState::FinishedSuccessfully, "Can't retrieve a scene when loading hasn't finished successfully.");

  m_LoadingState = LoadingState::FinishedAndRetrieved;

  XII_LOCK(m_pWorld->GetWriteMarker());
  m_pWorld->SetWorldSimulationEnabled(true);

  return std::move(m_pWorld);
}

void xiiSceneLoadUtility::LoadingFailed(const xiiFormatString& reason)
{
  XII_ASSERT_DEV(m_LoadingState == LoadingState::Ongoing, "Invalid loading state");
  m_LoadingState = LoadingState::Failed;

  xiiStringBuilder tmp;
  m_sFailureReason = reason.GetText(tmp);
}

void xiiSceneLoadUtility::TickSceneLoading()
{
  switch (m_LoadingState)
  {
    case LoadingState::FinishedSuccessfully:
    case LoadingState::Failed:
    case LoadingState::Cancelled:
    case LoadingState::NotStarted:
    case LoadingState::FinishedAndRetrieved:
      return;

    default:
      break;
  }

  XII_PROFILE_SCOPE("TickSceneLoading");

  // update our current loading progress
  {
    m_fLoadingProgress = fCollectionPreloadPiece;

    // if we have a collection, preload that first
    if (m_hPreloadCollection.IsValid())
    {
      m_fLoadingProgress = 0.0f;

      xiiResourceLock<xiiCollectionResource> pCollection(m_hPreloadCollection, xiiResourceAcquireMode::AllowLoadingFallback_NeverFail);

      if (pCollection.GetAcquireResult() == xiiResourceAcquireResult::Final)
      {
        // Queue a bounded number each frame; true means more entries remain, not failure.
        if (pCollection->PreloadResources(64))
          return;

        float progress = 0.0f;
        if (pCollection->IsLoadingFinished(&progress))
        {
          m_fLoadingProgress = fCollectionPreloadPiece;
        }
        else
        {
          m_fLoadingProgress = progress * fCollectionPreloadPiece;
        }
      }
      else if (xiiResourceManager::GetLoadingState(m_hPreloadCollection) == xiiResourceState::LoadedResourceMissing)
      {
        LoadingFailed("The preload collection is missing. Export it and include its dependencies in the package.");
        return;
      }
    }

    // if preloading the collection is finished (or we just don't have one) add the world instantiation progress
    if (m_fLoadingProgress == fCollectionPreloadPiece)
    {
      m_fLoadingProgress += m_InstantiationProgress.GetCompletion() * (1.0f - fCollectionPreloadPiece);
    }
  }

  // as long as we are still pre-loading assets from the collection, don't do anything else
  if (m_fLoadingProgress < fCollectionPreloadPiece)
    return;

  if (!m_pReadTask)
  {
    m_LoadingPhase = LoadingPhase::Reading;
    m_pReadTask    = XII_DEFAULT_NEW(xiiSceneReadTask, m_sRedirectedFile);
    // A custom type-lookup callback may have application-specific thread affinity. Preserve
    // that contract; the normal RTTI/tag registry lookup and graph read are thread-safe.
    if (xiiWorldReader::s_FindComponentTypeCallback.IsValid())
      m_pReadTask->Execute();
    else
      xiiTaskSystem::StartSingleTask(m_pReadTask, xiiTaskPriority::FileAccess);
    return;
  }

  if (!m_pReadTask->IsTaskFinished())
    return;

  if (!m_pReadTask->m_sError.IsEmpty())
  {
    LoadingFailed(xiiFmt("{} Scene: '{}'", m_pReadTask->m_sError, m_sRedirectedFile));
    return;
  }

  // Create and instantiate the world only on the owning thread.
  if (m_pWorld == nullptr)
  {
    XII_LOG_BLOCK("LoadObjectGraph", m_sRedirectedFile);

    xiiWorldDescription desc(m_sRedirectedFile);
    m_pWorld = XII_DEFAULT_NEW(xiiWorld, desc);
    m_pWorld->SetWorldSimulationEnabled(false);

    XII_LOCK(m_pWorld->GetWriteMarker());

    m_LoadingPhase          = LoadingPhase::Instantiating;
    m_pInstantiationContext = m_pReadTask->m_Reader.InstantiateWorld(*m_pWorld, nullptr, xiiTime::MakeFromMilliseconds(1), &m_InstantiationProgress);
  }
  else if (m_pInstantiationContext)
  {
    xiiWorldReader::InstantiationContextBase::StepResult res = m_pInstantiationContext->Step();

    if (res == xiiWorldReader::InstantiationContextBase::StepResult::ContinueNextFrame)
    {
      // TODO: can we finish the world instantiation without updating the entire world?
      // E.g. only finish component instantiation?
      // also we may want to step the world only with a very small (and fixed!) time-step

      XII_LOCK(m_pWorld->GetWriteMarker());
      m_pWorld->Update();
    }
    else if (res == xiiWorldReader::InstantiationContextBase::StepResult::Finished)
    {
      // TODO: ticking twice seems to fix some Jolt physics issues
      XII_LOCK(m_pWorld->GetWriteMarker());
      m_pWorld->Update();

      m_pInstantiationContext = nullptr;
      m_LoadingState          = LoadingState::FinishedSuccessfully;
      m_LoadingPhase          = LoadingPhase::Ready;
      m_fLoadingProgress      = 1.0f;
    }

    if (m_LoadingState == LoadingState::Ongoing)
      m_fLoadingProgress = fCollectionPreloadPiece + m_InstantiationProgress.GetCompletion() * (1.0f - fCollectionPreloadPiece);
  }
  else
  {
    XII_REPORT_FAILURE("Invalid code path.");
  }
}

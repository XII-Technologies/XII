/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GameEngine/GameEnginePCH.h>

#include <Core/Collection/CollectionResource.h>
#include <Foundation/Utilities/AssetFileHeader.h>
#include <GameEngine/GameApplication/GameApplication.h>
#include <GameEngine/Utils/SceneLoadUtil.h>

// preloading assets is considered to be the vast majority of scene loading
constexpr float fCollectionPreloadPiece = 0.9f;

xiiSceneLoadUtility::xiiSceneLoadUtility()  = default;
xiiSceneLoadUtility::~xiiSceneLoadUtility() = default;

void xiiSceneLoadUtility::StartSceneLoading(xiiStringView sSceneFile, xiiStringView sPreloadCollectionFile)
{
  XII_ASSERT_DEV(m_LoadingState == LoadingState::NotStarted, "Can't reuse a xiiSceneLoadUtility.");

  XII_LOG_BLOCK("StartSceneLoading");

  m_LoadingState = LoadingState::Ongoing;

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
        if (pCollection->PreloadResources())
        {
          XII_REPORT_FAILURE("Failed to start preloading all resources.");
        }

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

  // if we haven't created a world yet, do so now, and set up an instantiation context
  if (m_pWorld == nullptr)
  {
    XII_LOG_BLOCK("LoadObjectGraph", m_sRedirectedFile);

    xiiWorldDesc desc(m_sRedirectedFile);
    m_pWorld = XII_DEFAULT_NEW(xiiWorld, desc);
    m_pWorld->SetWorldSimulationEnabled(false);

    XII_LOCK(m_pWorld->GetWriteMarker());

    if (m_FileReader.Open(m_sRedirectedFile).Failed())
    {
      LoadingFailed("Failed to open the file.");
      return;
    }
    else
    {
      // Read and skip the asset file header
      xiiAssetFileHeader header;
      header.Read(m_FileReader).AssertSuccess();

      char szSceneTag[16];
      m_FileReader.ReadBytes(szSceneTag, sizeof(char) * 16);

      if (!xiiStringUtils::IsEqualN(szSceneTag, "[xiiBinaryScene]", 16))
      {
        LoadingFailed("The given file isn't an object-graph file.");
        return;
      }

      if (m_WorldReader.ReadWorldDescription(m_FileReader).Failed())
      {
        LoadingFailed("Error reading world description.");
        return;
      }

      // TODO: make frame time configurable ?
      m_pInstantiationContext = m_WorldReader.InstantiateWorld(*m_pWorld, nullptr, xiiTime::MakeFromMilliseconds(1), &m_InstantiationProgress);
    }
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
    }

    m_fLoadingProgress = fCollectionPreloadPiece + m_InstantiationProgress.GetCompletion() * (1.0f - fCollectionPreloadPiece);
  }
  else
  {
    XII_REPORT_FAILURE("Invalid code path.");
  }
}

XII_STATICLINK_FILE(GameEngine, GameEngine_Utils_Implementation_SceneLoadUtil);

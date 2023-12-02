#pragma once

#include <GameEngine/GameEngineDLL.h>

#include <Core/ResourceManager/ResourceHandle.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/Utilities/Progress.h>
#include <Foundation/Types/UniquePtr.h>

using xiiCollectionResourceHandle = xiiTypedResourceHandle<class xiiCollectionResource>;

/// \brief This class allows to load a scene in the background and switch to it, once loading has finished.
class XII_GAMEENGINE_DLL xiiSceneLoadUtility
{
  XII_DISALLOW_COPY_AND_ASSIGN(xiiSceneLoadUtility);

public:
  xiiSceneLoadUtility();
  ~xiiSceneLoadUtility();

  enum class LoadingState
  {
    NotStarted,
    Ongoing,
    FinishedSuccessfully,
    Failed,
  };

  /// \brief Returns whether loading is still ongoing or finished.
  LoadingState GetLoadingState() const { return m_LoadingState; }

  /// \brief Returns a loading progress value in 0 to 1 range.
  float GetLoadingProgress() const { return m_fLoadingProgress; }

  /// \brief In case loading failed, this returns what went wrong.
  xiiStringView GetLoadingFailureReason() const { return m_sFailureReason; }

  /// \brief Starts loading a scene. If provided, the assets in the collection are loaded first and then the scene is instantiated.
  ///
  /// Using a collection will make loading in the background much smoother. Without it, most assets will be loaded once the scene gets updated
  /// for the first time, resulting in very long delays.
  void StartSceneLoading(xiiStringView sSceneFile, xiiStringView sPreloadCollectionFile);

  /// \brief This has to be called periodically (usually once per frame) to progress the scene loading.
  ///
  /// Call GetLoadingState() afterwards to check whether loading has finished or failed.
  void TickSceneLoading();

  /// \brief Once loading is finished successfully, call this to take ownership of the loaded scene.
  ///
  /// Afterwards there is no point in keeping the xiiSceneLoadUtility around anymore and it should be deleted.
  xiiUniquePtr<xiiWorld> RetrieveLoadedScene();

private:
  void LoadingFailed(const xiiFormatString& reason);

  LoadingState m_LoadingState     = LoadingState::NotStarted;
  float        m_fLoadingProgress = 0.0f;
  xiiString    m_sFailureReason;

  xiiString                                              m_sFile;
  xiiCollectionResourceHandle                            m_hPreloadCollection;
  xiiFileReader                                          m_FileReader;
  xiiWorldReader                                         m_WorldReader;
  xiiUniquePtr<xiiWorld>                                 m_pWorld;
  xiiUniquePtr<xiiWorldReader::InstantiationContextBase> m_pInstantiationContext;
  xiiProgress                                            m_InstantiationProgress;
};

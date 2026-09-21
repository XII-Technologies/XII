/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Core/ResourceManager/Implementation/Declarations.h>
#include <Core/ResourceManager/ResourceTypeLoader.h>
#include <Foundation/Threading/TaskSystem.h>
#include <Foundation/Types/UniquePtr.h>

/// [internal] Worker task for loading resources (typically from disk).
class XII_CORE_DLL xiiResourceManagerWorkerDataLoad final : public xiiTask
{
public:
  ~xiiResourceManagerWorkerDataLoad();

private:
  friend class xiiResourceManager;
  friend class xiiResourceManagerState;

  xiiResourceManagerWorkerDataLoad();

  virtual void Execute() override;
};

/// [internal] Worker task for uploading resource data.
/// Depending on the resource type, this may get scheduled to run on the main thread or on any thread.
class XII_CORE_DLL xiiResourceManagerWorkerUpdateContent final : public xiiTask
{
public:
  ~xiiResourceManagerWorkerUpdateContent();

  xiiResourceLoadData    m_LoaderData;
  xiiResource*           m_pResourceToLoad = nullptr;
  xiiResourceTypeLoader* m_pLoader         = nullptr;
  // this is only used to clean up a custom loader at the right time, if one is used
  // m_pLoader is always set, no need to go through m_pCustomLoader
  xiiUniquePtr<xiiResourceTypeLoader> m_pCustomLoader;

private:
  friend class xiiResourceManager;
  friend class xiiResourceManagerState;
  friend class xiiResourceManagerWorkerDataLoad;

  xiiResourceManagerWorkerUpdateContent();

  virtual void Execute() override;
};

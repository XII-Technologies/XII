/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <MujocoPlugin/MujocoPluginDLL.h>

#include <Core/ResourceManager/Resource.h>
#include <Foundation/Threading/AtomicInteger.h>
#include <Foundation/Threading/Mutex.h>
#include <Foundation/Types/SharedPtr.h>

class xiiMuJoCoCompiledModel;
using xiiMuJoCoModelResourceHandle = xiiTypedResourceHandle<class xiiMuJoCoModelResource>;

/// A compiled MuJoCo model. The default resource loader reads a loose MJB file.
/// Custom streams use the same format: the loader's source-path string followed by MJB bytes.
class XII_MUJOCOPLUGIN_DLL xiiMuJoCoModelResource final : public xiiResource
{
  XII_ADD_DYNAMIC_REFLECTION(xiiMuJoCoModelResource, xiiResource);
  XII_RESOURCE_DECLARE_COMMON_CODE(xiiMuJoCoModelResource);

public:
  xiiMuJoCoModelResource();
  ~xiiMuJoCoModelResource() override;

private:
  friend class xiiMuJoCoPhysicsWorldModule;

  xiiResourceLoadDescription UnloadData(Unload whatToUnload) override;
  xiiResourceLoadDescription UpdateContent(xiiStreamReader* pStream) override;
  void                       UpdateMemoryUsage(MemoryUsage& out_usage) override;

  xiiSharedPtr<xiiMuJoCoCompiledModel> GetModel(xiiUInt32& out_uiRevision) const;

  mutable xiiMutex                     m_ModelMutex;
  xiiSharedPtr<xiiMuJoCoCompiledModel> m_pModel;
  xiiAtomicIntegerU32                  m_uiRevision;
};

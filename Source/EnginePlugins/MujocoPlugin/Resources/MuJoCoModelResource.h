/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <MujocoPlugin/MujocoPluginDLL.h>

#include <Core/ResourceManager/Resource.h>
#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Threading/AtomicInteger.h>
#include <Foundation/Threading/Mutex.h>
#include <Foundation/Types/SharedPtr.h>

class xiiMuJoCoCompiledModel;
using xiiMuJoCoModelResourceHandle = xiiTypedResourceHandle<class xiiMuJoCoModelResource>;

/// Versioned MJB and optional source snapshot used after the standard XII asset header.
struct XII_MUJOCOPLUGIN_DLL xiiMuJoCoModelResourceDescriptor
{
  struct SourceFile
  {
    xiiString                 m_sPath;
    xiiDynamicArray<xiiUInt8> m_Data;
  };

  xiiDynamicArray<xiiUInt8> m_ModelData;
  /// The first file is the root document. Paths are keys in an in-memory filesystem.
  xiiDynamicArray<SourceFile> m_SourceFiles;

  [[nodiscard]] xiiResult Save(xiiStreamWriter& stream) const;
  [[nodiscard]] xiiResult Load(xiiStreamReader& stream);
  static xiiUInt64        GetCompilerVersion();
};

/// Immutable compiled model storage shared with independent, per-world mjData instances.
/// Accepts transformed XII model assets and, for compatibility, loose native MJB files.
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

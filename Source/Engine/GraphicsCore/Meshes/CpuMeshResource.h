#pragma once

#include <GraphicsCore/Meshes/MeshResourceDescriptor.h>

class XII_GRAPHICSCORE_DLL xiiCpuMeshResource : public xiiResource
{
  XII_ADD_DYNAMIC_REFLECTION(xiiCpuMeshResource, xiiResource);
  XII_RESOURCE_DECLARE_COMMON_CODE(xiiCpuMeshResource);
  XII_RESOURCE_DECLARE_CREATEABLE(xiiCpuMeshResource, xiiMeshResourceDescriptor);

public:
  xiiCpuMeshResource();

  const xiiMeshResourceDescriptor& GetDescriptor() const { return m_Descriptor; }

private:
  virtual xiiResourceLoadDesc UnloadData(Unload WhatToUnload) override;
  virtual xiiResourceLoadDesc UpdateContent(xiiStreamReader* Stream) override;
  virtual void                UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;

  xiiMeshResourceDescriptor m_Descriptor;
};

using xiiCpuMeshResourceHandle = xiiTypedResourceHandle<class xiiCpuMeshResource>;

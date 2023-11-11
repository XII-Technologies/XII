#pragma once

#include <Core/Graphics/AmbientCubeBasis.h>
#include <Core/ResourceManager/Resource.h>
#include <GraphicsCore/BakedProbes/BakingUtils.h>

using xiiProbeTreeSectorResourceHandle = xiiTypedResourceHandle<class xiiProbeTreeSectorResource>;

struct XII_RENDERERCORE_DLL xiiProbeTreeSectorResourceDescriptor
{
  XII_DISALLOW_COPY_AND_ASSIGN(xiiProbeTreeSectorResourceDescriptor);

  xiiProbeTreeSectorResourceDescriptor();
  ~xiiProbeTreeSectorResourceDescriptor();
  xiiProbeTreeSectorResourceDescriptor& operator=(xiiProbeTreeSectorResourceDescriptor&& other);

  xiiVec3    m_vGridOrigin;
  xiiVec3    m_vProbeSpacing;
  xiiVec3U32 m_vProbeCount;

  xiiDynamicArray<xiiVec3>                    m_ProbePositions;
  xiiDynamicArray<xiiCompressedSkyVisibility> m_SkyVisibility;

  void      Clear();
  xiiUInt64 GetHeapMemoryUsage() const;

  xiiResult Serialize(xiiStreamWriter& inout_stream) const;
  xiiResult Deserialize(xiiStreamReader& inout_stream);
};

class XII_RENDERERCORE_DLL xiiProbeTreeSectorResource : public xiiResource
{
  XII_ADD_DYNAMIC_REFLECTION(xiiProbeTreeSectorResource, xiiResource);
  XII_RESOURCE_DECLARE_COMMON_CODE(xiiProbeTreeSectorResource);
  XII_RESOURCE_DECLARE_CREATEABLE(xiiProbeTreeSectorResource, xiiProbeTreeSectorResourceDescriptor);

public:
  xiiProbeTreeSectorResource();
  ~xiiProbeTreeSectorResource();

  const xiiVec3&    GetGridOrigin() const { return m_Desc.m_vGridOrigin; }
  const xiiVec3&    GetProbeSpacing() const { return m_Desc.m_vProbeSpacing; }
  const xiiVec3U32& GetProbeCount() const { return m_Desc.m_vProbeCount; }

  xiiArrayPtr<const xiiVec3>                    GetProbePositions() const { return m_Desc.m_ProbePositions; }
  xiiArrayPtr<const xiiCompressedSkyVisibility> GetSkyVisibility() const { return m_Desc.m_SkyVisibility; }

private:
  virtual xiiResourceLoadDesc UnloadData(Unload WhatToUnload) override;
  virtual xiiResourceLoadDesc UpdateContent(xiiStreamReader* Stream) override;
  virtual void                UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;

private:
  xiiProbeTreeSectorResourceDescriptor m_Desc;
};

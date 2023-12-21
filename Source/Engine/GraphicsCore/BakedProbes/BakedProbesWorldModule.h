#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>

#include <Core/Graphics/AmbientCubeBasis.h>
#include <Core/World/WorldModule.h>
#include <GraphicsCore/Declarations.h>

using xiiProbeTreeSectorResourceHandle = xiiTypedResourceHandle<class xiiProbeTreeSectorResource>;

class XII_GRAPHICSCORE_DLL xiiBakedProbesWorldModule : public xiiWorldModule
{
  XII_DECLARE_WORLD_MODULE();
  XII_ADD_DYNAMIC_REFLECTION(xiiBakedProbesWorldModule, xiiWorldModule);

public:
  xiiBakedProbesWorldModule(xiiWorld* pWorld);
  ~xiiBakedProbesWorldModule();

  virtual void Initialize() override;
  virtual void Deinitialize() override;

  bool HasProbeData() const;

  struct ProbeIndexData
  {
    static constexpr xiiUInt32 NumProbes = 8;
    xiiUInt32                  m_probeIndices[NumProbes];
    float                      m_probeWeights[NumProbes];
  };

  xiiResult GetProbeIndexData(const xiiVec3& vGlobalPosition, const xiiVec3& vNormal, ProbeIndexData& out_probeIndexData) const;

  xiiAmbientCube<float> GetSkyVisibility(const ProbeIndexData& indexData) const;

private:
  friend class xiiBakedProbesComponent;

  void SetProbeTreeResourcePrefix(const xiiHashedString& prefix);

  xiiProbeTreeSectorResourceHandle m_hProbeTree;
};

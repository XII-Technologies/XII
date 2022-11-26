#pragma once

#include <BakingPlugin/BakingPluginDLL.h>

class xiiBakingScene;

class XII_BAKINGPLUGIN_DLL xiiTracerInterface
{
public:
  virtual xiiResult BuildScene(const xiiBakingScene& scene) = 0;

  struct Ray
  {
    XII_DECLARE_POD_TYPE();

    xiiVec3 m_vStartPos;
    xiiVec3 m_vDir;
    float   m_fDistance;
  };

  struct Hit
  {
    XII_DECLARE_POD_TYPE();

    xiiVec3 m_vPosition;
    xiiVec3 m_vNormal;
    float   m_fDistance;
  };

  virtual void TraceRays(xiiArrayPtr<const Ray> rays, xiiArrayPtr<Hit> hits) = 0;
};

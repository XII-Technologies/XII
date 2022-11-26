#pragma once

#include <BakingPlugin/Tracer/TracerInterface.h>
#include <Foundation/Types/UniquePtr.h>

class XII_BAKINGPLUGIN_DLL xiiTracerEmbree : public xiiTracerInterface
{
public:
  xiiTracerEmbree();
  ~xiiTracerEmbree();

  virtual xiiResult BuildScene(const xiiBakingScene& scene) override;

  virtual void TraceRays(xiiArrayPtr<const Ray> rays, xiiArrayPtr<Hit> hits) override;

private:
  struct Data;

  xiiUniquePtr<Data> m_pData;
};

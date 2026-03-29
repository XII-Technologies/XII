#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>

#include <Core/World/WorldModule.h>

class XII_GRAPHICSCORE_DLL xiiRenderWorldModule : public xiiWorldModule
{
  XII_DECLARE_WORLD_MODULE();

  XII_ADD_DYNAMIC_REFLECTION(xiiRenderWorldModule, xiiWorldModule);

public:
  xiiRenderWorldModule(xiiWorld* pWorld);
  virtual ~xiiRenderWorldModule();

  virtual void Initialize() override;
};

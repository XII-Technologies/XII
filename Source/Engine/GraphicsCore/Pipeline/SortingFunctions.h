#pragma once

#include <GraphicsCore/Pipeline/Declarations.h>

class XII_GRAPHICSCORE_DLL xiiRenderSortingFunctions
{
public:
  static xiiUInt64 ByRenderDataThenFrontToBack(const xiiRenderData* pRenderData, const xiiCamera& camera);
  static xiiUInt64 BackToFrontThenByRenderData(const xiiRenderData* pRenderData, const xiiCamera& camera);
};

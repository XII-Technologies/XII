#pragma once

#include <RendererCore/Pipeline/Declarations.h>

class XII_RENDERERCORE_DLL xiiRenderSortingFunctions
{
public:
  static xiiUInt64 ByRenderDataThenFrontToBack(const xiiRenderData* pRenderData, xiiUInt32 uiRenderDataSortingKey, const xiiCamera& camera);
  static xiiUInt64 BackToFrontThenByRenderData(const xiiRenderData* pRenderData, xiiUInt32 uiRenderDataSortingKey, const xiiCamera& camera);
};

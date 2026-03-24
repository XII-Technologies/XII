#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>

#include <Foundation/Containers/DynamicArray.h>

#include <GraphicsCore/../../../Data/Base/Shaders/Common/ObjectConstants.h>

class xiiRenderContext;

struct XII_GRAPHICSCORE_DLL xiiInstanceData
{
  XII_ALWAYS_INLINE xiiArrayPtr<xiiPerInstanceData> GetInstanceData(xiiUInt32 uiCount, xiiUInt32& out_uiInstanceDataOffset)
  {
    m_InstanceData.SetCount(uiCount);
    out_uiInstanceDataOffset = 0;
    return m_InstanceData;
  }

  XII_ALWAYS_INLINE void UpdateInstanceData(xiiSharedPtr<xiiGALCommandList> pCommandList, xiiUInt32 uiCount)
  {
    XII_IGNORE_UNUSED(pCommandList);
    XII_IGNORE_UNUSED(uiCount);
  }

  XII_ALWAYS_INLINE void BindResources(xiiRenderContext* pRenderContext)
  {
    XII_IGNORE_UNUSED(pRenderContext);
  }

  xiiDynamicArray<xiiPerInstanceData> m_InstanceData;
};

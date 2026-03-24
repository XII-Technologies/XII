#pragma once

#include <GraphicsCore/Pipeline/RenderData.h>

struct XII_GRAPHICSCORE_DLL xiiMsgExtractOccluderData : public xiiMessage
{
  XII_DECLARE_MESSAGE_TYPE(xiiMsgExtractOccluderData, xiiMessage);

  void AddOccluder(const xiiRasterizerObject* pObject, const xiiTransform& transform)
  {
    auto& d       = m_ExtractedOccluderData.ExpandAndGetRef();
    d.m_pObject   = pObject;
    d.m_Transform = transform;
  }

private:
  friend class xiiRenderPipeline;

  struct Data
  {
    const xiiRasterizerObject* m_pObject = nullptr;
    xiiTransform               m_Transform;
  };

  xiiHybridArray<Data, 16> m_ExtractedOccluderData;
};

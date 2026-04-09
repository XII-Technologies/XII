#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>
#include <GraphicsCore/Pipeline/ExtractedRenderData.h>

#include <Foundation/Communication/Message.h>

class xiiView;

/// \brief Sent to components or component managers to request them to extract their render data into the given structure.
struct XII_GRAPHICSCORE_DLL xiiMsgExtractRenderData : public xiiMessage
{
  XII_DECLARE_MESSAGE_TYPE(xiiMsgExtractRenderData, xiiMessage);

  const xiiView*          m_pView                = nullptr;
  xiiExtractedRenderData* m_pExtractedRenderData = nullptr;
};

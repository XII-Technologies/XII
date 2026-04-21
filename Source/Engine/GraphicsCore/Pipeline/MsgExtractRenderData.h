#pragma once

#include <Foundation/Communication/Message.h>
#include <GraphicsCore/GraphicsCoreDLL.h>
#include <GraphicsCore/Pipeline/Declarations.h>
#include <GraphicsCore/Pipeline/ExtractedRenderData.h>

class xiiView;

/// \brief Sent to components or component managers to request them to extract their render data into the given structure.
struct XII_GRAPHICSCORE_DLL xiiMsgExtractRenderData : public xiiMessage
{
  XII_DECLARE_MESSAGE_TYPE(xiiMsgExtractRenderData, xiiMessage);

public:
  using SubmitRenderDataFunction = void (*)(void* pContext, const xiiMsgExtractRenderData& msg, xiiRenderData* pRenderData, xiiRenderData::Caching::Enum caching);

  /// \brief Adds a single extracted render data item.
  void AddRenderData(xiiRenderData* pRenderData, xiiRenderData::Caching::Enum caching = xiiRenderData::Caching::Never);

  /// \brief Pushes a batch of extracted data safely to the internal list.
  void AddRenderDataBatch(const xiiRenderDataBatch& batch, xiiRenderData::Caching::Enum caching = xiiRenderData::Caching::Never);

public:
  const xiiView*          m_pView                = nullptr;
  xiiExtractedRenderData* m_pExtractedRenderData = nullptr;

  SubmitRenderDataFunction m_SubmitRenderDataFunction = nullptr;
  void*                    m_pSubmitRenderDataContext = nullptr;

  xiiGameObjectHandle m_hCurrentObject;
  xiiComponentHandle  m_hCurrentComponent;
};

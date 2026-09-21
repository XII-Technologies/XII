/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>

#include <Foundation/Communication/Message.h>
#include <GraphicsCore/Pipeline/Declarations.h>
#include <GraphicsCore/Pipeline/ExtractedRenderData.h>

class xiiView;

/// Sent to components or component managers to request them to extract their render data into the given structure.
struct XII_GRAPHICSCORE_DLL xiiMsgExtractRenderData : public xiiMessage
{
  XII_DECLARE_MESSAGE_TYPE(xiiMsgExtractRenderData, xiiMessage);

public:
  using SubmitRenderDataFunction = void (*)(void* pContext, const xiiMsgExtractRenderData& msg, xiiRenderData* pRenderData, xiiRenderData::Caching::Enum caching);

  /// Adds a single extracted render data item.
  void AddRenderData(xiiRenderData* pRenderData, xiiRenderData::Caching::Enum caching = xiiRenderData::Caching::Never);

  /// Pushes a batch of extracted data safely to the internal list.
  void AddRenderDataBatch(const xiiRenderDataBatch& batch, xiiRenderData::Caching::Enum caching = xiiRenderData::Caching::Never);

public:
  const xiiView*          m_pView                = nullptr; ///< The view for which the render data should be extracted. Components can use this to decide what data to extract based on view properties (e.g. camera usage hint).
  xiiExtractedRenderData* m_pExtractedRenderData = nullptr; ///< The structure to which extracted render data should be submitted. Components fill this by calling AddRenderData() and AddRenderDataBatch().

  SubmitRenderDataFunction m_SubmitRenderDataFunction = nullptr; ///< The function pointer to call for submitting render data. This is used internally by xiiRenderWorldModule to route submissions from components to the correct view's extracted data cache, and should not be called directly by components. Components should call AddRenderData() and AddRenderDataBatch() which will forward to this function pointer with the correct context.
  void*                    m_pSubmitRenderDataContext = nullptr; ///< The context pointer to pass when calling m_SubmitRenderDataFunction. This is used internally by xiiRenderWorldModule to route submissions from components to the correct view's extracted data cache, and should not be used directly by components.

  xiiGameObjectHandle m_hCurrentObject;    ///< The game object for which render data is currently being extracted. This is set by xiiRenderWorldModule before sending the message to components, so that components can use this to decide what data to extract based on object properties.
  xiiComponentHandle  m_hCurrentComponent; ///< The component for which render data is currently being extracted. This is set by xiiRenderWorldModule before sending the message to components, so that components can use this to decide what data to extract based on component properties.
};

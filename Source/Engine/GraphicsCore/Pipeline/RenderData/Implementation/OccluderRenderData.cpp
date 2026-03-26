#include <GraphicsCore/GraphicsCorePCH.h>

#include <GraphicsCore/Pipeline/RenderData/OccluderRenderData.h>

// clang-format off
XII_IMPLEMENT_MESSAGE_TYPE(xiiMsgExtractOccluderData);

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiMsgExtractOccluderData, 1, xiiRTTIDefaultAllocator<xiiMsgExtractOccluderData>)
{
  XII_BEGIN_ATTRIBUTES
  {
    new xiiExcludeFromScript()
  }
  XII_END_ATTRIBUTES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

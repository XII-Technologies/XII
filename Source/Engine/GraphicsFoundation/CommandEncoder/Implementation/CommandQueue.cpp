#include <GraphicsFoundation/GraphicsFoundationPCH.h>

#include <GraphicsFoundation/CommandEncoder/CommandQueue.h>

// clang-format off

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiGALCommandQueue, xiiNoBase, 1, xiiRTTINoAllocator)
{
}
XII_END_STATIC_REFLECTED_TYPE;

// clang-format on

xiiGALCommandQueue::xiiGALCommandQueue(const xiiGALCommandQueueCreationDescription& creationDescription) :
  xiiGALObject<xiiGALCommandQueueCreationDescription>(creationDescription)
{
}

xiiGALCommandQueue::~xiiGALCommandQueue() = default;

XII_STATICLINK_FILE(GraphicsFoundation, GraphicsFoundation_CommandEncoder_Implementation_CommandQueue);

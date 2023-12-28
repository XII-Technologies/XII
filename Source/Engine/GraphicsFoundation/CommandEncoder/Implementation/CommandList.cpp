#include <GraphicsFoundation/GraphicsFoundationPCH.h>

#include <GraphicsFoundation/CommandEncoder/CommandList.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiGALCommandList, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiGALCommandList::xiiGALCommandList() :
  xiiGALDeviceObject()
{
}

xiiGALCommandList::~xiiGALCommandList() = default;

XII_STATICLINK_FILE(GraphicsFoundation, GraphicsFoundation_CommandEncoder_Implementation_CommandList);

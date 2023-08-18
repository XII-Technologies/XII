#include <GraphicsFoundation/GraphicsFoundationPCH.h>

#include <GraphicsFoundation/CommandEncoder/CommandList.h>

// clang-format off

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiGALCommandList, xiiNoBase, 1, xiiRTTINoAllocator)
{
}
XII_END_STATIC_REFLECTED_TYPE;

// clang-format on

xiiGALCommandList::xiiGALCommandList(const xiiGALCommandListCreationDescription& creationDescription) :
  xiiGALObject<xiiGALCommandListCreationDescription>(creationDescription)
{
}

xiiGALCommandList::~xiiGALCommandList() = default;

XII_STATICLINK_FILE(GraphicsFoundation, GraphicsFoundation_CommandEncoder_Implementation_CommandList);

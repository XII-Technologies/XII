#include <GraphicsFoundation/GraphicsFoundationPCH.h>

#include <GraphicsFoundation/CommandEncoder/CommandList.h>

xiiGALCommandList::xiiGALCommandList(const xiiGALCommandListCreationDescription& creationDescription) :
  xiiGALResource<xiiGALCommandListCreationDescription>(creationDescription)
{
}

xiiGALCommandList::~xiiGALCommandList() = default;

XII_STATICLINK_FILE(GraphicsFoundation, GraphicsFoundation_CommandEncoder_Implementation_CommandList);

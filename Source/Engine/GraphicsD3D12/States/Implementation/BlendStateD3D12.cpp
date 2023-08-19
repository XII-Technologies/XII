#include <GraphicsD3D12/GraphicsD3D12PCH.h>

#include <GraphicsD3D12/States/BlendStateD3D12.h>

xiiGALBlendStateD3D12::xiiGALBlendStateD3D12(const xiiGALBlendStateCreationDescription& creationDescription) :
  xiiGALBlendState(creationDescription)
{
}

xiiGALBlendStateD3D12::~xiiGALBlendStateD3D12() = default;

XII_STATICLINK_FILE(GraphicsD3D12, GraphicsD3D12_States_Implementation_BlendStateD3D12);

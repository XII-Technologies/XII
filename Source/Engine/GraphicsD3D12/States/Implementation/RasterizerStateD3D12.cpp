#include <GraphicsD3D12/GraphicsD3D12PCH.h>

#include <GraphicsD3D12/States/RasterizerStateD3D12.h>

xiiGALRasterizerStateD3D12::xiiGALRasterizerStateD3D12(const xiiGALRasterizerStateCreationDescription& creationDescription) :
  xiiGALRasterizerState(creationDescription)
{
}

xiiGALRasterizerStateD3D12::~xiiGALRasterizerStateD3D12() = default;

XII_STATICLINK_FILE(GraphicsD3D12, GraphicsD3D12_States_Implementation_RasterizerStateD3D12);

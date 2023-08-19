#include <GraphicsD3D12/GraphicsD3D12PCH.h>

#include <GraphicsD3D12/States/DepthStencilStateD3D12.h>

xiiGALDepthStencilStateD3D12::xiiGALDepthStencilStateD3D12(const xiiGALDepthStencilStateCreationDescription& creationDescription) :
  xiiGALDepthStencilState(creationDescription)
{
}

xiiGALDepthStencilStateD3D12::~xiiGALDepthStencilStateD3D12() = default;

XII_STATICLINK_FILE(GraphicsD3D12, GraphicsD3D12_States_Implementation_DepthStencilStateD3D12);

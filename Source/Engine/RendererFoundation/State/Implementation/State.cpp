#include <RendererFoundation/RendererFoundationPCH.h>

#include <RendererFoundation/RendererFoundationDLL.h>
#include <RendererFoundation/State/State.h>

xiiGALBlendState::xiiGALBlendState(const xiiGALBlendStateCreationDescription& Description) :
  xiiGALObject(Description)
{
}

xiiGALBlendState::~xiiGALBlendState() {}



xiiGALDepthStencilState::xiiGALDepthStencilState(const xiiGALDepthStencilStateCreationDescription& Description) :
  xiiGALObject(Description)
{
}

xiiGALDepthStencilState::~xiiGALDepthStencilState() {}



xiiGALRasterizerState::xiiGALRasterizerState(const xiiGALRasterizerStateCreationDescription& Description) :
  xiiGALObject(Description)
{
}

xiiGALRasterizerState::~xiiGALRasterizerState() {}


xiiGALSamplerState::xiiGALSamplerState(const xiiGALSamplerStateCreationDescription& Description) :
  xiiGALObject(Description)
{
}

xiiGALSamplerState::~xiiGALSamplerState() {}



XII_STATICLINK_FILE(RendererFoundation, RendererFoundation_State_Implementation_State);

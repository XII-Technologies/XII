#include <RendererFoundation/RendererFoundationPCH.h>

#include <RendererFoundation/RendererFoundationDLL.h>
#include <RendererFoundation/State/State.h>

//////////////////////////////////////////////////////////////////////////

xiiGALBlendState::xiiGALBlendState(const xiiGALBlendStateCreationDescription& Description) :
  xiiGALObject(Description)
{
}

xiiGALBlendState::~xiiGALBlendState() = default;

//////////////////////////////////////////////////////////////////////////

xiiGALDepthStencilState::xiiGALDepthStencilState(const xiiGALDepthStencilStateCreationDescription& Description) :
  xiiGALObject(Description)
{
}

xiiGALDepthStencilState::~xiiGALDepthStencilState() = default;

//////////////////////////////////////////////////////////////////////////

xiiGALRasterizerState::xiiGALRasterizerState(const xiiGALRasterizerStateCreationDescription& Description) :
  xiiGALObject(Description)
{
}

xiiGALRasterizerState::~xiiGALRasterizerState() = default;

//////////////////////////////////////////////////////////////////////////

xiiGALSamplerState::xiiGALSamplerState(const xiiGALSamplerStateCreationDescription& Description) :
  xiiGALObject(Description)
{
}

xiiGALSamplerState::~xiiGALSamplerState() = default;

//////////////////////////////////////////////////////////////////////////


XII_STATICLINK_FILE(RendererFoundation, RendererFoundation_State_Implementation_State);

#include <GraphicsVulkan/GraphicsVulkanPCH.h>

#include <GraphicsVulkan/States/RasterizerStateVulkan.h>

#include <GraphicsVulkan/Utilities/VulkanTypeConversions.h>

xiiGALRasterizerStateVulkan::xiiGALRasterizerStateVulkan(const xiiGALRasterizerStateCreationDescription& creationDescription) :
  xiiGALRasterizerState(creationDescription)
{
}

xiiGALRasterizerStateVulkan::~xiiGALRasterizerStateVulkan() = default;

xiiResult xiiGALRasterizerStateVulkan::InitPlatform(xiiGALDevice* pDevice)
{
  m_RasterizerState.FillMode              = xiiDiligentTypeConversions::GetFillMode(m_Description.m_FillMode);
  m_RasterizerState.CullMode              = xiiDiligentTypeConversions::GetCullMode(m_Description.m_CullMode);
  m_RasterizerState.FrontCounterClockwise = m_Description.m_bFrontCounterClockwise;
  m_RasterizerState.DepthBias             = m_Description.m_iDepthBias;
  m_RasterizerState.DepthBiasClamp        = m_Description.m_fDepthBiasClamp;
  m_RasterizerState.SlopeScaledDepthBias  = m_Description.m_fSlopeScaledDepthBias;
  m_RasterizerState.DepthClipEnable       = m_Description.m_bDepthClipEnable;
  m_RasterizerState.AntialiasedLineEnable = m_Description.m_bAntialiasedLineEnable;

  m_RasterizerState.ScissorEnable = m_Description.m_bScissorEnable;

  return XII_SUCCESS;
}

xiiResult xiiGALRasterizerStateVulkan::DeInitPlatform(xiiGALDevice* pDevice)
{
  return XII_SUCCESS;
}

XII_STATICLINK_FILE(GraphicsVulkan, GraphicsVulkan_States_Implementation_RasterizerStateVulkan);

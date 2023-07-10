
#pragma once

#include <Foundation/Math/Color.h>
#include <Foundation/Math/Rect.h>
#include <RendererFoundation/RendererFoundationDLL.h>

struct XII_RENDERERFOUNDATION_DLL xiiGALCommandEncoderState
{
  virtual void InvalidateState();

  xiiGALShaderHandle m_hShader;

  xiiGALBufferHandle m_hConstantBuffers[XII_GAL_MAX_CONSTANT_BUFFER_COUNT];

  xiiHybridArray<xiiGALResourceViewHandle, 16>  m_hResourceViews[xiiGALShaderStage::ENUM_COUNT];
  xiiHybridArray<const xiiGALResourceBase*, 16> m_pResourcesForResourceViews[xiiGALShaderStage::ENUM_COUNT];

  xiiHybridArray<xiiGALUnorderedAccessViewHandle, 16> m_hUnorderedAccessViews;
  xiiHybridArray<const xiiGALResourceBase*, 16>       m_pResourcesForUnorderedAccessViews;

  xiiGALSamplerStateHandle m_hSamplerStates[xiiGALShaderStage::ENUM_COUNT][XII_GAL_MAX_SAMPLER_COUNT];
};

struct XII_RENDERERFOUNDATION_DLL xiiGALCommandEncoderRenderState : public xiiGALCommandEncoderState
{
  virtual ~xiiGALCommandEncoderRenderState() = default;

  virtual void InvalidateState() override;

  xiiGALBufferHandle m_hVertexBuffers[XII_GAL_MAX_VERTEX_BUFFER_COUNT];
  xiiGALBufferHandle m_hIndexBuffer;

  xiiGALVertexDeclarationHandle m_hVertexDeclaration;
  xiiGALPrimitiveTopology::Enum m_Topology = xiiGALPrimitiveTopology::ENUM_COUNT;

  xiiGALBlendStateHandle m_hBlendState;
  xiiColor               m_BlendFactor  = xiiColor(0, 0, 0, 0);
  xiiUInt32              m_uiSampleMask = 0;

  xiiGALDepthStencilStateHandle m_hDepthStencilState;
  xiiUInt8                      m_uiStencilRefValue = 0;

  xiiGALRasterizerStateHandle m_hRasterizerState;

  xiiRectU32   m_ScissorRect       = xiiRectU32(0xFFFFFFFF, 0xFFFFFFFF, 0, 0);
  xiiRectFloat m_ViewPortRect      = xiiRectFloat(xiiMath::MaxValue<float>(), xiiMath::MaxValue<float>(), 0.0f, 0.0f);
  float        m_fViewPortMinDepth = xiiMath::MaxValue<float>();
  float        m_fViewPortMaxDepth = -xiiMath::MaxValue<float>();
};

#pragma once

#include <GraphicsFoundation/GraphicsFoundationDLL.h>

#include <Foundation/Math/Rect.h>
#include <GraphicsFoundation/Resources/Resource.h>

/// \brief This describes the command encoder state.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALCommandEncoderState
{
  virtual void InvalidateState();

  xiiGALShaderHandle m_hShader;

  xiiGALBufferHandle m_hConstantBuffers[XII_GAL_MAX_CONSTANT_BUFFER_COUNT];

  xiiHybridArray<xiiGALBufferViewHandle, 16U>  m_hBufferViews[xiiGALShaderStage::ENUM_COUNT];
  xiiHybridArray<xiiGALTextureViewHandle, 16U> m_hTextureViews[xiiGALShaderStage::ENUM_COUNT];

  xiiHybridArray<const xiiGALResourceBase*, 16U> m_pResourcesForBufferViews[xiiGALShaderStage::ENUM_COUNT];
  xiiHybridArray<const xiiGALResourceBase*, 16U> m_pResourcesForTextureViews[xiiGALShaderStage::ENUM_COUNT];

  xiiGALSamplerHandle m_hSamplerStates[xiiGALShaderStage::ENUM_COUNT][XII_GAL_MAX_SAMPLER_COUNT];
};

struct XII_GRAPHICSFOUNDATION_DLL xiiGALCommandEncoderGraphicsState : public xiiGALCommandEncoderState
{
  virtual ~xiiGALCommandEncoderGraphicsState() = default;

  virtual void InvalidateState() override;

  xiiGALBufferHandle m_hVertexBuffers[XII_GAL_MAX_VERTEX_BUFFER_COUNT];
  xiiGALBufferHandle m_hIndexBuffer;

  xiiGALInputLayoutHandle          m_hInputLayout;
  xiiEnum<xiiGALPrimitiveTopology> m_Topology = xiiGALPrimitiveTopology::Undefined;

  xiiGALBlendStateHandle m_hBlendState;
  xiiColor               m_BlendFactor  = xiiColor::Black;
  xiiUInt32              m_uiSampleMask = 0U;

  xiiGALDepthStencilStateHandle m_hDepthStencilState;
  xiiUInt8                      m_uiStencilRefValue = 0U;

  xiiGALRasterizerStateHandle m_hRasterizerState;

  xiiRectU32   m_ScissorRect       = xiiRectU32(0xFFFFFFFFU, 0xFFFFFFFFU, 0U, 0U);
  xiiRectFloat m_ViewPortRect      = xiiRectFloat(xiiMath::MaxValue<float>(), xiiMath::MaxValue<float>(), 0.0f, 0.0f);
  float        m_fViewPortMinDepth = xiiMath::MaxValue<float>();
  float        m_fViewPortMaxDepth = -xiiMath::MaxValue<float>();
};

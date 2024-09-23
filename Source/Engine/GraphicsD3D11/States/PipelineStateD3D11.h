#pragma once

#include <GraphicsD3D11/GraphicsD3D11DLL.h>

#include <GraphicsFoundation/States/PipelineState.h>

#include <GraphicsD3D11/Resources/RenderPassD3D11.h>
#include <GraphicsD3D11/Shader/InputLayoutD3D11.h>
#include <GraphicsD3D11/Shader/ShaderD3D11.h>
#include <GraphicsD3D11/States/BlendStateD3D11.h>
#include <GraphicsD3D11/States/DepthStencilStateD3D11.h>
#include <GraphicsD3D11/States/PipelineResourceSignatureD3D11.h>
#include <GraphicsD3D11/States/RasterizerStateD3D11.h>

struct ID3D11VertexShader;
struct ID3D11PixelShader;
struct ID3D11GeometryShader;
struct ID3D11DomainShader;
struct ID3D11HullShader;
struct ID3D11ComputeShader;

class XII_GRAPHICSD3D11_DLL xiiGALPipelineStateD3D11 final : public xiiGALPipelineState
{
  XII_ADD_DYNAMIC_REFLECTION(xiiGALPipelineStateD3D11, xiiGALPipelineState);

public:
  struct ShaderType
  {
    using StorageType = xiiInt8;

    enum Enum : StorageType
    {
      Unknown = -1,
      Vertex,
      Pixel,
      Compute,
      Domain,
      Hull,
      Geometry,

      ENUM_COUNT
    };

    static ShaderType::Enum GetIndex(xiiBitflags<xiiGALShaderType> type);
  };

  XII_ALWAYS_INLINE ID3D11BlendState* GetD3D11BlendState() const { return m_pBlendStateD3D11 ? m_pBlendStateD3D11->GetBlendState() : nullptr; };
  XII_ALWAYS_INLINE ID3D11RasterizerState* GetD3D11RasterizerState() const { return m_pRasterizerStateD3D11 ? m_pRasterizerStateD3D11->GetRasterizerState() : nullptr; };
  XII_ALWAYS_INLINE ID3D11DepthStencilState* GetD3D11DepthStencilState() const { return m_pDepthStencilStateD3D11 ? m_pDepthStencilStateD3D11->GetDepthStencilState() : nullptr; };
  XII_ALWAYS_INLINE ID3D11InputLayout* GetD3D11InputLayout() const { return m_pInputLayoutD3D11 ? m_pInputLayoutD3D11->GetInputLayout() : nullptr; };
  XII_ALWAYS_INLINE ID3D11VertexShader* GetD3D11VertexShader() const { return m_pVertexShaderD3D11 != nullptr ? static_cast<ID3D11VertexShader*>(m_pVertexShaderD3D11->GetD3D11Shader()) : nullptr; };
  XII_ALWAYS_INLINE ID3D11PixelShader* GetD3D11PixelShader() const { return m_pPixelShaderD3D11 != nullptr ? static_cast<ID3D11PixelShader*>(m_pPixelShaderD3D11->GetD3D11Shader()) : nullptr; };
  XII_ALWAYS_INLINE ID3D11GeometryShader* GetD3D11GeometryShader() const { return m_pGeometryShaderD3D11 != nullptr ? static_cast<ID3D11GeometryShader*>(m_pGeometryShaderD3D11->GetD3D11Shader()) : nullptr; };
  XII_ALWAYS_INLINE ID3D11DomainShader* GetD3D11DomainShader() const { return m_pDomainShaderD3D11 != nullptr ? static_cast<ID3D11DomainShader*>(m_pDomainShaderD3D11->GetD3D11Shader()) : nullptr; };
  XII_ALWAYS_INLINE ID3D11HullShader* GetD3D11HullShader() const { return m_pHullShaderD3D11 != nullptr ? static_cast<ID3D11HullShader*>(m_pHullShaderD3D11->GetD3D11Shader()) : nullptr; };
  XII_ALWAYS_INLINE ID3D11ComputeShader* GetD3D11ComputeShader() const { return m_pComputeShaderD3D11 != nullptr ? static_cast<ID3D11ComputeShader*>(m_pComputeShaderD3D11->GetD3D11Shader()) : nullptr; };

  xiiResult CommitShaderResources(xiiGALCommandListD3D11* pCommandListD3D11);

  bool UnsetResourceViews(const xiiGALResource* pResource);
  bool UnsetUnorderedAccessViews(const xiiGALResource* pResource);

protected:
  friend class xiiGALDeviceD3D11;
  friend class xiiMemoryUtils;

  xiiGALPipelineStateD3D11(xiiGALDeviceD3D11* pDeviceD3D11, const xiiGALPipelineStateCreationDescription& creationDescription);

  virtual ~xiiGALPipelineStateD3D11();

  virtual xiiResult InitPlatform() override final;

  virtual xiiResult DeInitPlatform() override final;

  virtual void SetConstantBufferPlatform(const xiiGALPipelineResourceDescription& bindingInformation, xiiGALBuffer* pConstantBuffer) override final;
  virtual void SetShaderResourceBufferViewPlatform(const xiiGALPipelineResourceDescription& bindingInformation, xiiGALBufferView* pBufferView) override final;
  virtual void SetShaderResourceTextureViewPlatform(const xiiGALPipelineResourceDescription& bindingInformation, xiiGALTextureView* pTextureView) override final;
  virtual void SetUnorderedAccessBufferViewPlatform(const xiiGALPipelineResourceDescription& bindingInformation, xiiGALBufferView* pBufferView) override final;
  virtual void SetUnorderedAccessTextureViewPlatform(const xiiGALPipelineResourceDescription& bindingInformation, xiiGALTextureView* pTextureView) override final;
  virtual void SetSamplerPlatform(const xiiGALPipelineResourceDescription& bindingInformation, xiiGALSampler* pSampler) override final;
  virtual void ResetBoundResources() override final;

protected:
  xiiGALShaderD3D11*            m_pVertexShaderD3D11      = nullptr;
  xiiGALShaderD3D11*            m_pPixelShaderD3D11       = nullptr;
  xiiGALShaderD3D11*            m_pDomainShaderD3D11      = nullptr;
  xiiGALShaderD3D11*            m_pHullShaderD3D11        = nullptr;
  xiiGALShaderD3D11*            m_pGeometryShaderD3D11    = nullptr;
  xiiGALShaderD3D11*            m_pComputeShaderD3D11     = nullptr;
  xiiGALBlendStateD3D11*        m_pBlendStateD3D11        = nullptr;
  xiiGALInputLayoutD3D11*       m_pInputLayoutD3D11       = nullptr;
  xiiGALRasterizerStateD3D11*   m_pRasterizerStateD3D11   = nullptr;
  xiiGALDepthStencilStateD3D11* m_pDepthStencilStateD3D11 = nullptr;

  xiiGALRenderPassD3D11*                m_pRenderPassD3D11                = nullptr;
  xiiGALPipelineResourceSignatureD3D11* m_pPipelineResourceSignatureD3D11 = nullptr;

  ID3D11Buffer*         m_pBoundConstantBuffers[ShaderType::ENUM_COUNT][XII_GAL_MAX_CONSTANT_BUFFER_COUNT] = {};
  xiiGAL::ModifiedRange m_BoundConstantBuffersRange[ShaderType::ENUM_COUNT];

  xiiHybridArray<ID3D11ShaderResourceView*, 16> m_pBoundShaderResourceViews[ShaderType::ENUM_COUNT] = {};
  xiiHybridArray<xiiGALResource*, 16>           m_ResourcesForResourceViews[ShaderType::ENUM_COUNT];
  xiiGAL::ModifiedRange                         m_BoundShaderResourceViewsRange[ShaderType::ENUM_COUNT];

  xiiHybridArray<ID3D11UnorderedAccessView*, 16> m_BoundUnoderedAccessViews;
  xiiHybridArray<xiiGALResource*, 16>            m_ResourcesForUnorderedAccessViews;
  xiiGAL::ModifiedRange                          m_BoundUnoderedAccessViewsRange;

  ID3D11SamplerState*   m_pBoundSamplerStates[ShaderType::ENUM_COUNT][XII_GAL_MAX_SAMPLER_COUNT] = {};
  xiiGAL::ModifiedRange m_BoundSamplerStatesRange[ShaderType::ENUM_COUNT];
};

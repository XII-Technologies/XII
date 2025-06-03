#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>

#include <Core/ResourceManager/Resource.h>
#include <Core/ResourceManager/ResourceTypeLoader.h>
#include <Foundation/Time/Timestamp.h>
#include <GraphicsFoundation/ShaderCompiler/PermutationGenerator.h>
#include <GraphicsFoundation/ShaderCompiler/ShaderPermutationBinary.h>

using xiiShaderPermutationResourceHandle = xiiTypedResourceHandle<class xiiShaderPermutationResource>;
using xiiShaderStateResourceHandle       = xiiTypedResourceHandle<class xiiShaderStateResource>;

struct xiiShaderPermutationResourceDescriptor
{
};

class XII_GRAPHICSCORE_DLL xiiShaderPermutationResource : public xiiResource
{
  XII_ADD_DYNAMIC_REFLECTION(xiiShaderPermutationResource, xiiResource);
  XII_RESOURCE_DECLARE_COMMON_CODE(xiiShaderPermutationResource);
  XII_RESOURCE_DECLARE_CREATEABLE(xiiShaderPermutationResource, xiiShaderPermutationResourceDescriptor);

public:
  xiiShaderPermutationResource();

  xiiBitflags<xiiGALShaderType> GetActiveShaderStages() const { return m_ActiveShaderStages; };
  xiiSharedPtr<xiiGALShader>    GetGALShader(xiiGALShaderType::Enum type) const { return m_ShaderData.GetValueOrDefault(type, ShaderData()).m_pShader; }
  const xiiGALShaderByteCode*   GetShaderByteCode(xiiGALShaderType::Enum type) const { return m_ShaderData.GetValueOrDefault(type, ShaderData()).m_pByteCode; }

  xiiSharedPtr<xiiGALPipelineResourceSignature> GetPipelineResourceSignature() const { return m_pPipelineResourceSignature; }

  xiiSharedPtr<xiiGALBlendState>        GetBlendState() const { return m_pBlendState; }
  xiiSharedPtr<xiiGALDepthStencilState> GetDepthStencilState() const { return m_pDepthStencilState; }
  xiiSharedPtr<xiiGALRasterizerState>   GetRasterizerState() const { return m_pRasterizerState; }

  bool IsShaderValid() const { return m_bShaderPermutationValid; }

  xiiArrayPtr<const xiiGALPermutationVariable> GetPermutationVars() const { return m_PermutationVariables; }

private:
  virtual xiiResourceLoadDesc    UnloadData(Unload WhatToUnload) override;
  virtual xiiResourceLoadDesc    UpdateContent(xiiStreamReader* pStream) override;
  virtual void                   UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;
  virtual xiiResourceTypeLoader* GetDefaultResourceTypeLoader() const override;

private:
  friend class xiiShaderPermutationUtilities;

  struct ShaderData
  {
    xiiSharedPtr<xiiGALShader>               m_pShader;
    xiiSharedPtr<const xiiGALShaderByteCode> m_pByteCode;
  };

  xiiBitflags<xiiGALShaderType>              m_ActiveShaderStages;
  xiiMap<xiiGALShaderType::Enum, ShaderData> m_ShaderData;

  bool                                          m_bShaderPermutationValid;
  xiiSharedPtr<xiiGALPipelineResourceSignature> m_pPipelineResourceSignature;

  xiiSharedPtr<xiiGALBlendState>        m_pBlendState;
  xiiSharedPtr<xiiGALDepthStencilState> m_pDepthStencilState;
  xiiSharedPtr<xiiGALRasterizerState>   m_pRasterizerState;

  xiiHybridArray<xiiGALPermutationVariable, 16> m_PermutationVariables;
};


class xiiShaderPermutationResourceLoader : public xiiResourceTypeLoader
{
public:
  virtual xiiResourceLoadData OpenDataStream(const xiiResource* pResource) override;
  virtual void                CloseDataStream(const xiiResource* pResource, const xiiResourceLoadData& loaderData) override;

  virtual bool IsResourceOutdated(const xiiResource* pResource) const override;

private:
  xiiResult RunCompiler(const xiiResource* pResource, xiiGALShaderPermutationBinary& BinaryInfo, bool bForce);
};

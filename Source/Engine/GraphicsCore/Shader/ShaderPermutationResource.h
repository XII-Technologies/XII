#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>

#include <Core/ResourceManager/Resource.h>
#include <Core/ResourceManager/ResourceTypeLoader.h>
#include <Foundation/Time/Timestamp.h>
#include <GraphicsCore/Shader/ShaderPermutationBinary.h>
#include <GraphicsCore/ShaderCompiler/PermutationGenerator.h>

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

  xiiGALShaderHandle          GetGALShader() const { return m_hShader; }
  const xiiGALShaderByteCode* GetShaderByteCode(xiiBitflags<xiiGALShaderStage> stage) const { return m_ByteCodes[xiiGALShaderStage::GetStageIndex((xiiGALShaderStage::Enum)stage.GetValue())]; }

  xiiGALPipelineResourceSignatureHandle GetPipelineResourceSignature() const { return m_hPipelineResourceSignature; }

  xiiGALBlendStateHandle        GetBlendState() const { return m_hBlendState; }
  xiiGALDepthStencilStateHandle GetDepthStencilState() const { return m_hDepthStencilState; }
  xiiGALRasterizerStateHandle   GetRasterizerState() const { return m_hRasterizerState; }

  bool IsShaderValid() const { return m_bShaderPermutationValid; }

  xiiArrayPtr<const xiiPermutationVar> GetPermutationVars() const { return m_PermutationVars; }

private:
  virtual xiiResourceLoadDesc    UnloadData(Unload WhatToUnload) override;
  virtual xiiResourceLoadDesc    UpdateContent(xiiStreamReader* Stream) override;
  virtual void                   UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;
  virtual xiiResourceTypeLoader* GetDefaultResourceTypeLoader() const override;

private:
  friend class xiiShaderManager;

  xiiSharedPtr<const xiiGALShaderByteCode> m_ByteCodes[xiiGALShaderStage::ENUM_COUNT];

  bool               m_bShaderPermutationValid;
  xiiGALShaderHandle m_hShader;

  xiiGALPipelineResourceSignatureHandle m_hPipelineResourceSignature;

  xiiGALBlendStateHandle        m_hBlendState;
  xiiGALDepthStencilStateHandle m_hDepthStencilState;
  xiiGALRasterizerStateHandle   m_hRasterizerState;

  xiiHybridArray<xiiPermutationVar, 16> m_PermutationVars;
};


class xiiShaderPermutationResourceLoader : public xiiResourceTypeLoader
{
public:
  virtual xiiResourceLoadData OpenDataStream(const xiiResource* pResource) override;
  virtual void                CloseDataStream(const xiiResource* pResource, const xiiResourceLoadData& loaderData) override;

  virtual bool IsResourceOutdated(const xiiResource* pResource) const override;

private:
  xiiResult RunCompiler(const xiiResource* pResource, xiiShaderPermutationBinary& BinaryInfo, bool bForce);
};

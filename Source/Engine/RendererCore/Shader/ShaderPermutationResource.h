#pragma once

#include <Core/ResourceManager/Resource.h>
#include <Core/ResourceManager/ResourceTypeLoader.h>
#include <Foundation/Time/Timestamp.h>
#include <RendererCore/RendererCoreDLL.h>
#include <RendererCore/Shader/ShaderPermutationBinary.h>
#include <RendererCore/ShaderCompiler/PermutationGenerator.h>

using xiiShaderPermutationResourceHandle = xiiTypedResourceHandle<class xiiShaderPermutationResource>;
using xiiShaderStateResourceHandle       = xiiTypedResourceHandle<class xiiShaderStateResource>;

struct xiiShaderPermutationResourceDescriptor
{
};

class XII_RENDERERCORE_DLL xiiShaderPermutationResource : public xiiResource
{
  XII_ADD_DYNAMIC_REFLECTION(xiiShaderPermutationResource, xiiResource);
  XII_RESOURCE_DECLARE_COMMON_CODE(xiiShaderPermutationResource);
  XII_RESOURCE_DECLARE_CREATEABLE(xiiShaderPermutationResource, xiiShaderPermutationResourceDescriptor);

public:
  xiiShaderPermutationResource();

  xiiGALShaderHandle          GetGALShader() const { return m_hShader; }
  const xiiShaderStageBinary* GetShaderStageBinary(xiiGALShaderStage::Enum stage) const { return m_pShaderStageBinaries[stage]; }

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

  xiiShaderStageBinary* m_pShaderStageBinaries[xiiGALShaderStage::ENUM_COUNT];

  bool               m_bShaderPermutationValid;
  xiiGALShaderHandle m_hShader;

  xiiGALBlendStateHandle        m_hBlendState;
  xiiGALDepthStencilStateHandle m_hDepthStencilState;
  xiiGALRasterizerStateHandle   m_hRasterizerState;

  xiiHybridArray<xiiPermutationVar, 16> m_PermutationVars;
};


class xiiShaderPermutationResourceLoader : public xiiResourceTypeLoader
{
public:
  virtual xiiResourceLoadData OpenDataStream(const xiiResource* pResource) override;
  virtual void                CloseDataStream(const xiiResource* pResource, const xiiResourceLoadData& LoaderData) override;

  virtual bool IsResourceOutdated(const xiiResource* pResource) const override;

private:
  xiiResult RunCompiler(const xiiResource* pResource, xiiShaderPermutationBinary& BinaryInfo, bool bForce);
};

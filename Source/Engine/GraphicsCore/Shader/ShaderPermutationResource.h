#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>

#include <Core/ResourceManager/Resource.h>
#include <Core/ResourceManager/ResourceTypeLoader.h>
#include <Foundation/Time/Timestamp.h>
#include <GraphicsFoundation/ShaderCompiler/ShaderPermutationBinary.h>
#include <GraphicsFoundation/ShaderCompiler/PermutationGenerator.h>

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
  xiiGALShaderHandle            GetGALShader(xiiGALShaderType::Enum type) const { return m_hShaders[xiiGALShaderType::GetStageIndex(type)]; }
  const xiiGALShaderByteCode*   GetShaderByteCode(xiiGALShaderType::Enum type) const { return m_ByteCodes[xiiGALShaderType::GetStageIndex(type)]; }

  xiiGALPipelineResourceSignatureHandle GetPipelineResourceSignature() const { return m_hPipelineResourceSignature; }

  xiiGALBlendStateHandle        GetBlendState() const { return m_hBlendState; }
  xiiGALDepthStencilStateHandle GetDepthStencilState() const { return m_hDepthStencilState; }
  xiiGALRasterizerStateHandle   GetRasterizerState() const { return m_hRasterizerState; }

  bool IsShaderValid() const { return m_bShaderPermutationValid; }

  xiiArrayPtr<const xiiGALPermutationVariable> GetPermutationVars() const { return m_PermutationVariables; }

private:
  virtual xiiResourceLoadDesc    UnloadData(Unload WhatToUnload) override;
  virtual xiiResourceLoadDesc    UpdateContent(xiiStreamReader* Stream) override;
  virtual void                   UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;
  virtual xiiResourceTypeLoader* GetDefaultResourceTypeLoader() const override;

private:
  friend class xiiGALShaderPermutationUtilities;

  xiiBitflags<xiiGALShaderType>            m_ActiveShaderStages;
  xiiSharedPtr<const xiiGALShaderByteCode> m_ByteCodes[xiiGALShaderType::ENUM_COUNT];
  xiiGALShaderHandle                       m_hShaders[xiiGALShaderType::ENUM_COUNT];

  bool                                  m_bShaderPermutationValid;
  xiiGALPipelineResourceSignatureHandle m_hPipelineResourceSignature;

  xiiGALBlendStateHandle        m_hBlendState;
  xiiGALDepthStencilStateHandle m_hDepthStencilState;
  xiiGALRasterizerStateHandle   m_hRasterizerState;

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

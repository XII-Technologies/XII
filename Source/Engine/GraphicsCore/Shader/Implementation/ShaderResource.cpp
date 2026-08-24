/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>

#include <GraphicsCore/Shader/ShaderResource.h>
#include <GraphicsFoundation/ShaderCompiler/ShaderParser.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiShaderResource, 1, xiiRTTIDefaultAllocator<xiiShaderResource>)
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_RESOURCE_IMPLEMENT_COMMON_CODE(xiiShaderResource);

xiiShaderResource::xiiShaderResource() :
  xiiResource(DoUpdate::OnAnyThread, 1)
{
  m_bShaderResourceIsValid = false;
}

xiiResourceLoadDescription xiiShaderResource::UnloadData(Unload WhatToUnload)
{
  m_bShaderResourceIsValid = false;
  m_PermutationVariablesUsed.Clear();

  xiiResourceLoadDescription res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable    = 0;
  res.m_State                      = xiiResourceState::Unloaded;

  return res;
}

xiiResourceLoadDescription xiiShaderResource::UpdateContent(xiiStreamReader* pStream)
{
  xiiResourceLoadDescription res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable    = 0;

  m_bShaderResourceIsValid = false;

  if (pStream == nullptr)
  {
    res.m_State = xiiResourceState::LoadedResourceMissing;
    return res;
  }

  // Skip the absolute file path data that the standard file reader writes into the stream.
  {
    xiiStringBuilder sAbsFilePath;
    (*pStream) >> sAbsFilePath;
  }

  xiiString sContent;
  sContent.ReadAll(*pStream);

  xiiGALShaderTextSectionizer shaderTextSections;
  xiiGALShaderSections::GetShaderSections(sContent.GetView(), shaderTextSections);

  xiiUInt32                                     uiFirstLine = 0U;
  xiiHybridArray<xiiGALPermutationVariable, 16> fixedPermutationVariables; // ignored here

  xiiStringView sPermutations = shaderTextSections.GetSectionContent(xiiGALShaderSections::Permutations, uiFirstLine);
  xiiGALShaderParser::ParsePermutationSection(sPermutations, m_PermutationVariablesUsed, fixedPermutationVariables);

  res.m_State              = xiiResourceState::Loaded;
  m_bShaderResourceIsValid = true;

  return res;
}

void xiiShaderResource::UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage)
{
  out_NewMemoryUsage.m_uiMemoryCPU = sizeof(xiiShaderResource) + (xiiUInt32)m_PermutationVariablesUsed.GetHeapMemoryUsage();
  out_NewMemoryUsage.m_uiMemoryGPU = 0;
}

XII_RESOURCE_IMPLEMENT_CREATEABLE(xiiShaderResource, xiiShaderResourceDescriptor)
{
  xiiResourceLoadDescription ret;
  ret.m_State                      = xiiResourceState::Loaded;
  ret.m_uiQualityLevelsDiscardable = 0;
  ret.m_uiQualityLevelsLoadable    = 0;

  m_bShaderResourceIsValid = false;

  return ret;
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Shader_Implementation_ShaderResource);

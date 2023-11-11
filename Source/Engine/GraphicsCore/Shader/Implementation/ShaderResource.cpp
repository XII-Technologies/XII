#include <GraphicsCore/GraphicsCorePCH.h>

#include <GraphicsCore/Shader/ShaderResource.h>
#include <GraphicsCore/ShaderCompiler/ShaderParser.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiShaderResource, 1, xiiRTTIDefaultAllocator<xiiShaderResource>)
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_RESOURCE_IMPLEMENT_COMMON_CODE(xiiShaderResource);
// clang-format on

xiiShaderResource::xiiShaderResource() :
  xiiResource(DoUpdate::OnAnyThread, 1)
{
  m_bShaderResourceIsValid = false;
}

xiiResourceLoadDesc xiiShaderResource::UnloadData(Unload WhatToUnload)
{
  m_bShaderResourceIsValid = false;
  m_PermutationVarsUsed.Clear();

  xiiResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable    = 0;
  res.m_State                      = xiiResourceState::Unloaded;

  return res;
}

xiiResourceLoadDesc xiiShaderResource::UpdateContent(xiiStreamReader* stream)
{
  xiiResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable    = 0;

  m_bShaderResourceIsValid = false;

  if (stream == nullptr)
  {
    res.m_State = xiiResourceState::LoadedResourceMissing;
    return res;
  }

  // skip the absolute file path data that the standard file reader writes into the stream
  {
    xiiStringBuilder sAbsFilePath;
    (*stream) >> sAbsFilePath;
  }

  xiiHybridArray<xiiPermutationVar, 16> fixedPermVars; // ignored here
  xiiShaderParser::ParsePermutationSection(*stream, m_PermutationVarsUsed, fixedPermVars);

  res.m_State              = xiiResourceState::Loaded;
  m_bShaderResourceIsValid = true;

  return res;
}

void xiiShaderResource::UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage)
{
  out_NewMemoryUsage.m_uiMemoryCPU = sizeof(xiiShaderResource) + (xiiUInt32)m_PermutationVarsUsed.GetHeapMemoryUsage();
  out_NewMemoryUsage.m_uiMemoryGPU = 0;
}

XII_RESOURCE_IMPLEMENT_CREATEABLE(xiiShaderResource, xiiShaderResourceDescriptor)
{
  xiiResourceLoadDesc ret;
  ret.m_State                      = xiiResourceState::Loaded;
  ret.m_uiQualityLevelsDiscardable = 0;
  ret.m_uiQualityLevelsLoadable    = 0;

  m_bShaderResourceIsValid = false;

  return ret;
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Shader_Implementation_ShaderResource);

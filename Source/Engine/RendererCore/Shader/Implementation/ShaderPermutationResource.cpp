#include <RendererCore/RendererCorePCH.h>

#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/OSFile.h>
#include <RendererCore/Shader/ShaderPermutationResource.h>
#include <RendererCore/ShaderCompiler/ShaderCompiler.h>
#include <RendererCore/ShaderCompiler/ShaderManager.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/Shader/Shader.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiShaderPermutationResource, 1, xiiRTTIDefaultAllocator<xiiShaderPermutationResource>)
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_RESOURCE_IMPLEMENT_COMMON_CODE(xiiShaderPermutationResource);
// clang-format on

static xiiShaderPermutationResourceLoader g_PermutationResourceLoader;

xiiShaderPermutationResource::xiiShaderPermutationResource() :
  xiiResource(DoUpdate::OnAnyThread, 1)
{
  m_bShaderPermutationValid = false;

  for (xiiUInt32 stage = 0; stage < xiiGALShaderStage::ENUM_COUNT; ++stage)
  {
    m_pShaderStageBinaries[stage] = nullptr;
  }
}

xiiResourceLoadDesc xiiShaderPermutationResource::UnloadData(Unload WhatToUnload)
{
  m_bShaderPermutationValid = false;

  auto pDevice = xiiGALDevice::GetDefaultDevice();

  if (!m_hShader.IsInvalidated())
  {
    pDevice->DestroyShader(m_hShader);
    m_hShader.Invalidate();
  }

  if (!m_hBlendState.IsInvalidated())
  {
    pDevice->DestroyBlendState(m_hBlendState);
    m_hBlendState.Invalidate();
  }

  if (!m_hDepthStencilState.IsInvalidated())
  {
    pDevice->DestroyDepthStencilState(m_hDepthStencilState);
    m_hDepthStencilState.Invalidate();
  }

  if (!m_hRasterizerState.IsInvalidated())
  {
    pDevice->DestroyRasterizerState(m_hRasterizerState);
    m_hRasterizerState.Invalidate();
  }

  xiiResourceLoadDesc res;
  res.m_State                      = xiiResourceState::Unloaded;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable    = 0;

  return res;
}

xiiResourceLoadDesc xiiShaderPermutationResource::UpdateContent(xiiStreamReader* Stream)
{
  xiiUInt32 uiGPUMem                = 0;
  ModifyMemoryUsage().m_uiMemoryGPU = 0;

  m_bShaderPermutationValid = false;

  xiiResourceLoadDesc res;
  res.m_State                      = xiiResourceState::Loaded;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable    = 0;

  if (Stream == nullptr)
  {
    xiiLog::Error("Shader Permutation '{0}': Data is not available", GetResourceID());
    return res;
  }

  xiiShaderPermutationBinary PermutationBinary;

  bool bOldVersion = false;
  if (PermutationBinary.Read(*Stream, bOldVersion).Failed())
  {
    xiiLog::Error("Shader Permutation '{0}': Could not read shader permutation binary", GetResourceID());
    return res;
  }

  auto pDevice = xiiGALDevice::GetDefaultDevice();

  // get the shader render state object
  {
    m_hBlendState        = pDevice->CreateBlendState(PermutationBinary.m_StateDescriptor.m_BlendDesc);
    m_hDepthStencilState = pDevice->CreateDepthStencilState(PermutationBinary.m_StateDescriptor.m_DepthStencilDesc);
    m_hRasterizerState   = pDevice->CreateRasterizerState(PermutationBinary.m_StateDescriptor.m_RasterizerDesc);
  }

  xiiGALShaderCreationDescription ShaderDesc;
  ShaderDesc.m_szName = GetResourceID();

  // iterate over all shader stages, add them to the descriptor
  for (xiiUInt32 stage = xiiGALShaderStage::VertexShader; stage < xiiGALShaderStage::ENUM_COUNT; ++stage)
  {
    const xiiUInt32 uiStageHash = PermutationBinary.m_uiShaderStageHashes[stage];

    if (uiStageHash == 0) // not used
      continue;

    xiiShaderStageBinary* pStageBin = xiiShaderStageBinary::LoadStageBinary((xiiGALShaderStage::Enum)stage, uiStageHash);

    if (pStageBin == nullptr)
    {
      xiiLog::Error("Shader Permutation '{0}': Stage '{1}' could not be loaded", GetResourceID(), xiiGALShaderStage::Names[stage]);
      return res;
    }

    // store not only the hash but also the pointer to the stage binary
    // since it contains other useful information (resource bindings), that we need for shader binding
    m_pShaderStageBinaries[stage] = pStageBin;

    XII_ASSERT_DEV(pStageBin->m_Stage == stage, "Invalid shader stage! Expected stage '{0}', but loaded data is for stage '{1}'", xiiGALShaderStage::Names[stage], xiiGALShaderStage::Names[pStageBin->m_Stage]);

    ShaderDesc.m_ByteCodes[stage] = pStageBin->m_GALByteCode;

    uiGPUMem += pStageBin->m_ByteCode.GetCount();
  }

  m_hShader = pDevice->CreateShader(ShaderDesc);

  if (m_hShader.IsInvalidated())
  {
    xiiLog::Error("Shader Permutation '{0}': Shader program creation failed", GetResourceID());
    return res;
  }

  m_PermutationVars = PermutationBinary.m_PermutationVars;

  m_bShaderPermutationValid = true;

  ModifyMemoryUsage().m_uiMemoryGPU = uiGPUMem;

  return res;
}

void xiiShaderPermutationResource::UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage)
{
  out_NewMemoryUsage.m_uiMemoryCPU = sizeof(xiiShaderPermutationResource);
  out_NewMemoryUsage.m_uiMemoryGPU = ModifyMemoryUsage().m_uiMemoryGPU;
}

XII_RESOURCE_IMPLEMENT_CREATEABLE(xiiShaderPermutationResource, xiiShaderPermutationResourceDescriptor)
{
  xiiResourceLoadDesc ret;
  ret.m_State                      = xiiResourceState::Loaded;
  ret.m_uiQualityLevelsDiscardable = 0;
  ret.m_uiQualityLevelsLoadable    = 0;

  return ret;
}

xiiResourceTypeLoader* xiiShaderPermutationResource::GetDefaultResourceTypeLoader() const
{
  return &g_PermutationResourceLoader;
}

struct ShaderPermutationResourceLoadData
{
  ShaderPermutationResourceLoadData() :
    m_Reader(&m_Storage)
  {
  }

  xiiContiguousMemoryStreamStorage m_Storage;
  xiiMemoryStreamReader            m_Reader;
};

xiiResult xiiShaderPermutationResourceLoader::RunCompiler(const xiiResource* pResource, xiiShaderPermutationBinary& BinaryInfo, bool bForce)
{
  if (xiiShaderManager::IsRuntimeCompilationEnabled())
  {
    if (!bForce)
    {
      // check whether any dependent file has changed, and trigger a recompilation if necessary
      if (BinaryInfo.m_DependencyFile.HasAnyFileChanged())
      {
        bForce = true;
      }
    }

    if (!bForce) // no recompilation necessary
      return XII_SUCCESS;

    xiiStringBuilder sPermutationFile = pResource->GetResourceID();

    sPermutationFile.ChangeFileExtension("");
    sPermutationFile.Shrink(xiiShaderManager::GetCacheDirectory().GetCharacterCount() + xiiShaderManager::GetActivePlatform().GetCharacterCount() + 2, 1);

    sPermutationFile.Shrink(0, 9); // remove underscore and the hash at the end
    sPermutationFile.Append(".xiiShader");

    xiiArrayPtr<const xiiPermutationVar> permutationVars = static_cast<const xiiShaderPermutationResource*>(pResource)->GetPermutationVars();

    xiiShaderCompiler sc;
    return sc.CompileShaderPermutationForPlatforms(sPermutationFile, permutationVars, xiiLog::GetThreadLocalLogSystem(), xiiShaderManager::GetActivePlatform());
  }
  else
  {
    if (bForce)
    {
      xiiLog::Error("Shader was forced to be compiled, but runtime shader compilation is not available");
      return XII_FAILURE;
    }
  }

  return XII_SUCCESS;
}

bool xiiShaderPermutationResourceLoader::IsResourceOutdated(const xiiResource* pResource) const
{
  // don't try to reload a file that cannot be found
  xiiStringBuilder sAbs;
  if (xiiFileSystem::ResolvePath(pResource->GetResourceID(), &sAbs, nullptr).Failed())
    return false;

#if XII_ENABLED(XII_SUPPORTS_FILE_STATS)
  if (pResource->GetLoadedFileModificationTime().IsValid())
  {
    xiiFileStats stat;
    if (xiiFileSystem::GetFileStats(pResource->GetResourceID(), stat).Failed())
      return false;

    if (!stat.m_LastModificationTime.Compare(pResource->GetLoadedFileModificationTime(), xiiTimestamp::CompareMode::FileTimeEqual))
      return true;
  }

#endif

  xiiDependencyFile dep;
  if (dep.ReadDependencyFile(pResource->GetResourceID()).Failed())
    return true;

  return dep.HasAnyFileChanged();
}

xiiResourceLoadData xiiShaderPermutationResourceLoader::OpenDataStream(const xiiResource* pResource)
{
  xiiResourceLoadData res;

  xiiShaderPermutationBinary permutationBinary;

  bool bNeedsCompilation = true;
  bool bOldVersion       = false;

  {
    xiiFileReader File;
    if (File.Open(pResource->GetResourceID().GetData()).Failed())
    {
      xiiLog::Debug("Shader Permutation '{0}' does not exist, triggering recompile.", pResource->GetResourceID());

      bNeedsCompilation = false;
      if (RunCompiler(pResource, permutationBinary, true).Failed())
        return res;

      // try again
      if (File.Open(pResource->GetResourceID().GetData()).Failed())
      {
        xiiLog::Debug("Shader Permutation '{0}' still does not exist after recompile.", pResource->GetResourceID());
        return res;
      }
    }

    res.m_sResourceDescription = File.GetFilePathRelative().GetData();

#if XII_ENABLED(XII_SUPPORTS_FILE_STATS)
    xiiFileStats stat;
    if (xiiFileSystem::GetFileStats(pResource->GetResourceID(), stat).Succeeded())
    {
      res.m_LoadedFileModificationDate = stat.m_LastModificationTime;
    }
#endif

    if (permutationBinary.Read(File, bOldVersion).Failed())
    {
      xiiLog::Error("Shader Permutation '{0}': Could not read shader permutation binary", pResource->GetResourceID());

      bNeedsCompilation = true;
    }

    if (bOldVersion)
    {
      xiiLog::Dev("Shader Permutation Binary version is outdated, recompiling shader.");
      bNeedsCompilation = true;
    }
  }

  if (bNeedsCompilation)
  {
    if (RunCompiler(pResource, permutationBinary, false).Failed())
      return res;

    xiiFileReader File;

    if (File.Open(pResource->GetResourceID().GetData()).Failed())
    {
      xiiLog::Error("Shader Permutation '{0}': Failed to open the file", pResource->GetResourceID());
      return res;
    }

    if (permutationBinary.Read(File, bOldVersion).Failed())
    {
      xiiLog::Error("Shader Permutation '{0}': Binary data could not be read", pResource->GetResourceID());
      return res;
    }

    File.Close();
  }

  ShaderPermutationResourceLoadData* pData = XII_DEFAULT_NEW(ShaderPermutationResourceLoadData);

  xiiMemoryStreamWriter w(&pData->m_Storage);

  // preload the files that are referenced in the .xiiPermutation file
  {
    // write the permutation file info back to the output stream, so that the resource can read it as well
    permutationBinary.Write(w).IgnoreResult();

    for (xiiUInt32 stage = xiiGALShaderStage::VertexShader; stage < xiiGALShaderStage::ENUM_COUNT; ++stage)
    {
      const xiiUInt32 uiStageHash = permutationBinary.m_uiShaderStageHashes[stage];

      if (uiStageHash == 0) // not used
        continue;

      // this is where the preloading happens
      xiiShaderStageBinary::LoadStageBinary((xiiGALShaderStage::Enum)stage, uiStageHash);
    }
  }

  res.m_pDataStream       = &pData->m_Reader;
  res.m_pCustomLoaderData = pData;

  return res;
}

void xiiShaderPermutationResourceLoader::CloseDataStream(const xiiResource* pResource, const xiiResourceLoadData& LoaderData)
{
  ShaderPermutationResourceLoadData* pData = static_cast<ShaderPermutationResourceLoadData*>(LoaderData.m_pCustomLoaderData);

  XII_DEFAULT_DELETE(pData);
}

XII_STATICLINK_FILE(RendererCore, RendererCore_Shader_Implementation_ShaderPermutationResource);

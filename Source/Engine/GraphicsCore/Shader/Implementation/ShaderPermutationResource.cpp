/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>

#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/OSFile.h>
#include <GraphicsCore/Shader/ShaderPermutationResource.h>
#include <GraphicsFoundation/Device/Device.h>
#include <GraphicsFoundation/Shader/Shader.h>
#include <GraphicsFoundation/ShaderCompiler/ShaderCompiler.h>
#include <GraphicsFoundation/ShaderCompiler/ShaderManager.h>
#include <GraphicsFoundation/ShaderCompiler/ShaderStageBinary.h>
#include <GraphicsFoundation/States/PipelineResourceSignature.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiShaderPermutationResource, 1, xiiRTTIDefaultAllocator<xiiShaderPermutationResource>)
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_RESOURCE_IMPLEMENT_COMMON_CODE(xiiShaderPermutationResource);

static xiiShaderPermutationResourceLoader g_PermutationResourceLoader;

xiiShaderPermutationResource::xiiShaderPermutationResource() :
  xiiResource(DoUpdate::OnAnyThread, 1)
{
  m_bShaderPermutationValid = false;
}

xiiResourceLoadDescription xiiShaderPermutationResource::UnloadData(Unload WhatToUnload)
{
  m_bShaderPermutationValid = false;

  for (auto it : m_ShaderData)
  {
    auto& shaderData = it.Value();

    shaderData.m_pShader.Clear();
  }

  m_pPipelineResourceSignature.Clear();
  m_pBlendState.Clear();
  m_pDepthStencilState.Clear();
  m_pRasterizerState.Clear();
  m_ShaderData.Clear();

  m_ActiveShaderStages = xiiGALShaderType::Unknown;

  xiiResourceLoadDescription res;
  res.m_State                      = xiiResourceState::Unloaded;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable    = 0;

  return res;
}

xiiResourceLoadDescription xiiShaderPermutationResource::UpdateContent(xiiStreamReader* pStream)
{
  xiiUInt32 uiGPUMemory             = 0;
  ModifyMemoryUsage().m_uiMemoryGPU = 0;

  m_bShaderPermutationValid = false;

  xiiResourceLoadDescription res;
  res.m_State                      = xiiResourceState::Loaded;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable    = 0;

  if (pStream == nullptr)
  {
    xiiLog::Error("Shader Permutation '{0}': Data is not available.", GetResourceID());
    return res;
  }

  xiiGALShaderPermutationBinary shaderPermutationBinary;

  bool bOldVersion = false;
  if (shaderPermutationBinary.Read(*pStream, bOldVersion).Failed())
  {
    xiiLog::Error("Shader Permutation '{0}': Could not read shader permutation binary.", GetResourceID());
    return res;
  }

  xiiSharedPtr<xiiGALDevice> pDevice = xiiGALDevice::GetDefaultDevice();

  // Retrieve the shader render state object.
  {
    m_pBlendState        = pDevice->CreateBlendState(shaderPermutationBinary.m_StateDescriptor.m_BlendDescription);
    m_pDepthStencilState = pDevice->CreateDepthStencilState(shaderPermutationBinary.m_StateDescriptor.m_DepthStencilDescription);
    m_pRasterizerState   = pDevice->CreateRasterizerState(shaderPermutationBinary.m_StateDescriptor.m_RasterizerDescription);
  }

  xiiGALPipelineResourceSignatureCreationDescription resourceSignatureDescription;

  // iterate over all shader stages, add them to the descriptor
  for (auto it : shaderPermutationBinary.m_ShaderStageHashes)
  {
    const xiiUInt32 uiStageHash = it.Value();

    if (uiStageHash == 0) // not used
      continue;

    xiiGALShaderStageBinary* pStageBinary = xiiGALShaderStageBinary::LoadStageBinary(it.Key(), uiStageHash, xiiGALShaderManager::GetActivePlatform());

    if (pStageBinary == nullptr)
    {
      xiiLog::Error("Shader Permutation '{0}': Stage '{1}' could not be loaded.", GetResourceID(), xiiGALShaderType::Names[xiiGALShaderType::GetStageIndex(it.Key())]);
      return res;
    }

    // Store not only the hash but also the pointer to the stage binary.
    // It contains other useful information (resource bindings), that we need for shader binding.
    {
      ShaderData shaderData;
      if (m_ShaderData.TryGetValue(it.Key(), shaderData))
      {
        shaderData.m_pByteCode = pStageBinary->GetByteCode();
      }
      else
      {
        m_ShaderData.Insert(it.Key(), ShaderData{.m_pByteCode = pStageBinary->GetByteCode()});
      }
    }

    XII_ASSERT_DEV(pStageBinary->GetByteCode()->m_ShaderStage == it.Key(), "Invalid shader stage! Expected stage '{0}', but loaded data is for stage '{1}'", xiiGALShaderType::Names[xiiGALShaderType::GetStageIndex(it.Key())], xiiGALShaderType::Names[xiiGALShaderType::GetStageIndex((xiiGALShaderType::Enum)pStageBinary->GetByteCode()->m_ShaderStage.GetValue())]);

    if (pStageBinary->GetByteCode()->IsValid())
    {
      ShaderData* pShaderData = m_ShaderData.GetValue(it.Key());

      xiiGALShaderCreationDescription shaderDescription;
      shaderDescription.m_ShaderType = it.Key();
      shaderDescription.m_ByteCode   = const_cast<xiiGALShaderByteCode*>(pStageBinary->GetByteCode().Borrow()); //TODO: Improve this and avoid const-cast.

      pShaderData->m_pShader = pDevice->CreateShader(shaderDescription);

      if (!pShaderData->m_pShader)
      {
        xiiLog::Error("Shader Permutation '{0}': Shader program creation for {1} shader failed.", GetResourceID(), xiiGALShaderType::Names[xiiGALShaderType::GetStageIndex(it.Key())]);
        return res;
      }
      pShaderData->m_pShader->SetDebugName(GetResourceID());

      m_ActiveShaderStages |= it.Key();

      uiGPUMemory += pStageBinary->GetByteCode()->m_ByteCode.GetCount();

      static xiiTempHashedString sLinearSampler("LinearSampler");
      static xiiTempHashedString sLinearClampSampler("LinearClampSampler");
      static xiiTempHashedString sPointSampler("PointSampler");
      static xiiTempHashedString sPointClampSampler("PointClampSampler");

      for (const xiiGALShaderResourceDescription& resource : pStageBinary->GetByteCode()->m_ShaderResourceBindings)
      {
        // Try to find an existing resource with the same bind set and bind slot.
        xiiGALPipelineResourceDescription* pExistingResource = nullptr;
        for (xiiGALPipelineResourceDescription& existing : resourceSignatureDescription.m_Resources)
        {
          if (existing.m_uiBindSet == resource.m_uiDescriptorSet && existing.m_uiBindSlot == resource.m_uiBindIndex)
          {
            pExistingResource = &existing;
            break;
          }
        }

        if (pExistingResource)
        {
          // Merge shader stages and reconcile array size.
          pExistingResource->m_ShaderStages |= resource.m_ShaderStages;
          pExistingResource->m_uiArraySize = xiiMath::Max(pExistingResource->m_uiArraySize, resource.m_uiArraySize);

          // If resource types differ, prefer the existing one but log a warning.
          if (pExistingResource->m_ResourceType != resource.m_Type)
          {
            xiiLog::Warning("Shader Permutation '{0}': Resource bind point set {1} slot {2} has conflicting types ({3} vs {4}). Keeping existing type.", GetResourceID(), resource.m_uiDescriptorSet, resource.m_uiBindIndex, pExistingResource->m_ResourceType, resource.m_Type);
          }
        }
        else
        {
          // No existing resource at this bind point -> create new entry.
          xiiGALPipelineResourceDescription& resourceSignature = resourceSignatureDescription.m_Resources.ExpandAndGetRef();

          resourceSignature.m_sName                 = resource.m_sName;
          resourceSignature.m_ResourceType          = resource.m_Type;
          resourceSignature.m_ShaderStages          = resource.m_ShaderStages;
          resourceSignature.m_uiArraySize           = resource.m_uiArraySize;
          resourceSignature.m_uiBindSlot            = resource.m_uiBindIndex;
          resourceSignature.m_uiBindSet             = resource.m_uiDescriptorSet;
          resourceSignature.m_PipelineResourceFlags = xiiGALPipelineResourceFlags::None;
        }

        // Immutable Samplers: only add if resource is a sampler and not already present.
        if (resource.m_Type == xiiGALShaderResourceType::Sampler)
        {
          const xiiHashedString& sImmutableSamplerName = resource.m_sName;

          // Check if an immutable sampler with this name already exists.
          bool bImmutableSamplerExists = false;
          for (const xiiGALImmutableSamplerDescription& immutableSampler : resourceSignatureDescription.m_ImmutableSamplers)
          {
            if (immutableSampler.m_SamplerOrTextureName == sImmutableSamplerName)
            {
              bImmutableSamplerExists = true;
              break;
            }
          }

          if (!bImmutableSamplerExists)
          {
            // Create immutable sampler based on known names.
            if (sImmutableSamplerName == sLinearSampler)
            {
              xiiGALImmutableSamplerDescription& linearSampler        = resourceSignatureDescription.m_ImmutableSamplers.ExpandAndGetRef();
              linearSampler.m_SamplerOrTextureName                    = sImmutableSamplerName;
              linearSampler.m_ShaderStages                            = xiiGALShaderType::AllGraphics;
              linearSampler.m_SamplerDescription.m_ComparisonFunction = xiiGALComparisonFunction::Never;
              linearSampler.m_SamplerDescription.m_BorderColor        = xiiColor::Black;
              linearSampler.m_SamplerDescription.m_fMipLODBias        = 0.0f;
              linearSampler.m_SamplerDescription.m_fMinLOD            = -1.0f;
              linearSampler.m_SamplerDescription.m_fMaxLOD            = 42000.0f;
              linearSampler.m_SamplerDescription.m_uiMaxAnisotropy    = 4U;
              linearSampler.m_SamplerDescription.m_MinFilter          = xiiGALFilterType::Linear;
              linearSampler.m_SamplerDescription.m_MagFilter          = xiiGALFilterType::Linear;
              linearSampler.m_SamplerDescription.m_MipFilter          = xiiGALFilterType::Linear;
              linearSampler.m_SamplerDescription.m_AddressU           = xiiGALTextureAddressMode::Wrap;
              linearSampler.m_SamplerDescription.m_AddressV           = xiiGALTextureAddressMode::Wrap;
              linearSampler.m_SamplerDescription.m_AddressW           = xiiGALTextureAddressMode::Wrap;
            }
            else if (sImmutableSamplerName == sLinearClampSampler)
            {
              xiiGALImmutableSamplerDescription& linearClampSampler        = resourceSignatureDescription.m_ImmutableSamplers.ExpandAndGetRef();
              linearClampSampler.m_SamplerOrTextureName                    = sImmutableSamplerName;
              linearClampSampler.m_ShaderStages                            = xiiGALShaderType::AllGraphics;
              linearClampSampler.m_SamplerDescription.m_ComparisonFunction = xiiGALComparisonFunction::Never;
              linearClampSampler.m_SamplerDescription.m_BorderColor        = xiiColor::Black;
              linearClampSampler.m_SamplerDescription.m_fMipLODBias        = 0.0f;
              linearClampSampler.m_SamplerDescription.m_fMinLOD            = -1.0f;
              linearClampSampler.m_SamplerDescription.m_fMaxLOD            = 42000.0f;
              linearClampSampler.m_SamplerDescription.m_uiMaxAnisotropy    = 4U;
              linearClampSampler.m_SamplerDescription.m_MinFilter          = xiiGALFilterType::Linear;
              linearClampSampler.m_SamplerDescription.m_MagFilter          = xiiGALFilterType::Linear;
              linearClampSampler.m_SamplerDescription.m_MipFilter          = xiiGALFilterType::Linear;
              linearClampSampler.m_SamplerDescription.m_AddressU           = xiiGALTextureAddressMode::Clamp;
              linearClampSampler.m_SamplerDescription.m_AddressV           = xiiGALTextureAddressMode::Clamp;
              linearClampSampler.m_SamplerDescription.m_AddressW           = xiiGALTextureAddressMode::Clamp;
            }
            else if (sImmutableSamplerName == sPointSampler)
            {
              xiiGALImmutableSamplerDescription& pointSampler        = resourceSignatureDescription.m_ImmutableSamplers.ExpandAndGetRef();
              pointSampler.m_SamplerOrTextureName                    = sImmutableSamplerName;
              pointSampler.m_ShaderStages                            = xiiGALShaderType::AllGraphics;
              pointSampler.m_SamplerDescription.m_ComparisonFunction = xiiGALComparisonFunction::Never;
              pointSampler.m_SamplerDescription.m_BorderColor        = xiiColor::Black;
              pointSampler.m_SamplerDescription.m_fMipLODBias        = 0.0f;
              pointSampler.m_SamplerDescription.m_fMinLOD            = -1.0f;
              pointSampler.m_SamplerDescription.m_fMaxLOD            = 42000.0f;
              pointSampler.m_SamplerDescription.m_uiMaxAnisotropy    = 4U;
              pointSampler.m_SamplerDescription.m_MinFilter          = xiiGALFilterType::Linear;
              pointSampler.m_SamplerDescription.m_MagFilter          = xiiGALFilterType::Linear;
              pointSampler.m_SamplerDescription.m_MipFilter          = xiiGALFilterType::Linear;
              pointSampler.m_SamplerDescription.m_AddressU           = xiiGALTextureAddressMode::Wrap;
              pointSampler.m_SamplerDescription.m_AddressV           = xiiGALTextureAddressMode::Wrap;
              pointSampler.m_SamplerDescription.m_AddressW           = xiiGALTextureAddressMode::Wrap;
            }
            else if (sImmutableSamplerName == sPointClampSampler)
            {
              xiiGALImmutableSamplerDescription& pointClampSampler        = resourceSignatureDescription.m_ImmutableSamplers.ExpandAndGetRef();
              pointClampSampler.m_SamplerOrTextureName                    = sImmutableSamplerName;
              pointClampSampler.m_ShaderStages                            = xiiGALShaderType::AllGraphics;
              pointClampSampler.m_SamplerDescription.m_ComparisonFunction = xiiGALComparisonFunction::Never;
              pointClampSampler.m_SamplerDescription.m_BorderColor        = xiiColor::Black;
              pointClampSampler.m_SamplerDescription.m_fMipLODBias        = 0.0f;
              pointClampSampler.m_SamplerDescription.m_fMinLOD            = -1.0f;
              pointClampSampler.m_SamplerDescription.m_fMaxLOD            = 42000.0f;
              pointClampSampler.m_SamplerDescription.m_uiMaxAnisotropy    = 4U;
              pointClampSampler.m_SamplerDescription.m_MinFilter          = xiiGALFilterType::Linear;
              pointClampSampler.m_SamplerDescription.m_MagFilter          = xiiGALFilterType::Linear;
              pointClampSampler.m_SamplerDescription.m_MipFilter          = xiiGALFilterType::Linear;
              pointClampSampler.m_SamplerDescription.m_AddressU           = xiiGALTextureAddressMode::Clamp;
              pointClampSampler.m_SamplerDescription.m_AddressV           = xiiGALTextureAddressMode::Clamp;
              pointClampSampler.m_SamplerDescription.m_AddressW           = xiiGALTextureAddressMode::Clamp;
            }
          }
        }
      }
    }
  }

  m_pPipelineResourceSignature = pDevice->CreatePipelineResourceSignature(resourceSignatureDescription);

  if (!m_pPipelineResourceSignature)
  {
    xiiLog::Error("Shader Permutation '{0}': Shader pipeline resource signature creation failed.", GetResourceID());
    return res;
  }

  m_PermutationVariables = shaderPermutationBinary.m_PermutationVariables;

  m_bShaderPermutationValid = true;

  ModifyMemoryUsage().m_uiMemoryGPU = uiGPUMemory;

  return res;
}

void xiiShaderPermutationResource::UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage)
{
  out_NewMemoryUsage.m_uiMemoryCPU = sizeof(xiiShaderPermutationResource);
  out_NewMemoryUsage.m_uiMemoryGPU = ModifyMemoryUsage().m_uiMemoryGPU;
}

XII_RESOURCE_IMPLEMENT_CREATEABLE(xiiShaderPermutationResource, xiiShaderPermutationResourceDescriptor)
{
  xiiResourceLoadDescription ret;
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

xiiResult xiiShaderPermutationResourceLoader::RunCompiler(const xiiResource* pResource, xiiGALShaderPermutationBinary& BinaryInfo, bool bForce)
{
  if (xiiGALShaderManager::IsRuntimeCompilationEnabled())
  {
    if (!bForce)
    {
      // check whether any dependent file has changed, and trigger a recompilation if necessary.
      if (BinaryInfo.m_DependencyFile.HasAnyFileChanged())
      {
        bForce = true;
      }
    }

    if (!bForce) // no recompilation necessary
      return XII_SUCCESS;

    xiiStringBuilder sPermutationFile = pResource->GetResourceID();

    sPermutationFile.ChangeFileExtension("");
    sPermutationFile.Shrink(xiiGALShaderManager::GetCacheDirectory().GetCharacterCount() + xiiGALShaderManager::GetActivePlatform().GetCharacterCount() + 2, 1);

    sPermutationFile.Shrink(0, 9); // remove underscore and the hash at the end
    sPermutationFile.Append(".xiiShader");

    xiiArrayPtr<const xiiGALPermutationVariable> permutationVariables = static_cast<const xiiShaderPermutationResource*>(pResource)->GetPermutationVariables();

    xiiGALShaderCompiler sc;
    return sc.CompileShaderPermutationForPlatforms(sPermutationFile, permutationVariables, xiiLog::GetThreadLocalLogSystem(), xiiGALShaderManager::GetActivePlatform());
  }
  else
  {
    if (bForce)
    {
      xiiLog::Error("Shader was forced to be compiled, but runtime shader compilation is not available.");
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

  xiiGALShaderPermutationBinary permutationBinary;

  bool bNeedsCompilation = true;
  bool bOldVersion       = false;

  {
    xiiFileReader File;
    if (File.Open(pResource->GetResourceID()).Failed())
    {
      xiiLog::Debug("Shader Permutation '{0}' does not exist, triggering recompile.", pResource->GetResourceID());

      bNeedsCompilation = false;
      if (RunCompiler(pResource, permutationBinary, true).Failed())
        return res;

      // try again
      if (File.Open(pResource->GetResourceID()).Failed())
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

    if (File.Open(pResource->GetResourceID()).Failed())
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

    for (auto it : permutationBinary.m_ShaderStageHashes)
    {
      const xiiUInt32 uiStageHash = it.Value();

      if (uiStageHash == 0) // not used
        continue;

      // this is where the preloading happens
      xiiGALShaderStageBinary::LoadStageBinary(it.Key(), uiStageHash, xiiGALShaderManager::GetActivePlatform());
    }
  }

  res.m_pDataStream       = &pData->m_Reader;
  res.m_pCustomLoaderData = pData;

  return res;
}

void xiiShaderPermutationResourceLoader::CloseDataStream(const xiiResource* pResource, const xiiResourceLoadData& loaderData)
{
  ShaderPermutationResourceLoadData* pData = static_cast<ShaderPermutationResourceLoadData*>(loaderData.m_pCustomLoaderData);

  XII_DEFAULT_DELETE(pData);
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Shader_Implementation_ShaderPermutationResource);

#include <GraphicsCore/GraphicsCorePCH.h>

#include <Core/Assets/AssetFileHeader.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/IO/OpenDdlReader.h>
#include <Foundation/IO/OpenDdlUtils.h>
#include <Foundation/Types/ScopeExit.h>
#include <GraphicsCore/Material/MaterialResource.h>
#include <GraphicsCore/RenderContext/RenderContext.h>
#include <GraphicsCore/RenderWorld/RenderWorld.h>
#include <GraphicsCore/Shader/ShaderPermutationResource.h>
#include <GraphicsCore/ShaderCompiler/ShaderManager.h>
#include <GraphicsCore/Textures/Texture2DResource.h>
#include <GraphicsCore/Textures/TextureCubeResource.h>
#include <GraphicsCore/Textures/TextureLoader.h>
#include <Texture/Image/Formats/DdsFileFormat.h>

#ifdef BUILDSYSTEM_ENABLE_ZSTD_SUPPORT
#  include <Foundation/IO/CompressedStreamZstd.h>
#endif

void xiiMaterialResourceDescriptor::Clear()
{
  m_hBaseMaterial.Invalidate();
  m_sSurface.Clear();
  m_hShader.Invalidate();
  m_PermutationVars.Clear();
  m_Parameters.Clear();
  m_Texture2DBindings.Clear();
  m_TextureCubeBindings.Clear();
  m_RenderDataCategory = xiiInvalidRenderDataCategory;
}

bool xiiMaterialResourceDescriptor::operator==(const xiiMaterialResourceDescriptor& other) const
{
  return m_hBaseMaterial == other.m_hBaseMaterial &&
    m_hShader == other.m_hShader &&
    m_PermutationVars == other.m_PermutationVars &&
    m_Parameters == other.m_Parameters &&
    m_Texture2DBindings == other.m_Texture2DBindings &&
    m_TextureCubeBindings == other.m_TextureCubeBindings &&
    m_RenderDataCategory == other.m_RenderDataCategory;
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiMaterialResource, 1, xiiRTTIDefaultAllocator<xiiMaterialResource>)
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_RESOURCE_IMPLEMENT_COMMON_CODE(xiiMaterialResource);
// clang-format on

// clang-format off
XII_BEGIN_SUBSYSTEM_DECLARATION(GraphicsCore, MaterialResource)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "Foundation",
    "Core"
  END_SUBSYSTEM_DEPENDENCIES

  ON_HIGHLEVELSYSTEMS_SHUTDOWN
  {
    xiiMaterialResource::ClearCache();
  }

XII_END_SUBSYSTEM_DECLARATION;
// clang-format on

xiiDeque<xiiMaterialResource::CachedValues> xiiMaterialResource::s_CachedValues;

xiiMaterialResource::xiiMaterialResource() :
  xiiResource(DoUpdate::OnAnyThread, 1)
{
  m_iLastUpdated          = 0;
  m_iLastConstantsUpdated = 0;
  m_uiCacheIndex          = xiiInvalidIndex;
  m_pCachedValues         = nullptr;

  xiiResourceManager::GetResourceEvents().AddEventHandler(xiiMakeDelegate(&xiiMaterialResource::OnResourceEvent, this));
}

xiiMaterialResource::~xiiMaterialResource()
{
  xiiResourceManager::GetResourceEvents().RemoveEventHandler(xiiMakeDelegate(&xiiMaterialResource::OnResourceEvent, this));
}

xiiHashedString xiiMaterialResource::GetPermutationValue(const xiiTempHashedString& sName)
{
  auto pCachedValues = GetOrUpdateCachedValues();

  xiiHashedString sResult;
  pCachedValues->m_PermutationVars.TryGetValue(sName, sResult);

  return sResult;
}

xiiHashedString xiiMaterialResource::GetSurface() const
{
  if (!m_mDesc.m_sSurface.IsEmpty())
    return m_mDesc.m_sSurface;

  if (m_mDesc.m_hBaseMaterial.IsValid())
  {
    xiiResourceLock<xiiMaterialResource> pBaseMaterial(m_mDesc.m_hBaseMaterial, xiiResourceAcquireMode::BlockTillLoaded);
    return pBaseMaterial->GetSurface();
  }

  return xiiHashedString();
}

void xiiMaterialResource::SetParameter(const xiiHashedString& sName, const xiiVariant& value)
{
  xiiUInt32 uiIndex = xiiInvalidIndex;
  for (xiiUInt32 i = 0; i < m_mDesc.m_Parameters.GetCount(); ++i)
  {
    if (m_mDesc.m_Parameters[i].m_Name == sName)
    {
      uiIndex = i;
      break;
    }
  }

  if (value.IsValid())
  {
    if (uiIndex != xiiInvalidIndex)
    {
      if (m_mDesc.m_Parameters[uiIndex].m_Value == value)
      {
        return;
      }

      m_mDesc.m_Parameters[uiIndex].m_Value = value;
    }
    else
    {
      auto& param   = m_mDesc.m_Parameters.ExpandAndGetRef();
      param.m_Name  = sName;
      param.m_Value = value;
    }
  }
  else
  {
    if (uiIndex == xiiInvalidIndex)
    {
      return;
    }

    m_mDesc.m_Parameters.RemoveAtAndSwap(uiIndex);
  }

  m_iLastModified.Increment();
  m_iLastConstantsModified.Increment();

  m_ModifiedEvent.Broadcast(this);
}

void xiiMaterialResource::SetParameter(const char* szName, const xiiVariant& value)
{
  xiiTempHashedString sName(szName);

  xiiUInt32 uiIndex = xiiInvalidIndex;
  for (xiiUInt32 i = 0; i < m_mDesc.m_Parameters.GetCount(); ++i)
  {
    if (m_mDesc.m_Parameters[i].m_Name == sName)
    {
      uiIndex = i;
      break;
    }
  }

  if (value.IsValid())
  {
    if (uiIndex != xiiInvalidIndex)
    {
      if (m_mDesc.m_Parameters[uiIndex].m_Value == value)
      {
        return;
      }

      m_mDesc.m_Parameters[uiIndex].m_Value = value;
    }
    else
    {
      auto& param = m_mDesc.m_Parameters.ExpandAndGetRef();
      param.m_Name.Assign(szName);
      param.m_Value = value;
    }
  }
  else
  {
    if (uiIndex == xiiInvalidIndex)
    {
      return;
    }

    m_mDesc.m_Parameters.RemoveAtAndSwap(uiIndex);
  }

  m_iLastModified.Increment();
  m_iLastConstantsModified.Increment();

  m_ModifiedEvent.Broadcast(this);
}

xiiVariant xiiMaterialResource::GetParameter(const xiiTempHashedString& sName)
{
  auto pCachedValues = GetOrUpdateCachedValues();

  xiiVariant value;
  pCachedValues->m_Parameters.TryGetValue(sName, value);

  return value;
}

void xiiMaterialResource::SetTexture2DBinding(const xiiHashedString& sName, const xiiTexture2DResourceHandle& value)
{
  xiiUInt32 uiIndex = xiiInvalidIndex;
  for (xiiUInt32 i = 0; i < m_mDesc.m_Texture2DBindings.GetCount(); ++i)
  {
    if (m_mDesc.m_Texture2DBindings[i].m_Name == sName)
    {
      uiIndex = i;
      break;
    }
  }

  if (value.IsValid())
  {
    if (uiIndex != xiiInvalidIndex)
    {
      m_mDesc.m_Texture2DBindings[uiIndex].m_Value = value;
    }
    else
    {
      auto& binding   = m_mDesc.m_Texture2DBindings.ExpandAndGetRef();
      binding.m_Name  = sName;
      binding.m_Value = value;
    }
  }
  else
  {
    if (uiIndex != xiiInvalidIndex)
    {
      m_mDesc.m_Texture2DBindings.RemoveAtAndSwap(uiIndex);
    }
  }

  m_iLastModified.Increment();

  m_ModifiedEvent.Broadcast(this);
}

void xiiMaterialResource::SetTexture2DBinding(const char* szName, const xiiTexture2DResourceHandle& value)
{
  xiiTempHashedString sName(szName);

  xiiUInt32 uiIndex = xiiInvalidIndex;
  for (xiiUInt32 i = 0; i < m_mDesc.m_Texture2DBindings.GetCount(); ++i)
  {
    if (m_mDesc.m_Texture2DBindings[i].m_Name == sName)
    {
      uiIndex = i;
      break;
    }
  }

  if (value.IsValid())
  {
    if (uiIndex != xiiInvalidIndex)
    {
      m_mDesc.m_Texture2DBindings[uiIndex].m_Value = value;
    }
    else
    {
      auto& binding = m_mDesc.m_Texture2DBindings.ExpandAndGetRef();
      binding.m_Name.Assign(szName);
      binding.m_Value = value;
    }
  }
  else
  {
    if (uiIndex != xiiInvalidIndex)
    {
      m_mDesc.m_Texture2DBindings.RemoveAtAndSwap(uiIndex);
    }
  }

  m_iLastModified.Increment();

  m_ModifiedEvent.Broadcast(this);
}

xiiTexture2DResourceHandle xiiMaterialResource::GetTexture2DBinding(const xiiTempHashedString& sName)
{
  auto pCachedValues = GetOrUpdateCachedValues();

  // Use pointer to prevent ref counting
  xiiTexture2DResourceHandle* pBinding;
  if (pCachedValues->m_Texture2DBindings.TryGetValue(sName, pBinding))
  {
    return *pBinding;
  }

  return xiiTexture2DResourceHandle();
}


void xiiMaterialResource::SetTextureCubeBinding(const xiiHashedString& sName, const xiiTextureCubeResourceHandle& value)
{
  xiiUInt32 uiIndex = xiiInvalidIndex;
  for (xiiUInt32 i = 0; i < m_mDesc.m_TextureCubeBindings.GetCount(); ++i)
  {
    if (m_mDesc.m_TextureCubeBindings[i].m_Name == sName)
    {
      uiIndex = i;
      break;
    }
  }

  if (value.IsValid())
  {
    if (uiIndex != xiiInvalidIndex)
    {
      m_mDesc.m_TextureCubeBindings[uiIndex].m_Value = value;
    }
    else
    {
      auto& binding   = m_mDesc.m_TextureCubeBindings.ExpandAndGetRef();
      binding.m_Name  = sName;
      binding.m_Value = value;
    }
  }
  else
  {
    if (uiIndex != xiiInvalidIndex)
    {
      m_mDesc.m_TextureCubeBindings.RemoveAtAndSwap(uiIndex);
    }
  }

  m_iLastModified.Increment();

  m_ModifiedEvent.Broadcast(this);
}

void xiiMaterialResource::SetTextureCubeBinding(const char* szName, const xiiTextureCubeResourceHandle& value)
{
  xiiTempHashedString sName(szName);

  xiiUInt32 uiIndex = xiiInvalidIndex;
  for (xiiUInt32 i = 0; i < m_mDesc.m_TextureCubeBindings.GetCount(); ++i)
  {
    if (m_mDesc.m_TextureCubeBindings[i].m_Name == sName)
    {
      uiIndex = i;
      break;
    }
  }

  if (value.IsValid())
  {
    if (uiIndex != xiiInvalidIndex)
    {
      m_mDesc.m_TextureCubeBindings[uiIndex].m_Value = value;
    }
    else
    {
      auto& binding = m_mDesc.m_TextureCubeBindings.ExpandAndGetRef();
      binding.m_Name.Assign(szName);
      binding.m_Value = value;
    }
  }
  else
  {
    if (uiIndex != xiiInvalidIndex)
    {
      m_mDesc.m_TextureCubeBindings.RemoveAtAndSwap(uiIndex);
    }
  }

  m_iLastModified.Increment();

  m_ModifiedEvent.Broadcast(this);
}

xiiTextureCubeResourceHandle xiiMaterialResource::GetTextureCubeBinding(const xiiTempHashedString& sName)
{
  auto pCachedValues = GetOrUpdateCachedValues();

  // Use pointer to prevent ref counting
  xiiTextureCubeResourceHandle* pBinding;
  if (pCachedValues->m_TextureCubeBindings.TryGetValue(sName, pBinding))
  {
    return *pBinding;
  }

  return xiiTextureCubeResourceHandle();
}

xiiRenderData::Category xiiMaterialResource::GetRenderDataCategory()
{
  auto pCachedValues = GetOrUpdateCachedValues();
  return pCachedValues->m_RenderDataCategory;
}

void xiiMaterialResource::PreserveCurrentDesc()
{
  m_mOriginalDesc = m_mDesc;
}

void xiiMaterialResource::ResetResource()
{
  if (m_mDesc != m_mOriginalDesc)
  {
    m_mDesc = m_mOriginalDesc;

    m_iLastModified.Increment();
    m_iLastConstantsModified.Increment();

    m_ModifiedEvent.Broadcast(this);
  }
}

const char* xiiMaterialResource::GetDefaultMaterialFileName(DefaultMaterialType materialType)
{
  switch (materialType)
  {
    case DefaultMaterialType::Fullbright:
      return "Base/Materials/BaseMaterials/Fullbright.xiiMaterialAsset";
    case DefaultMaterialType::FullbrightAlphaTest:
      return "Base/Materials/BaseMaterials/FullbrightAlphaTest.xiiMaterialAsset";
    case DefaultMaterialType::Lit:
      return "Base/Materials/BaseMaterials/Lit.xiiMaterialAsset";
    case DefaultMaterialType::LitAlphaTest:
      return "Base/Materials/BaseMaterials/LitAlphaTest.xiiMaterialAsset";
    case DefaultMaterialType::Sky:
      return "Base/Materials/BaseMaterials/Sky.xiiMaterialAsset";
    case DefaultMaterialType::MissingMaterial:
      return "Base/Materials/Common/MissingMaterial.xiiMaterialAsset";
    default:
      XII_ASSERT_NOT_IMPLEMENTED;
      return "";
  }
}

xiiResourceLoadDesc xiiMaterialResource::UnloadData(Unload WhatToUnload)
{
  if (m_mDesc.m_hBaseMaterial.IsValid())
  {
    xiiResourceLock<xiiMaterialResource> pBaseMaterial(m_mDesc.m_hBaseMaterial, xiiResourceAcquireMode::PointerOnly);

    auto d = xiiMakeDelegate(&xiiMaterialResource::OnBaseMaterialModified, this);
    if (pBaseMaterial->m_ModifiedEvent.HasEventHandler(d))
    {
      pBaseMaterial->m_ModifiedEvent.RemoveEventHandler(d);
    }
  }

  m_mDesc.Clear();
  m_mOriginalDesc.Clear();

  if (!m_hConstantBufferStorage.IsInvalidated())
  {
    xiiRenderContext::DeleteConstantBufferStorage(m_hConstantBufferStorage);
    m_hConstantBufferStorage.Invalidate();
  }

  DeallocateCache(m_uiCacheIndex);
  m_uiCacheIndex  = xiiInvalidIndex;
  m_pCachedValues = nullptr;

  xiiResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable    = 0;
  res.m_State                      = xiiResourceState::Unloaded;

  return res;
}

xiiResourceLoadDesc xiiMaterialResource::UpdateContent(xiiStreamReader* pOuterStream)
{
  m_mDesc.Clear();
  m_mOriginalDesc.Clear();

  xiiResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable    = 0;
  res.m_State                      = xiiResourceState::Loaded;

  if (pOuterStream == nullptr)
  {
    res.m_State = xiiResourceState::LoadedResourceMissing;
    return res;
  }

  xiiStringBuilder sAbsFilePath;
  (*pOuterStream) >> sAbsFilePath;

  if (sAbsFilePath.HasExtension("xiiMaterialBin"))
  {
    xiiStringBuilder sTemp, sTemp2;

    xiiAssetFileHeader AssetHash;
    AssetHash.Read(*pOuterStream).IgnoreResult();

    xiiUInt8 uiVersion = 0;
    (*pOuterStream) >> uiVersion;
    XII_ASSERT_DEV(uiVersion >= 4 && uiVersion <= 7, "Unknown xiiMaterialBin version {0}", uiVersion);

    xiiUInt8 uiCompressionMode = 0;
    if (uiVersion >= 6)
    {
      *pOuterStream >> uiCompressionMode;
    }

    xiiStreamReader* pInnerStream = pOuterStream;

#ifdef BUILDSYSTEM_ENABLE_ZSTD_SUPPORT
    xiiCompressedStreamReaderZstd decompressorZstd;
#endif

    switch (uiCompressionMode)
    {
      case 0:
        break;

      case 1:
#ifdef BUILDSYSTEM_ENABLE_ZSTD_SUPPORT
        decompressorZstd.SetInputStream(pOuterStream);
        pInnerStream = &decompressorZstd;
        break;
#else
        xiiLog::Error("Material resource is compressed with zstandard, but support for this compressor is not compiled in.");
        res.m_State = xiiResourceState::LoadedResourceMissing;
        return res;
#endif

      default:
        xiiLog::Error("Material resource is compressed with an unknown algorithm.");
        res.m_State = xiiResourceState::LoadedResourceMissing;
        return res;
    }

    xiiStreamReader& s = *pInnerStream;

    // Base material
    {
      s >> sTemp;

      if (!sTemp.IsEmpty())
        m_mDesc.m_hBaseMaterial = xiiResourceManager::LoadResource<xiiMaterialResource>(sTemp);
    }

    // Surface
    {
      s >> sTemp;
      m_mDesc.m_sSurface.Assign(sTemp.GetView());
    }

    // Shader
    {
      s >> sTemp;

      if (!sTemp.IsEmpty())
        m_mDesc.m_hShader = xiiResourceManager::LoadResource<xiiShaderResource>(sTemp);
    }

    // Permutation Variables
    {
      xiiUInt16 uiPermVars;
      s >> uiPermVars;

      m_mDesc.m_PermutationVars.Reserve(uiPermVars);

      for (xiiUInt16 i = 0; i < uiPermVars; ++i)
      {
        s >> sTemp;
        s >> sTemp2;

        if (!sTemp.IsEmpty() && !sTemp2.IsEmpty())
        {
          AddPermutationVar(sTemp, sTemp2);
        }
      }
    }

    // 2D Textures
    {
      xiiUInt16 uiTextures = 0;
      s >> uiTextures;

      m_mDesc.m_Texture2DBindings.Reserve(uiTextures);

      for (xiiUInt16 i = 0; i < uiTextures; ++i)
      {
        s >> sTemp;
        s >> sTemp2;

        if (!sTemp.IsEmpty() && !sTemp2.IsEmpty())
        {
          xiiMaterialResourceDescriptor::Texture2DBinding& tc = m_mDesc.m_Texture2DBindings.ExpandAndGetRef();
          tc.m_Name.Assign(sTemp.GetData());
          tc.m_Value = xiiResourceManager::LoadResource<xiiTexture2DResource>(sTemp2);
        }
      }
    }

    // Cube Textures
    {
      xiiUInt16 uiTextures = 0;
      s >> uiTextures;

      m_mDesc.m_TextureCubeBindings.Reserve(uiTextures);

      for (xiiUInt16 i = 0; i < uiTextures; ++i)
      {
        s >> sTemp;
        s >> sTemp2;

        if (!sTemp.IsEmpty() && !sTemp2.IsEmpty())
        {
          xiiMaterialResourceDescriptor::TextureCubeBinding& tc = m_mDesc.m_TextureCubeBindings.ExpandAndGetRef();
          tc.m_Name.Assign(sTemp.GetData());
          tc.m_Value = xiiResourceManager::LoadResource<xiiTextureCubeResource>(sTemp2);
        }
      }
    }

    // Shader constants
    {
      xiiUInt16 uiConstants = 0;
      s >> uiConstants;

      m_mDesc.m_Parameters.Reserve(uiConstants);

      xiiVariant vTemp;

      for (xiiUInt16 i = 0; i < uiConstants; ++i)
      {
        s >> sTemp;
        s >> vTemp;

        if (!sTemp.IsEmpty() && vTemp.IsValid())
        {
          xiiMaterialResourceDescriptor::Parameter& tc = m_mDesc.m_Parameters.ExpandAndGetRef();
          tc.m_Name.Assign(sTemp.GetData());
          tc.m_Value = vTemp;
        }
      }
    }

    // Render data category
    if (uiVersion >= 7)
    {
      xiiStringBuilder sRenderDataCategoryName;
      s >> sRenderDataCategoryName;

      xiiTempHashedString sCategoryNameHashed(sRenderDataCategoryName.GetView());
      if (sCategoryNameHashed != xiiTempHashedString("<Invalid>"))
      {
        m_mDesc.m_RenderDataCategory = xiiRenderData::FindCategory(sCategoryNameHashed);
        if (m_mDesc.m_RenderDataCategory == xiiInvalidRenderDataCategory)
        {
          xiiLog::Error("Material '{}' uses an invalid render data category '{}'", GetResourceDescription(), sRenderDataCategoryName);
        }
      }
    }

    if (uiVersion >= 5)
    {
      xiiStreamReader& s = *pInnerStream;

      xiiStringBuilder sResourceName;
      s >> sResourceName;

      xiiTextureResourceLoader::LoadedData embedded;

      while (!sResourceName.IsEmpty())
      {
        xiiUInt32 dataSize = 0;
        s >> dataSize;

        xiiTextureResourceLoader::LoadTexFile(s, embedded).IgnoreResult();
        embedded.m_bIsFallback = true;

        xiiDefaultMemoryStreamStorage storage;
        xiiMemoryStreamWriter         loadStreamWriter(&storage);
        xiiTextureResourceLoader::WriteTextureLoadStream(loadStreamWriter, embedded);

        xiiMemoryStreamReader loadStreamReader(&storage);

        xiiTexture2DResourceHandle hTexture = xiiResourceManager::LoadResource<xiiTexture2DResource>(sResourceName);
        xiiResourceManager::SetResourceLowResData(hTexture, &loadStreamReader);

        s >> sResourceName;
      }
    }
  }

  if (sAbsFilePath.HasExtension("xiiMaterial"))
  {
    xiiOpenDdlReader reader;

    if (reader.ParseDocument(*pOuterStream, 0, xiiLog::GetThreadLocalLogSystem()).Failed())
    {
      res.m_State = xiiResourceState::LoadedResourceMissing;
      return res;
    }

    const xiiOpenDdlReaderElement* pRoot = reader.GetRootElement();

    // Read the base material
    if (const xiiOpenDdlReaderElement* pBase = pRoot->FindChildOfType(xiiOpenDdlPrimitiveType::String, "BaseMaterial"))
    {
      m_mDesc.m_hBaseMaterial = xiiResourceManager::LoadResource<xiiMaterialResource>(pBase->GetPrimitivesString()[0]);
    }

    // Read the shader
    if (const xiiOpenDdlReaderElement* pShader = pRoot->FindChildOfType(xiiOpenDdlPrimitiveType::String, "Shader"))
    {
      m_mDesc.m_hShader = xiiResourceManager::LoadResource<xiiShaderResource>(pShader->GetPrimitivesString()[0]);
    }

    // Read the render data category
    if (const xiiOpenDdlReaderElement* pRenderDataCategory = pRoot->FindChildOfType(xiiOpenDdlPrimitiveType::String, "RenderDataCategory"))
    {
      m_mDesc.m_RenderDataCategory = xiiRenderData::FindCategory(xiiTempHashedString(pRenderDataCategory->GetPrimitivesString()[0]));
    }

    for (const xiiOpenDdlReaderElement* pChild = pRoot->GetFirstChild(); pChild != nullptr; pChild = pChild->GetSibling())
    {
      // Read the shader permutation variables
      if (pChild->IsCustomType("Permutation"))
      {
        const xiiOpenDdlReaderElement* pName  = pChild->FindChildOfType(xiiOpenDdlPrimitiveType::String, "Variable");
        const xiiOpenDdlReaderElement* pValue = pChild->FindChildOfType(xiiOpenDdlPrimitiveType::String, "Value");

        if (pName && pValue)
        {
          AddPermutationVar(pName->GetPrimitivesString()[0], pValue->GetPrimitivesString()[0]);
        }
      }

      // Read the shader constants
      if (pChild->IsCustomType("Constant"))
      {
        const xiiOpenDdlReaderElement* pName  = pChild->FindChildOfType(xiiOpenDdlPrimitiveType::String, "Variable");
        const xiiOpenDdlReaderElement* pValue = pChild->FindChild("Value");

        xiiVariant value;
        if (pName && pValue && xiiOpenDdlUtils::ConvertToVariant(pValue, value).Succeeded())
        {
          xiiMaterialResourceDescriptor::Parameter& sc = m_mDesc.m_Parameters.ExpandAndGetRef();
          sc.m_Name.Assign(pName->GetPrimitivesString()[0]);
          sc.m_Value = value;
        }
      }

      // Read the texture references
      if (pChild->IsCustomType("Texture2D"))
      {
        const xiiOpenDdlReaderElement* pName  = pChild->FindChildOfType(xiiOpenDdlPrimitiveType::String, "Variable");
        const xiiOpenDdlReaderElement* pValue = pChild->FindChildOfType(xiiOpenDdlPrimitiveType::String, "Value");

        if (pName && pValue)
        {
          xiiMaterialResourceDescriptor::Texture2DBinding& tc = m_mDesc.m_Texture2DBindings.ExpandAndGetRef();
          tc.m_Name.Assign(pName->GetPrimitivesString()[0]);
          tc.m_Value = xiiResourceManager::LoadResource<xiiTexture2DResource>(pValue->GetPrimitivesString()[0]);
        }
      }

      // Read the texture references
      if (pChild->IsCustomType("TextureCube"))
      {
        const xiiOpenDdlReaderElement* pName  = pChild->FindChildOfType(xiiOpenDdlPrimitiveType::String, "Variable");
        const xiiOpenDdlReaderElement* pValue = pChild->FindChildOfType(xiiOpenDdlPrimitiveType::String, "Value");

        if (pName && pValue)
        {
          xiiMaterialResourceDescriptor::TextureCubeBinding& tc = m_mDesc.m_TextureCubeBindings.ExpandAndGetRef();
          tc.m_Name.Assign(pName->GetPrimitivesString()[0]);
          tc.m_Value = xiiResourceManager::LoadResource<xiiTextureCubeResource>(pValue->GetPrimitivesString()[0]);
        }
      }
    }
  }

  if (m_mDesc.m_hBaseMaterial.IsValid())
  {
    // Block till the base material has been fully loaded to ensure that all parameters have their final value once this material is loaded.
    xiiResourceLock<xiiMaterialResource> pBaseMaterial(m_mDesc.m_hBaseMaterial, xiiResourceAcquireMode::BlockTillLoaded);

    if (!pBaseMaterial->m_ModifiedEvent.HasEventHandler(xiiMakeDelegate(&xiiMaterialResource::OnBaseMaterialModified, this)))
    {
      pBaseMaterial->m_ModifiedEvent.AddEventHandler(xiiMakeDelegate(&xiiMaterialResource::OnBaseMaterialModified, this));
    }
  }

  m_mOriginalDesc = m_mDesc;

  m_iLastModified.Increment();
  m_iLastConstantsModified.Increment();

  m_ModifiedEvent.Broadcast(this);

  return res;
}

void xiiMaterialResource::UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage)
{
  out_NewMemoryUsage.m_uiMemoryCPU = sizeof(xiiMaterialResource) + (xiiUInt32)(m_mDesc.m_PermutationVars.GetHeapMemoryUsage() + m_mDesc.m_Parameters.GetHeapMemoryUsage() + m_mDesc.m_Texture2DBindings.GetHeapMemoryUsage() + m_mDesc.m_TextureCubeBindings.GetHeapMemoryUsage() + m_mOriginalDesc.m_PermutationVars.GetHeapMemoryUsage() + m_mOriginalDesc.m_Parameters.GetHeapMemoryUsage() + m_mOriginalDesc.m_Texture2DBindings.GetHeapMemoryUsage() + m_mOriginalDesc.m_TextureCubeBindings.GetHeapMemoryUsage());

  out_NewMemoryUsage.m_uiMemoryGPU = 0;
}

XII_RESOURCE_IMPLEMENT_CREATEABLE(xiiMaterialResource, xiiMaterialResourceDescriptor)
{
  m_mDesc         = descriptor;
  m_mOriginalDesc = descriptor;

  xiiResourceLoadDesc res;
  res.m_State                      = xiiResourceState::Loaded;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable    = 0;

  if (m_mDesc.m_hBaseMaterial.IsValid())
  {
    // Can't block here for the base material since this would result in a deadlock
    xiiResourceLock<xiiMaterialResource> pBaseMaterial(m_mDesc.m_hBaseMaterial, xiiResourceAcquireMode::PointerOnly);
    pBaseMaterial->m_ModifiedEvent.AddEventHandler(xiiMakeDelegate(&xiiMaterialResource::OnBaseMaterialModified, this));
  }

  m_iLastModified.Increment();
  m_iLastConstantsModified.Increment();

  return res;
}

void xiiMaterialResource::OnBaseMaterialModified(const xiiMaterialResource* pModifiedMaterial)
{
  XII_ASSERT_DEV(m_mDesc.m_hBaseMaterial == pModifiedMaterial, "Implementation error");

  m_iLastModified.Increment();
  m_iLastConstantsModified.Increment();

  m_ModifiedEvent.Broadcast(this);
}

void xiiMaterialResource::OnResourceEvent(const xiiResourceEvent& resourceEvent)
{
  if (resourceEvent.m_Type != xiiResourceEvent::Type::ResourceContentUpdated)
    return;

  if (m_pCachedValues != nullptr && m_pCachedValues->m_hShader == resourceEvent.m_pResource)
  {
    m_iLastConstantsModified.Increment();
  }
}

void xiiMaterialResource::AddPermutationVar(xiiStringView sName, xiiStringView sValue)
{
  xiiHashedString sNameHashed;
  sNameHashed.Assign(sName);
  xiiHashedString sValueHashed;
  sValueHashed.Assign(sValue);

  if (xiiShaderManager::IsPermutationValueAllowed(sNameHashed, sValueHashed))
  {
    xiiPermutationVar& pv = m_mDesc.m_PermutationVars.ExpandAndGetRef();
    pv.m_sName            = sNameHashed;
    pv.m_sValue           = sValueHashed;
  }
}

bool xiiMaterialResource::IsModified()
{
  return m_iLastModified != m_iLastUpdated;
}

bool xiiMaterialResource::AreConstantsModified()
{
  return m_iLastConstantsModified != m_iLastConstantsUpdated;
}

void xiiMaterialResource::UpdateConstantBuffer(xiiShaderPermutationResource* pShaderPermutation)
{
  if (pShaderPermutation == nullptr)
    return;

  xiiTempHashedString                    sConstantBufferName("xiiMaterialConstants");
  const xiiGALShaderResourceDescription* pBinding = pShaderPermutation->GetShaderByteCode(xiiGALShaderType::Pixel)->GetDescription(sConstantBufferName);

  if (pBinding == nullptr)
    return;

  auto pCachedValues = GetOrUpdateCachedValues();

  m_iLastConstantsUpdated = m_iLastConstantsModified;

  if (m_hConstantBufferStorage.IsInvalidated())
  {
    m_hConstantBufferStorage = xiiRenderContext::CreateConstantBufferStorage(pBinding->m_uiTotalSize);
  }

  xiiConstantBufferStorageBase* pStorage = nullptr;
  if (xiiRenderContext::TryGetConstantBufferStorage(m_hConstantBufferStorage, pStorage))
  {
    xiiArrayPtr<xiiUInt8> data = pStorage->GetRawDataForWriting();
    if (data.GetCount() != pBinding->m_uiTotalSize)
    {
      xiiRenderContext::DeleteConstantBufferStorage(m_hConstantBufferStorage);
      m_hConstantBufferStorage = xiiRenderContext::CreateConstantBufferStorage(pBinding->m_uiTotalSize);

      XII_VERIFY(xiiRenderContext::TryGetConstantBufferStorage(m_hConstantBufferStorage, pStorage), "");
    }

    for (auto& member : pBinding->m_Variables)
    {
      if (member.m_uiOffset + xiiGALShaderPrimitiveType::GetPrimitiveTypeSize(member.m_PrimitiveType) <= data.GetCount())
      {
        xiiUInt8* pDestination = &data[member.m_uiOffset];

        xiiVariant* pValue = nullptr;
        pCachedValues->m_Parameters.TryGetValue(member.m_sName, pValue);

        xiiGALShaderVariableDescription::CopyDataFormVariant(pDestination, pValue, member);
      }
    }
  }
}

xiiMaterialResource::CachedValues* xiiMaterialResource::GetOrUpdateCachedValues()
{
  if (!IsModified())
  {
    XII_ASSERT_DEV(m_pCachedValues != nullptr, "");
    return m_pCachedValues;
  }

  xiiHybridArray<xiiMaterialResource*, 16> materialHierarchy;
  xiiMaterialResource*                     pCurrentMaterial = this;

  while (true)
  {
    materialHierarchy.PushBack(pCurrentMaterial);

    const xiiMaterialResourceHandle& hBaseMaterial = pCurrentMaterial->m_mDesc.m_hBaseMaterial;
    if (!hBaseMaterial.IsValid())
      break;

    // Ensure that the base material is loaded at this point.
    // For loaded materials this will always be the case but is still necessary for runtime created materials.
    pCurrentMaterial = xiiResourceManager::BeginAcquireResource(hBaseMaterial, xiiResourceAcquireMode::BlockTillLoaded);
  }

  XII_SCOPE_EXIT(for (xiiUInt32 i = materialHierarchy.GetCount(); i-- > 1;) {
    xiiMaterialResource* pMaterial = materialHierarchy[i];
    xiiResourceManager::EndAcquireResource(pMaterial);

    materialHierarchy[i] = nullptr;
  });

  XII_LOCK(m_UpdateCacheMutex);

  if (!IsModified())
  {
    XII_ASSERT_DEV(m_pCachedValues != nullptr, "");
    return m_pCachedValues;
  }

  m_pCachedValues = AllocateCache(m_uiCacheIndex);

  // set state of parent material first
  for (xiiUInt32 i = materialHierarchy.GetCount(); i-- > 0;)
  {
    xiiMaterialResource*                 pMaterial = materialHierarchy[i];
    const xiiMaterialResourceDescriptor& desc      = pMaterial->m_mDesc;

    if (desc.m_hShader.IsValid())
      m_pCachedValues->m_hShader = desc.m_hShader;

    for (const auto& permutationVar : desc.m_PermutationVars)
    {
      m_pCachedValues->m_PermutationVars.Insert(permutationVar.m_sName, permutationVar.m_sValue);
    }

    for (const auto& param : desc.m_Parameters)
    {
      m_pCachedValues->m_Parameters.Insert(param.m_Name, param.m_Value);
    }

    for (const auto& textureBinding : desc.m_Texture2DBindings)
    {
      m_pCachedValues->m_Texture2DBindings.Insert(textureBinding.m_Name, textureBinding.m_Value);
    }

    for (const auto& textureBinding : desc.m_TextureCubeBindings)
    {
      m_pCachedValues->m_TextureCubeBindings.Insert(textureBinding.m_Name, textureBinding.m_Value);
    }

    if (desc.m_RenderDataCategory != xiiInvalidRenderDataCategory)
    {
      m_pCachedValues->m_RenderDataCategory = desc.m_RenderDataCategory;
    }
  }

  if (m_pCachedValues->m_RenderDataCategory == xiiInvalidRenderDataCategory)
  {
    xiiHashedString sBlendModeValue;
    if (m_pCachedValues->m_PermutationVars.TryGetValue("BLEND_MODE", sBlendModeValue))
    {
      if (sBlendModeValue == xiiTempHashedString("BLEND_MODE_OPAQUE"))
      {
        m_pCachedValues->m_RenderDataCategory = xiiDefaultRenderDataCategories::LitOpaque;
      }
      else if (sBlendModeValue == xiiTempHashedString("BLEND_MODE_MASKED"))
      {
        m_pCachedValues->m_RenderDataCategory = xiiDefaultRenderDataCategories::LitMasked;
      }
      else
      {
        m_pCachedValues->m_RenderDataCategory = xiiDefaultRenderDataCategories::LitTransparent;
      }
    }
    else
    {
      m_pCachedValues->m_RenderDataCategory = xiiDefaultRenderDataCategories::LitOpaque;
    }
  }

  m_iLastUpdated = m_iLastModified;
  return m_pCachedValues;
}

namespace
{
  static xiiMutex s_MaterialCacheMutex;

  struct FreeCacheEntry
  {
    XII_DECLARE_POD_TYPE();

    xiiUInt32 m_uiIndex;
    xiiUInt64 m_uiFrame;
  };

  static xiiDynamicArray<FreeCacheEntry, xiiStaticAllocatorWrapper> s_FreeMaterialCacheEntries;
} // namespace

void xiiMaterialResource::CachedValues::Reset()
{
  m_hShader.Invalidate();
  m_PermutationVars.Clear();
  m_Parameters.Clear();
  m_Texture2DBindings.Clear();
  m_TextureCubeBindings.Clear();
  m_RenderDataCategory = xiiInvalidRenderDataCategory;
}

// static
xiiMaterialResource::CachedValues* xiiMaterialResource::AllocateCache(xiiUInt32& inout_uiCacheIndex)
{
  XII_LOCK(s_MaterialCacheMutex);

  xiiUInt32 uiOldCacheIndex = inout_uiCacheIndex;

  xiiUInt64 uiCurrentFrame = xiiRenderWorld::GetFrameCounter();
  if (!s_FreeMaterialCacheEntries.IsEmpty() && s_FreeMaterialCacheEntries[0].m_uiFrame < uiCurrentFrame)
  {
    inout_uiCacheIndex = s_FreeMaterialCacheEntries[0].m_uiIndex;
    s_FreeMaterialCacheEntries.RemoveAtAndCopy(0);
  }
  else
  {
    inout_uiCacheIndex = s_CachedValues.GetCount();
    s_CachedValues.ExpandAndGetRef();
  }

  DeallocateCache(uiOldCacheIndex);

  return &s_CachedValues[inout_uiCacheIndex];
}

// static
void xiiMaterialResource::DeallocateCache(xiiUInt32 uiCacheIndex)
{
  if (uiCacheIndex != xiiInvalidIndex)
  {
    XII_LOCK(s_MaterialCacheMutex);

    if (uiCacheIndex < s_CachedValues.GetCount())
    {
      s_CachedValues[uiCacheIndex].Reset();

      auto& freeEntry     = s_FreeMaterialCacheEntries.ExpandAndGetRef();
      freeEntry.m_uiIndex = uiCacheIndex;
      freeEntry.m_uiFrame = xiiRenderWorld::GetFrameCounter();
    }
  }
}

// static
void xiiMaterialResource::ClearCache()
{
  XII_LOCK(s_MaterialCacheMutex);

  s_CachedValues.Clear();
  s_FreeMaterialCacheEntries.Clear();
}

const xiiMaterialResourceDescriptor& xiiMaterialResource::GetCurrentDesc() const
{
  return m_mDesc;
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Material_Implementation_MaterialResource);

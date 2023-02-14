#include <RendererCore/RendererCorePCH.h>

#include <Core/Assets/AssetFileHeader.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/IO/OpenDdlReader.h>
#include <Foundation/IO/OpenDdlUtils.h>
#include <Foundation/Types/ScopeExit.h>
#include <RendererCore/Material/MaterialResource.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererCore/RenderWorld/RenderWorld.h>
#include <RendererCore/Shader/ShaderPermutationResource.h>
#include <RendererCore/ShaderCompiler/ShaderManager.h>
#include <RendererCore/Textures/Texture2DResource.h>
#include <RendererCore/Textures/TextureCubeResource.h>
#include <RendererCore/Textures/TextureLoader.h>
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
}

bool xiiMaterialResourceDescriptor::operator==(const xiiMaterialResourceDescriptor& other) const
{
  return m_hBaseMaterial == other.m_hBaseMaterial && m_hShader == other.m_hShader && m_PermutationVars == other.m_PermutationVars && m_Parameters == other.m_Parameters && m_Texture2DBindings == other.m_Texture2DBindings && m_TextureCubeBindings == other.m_TextureCubeBindings;
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiMaterialResource, 1, xiiRTTIDefaultAllocator<xiiMaterialResource>)
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_RESOURCE_IMPLEMENT_COMMON_CODE(xiiMaterialResource);
// clang-format on

// clang-format off
XII_BEGIN_SUBSYSTEM_DECLARATION(RendererCore, MaterialResource)

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

const char* xiiMaterialResource::GetDefaultMaterialFileName(DefaultMaterialType MaterialType)
{
  switch (MaterialType)
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

xiiResourceLoadDesc xiiMaterialResource::UpdateContent(xiiStreamReader* Stream)
{
  m_mDesc.Clear();
  m_mOriginalDesc.Clear();

  xiiResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable    = 0;
  res.m_State                      = xiiResourceState::Loaded;

  if (Stream == nullptr)
  {
    res.m_State = xiiResourceState::LoadedResourceMissing;
    return res;
  }

  xiiStringBuilder sAbsFilePath;
  (*Stream) >> sAbsFilePath;

  if (sAbsFilePath.HasExtension("xiiMaterialBin"))
  {
    xiiStringBuilder sTemp, sTemp2;

    xiiAssetFileHeader AssetHash;
    AssetHash.Read(*Stream).IgnoreResult();

    xiiUInt8 uiVersion = 0;
    (*Stream) >> uiVersion;
    XII_ASSERT_DEV(uiVersion <= 6, "Unknown xiiMaterialBin version {0}", uiVersion);

    xiiUInt8 uiCompressionMode = 0;
    if (uiVersion >= 6)
    {
      *Stream >> uiCompressionMode;
    }

    xiiStreamReader* pCompressor = Stream;

#ifdef BUILDSYSTEM_ENABLE_ZSTD_SUPPORT
    xiiCompressedStreamReaderZstd decompressorZstd;
#endif

    switch (uiCompressionMode)
    {
      case 0:
        break;

      case 1:
#ifdef BUILDSYSTEM_ENABLE_ZSTD_SUPPORT
        decompressorZstd.SetInputStream(Stream);
        pCompressor = &decompressorZstd;
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

    xiiStreamReader& stream = *pCompressor;

    // Base material
    {
      stream >> sTemp;

      if (!sTemp.IsEmpty())
        m_mDesc.m_hBaseMaterial = xiiResourceManager::LoadResource<xiiMaterialResource>(sTemp);
    }

    if (uiVersion >= 4)
    {
      stream >> sTemp;

      if (!sTemp.IsEmpty())
      {
        m_mDesc.m_sSurface.Assign(sTemp.GetData());
      }
    }

    // Shader
    {
      stream >> sTemp;

      if (!sTemp.IsEmpty())
        m_mDesc.m_hShader = xiiResourceManager::LoadResource<xiiShaderResource>(sTemp);
    }

    // Permutation Variables
    {
      xiiUInt16 uiPermVars;
      stream >> uiPermVars;

      m_mDesc.m_PermutationVars.Reserve(uiPermVars);

      for (xiiUInt16 i = 0; i < uiPermVars; ++i)
      {
        stream >> sTemp;
        stream >> sTemp2;

        if (!sTemp.IsEmpty() && !sTemp2.IsEmpty())
        {
          AddPermutationVar(sTemp, sTemp2);
        }
      }
    }

    // 2D Textures
    {
      xiiUInt16 uiTextures = 0;
      stream >> uiTextures;

      m_mDesc.m_Texture2DBindings.Reserve(uiTextures);

      for (xiiUInt16 i = 0; i < uiTextures; ++i)
      {
        stream >> sTemp;
        stream >> sTemp2;

        if (sTemp.IsEmpty() || sTemp2.IsEmpty())
          continue;

        xiiMaterialResourceDescriptor::Texture2DBinding& tc = m_mDesc.m_Texture2DBindings.ExpandAndGetRef();
        tc.m_Name.Assign(sTemp.GetData());
        tc.m_Value = xiiResourceManager::LoadResource<xiiTexture2DResource>(sTemp2);
      }
    }

    // Cube Textures
    if (uiVersion >= 3)
    {
      xiiUInt16 uiTextures = 0;
      stream >> uiTextures;

      m_mDesc.m_TextureCubeBindings.Reserve(uiTextures);

      for (xiiUInt16 i = 0; i < uiTextures; ++i)
      {
        stream >> sTemp;
        stream >> sTemp2;

        if (sTemp.IsEmpty() || sTemp2.IsEmpty())
          continue;

        xiiMaterialResourceDescriptor::TextureCubeBinding& tc = m_mDesc.m_TextureCubeBindings.ExpandAndGetRef();
        tc.m_Name.Assign(sTemp.GetData());
        tc.m_Value = xiiResourceManager::LoadResource<xiiTextureCubeResource>(sTemp2);
      }
    }


    if (uiVersion >= 2)
    {
      // Shader constants

      xiiUInt16 uiConstants = 0;

      stream >> uiConstants;

      m_mDesc.m_Parameters.Reserve(uiConstants);

      xiiVariant vTemp;

      for (xiiUInt16 i = 0; i < uiConstants; ++i)
      {
        stream >> sTemp;
        stream >> vTemp;

        if (sTemp.IsEmpty() || !vTemp.IsValid())
          continue;

        xiiMaterialResourceDescriptor::Parameter& tc = m_mDesc.m_Parameters.ExpandAndGetRef();
        tc.m_Name.Assign(sTemp.GetData());
        tc.m_Value = vTemp;
      }
    }

    if (uiVersion >= 5)
    {
      auto& s = *Stream;

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
    xiiStringBuilder tmp, tmp2;
    xiiOpenDdlReader reader;

    if (reader.ParseDocument(*Stream, 0, xiiLog::GetThreadLocalLogSystem()).Failed())
    {
      res.m_State = xiiResourceState::LoadedResourceMissing;
      return res;
    }

    const xiiOpenDdlReaderElement* pRoot = reader.GetRootElement();

    const xiiOpenDdlReaderElement* pBase   = pRoot->FindChildOfType(xiiOpenDdlPrimitiveType::String, "BaseMaterial");
    const xiiOpenDdlReaderElement* pshader = pRoot->FindChildOfType(xiiOpenDdlPrimitiveType::String, "Shader");

    // Read the base material
    if (pBase)
    {
      tmp                     = pBase->GetPrimitivesString()[0];
      m_mDesc.m_hBaseMaterial = xiiResourceManager::LoadResource<xiiMaterialResource>(tmp);
    }

    // Read the shader
    if (pshader)
    {
      tmp               = pshader->GetPrimitivesString()[0];
      m_mDesc.m_hShader = xiiResourceManager::LoadResource<xiiShaderResource>(tmp);
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
          tmp  = pName->GetPrimitivesString()[0];
          tmp2 = pValue->GetPrimitivesString()[0];

          AddPermutationVar(tmp, tmp2);
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

          tmp = pName->GetPrimitivesString()[0];
          sc.m_Name.Assign(tmp.GetData());

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

          tmp = pName->GetPrimitivesString()[0];
          tc.m_Name.Assign(tmp.GetData());

          tmp        = pValue->GetPrimitivesString()[0];
          tc.m_Value = xiiResourceManager::LoadResource<xiiTexture2DResource>(tmp);
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

          tmp = pName->GetPrimitivesString()[0];
          tc.m_Name.Assign(tmp.GetData());

          tmp        = pValue->GetPrimitivesString()[0];
          tc.m_Value = xiiResourceManager::LoadResource<xiiTextureCubeResource>(tmp);
        }
      }
    }
  }

  if (m_mDesc.m_hBaseMaterial.IsValid())
  {
    // Block till the base material has been fully loaded to ensure that all parameters have their final value once this material is loaded.
    xiiResourceLock<xiiMaterialResource> pBaseMaterial(m_mDesc.m_hBaseMaterial, xiiResourceAcquireMode::BlockTillLoaded);
    pBaseMaterial->m_ModifiedEvent.AddEventHandler(xiiMakeDelegate(&xiiMaterialResource::OnBaseMaterialModified, this));
  }

  m_mOriginalDesc = m_mDesc;

  m_iLastModified.Increment();
  m_iLastConstantsModified.Increment();

  m_ModifiedEvent.Broadcast(this);

  return res;
}

void xiiMaterialResource::UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage)
{
  out_NewMemoryUsage.m_uiMemoryCPU =
    sizeof(xiiMaterialResource) + (xiiUInt32)(m_mDesc.m_PermutationVars.GetHeapMemoryUsage() + m_mDesc.m_Parameters.GetHeapMemoryUsage() + m_mDesc.m_Texture2DBindings.GetHeapMemoryUsage() + m_mDesc.m_TextureCubeBindings.GetHeapMemoryUsage() + m_mOriginalDesc.m_PermutationVars.GetHeapMemoryUsage() + m_mOriginalDesc.m_Parameters.GetHeapMemoryUsage() + m_mOriginalDesc.m_Texture2DBindings.GetHeapMemoryUsage() + m_mOriginalDesc.m_TextureCubeBindings.GetHeapMemoryUsage());

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

void xiiMaterialResource::AddPermutationVar(const char* szName, const char* szValue)
{
  xiiHashedString sName;
  sName.Assign(szName);
  xiiHashedString sValue;
  sValue.Assign(szValue);

  if (xiiShaderManager::IsPermutationValueAllowed(sName, sValue))
  {
    xiiPermutationVar& pv = m_mDesc.m_PermutationVars.ExpandAndGetRef();
    pv.m_sName            = sName;
    pv.m_sValue           = sValue;
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

  xiiTempHashedString             sConstantBufferName("xiiMaterialConstants");
  const xiiShaderResourceBinding* pBinding = pShaderPermutation->GetShaderStageBinary(xiiGALShaderStage::PixelShader)->GetShaderResourceBinding(sConstantBufferName);
  if (pBinding == nullptr)
  {
    pBinding = pShaderPermutation->GetShaderStageBinary(xiiGALShaderStage::VertexShader)->GetShaderResourceBinding(sConstantBufferName);
  }

  const xiiShaderConstantBufferLayout* pLayout = pBinding != nullptr ? pBinding->m_pLayout : nullptr;
  if (pLayout == nullptr)
    return;

  auto pCachedValues = GetOrUpdateCachedValues();

  m_iLastConstantsUpdated = m_iLastConstantsModified;

  if (m_hConstantBufferStorage.IsInvalidated())
  {
    m_hConstantBufferStorage = xiiRenderContext::CreateConstantBufferStorage(pLayout->m_uiTotalSize, XII_STRINGIZE(xiiShaderConstantBufferLayout));
  }

  xiiConstantBufferStorageBase* pStorage = nullptr;
  if (xiiRenderContext::TryGetConstantBufferStorage(m_hConstantBufferStorage, pStorage))
  {
    xiiArrayPtr<xiiUInt8> data = pStorage->GetRawDataForWriting();
    if (data.GetCount() != pLayout->m_uiTotalSize)
    {
      xiiRenderContext::DeleteConstantBufferStorage(m_hConstantBufferStorage);
      m_hConstantBufferStorage = xiiRenderContext::CreateConstantBufferStorage(pLayout->m_uiTotalSize, XII_STRINGIZE(xiiShaderConstantBufferLayout));

      XII_VERIFY(xiiRenderContext::TryGetConstantBufferStorage(m_hConstantBufferStorage, pStorage), "");
    }

    for (auto& constant : pLayout->m_Constants)
    {
      if (constant.m_uiOffset + xiiShaderConstantBufferLayout::Constant::s_TypeSize[constant.m_Type.GetValue()] <= data.GetCount())
      {
        xiiUInt8* pDest = &data[constant.m_uiOffset];

        xiiVariant* pValue = nullptr;
        pCachedValues->m_Parameters.TryGetValue(constant.m_sName, pValue);

        constant.CopyDataFormVariant(pDest, pValue);
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

  m_pCachedValues = AllocateCache();

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

xiiMaterialResource::CachedValues* xiiMaterialResource::AllocateCache()
{
  XII_LOCK(s_MaterialCacheMutex);

  xiiUInt32 uiOldCacheIndex = m_uiCacheIndex;

  xiiUInt64 uiCurrentFrame = xiiRenderWorld::GetFrameCounter();
  if (!s_FreeMaterialCacheEntries.IsEmpty() && s_FreeMaterialCacheEntries[0].m_uiFrame < uiCurrentFrame)
  {
    m_uiCacheIndex = s_FreeMaterialCacheEntries[0].m_uiIndex;
    s_FreeMaterialCacheEntries.RemoveAtAndCopy(0);
  }
  else
  {
    m_uiCacheIndex = s_CachedValues.GetCount();
    s_CachedValues.ExpandAndGetRef();
  }

  DeallocateCache(uiOldCacheIndex);

  return &s_CachedValues[m_uiCacheIndex];
}

void xiiMaterialResource::DeallocateCache(xiiUInt32 uiCacheIndex)
{
  if (uiCacheIndex != xiiInvalidIndex)
  {
    XII_LOCK(s_MaterialCacheMutex);

    if (uiCacheIndex < s_CachedValues.GetCount())
    {
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

XII_STATICLINK_FILE(RendererCore, RendererCore_Material_Implementation_MaterialResource);

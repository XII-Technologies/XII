/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>

#include <Core/ResourceManager/ResourceManager.h>
#include <GraphicsCore/Material/MaterialShaderBinding.h>
#include <GraphicsCore/Textures/Texture2DResource.h>
#include <GraphicsCore/Textures/TextureCubeResource.h>
#include <GraphicsFoundation/CommandEncoder/CommandList.h>
#include <Shaders/Pipeline/Passes/Geometry/GpuSceneGBufferConstants.h>

namespace
{
  xiiStatus ValidateSceneBinding(const xiiGALShaderResourceDescription& resource, xiiGALShaderType::Enum stage, bool transparent, bool& known)
  {
    const auto name   = resource.m_sName.GetString();
    auto       type   = xiiGALShaderResourceType::Unknown;
    auto       stages = xiiGALShaderType::Vertex | xiiGALShaderType::Pixel;
    bool       array  = false;
    if (name == "xiiNativeMaterialViewConstants" || name == "xiiGpuSceneGBufferConstants")
      type = xiiGALShaderResourceType::ConstantBuffer;
    else if (name == "g_SceneInstances")
      type = xiiGALShaderResourceType::BufferSRV;
    else if (name == "g_SkinningMatrices" || name == "g_Geometry" || name == "g_Meshlets" || name == "g_VisibleMeshlets" || name == "g_Buffers")
    {
      type   = xiiGALShaderResourceType::BufferSRV;
      stages = xiiGALShaderType::Vertex;
      array  = name == "g_Buffers";
    }
    else if (name == "g_SurfaceMaterials")
    {
      type   = xiiGALShaderResourceType::BufferSRV;
      stages = xiiGALShaderType::Pixel;
    }
    else if (name == "g_Textures")
    {
      type   = xiiGALShaderResourceType::TextureSRV;
      stages = xiiGALShaderType::Pixel;
      array  = true;
    }
    else if (name == "g_MaterialSamplers")
    {
      type   = xiiGALShaderResourceType::Sampler;
      stages = xiiGALShaderType::Pixel;
      array  = true;
    }
    else if (name == "g_sLinearWrap" || name == "g_sAnisotropicWrap")
      type = xiiGALShaderResourceType::Sampler;
    else if (transparent)
    {
      stages = xiiGALShaderType::Pixel;
      for (auto key : {"PerFrameCamera", "xiiEnvironmentLightConstants", "xiiLightClusterConstants", "xiiTransparentRasterConstants", "xiiForwardRefractionConstants", "xiiShadowCascadeConstants", "xiiSpotShadowConstants", "xiiAtmosphereConstants"})
        if (name == key)
          type = xiiGALShaderResourceType::ConstantBuffer;
      for (auto key : {"g_Lights", "g_Counts", "g_Candidates", "g_IESProfiles", "g_LightGrid", "g_LightIndices", "g_Overflow"})
        if (name == key)
          type = xiiGALShaderResourceType::BufferSRV;
      for (auto key : {"g_RefractionDepth", "g_RefractionBackground", "g_ShadowAtlas", "g_SpotShadowAtlas", "g_Transmittance", "g_MultiScatter"})
        if (name == key)
          type = xiiGALShaderResourceType::TextureSRV;
      if (name == "LinearClampSampler" || name == "g_sRefractionLinearClamp")
        type = xiiGALShaderResourceType::Sampler;
    }
    known = type != xiiGALShaderResourceType::Unknown;
    if (known && (resource.m_Type != type || !stages.IsSet(stage) || (!array && resource.m_uiArraySize != 1)))
      return xiiStatus(xiiFmt("Native surface binding '{}' has an incompatible type, array count or stage.", name));
    return XII_SUCCESS;
  }

  xiiResult ParameterType(const xiiGALShaderVariableDescription& variable, xiiMaterialParameterType::Enum& type, xiiUInt32& bytes)
  {
    if (variable.m_uiArraySize > 1 || !variable.m_Members.IsEmpty())
      return XII_FAILURE;
    const auto columns = variable.m_uiColumnCount, rows = variable.m_uiRowCount;
    bytes = columns * 4;
    if (rows == 1 && columns == 1)
    {
      switch (variable.m_PrimitiveType)
      {
        case xiiGALShaderPrimitiveType::Bool:
          type = xiiMaterialParameterType::Bool;
          return XII_SUCCESS;
        case xiiGALShaderPrimitiveType::Int32:
          type = xiiMaterialParameterType::Int;
          return XII_SUCCESS;
        case xiiGALShaderPrimitiveType::UInt32:
          type = xiiMaterialParameterType::UInt;
          return XII_SUCCESS;
        default:
          break;
      }
    }
    if (variable.m_PrimitiveType != xiiGALShaderPrimitiveType::Float32)
      return XII_FAILURE;
    if (rows == 1 && columns >= 1 && columns <= 4)
    {
      type = static_cast<xiiMaterialParameterType::Enum>(xiiMaterialParameterType::Float + columns - 1);
      return XII_SUCCESS;
    }
    if (rows == columns && (rows == 3 || rows == 4) && variable.m_Class == xiiGALShaderVariableClassType::MatrixColumns)
    {
      type  = rows == 3 ? xiiMaterialParameterType::Matrix3 : xiiMaterialParameterType::Matrix4;
      bytes = rows * 16;
      return XII_SUCCESS;
    }
    return XII_FAILURE;
  }
} // namespace

xiiStatus xiiMaterialShaderBinding::Build(const xiiMaterialInstanceSnapshot& snapshot, const xiiShaderPermutationResource& shader, bool bTransparent, bool bPicking)
{
  *this = {};
  if (!snapshot.m_pSchema || snapshot.m_pSchema->GetDomain() != xiiMaterialDomain::Surface || (!bPicking && snapshot.m_RuntimeState.IsTranslucent() != bTransparent))
    return xiiStatus("The authored surface shader does not match this raster pass's material domain or blend class.");
  if (shader.GetActiveShaderStages() != (xiiGALShaderType::Vertex | xiiGALShaderType::Pixel))
    return xiiStatus("Native authored surfaces require vertex and pixel stages with no vertex stream inputs.");
  const auto* vertex = shader.GetShaderByteCode(xiiGALShaderType::Vertex);
  if (!vertex || !vertex->m_VertexInputLayout.IsEmpty() || !vertex->GetDescription("g_SceneInstances") || !vertex->GetDescription("g_VisibleMeshlets"))
    return xiiStatus("The vertex shader must consume the native GPU-scene surface interface.");
  const auto depth = shader.GetDepthStencilState()->GetDescription();
  if (!depth.m_bDepthEnable || depth.m_bDepthWriteEnable || depth.m_ComparisonDepthFunction != xiiGALComparisonFunction::GreaterEqual)
    return xiiStatus("Native G-buffer shaders require read-only reverse-Z depth (GreaterEqual).");
  for (const auto& target : shader.GetBlendState()->GetDescription().m_RenderTargets)
    if (!bTransparent && target.m_bBlendEnable)
      return xiiStatus("Native G-buffer shaders cannot blend their material targets.");

  const xiiGALShaderResourceDescription* constantsLayout = nullptr;
  for (auto stage : {xiiGALShaderType::Vertex, xiiGALShaderType::Pixel})
  {
    for (const auto& resource : shader.GetShaderByteCode(stage)->m_ShaderResourceBindings)
    {
      const auto name         = resource.m_sName.GetString();
      bool       sceneBinding = false;
      XII_SUCCEED_OR_RETURN(ValidateSceneBinding(resource, stage, bTransparent, sceneBinding));
      if (name == "xiiMaterialConstants")
      {
        if (resource.m_Type != xiiGALShaderResourceType::ConstantBuffer || resource.m_uiTotalSize == 0 || resource.m_uiTotalSize > 65536)
          return xiiStatus("Invalid reflected material constant buffer.");
        if (!m_Constants.IsEmpty() && m_Constants.GetCount() != resource.m_uiTotalSize)
          return xiiStatus("Material constants must have the same layout in all consuming stages.");
        if (constantsLayout)
        {
          if (constantsLayout->m_Variables.GetCount() != resource.m_Variables.GetCount())
            return xiiStatus("Material constant layouts disagree across shader stages.");
          for (xiiUInt32 i = 0; i < resource.m_Variables.GetCount(); ++i)
          {
            const auto& a = constantsLayout->m_Variables[i];
            const auto& b = resource.m_Variables[i];
            if (a.m_sName != b.m_sName || a.m_Class != b.m_Class || a.m_PrimitiveType != b.m_PrimitiveType || a.m_uiOffset != b.m_uiOffset || a.m_uiArraySize != b.m_uiArraySize || a.m_uiRowCount != b.m_uiRowCount || a.m_uiColumnCount != b.m_uiColumnCount)
              return xiiStatus("Material constant layouts disagree across shader stages.");
          }
        }
        constantsLayout = &resource;
        xiiDynamicArray<xiiUInt8> packed;
        packed.SetCount(resource.m_uiTotalSize, 0);
        for (const auto& variable : resource.m_Variables)
        {
          xiiMaterialParameterType::Enum type;
          xiiUInt32                      bytes = 0;
          if (ParameterType(variable, type, bytes).Failed() || variable.m_uiOffset > packed.GetCount() || bytes > packed.GetCount() - variable.m_uiOffset)
            return xiiStatus(xiiFmt("Unsupported material constant '{}' layout.", variable.m_sName));
          const auto* parameter = snapshot.m_pSchema->FindParameter(variable.m_sName);
          if (!parameter)
            continue; // Unassigned reflected parameters have zero defaults.
          if (parameter->m_Type != type && !(type == xiiMaterialParameterType::Float4 && parameter->m_Type == xiiMaterialParameterType::Color))
            return xiiStatus(xiiFmt("Material parameter '{}' has an incompatible reflected type.", variable.m_sName));
          if (parameter->m_uiOffset > snapshot.m_ParameterData.GetCount() || bytes > snapshot.m_ParameterData.GetCount() - parameter->m_uiOffset)
            return xiiStatus("Invalid native material parameter snapshot.");
          xiiMemoryUtils::Copy(packed.GetData() + variable.m_uiOffset, snapshot.m_ParameterData.GetData() + parameter->m_uiOffset, bytes);
        }
        if (!m_Constants.IsEmpty() && packed != m_Constants)
          return xiiStatus("Material constant layouts disagree across shader stages.");
        m_Constants = std::move(packed);
        m_ConstantStages |= stage;
      }
      else if (sceneBinding)
      {
        if (name == "xiiGpuSceneGBufferConstants" && (resource.m_Type != xiiGALShaderResourceType::ConstantBuffer || resource.m_uiTotalSize != sizeof(xiiGpuSceneGBufferConstants)))
          return xiiStatus("The shader uses an incompatible native surface camera layout.");
      }
      else if (resource.m_Type == xiiGALShaderResourceType::TextureSRV && resource.m_uiArraySize == 1)
      {
        const auto                 index = snapshot.m_pSchema->FindTextureIndex(xiiMaterialParameterId::Make(name));
        xiiMaterialResourceBinding binding;
        if (index < snapshot.m_ResourceBindings.GetCount())
          binding = snapshot.m_ResourceBindings[index];
        const auto* definition = snapshot.m_pSchema->FindTexture(xiiMaterialParameterId::Make(name));
        if (!binding.m_hTexture2D.IsValid() && !binding.m_hTextureCube.IsValid())
        {
          if (definition && definition->m_bRequired)
            return xiiStatus(xiiFmt("Required material texture '{}' is unbound.", name));
          // Use the engine's shared missing-resource texture for optional empty
          // slots. No per-material GPU allocation or descriptor placeholder.
          if (resource.m_TextureType == xiiGALShaderTextureType::Texture2D)
            binding.m_hTexture2D = xiiResourceManager::GetResourceTypeMissingFallback<xiiTexture2DResource>();
          else if (resource.m_TextureType == xiiGALShaderTextureType::TextureCube)
            binding.m_hTextureCube = xiiResourceManager::GetResourceTypeMissingFallback<xiiTextureCubeResource>();
        }
        Texture texture;
        texture.m_Name         = resource.m_sName;
        texture.m_hTexture2D   = binding.m_hTexture2D;
        texture.m_hTextureCube = binding.m_hTextureCube;
        if (resource.m_TextureType == xiiGALShaderTextureType::Texture2D && binding.m_hTexture2D.IsValid())
        {
          xiiResourceLock<xiiTexture2DResource> loaded(binding.m_hTexture2D, xiiResourceAcquireMode::BlockTillLoaded);
          texture.m_Texture = loaded->GetGALTexture();
          texture.m_Sampler = loaded->GetGALSampler();
        }
        else if (resource.m_TextureType == xiiGALShaderTextureType::TextureCube && binding.m_hTextureCube.IsValid())
        {
          xiiResourceLock<xiiTextureCubeResource> loaded(binding.m_hTextureCube, xiiResourceAcquireMode::BlockTillLoaded);
          texture.m_Texture = loaded->GetGALTexture();
          texture.m_Sampler = loaded->GetGALSampler();
        }
        if (!texture.m_Texture || !texture.m_Sampler)
          return xiiStatus(xiiFmt("Material texture '{}' has an incompatible or unavailable resource.", name));
        m_Textures.PushBack(std::move(texture));
      }
      else if (resource.m_Type == xiiGALShaderResourceType::Sampler)
      {
        xiiStringBuilder textureName(name);
        if (!textureName.TrimWordEnd("_AutoSampler") || resource.m_uiArraySize != 1)
          return xiiStatus(xiiFmt("Material sampler '{}' must belong to a reflected texture.", name));
        const auto* texture = shader.GetShaderByteCode(stage)->GetDescription(xiiTempHashedString(textureName));
        if (!texture || texture->m_Type != xiiGALShaderResourceType::TextureSRV || texture->m_uiArraySize != 1)
          return xiiStatus(xiiFmt("Material sampler '{}' has no matching texture.", name));
      }
      else
        return xiiStatus(xiiFmt("Unsupported authored surface binding '{}'.", name));
    }
  }
  m_Vertex     = shader.GetGALShader(xiiGALShaderType::Vertex);
  m_Pixel      = shader.GetGALShader(xiiGALShaderType::Pixel);
  m_Signature  = shader.GetPipelineResourceSignature();
  m_Blend      = shader.GetBlendState();
  m_Depth      = shader.GetDepthStencilState();
  m_Rasterizer = shader.GetRasterizerState();
  return XII_SUCCESS;
}

void xiiMaterialShaderBinding::Bind(xiiGALCommandList& commands, xiiGALBuffer* constants) const
{
  if (constants)
    commands.ResolveAndSetConstantBuffer("xiiMaterialConstants", constants);
  for (const auto& texture : m_Textures)
  {
    commands.ResolveAndSetShaderResourceTextureView(texture.m_Name, texture.m_Texture->GetDefaultView(xiiGALTextureViewType::ShaderResource));
    const xiiStringBuilder sampler(texture.m_Name.GetString(), "_AutoSampler");
    commands.ResolveAndSetSampler(xiiTempHashedString(sampler), texture.m_Sampler.Borrow());
  }
}

bool xiiMaterialShaderBinding::AreTexturesCurrent() const
{
  for (const auto& texture : m_Textures)
  {
    if (texture.m_hTexture2D.IsValid())
    {
      xiiResourceLock<xiiTexture2DResource> loaded(texture.m_hTexture2D, xiiResourceAcquireMode::BlockTillLoaded);
      if (texture.m_Texture != loaded->GetGALTexture() || texture.m_Sampler != loaded->GetGALSampler())
        return false;
    }
    else if (texture.m_hTextureCube.IsValid())
    {
      xiiResourceLock<xiiTextureCubeResource> loaded(texture.m_hTextureCube, xiiResourceAcquireMode::BlockTillLoaded);
      if (texture.m_Texture != loaded->GetGALTexture() || texture.m_Sampler != loaded->GetGALSampler())
        return false;
    }
  }
  return true;
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Material_Implementation_MaterialShaderBinding);

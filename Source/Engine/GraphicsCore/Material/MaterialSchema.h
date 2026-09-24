/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Containers/HashTable.h>
#include <Foundation/Strings/HashedString.h>
#include <Foundation/Types/Variant.h>
#include <GraphicsCore/Material/MaterialTypes.h>
#include <GraphicsCore/Shader/ShaderResource.h>
#include <GraphicsFoundation/Shader/ShaderByteCode.h>

/// Describes one typed value in a material parameter block.
struct XII_GRAPHICSCORE_DLL xiiMaterialParameterDefinition
{
  xiiMaterialParameterId                  m_Id;
  xiiHashedString                         m_sName;
  xiiString                               m_sDisplayName;
  xiiString                               m_sCategory;
  xiiEnum<xiiMaterialParameterType>       m_Type            = xiiMaterialParameterType::Float;
  xiiEnum<xiiMaterialUpdateFrequency>     m_UpdateFrequency = xiiMaterialUpdateFrequency::Static;
  xiiBitflags<xiiMaterialParameterFlags>  m_Flags           = xiiMaterialParameterFlags::Default;
  xiiVariant                              m_DefaultValue;
  xiiVariant                              m_MinValue;
  xiiVariant                              m_MaxValue;
  xiiUInt32                               m_uiOffset = 0U;
  xiiUInt32                               m_uiSize   = 0U;
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiMaterialParameterDefinition);

/// Describes one texture binding or bindless texture index in a material.
struct XII_GRAPHICSCORE_DLL xiiMaterialTextureDefinition
{
  xiiMaterialParameterId              m_Id;
  xiiHashedString                     m_sName;
  xiiString                           m_sDisplayName;
  xiiEnum<xiiGALShaderTextureType>    m_TextureType     = xiiGALShaderTextureType::Texture2D;
  xiiEnum<xiiMaterialUpdateFrequency> m_UpdateFrequency = xiiMaterialUpdateFrequency::Static;
  xiiUInt32                           m_uiBindingSlot    = xiiInvalidIndex;
  bool                                m_bRequired        = false;
  bool                                m_bBindless        = true;
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiMaterialTextureDefinition);

/// Authoring description. Build() turns this into a validated immutable schema.
struct XII_GRAPHICSCORE_DLL xiiMaterialSchemaDescription
{
  xiiString                           m_sName;
  xiiShaderResourceHandle             m_hShader;
  xiiEnum<xiiMaterialDomain>          m_Domain       = xiiMaterialDomain::Surface;
  xiiEnum<xiiMaterialShadingModel>    m_ShadingModel = xiiMaterialShadingModel::Lit;
  xiiUInt32                           m_uiVersion     = 1U;
  xiiDynamicArray<xiiMaterialParameterDefinition> m_Parameters;
  xiiDynamicArray<xiiMaterialTextureDefinition>   m_Textures;

  xiiMaterialParameterDefinition& AddParameter(xiiStringView sName, xiiMaterialParameterType::Enum type, const xiiVariant& defaultValue = {});
  xiiMaterialTextureDefinition&   AddTexture(xiiStringView sName, xiiGALShaderTextureType::Enum textureType = xiiGALShaderTextureType::Texture2D);
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiMaterialSchemaDescription);

/// Validated, densely packed contract shared by material resources and runtime instances.
///
/// Schemas are immutable after Build(). Stable IDs remove hashed-string lookup from hot paths,
/// while retaining names and reflected metadata for editors and diagnostics.
class XII_GRAPHICSCORE_DLL xiiMaterialSchema
{
public:
  xiiResult Build(const xiiMaterialSchemaDescription& description, xiiStringBuilder* out_pError = nullptr);
  void      Clear();

  [[nodiscard]] bool IsValid() const { return m_bValid; }

  [[nodiscard]] const xiiString& GetName() const { return m_sName; }
  [[nodiscard]] const xiiShaderResourceHandle& GetShader() const { return m_hShader; }
  [[nodiscard]] xiiEnum<xiiMaterialDomain> GetDomain() const { return m_Domain; }
  [[nodiscard]] xiiEnum<xiiMaterialShadingModel> GetShadingModel() const { return m_ShadingModel; }
  [[nodiscard]] xiiUInt32 GetVersion() const { return m_uiVersion; }
  [[nodiscard]] xiiUInt32 GetParameterBlockSize() const { return m_uiParameterBlockSize; }
  [[nodiscard]] xiiUInt64 GetLayoutHash() const { return m_uiLayoutHash; }

  [[nodiscard]] xiiArrayPtr<const xiiMaterialParameterDefinition> GetParameters() const { return m_Parameters; }
  [[nodiscard]] xiiArrayPtr<const xiiMaterialTextureDefinition> GetTextures() const { return m_Textures; }

  [[nodiscard]] const xiiMaterialParameterDefinition* FindParameter(xiiMaterialParameterId id) const;
  [[nodiscard]] const xiiMaterialParameterDefinition* FindParameter(const xiiTempHashedString& sName) const;
  [[nodiscard]] const xiiMaterialTextureDefinition* FindTexture(xiiMaterialParameterId id) const;
  [[nodiscard]] const xiiMaterialTextureDefinition* FindTexture(const xiiTempHashedString& sName) const;

  static xiiUInt32 GetPackedSize(xiiMaterialParameterType::Enum type);
  static xiiUInt32 GetPackedAlignment(xiiMaterialParameterType::Enum type);
  static bool      IsValueCompatible(xiiMaterialParameterType::Enum type, const xiiVariant& value);

private:
  xiiString                           m_sName;
  xiiShaderResourceHandle             m_hShader;
  xiiEnum<xiiMaterialDomain>          m_Domain       = xiiMaterialDomain::Surface;
  xiiEnum<xiiMaterialShadingModel>    m_ShadingModel = xiiMaterialShadingModel::Lit;
  xiiUInt32                           m_uiVersion     = 0U;
  xiiUInt32                           m_uiParameterBlockSize = 0U;
  xiiUInt64                           m_uiLayoutHash         = 0U;
  xiiDynamicArray<xiiMaterialParameterDefinition> m_Parameters;
  xiiDynamicArray<xiiMaterialTextureDefinition>   m_Textures;
  xiiHashTable<xiiUInt64, xiiUInt32>               m_ParameterLookup;
  xiiHashTable<xiiUInt64, xiiUInt32>               m_TextureLookup;
  bool                                               m_bValid = false;
};


/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GraphicsFoundation/GraphicsFoundationDLL.h>

#include <GraphicsFoundation/Shader/ShaderByteCode.h>
#include <GraphicsFoundation/ShaderCompiler/Descriptors.h>
#include <GraphicsFoundation/ShaderCompiler/ShaderTextSectionizer.h>

class xiiPropertyAttribute;

class XII_GRAPHICSFOUNDATION_DLL xiiGALShaderParser
{
public:
  struct AttributeDefinition
  {
    xiiString                      m_sType;
    xiiHybridArray<xiiVariant, 8U> m_Values;
  };

  struct ParameterDefinition
  {
    const xiiRTTI* m_pType = nullptr;
    xiiString      m_sType;
    xiiString      m_sName;

    xiiHybridArray<AttributeDefinition, 4U> m_Attributes;
  };

  struct EnumValue
  {
    xiiHashedString m_sValueName;
    xiiInt32        m_iValueValue = 0;
  };

  struct EnumDefinition
  {
    xiiString                      m_sName;
    xiiUInt32                      m_uiDefaultValue = 0U;
    xiiHybridArray<EnumValue, 16U> m_Values;
  };

  static xiiResult PreprocessSection(xiiStreamReader& inout_stream, xiiEnum<xiiGALShaderSections> section, xiiArrayPtr<xiiString> pCustomDefines, xiiStringBuilder& out_sResult);

  static void ParseMaterialParameterSection(xiiStreamReader& inout_stream, xiiHybridArray<ParameterDefinition, 16>& out_parameter, xiiHybridArray<EnumDefinition, 4>& out_enumDefinitions);

  static void ParsePermutationSection(xiiStringView sPermutationSection, xiiHybridArray<xiiHashedString, 16>& out_permutationVariables, xiiHybridArray<xiiGALPermutationVariable, 16>& out_fixedPermutationVariables);

  static void ParsePermutationVariableConfiguration(xiiStringView sPermutationVarConfig, xiiVariant& out_defaultValue, EnumDefinition& out_enumDefinition);

  /// Tries to find shader resource declarations inside the shader source.
  ///
  /// Used by the shader compiler implementations to generate resource mappings to sets/slots without creating conflicts across shader stages. For a list of supported resource declarations and possible pitfalls, please refer to https://docs.xiitechnologies.com/graphics/shaders/shader-resources.html.
  /// \param sShaderStageSource The shader source to parse.
  /// \param out_Resources The shader resources found inside the source.
  static void ParseShaderResources(xiiStringView sShaderStageSource, xiiDynamicArray<xiiGALShaderResourceDefinition>& out_resources);

  /// Delegate to creates a new declaration and register binding for a specific shader xiiGALShaderResourceDefinition.
  /// \param sPlatform The platform for which the shader is being compiled. Will be one of the values returned by GetSupportedPlatforms.
  /// \param sDeclaration The shader resource declaration without any attributes, e.g. "Texture2D DiffuseTexture"
  /// \param binding The binding that needs to be set on the output out_sDeclaration.
  /// \param out_sDeclaration The new declaration that changes sDeclaration according to the provided 'binding', e.g. "Texture2D DiffuseTexture : register(t0, space5)"
  using CreateResourceDeclaration = xiiDelegate<void(xiiStringView, xiiStringView, const xiiGALShaderResourceDescription&, xiiStringBuilder&)>;

  /// Merges the shader resource bindings of all used shader stages.
  ///
  /// The function can fail if a shader resource of the same name has different signatures in two stages. E.g. the type, slot or set is different. Shader resources must be uniquely identified via name.
  /// \param spd The shader currently being processed.
  /// \param out_bindings A hashmap from shader resource name to shader resource binding. If a binding is used in multiple stages, xiiGALShaderResourceDescription::m_Stages will be the combination of all used stages.
  /// \param pLog Log interface to write errors to.
  /// \return Returns failure if the shader stages could not be merged.
  static xiiResult MergeShaderResourceBindings(const xiiGALShaderProgramData& spd, xiiHashTable<xiiHashedString, xiiGALShaderResourceDescription>& out_bindings, xiiLogInterface* pLog);

  /// Makes sure that bindings fulfills the basic requirements that the graphics abstraction layer (GAL) has for resource bindings in a shader, e.g. that each binding has a set / slot set.
  static xiiResult SanityCheckShaderResourceBindings(const xiiHashTable<xiiHashedString, xiiGALShaderResourceDescription>& bindings, xiiLogInterface* pLog);

  /// Creates a new shader source code that patches all shader resources to contain fixed set / slot bindings.
  /// \param sPlatform The platform for which the shader should be patched.
  /// \param sShaderStageSource The original shader source code that should be patched.
  /// \param resources A list of all shader resources that need to be patched within sShaderStageSource.
  /// \param bindings The binding information that each shader resource should have after patching. These bindings must have unique set / slots combinations for each resource.
  /// \param createDeclaration The callback to be called to generate the new shader resource declaration.
  /// \param out_shaderStageSource The new shader source code after patching.
  static void ApplyShaderResourceBindings(xiiStringView sPlatform, xiiStringView sShaderStageSource, const xiiDynamicArray<xiiGALShaderResourceDefinition>& resources, const xiiHashTable<xiiHashedString, xiiGALShaderResourceDescription>& bindings, const CreateResourceDeclaration& createDeclaration, xiiStringBuilder& out_sShaderStageSource);
};

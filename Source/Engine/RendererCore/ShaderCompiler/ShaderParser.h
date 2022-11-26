#pragma once

#include <Foundation/Strings/String.h>
#include <RendererCore/Declarations.h>

class xiiPropertyAttribute;

class XII_RENDERERCORE_DLL xiiShaderParser
{
public:
  struct AttributeDefinition
  {
    xiiString                     m_sType;
    xiiHybridArray<xiiVariant, 8> m_Values;
  };

  struct ParameterDefinition
  {
    const xiiRTTI* m_pType = nullptr;
    xiiString      m_sType;
    xiiString      m_sName;

    xiiHybridArray<AttributeDefinition, 4> m_Attributes;
  };

  struct EnumValue
  {
    xiiHashedString m_sValueName;
    xiiInt32        m_iValueValue = 0;
  };

  struct EnumDefinition
  {
    xiiString                     m_sName;
    xiiUInt32                     m_uiDefaultValue = 0;
    xiiHybridArray<EnumValue, 16> m_Values;
  };

  static void ParseMaterialParameterSection(
    xiiStreamReader&                         stream,
    xiiHybridArray<ParameterDefinition, 16>& out_Parameter,
    xiiHybridArray<EnumDefinition, 4>&       out_EnumDefinitions);

  static void ParsePermutationSection(
    xiiStreamReader&                       stream,
    xiiHybridArray<xiiHashedString, 16>&   out_PermVars,
    xiiHybridArray<xiiPermutationVar, 16>& out_FixedPermVars);
  static void ParsePermutationSection(
    xiiStringView                          sPermutationSection,
    xiiHybridArray<xiiHashedString, 16>&   out_PermVars,
    xiiHybridArray<xiiPermutationVar, 16>& out_FixedPermVars);

  static void ParsePermutationVarConfig(xiiStringView sPermutationVarConfig, xiiVariant& out_DefaultValue, EnumDefinition& out_EnumDefinition);
};

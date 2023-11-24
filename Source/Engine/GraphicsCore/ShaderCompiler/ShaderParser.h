#pragma once

#include <Foundation/Strings/String.h>
#include <GraphicsCore/Declarations.h>

class xiiPropertyAttribute;

class XII_GRAPHICSCORE_DLL xiiShaderParser
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

  static void ParseMaterialParameterSection(xiiStreamReader& inout_stream, xiiHybridArray<ParameterDefinition, 16>& out_parameter, xiiHybridArray<EnumDefinition, 4>& out_enumDefinitions);

  static void ParsePermutationSection(xiiStreamReader& inout_stream, xiiHybridArray<xiiHashedString, 16>& out_permVars, xiiHybridArray<xiiPermutationVar, 16>& out_fixedPermVars);
  static void ParsePermutationSection(xiiStringView sPermutationSection, xiiHybridArray<xiiHashedString, 16>& out_permVars, xiiHybridArray<xiiPermutationVar, 16>& out_fixedPermVars);

  static void ParsePermutationVarConfig(xiiStringView sPermutationVarConfig, xiiVariant& out_defaultValue, EnumDefinition& out_enumDefinition);
};

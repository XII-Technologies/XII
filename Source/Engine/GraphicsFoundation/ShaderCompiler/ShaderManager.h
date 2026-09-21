/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GraphicsFoundation/GraphicsFoundationDLL.h>

#include <GraphicsFoundation/ShaderCompiler/ShaderParser.h>

class XII_GRAPHICSFOUNDATION_DLL xiiGALShaderManager
{
public:
  static void             Configure(xiiStringView sActivePlatform, bool bEnableRuntimeCompilation, xiiStringView sShaderCacheDirectory = ":shadercache/ShaderCache"_xiisv, xiiStringView sPermutationVariableSubDirectory = "Shaders/PermutationVariables"_xiisv);
  static const xiiString& GetPermutationVarSubDirectory() { return s_sPermutationVariableSubDirectory; }
  static const xiiString& GetActivePlatform() { return s_sPlatform; }
  static const xiiString& GetCacheDirectory() { return s_sShaderCacheDirectory; }
  static bool             IsRuntimeCompilationEnabled() { return s_bEnableRuntimeCompilation; }

  static void ReloadPermutationVarConfig(xiiStringView sName, const xiiTempHashedString& sHashedName);
  static bool IsPermutationValueAllowed(xiiStringView sName, const xiiTempHashedString& sHashedName, const xiiTempHashedString& sValue, xiiHashedString& out_sName, xiiHashedString& out_sValue);
  static bool IsPermutationValueAllowed(const xiiHashedString& sName, const xiiHashedString& sValue);

  /// If the given permutation variable is an enum variable, this returns the possible values.
  /// Returns an empty array for other types of permutation variables.
  static xiiArrayPtr<const xiiGALShaderParser::EnumValue> GetPermutationEnumValues(const xiiHashedString& sName);

  /// Same as GetPermutationEnumValues() but also returns values for other types of variables.
  /// E.g. returns TRUE and FALSE for boolean variables.
  static void GetPermutationValues(const xiiHashedString& sName, xiiDynamicArray<xiiHashedString>& out_values);

  static xiiUInt32 FilterPermutationVariables(xiiArrayPtr<const xiiHashedString> usedVariables, const xiiHashTable<xiiHashedString, xiiHashedString>& permutationVariables, xiiDynamicArray<xiiGALPermutationVariable>& out_FilteredPermutationVariables);

private:
  static bool      s_bEnableRuntimeCompilation;
  static xiiString s_sPlatform;
  static xiiString s_sPermutationVariableSubDirectory;
  static xiiString s_sShaderCacheDirectory;
};

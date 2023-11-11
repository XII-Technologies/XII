#pragma once

#include <Foundation/Containers/HashTable.h>
#include <GraphicsCore/Declarations.h>
#include <GraphicsCore/ShaderCompiler/PermutationGenerator.h>
#include <GraphicsCore/ShaderCompiler/ShaderParser.h>

class XII_RENDERERCORE_DLL xiiShaderManager
{
public:
  static void             Configure(const char* szActivePlatform, bool bEnableRuntimeCompilation, const char* szShaderCacheDirectory = ":shadercache/ShaderCache", const char* szPermVarSubDirectory = "Shaders/PermutationVars");
  static const xiiString& GetPermutationVarSubDirectory() { return s_sPermVarSubDir; }
  static const xiiString& GetActivePlatform() { return s_sPlatform; }
  static const xiiString& GetCacheDirectory() { return s_sShaderCacheDirectory; }
  static bool             IsRuntimeCompilationEnabled() { return s_bEnableRuntimeCompilation; }

  static void ReloadPermutationVarConfig(const char* szName, const xiiTempHashedString& sHashedName);
  static bool IsPermutationValueAllowed(const char* szName, const xiiTempHashedString& sHashedName, const xiiTempHashedString& sValue, xiiHashedString& out_sName, xiiHashedString& out_sValue);
  static bool IsPermutationValueAllowed(const xiiHashedString& sName, const xiiHashedString& sValue);

  /// \brief If the given permutation variable is an enum variable, this returns the possible values.
  /// Returns an empty array for other types of permutation variables.
  static xiiArrayPtr<const xiiShaderParser::EnumValue> GetPermutationEnumValues(const xiiHashedString& sName);

  /// \brief Same as GetPermutationEnumValues() but also returns values for other types of variables.
  /// E.g. returns TRUE and FALSE for boolean variables.
  static void GetPermutationValues(const xiiHashedString& sName, xiiDynamicArray<xiiHashedString>& out_values);

  static void PreloadPermutations(
    xiiShaderResourceHandle                               hShader,
    const xiiHashTable<xiiHashedString, xiiHashedString>& permVars,
    xiiTime                                               shouldBeAvailableIn);
  static xiiShaderPermutationResourceHandle PreloadSinglePermutation(
    xiiShaderResourceHandle                               hShader,
    const xiiHashTable<xiiHashedString, xiiHashedString>& permVars,
    bool                                                  bAllowFallback);

private:
  static xiiUInt32                          FilterPermutationVars(xiiArrayPtr<const xiiHashedString> usedVars, const xiiHashTable<xiiHashedString, xiiHashedString>& permVars, xiiDynamicArray<xiiPermutationVar>& out_FilteredPermutationVariables);
  static xiiShaderPermutationResourceHandle PreloadSinglePermutationInternal(
    const char*                    szResourceId,
    xiiUInt64                      uiResourceIdHash,
    xiiUInt32                      uiPermutationHash,
    xiiArrayPtr<xiiPermutationVar> filteredPermutationVariables);

  static bool      s_bEnableRuntimeCompilation;
  static xiiString s_sPlatform;
  static xiiString s_sPermVarSubDir;
  static xiiString s_sShaderCacheDirectory;
};

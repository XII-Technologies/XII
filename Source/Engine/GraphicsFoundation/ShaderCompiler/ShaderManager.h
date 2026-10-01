/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Configuration/StaticSubSystem.h>
#include <Foundation/Types/UniquePtr.h>
#include <GraphicsFoundation/GraphicsFoundationDLL.h>

#include <GraphicsFoundation/ShaderCompiler/ShaderParser.h>

class XII_GRAPHICSFOUNDATION_DLL xiiGALShaderManager
{
  XII_DISALLOW_COPY_AND_ASSIGN(xiiGALShaderManager);
  XII_MAKE_SUBSYSTEM_STARTUP_FRIEND(GraphicsFoundation, ShaderCompiler);

public:
  xiiGALShaderManager() = delete;

  static void             Configure(xiiStringView sActivePlatform, bool bEnableRuntimeCompilation, xiiStringView sShaderCacheDirectory = ":shadercache/ShaderCache"_xiisv, xiiStringView sPermutationVariableSubDirectory = "Shaders/PermutationVariables"_xiisv);
  static const xiiString& GetPermutationVarSubDirectory();
  static const xiiString& GetActivePlatform();
  static const xiiString& GetCacheDirectory();
  static bool             IsRuntimeCompilationEnabled();
  static bool             IsInitialized();

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
  struct PermutationVarConfig;
  class State;

  static void Startup();
  static void Shutdown();

  static const PermutationVarConfig* FindConfig(xiiStringView sName, const xiiTempHashedString& sHashedName);
  static const PermutationVarConfig* FindConfig(const xiiHashedString& sName);
  static bool                        IsValueAllowed(const PermutationVarConfig& config, const xiiTempHashedString& sValue, xiiHashedString& out_sValue);
  static bool                        IsValueAllowed(const PermutationVarConfig& config, const xiiTempHashedString& sValue);

  static xiiUniquePtr<State> s_pState;
};

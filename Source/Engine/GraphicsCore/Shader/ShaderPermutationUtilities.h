/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Configuration/StaticSubSystem.h>
#include <Foundation/Types/UniquePtr.h>
#include <GraphicsCore/GraphicsCoreDLL.h>

#include <GraphicsCore/Shader/ShaderPermutationResource.h>
#include <GraphicsCore/Shader/ShaderResource.h>

class xiiShaderPermutationUtilitiesState;

/// Process-wide shader permutation path cache.
///
/// The cache is created after Foundation startup and destroyed before allocator
/// shutdown. Public operations are thread-safe because render pipelines can
/// preload shader permutations concurrently.
class XII_GRAPHICSCORE_DLL xiiShaderPermutationUtilities
{
  XII_DISALLOW_COPY_AND_ASSIGN(xiiShaderPermutationUtilities);
  XII_MAKE_SUBSYSTEM_STARTUP_FRIEND(GraphicsCore, ShaderPermutationUtilities);

public:
  xiiShaderPermutationUtilities() = delete;

  /// Returns an invalid handle when the shader is unavailable or invalid. Validate before acquiring the permutation.
  /// Set bAllowFallback to false when constructing a required pipeline: a loading fallback does not describe the requested shader.
  static xiiShaderPermutationResourceHandle PreloadSinglePermutation(xiiShaderResourceHandle hShader, const xiiHashTable<xiiHashedString, xiiHashedString>& permVars, bool bAllowFallback);

private:
  static void Startup();
  static void Shutdown();

  static xiiShaderPermutationResourceHandle PreloadSinglePermutationInternal(xiiStringView sResourceId, xiiUInt64 uiResourceIdHash, xiiUInt32 uiPermutationHash, xiiArrayPtr<xiiGALPermutationVariable> filteredPermutationVariables);

  static xiiUniquePtr<xiiShaderPermutationUtilitiesState> s_pState;
};

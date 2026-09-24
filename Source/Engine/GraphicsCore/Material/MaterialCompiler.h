/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Containers/DynamicArray.h>
#include <GraphicsCore/Material/MaterialSchema.h>

class xiiShaderPermutationResource;

struct XII_GRAPHICSCORE_DLL xiiMaterialCompilationSeverity
{
  using StorageType = xiiUInt8;

  enum Enum : StorageType
  {
    Info,
    Warning,
    Error,

    Default = Info
  };
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiMaterialCompilationSeverity);

/// Tool-facing shader-contract diagnostic emitted by xiiMaterialCompiler.
struct XII_GRAPHICSCORE_DLL xiiMaterialCompilationMessage
{
  xiiEnum<xiiMaterialCompilationSeverity> m_Severity = xiiMaterialCompilationSeverity::Info;
  xiiString                               m_sParameter;
  xiiString                               m_sMessage;
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiMaterialCompilationMessage);

struct XII_GRAPHICSCORE_DLL xiiCompiledMaterialLayout
{
  xiiUInt64                                      m_uiSchemaHash       = 0ULL;
  xiiUInt32                                      m_uiShaderBlockSize  = 0U;
  xiiUInt32                                      m_uiParameterCount   = 0U;
  xiiUInt32                                      m_uiTextureCount     = 0U;
  xiiDynamicArray<xiiMaterialCompilationMessage> m_Messages;
  bool                                           m_bValid = false;
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiCompiledMaterialLayout);

/// Validates an immutable material schema against shader reflection before a pipeline is cached.
/// This catches stale assets, backend packing differences, missing resources and incompatible
/// specialization permutations without relying on draw-time validation.
class XII_GRAPHICSCORE_DLL xiiMaterialCompiler
{
public:
  static xiiResult ValidateShaderLayout(const xiiMaterialSchema& schema, const xiiShaderPermutationResource& permutation, xiiCompiledMaterialLayout& out_layout, xiiStringView sParameterBlockName = "xiiMaterialConstants");
};


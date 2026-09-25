#pragma once

#include <Shaders/Common/ShaderResourceMacros.h>

/// Device-specific mesh dispatch limits used to tile a compact meshlet list over X/Y/Z.
DECLARE_CONSTANT_BUFFER_AUTO(xiiGpuSceneCommandBuildConstants)
{
  UINT1(MaxMeshGroupCountX);
  UINT1(MaxMeshGroupCountY);
  UINT1(MaxMeshGroupTotalCount);
  UINT1(Padding);
};

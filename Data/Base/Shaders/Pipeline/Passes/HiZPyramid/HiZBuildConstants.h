/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Shaders/Common/ShaderResourceMacros.h>

/// Per-dispatch constants for one mip level of a reversed-Z pyramid reduction.
///
/// Bound at slot b3 for each individual dispatch in the per-mip loop.
/// The execute callback uploads a fresh instance for each mip.
DECLARE_CONSTANT_BUFFER_AUTO(xiiHiZBuildConstants)
{
  UINT2(SrcSize); ///< Width x Height of the source mip level (mip N).
  UINT2(DstSize); ///< Width x Height of the destination mip level (mip N+1).
  UINT1(Reduce);  ///< Zero copies scene depth into mip zero; one reduces a 2x2 source footprint.
  FLOAT1(_Pad0);
  FLOAT2(_Pad1);
};

/// Copyright (c) Theophilus Eriata. All Rights Reserved.
#pragma once
#include <Shaders/Common/ShaderResourceMacros.h>
DECLARE_CONSTANT_BUFFER_AUTO(xiiSparseVoxelPoolConstants) { UINT4(PoolWork); }; ///< count, dispatch width, history enabled, reserved.

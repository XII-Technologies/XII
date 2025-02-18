#pragma once

// This file is included both in shader code and in C++ code

DECLARE_CONSTANT_BUFFER(xiiTextureSampleConstants, 0, 1)
{
  MAT4(ModelMatrix);
  MAT4(ViewProjectionMatrix);
};

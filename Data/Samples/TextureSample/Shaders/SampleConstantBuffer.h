#pragma once

// This file is included both in shader code and in C++ code

DECLARE_CONSTANT_BUFFER_AUTO(xiiTextureSampleConstants)
{
  MAT4(ModelMatrix);
  MAT4(ViewProjectionMatrix);
};

#pragma once

#include <Foundation/Math/Vec4.h>

namespace xiiInternal
{
  typedef xiiVec4    QuadFloat;
  typedef xiiVec4I32 QuadInt;
  typedef xiiVec4U32 QuadUInt;

  struct QuadBool
  {
    bool x, y, z, w;
  };
} // namespace xiiInternal

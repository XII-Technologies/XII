/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GraphicsFoundation/GraphicsFoundationDLL.h>

#include <Foundation/Math/Float16.h>
#include <Foundation/Math/Vec3.h>

/// Provides shader utilities through static methods.
class XII_GRAPHICSFOUNDATION_DLL xiiGALShaderUtilities
{
public:
  /// Converts a 3-component 32-bit vector into the RGB10 format.
  XII_ALWAYS_INLINE static xiiUInt32 Float3ToRGB10(xiiVec3 value)
  {
    const xiiVec3 unsignedValue = value * 0.5f + xiiVec3(0.5f);

    const xiiUInt32 r = xiiMath::ColorFloatToUnsignedInt<10>(unsignedValue.x);
    const xiiUInt32 g = xiiMath::ColorFloatToUnsignedInt<10>(unsignedValue.y);
    const xiiUInt32 b = xiiMath::ColorFloatToUnsignedInt<10>(unsignedValue.z);

    return r | (g << 10) | (b << 20);
  }

  /// Packs 2 16-bit floats into a 32-bit unsigned integer.
  XII_ALWAYS_INLINE static xiiUInt32 PackFloat16intoUint(xiiFloat16 x, xiiFloat16 y)
  {
    const xiiUInt32 r = x.GetRawData();
    const xiiUInt32 g = y.GetRawData();

    return r | (g << 16);
  }

  /// Converts a 2 component 32-bit vector into a 32-bit unsigned integer.
  XII_ALWAYS_INLINE static xiiUInt32 Float2ToRG16F(xiiVec2 value)
  {
    const xiiUInt32 r = xiiFloat16(value.x).GetRawData();
    const xiiUInt32 g = xiiFloat16(value.y).GetRawData();

    return r | (g << 16);
  }

  /// Converts a 32-bit 4 component vector into a 4 compponent 16-bit values stored in 2 32-bit unsigned integers.
  XII_ALWAYS_INLINE static void Float4ToRGBA16F(xiiVec4 value, xiiUInt32& out_uiRG, xiiUInt32& out_uiBA)
  {
    out_uiRG = Float2ToRG16F(xiiVec2(value.x, value.y));
    out_uiBA = Float2ToRG16F(xiiVec2(value.z, value.w));
  }
};

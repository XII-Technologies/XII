/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

/// A 16 bit IEEE float class. Often called "half"
///
/// This class only contains functions to convert between float and float16. It does not support any mathematical operations.
/// It is only intended for conversion, always do all mathematical operations on regular floats (or let the GPU do them on halfs).
class XII_FOUNDATION_DLL xiiFloat16
{
public:
  // Means that vectors can be copied using memcpy instead of copy construction.
  XII_DECLARE_POD_TYPE();

  /// Default constructor does not initialize the value.
  xiiFloat16() = default;

  /// Create float16 from float.
  xiiFloat16(float f); // [tested]

  /// Create float16 from float.
  void operator=(float f); // [tested]

  /// Create float16 from raw data.
  void SetRawData(xiiUInt16 uiData) { m_uiData = uiData; } // [tested]

  /// Returns the raw 16 Bit data.
  xiiUInt16 GetRawData() const { return m_uiData; } // [tested]

  /// Convert float16 to float.
  operator float() const; // [tested]

  /// Returns true, if both values are identical.
  bool operator==(const xiiFloat16& c2) { return m_uiData == c2.m_uiData; } // [tested]

  /// Returns true, if both values are not identical.
  bool operator!=(const xiiFloat16& c2) { return m_uiData != c2.m_uiData; } // [tested]

private:
  /// Raw 16 float data.
  xiiUInt16 m_uiData;
};

/// A simple helper class to use half-precision floats (xiiFloat16) as vectors
class XII_FOUNDATION_DLL xiiFloat16Vec2
{
public:
  // Means that vectors can be copied using memcpy instead of copy construction.
  XII_DECLARE_POD_TYPE();

  xiiFloat16Vec2() = default;
  xiiFloat16Vec2(const xiiVec2& vVec);

  void operator=(const xiiVec2& vVec);
       operator xiiVec2() const;

  xiiFloat16 x, y;
};

/// A simple helper class to use half-precision floats (xiiFloat16) as vectors
class XII_FOUNDATION_DLL xiiFloat16Vec3
{
public:
  // Means that vectors can be copied using memcpy instead of copy construction.
  XII_DECLARE_POD_TYPE();

  xiiFloat16Vec3() = default;
  xiiFloat16Vec3(const xiiVec3& vVec);

  void operator=(const xiiVec3& vVec);
       operator xiiVec3() const;

  xiiFloat16 x, y, z;
};

/// A simple helper class to use half-precision floats (xiiFloat16) as vectors
class XII_FOUNDATION_DLL xiiFloat16Vec4
{
public:
  // Means that vectors can be copied using memcpy instead of copy construction.
  XII_DECLARE_POD_TYPE();

  xiiFloat16Vec4() = default;
  xiiFloat16Vec4(const xiiVec4& vVec);

  void operator=(const xiiVec4& vVec);
       operator xiiVec4() const;

  xiiFloat16 x, y, z, w;
};

#pragma once

/// \brief A 16 bit IEEE float class. Often called "half"
///
/// This class only contains functions to convert between float and float16. It does not support any mathematical operations.
/// It is only intended for conversion, always do all mathematical operations on regular floats (or let the GPU do them on halfs).
class XII_FOUNDATION_DLL xiiFloat16
{
public:
  // Means that vectors can be copied using memcpy instead of copy construction.
  XII_DECLARE_POD_TYPE();

  /// \brief Default constructor does not initialize the value.
  xiiFloat16() = default;

  /// \brief Create float16 from float.
  xiiFloat16(float f); // [tested]

  /// \brief Create float16 from float.
  void operator=(float f); // [tested]

  /// \brief Create float16 from raw data.
  void SetRawData(xiiUInt16 data) { m_uiData = data; } // [tested]

  /// \brief Returns the raw 16 Bit data.
  xiiUInt16 GetRawData() const { return m_uiData; } // [tested]

  /// \brief Convert float16 to float.
  operator float() const; // [tested]

  /// \brief Returns true, if both values are identical.
  bool operator==(const xiiFloat16& c2) { return m_uiData == c2.m_uiData; } // [tested]

  /// \brief Returns true, if both values are not identical.
  bool operator!=(const xiiFloat16& c2) { return m_uiData != c2.m_uiData; } // [tested]

private:
  /// Raw 16 float data.
  xiiUInt16 m_uiData;
};

/// \brief A simple helper class to use half-precision floats (xiiFloat16) as vectors
class XII_FOUNDATION_DLL xiiFloat16Vec2
{
public:
  // Means that vectors can be copied using memcpy instead of copy construction.
  XII_DECLARE_POD_TYPE();

  xiiFloat16Vec2() = default;
  xiiFloat16Vec2(const xiiVec2& vec);

  void operator=(const xiiVec2& vec);
  operator xiiVec2() const;

  xiiFloat16 x, y;
};

/// \brief A simple helper class to use half-precision floats (xiiFloat16) as vectors
class XII_FOUNDATION_DLL xiiFloat16Vec3
{
public:
  // Means that vectors can be copied using memcpy instead of copy construction.
  XII_DECLARE_POD_TYPE();

  xiiFloat16Vec3() = default;
  xiiFloat16Vec3(const xiiVec3& vec);

  void operator=(const xiiVec3& vec);
  operator xiiVec3() const;

  xiiFloat16 x, y, z;
};

/// \brief A simple helper class to use half-precision floats (xiiFloat16) as vectors
class XII_FOUNDATION_DLL xiiFloat16Vec4
{
public:
  // Means that vectors can be copied using memcpy instead of copy construction.
  XII_DECLARE_POD_TYPE();

  xiiFloat16Vec4() = default;
  xiiFloat16Vec4(const xiiVec4& vec);

  void operator=(const xiiVec4& vec);
  operator xiiVec4() const;

  xiiFloat16 x, y, z, w;
};

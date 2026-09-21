/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Strings/HashedString.h>

/// A single stream in a stream group holding contiguous data of a given type.
class XII_FOUNDATION_DLL xiiProcessingStream
{
public:
  /// The data types which can be stored in the stream.
  /// When adding new data types the GetDataTypeSize() of xiiProcessingStream needs to be updated.
  enum class DataType : xiiUInt8
  {
    Half,  // xiiFloat16
    Half2, // 2x xiiFloat16
    Half3, // 3x xiiFloat16
    Half4, // 4x xiiFloat16

    Float,  // float
    Float2, // 2x float, e.g. xiiVec2
    Float3, // 3x float, e.g. xiiVec3
    Float4, // 4x float, e.g. xiiVec4

    Double,  // double
    Double2, // 2x double, e.g. xiiVec2d
    Double3, // 3x double, e.g. xiiVec3d
    Double4, // 4x double, e.g. xiiVec4d

    Byte,
    Byte2,
    Byte3,
    Byte4,

    Short,
    Short2,
    Short3,
    Short4,

    Int,
    Int2,
    Int3,
    Int4,

    Count
  };

  xiiProcessingStream();
  xiiProcessingStream(const xiiHashedString& sName, DataType type, xiiUInt16 uiStride, xiiUInt16 uiAlignment);
  xiiProcessingStream(const xiiHashedString& sName, xiiArrayPtr<xiiUInt8> data, DataType type, xiiUInt16 uiStride);
  xiiProcessingStream(const xiiHashedString& sName, xiiArrayPtr<xiiUInt8> data, DataType type);
  ~xiiProcessingStream();

  /// Returns a const pointer to the data casted to the type T, note that no type check is done!
  template <typename T>
  const T* GetData() const
  {
    return static_cast<const T*>(GetData());
  }

  /// Returns a const pointer to the start of the data block.
  const void* GetData() const { return m_pData; }

  /// Returns a non-const pointer to the data casted to the type T, note that no type check is done!
  template <typename T>
  T* GetWritableData() const
  {
    return static_cast<T*>(GetWritableData());
  }

  /// Returns a non-const pointer to the start of the data block.
  void* GetWritableData() const { return m_pData; }

  xiiUInt64 GetDataSize() const { return m_uiDataSize; }

  /// Returns the name of the stream
  const xiiHashedString& GetName() const { return m_sName; }

  /// Returns the alignment which was used to allocate the stream.
  xiiUInt16 GetAlignment() const { return m_uiAlignment; }

  /// Returns the data type of the stream.
  DataType GetDataType() const { return m_Type; }

  /// Returns the size of one stream element in bytes.
  xiiUInt16 GetElementSize() const { return m_uiTypeSize; }

  /// Returns the stride between two elements of the stream in bytes.
  xiiUInt16 GetElementStride() const { return m_uiStride; }

  static xiiUInt16     GetDataTypeSize(DataType type);
  static xiiStringView GetDataTypeName(DataType type);

protected:
  friend class xiiProcessingStreamGroup;

  void SetSize(xiiUInt64 uiNumElements);
  void FreeData();

  void*     m_pData      = nullptr;
  xiiUInt64 m_uiDataSize = 0; // in bytes

  xiiUInt16 m_uiAlignment = 0;
  xiiUInt16 m_uiTypeSize  = 0;
  xiiUInt16 m_uiStride    = 0;
  DataType  m_Type;
  bool      m_bExternalMemory = false;

  xiiHashedString m_sName;
};

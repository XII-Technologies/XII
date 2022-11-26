#pragma once

#include <Foundation/Containers/Bitfield.h>
#include <Foundation/Containers/HybridArray.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Strings/StringBuilder.h>
#include <Foundation/Types/Bitflags.h>
#include <Foundation/Types/Enum.h>

// Standard operators for overloads of common data types

/// bool versions

inline xiiStreamWriter& operator<<(xiiStreamWriter& stream, bool bValue)
{
  xiiUInt8 uiValue = bValue ? 1 : 0;
  stream.WriteBytes(&uiValue, sizeof(xiiUInt8)).IgnoreResult();
  return stream;
}

inline xiiStreamReader& operator>>(xiiStreamReader& stream, bool& bValue)
{
  xiiUInt8 uiValue = 0;
  stream.ReadBytes(&uiValue, sizeof(xiiUInt8));
  bValue = (uiValue != 0);
  return stream;
}

/// unsigned int versions

inline xiiStreamWriter& operator<<(xiiStreamWriter& stream, xiiUInt8 uiValue)
{
  stream.WriteBytes(&uiValue, sizeof(xiiUInt8)).IgnoreResult();
  return stream;
}

inline xiiStreamReader& operator>>(xiiStreamReader& stream, xiiUInt8& uiValue)
{
  stream.ReadBytes(&uiValue, sizeof(xiiUInt8));
  return stream;
}

inline xiiResult SerializeArray(xiiStreamWriter& stream, const xiiUInt8* pArray, xiiUInt64 uiCount)
{
  return stream.WriteBytes(pArray, sizeof(xiiUInt8) * uiCount);
}

inline xiiResult DeserializeArray(xiiStreamReader& stream, xiiUInt8* pArray, xiiUInt64 uiCount)
{
  const xiiUInt64 uiNumBytes = sizeof(xiiUInt8) * uiCount;
  if (stream.ReadBytes(pArray, uiNumBytes) == uiNumBytes)
    return XII_SUCCESS;

  return XII_FAILURE;
}


inline xiiStreamWriter& operator<<(xiiStreamWriter& stream, xiiUInt16 uiValue)
{
  stream.WriteWordValue(&uiValue).IgnoreResult();
  return stream;
}

inline xiiStreamReader& operator>>(xiiStreamReader& stream, xiiUInt16& uiValue)
{
  stream.ReadWordValue(&uiValue).IgnoreResult();
  return stream;
}

inline xiiResult SerializeArray(xiiStreamWriter& stream, const xiiUInt16* pArray, xiiUInt64 uiCount)
{
  return stream.WriteBytes(pArray, sizeof(xiiUInt16) * uiCount);
}

inline xiiResult DeserializeArray(xiiStreamReader& stream, xiiUInt16* pArray, xiiUInt64 uiCount)
{
  const xiiUInt64 uiNumBytes = sizeof(xiiUInt16) * uiCount;
  if (stream.ReadBytes(pArray, uiNumBytes) == uiNumBytes)
    return XII_SUCCESS;

  return XII_FAILURE;
}


inline xiiStreamWriter& operator<<(xiiStreamWriter& stream, xiiUInt32 uiValue)
{
  stream.WriteDWordValue(&uiValue).IgnoreResult();
  return stream;
}

inline xiiStreamReader& operator>>(xiiStreamReader& stream, xiiUInt32& uiValue)
{
  stream.ReadDWordValue(&uiValue).IgnoreResult();
  return stream;
}

inline xiiResult SerializeArray(xiiStreamWriter& stream, const xiiUInt32* pArray, xiiUInt64 uiCount)
{
  return stream.WriteBytes(pArray, sizeof(xiiUInt32) * uiCount);
}

inline xiiResult DeserializeArray(xiiStreamReader& stream, xiiUInt32* pArray, xiiUInt64 uiCount)
{
  const xiiUInt64 uiNumBytes = sizeof(xiiUInt32) * uiCount;
  if (stream.ReadBytes(pArray, uiNumBytes) == uiNumBytes)
    return XII_SUCCESS;

  return XII_FAILURE;
}


inline xiiStreamWriter& operator<<(xiiStreamWriter& stream, xiiUInt64 uiValue)
{
  stream.WriteQWordValue(&uiValue).IgnoreResult();
  return stream;
}

inline xiiStreamReader& operator>>(xiiStreamReader& stream, xiiUInt64& uiValue)
{
  stream.ReadQWordValue(&uiValue).IgnoreResult();
  return stream;
}

inline xiiResult SerializeArray(xiiStreamWriter& stream, const xiiUInt64* pArray, xiiUInt64 uiCount)
{
  return stream.WriteBytes(pArray, sizeof(xiiUInt64) * uiCount);
}

inline xiiResult DeserializeArray(xiiStreamReader& stream, xiiUInt64* pArray, xiiUInt64 uiCount)
{
  const xiiUInt64 uiNumBytes = sizeof(xiiUInt64) * uiCount;
  if (stream.ReadBytes(pArray, uiNumBytes) == uiNumBytes)
    return XII_SUCCESS;

  return XII_FAILURE;
}

/// signed int versions

inline xiiStreamWriter& operator<<(xiiStreamWriter& stream, xiiInt8 iValue)
{
  stream.WriteBytes(reinterpret_cast<const xiiUInt8*>(&iValue), sizeof(xiiInt8)).IgnoreResult();
  return stream;
}

inline xiiStreamReader& operator>>(xiiStreamReader& stream, xiiInt8& iValue)
{
  stream.ReadBytes(reinterpret_cast<xiiUInt8*>(&iValue), sizeof(xiiInt8));
  return stream;
}

inline xiiResult SerializeArray(xiiStreamWriter& stream, const xiiInt8* pArray, xiiUInt64 uiCount)
{
  return stream.WriteBytes(pArray, sizeof(xiiInt8) * uiCount);
}

inline xiiResult DeserializeArray(xiiStreamReader& stream, xiiInt8* pArray, xiiUInt64 uiCount)
{
  const xiiUInt64 uiNumBytes = sizeof(xiiInt8) * uiCount;
  if (stream.ReadBytes(pArray, uiNumBytes) == uiNumBytes)
    return XII_SUCCESS;

  return XII_FAILURE;
}


inline xiiStreamWriter& operator<<(xiiStreamWriter& stream, xiiInt16 iValue)
{
  stream.WriteWordValue(&iValue).IgnoreResult();
  return stream;
}

inline xiiStreamReader& operator>>(xiiStreamReader& stream, xiiInt16& iValue)
{
  stream.ReadWordValue(&iValue).IgnoreResult();
  return stream;
}

inline xiiResult SerializeArray(xiiStreamWriter& stream, const xiiInt16* pArray, xiiUInt64 uiCount)
{
  return stream.WriteBytes(pArray, sizeof(xiiInt16) * uiCount);
}

inline xiiResult DeserializeArray(xiiStreamReader& stream, xiiInt16* pArray, xiiUInt64 uiCount)
{
  const xiiUInt64 uiNumBytes = sizeof(xiiInt16) * uiCount;
  if (stream.ReadBytes(pArray, uiNumBytes) == uiNumBytes)
    return XII_SUCCESS;

  return XII_FAILURE;
}


inline xiiStreamWriter& operator<<(xiiStreamWriter& stream, xiiInt32 iValue)
{
  stream.WriteDWordValue(&iValue).IgnoreResult();
  return stream;
}

inline xiiStreamReader& operator>>(xiiStreamReader& stream, xiiInt32& iValue)
{
  stream.ReadDWordValue(&iValue).IgnoreResult();
  return stream;
}

inline xiiResult SerializeArray(xiiStreamWriter& stream, const xiiInt32* pArray, xiiUInt64 uiCount)
{
  return stream.WriteBytes(pArray, sizeof(xiiInt32) * uiCount);
}

inline xiiResult DeserializeArray(xiiStreamReader& stream, xiiInt32* pArray, xiiUInt64 uiCount)
{
  const xiiUInt64 uiNumBytes = sizeof(xiiInt32) * uiCount;
  if (stream.ReadBytes(pArray, uiNumBytes) == uiNumBytes)
    return XII_SUCCESS;

  return XII_FAILURE;
}


inline xiiStreamWriter& operator<<(xiiStreamWriter& stream, xiiInt64 iValue)
{
  stream.WriteQWordValue(&iValue).IgnoreResult();
  return stream;
}

inline xiiStreamReader& operator>>(xiiStreamReader& stream, xiiInt64& iValue)
{
  stream.ReadQWordValue(&iValue).IgnoreResult();
  return stream;
}

inline xiiResult SerializeArray(xiiStreamWriter& stream, const xiiInt64* pArray, xiiUInt64 uiCount)
{
  return stream.WriteBytes(pArray, sizeof(xiiInt64) * uiCount);
}

inline xiiResult DeserializeArray(xiiStreamReader& stream, xiiInt64* pArray, xiiUInt64 uiCount)
{
  const xiiUInt64 uiNumBytes = sizeof(xiiInt64) * uiCount;
  if (stream.ReadBytes(pArray, uiNumBytes) == uiNumBytes)
    return XII_SUCCESS;

  return XII_FAILURE;
}


/// float and double versions

inline xiiStreamWriter& operator<<(xiiStreamWriter& stream, float fValue)
{
  stream.WriteDWordValue(&fValue).IgnoreResult();
  return stream;
}

inline xiiStreamReader& operator>>(xiiStreamReader& stream, float& fValue)
{
  stream.ReadDWordValue(&fValue).IgnoreResult();
  return stream;
}

inline xiiResult SerializeArray(xiiStreamWriter& stream, const float* pArray, xiiUInt64 uiCount)
{
  return stream.WriteBytes(pArray, sizeof(float) * uiCount);
}

inline xiiResult DeserializeArray(xiiStreamReader& stream, float* pArray, xiiUInt64 uiCount)
{
  const xiiUInt64 uiNumBytes = sizeof(float) * uiCount;
  if (stream.ReadBytes(pArray, uiNumBytes) == uiNumBytes)
    return XII_SUCCESS;

  return XII_FAILURE;
}


inline xiiStreamWriter& operator<<(xiiStreamWriter& stream, double fValue)
{
  stream.WriteQWordValue(&fValue).IgnoreResult();
  return stream;
}

inline xiiStreamReader& operator>>(xiiStreamReader& stream, double& fValue)
{
  stream.ReadQWordValue(&fValue).IgnoreResult();
  return stream;
}

inline xiiResult SerializeArray(xiiStreamWriter& stream, const double* pArray, xiiUInt64 uiCount)
{
  return stream.WriteBytes(pArray, sizeof(double) * uiCount);
}

inline xiiResult DeserializeArray(xiiStreamReader& stream, double* pArray, xiiUInt64 uiCount)
{
  const xiiUInt64 uiNumBytes = sizeof(double) * uiCount;
  if (stream.ReadBytes(pArray, uiNumBytes) == uiNumBytes)
    return XII_SUCCESS;

  return XII_FAILURE;
}


// C-style strings
// No read equivalent for C-style strings (but can be read as xiiString & xiiStringBuilder instances)

XII_FOUNDATION_DLL xiiStreamWriter& operator<<(xiiStreamWriter& stream, const char* szValue);

// xiiHybridString

template <xiiUInt16 Size, typename AllocatorWrapper>
inline xiiStreamWriter& operator<<(xiiStreamWriter& stream, const xiiHybridString<Size, AllocatorWrapper>& sValue)
{
  stream.WriteString(sValue.GetView()).IgnoreResult();
  return stream;
}

template <xiiUInt16 Size, typename AllocatorWrapper>
inline xiiStreamReader& operator>>(xiiStreamReader& stream, xiiHybridString<Size, AllocatorWrapper>& sValue)
{
  xiiStringBuilder builder;
  stream.ReadString(builder).IgnoreResult();
  sValue = std::move(builder);

  return stream;
}

// xiiStringBuilder

XII_FOUNDATION_DLL xiiStreamWriter& operator<<(xiiStreamWriter& stream, const xiiStringBuilder& sValue);
XII_FOUNDATION_DLL xiiStreamReader& operator>>(xiiStreamReader& stream, xiiStringBuilder& sValue);

// xiiEnum

template <typename T>
inline xiiStreamWriter& operator<<(xiiStreamWriter& stream, const xiiEnum<T>& value)
{
  stream << value.GetValue();

  return stream;
}

template <typename T>
inline xiiStreamReader& operator>>(xiiStreamReader& stream, xiiEnum<T>& value)
{
  typename T::StorageType storedValue = T::Default;
  stream >> storedValue;
  value.SetValue(storedValue);

  return stream;
}

// xiiBitflags

template <typename T>
inline xiiStreamWriter& operator<<(xiiStreamWriter& stream, const xiiBitflags<T>& value)
{
  stream << value.GetValue();

  return stream;
}

template <typename T>
inline xiiStreamReader& operator>>(xiiStreamReader& stream, xiiBitflags<T>& value)
{
  typename T::StorageType storedValue = T::Default;
  stream >> storedValue;
  value.SetValue(storedValue);

  return stream;
}

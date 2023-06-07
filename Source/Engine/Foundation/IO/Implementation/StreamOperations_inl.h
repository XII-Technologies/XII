#pragma once

#include <Foundation/Containers/Bitfield.h>
#include <Foundation/Containers/HybridArray.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Strings/StringBuilder.h>
#include <Foundation/Types/Bitflags.h>
#include <Foundation/Types/Enum.h>

// Standard operators for overloads of common data types

/// Boolean versions

inline xiiStreamWriter& operator<<(xiiStreamWriter& ref_stream, bool bValue)
{
  xiiUInt8 uiValue = bValue ? 1 : 0;
  ref_stream.WriteBytes(&uiValue, sizeof(xiiUInt8)).AssertSuccess();
  return ref_stream;
}

inline xiiStreamReader& operator>>(xiiStreamReader& ref_stream, bool& ref_bValue)
{
  xiiUInt8 uiValue = 0;
  XII_VERIFY(ref_stream.ReadBytes(&uiValue, sizeof(xiiUInt8)) == sizeof(xiiUInt8), "End of stream reached.");
  ref_bValue = (uiValue != 0);
  return ref_stream;
}

/// Unsigned Integer versions

inline xiiStreamWriter& operator<<(xiiStreamWriter& ref_stream, xiiUInt8 uiValue)
{
  ref_stream.WriteBytes(&uiValue, sizeof(xiiUInt8)).AssertSuccess();
  return ref_stream;
}

inline xiiStreamReader& operator>>(xiiStreamReader& ref_stream, xiiUInt8& ref_uiValue)
{
  XII_VERIFY(ref_stream.ReadBytes(&ref_uiValue, sizeof(xiiUInt8)) == sizeof(xiiUInt8), "End of stream reached.");
  return ref_stream;
}

inline xiiResult SerializeArray(xiiStreamWriter& ref_stream, const xiiUInt8* pArray, xiiUInt64 uiCount)
{
  return ref_stream.WriteBytes(pArray, sizeof(xiiUInt8) * uiCount);
}

inline xiiResult DeserializeArray(xiiStreamReader& ref_stream, xiiUInt8* pArray, xiiUInt64 uiCount)
{
  const xiiUInt64 uiNumBytes = sizeof(xiiUInt8) * uiCount;
  if (ref_stream.ReadBytes(pArray, uiNumBytes) == uiNumBytes)
    return XII_SUCCESS;

  return XII_FAILURE;
}


inline xiiStreamWriter& operator<<(xiiStreamWriter& ref_stream, xiiUInt16 uiValue)
{
  ref_stream.WriteWordValue(&uiValue).AssertSuccess();
  return ref_stream;
}

inline xiiStreamReader& operator>>(xiiStreamReader& ref_stream, xiiUInt16& ref_uiValue)
{
  ref_stream.ReadWordValue(&ref_uiValue).AssertSuccess();
  return ref_stream;
}

inline xiiResult SerializeArray(xiiStreamWriter& ref_stream, const xiiUInt16* pArray, xiiUInt64 uiCount)
{
  return ref_stream.WriteBytes(pArray, sizeof(xiiUInt16) * uiCount);
}

inline xiiResult DeserializeArray(xiiStreamReader& ref_stream, xiiUInt16* pArray, xiiUInt64 uiCount)
{
  const xiiUInt64 uiNumBytes = sizeof(xiiUInt16) * uiCount;
  if (ref_stream.ReadBytes(pArray, uiNumBytes) == uiNumBytes)
    return XII_SUCCESS;

  return XII_FAILURE;
}


inline xiiStreamWriter& operator<<(xiiStreamWriter& ref_stream, xiiUInt32 uiValue)
{
  ref_stream.WriteDWordValue(&uiValue).AssertSuccess();
  return ref_stream;
}

inline xiiStreamReader& operator>>(xiiStreamReader& ref_stream, xiiUInt32& ref_uiValue)
{
  ref_stream.ReadDWordValue(&ref_uiValue).AssertSuccess();
  return ref_stream;
}

inline xiiResult SerializeArray(xiiStreamWriter& ref_stream, const xiiUInt32* pArray, xiiUInt64 uiCount)
{
  return ref_stream.WriteBytes(pArray, sizeof(xiiUInt32) * uiCount);
}

inline xiiResult DeserializeArray(xiiStreamReader& ref_stream, xiiUInt32* pArray, xiiUInt64 uiCount)
{
  const xiiUInt64 uiNumBytes = sizeof(xiiUInt32) * uiCount;
  if (ref_stream.ReadBytes(pArray, uiNumBytes) == uiNumBytes)
    return XII_SUCCESS;

  return XII_FAILURE;
}


inline xiiStreamWriter& operator<<(xiiStreamWriter& ref_stream, xiiUInt64 uiValue)
{
  ref_stream.WriteQWordValue(&uiValue).AssertSuccess();
  return ref_stream;
}

inline xiiStreamReader& operator>>(xiiStreamReader& ref_stream, xiiUInt64& ref_uiValue)
{
  ref_stream.ReadQWordValue(&ref_uiValue).AssertSuccess();
  return ref_stream;
}

inline xiiResult SerializeArray(xiiStreamWriter& ref_stream, const xiiUInt64* pArray, xiiUInt64 uiCount)
{
  return ref_stream.WriteBytes(pArray, sizeof(xiiUInt64) * uiCount);
}

inline xiiResult DeserializeArray(xiiStreamReader& ref_stream, xiiUInt64* pArray, xiiUInt64 uiCount)
{
  const xiiUInt64 uiNumBytes = sizeof(xiiUInt64) * uiCount;
  if (ref_stream.ReadBytes(pArray, uiNumBytes) == uiNumBytes)
    return XII_SUCCESS;

  return XII_FAILURE;
}

/// Signed Integer versions

inline xiiStreamWriter& operator<<(xiiStreamWriter& ref_stream, xiiInt8 iValue)
{
  ref_stream.WriteBytes(reinterpret_cast<const xiiUInt8*>(&iValue), sizeof(xiiInt8)).AssertSuccess();
  return ref_stream;
}

inline xiiStreamReader& operator>>(xiiStreamReader& ref_stream, xiiInt8& ref_iValue)
{
  XII_VERIFY(ref_stream.ReadBytes(reinterpret_cast<xiiUInt8*>(&ref_iValue), sizeof(xiiInt8)) == sizeof(xiiInt8), "End of stream reached.");
  return ref_stream;
}

inline xiiResult SerializeArray(xiiStreamWriter& ref_stream, const xiiInt8* pArray, xiiUInt64 uiCount)
{
  return ref_stream.WriteBytes(pArray, sizeof(xiiInt8) * uiCount);
}

inline xiiResult DeserializeArray(xiiStreamReader& ref_stream, xiiInt8* pArray, xiiUInt64 uiCount)
{
  const xiiUInt64 uiNumBytes = sizeof(xiiInt8) * uiCount;
  if (ref_stream.ReadBytes(pArray, uiNumBytes) == uiNumBytes)
    return XII_SUCCESS;

  return XII_FAILURE;
}


inline xiiStreamWriter& operator<<(xiiStreamWriter& ref_stream, xiiInt16 iValue)
{
  ref_stream.WriteWordValue(&iValue).AssertSuccess();
  return ref_stream;
}

inline xiiStreamReader& operator>>(xiiStreamReader& ref_stream, xiiInt16& ref_iValue)
{
  ref_stream.ReadWordValue(&ref_iValue).AssertSuccess();
  return ref_stream;
}

inline xiiResult SerializeArray(xiiStreamWriter& ref_stream, const xiiInt16* pArray, xiiUInt64 uiCount)
{
  return ref_stream.WriteBytes(pArray, sizeof(xiiInt16) * uiCount);
}

inline xiiResult DeserializeArray(xiiStreamReader& ref_stream, xiiInt16* pArray, xiiUInt64 uiCount)
{
  const xiiUInt64 uiNumBytes = sizeof(xiiInt16) * uiCount;
  if (ref_stream.ReadBytes(pArray, uiNumBytes) == uiNumBytes)
    return XII_SUCCESS;

  return XII_FAILURE;
}


inline xiiStreamWriter& operator<<(xiiStreamWriter& ref_stream, xiiInt32 iValue)
{
  ref_stream.WriteDWordValue(&iValue).AssertSuccess();
  return ref_stream;
}

inline xiiStreamReader& operator>>(xiiStreamReader& ref_stream, xiiInt32& ref_iValue)
{
  ref_stream.ReadDWordValue(&ref_iValue).AssertSuccess();
  return ref_stream;
}

inline xiiResult SerializeArray(xiiStreamWriter& ref_stream, const xiiInt32* pArray, xiiUInt64 uiCount)
{
  return ref_stream.WriteBytes(pArray, sizeof(xiiInt32) * uiCount);
}

inline xiiResult DeserializeArray(xiiStreamReader& ref_stream, xiiInt32* pArray, xiiUInt64 uiCount)
{
  const xiiUInt64 uiNumBytes = sizeof(xiiInt32) * uiCount;
  if (ref_stream.ReadBytes(pArray, uiNumBytes) == uiNumBytes)
    return XII_SUCCESS;

  return XII_FAILURE;
}


inline xiiStreamWriter& operator<<(xiiStreamWriter& ref_stream, xiiInt64 iValue)
{
  ref_stream.WriteQWordValue(&iValue).AssertSuccess();
  return ref_stream;
}

inline xiiStreamReader& operator>>(xiiStreamReader& ref_stream, xiiInt64& ref_iValue)
{
  ref_stream.ReadQWordValue(&ref_iValue).AssertSuccess();
  return ref_stream;
}

inline xiiResult SerializeArray(xiiStreamWriter& ref_stream, const xiiInt64* pArray, xiiUInt64 uiCount)
{
  return ref_stream.WriteBytes(pArray, sizeof(xiiInt64) * uiCount);
}

inline xiiResult DeserializeArray(xiiStreamReader& ref_stream, xiiInt64* pArray, xiiUInt64 uiCount)
{
  const xiiUInt64 uiNumBytes = sizeof(xiiInt64) * uiCount;
  if (ref_stream.ReadBytes(pArray, uiNumBytes) == uiNumBytes)
    return XII_SUCCESS;

  return XII_FAILURE;
}


/// Float and Double versions

inline xiiStreamWriter& operator<<(xiiStreamWriter& ref_stream, float fValue)
{
  ref_stream.WriteDWordValue(&fValue).AssertSuccess();
  return ref_stream;
}

inline xiiStreamReader& operator>>(xiiStreamReader& ref_stream, float& ref_fValue)
{
  ref_stream.ReadDWordValue(&ref_fValue).AssertSuccess();
  return ref_stream;
}

inline xiiResult SerializeArray(xiiStreamWriter& ref_stream, const float* pArray, xiiUInt64 uiCount)
{
  return ref_stream.WriteBytes(pArray, sizeof(float) * uiCount);
}

inline xiiResult DeserializeArray(xiiStreamReader& ref_stream, float* pArray, xiiUInt64 uiCount)
{
  const xiiUInt64 uiNumBytes = sizeof(float) * uiCount;
  if (ref_stream.ReadBytes(pArray, uiNumBytes) == uiNumBytes)
    return XII_SUCCESS;

  return XII_FAILURE;
}


inline xiiStreamWriter& operator<<(xiiStreamWriter& ref_stream, double fValue)
{
  ref_stream.WriteQWordValue(&fValue).AssertSuccess();
  return ref_stream;
}

inline xiiStreamReader& operator>>(xiiStreamReader& ref_stream, double& ref_fValue)
{
  ref_stream.ReadQWordValue(&ref_fValue).AssertSuccess();
  return ref_stream;
}

inline xiiResult SerializeArray(xiiStreamWriter& ref_stream, const double* pArray, xiiUInt64 uiCount)
{
  return ref_stream.WriteBytes(pArray, sizeof(double) * uiCount);
}

inline xiiResult DeserializeArray(xiiStreamReader& ref_stream, double* pArray, xiiUInt64 uiCount)
{
  const xiiUInt64 uiNumBytes = sizeof(double) * uiCount;
  if (ref_stream.ReadBytes(pArray, uiNumBytes) == uiNumBytes)
    return XII_SUCCESS;

  return XII_FAILURE;
}


// C-style strings
// No read equivalent for C-style strings (but can be read as xiiString & xiiStringBuilder instances)

XII_FOUNDATION_DLL xiiStreamWriter& operator<<(xiiStreamWriter& ref_stream, const char* szValue);
XII_FOUNDATION_DLL xiiStreamWriter& operator<<(xiiStreamWriter& ref_stream, xiiStringView sValue);

// xiiHybridString

template <xiiUInt16 Size, typename AllocatorWrapper>
inline xiiStreamWriter& operator<<(xiiStreamWriter& ref_stream, const xiiHybridString<Size, AllocatorWrapper>& sValue)
{
  ref_stream.WriteString(sValue.GetView()).AssertSuccess();
  return ref_stream;
}

template <xiiUInt16 Size, typename AllocatorWrapper>
inline xiiStreamReader& operator>>(xiiStreamReader& ref_stream, xiiHybridString<Size, AllocatorWrapper>& ref_sValue)
{
  xiiStringBuilder builder;
  ref_stream.ReadString(builder).AssertSuccess();
  ref_sValue = std::move(builder);

  return ref_stream;
}

// xiiStringBuilder

XII_FOUNDATION_DLL xiiStreamWriter& operator<<(xiiStreamWriter& ref_stream, const xiiStringBuilder& sValue);
XII_FOUNDATION_DLL xiiStreamReader& operator>>(xiiStreamReader& ref_stream, xiiStringBuilder& ref_sValue);

// xiiEnum

template <typename T>
inline xiiStreamWriter& operator<<(xiiStreamWriter& ref_stream, const xiiEnum<T>& value)
{
  ref_stream << value.GetValue();

  return ref_stream;
}

template <typename T>
inline xiiStreamReader& operator>>(xiiStreamReader& ref_stream, xiiEnum<T>& value)
{
  typename T::StorageType storedValue = T::Default;
  ref_stream >> storedValue;
  value.SetValue(storedValue);

  return ref_stream;
}

// xiiBitflags

template <typename T>
inline xiiStreamWriter& operator<<(xiiStreamWriter& ref_stream, const xiiBitflags<T>& value)
{
  ref_stream << value.GetValue();

  return ref_stream;
}

template <typename T>
inline xiiStreamReader& operator>>(xiiStreamReader& ref_stream, xiiBitflags<T>& value)
{
  typename T::StorageType storedValue = T::Default;
  ref_stream >> storedValue;
  value.SetValue(storedValue);

  return ref_stream;
}

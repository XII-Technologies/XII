#pragma once

#include <Foundation/Containers/HybridArray.h>
#include <Foundation/Strings/StringView.h>

#if XII_ENABLED(XII_PLATFORM_WINDOWS_UWP)
// Include our windows.h header first to get rid of defines.
#  include <Foundation/Basics/Platform/Win/IncludeWindows.h>
// For HString, HStringReference and co.
#  include <wrl/wrappers/corewrappers.h>
#endif

/// \brief A very simple string class that should only be used to temporarily convert text to the OSes native wchar_t convention (16 or 32
/// Bit).
///
/// This should be used when one needs to output text via some function that only accepts wchar_t strings.
/// DO NOT use this for storage or anything else that is not temporary.
/// wchar_t is 16 Bit on Windows and 32 Bit on most other platforms. This class will always automatically convert to the correct format.
class XII_FOUNDATION_DLL xiiStringWChar
{
public:
  xiiStringWChar(xiiAllocatorBase* pAllocator = xiiFoundation::GetDefaultAllocator());
  xiiStringWChar(const xiiUInt16* pUtf16, xiiAllocatorBase* pAllocator = xiiFoundation::GetDefaultAllocator());
  xiiStringWChar(const xiiUInt32* pUtf32, xiiAllocatorBase* pAllocator = xiiFoundation::GetDefaultAllocator());
  xiiStringWChar(const wchar_t* pUtf32, xiiAllocatorBase* pAllocator = xiiFoundation::GetDefaultAllocator());
  xiiStringWChar(xiiStringView sUtf8, xiiAllocatorBase* pAllocator = xiiFoundation::GetDefaultAllocator());

  void operator=(const xiiUInt16* pUtf16);
  void operator=(const xiiUInt32* pUtf32);
  void operator=(const wchar_t* pUtf32);
  void operator=(xiiStringView sUtf8);

  XII_ALWAYS_INLINE       operator const wchar_t*() const { return &m_Data[0]; }
  XII_ALWAYS_INLINE const wchar_t* GetData() const { return &m_Data[0]; }
  XII_ALWAYS_INLINE xiiUInt32      GetElementCount() const { return m_Data.GetCount() - 1; /* exclude the '\0' terminator */ }

private:
  static constexpr xiiUInt32          BufferSize = 1024;
  xiiHybridArray<wchar_t, BufferSize> m_Data;
};


/// \brief A small string class that converts any other encoding to Utf8.
///
/// Use this class only temporarily. Do not use it for storage.
class XII_FOUNDATION_DLL xiiStringUtf8
{
public:
  xiiStringUtf8(xiiAllocatorBase* pAllocator = xiiFoundation::GetDefaultAllocator());
  xiiStringUtf8(const char* szUtf8, xiiAllocatorBase* pAllocator = xiiFoundation::GetDefaultAllocator());
  xiiStringUtf8(const xiiUInt16* pUtf16, xiiAllocatorBase* pAllocator = xiiFoundation::GetDefaultAllocator());
  xiiStringUtf8(const xiiUInt32* pUtf32, xiiAllocatorBase* pAllocator = xiiFoundation::GetDefaultAllocator());
  xiiStringUtf8(const wchar_t* pWChar, xiiAllocatorBase* pAllocator = xiiFoundation::GetDefaultAllocator());

#if XII_ENABLED(XII_PLATFORM_WINDOWS_UWP)
  xiiStringUtf8(const Microsoft::WRL::Wrappers::HString& hstring, xiiAllocatorBase* pAllocator = xiiFoundation::GetDefaultAllocator());
  xiiStringUtf8(const HSTRING& hstring, xiiAllocatorBase* pAllocator = xiiFoundation::GetDefaultAllocator());
#endif

  void operator=(const char* szUtf8);
  void operator=(const xiiUInt16* pUtf16);
  void operator=(const xiiUInt32* pUtf32);
  void operator=(const wchar_t* pWChar);

#if XII_ENABLED(XII_PLATFORM_WINDOWS_UWP)
  void operator=(const Microsoft::WRL::Wrappers::HString& hstring);
  void operator=(const HSTRING& hstring);
#endif

  XII_ALWAYS_INLINE operator const char*() const
  {
    return &m_Data[0];
  }
  XII_ALWAYS_INLINE const char* GetData() const
  {
    return &m_Data[0];
  }
  XII_ALWAYS_INLINE xiiUInt32 GetElementCount() const
  {
    return m_Data.GetCount() - 1; /* exclude the '\0' terminator */
  }
  XII_ALWAYS_INLINE operator xiiStringView() const
  {
    return GetView();
  }
  XII_ALWAYS_INLINE xiiStringView GetView() const
  {
    return xiiStringView(&m_Data[0], GetElementCount());
  }

private:
  static constexpr xiiUInt32       BufferSize = 1024;
  xiiHybridArray<char, BufferSize> m_Data;
};



/// \brief A very simple class to convert text to Utf16 encoding.
///
/// Use this class only temporarily, if you need to output something in Utf16 format, e.g. for writing it to a file.
/// Never use this for storage.
/// When working with OS functions that expect '16 Bit strings', use xiiStringWChar instead.
class XII_FOUNDATION_DLL xiiStringUtf16
{
public:
  xiiStringUtf16(xiiAllocatorBase* pAllocator = xiiFoundation::GetDefaultAllocator());
  xiiStringUtf16(const char* szUtf8, xiiAllocatorBase* pAllocator = xiiFoundation::GetDefaultAllocator());
  xiiStringUtf16(const xiiUInt16* pUtf16, xiiAllocatorBase* pAllocator = xiiFoundation::GetDefaultAllocator());
  xiiStringUtf16(const xiiUInt32* pUtf32, xiiAllocatorBase* pAllocator = xiiFoundation::GetDefaultAllocator());
  xiiStringUtf16(const wchar_t* pUtf32, xiiAllocatorBase* pAllocator = xiiFoundation::GetDefaultAllocator());

  void operator=(const char* szUtf8);
  void operator=(const xiiUInt16* pUtf16);
  void operator=(const xiiUInt32* pUtf32);
  void operator=(const wchar_t* pUtf32);

  XII_ALWAYS_INLINE const xiiUInt16* GetData() const { return &m_Data[0]; }
  XII_ALWAYS_INLINE xiiUInt32        GetElementCount() const { return m_Data.GetCount() - 1; /* exclude the '\0' terminator */ }

private:
  static constexpr xiiUInt32            BufferSize = 1024;
  xiiHybridArray<xiiUInt16, BufferSize> m_Data;
};



/// \brief This class only exists for completeness.
///
/// There should be no case where it is preferred over other classes.
class XII_FOUNDATION_DLL xiiStringUtf32
{
public:
  xiiStringUtf32(xiiAllocatorBase* pAllocator = xiiFoundation::GetDefaultAllocator());
  xiiStringUtf32(const char* szUtf8, xiiAllocatorBase* pAllocator = xiiFoundation::GetDefaultAllocator());
  xiiStringUtf32(const xiiUInt16* pUtf16, xiiAllocatorBase* pAllocator = xiiFoundation::GetDefaultAllocator());
  xiiStringUtf32(const xiiUInt32* pUtf32, xiiAllocatorBase* pAllocator = xiiFoundation::GetDefaultAllocator());
  xiiStringUtf32(const wchar_t* pWChar, xiiAllocatorBase* pAllocator = xiiFoundation::GetDefaultAllocator());

  void operator=(const char* szUtf8);
  void operator=(const xiiUInt16* pUtf16);
  void operator=(const xiiUInt32* pUtf32);
  void operator=(const wchar_t* pWChar);

  XII_ALWAYS_INLINE const xiiUInt32* GetData() const { return &m_Data[0]; }
  XII_ALWAYS_INLINE xiiUInt32        GetElementCount() const { return m_Data.GetCount() - 1; /* exclude the '\0' terminator */ }

private:
  static constexpr xiiUInt32            BufferSize = 1024;
  xiiHybridArray<xiiUInt32, BufferSize> m_Data;
};


#if XII_ENABLED(XII_PLATFORM_WINDOWS_UWP)

/// \brief A very simple string class that should only be used to temporarily convert text to the OSes native HString (on UWP platforms).
///
/// This should be used when one needs to output text via some function that only accepts HString strings.
/// DO NOT use this for storage or anything else that is not temporary.
class XII_FOUNDATION_DLL xiiStringHString
{
public:
  xiiStringHString();
  xiiStringHString(const char* szUtf8);
  xiiStringHString(const xiiUInt16* szUtf16);
  xiiStringHString(const xiiUInt32* szUtf32);
  xiiStringHString(const wchar_t* szWChar);

  void operator=(const char* szUtf8);
  void operator=(const xiiUInt16* szUtf16);
  void operator=(const xiiUInt32* szUtf32);
  void operator=(const wchar_t* szWChar);

  /// \brief Unfortunately you cannot assign HStrings, so you cannot copy the result to another HString, you have to use this result
  /// directly
  XII_ALWAYS_INLINE const Microsoft::WRL::Wrappers::HString& GetData() const { return m_Data; }

private:
  Microsoft::WRL::Wrappers::HString m_Data;
};

#endif


#include <Foundation/Strings/Implementation/StringConversion_inl.h>

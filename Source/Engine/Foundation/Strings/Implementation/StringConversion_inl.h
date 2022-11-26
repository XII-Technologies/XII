#pragma once

#include <Foundation/ThirdParty/utf8/utf8.h>

#include <Foundation/Strings/UnicodeUtils.h>

// **************** xiiStringWChar ****************

inline xiiStringWChar::xiiStringWChar(xiiAllocatorBase* pAllocator) :
  m_Data(pAllocator)
{
  m_Data.PushBack('\0');
}

inline xiiStringWChar::xiiStringWChar(const xiiUInt16* szUtf16, xiiAllocatorBase* pAllocator) :
  m_Data(pAllocator)
{
  *this = szUtf16;
}

inline xiiStringWChar::xiiStringWChar(const xiiUInt32* szUtf32, xiiAllocatorBase* pAllocator) :
  m_Data(pAllocator)
{
  *this = szUtf32;
}

inline xiiStringWChar::xiiStringWChar(const wchar_t* szWChar, xiiAllocatorBase* pAllocator) :
  m_Data(pAllocator)
{
  *this = szWChar;
}

inline xiiStringWChar::xiiStringWChar(xiiStringView sUtf8, xiiAllocatorBase* pAllocator /*= xiiFoundation::GetDefaultAllocator()*/)
{
  *this = sUtf8;
}


// **************** xiiStringUtf8 ****************

inline xiiStringUtf8::xiiStringUtf8(xiiAllocatorBase* pAllocator) :
  m_Data(pAllocator)
{
  m_Data.PushBack('\0');
}

inline xiiStringUtf8::xiiStringUtf8(const char* szUtf8, xiiAllocatorBase* pAllocator) :
  m_Data(pAllocator)
{
  *this = szUtf8;
}

inline xiiStringUtf8::xiiStringUtf8(const xiiUInt16* szUtf16, xiiAllocatorBase* pAllocator) :
  m_Data(pAllocator)
{
  *this = szUtf16;
}

inline xiiStringUtf8::xiiStringUtf8(const xiiUInt32* szUtf32, xiiAllocatorBase* pAllocator) :
  m_Data(pAllocator)
{
  *this = szUtf32;
}

inline xiiStringUtf8::xiiStringUtf8(const wchar_t* szWChar, xiiAllocatorBase* pAllocator) :
  m_Data(pAllocator)
{
  *this = szWChar;
}

#if XII_ENABLED(XII_PLATFORM_WINDOWS_UWP)

inline xiiStringUtf8::xiiStringUtf8(
  const Microsoft::WRL::Wrappers::HString& hstring,
  xiiAllocatorBase*                        pAllocator /*= xiiFoundation::GetDefaultAllocator()*/) :
  m_Data(pAllocator)
{
  *this = hstring;
}

inline xiiStringUtf8::xiiStringUtf8(const HSTRING& hstring, xiiAllocatorBase* pAllocator /*= xiiFoundation::GetDefaultAllocator()*/)
{
  *this = hstring;
}

#endif

// **************** xiiStringUtf16 ****************

inline xiiStringUtf16::xiiStringUtf16(xiiAllocatorBase* pAllocator) :
  m_Data(pAllocator)
{
  m_Data.PushBack('\0');
}

inline xiiStringUtf16::xiiStringUtf16(const char* szUtf8, xiiAllocatorBase* pAllocator) :
  m_Data(pAllocator)
{
  *this = szUtf8;
}

inline xiiStringUtf16::xiiStringUtf16(const xiiUInt16* szUtf16, xiiAllocatorBase* pAllocator) :
  m_Data(pAllocator)
{
  *this = szUtf16;
}

inline xiiStringUtf16::xiiStringUtf16(const xiiUInt32* szUtf32, xiiAllocatorBase* pAllocator) :
  m_Data(pAllocator)
{
  *this = szUtf32;
}

inline xiiStringUtf16::xiiStringUtf16(const wchar_t* szWChar, xiiAllocatorBase* pAllocator) :
  m_Data(pAllocator)
{
  *this = szWChar;
}



// **************** xiiStringUtf32 ****************

inline xiiStringUtf32::xiiStringUtf32(xiiAllocatorBase* pAllocator) :
  m_Data(pAllocator)
{
  m_Data.PushBack('\0');
}

inline xiiStringUtf32::xiiStringUtf32(const char* szUtf8, xiiAllocatorBase* pAllocator) :
  m_Data(pAllocator)
{
  *this = szUtf8;
}

inline xiiStringUtf32::xiiStringUtf32(const xiiUInt16* szUtf16, xiiAllocatorBase* pAllocator) :
  m_Data(pAllocator)
{
  *this = szUtf16;
}

inline xiiStringUtf32::xiiStringUtf32(const xiiUInt32* szUtf32, xiiAllocatorBase* pAllocator) :
  m_Data(pAllocator)
{
  *this = szUtf32;
}

inline xiiStringUtf32::xiiStringUtf32(const wchar_t* szWChar, xiiAllocatorBase* pAllocator) :
  m_Data(pAllocator)
{
  *this = szWChar;
}

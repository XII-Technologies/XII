/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/IO/Stream.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Strings/StringBuilder.h>

template <xiiUInt16 Size>
xiiHybridStringBase<Size>::xiiHybridStringBase(const xiiStringBuilder& rhs, xiiAllocator* pAllocator) :
  m_Data(pAllocator)
{
  *this = rhs;
}

template <xiiUInt16 Size>
xiiHybridStringBase<Size>::xiiHybridStringBase(xiiStringBuilder&& rhs, xiiAllocator* pAllocator) :
  m_Data(pAllocator)
{
  *this = std::move(rhs);
}

template <xiiUInt16 Size, typename A>
XII_ALWAYS_INLINE xiiHybridString<Size, A>::xiiHybridString(const xiiStringBuilder& rhs) :
  xiiHybridStringBase<Size>(rhs, A::GetAllocator())
{
}

template <xiiUInt16 Size, typename A>
XII_ALWAYS_INLINE xiiHybridString<Size, A>::xiiHybridString(xiiStringBuilder&& rhs) :
  xiiHybridStringBase<Size>(std::move(rhs), A::GetAllocator())
{
}

template <xiiUInt16 Size>
void xiiHybridStringBase<Size>::operator=(const xiiStringBuilder& rhs)
{
  m_Data = rhs.m_Data;
}

template <xiiUInt16 Size>
void xiiHybridStringBase<Size>::operator=(xiiStringBuilder&& rhs)
{
  m_Data = std::move(rhs.m_Data);
}

template <xiiUInt16 Size, typename A>
XII_ALWAYS_INLINE void xiiHybridString<Size, A>::operator=(const xiiStringBuilder& rhs)
{
  xiiHybridStringBase<Size>::operator=(rhs);
}

template <xiiUInt16 Size, typename A>
XII_ALWAYS_INLINE void xiiHybridString<Size, A>::operator=(xiiStringBuilder&& rhs)
{
  xiiHybridStringBase<Size>::operator=(std::move(rhs));
}

template <xiiUInt16 Size>
void xiiHybridStringBase<Size>::ReadAll(xiiStreamReader& ref_stream)
{
  Clear();

  xiiHybridArray<xiiUInt8, 1024 * 4> Bytes(m_Data.GetAllocator());
  xiiUInt8                           Temp[1024];

  while (true)
  {
    const xiiUInt32 uiRead = (xiiUInt32)ref_stream.ReadBytes(Temp, 1024);

    if (uiRead == 0)
      break;

    Bytes.PushBackRange(xiiArrayPtr<xiiUInt8>(Temp, uiRead));
  }

  Bytes.PushBack('\0');

  *this = (const char*)&Bytes[0];
}

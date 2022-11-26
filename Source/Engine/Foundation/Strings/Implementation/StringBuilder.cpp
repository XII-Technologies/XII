#include <Foundation/FoundationPCH.h>

#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Strings/FormatString.h>
#include <Foundation/Strings/StringBuilder.h>

#include <stdarg.h>

xiiStringBuilder::xiiStringBuilder(xiiStringView pData1, xiiStringView pData2, xiiStringView pData3, xiiStringView pData4, xiiStringView pData5, xiiStringView pData6)
{
  m_uiCharacterCount = 0;
  AppendTerminator();

  Append(pData1, pData2, pData3, pData4, pData5, pData6);
}

void xiiStringBuilder::Set(xiiStringView pData1, xiiStringView pData2, xiiStringView pData3, xiiStringView pData4, xiiStringView pData5, xiiStringView pData6)
{
  Clear();
  Append(pData1, pData2, pData3, pData4, pData5, pData6);
}

void xiiStringBuilder::SetSubString_FromTo(const char* pStart, const char* pEnd)
{
  XII_ASSERT_DEBUG(xiiUnicodeUtils::IsValidUtf8(pStart, pEnd), "Invalid substring, the start does not point to a valid Utf-8 character");

  xiiStringView view(pStart, pEnd);
  *this = view;
}

void xiiStringBuilder::SetSubString_ElementCount(const char* pStart, xiiUInt32 uiElementCount)
{
  XII_ASSERT_DEBUG(
    xiiUnicodeUtils::IsValidUtf8(pStart, pStart + uiElementCount), "Invalid substring, the start does not point to a valid Utf-8 character");

  xiiStringView view(pStart, pStart + uiElementCount);
  *this = view;
}

void xiiStringBuilder::SetSubString_CharacterCount(const char* pStart, xiiUInt32 uiCharacterCount)
{
  const char* pEnd = pStart;
  xiiUnicodeUtils::MoveToNextUtf8(pEnd, uiCharacterCount);

  xiiStringView view(pStart, pEnd);
  *this = view;
}

void xiiStringBuilder::Append(xiiStringView pData1, xiiStringView pData2, xiiStringView pData3, xiiStringView pData4, xiiStringView pData5, xiiStringView pData6)
{
  // it is not possible to find out how many parameters were passed to a vararg function
  // with a fixed size of parameters we do not need to have a parameter that tells us how many strings will come

  const xiiUInt32 uiMaxParams = 6;

  const xiiStringView pStrings[uiMaxParams] = {pData1, pData2, pData3, pData4, pData5, pData6};
  xiiUInt32           uiStrLen[uiMaxParams] = {0};

  xiiUInt32 uiMoreBytes = 0;

  // first figure out how much the string has to grow
  for (xiiUInt32 i = 0; i < uiMaxParams; ++i)
  {
    if (pStrings[i].IsEmpty())
      continue;

    XII_ASSERT_DEBUG(pStrings[i].GetStartPointer() < m_Data.GetData() || pStrings[i].GetStartPointer() >= m_Data.GetData() + m_Data.GetCapacity(),
                     "Parameter {0} comes from the string builders own storage. This type assignment is not allowed.", i);

    xiiUInt32 uiCharacters = 0;
    xiiStringUtils::GetCharacterAndElementCount(pStrings[i].GetStartPointer(), uiCharacters, uiStrLen[i], pStrings[i].GetEndPointer());
    uiMoreBytes += uiStrLen[i];
    m_uiCharacterCount += uiCharacters;

    XII_ASSERT_DEBUG(xiiUnicodeUtils::IsValidUtf8(pStrings[i].GetStartPointer(), pStrings[i].GetEndPointer()), "Parameter {0} is not a valid Utf8 sequence.", i + 1);
  }

  xiiUInt32 uiPrevCount = m_Data.GetCount(); // already contains a 0 terminator
  XII_ASSERT_DEBUG(uiPrevCount > 0, "There should be a 0 terminator somewhere around here.");

  // now resize
  m_Data.SetCountUninitialized(uiPrevCount + uiMoreBytes);

  // and then append all the strings
  for (xiiUInt32 i = 0; i < uiMaxParams; ++i)
  {
    if (uiStrLen[i] == 0)
      continue;

    // make enough room to copy the entire string, including the T-800
    xiiStringUtils::Copy(&m_Data[uiPrevCount - 1], uiStrLen[i] + 1, pStrings[i].GetStartPointer(), pStrings[i].GetStartPointer() + uiStrLen[i]);

    uiPrevCount += uiStrLen[i];
  }
}

void xiiStringBuilder::Prepend(xiiStringView pData1, xiiStringView pData2, xiiStringView pData3, xiiStringView pData4, xiiStringView pData5, xiiStringView pData6)
{
  // it is not possible to find out how many parameters were passed to a vararg function
  // with a fixed size of parameters we do not need to have a parameter that tells us how many strings will come

  const xiiUInt32 uiMaxParams = 6;

  const xiiStringView pStrings[uiMaxParams] = {pData1, pData2, pData3, pData4, pData5, pData6};
  xiiUInt32           uiStrLen[uiMaxParams] = {0};

  xiiUInt32 uiMoreBytes = 0;

  // first figure out how much the string has to grow
  for (xiiUInt32 i = 0; i < uiMaxParams; ++i)
  {
    if (pStrings[i].IsEmpty())
      continue;

    xiiUInt32 uiCharacters = 0;
    xiiStringUtils::GetCharacterAndElementCount(pStrings[i].GetStartPointer(), uiCharacters, uiStrLen[i], pStrings[i].GetEndPointer());
    uiMoreBytes += uiStrLen[i];
    m_uiCharacterCount += uiCharacters;

    XII_ASSERT_DEBUG(xiiUnicodeUtils::IsValidUtf8(pStrings[i].GetStartPointer(), pStrings[i].GetEndPointer()), "Parameter {0} is not a valid Utf8 sequence.", i + 1);
  }

  xiiUInt32 uiPrevCount = m_Data.GetCount(); // already contains a 0 terminator
  XII_ASSERT_DEBUG(uiPrevCount > 0, "There should be a 0 terminator somewhere around here.");

  // now resize
  m_Data.SetCountUninitialized(uiPrevCount + uiMoreBytes);

  // move the previous string data at the end
  xiiMemoryUtils::CopyOverlapped(&m_Data[0] + uiMoreBytes, GetData(), uiPrevCount);

  xiiUInt32 uiWritePos = 0;

  // and then prepend all the strings
  for (xiiUInt32 i = 0; i < uiMaxParams; ++i)
  {
    if (uiStrLen[i] == 0)
      continue;

    // make enough room to copy the entire string, including the T-800
    xiiMemoryUtils::Copy(&m_Data[uiWritePos], pStrings[i].GetStartPointer(), uiStrLen[i]);

    uiWritePos += uiStrLen[i];
  }
}

void xiiStringBuilder::PrintfArgs(const char* szUtf8Format, va_list args0)
{
  va_list args;
  va_copy(args, args0);

  Clear();

  const xiiUInt32 TempBuffer = 4096;

  char           szTemp[TempBuffer];
  const xiiInt32 iCount = xiiStringUtils::vsnprintf(szTemp, TempBuffer - 1, szUtf8Format, args);

  XII_ASSERT_DEV(iCount != -1, "There was an error while formatting the string. Probably and unescaped usage of the %% sign.");

  if (iCount == -1)
  {
    va_end(args);
    return;
  }

  if (iCount > TempBuffer - 1)
  {
    xiiDynamicArray<char> Temp;
    Temp.SetCountUninitialized(iCount + 1);

    xiiStringUtils::vsnprintf(&Temp[0], iCount + 1, szUtf8Format, args);

    Append(&Temp[0]);
  }
  else
  {
    Append(&szTemp[0]);
  }

  va_end(args);
}

void xiiStringBuilder::ChangeCharacterNonASCII(iterator& it, xiiUInt32 uiCharacter)
{
  char* pPos = const_cast<char*>(it.GetData()); // yes, I know...

  const xiiUInt32 uiOldCharLength = xiiUnicodeUtils::GetUtf8SequenceLength(*pPos);
  const xiiUInt32 uiNewCharLength = xiiUnicodeUtils::GetSizeForCharacterInUtf8(uiCharacter);

  // if the old character and the new one are encoded with the same length, we can replace the character in-place
  if (uiNewCharLength == uiOldCharLength)
  {
    // just overwrite all characters at the given position with the new Utf8 string
    xiiUnicodeUtils::EncodeUtf32ToUtf8(uiCharacter, pPos);

    // if the encoding length is identical, this will also handle all ASCII strings
    // if the string was pure ASCII before, this won't change, so no need to update that state
    return;
  }

  // in this case we can still update the string without reallocation, but the tail of the string has to be moved forwards
  if (uiNewCharLength < uiOldCharLength)
  {
    // just overwrite all characters at the given position with the new Utf8 string
    xiiUnicodeUtils::EncodeUtf32ToUtf8(uiCharacter, pPos);

    // pPos will be changed (moved forwards) to the next character position

    // how much has changed
    const xiiUInt32 uiDifference       = uiOldCharLength - uiNewCharLength;
    const xiiUInt32 uiTrailStringBytes = (xiiUInt32)(GetData() + GetElementCount() - it.GetData() - uiOldCharLength + 1); // ???

    // move the trailing characters forwards
    xiiMemoryUtils::CopyOverlapped(pPos, pPos + uiDifference, uiTrailStringBytes);

    // update the data array
    m_Data.PopBack(uiDifference);

    // 'It' references this already, no need to change anything.
  }
  else
  {
    // in this case we insert a character that is longer int Utf8 encoding than the character that already exists there *sigh*
    // so we must first move the trailing string backwards to make room, then we can write the new char in there

    // how much has changed
    const xiiUInt32 uiDifference       = uiNewCharLength - uiOldCharLength;
    const xiiUInt32 uiTrailStringBytes = (xiiUInt32)(GetData() + GetElementCount() - it.GetData() - uiOldCharLength + 1);
    auto            iCurrentPos        = (it.GetData() - GetData());
    // resize the array
    m_Data.SetCountUninitialized(m_Data.GetCount() + uiDifference);

    // these might have changed (array realloc)
    pPos = &m_Data[0] + iCurrentPos;
    it.SetCurrentPosition(pPos);

    // move the trailing string backwards
    xiiMemoryUtils::CopyOverlapped(pPos + uiNewCharLength, pPos + uiOldCharLength, uiTrailStringBytes);

    // just overwrite all characters at the given position with the new Utf8 string
    xiiUnicodeUtils::EncodeUtf32ToUtf8(uiCharacter, pPos);
  }
}

void xiiStringBuilder::Shrink(xiiUInt32 uiShrinkCharsFront, xiiUInt32 uiShrinkCharsBack)
{
  if (uiShrinkCharsFront + uiShrinkCharsBack >= m_uiCharacterCount)
  {
    Clear();
    return;
  }

  const char* szNewStart = &m_Data[0];

  if (IsPureASCII())
  {
    if (uiShrinkCharsBack > 0)
    {
      m_Data.PopBack(uiShrinkCharsBack + 1);
      AppendTerminator();
    }

    szNewStart = &m_Data[uiShrinkCharsFront];
  }
  else
  {
    if (uiShrinkCharsBack > 0)
    {
      const char* szEnd    = GetData() + GetElementCount();
      const char* szNewEnd = szEnd;
      xiiUnicodeUtils::MoveToPriorUtf8(szNewEnd, uiShrinkCharsBack);

      const xiiUInt32 uiLessBytes = (xiiUInt32)(szEnd - szNewEnd);

      m_Data.PopBack(uiLessBytes + 1);
      AppendTerminator();
    }

    xiiUnicodeUtils::MoveToNextUtf8(szNewStart, uiShrinkCharsFront);
  }

  if (szNewStart > &m_Data[0])
  {
    const xiiUInt32 uiLessBytes = (xiiUInt32)(szNewStart - &m_Data[0]);

    xiiMemoryUtils::CopyOverlapped(&m_Data[0], szNewStart, m_Data.GetCount() - uiLessBytes);
    m_Data.PopBack(uiLessBytes);
  }

  m_uiCharacterCount -= uiShrinkCharsFront;
  m_uiCharacterCount -= uiShrinkCharsBack;
}

void xiiStringBuilder::ReplaceSubString(const char* szStartPos, const char* szEndPos, xiiStringView szReplaceWith)
{
  XII_ASSERT_DEV(xiiMath::IsInRange(szStartPos, GetData(), GetData() + m_Data.GetCount()), "szStartPos is not inside this string.");
  XII_ASSERT_DEV(xiiMath::IsInRange(szEndPos, GetData(), GetData() + m_Data.GetCount()), "szEndPos is not inside this string.");
  XII_ASSERT_DEV(szStartPos <= szEndPos, "xiiStartPos must be before xiiEndPos");

  xiiUInt32 uiWordChars = 0;
  xiiUInt32 uiWordBytes = 0;
  xiiStringUtils::GetCharacterAndElementCount(szReplaceWith.GetStartPointer(), uiWordChars, uiWordBytes, szReplaceWith.GetEndPointer());

  const xiiUInt32 uiSubStringBytes = (xiiUInt32)(szEndPos - szStartPos);

  char*       szWritePos = const_cast<char*>(szStartPos); // szStartPos points into our own data anyway
  const char* szReadPos  = szReplaceWith.GetStartPointer();

  // most simple case, just replace characters
  if (uiSubStringBytes == uiWordBytes)
  {
    while (szWritePos < szEndPos)
    {
      if (!xiiUnicodeUtils::IsUtf8ContinuationByte(*szWritePos))
        --m_uiCharacterCount;

      *szWritePos = *szReadPos;
      ++szWritePos;
      ++szReadPos;
    }

    // the number of bytes might be identical, but that does not mean that the number of characters is also identical
    // therefore we subtract the number of characters that were found in the old substring
    // and add the number of characters for the new substring
    m_uiCharacterCount += uiWordChars;
    return;
  }

  // the replacement is shorter than the existing stuff -> move characters to the left, no reallocation needed
  if (uiWordBytes < uiSubStringBytes)
  {
    m_uiCharacterCount -= xiiStringUtils::GetCharacterCount(szStartPos, szEndPos);
    m_uiCharacterCount += uiWordChars;

    // first copy the replacement to the correct position
    xiiMemoryUtils::Copy(szWritePos, szReplaceWith.GetStartPointer(), uiWordBytes);

    const xiiUInt32 uiDifference = uiSubStringBytes - uiWordBytes;

    const char* szStringEnd = GetData() + m_Data.GetCount();

    // now move all the characters from behind the replaced string to the correct position
    xiiMemoryUtils::CopyOverlapped(szWritePos + uiWordBytes, szWritePos + uiSubStringBytes, szStringEnd - (szWritePos + uiSubStringBytes));

    m_Data.PopBack(uiDifference);

    return;
  }

  // else the replacement is longer than the existing word
  {
    m_uiCharacterCount -= xiiStringUtils::GetCharacterCount(szStartPos, szEndPos);
    m_uiCharacterCount += uiWordChars;

    const xiiUInt32 uiDifference            = uiWordBytes - uiSubStringBytes;
    const xiiUInt64 uiRelativeWritePosition = szWritePos - GetData();
    const xiiUInt64 uiDataByteCountBefore   = m_Data.GetCount();

    m_Data.SetCountUninitialized(m_Data.GetCount() + uiDifference);

    // all pointer are now possibly invalid since the data may be reallocated!
    szWritePos              = const_cast<char*>(GetData()) + uiRelativeWritePosition;
    const char* szStringEnd = GetData() + uiDataByteCountBefore;

    // first move the characters to the proper position from back to front
    xiiMemoryUtils::CopyOverlapped(szWritePos + uiWordBytes, szWritePos + uiSubStringBytes, szStringEnd - (szWritePos + uiSubStringBytes));

    // now copy the replacement to the correct position
    xiiMemoryUtils::Copy(szWritePos, szReplaceWith.GetStartPointer(), uiWordBytes);
  }
}

const char* xiiStringBuilder::ReplaceFirst(xiiStringView sSearchFor, xiiStringView sReplacement, const char* szStartSearchAt)
{
  if (szStartSearchAt == nullptr)
    szStartSearchAt = GetData();
  else
  {
    XII_ASSERT_DEV(xiiMath::IsInRange(szStartSearchAt, GetData(), GetData() + m_Data.GetCount() - 1), "szStartSearchAt is not inside the string range.");
  }

  const char* szFoundAt = xiiStringUtils::FindSubString(szStartSearchAt, sSearchFor.GetStartPointer(), GetData() + m_Data.GetCount() - 1, sSearchFor.GetEndPointer());

  if (szFoundAt == nullptr)
    return nullptr;

  const xiiUInt32 uiOffset = (xiiUInt32)(szFoundAt - GetData());

  const xiiUInt32 uiSearchStrLength = sSearchFor.GetElementCount();

  ReplaceSubString(szFoundAt, szFoundAt + uiSearchStrLength, sReplacement);

  return GetData() + uiOffset; // memory might have been reallocated
}

const char* xiiStringBuilder::ReplaceLast(xiiStringView sSearchFor, xiiStringView sReplacement, const char* szStartSearchAt)
{
  if (szStartSearchAt == nullptr)
    szStartSearchAt = GetData() + m_Data.GetCount() - 1;
  else
  {
    XII_ASSERT_DEV(xiiMath::IsInRange(szStartSearchAt, GetData(), GetData() + m_Data.GetCount() - 1), "szStartSearchAt is not inside the string range.");
  }

  const char* szFoundAt = xiiStringUtils::FindLastSubString(GetData(), sSearchFor.GetStartPointer(), szStartSearchAt, GetData() + m_Data.GetCount() - 1, sSearchFor.GetEndPointer());

  if (szFoundAt == nullptr)
    return nullptr;

  const xiiUInt32 uiOffset = (xiiUInt32)(szFoundAt - GetData());

  const xiiUInt32 uiSearchStrLength = sSearchFor.GetElementCount();

  ReplaceSubString(szFoundAt, szFoundAt + uiSearchStrLength, sReplacement);

  return GetData() + uiOffset; // memory might have been reallocated
}

xiiUInt32 xiiStringBuilder::ReplaceAll(xiiStringView sSearchFor, xiiStringView sReplacement)
{
  const xiiUInt32 uiSearchBytes = sSearchFor.GetElementCount();
  const xiiUInt32 uiWordBytes   = sReplacement.GetElementCount();

  xiiUInt32 uiReplacements = 0;
  xiiUInt32 uiOffset       = 0;

  while (true)
  {
    // during ReplaceSubString the string data might get reallocated and the memory addresses do not stay valid
    // so we need to work with offsets and recompute the pointers every time
    const char* szFoundAt = xiiStringUtils::FindSubString(GetData() + uiOffset, sSearchFor.GetStartPointer(), GetData() + m_Data.GetCount() - 1, sSearchFor.GetEndPointer());

    if (szFoundAt == nullptr)
      return uiReplacements;

    // do not search withing the replaced part, otherwise we get recursive replacement which will not end
    uiOffset = static_cast<xiiUInt32>(szFoundAt - GetData()) + uiWordBytes;

    ReplaceSubString(szFoundAt, szFoundAt + uiSearchBytes, sReplacement);

    ++uiReplacements;
  }

  return uiReplacements;
}


const char* xiiStringBuilder::ReplaceFirst_NoCase(xiiStringView sSearchFor, xiiStringView sReplacement, const char* szStartSearchAt)
{
  if (szStartSearchAt == nullptr)
    szStartSearchAt = GetData();
  else
  {
    XII_ASSERT_DEV(xiiMath::IsInRange(szStartSearchAt, GetData(), GetData() + m_Data.GetCount() - 1), "szStartSearchAt is not inside the string range.");
  }

  const char* szFoundAt = xiiStringUtils::FindSubString_NoCase(szStartSearchAt, sSearchFor.GetStartPointer(), GetData() + m_Data.GetCount() - 1, sSearchFor.GetEndPointer());

  if (szFoundAt == nullptr)
    return nullptr;

  const xiiUInt32 uiOffset = (xiiUInt32)(szFoundAt - GetData());

  const xiiUInt32 uiSearchStrLength = sSearchFor.GetElementCount();

  ReplaceSubString(szFoundAt, szFoundAt + uiSearchStrLength, sReplacement);

  return GetData() + uiOffset; // memory might have been reallocated
}

const char* xiiStringBuilder::ReplaceLast_NoCase(xiiStringView sSearchFor, xiiStringView sReplacement, const char* szStartSearchAt)
{
  if (szStartSearchAt == nullptr)
    szStartSearchAt = GetData() + m_Data.GetCount() - 1;
  else
  {
    XII_ASSERT_DEV(xiiMath::IsInRange(szStartSearchAt, GetData(), GetData() + m_Data.GetCount() - 1), "szStartSearchAt is not inside the string range.");
  }

  const char* szFoundAt = xiiStringUtils::FindLastSubString_NoCase(GetData(), sSearchFor.GetStartPointer(), szStartSearchAt, GetData() + m_Data.GetCount() - 1, sSearchFor.GetEndPointer());

  if (szFoundAt == nullptr)
    return nullptr;

  const xiiUInt32 uiOffset = (xiiUInt32)(szFoundAt - GetData());

  const xiiUInt32 uiSearchStrLength = sSearchFor.GetElementCount();

  ReplaceSubString(szFoundAt, szFoundAt + uiSearchStrLength, sReplacement);

  return GetData() + uiOffset; // memory might have been reallocated
}

xiiUInt32 xiiStringBuilder::ReplaceAll_NoCase(xiiStringView sSearchFor, xiiStringView sReplacement)
{
  const xiiUInt32 uiSearchBytes = sSearchFor.GetElementCount();
  const xiiUInt32 uiWordBytes   = sReplacement.GetElementCount();

  xiiUInt32 uiReplacements = 0;
  xiiUInt32 uiOffset       = 0;

  while (true)
  {
    // during ReplaceSubString the string data might get reallocated and the memory addresses do not stay valid
    // so we need to work with offsets and recompute the pointers every time
    const char* szFoundAt = xiiStringUtils::FindSubString_NoCase(GetData() + uiOffset, sSearchFor.GetStartPointer(), GetData() + m_Data.GetCount() - 1, sSearchFor.GetEndPointer());

    if (szFoundAt == nullptr)
      return uiReplacements;

    // do not search withing the replaced part, otherwise we get recursive replacement which will not end
    uiOffset = static_cast<xiiUInt32>(szFoundAt - GetData()) + uiWordBytes;

    ReplaceSubString(szFoundAt, szFoundAt + uiSearchBytes, sReplacement);

    ++uiReplacements;
  }

  return uiReplacements;
}

const char* xiiStringBuilder::ReplaceWholeWord(const char* szSearchFor, xiiStringView szReplaceWith, xiiStringUtils::XII_CHARACTER_FILTER IsDelimiterCB)
{
  const char* szPos = FindWholeWord(szSearchFor, IsDelimiterCB);

  if (szPos == nullptr)
    return nullptr;

  const xiiUInt32 uiOffset = static_cast<xiiUInt32>(szPos - GetData());

  ReplaceSubString(szPos, szPos + xiiStringUtils::GetStringElementCount(szSearchFor), szReplaceWith);
  return GetData() + uiOffset;
}

const char* xiiStringBuilder::ReplaceWholeWord_NoCase(const char* szSearchFor, xiiStringView szReplaceWith, xiiStringUtils::XII_CHARACTER_FILTER IsDelimiterCB)
{
  const char* szPos = FindWholeWord_NoCase(szSearchFor, IsDelimiterCB);

  if (szPos == nullptr)
    return nullptr;

  const xiiUInt32 uiOffset = static_cast<xiiUInt32>(szPos - GetData());

  ReplaceSubString(szPos, szPos + xiiStringUtils::GetStringElementCount(szSearchFor), szReplaceWith);
  return GetData() + uiOffset;
}


xiiUInt32 xiiStringBuilder::ReplaceWholeWordAll(const char* szSearchFor, xiiStringView szReplaceWith, xiiStringUtils::XII_CHARACTER_FILTER IsDelimiterCB)
{
  const xiiUInt32 uiSearchBytes = xiiStringUtils::GetStringElementCount(szSearchFor);
  const xiiUInt32 uiWordBytes   = xiiStringUtils::GetStringElementCount(szReplaceWith.GetStartPointer(), szReplaceWith.GetEndPointer());

  xiiUInt32 uiReplacements = 0;
  xiiUInt32 uiOffset       = 0;

  while (true)
  {
    // during ReplaceSubString the string data might get reallocated and the memory addresses do not stay valid
    // so we need to work with offsets and recompute the pointers every time
    const char* szFoundAt = xiiStringUtils::FindWholeWord(GetData() + uiOffset, szSearchFor, IsDelimiterCB, GetData() + m_Data.GetCount() - 1);

    if (szFoundAt == nullptr)
      return uiReplacements;

    // do not search withing the replaced part, otherwise we get recursive replacement which will not end
    uiOffset = static_cast<xiiUInt32>(szFoundAt - GetData()) + uiWordBytes;

    ReplaceSubString(szFoundAt, szFoundAt + uiSearchBytes, szReplaceWith);

    ++uiReplacements;
  }

  return uiReplacements;
}

xiiUInt32 xiiStringBuilder::ReplaceWholeWordAll_NoCase(const char* szSearchFor, xiiStringView szReplaceWith, xiiStringUtils::XII_CHARACTER_FILTER IsDelimiterCB)
{
  const xiiUInt32 uiSearchBytes = xiiStringUtils::GetStringElementCount(szSearchFor);
  const xiiUInt32 uiWordBytes   = xiiStringUtils::GetStringElementCount(szReplaceWith.GetStartPointer(), szReplaceWith.GetEndPointer());

  xiiUInt32 uiReplacements = 0;
  xiiUInt32 uiOffset       = 0;

  while (true)
  {
    // during ReplaceSubString the string data might get reallocated and the memory addresses do not stay valid
    // so we need to work with offsets and recompute the pointers every time
    const char* szFoundAt = xiiStringUtils::FindWholeWord_NoCase(GetData() + uiOffset, szSearchFor, IsDelimiterCB, GetData() + m_Data.GetCount() - 1);

    if (szFoundAt == nullptr)
      return uiReplacements;

    // do not search withing the replaced part, otherwise we get recursive replacement which will not end
    uiOffset = static_cast<xiiUInt32>(szFoundAt - GetData()) + uiWordBytes;

    ReplaceSubString(szFoundAt, szFoundAt + uiSearchBytes, szReplaceWith);

    ++uiReplacements;
  }

  return uiReplacements;
}

void xiiStringBuilder::operator=(xiiStringView rhs)
{
  xiiUInt32 uiBytes;
  xiiUInt32 uiCharacters;

  xiiStringUtils::GetCharacterAndElementCount(rhs.GetStartPointer(), uiCharacters, uiBytes, rhs.GetEndPointer());

  // if we need more room, allocate up front (rhs cannot use our own data in this case)
  if (uiBytes + 1 > m_Data.GetCount())
    m_Data.SetCountUninitialized(uiBytes + 1);

  // the data might actually come from our very own string, so we 'move' the memory in there, just to be safe
  // if it comes from our own array, the data will always be a sub-set -> smaller than this array
  // in this case we defer the SetCount till later, to ensure that the data is not corrupted (destructed) before we copy it
  // however, when the new data is larger than the old, it cannot be from our own data, so we can (and must) reallocate before copying
  xiiMemoryUtils::CopyOverlapped(&m_Data[0], rhs.GetStartPointer(), uiBytes);

  m_Data.SetCountUninitialized(uiBytes + 1);
  m_Data[uiBytes] = '\0';

  m_uiCharacterCount = uiCharacters;
}

enum PathUpState
{
  NotStarted,
  OneDot,
  TwoDots,
  FoundDotSlash,
  FoundDotDotSlash,
  Invalid,
};

void xiiStringBuilder::MakeCleanPath()
{
  if (IsEmpty())
    return;

  Trim(" \t\r\n");

  RemoveDoubleSlashesInPath();

  // remove Windows specific DOS device path indicators from the start
  TrimWordStart("//?/");
  TrimWordStart("//./");

  const char* const szEndPos      = &m_Data[m_Data.GetCount() - 1];
  const char*       szCurReadPos  = &m_Data[0];
  char* const       szCurWritePos = &m_Data[0];
  int               writeOffset   = 0;

  xiiInt32    iLevelsDown = 0;
  PathUpState FoundPathUp = NotStarted;

  while (szCurReadPos < szEndPos)
  {
    char CurChar = *szCurReadPos;

    if (CurChar == '.')
    {
      if (FoundPathUp == NotStarted)
        FoundPathUp = OneDot;
      else if (FoundPathUp == OneDot)
        FoundPathUp = TwoDots;
      else
        FoundPathUp = Invalid;
    }
    else if (xiiPathUtils::IsPathSeparator(CurChar))
    {
      CurChar = '/';

      if (FoundPathUp == OneDot)
      {
        FoundPathUp = FoundDotSlash;
      }
      else if (FoundPathUp == TwoDots)
      {
        FoundPathUp = FoundDotDotSlash;
      }
      else
      {
        ++iLevelsDown;
        FoundPathUp = NotStarted;
      }
    }
    else
      FoundPathUp = NotStarted;

    if (FoundPathUp == FoundDotDotSlash)
    {
      if (iLevelsDown > 0)
      {
        --iLevelsDown;
        XII_ASSERT_DEBUG(writeOffset >= 3, "invalid write offset");
        writeOffset -= 3; // go back, skip two dots, one slash

        while ((writeOffset > 0) && (szCurWritePos[writeOffset - 1] != '/'))
        {
          XII_ASSERT_DEBUG(writeOffset > 0, "invalid write offset");
          --writeOffset;
        }
      }
      else
      {
        szCurWritePos[writeOffset] = '/';
        ++writeOffset;
      }

      FoundPathUp = NotStarted;
    }
    else if (FoundPathUp == FoundDotSlash)
    {
      XII_ASSERT_DEBUG(writeOffset > 0, "invalid write offset");
      writeOffset -= 1; // go back to where we wrote the dot

      FoundPathUp = NotStarted;
    }
    else
    {
      szCurWritePos[writeOffset] = CurChar;
      ++writeOffset;
    }

    ++szCurReadPos;
  }

  const xiiUInt32 uiPrevByteCount = m_Data.GetCount();
  const xiiUInt32 uiNewByteCount  = (xiiUInt32)(writeOffset) + 1;

  XII_ASSERT_DEBUG(uiPrevByteCount >= uiNewByteCount, "It should not be possible that a path grows during cleanup. Old: {0} Bytes, New: {1} Bytes",
                   uiPrevByteCount, uiNewByteCount);

  // we will only remove characters and only ASCII ones (slash, backslash, dot)
  // so the number of characters shrinks equally to the number of bytes
  m_uiCharacterCount -= (uiPrevByteCount - uiNewByteCount);

  // make sure to write the terminating \0 and reset the count
  szCurWritePos[writeOffset] = '\0';
  m_Data.SetCountUninitialized(uiNewByteCount);
}

void xiiStringBuilder::PathParentDirectory(xiiUInt32 uiLevelsUp)
{
  XII_ASSERT_DEV(uiLevelsUp > 0, "We have to do something!");

  for (xiiUInt32 i = 0; i < uiLevelsUp; ++i)
    AppendPath("../");

  MakeCleanPath();
}

void xiiStringBuilder::AppendPath(xiiStringView sPath1, xiiStringView sPath2, xiiStringView sPath3, xiiStringView sPath4)
{
  const xiiStringView sPaths[4] = {sPath1, sPath2, sPath3, sPath4};

  for (xiiUInt32 i = 0; i < 4; ++i)
  {
    xiiStringView sThisPath = sPaths[i];

    if (!sThisPath.IsEmpty())
    {
      if ((IsEmpty() && xiiPathUtils::IsAbsolutePath(sPaths[i])))
      {
        // this is for Linux systems where absolute paths start with a slash, wouldn't want to remove that
      }
      else
      {
        // prevent creating multiple path separators through concatenation
        while (xiiPathUtils::IsPathSeparator(*sThisPath.GetStartPointer()))
          sThisPath.Shrink(1, 0);
      }

      if (IsEmpty() || xiiPathUtils::IsPathSeparator(GetIteratorBack().GetCharacter()))
        Append(sThisPath);
      else
        Append("/", sThisPath);
    }
  }
}

void xiiStringBuilder::AppendWithSeparator(xiiStringView optional, xiiStringView sText1, xiiStringView sText2 /*= xiiStringView()*/, xiiStringView sText3 /*= xiiStringView()*/, xiiStringView sText4 /*= xiiStringView()*/, xiiStringView sText5 /*= xiiStringView()*/, xiiStringView sText6 /*= xiiStringView()*/)
{
  // if this string already ends with the optional string, reset it to be empty
  if (IsEmpty() || xiiStringUtils::EndsWith(GetData(), optional.GetStartPointer(), GetData() + GetElementCount(), optional.GetEndPointer()))
  {
    optional = xiiStringView();
  }

  const xiiUInt32 uiMaxParams = 7;

  const xiiStringView pStrings[uiMaxParams] = {optional, sText1, sText2, sText3, sText4, sText5, sText6};
  xiiUInt32           uiStrLen[uiMaxParams] = {0};
  xiiUInt32           uiMoreBytes           = 0;

  // first figure out how much the string has to grow
  for (xiiUInt32 i = 0; i < uiMaxParams; ++i)
  {
    if (pStrings[i].IsEmpty())
      continue;

    XII_ASSERT_DEBUG(pStrings[i].GetStartPointer() < m_Data.GetData() || pStrings[i].GetStartPointer() >= m_Data.GetData() + m_Data.GetCapacity(),
                     "Parameter {0} comes from the string builders own storage. This type assignment is not allowed.", i);

    xiiUInt32 uiCharacters = 0;
    xiiStringUtils::GetCharacterAndElementCount(pStrings[i].GetStartPointer(), uiCharacters, uiStrLen[i], pStrings[i].GetEndPointer());
    uiMoreBytes += uiStrLen[i];
    m_uiCharacterCount += uiCharacters;

    XII_ASSERT_DEV(xiiUnicodeUtils::IsValidUtf8(pStrings[i].GetStartPointer(), pStrings[i].GetEndPointer()), "Parameter {0} is not a valid Utf8 sequence.", i + 1);
  }

  xiiUInt32 uiPrevCount = m_Data.GetCount(); // already contains a 0 terminator
  XII_ASSERT_DEBUG(uiPrevCount > 0, "There should be a 0 terminator somewhere around here.");

  // now resize
  m_Data.SetCountUninitialized(uiPrevCount + uiMoreBytes);

  // and then append all the strings
  for (xiiUInt32 i = 0; i < uiMaxParams; ++i)
  {
    if (uiStrLen[i] == 0)
      continue;

    // make enough room to copy the entire string, including the T-800
    xiiStringUtils::Copy(&m_Data[uiPrevCount - 1], uiStrLen[i] + 1, pStrings[i].GetStartPointer(), pStrings[i].GetStartPointer() + uiStrLen[i]);

    uiPrevCount += uiStrLen[i];
  }
}

void xiiStringBuilder::ChangeFileName(xiiStringView sNewFileName)
{
  xiiStringView it = xiiPathUtils::GetFileName(GetView());

  ReplaceSubString(it.GetStartPointer(), it.GetEndPointer(), sNewFileName);
}

void xiiStringBuilder::ChangeFileNameAndExtension(xiiStringView sNewFileNameWithExtension)
{
  xiiStringView it = xiiPathUtils::GetFileNameAndExtension(GetView());

  ReplaceSubString(it.GetStartPointer(), it.GetEndPointer(), sNewFileNameWithExtension);
}

void xiiStringBuilder::ChangeFileExtension(xiiStringView sNewExtension)
{
  while (sNewExtension.StartsWith("."))
  {
    sNewExtension.Shrink(1, 0);
  }

  const xiiStringView it = xiiPathUtils::GetFileExtension(GetView());

  if (it.IsEmpty() && !EndsWith("."))
    Append(".", sNewExtension);
  else
    ReplaceSubString(it.GetStartPointer(), it.GetEndPointer(), sNewExtension);
}

void xiiStringBuilder::RemoveFileExtension()
{
  if (HasAnyExtension())
  {
    ChangeFileExtension("");
    Shrink(0, 1); // remove the dot
  }
}

xiiResult xiiStringBuilder::MakeRelativeTo(xiiStringView sAbsolutePathToMakeThisRelativeTo)
{
  xiiStringBuilder sAbsBase = sAbsolutePathToMakeThisRelativeTo;
  sAbsBase.MakeCleanPath();
  xiiStringBuilder sAbsThis = *this;
  sAbsThis.MakeCleanPath();

  if (sAbsBase.IsEqual_NoCase(sAbsThis.GetData()))
  {
    Clear();
    return XII_SUCCESS;
  }

  if (!sAbsBase.EndsWith("/"))
    sAbsBase.Append("/");

  if (!sAbsThis.EndsWith("/"))
  {
    sAbsThis.Append("/");

    if (sAbsBase.StartsWith(sAbsThis.GetData()))
    {
      Clear();
      const char* szStart = &sAbsBase.GetData()[sAbsThis.GetElementCount()];

      while (*szStart != '\0')
      {
        if (*szStart == '/')
          Append("../");

        ++szStart;
      }

      return XII_SUCCESS;
    }
    else
      sAbsThis.Shrink(0, 1);
  }

  const xiiUInt32 uiMinLen = xiiMath::Min(sAbsBase.GetElementCount(), sAbsThis.GetElementCount());

  xiiInt32 iSame = uiMinLen - 1;
  for (; iSame >= 0; --iSame)
  {
    if (sAbsBase.GetData()[iSame] != '/')
      continue;

    // We need to check here if sAbsThis starts with sAbsBase in the range[0, iSame + 1]. However, we can't compare the first N bytes because those might not be a valid utf8 substring in absBase.
    // Thus we can't use IsEqualN_NoCase as N would need to be the number of characters, not bytes. Computing the number of characters in absBase would mean iterating the string twice.
    // As an alternative, as we know [0, iSame + 1] is a valid utf8 string in sAbsBase we can ask whether absThis starts with that substring.
    if (xiiStringUtils::StartsWith_NoCase(sAbsThis.GetData(), sAbsBase.GetData(), sAbsThis.GetData() + sAbsThis.GetElementCount(), sAbsBase.GetData() + iSame + 1))
      break;
  }

  if (iSame < 0)
  {
    return XII_FAILURE;
  }

  Clear();

  for (xiiUInt32 ui = iSame + 1; ui < sAbsBase.GetElementCount(); ++ui)
  {
    if (sAbsBase.GetData()[ui] == '/')
      Append("../");
  }

  if (sAbsThis.GetData()[iSame] == '/')
    ++iSame;

  Append(&(sAbsThis.GetData()[iSame]));

  return XII_SUCCESS;
}

/// An empty folder (zero length) does not contain ANY files.\n
/// A non-existing file-name (zero length) is never in any folder.\n
/// Example:\n
/// IsFileBelowFolder ("", "XYZ") -> always false\n
/// IsFileBelowFolder ("XYZ", "") -> always false\n
/// IsFileBelowFolder ("", "") -> always false\n
bool xiiStringBuilder::IsPathBelowFolder(const char* szPathToFolder)
{
  XII_ASSERT_DEV(!xiiStringUtils::IsNullOrEmpty(szPathToFolder), "The given path must not be empty. Because is 'nothing' under the empty path, or 'everything' ?");

  // a non-existing file is never in any folder
  if (IsEmpty())
    return false;

  MakeCleanPath();

  xiiStringBuilder sBasePath(szPathToFolder);
  sBasePath.MakeCleanPath();

  if (IsEqual_NoCase(sBasePath.GetData()))
    return true;

  if (!sBasePath.EndsWith("/"))
    sBasePath.Append("/");

  return StartsWith_NoCase(sBasePath.GetData());
}

void xiiStringBuilder::MakePathSeparatorsNative()
{
  const char sep = xiiPathUtils::OsSpecificPathSeparator;

  MakeCleanPath();
  ReplaceAll("/", xiiStringView(&sep, 1));
}

void xiiStringBuilder::RemoveDoubleSlashesInPath()
{
  if (IsEmpty())
    return;

  const char* szReadPos     = &m_Data[0];
  char*       szCurWritePos = &m_Data[0];

  xiiInt32 iAllowedSlashes = 2;

  while (*szReadPos != '\0')
  {
    char CurChar = *szReadPos;
    ++szReadPos;

    if (CurChar == '\\')
      CurChar = '/';

    if (CurChar != '/')
      iAllowedSlashes = 1;
    else
    {
      if (iAllowedSlashes > 0)
        --iAllowedSlashes;
      else
        continue;
    }

    *szCurWritePos = CurChar;
    ++szCurWritePos;
  }


  const xiiUInt32 uiPrevByteCount = m_Data.GetCount();
  const xiiUInt32 uiNewByteCount  = (xiiUInt32)(szCurWritePos - &m_Data[0]) + 1;

  XII_ASSERT_DEBUG(uiPrevByteCount >= uiNewByteCount, "It should not be possible that a path grows during cleanup. Old: {0} Bytes, New: {1} Bytes",
                   uiPrevByteCount, uiNewByteCount);

  // we will only remove characters and only ASCII ones (slash, backslash)
  // so the number of characters shrinks equally to the number of bytes
  m_uiCharacterCount -= (uiPrevByteCount - uiNewByteCount);

  // make sure to write the terminating \0 and reset the count
  *szCurWritePos = '\0';
  m_Data.SetCountUninitialized(uiNewByteCount);
}


void xiiStringBuilder::ReadAll(xiiStreamReader& Stream)
{
  Clear();

  xiiHybridArray<xiiUInt8, 1024 * 4> Bytes(m_Data.GetAllocator());
  xiiUInt8                           Temp[1024];

  while (true)
  {
    const xiiUInt32 uiRead = (xiiUInt32)Stream.ReadBytes(Temp, 1024);

    if (uiRead == 0)
      break;

    Bytes.PushBackRange(xiiArrayPtr<xiiUInt8>(Temp, uiRead));
  }

  Bytes.PushBack('\0');

  *this = (const char*)&Bytes[0];
}

void xiiStringBuilder::Trim(const char* szTrimChars)
{
  return Trim(szTrimChars, szTrimChars);
}

void xiiStringBuilder::Trim(const char* szTrimCharsStart, const char* szTrimCharsEnd)
{
  const char* szNewStart = GetData();
  const char* szNewEnd   = GetData() + GetElementCount();
  xiiStringUtils::Trim(szNewStart, szNewEnd, szTrimCharsStart, szTrimCharsEnd);
  Shrink(xiiStringUtils::GetCharacterCount(GetData(), szNewStart), xiiStringUtils::GetCharacterCount(szNewEnd, GetData() + GetElementCount()));
}

bool xiiStringBuilder::TrimWordStart(const char* szWord1, const char* szWord2 /*= nullptr*/, const char* szWord3 /*= nullptr*/, const char* szWord4 /*= nullptr*/, const char* szWord5 /*= nullptr*/)
{
  /// \test TrimWordStart
  bool trimmed = false;

  while (true)
  {
    if (!xiiStringUtils::IsNullOrEmpty(szWord1) && StartsWith_NoCase(szWord1))
    {
      Shrink(xiiStringUtils::GetCharacterCount(szWord1), 0);
      trimmed = true;
      continue;
    }

    if (!xiiStringUtils::IsNullOrEmpty(szWord2) && StartsWith_NoCase(szWord2))
    {
      Shrink(xiiStringUtils::GetCharacterCount(szWord2), 0);
      trimmed = true;
      continue;
    }

    if (!xiiStringUtils::IsNullOrEmpty(szWord3) && StartsWith_NoCase(szWord3))
    {
      Shrink(xiiStringUtils::GetCharacterCount(szWord3), 0);
      trimmed = true;
      continue;
    }

    if (!xiiStringUtils::IsNullOrEmpty(szWord4) && StartsWith_NoCase(szWord4))
    {
      Shrink(xiiStringUtils::GetCharacterCount(szWord4), 0);
      trimmed = true;
      continue;
    }

    if (!xiiStringUtils::IsNullOrEmpty(szWord5) && StartsWith_NoCase(szWord5))
    {
      Shrink(xiiStringUtils::GetCharacterCount(szWord5), 0);
      trimmed = true;
      continue;
    }

    return trimmed;
  }
}

bool xiiStringBuilder::TrimWordEnd(const char* szWord1, const char* szWord2 /*= nullptr*/, const char* szWord3 /*= nullptr*/, const char* szWord4 /*= nullptr*/, const char* szWord5 /*= nullptr*/)
{
  /// \test TrimWordEnd

  bool trimmed = false;

  while (true)
  {

    if (!xiiStringUtils::IsNullOrEmpty(szWord1) && EndsWith_NoCase(szWord1))
    {
      Shrink(0, xiiStringUtils::GetCharacterCount(szWord1));
      trimmed = true;
      continue;
    }

    if (!xiiStringUtils::IsNullOrEmpty(szWord2) && EndsWith_NoCase(szWord2))
    {
      Shrink(0, xiiStringUtils::GetCharacterCount(szWord2));
      trimmed = true;
      continue;
    }

    if (!xiiStringUtils::IsNullOrEmpty(szWord3) && EndsWith_NoCase(szWord3))
    {
      Shrink(0, xiiStringUtils::GetCharacterCount(szWord3));
      trimmed = true;
      continue;
    }

    if (!xiiStringUtils::IsNullOrEmpty(szWord4) && EndsWith_NoCase(szWord4))
    {
      Shrink(0, xiiStringUtils::GetCharacterCount(szWord4));
      trimmed = true;
      continue;
    }

    if (!xiiStringUtils::IsNullOrEmpty(szWord5) && EndsWith_NoCase(szWord5))
    {
      Shrink(0, xiiStringUtils::GetCharacterCount(szWord5));
      trimmed = true;
      continue;
    }

    return trimmed;
  }
}

void xiiStringBuilder::Format(const xiiFormatString& string)
{
  Clear();
  const char* szText = string.GetText(*this);

  // this is for the case that GetText does not use the xiiStringBuilder as temp storage
  if (szText != GetData())
    *this = szText;
}

void xiiStringBuilder::AppendFormat(const xiiFormatString& string)
{
  xiiStringBuilder tmp;
  xiiStringView    view = string.GetText(tmp);

  Append(view);
}

void xiiStringBuilder::PrependFormat(const xiiFormatString& string)
{
  xiiStringBuilder tmp;

  Prepend(string.GetText(tmp));
}

void xiiStringBuilder::Printf(const char* szUtf8Format, ...)
{
  va_list args;
  va_start(args, szUtf8Format);

  PrintfArgs(szUtf8Format, args);

  va_end(args);
}

XII_STATICLINK_FILE(Foundation, Foundation_Strings_Implementation_StringBuilder);

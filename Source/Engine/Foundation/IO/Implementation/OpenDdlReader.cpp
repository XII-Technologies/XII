#include <Foundation/FoundationPCH.h>

#include <Foundation/IO/OpenDdlReader.h>

xiiOpenDdlReader::xiiOpenDdlReader()
{
  m_pCurrentChunk      = nullptr;
  m_uiBytesInChunkLeft = 0;
}

xiiOpenDdlReader::~xiiOpenDdlReader()
{
  ClearDataChunks();
}

xiiResult xiiOpenDdlReader::ParseDocument(xiiStreamReader& ref_stream, xiiUInt32 uiFirstLineOffset, xiiLogInterface* pLog, xiiUInt32 uiCacheSizeInKB)
{
  XII_ASSERT_DEBUG(m_ObjectStack.IsEmpty(), "A reader can only be used once.");

  SetLogInterface(pLog);
  SetCacheSize(uiCacheSizeInKB);
  SetInputStream(ref_stream, uiFirstLineOffset);

  m_TempCache.Reserve(s_uiChunkSize);

  xiiOpenDdlReaderElement* pElement = &m_Elements.ExpandAndGetRef();
  pElement->m_pFirstChild           = nullptr;
  pElement->m_pLastChild            = nullptr;
  pElement->m_PrimitiveType         = xiiOpenDdlPrimitiveType::Custom;
  pElement->m_pSiblingElement       = nullptr;
  pElement->m_szCustomType          = CopyString("root");
  pElement->m_szName                = nullptr;
  pElement->m_uiNumChildElements    = 0;

  m_ObjectStack.PushBack(pElement);

  return ParseAll();
}

const xiiOpenDdlReaderElement* xiiOpenDdlReader::GetRootElement() const
{
  XII_ASSERT_DEBUG(!m_ObjectStack.IsEmpty(), "The reader has not parsed any document yet or an error occurred during parsing.");

  return m_ObjectStack[0];
}


const xiiOpenDdlReaderElement* xiiOpenDdlReader::FindElement(const char* szGlobalName) const
{
  return m_GlobalNames.GetValueOrDefault(szGlobalName, nullptr);
}

const char* xiiOpenDdlReader::CopyString(const xiiStringView& string)
{
  if (string.IsEmpty())
    return nullptr;

  // no idea how to make this more efficient without running into lots of other problems
  m_Strings.PushBack(string);
  return m_Strings.PeekBack().GetData();
}

xiiOpenDdlReaderElement* xiiOpenDdlReader::CreateElement(xiiOpenDdlPrimitiveType type, const char* szType, const char* szName, bool bGlobalName)
{
  xiiOpenDdlReaderElement* pElement = &m_Elements.ExpandAndGetRef();
  pElement->m_pFirstChild           = nullptr;
  pElement->m_pLastChild            = nullptr;
  pElement->m_PrimitiveType         = type;
  pElement->m_pSiblingElement       = nullptr;
  pElement->m_szCustomType          = szType;
  pElement->m_szName                = CopyString(szName);
  pElement->m_uiNumChildElements    = 0;

  if (bGlobalName)
  {
    pElement->m_uiNumChildElements = XII_BIT(31);
  }

  if (bGlobalName && !xiiStringUtils::IsNullOrEmpty(szName))
  {
    m_GlobalNames[szName] = pElement;
  }

  xiiOpenDdlReaderElement* pParent = m_ObjectStack.PeekBack();
  pParent->m_uiNumChildElements++;

  if (pParent->m_pFirstChild == nullptr)
  {
    pParent->m_pFirstChild = pElement;
    pParent->m_pLastChild  = pElement;
  }
  else
  {
    ((xiiOpenDdlReaderElement*)pParent->m_pLastChild)->m_pSiblingElement = pElement;
    pParent->m_pLastChild                                                = pElement;
  }

  m_ObjectStack.PushBack(pElement);

  return pElement;
}


void xiiOpenDdlReader::OnBeginObject(const char* szType, const char* szName, bool bGlobalName)
{
  CreateElement(xiiOpenDdlPrimitiveType::Custom, CopyString(szType), szName, bGlobalName);
}

void xiiOpenDdlReader::OnEndObject()
{
  m_ObjectStack.PopBack();
}

void xiiOpenDdlReader::OnBeginPrimitiveList(xiiOpenDdlPrimitiveType type, const char* szName, bool bGlobalName)
{
  CreateElement(type, nullptr, szName, bGlobalName);

  m_TempCache.Clear();
}

void xiiOpenDdlReader::OnEndPrimitiveList()
{
  // if we had to temporarily store the primitive data, copy it into a new destination
  if (!m_TempCache.IsEmpty())
  {
    xiiUInt8* pTarget                       = AllocateBytes(m_TempCache.GetCount());
    m_ObjectStack.PeekBack()->m_pFirstChild = pTarget;

    xiiMemoryUtils::Copy(pTarget, m_TempCache.GetData(), m_TempCache.GetCount());
  }

  m_ObjectStack.PopBack();
}

void xiiOpenDdlReader::StorePrimitiveData(bool bThisIsAll, xiiUInt32 bytecount, const xiiUInt8* pData)
{
  xiiUInt8* pTarget = nullptr;

  if (!bThisIsAll || !m_TempCache.IsEmpty())
  {
    // if this is not all, accumulate the data in a temp buffer
    xiiUInt32 offset = m_TempCache.GetCount();
    m_TempCache.SetCountUninitialized(m_TempCache.GetCount() + bytecount);
    pTarget = &m_TempCache[offset]; // have to index m_TempCache after the resize, otherwise it could be empty and not like it
  }
  else
  {
    // otherwise, allocate the final storage immediately
    pTarget                                 = AllocateBytes(bytecount);
    m_ObjectStack.PeekBack()->m_pFirstChild = pTarget;
  }

  xiiMemoryUtils::Copy(pTarget, pData, bytecount);
}


void xiiOpenDdlReader::OnPrimitiveBool(xiiUInt32 count, const bool* pData, bool bThisIsAll)
{
  StorePrimitiveData(bThisIsAll, sizeof(bool) * count, (const xiiUInt8*)pData);
  m_ObjectStack.PeekBack()->m_uiNumChildElements += count;
}

void xiiOpenDdlReader::OnPrimitiveInt8(xiiUInt32 count, const xiiInt8* pData, bool bThisIsAll)
{
  StorePrimitiveData(bThisIsAll, sizeof(xiiInt8) * count, (const xiiUInt8*)pData);
  m_ObjectStack.PeekBack()->m_uiNumChildElements += count;
}

void xiiOpenDdlReader::OnPrimitiveInt16(xiiUInt32 count, const xiiInt16* pData, bool bThisIsAll)
{
  StorePrimitiveData(bThisIsAll, sizeof(xiiInt16) * count, (const xiiUInt8*)pData);
  m_ObjectStack.PeekBack()->m_uiNumChildElements += count;
}

void xiiOpenDdlReader::OnPrimitiveInt32(xiiUInt32 count, const xiiInt32* pData, bool bThisIsAll)
{
  StorePrimitiveData(bThisIsAll, sizeof(xiiInt32) * count, (const xiiUInt8*)pData);
  m_ObjectStack.PeekBack()->m_uiNumChildElements += count;
}

void xiiOpenDdlReader::OnPrimitiveInt64(xiiUInt32 count, const xiiInt64* pData, bool bThisIsAll)
{
  StorePrimitiveData(bThisIsAll, sizeof(xiiInt64) * count, (const xiiUInt8*)pData);
  m_ObjectStack.PeekBack()->m_uiNumChildElements += count;
}

void xiiOpenDdlReader::OnPrimitiveUInt8(xiiUInt32 count, const xiiUInt8* pData, bool bThisIsAll)
{
  StorePrimitiveData(bThisIsAll, sizeof(xiiUInt8) * count, (const xiiUInt8*)pData);
  m_ObjectStack.PeekBack()->m_uiNumChildElements += count;
}

void xiiOpenDdlReader::OnPrimitiveUInt16(xiiUInt32 count, const xiiUInt16* pData, bool bThisIsAll)
{
  StorePrimitiveData(bThisIsAll, sizeof(xiiUInt16) * count, (const xiiUInt8*)pData);
  m_ObjectStack.PeekBack()->m_uiNumChildElements += count;
}

void xiiOpenDdlReader::OnPrimitiveUInt32(xiiUInt32 count, const xiiUInt32* pData, bool bThisIsAll)
{
  StorePrimitiveData(bThisIsAll, sizeof(xiiUInt32) * count, (const xiiUInt8*)pData);
  m_ObjectStack.PeekBack()->m_uiNumChildElements += count;
}

void xiiOpenDdlReader::OnPrimitiveUInt64(xiiUInt32 count, const xiiUInt64* pData, bool bThisIsAll)
{
  StorePrimitiveData(bThisIsAll, sizeof(xiiUInt64) * count, (const xiiUInt8*)pData);
  m_ObjectStack.PeekBack()->m_uiNumChildElements += count;
}

void xiiOpenDdlReader::OnPrimitiveFloat(xiiUInt32 count, const float* pData, bool bThisIsAll)
{
  StorePrimitiveData(bThisIsAll, sizeof(float) * count, (const xiiUInt8*)pData);
  m_ObjectStack.PeekBack()->m_uiNumChildElements += count;
}

void xiiOpenDdlReader::OnPrimitiveDouble(xiiUInt32 count, const double* pData, bool bThisIsAll)
{
  StorePrimitiveData(bThisIsAll, sizeof(double) * count, (const xiiUInt8*)pData);
  m_ObjectStack.PeekBack()->m_uiNumChildElements += count;
}

void xiiOpenDdlReader::OnPrimitiveString(xiiUInt32 count, const xiiStringView* pData, bool bThisIsAll)
{
  const xiiUInt32 uiDataSize = count * sizeof(xiiStringView);

  const xiiUInt32 offset = m_TempCache.GetCount();
  m_TempCache.SetCountUninitialized(m_TempCache.GetCount() + uiDataSize);
  xiiStringView* pTarget = (xiiStringView*)&m_TempCache[offset];

  for (xiiUInt32 i = 0; i < count; ++i)
  {
    const char* szStart = CopyString(pData[i]);
    pTarget[i]          = xiiStringView(szStart, szStart + pData[i].GetElementCount());
  }

  m_ObjectStack.PeekBack()->m_uiNumChildElements += count;
}


void xiiOpenDdlReader::OnParsingError(const char* szMessage, bool bFatal, xiiUInt32 uiLine, xiiUInt32 uiColumn)
{
  if (bFatal)
  {
    m_ObjectStack.Clear();
    m_GlobalNames.Clear();
    m_Elements.Clear();

    ClearDataChunks();
  }
}

//////////////////////////////////////////////////////////////////////////

void xiiOpenDdlReader::ClearDataChunks()
{
  for (xiiUInt32 i = 0; i < m_DataChunks.GetCount(); ++i)
  {
    XII_DEFAULT_DELETE(m_DataChunks[i]);
  }

  m_DataChunks.Clear();
}

xiiUInt8* xiiOpenDdlReader::AllocateBytes(xiiUInt32 uiNumBytes)
{
  uiNumBytes = xiiMemoryUtils::AlignSize(uiNumBytes, static_cast<xiiUInt32>(XII_ALIGNMENT_MINIMUM));

  // if the requested data is very large, just allocate it as an individual chunk
  if (uiNumBytes > s_uiChunkSize / 2)
  {
    xiiUInt8* pResult = XII_DEFAULT_NEW_ARRAY(xiiUInt8, uiNumBytes).GetPtr();
    m_DataChunks.PushBack(pResult);
    return pResult;
  }

  // if our current chunk is too small, discard the remaining free bytes and just allocate a new chunk
  if (m_uiBytesInChunkLeft < uiNumBytes)
  {
    m_pCurrentChunk      = XII_DEFAULT_NEW_ARRAY(xiiUInt8, s_uiChunkSize).GetPtr();
    m_uiBytesInChunkLeft = s_uiChunkSize;
    m_DataChunks.PushBack(m_pCurrentChunk);
  }

  // no fulfill the request from the current chunk
  xiiUInt8* pResult = m_pCurrentChunk;
  m_pCurrentChunk += uiNumBytes;
  m_uiBytesInChunkLeft -= uiNumBytes;

  return pResult;
}

//////////////////////////////////////////////////////////////////////////

xiiUInt32 xiiOpenDdlReaderElement::GetNumChildObjects() const
{
  if (m_PrimitiveType != xiiOpenDdlPrimitiveType::Custom)
    return 0;

  return m_uiNumChildElements & (~XII_BIT(31)); // Bit 31 stores whether the name is global
}

xiiUInt32 xiiOpenDdlReaderElement::GetNumPrimitives() const
{
  if (m_PrimitiveType == xiiOpenDdlPrimitiveType::Custom)
    return 0;

  return m_uiNumChildElements & (~XII_BIT(31)); // Bit 31 stores whether the name is global
}


bool xiiOpenDdlReaderElement::HasPrimitives(xiiOpenDdlPrimitiveType type, xiiUInt32 uiMinNumberOfPrimitives /*= 1*/) const
{
  /// \test This is new

  if (m_PrimitiveType != type)
    return false;

  return m_uiNumChildElements >= uiMinNumberOfPrimitives;
}

const xiiOpenDdlReaderElement* xiiOpenDdlReaderElement::FindChild(const char* szName) const
{
  XII_ASSERT_DEBUG(m_PrimitiveType == xiiOpenDdlPrimitiveType::Custom, "Cannot search for a child object in a primitives list");

  const xiiOpenDdlReaderElement* pChild = static_cast<const xiiOpenDdlReaderElement*>(m_pFirstChild);

  while (pChild)
  {
    if (xiiStringUtils::IsEqual(pChild->GetName(), szName))
    {
      return pChild;
    }

    pChild = pChild->GetSibling();
  }

  return nullptr;
}

const xiiOpenDdlReaderElement* xiiOpenDdlReaderElement::FindChildOfType(xiiOpenDdlPrimitiveType type, const char* szName, xiiUInt32 uiMinNumberOfPrimitives /* = 1*/) const
{
  /// \test This is new

  XII_ASSERT_DEBUG(m_PrimitiveType == xiiOpenDdlPrimitiveType::Custom, "Cannot search for a child object in a primitives list");

  const xiiOpenDdlReaderElement* pChild = static_cast<const xiiOpenDdlReaderElement*>(m_pFirstChild);

  while (pChild)
  {
    if (pChild->GetPrimitivesType() == type && xiiStringUtils::IsEqual(pChild->GetName(), szName))
    {
      if (type == xiiOpenDdlPrimitiveType::Custom || pChild->GetNumPrimitives() >= uiMinNumberOfPrimitives)
        return pChild;
    }

    pChild = pChild->GetSibling();
  }

  return nullptr;
}

const xiiOpenDdlReaderElement* xiiOpenDdlReaderElement::FindChildOfType(const char* szType, const char* szName /*= nullptr*/) const
{
  const xiiOpenDdlReaderElement* pChild = static_cast<const xiiOpenDdlReaderElement*>(m_pFirstChild);

  while (pChild)
  {
    if (pChild->GetPrimitivesType() == xiiOpenDdlPrimitiveType::Custom && xiiStringUtils::IsEqual(pChild->GetCustomType(), szType) && (szName == nullptr || xiiStringUtils::IsEqual(pChild->GetName(), szName)))
    {
      return pChild;
    }

    pChild = pChild->GetSibling();
  }

  return nullptr;
}


XII_STATICLINK_FILE(Foundation, Foundation_IO_Implementation_OpenDdlReader);

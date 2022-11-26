#include <Foundation/FoundationPCH.h>

#include <Foundation/IO/MemoryStream.h>

// Reader implementation

xiiMemoryStreamReader::xiiMemoryStreamReader(const xiiMemoryStreamStorageInterface* pStreamStorage) :
  m_pStreamStorage(pStreamStorage)
{
}

xiiMemoryStreamReader::~xiiMemoryStreamReader() {}

xiiUInt64 xiiMemoryStreamReader::ReadBytes(void* pReadBuffer, xiiUInt64 uiBytesToRead)
{
  XII_ASSERT_RELEASE(m_pStreamStorage != nullptr, "The memory stream reader needs a valid memory storage object!");

  const xiiUInt64 uiBytes = xiiMath::Min<xiiUInt64>(uiBytesToRead, m_pStreamStorage->GetStorageSize64() - m_uiReadPosition);

  if (uiBytes == 0)
    return 0;

  if (pReadBuffer)
  {
    xiiUInt64 uiBytesLeft = uiBytes;

    while (uiBytesLeft > 0)
    {
      xiiArrayPtr<const xiiUInt8> data = m_pStreamStorage->GetContiguousMemoryRange(m_uiReadPosition);

      XII_ASSERT_DEV(!data.IsEmpty(), "MemoryStreamStorage returned an empty contiguous memory block.");

      const xiiUInt64 toRead = xiiMath::Min<xiiUInt64>(data.GetCount(), uiBytesLeft);

      xiiMemoryUtils::Copy(static_cast<xiiUInt8*>(pReadBuffer), data.GetPtr(), static_cast<size_t>(toRead)); // Down-cast to size_t for 32-bit.

      pReadBuffer = xiiMemoryUtils::AddByteOffset(pReadBuffer, static_cast<size_t>(toRead)); // Down-cast to size_t for 32-bit.

      m_uiReadPosition += toRead;
      uiBytesLeft -= toRead;
    }
  }
  else
  {
    m_uiReadPosition += uiBytes;
  }

  return uiBytes;
}

xiiUInt64 xiiMemoryStreamReader::SkipBytes(xiiUInt64 uiBytesToSkip)
{
  XII_ASSERT_RELEASE(m_pStreamStorage != nullptr, "The memory stream reader needs a valid memory storage object!");

  const xiiUInt64 uiBytes = xiiMath::Min<xiiUInt64>(uiBytesToSkip, m_pStreamStorage->GetStorageSize64() - m_uiReadPosition);

  m_uiReadPosition += uiBytes;

  return uiBytes;
}

void xiiMemoryStreamReader::SetReadPosition(xiiUInt64 uiReadPosition)
{
  XII_ASSERT_RELEASE(uiReadPosition <= GetByteCount64(), "Read position must be between 0 and GetByteCount()!");
  m_uiReadPosition = uiReadPosition;
}

xiiUInt32 xiiMemoryStreamReader::GetByteCount32() const
{
  XII_ASSERT_RELEASE(m_pStreamStorage != nullptr, "The memory stream reader needs a valid memory storage object!");

  return m_pStreamStorage->GetStorageSize32();
}

xiiUInt64 xiiMemoryStreamReader::GetByteCount64() const
{
  XII_ASSERT_RELEASE(m_pStreamStorage != nullptr, "The memory stream reader needs a valid memory storage object!");

  return m_pStreamStorage->GetStorageSize64();
}

void xiiMemoryStreamReader::SetDebugSourceInformation(const char* szDebugSourceInformation)
{
  m_sDebugSourceInformation = szDebugSourceInformation;
}

//////////////////////////////////////////////////////////////////////////

// Writer implementation
xiiMemoryStreamWriter::xiiMemoryStreamWriter(xiiMemoryStreamStorageInterface* pStreamStorage) :
  m_pStreamStorage(pStreamStorage), m_uiWritePosition(0)
{
}

xiiMemoryStreamWriter::~xiiMemoryStreamWriter() = default;

xiiResult xiiMemoryStreamWriter::WriteBytes(const void* pWriteBuffer, xiiUInt64 uiBytesToWrite)
{
  XII_ASSERT_DEV(m_pStreamStorage != nullptr, "The memory stream writer needs a valid memory storage object!");

  if (uiBytesToWrite == 0)
    return XII_SUCCESS;

  XII_ASSERT_DEBUG(pWriteBuffer != nullptr, "No valid buffer containing data given!");

  // Reserve the memory in the storage object, grow size if appending data (don't shrink)
  m_pStreamStorage->SetInternalSize(xiiMath::Max(m_pStreamStorage->GetStorageSize64(), m_uiWritePosition + uiBytesToWrite));

  {
    xiiUInt64 uiBytesLeft = uiBytesToWrite;

    while (uiBytesLeft > 0)
    {
      xiiArrayPtr<xiiUInt8> data = m_pStreamStorage->GetContiguousMemoryRange(m_uiWritePosition);

      XII_ASSERT_DEV(!data.IsEmpty(), "MemoryStreamStorage returned an empty contiguous memory block.");

      const xiiUInt64 toWrite = xiiMath::Min<xiiUInt64>(data.GetCount(), uiBytesLeft);

      xiiMemoryUtils::Copy(data.GetPtr(), static_cast<const xiiUInt8*>(pWriteBuffer), static_cast<size_t>(toWrite)); // Down-cast to size_t for 32-bit.

      pWriteBuffer = xiiMemoryUtils::AddByteOffset(pWriteBuffer, static_cast<size_t>(toWrite)); // Down-cast to size_t for 32-bit.

      m_uiWritePosition += toWrite;
      uiBytesLeft -= toWrite;
    }
  }

  return XII_SUCCESS;
}

void xiiMemoryStreamWriter::SetWritePosition(xiiUInt64 uiWritePosition)
{
  XII_ASSERT_RELEASE(m_pStreamStorage != nullptr, "The memory stream writer needs a valid memory storage object!");

  XII_ASSERT_RELEASE(uiWritePosition <= GetByteCount64(), "Write position must be between 0 and GetByteCount()!");
  m_uiWritePosition = uiWritePosition;
}

xiiUInt32 xiiMemoryStreamWriter::GetByteCount32() const
{
  XII_ASSERT_DEV(m_uiWritePosition <= 0xFFFFFFFFllu, "Use GetByteCount64 instead of GetByteCount32");
  return (xiiUInt32)m_uiWritePosition;
}

xiiUInt64 xiiMemoryStreamWriter::GetByteCount64() const
{
  return m_uiWritePosition;
}

//////////////////////////////////////////////////////////////////////////

xiiMemoryStreamStorageInterface::xiiMemoryStreamStorageInterface()  = default;
xiiMemoryStreamStorageInterface::~xiiMemoryStreamStorageInterface() = default;

void xiiMemoryStreamStorageInterface::ReadAll(xiiStreamReader& Stream, xiiUInt64 uiMaxBytes /*= 0xFFFFFFFFFFFFFFFFllu*/)
{
  Clear();
  xiiMemoryStreamWriter w(this);

  xiiUInt8 uiTemp[1024 * 8];

  while (uiMaxBytes > 0)
  {
    const xiiUInt64 uiToRead = xiiMath::Min<xiiUInt64>(uiMaxBytes, XII_ARRAY_SIZE(uiTemp));

    const xiiUInt64 uiRead = Stream.ReadBytes(uiTemp, uiToRead);
    uiMaxBytes -= uiRead;

    w.WriteBytes(uiTemp, uiRead).IgnoreResult();

    if (uiRead < uiToRead)
      break;
  }
}

//////////////////////////////////////////////////////////////////////////


xiiRawMemoryStreamReader::xiiRawMemoryStreamReader() = default;

xiiRawMemoryStreamReader::xiiRawMemoryStreamReader(const void* pData, xiiUInt64 uiDataSize)
{
  Reset(pData, uiDataSize);
}

xiiRawMemoryStreamReader::~xiiRawMemoryStreamReader() = default;

void xiiRawMemoryStreamReader::Reset(const void* pData, xiiUInt64 uiDataSize)
{
  m_pRawMemory     = static_cast<const xiiUInt8*>(pData);
  m_uiChunkSize    = uiDataSize;
  m_uiReadPosition = 0;
}

xiiUInt64 xiiRawMemoryStreamReader::ReadBytes(void* pReadBuffer, xiiUInt64 uiBytesToRead)
{
  const xiiUInt64 uiBytes = xiiMath::Min<xiiUInt64>(uiBytesToRead, m_uiChunkSize - m_uiReadPosition);

  if (uiBytes == 0)
    return 0;

  if (pReadBuffer)
  {
    xiiMemoryUtils::Copy(static_cast<xiiUInt8*>(pReadBuffer), &m_pRawMemory[m_uiReadPosition], static_cast<size_t>(uiBytes));
  }

  m_uiReadPosition += uiBytes;

  return uiBytes;
}

xiiUInt64 xiiRawMemoryStreamReader::SkipBytes(xiiUInt64 uiBytesToSkip)
{
  const xiiUInt64 uiBytes = xiiMath::Min<xiiUInt64>(uiBytesToSkip, m_uiChunkSize - m_uiReadPosition);

  m_uiReadPosition += uiBytes;

  return uiBytes;
}

void xiiRawMemoryStreamReader::SetReadPosition(xiiUInt64 uiReadPosition)
{
  XII_ASSERT_RELEASE(uiReadPosition < GetByteCount(), "Read position must be between 0 and GetByteCount()!");
  m_uiReadPosition = uiReadPosition;
}

xiiUInt64 xiiRawMemoryStreamReader::GetByteCount() const
{
  return m_uiChunkSize;
}

void xiiRawMemoryStreamReader::SetDebugSourceInformation(const char* szDebugSourceInformation)
{
  m_sDebugSourceInformation = szDebugSourceInformation;
}

//////////////////////////////////////////////////////////////////////////


xiiRawMemoryStreamWriter::xiiRawMemoryStreamWriter() = default;

xiiRawMemoryStreamWriter::xiiRawMemoryStreamWriter(void* pData, xiiUInt64 uiDataSize)
{
  Reset(pData, uiDataSize);
}

xiiRawMemoryStreamWriter::~xiiRawMemoryStreamWriter() = default;

void xiiRawMemoryStreamWriter::Reset(void* pData, xiiUInt64 uiDataSize)
{
  XII_ASSERT_DEV(pData != nullptr, "Invalid memory stream storage");

  m_pRawMemory      = static_cast<xiiUInt8*>(pData);
  m_uiChunkSize     = uiDataSize;
  m_uiWritePosition = 0;
}

xiiResult xiiRawMemoryStreamWriter::WriteBytes(const void* pWriteBuffer, xiiUInt64 uiBytesToWrite)
{
  const xiiUInt64 uiBytes = xiiMath::Min<xiiUInt64>(uiBytesToWrite, m_uiChunkSize - m_uiWritePosition);

  xiiMemoryUtils::Copy(&m_pRawMemory[m_uiWritePosition], static_cast<const xiiUInt8*>(pWriteBuffer), static_cast<size_t>(uiBytes));

  m_uiWritePosition += uiBytes;

  if (uiBytes < uiBytesToWrite)
    return XII_FAILURE;

  return XII_SUCCESS;
}

xiiUInt64 xiiRawMemoryStreamWriter::GetStorageSize() const
{
  return m_uiChunkSize;
}

xiiUInt64 xiiRawMemoryStreamWriter::GetNumWrittenBytes() const
{
  return m_uiWritePosition;
}

void xiiRawMemoryStreamWriter::SetDebugSourceInformation(const char* szDebugSourceInformation)
{
  m_sDebugSourceInformation = szDebugSourceInformation;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

xiiDefaultMemoryStreamStorage::xiiDefaultMemoryStreamStorage(xiiUInt32 uiInitialCapacity, xiiAllocatorBase* pAllocator) :
  m_Chunks(pAllocator)
{
  Reserve(uiInitialCapacity);
}

xiiDefaultMemoryStreamStorage::~xiiDefaultMemoryStreamStorage()
{
  Clear();
}

void xiiDefaultMemoryStreamStorage::Reserve(xiiUInt64 bytes)
{
  if (m_Chunks.IsEmpty())
  {
    auto& chunk           = m_Chunks.ExpandAndGetRef();
    chunk.m_Bytes         = xiiByteArrayPtr(m_InplaceMemory);
    chunk.m_uiStartOffset = 0;
    m_uiCapacity          = m_Chunks[0].m_Bytes.GetCount();
  }

  while (m_uiCapacity < bytes)
  {
    AddChunk(static_cast<xiiUInt32>(xiiMath::Min<xiiUInt64>(bytes - m_uiCapacity, xiiMath::MaxValue<xiiUInt32>())));
  }
}

xiiUInt64 xiiDefaultMemoryStreamStorage::GetStorageSize64() const
{
  return m_uiInternalSize;
}

void xiiDefaultMemoryStreamStorage::Clear()
{
  m_uiInternalSize      = 0;
  m_uiLastByteAccessed  = 0;
  m_uiLastChunkAccessed = 0;
  Compact();
}

void xiiDefaultMemoryStreamStorage::Compact()
{
  // skip chunk 0, because that's where our inplace storage is used
  while (m_Chunks.GetCount() > 1)
  {
    auto& chunk = m_Chunks.PeekBack();

    if (m_uiInternalSize > m_uiCapacity - chunk.m_Bytes.GetCount())
      break;

    m_uiCapacity -= chunk.m_Bytes.GetCount();

    xiiUInt8* pData = chunk.m_Bytes.GetPtr();
    XII_DELETE_RAW_BUFFER(m_Chunks.GetAllocator(), pData);

    m_Chunks.PopBack();
  }
}

xiiUInt64 xiiDefaultMemoryStreamStorage::GetHeapMemoryUsage() const
{
  return m_Chunks.GetHeapMemoryUsage() + m_uiCapacity - m_Chunks[0].m_Bytes.GetCount();
}

xiiResult xiiDefaultMemoryStreamStorage::CopyToStream(xiiStreamWriter& stream) const
{
  xiiUInt64 uiBytesLeft    = m_uiInternalSize;
  xiiUInt64 uiReadPosition = 0;

  while (uiBytesLeft > 0)
  {
    xiiArrayPtr<const xiiUInt8> data = GetContiguousMemoryRange(uiReadPosition);

    XII_ASSERT_DEV(!data.IsEmpty(), "MemoryStreamStorage returned an empty contiguous memory block.");

    XII_SUCCEED_OR_RETURN(stream.WriteBytes(data.GetPtr(), data.GetCount()));

    uiReadPosition += data.GetCount();
    uiBytesLeft -= data.GetCount();
  }

  return XII_SUCCESS;
}

xiiArrayPtr<const xiiUInt8> xiiDefaultMemoryStreamStorage::GetContiguousMemoryRange(xiiUInt64 uiStartByte) const
{
  if (uiStartByte >= m_uiInternalSize)
    return {};

  // remember the last access (byte offset) and in which chunk that ended up, to speed up this lookup
  // if a read comes in that's not AFTER the previous one, just reset to the start

  if (uiStartByte < m_uiLastByteAccessed)
  {
    m_uiLastChunkAccessed = 0;
  }

  m_uiLastByteAccessed = uiStartByte;

  for (; m_uiLastChunkAccessed < m_Chunks.GetCount(); ++m_uiLastChunkAccessed)
  {
    const auto& chunk = m_Chunks[m_uiLastChunkAccessed];

    if (uiStartByte < chunk.m_uiStartOffset + chunk.m_Bytes.GetCount())
    {
      const xiiUInt64 uiStartByteRel = uiStartByte - chunk.m_uiStartOffset;       // start offset into the chunk
      const xiiUInt64 uiMaxLenRel    = chunk.m_Bytes.GetCount() - uiStartByteRel; // max number of bytes to use from this chunk
      const xiiUInt64 uiMaxRangeRel  = m_uiInternalSize - uiStartByte;            // the 'stored data' might be less than the capacity of the chunk

      return {chunk.m_Bytes.GetPtr() + uiStartByteRel, static_cast<xiiUInt32>(xiiMath::Min<xiiUInt64>(uiMaxRangeRel, uiMaxLenRel))};
    }
  }

  return {};
}

xiiArrayPtr<xiiUInt8> xiiDefaultMemoryStreamStorage::GetContiguousMemoryRange(xiiUInt64 uiStartByte)
{
  xiiArrayPtr<const xiiUInt8> constData = const_cast<const xiiDefaultMemoryStreamStorage*>(this)->GetContiguousMemoryRange(uiStartByte);
  return {const_cast<xiiUInt8*>(constData.GetPtr()), constData.GetCount()};
}

void xiiDefaultMemoryStreamStorage::SetInternalSize(xiiUInt64 uiSize)
{
  Reserve(uiSize);

  m_uiInternalSize = uiSize;
}

void xiiDefaultMemoryStreamStorage::AddChunk(xiiUInt32 uiMinimumSize)
{
  auto& chunk = m_Chunks.ExpandAndGetRef();

  xiiUInt32 uiSize = 0;

  if (m_Chunks.GetCount() < 4)
  {
    uiSize = 1024 * 4; // 4 KB
  }
  else if (m_Chunks.GetCount() < 8)
  {
    uiSize = 1024 * 64; // 64 KB
  }
  else if (m_Chunks.GetCount() < 16)
  {
    uiSize = 1024 * 1024 * 4; // 4 MB
  }
  else
  {
    uiSize = 1024 * 1024 * 64; // 64 MB
  }

  uiSize = xiiMath::Max(uiSize, uiMinimumSize);

  const auto& prevChunk = m_Chunks[m_Chunks.GetCount() - 2];

  chunk.m_Bytes         = xiiArrayPtr<xiiUInt8>(XII_NEW_RAW_BUFFER(m_Chunks.GetAllocator(), xiiUInt8, uiSize), uiSize);
  chunk.m_uiStartOffset = prevChunk.m_uiStartOffset + prevChunk.m_Bytes.GetCount();
  m_uiCapacity += chunk.m_Bytes.GetCount();
}


XII_STATICLINK_FILE(Foundation, Foundation_IO_Implementation_MemoryStream);

/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Containers/HybridArray.h>
#include <Foundation/IO/Stream.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Types/RefCounted.h>

class xiiMemoryStreamReader;
class xiiMemoryStreamWriter;

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

/// \brief Instances of this class act as storage for memory streams
class XII_FOUNDATION_DLL xiiMemoryStreamStorageInterface
{
public:
  xiiMemoryStreamStorageInterface();
  virtual ~xiiMemoryStreamStorageInterface();

  /// \brief Returns the number of bytes that are currently stored. Asserts that the stored amount is less than 4GB.
  xiiUInt32 GetStorageSize32() const
  {
    XII_ASSERT_ALWAYS(GetStorageSize64() <= xiiMath::MaxValue<xiiUInt32>(), "The memory stream storage object has grown beyond 4GB. The code using it has to be adapted to support this.");
    return (xiiUInt32)GetStorageSize64();
  }

  /// \brief Returns the number of bytes that are currently stored.
  virtual xiiUInt64 GetStorageSize64() const = 0; // [tested]

  /// \brief Clears the entire storage. All readers and writers must be reset to start from the beginning again.
  virtual void Clear() = 0;

  /// \brief Deallocates any allocated memory that's not needed to hold the currently stored data.
  virtual void Compact() = 0;

  /// \brief Returns the amount of bytes that are currently allocated on the heap.
  virtual xiiUInt64 GetHeapMemoryUsage() const = 0;

  /// \brief Copies all data from the given stream into the storage.
  void ReadAll(xiiStreamReader& ref_stream, xiiUInt64 uiMaxBytes = xiiMath::MaxValue<xiiUInt64>());

  /// \brief Reserves N bytes of storage.
  virtual void Reserve(xiiUInt64 uiBytes) = 0;

  /// \brief Writes the entire content of the storage to the provided stream.
  virtual xiiResult CopyToStream(xiiStreamWriter& ref_stream) const = 0;

  /// \brief Returns a read-only xiiArrayPtr that represents a contiguous area in memory which starts at the given first byte.
  ///
  /// This piece of memory can be read/copied/modified in one operation (memcpy etc).
  /// The next byte after this slice may be located somewhere entirely different in memory.
  /// Call GetContiguousMemoryRange() again with the next byte after this range, to get access to the next memory area.
  ///
  /// Chunks may differ in size.
  virtual xiiArrayPtr<const xiiUInt8> GetContiguousMemoryRange(xiiUInt64 uiStartByte) const = 0;

  /// Non-const overload of GetContiguousMemoryRange().
  virtual xiiArrayPtr<xiiUInt8> GetContiguousMemoryRange(xiiUInt64 uiStartByte) = 0;

private:
  virtual void SetInternalSize(xiiUInt64 uiSize) = 0;

  friend class xiiMemoryStreamReader;
  friend class xiiMemoryStreamWriter;
};


//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

/// \brief Templated implementation of xiiMemoryStreamStorageInterface that adapts most standard XII containers to the interface.
///
/// Note that xiiMemoryStreamContainerStorage assumes contiguous storage, so using a xiiDeque for storage will not work.
template <typename CONTAINER>
class xiiMemoryStreamContainerStorage : public xiiMemoryStreamStorageInterface
{
public:
  /// \brief Creates the storage object for a memory stream. Use \a uiInitialCapacity to reserve some memory up front.
  xiiMemoryStreamContainerStorage(xiiUInt32 uiInitialCapacity = 0, xiiAllocator* pAllocator = xiiFoundation::GetDefaultAllocator()) :
    m_Storage(pAllocator)
  {
    m_Storage.Reserve(uiInitialCapacity);
  }

  virtual xiiUInt64 GetStorageSize64() const override { return m_Storage.GetCount(); }
  virtual void      Clear() override { m_Storage.Clear(); }
  virtual void      Compact() override { m_Storage.Compact(); }
  virtual xiiUInt64 GetHeapMemoryUsage() const override { return m_Storage.GetHeapMemoryUsage(); }

  virtual void Reserve(xiiUInt64 uiBytes) override
  {
    XII_ASSERT_DEV(uiBytes <= xiiMath::MaxValue<xiiUInt32>(), "xiiMemoryStreamContainerStorage only supports 32 bit addressable sizes.");
    m_Storage.Reserve(static_cast<xiiUInt32>(uiBytes));
  }

  virtual xiiResult CopyToStream(xiiStreamWriter& ref_stream) const override
  {
    return ref_stream.WriteBytes(m_Storage.GetData(), m_Storage.GetCount());
  }

  virtual xiiArrayPtr<const xiiUInt8> GetContiguousMemoryRange(xiiUInt64 uiStartByte) const override
  {
    if (uiStartByte >= m_Storage.GetCount())
      return {};

    return xiiArrayPtr<const xiiUInt8>(m_Storage.GetData() + uiStartByte, m_Storage.GetCount() - static_cast<xiiUInt32>(uiStartByte));
  }

  virtual xiiArrayPtr<xiiUInt8> GetContiguousMemoryRange(xiiUInt64 uiStartByte) override
  {
    if (uiStartByte >= m_Storage.GetCount())
      return {};

    return xiiArrayPtr<xiiUInt8>(m_Storage.GetData() + uiStartByte, m_Storage.GetCount() - static_cast<xiiUInt32>(uiStartByte));
  }

  /// \brief The data is guaranteed to be contiguous.
  const xiiUInt8* GetData() const { return m_Storage.GetData(); }

private:
  virtual void SetInternalSize(xiiUInt64 uiSize) override
  {
    XII_ASSERT_DEV(uiSize <= xiiMath::MaxValue<xiiUInt32>(), "Storage that large is not supported.");
    m_Storage.SetCountUninitialized(static_cast<xiiUInt32>(uiSize));
  }

  CONTAINER m_Storage;
};


//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////


/// xiiContiguousMemoryStreamStorage holds internally a xiiHybridArray<xiiUInt8, 256>, to prevent allocations when only small temporary memory streams
/// are needed. That means it will have a memory overhead of that size.
/// Also it reallocates memory on demand, and the data is guaranteed to be contiguous. This may be desirable,
/// but can have a high performance overhead when data grows very large.
class XII_FOUNDATION_DLL xiiContiguousMemoryStreamStorage : public xiiMemoryStreamContainerStorage<xiiHybridArray<xiiUInt8, 256>>
{
public:
  xiiContiguousMemoryStreamStorage(xiiUInt32 uiInitialCapacity = 0, xiiAllocator* pAllocator = xiiFoundation::GetDefaultAllocator()) :
    xiiMemoryStreamContainerStorage<xiiHybridArray<xiiUInt8, 256>>(uiInitialCapacity, pAllocator)
  {
  }
};

/// \brief The default implementation for memory stream storage.
///
/// This implementation of xiiMemoryStreamStorageInterface handles use cases both from very small to extremely large storage needs.
/// It starts out with some inplace memory that can accommodate small amounts of data.
/// To grow, additional chunks of data are allocated. No memory ever needs to be copied to grow the container.
/// However, that also means that the memory isn't stored in one contiguous array, therefore data has to be accessed piece-wise
/// through GetContiguousMemoryRange().
class XII_FOUNDATION_DLL xiiDefaultMemoryStreamStorage final : public xiiMemoryStreamStorageInterface
{
public:
  xiiDefaultMemoryStreamStorage(xiiUInt32 uiInitialCapacity = 0, xiiAllocator* pAllocator = xiiFoundation::GetDefaultAllocator());
  ~xiiDefaultMemoryStreamStorage();

  virtual void Reserve(xiiUInt64 uiBytes) override; // [tested]

  virtual xiiUInt64                   GetStorageSize64() const override; // [tested]
  virtual void                        Clear() override;
  virtual void                        Compact() override;
  virtual xiiUInt64                   GetHeapMemoryUsage() const override;
  virtual xiiResult                   CopyToStream(xiiStreamWriter& ref_stream) const override;
  virtual xiiArrayPtr<const xiiUInt8> GetContiguousMemoryRange(xiiUInt64 uiStartByte) const override; // [tested]
  virtual xiiArrayPtr<xiiUInt8>       GetContiguousMemoryRange(xiiUInt64 uiStartByte) override;       // [tested]

private:
  virtual void SetInternalSize(xiiUInt64 uiSize) override;

  void AddChunk(xiiUInt32 uiMinimumSize);

  struct Chunk
  {
    xiiUInt64             m_uiStartOffset = 0;
    xiiArrayPtr<xiiUInt8> m_Bytes;
  };

  xiiHybridArray<Chunk, 16> m_Chunks;

  xiiUInt64         m_uiCapacity     = 0;
  xiiUInt64         m_uiInternalSize = 0;
  xiiUInt8          m_InplaceMemory[512]; // used for the very first bytes, might cover small memory streams without an allocation
  mutable xiiUInt32 m_uiLastChunkAccessed = 0;
  mutable xiiUInt64 m_uiLastByteAccessed  = 0;
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

/// \brief Wrapper around an existing container to implement xiiMemoryStreamStorageInterface
template <typename CONTAINER>
class xiiMemoryStreamContainerWrapperStorage : public xiiMemoryStreamStorageInterface
{
public:
  xiiMemoryStreamContainerWrapperStorage(CONTAINER* pContainer) { m_pStorage = pContainer; }

  virtual xiiUInt64 GetStorageSize64() const override { return m_pStorage->GetCount(); }

  virtual void Clear() override
  {
    if constexpr (!std::is_const<CONTAINER>::value)
    {
      m_pStorage->Clear();
    }
  }

  virtual void Compact() override
  {
    if constexpr (!std::is_const<CONTAINER>::value)
    {
      m_pStorage->Compact();
    }
  }

  virtual xiiUInt64 GetHeapMemoryUsage() const override { return m_pStorage->GetHeapMemoryUsage(); }

  virtual void Reserve(xiiUInt64 uiBytes) override
  {
    if constexpr (!std::is_const<CONTAINER>::value)
    {
      XII_ASSERT_DEV(uiBytes <= xiiMath::MaxValue<xiiUInt32>(), "xiiMemoryStreamContainerWrapperStorage only supports 32 bit addressable sizes.");
      m_pStorage->Reserve(static_cast<xiiUInt32>(uiBytes));
    }
  }

  virtual xiiResult CopyToStream(xiiStreamWriter& ref_stream) const override
  {
    return ref_stream.WriteBytes(m_pStorage->GetData(), m_pStorage->GetCount());
  }

  virtual xiiArrayPtr<const xiiUInt8> GetContiguousMemoryRange(xiiUInt64 uiStartByte) const override
  {
    if (uiStartByte >= m_pStorage->GetCount())
      return {};

    return xiiArrayPtr<const xiiUInt8>(m_pStorage->GetData() + uiStartByte, m_pStorage->GetCount() - static_cast<xiiUInt32>(uiStartByte));
  }

  virtual xiiArrayPtr<xiiUInt8> GetContiguousMemoryRange(xiiUInt64 uiStartByte) override
  {
    if constexpr (!std::is_const<CONTAINER>::value)
    {
      if (uiStartByte >= m_pStorage->GetCount())
        return {};

      return xiiArrayPtr<xiiUInt8>(m_pStorage->GetData() + uiStartByte, m_pStorage->GetCount() - static_cast<xiiUInt32>(uiStartByte));
    }
    else
    {
      return {};
    }
  }

private:
  virtual void SetInternalSize(xiiUInt64 uiSize) override
  {
    if (!std::is_const<CONTAINER>::value)
    {
      XII_ASSERT_DEV(uiSize <= xiiMath::MaxValue<xiiUInt32>(), "xiiMemoryStreamContainerWrapperStorage only supports up to 4GB sizes.");
      m_pStorage->SetCountUninitialized(static_cast<xiiUInt32>(uiSize));
    }
  }

  CONTAINER* m_pStorage;
};


//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

/// \brief A reader which can access a memory stream.
///
/// Please note that the functions exposed by this object are not thread safe! If access to the same xiiMemoryStreamStorage object from
/// multiple threads is desired please create one instance of xiiMemoryStreamReader per thread.
class XII_FOUNDATION_DLL xiiMemoryStreamReader : public xiiStreamReader
{
public:
  /// \brief Pass the memory storage object from which to read from.
  /// Pass nullptr if you are going to set the storage stream later via SetStorage().
  xiiMemoryStreamReader(const xiiMemoryStreamStorageInterface* pStreamStorage = nullptr);

  ~xiiMemoryStreamReader();

  /// \brief Sets the storage object upon which to operate. Resets the read position to zero.
  /// Pass nullptr if you want to detach from any previous storage stream, for example to ensure its reference count gets properly reduced.
  void SetStorage(const xiiMemoryStreamStorageInterface* pStreamStorage)
  {
    m_pStreamStorage = pStreamStorage;
    m_uiReadPosition = 0;
  }

  /// \brief Reads either uiBytesToRead or the amount of remaining bytes in the stream into pReadBuffer.
  ///
  /// It is valid to pass nullptr for pReadBuffer, in this case the memory stream position is only advanced by the given number of bytes.
  virtual xiiUInt64 ReadBytes(void* pReadBuffer, xiiUInt64 uiBytesToRead) override; // [tested]

  /// \brief Skips bytes in the stream (e.g. for skipping objects which can't be serialized due to missing information etc.)
  virtual xiiUInt64 SkipBytes(xiiUInt64 uiBytesToSkip) override; // [tested]

  /// \brief Sets the read position to be used
  void SetReadPosition(xiiUInt64 uiReadPosition); // [tested]

  /// \brief Returns the current read position
  xiiUInt64 GetReadPosition() const { return m_uiReadPosition; }

  /// \brief Returns the total available bytes in the memory stream
  xiiUInt32 GetByteCount32() const; // [tested]
  xiiUInt64 GetByteCount64() const; // [tested]

  /// \brief Allows to set a string as the source of information in the memory stream for debug purposes.
  void SetDebugSourceInformation(xiiStringView sDebugSourceInformation);

private:
  const xiiMemoryStreamStorageInterface* m_pStreamStorage = nullptr;

  xiiString m_sDebugSourceInformation;

  xiiUInt64 m_uiReadPosition = 0;
};


//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

/// \brief A writer which can access a memory stream
///
/// Please note that the functions exposed by this object are not thread safe!
class XII_FOUNDATION_DLL xiiMemoryStreamWriter : public xiiStreamWriter
{
public:
  /// \brief Pass the memory storage object to which to write to.
  xiiMemoryStreamWriter(xiiMemoryStreamStorageInterface* pStreamStorage = nullptr);

  ~xiiMemoryStreamWriter();

  /// \brief Sets the storage object upon which to operate. Resets the write position to the end of the storage stream.
  /// Pass nullptr if you want to detach from any previous storage stream, for example to ensure its reference count gets properly reduced.
  void SetStorage(xiiMemoryStreamStorageInterface* pStreamStorage)
  {
    m_pStreamStorage  = pStreamStorage;
    m_uiWritePosition = 0;
    if (m_pStreamStorage)
      m_uiWritePosition = m_pStreamStorage->GetStorageSize64();
  }

  /// \brief Copies uiBytesToWrite from pWriteBuffer into the memory stream.
  ///
  /// pWriteBuffer must be a valid buffer and must hold that much data.
  virtual xiiResult WriteBytes(const void* pWriteBuffer, xiiUInt64 uiBytesToWrite) override; // [tested]

  /// \brief Sets the write position to be used
  void SetWritePosition(xiiUInt64 uiWritePosition); // [tested]

  /// \brief Returns the current write position
  xiiUInt64 GetWritePosition() const { return m_uiWritePosition; }

  /// \brief Returns the total stored bytes in the memory stream
  xiiUInt32 GetByteCount32() const; // [tested]
  xiiUInt64 GetByteCount64() const; // [tested]

private:
  xiiMemoryStreamStorageInterface* m_pStreamStorage = nullptr;

  xiiUInt64 m_uiWritePosition = 0;
};


//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

/// \brief Maps a raw chunk of memory to the xiiStreamReader interface.
class XII_FOUNDATION_DLL xiiRawMemoryStreamReader : public xiiStreamReader
{
public:
  xiiRawMemoryStreamReader();

  /// \brief Initialize the raw memory reader with the chunk of memory that is the data storage.
  xiiRawMemoryStreamReader(const void* pData, xiiUInt64 uiDataSize); // [tested]

  /// \brief Initialize the raw memory reader with the chunk of memory from a standard XII container.
  /// \note The container must store the data in a contiguous array.
  template <typename CONTAINER>
  xiiRawMemoryStreamReader(const CONTAINER& container) // [tested]
  {
    Reset(container);
  }

  ~xiiRawMemoryStreamReader();

  void Reset(const void* pData, xiiUInt64 uiDataSize); // [tested]

  template <typename CONTAINER>
  void Reset(const CONTAINER& container) // [tested]
  {
    Reset(static_cast<const xiiUInt8*>(container.GetData()), container.GetCount());
  }

  /// \brief Reads either uiBytesToRead or the amount of remaining bytes in the stream into pReadBuffer.
  ///
  /// It is valid to pass nullptr for pReadBuffer, in this case the memory stream position is only advanced by the given number of bytes.
  virtual xiiUInt64 ReadBytes(void* pReadBuffer, xiiUInt64 uiBytesToRead) override; // [tested]

  /// \brief Skips bytes in the stream (e.g. for skipping objects which can't be serialized due to missing information etc.)
  virtual xiiUInt64 SkipBytes(xiiUInt64 uiBytesToSkip) override; // [tested]

  /// \brief Sets the read position to be used
  void SetReadPosition(xiiUInt64 uiReadPosition); // [tested]

  /// \brief Returns the current read position in the raw memory block
  xiiUInt64 GetReadPosition() const { return m_uiReadPosition; }

  /// \brief Returns the total available bytes in the memory stream
  xiiUInt64 GetByteCount() const; // [tested]

  /// \brief Allows to set a string as the source of information in the memory stream for debug purposes.
  void SetDebugSourceInformation(xiiStringView sDebugSourceInformation);

private:
  const xiiUInt8* m_pRawMemory = nullptr;

  xiiUInt64 m_uiChunkSize    = 0;
  xiiUInt64 m_uiReadPosition = 0;

  xiiString m_sDebugSourceInformation;
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////


/// \brief Maps a raw chunk of memory to the xiiStreamReader interface.
class XII_FOUNDATION_DLL xiiRawMemoryStreamWriter : public xiiStreamWriter
{
public:
  xiiRawMemoryStreamWriter(); // [tested]

  /// \brief Initialize the raw memory reader with the chunk of memory that is the data storage.
  xiiRawMemoryStreamWriter(void* pData, xiiUInt64 uiDataSize); // [tested]

  /// \brief Initialize the raw memory reader with the chunk of memory from a standard XII container.
  /// \note The container must store the data in a contiguous array.
  template <typename CONTAINER>
  xiiRawMemoryStreamWriter(CONTAINER& ref_container) // [tested]
  {
    Reset(ref_container);
  }

  ~xiiRawMemoryStreamWriter(); // [tested]

  void Reset(void* pData, xiiUInt64 uiDataSize); // [tested]

  template <typename CONTAINER>
  void Reset(CONTAINER& ref_container) // [tested]
  {
    Reset(static_cast<xiiUInt8*>(ref_container.GetData()), ref_container.GetCount());
  }

  /// \brief Returns the total available bytes in the memory stream
  xiiUInt64 GetStorageSize() const; // [tested]

  /// \brief Returns the number of bytes written to the storage
  xiiUInt64 GetNumWrittenBytes() const; // [tested]

  /// \brief Allows to set a string as the source of information in the memory stream for debug purposes.
  void SetDebugSourceInformation(xiiStringView sDebugSourceInformation);

  /// \brief Copies uiBytesToWrite from pWriteBuffer into the memory stream.
  ///
  /// pWriteBuffer must be a valid buffer and must hold that much data.
  virtual xiiResult WriteBytes(const void* pWriteBuffer, xiiUInt64 uiBytesToWrite) override; // [tested]

private:
  xiiUInt8* m_pRawMemory = nullptr;

  xiiUInt64 m_uiChunkSize     = 0;
  xiiUInt64 m_uiWritePosition = 0;

  xiiString m_sDebugSourceInformation;
};

/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Foundation/FoundationPCH.h>

#include <Foundation/IO/Archive/ArchiveUtils.h>

#include <Foundation/Algorithm/HashStream.h>
#include <Foundation/IO/CompressedStreamZstd.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/MemoryMappedFile.h>
#include <Foundation/IO/MemoryStream.h>
#include <Foundation/Logging/Log.h>

xiiHybridArray<xiiString, 4, xiiStaticAllocatorWrapper>& xiiArchiveUtils::GetAcceptedArchiveFileExtensions()
{
  static xiiHybridArray<xiiString, 4, xiiStaticAllocatorWrapper> extensions;

  if (extensions.IsEmpty())
  {
    extensions.PushBack("xiiArchive");
  }

  return extensions;
}

bool xiiArchiveUtils::IsAcceptedArchiveFileExtensions(xiiStringView sExtension)
{
  for (const auto& ext : GetAcceptedArchiveFileExtensions())
  {
    if (sExtension.IsEqual_NoCase(ext.GetView()))
      return true;
  }

  return false;
}

xiiResult xiiArchiveUtils::WriteHeader(xiiStreamWriter& ref_stream)
{
  static_assert(17 == ArchiveHeaderSize);

  const char* szTag = "XIIARCHIVE";
  XII_SUCCEED_OR_RETURN(ref_stream.WriteBytes(szTag, 11));

  const xiiUInt8 uiArchiveVersion = 4;

  // Version 2: Added end-of-file marker for file corruption (cutoff) detection
  // Version 3: HashedStrings changed from MurmurHash to xxHash
  // Version 4: use 64 Bit string hashes
  ref_stream << uiArchiveVersion;

  const xiiUInt8 uiPadding[5] = {0, 0, 0, 0, 0};
  XII_SUCCEED_OR_RETURN(ref_stream.WriteBytes(uiPadding, 5));

  return XII_SUCCESS;
}

xiiResult xiiArchiveUtils::ReadHeader(xiiStreamReader& ref_stream, xiiUInt8& out_uiVersion)
{
  static_assert(17 == ArchiveHeaderSize);

  char szTag[11];
  if (ref_stream.ReadBytes(szTag, 11) != 11 || !xiiStringUtils::IsEqual(szTag, "XIIARCHIVE"))
  {
    xiiLog::Error("Invalid or corrupted archive. Archive-marker not found.");
    return XII_FAILURE;
  }

  out_uiVersion = 0;
  ref_stream >> out_uiVersion;

  if (out_uiVersion != 1 && out_uiVersion != 2 && out_uiVersion != 3 && out_uiVersion != 4)
  {
    xiiLog::Error("Unsupported archive version '{}'.", out_uiVersion);
    return XII_FAILURE;
  }

  xiiUInt8 uiPadding[5] = {255, 255, 255, 255, 255};
  if (ref_stream.ReadBytes(uiPadding, 5) != 5)
  {
    xiiLog::Error("Invalid or corrupted archive. Missing header data.");
    return XII_FAILURE;
  }

  const xiiUInt8 uiZeroPadding[5] = {0, 0, 0, 0, 0};

  if (xiiMemoryUtils::Compare<xiiUInt8>(uiPadding, uiZeroPadding, 5) != 0)
  {
    xiiLog::Error("Invalid or corrupted archive. Unexpected header data.");
    return XII_FAILURE;
  }

  return XII_SUCCESS;
}

xiiResult xiiArchiveUtils::WriteEntryPreprocessed(xiiStreamWriter& ref_stream, xiiConstByteArrayPtr entryData, xiiUInt32 uiPathStringOffset, xiiArchiveCompressionMode compression, xiiUInt32 uiUncompressedEntryDataSize, xiiArchiveEntry& ref_tocEntry, xiiUInt64& inout_uiCurrentStreamPosition)
{
  XII_SUCCEED_OR_RETURN(ref_stream.WriteBytes(entryData.GetPtr(), entryData.GetCount()));

  ref_tocEntry.m_uiPathStringOffset     = uiPathStringOffset;
  ref_tocEntry.m_uiDataStartOffset      = inout_uiCurrentStreamPosition;
  ref_tocEntry.m_uiUncompressedDataSize = uiUncompressedEntryDataSize;
  ref_tocEntry.m_uiStoredDataSize       = entryData.GetCount();
  ref_tocEntry.m_CompressionMode        = compression;

  inout_uiCurrentStreamPosition += entryData.GetCount();

  return XII_SUCCESS;
}

xiiResult xiiArchiveUtils::WriteEntry(xiiStreamWriter& ref_stream, xiiStringView sAbsSourcePath, xiiUInt32 uiPathStringOffset, xiiArchiveCompressionMode compression, xiiInt32 iCompressionLevel, xiiArchiveEntry& inout_tocEntry, xiiUInt64& inout_uiCurrentStreamPosition, FileWriteProgressCallback progress /*= FileWriteProgressCallback()*/)
{
  XII_IGNORE_UNUSED(iCompressionLevel);

  xiiFileReader file;
  XII_SUCCEED_OR_RETURN(file.Open(sAbsSourcePath, 1024 * 1024));

  const xiiUInt64 uiMaxBytes = file.GetFileSize();

#ifdef BUILDSYSTEM_ENABLE_ZSTD_SUPPORT
  constexpr xiiUInt32 uiMaxNumWorkerThreads = 12U;

  xiiUInt32 uiWorkerThreadCount;
  if (uiMaxBytes > xiiMath::MaxValue<xiiUInt32>())
  {
    uiWorkerThreadCount = uiMaxNumWorkerThreads;
  }
  else
  {
    constexpr xiiUInt32 uiBytesPerThread = 1024u * 1024u;
    uiWorkerThreadCount                  = xiiMath::Clamp((xiiUInt32)floor(uiMaxBytes / uiBytesPerThread), 1u, uiMaxNumWorkerThreads);
  }
#endif

  inout_tocEntry.m_uiPathStringOffset     = uiPathStringOffset;
  inout_tocEntry.m_uiDataStartOffset      = inout_uiCurrentStreamPosition;
  inout_tocEntry.m_uiUncompressedDataSize = 0;

  xiiStreamWriter* pWriter = &ref_stream;

#ifdef BUILDSYSTEM_ENABLE_ZSTD_SUPPORT
  xiiCompressedStreamWriterZstd zstdWriter;
#endif

  switch (compression)
  {
    case xiiArchiveCompressionMode::Uncompressed:
      break;

#ifdef BUILDSYSTEM_ENABLE_ZSTD_SUPPORT
    case xiiArchiveCompressionMode::Compressed_zstd:
    {
      zstdWriter.SetOutputStream(&ref_stream, uiWorkerThreadCount, (xiiCompressedStreamWriterZstd::Compression)iCompressionLevel);
      pWriter = &zstdWriter;
    }
    break;
#endif

    default:
    {
      compression = xiiArchiveCompressionMode::Uncompressed;
    }
    break;
  }

  inout_tocEntry.m_CompressionMode = compression;

  xiiUInt64                 uiRead = 0;
  xiiDynamicArray<xiiUInt8> buffer;
  buffer.SetCountUninitialized(1024 * 32);

  while (true)
  {
    uiRead = file.ReadBytes(buffer.GetData(), buffer.GetCount());

    if (uiRead == 0)
      break;

    inout_tocEntry.m_uiUncompressedDataSize += uiRead;

    if (progress.IsValid())
    {
      if (!progress(inout_tocEntry.m_uiUncompressedDataSize, uiMaxBytes))
        return XII_FAILURE;
    }

    XII_SUCCEED_OR_RETURN(pWriter->WriteBytes(buffer.GetData(), uiRead));
  }


  switch (compression)
  {
#ifdef BUILDSYSTEM_ENABLE_ZSTD_SUPPORT
    case xiiArchiveCompressionMode::Compressed_zstd:
      XII_SUCCEED_OR_RETURN(zstdWriter.FinishCompressedStream());
      inout_tocEntry.m_uiStoredDataSize = zstdWriter.GetWrittenBytes();
      break;
#endif

    case xiiArchiveCompressionMode::Uncompressed:
    default:
    {
      inout_tocEntry.m_uiStoredDataSize = inout_tocEntry.m_uiUncompressedDataSize;
    }
    break;
  }

  inout_uiCurrentStreamPosition += inout_tocEntry.m_uiStoredDataSize;

  return XII_SUCCESS;
}

xiiResult xiiArchiveUtils::WriteEntryOptimal(xiiStreamWriter& ref_stream, xiiStringView sAbsSourcePath, xiiUInt32 uiPathStringOffset, xiiArchiveCompressionMode compression, xiiInt32 iCompressionLevel, xiiArchiveEntry& ref_tocEntry, xiiUInt64& inout_uiCurrentStreamPosition, FileWriteProgressCallback progress /*= FileWriteProgressCallback()*/)
{
  if (compression == xiiArchiveCompressionMode::Uncompressed)
  {
    return WriteEntry(ref_stream, sAbsSourcePath, uiPathStringOffset, xiiArchiveCompressionMode::Uncompressed, iCompressionLevel, ref_tocEntry, inout_uiCurrentStreamPosition, progress);
  }
  else
  {
    xiiDefaultMemoryStreamStorage storage;
    xiiMemoryStreamWriter         writer(&storage);

    xiiUInt64 streamPos = inout_uiCurrentStreamPosition;
    XII_SUCCEED_OR_RETURN(WriteEntry(writer, sAbsSourcePath, uiPathStringOffset, compression, iCompressionLevel, ref_tocEntry, streamPos, progress));

    if (ref_tocEntry.m_uiStoredDataSize * 12 >= ref_tocEntry.m_uiUncompressedDataSize * 10)
    {
      // less than 20% size saving -> go uncompressed
      return WriteEntry(ref_stream, sAbsSourcePath, uiPathStringOffset, xiiArchiveCompressionMode::Uncompressed, iCompressionLevel, ref_tocEntry, inout_uiCurrentStreamPosition, progress);
    }
    else
    {
      auto res                      = storage.CopyToStream(ref_stream);
      inout_uiCurrentStreamPosition = streamPos;

      return res;
    }
  }
}

#ifdef BUILDSYSTEM_ENABLE_ZSTD_SUPPORT

class xiiCompressedStreamReaderZstdWithSource : public xiiCompressedStreamReaderZstd
{
public:
  xiiRawMemoryStreamReader m_Source;
};

#endif

xiiUniquePtr<xiiStreamReader> xiiArchiveUtils::CreateEntryReader(const xiiArchiveEntry& entry, const void* pStartOfArchiveData)
{
  xiiUniquePtr<xiiStreamReader> reader;

  switch (entry.m_CompressionMode)
  {
    case xiiArchiveCompressionMode::Uncompressed:
    {
      reader                               = XII_DEFAULT_NEW(xiiRawMemoryStreamReader);
      xiiRawMemoryStreamReader* pRawReader = static_cast<xiiRawMemoryStreamReader*>(reader.Borrow());
      ConfigureRawMemoryStreamReader(entry, pStartOfArchiveData, *pRawReader);
    }
    break;

#ifdef BUILDSYSTEM_ENABLE_ZSTD_SUPPORT
    case xiiArchiveCompressionMode::Compressed_zstd:
    {
      reader                                              = XII_DEFAULT_NEW(xiiCompressedStreamReaderZstdWithSource);
      xiiCompressedStreamReaderZstdWithSource* pRawReader = static_cast<xiiCompressedStreamReaderZstdWithSource*>(reader.Borrow());
      ConfigureRawMemoryStreamReader(entry, pStartOfArchiveData, pRawReader->m_Source);
      pRawReader->SetInputStream(&pRawReader->m_Source);
    }
    break;
#endif

    default:
      XII_REPORT_FAILURE("Archive entry compression mode '{}' is not supported by xiiArchiveReader", (xiiInt32)entry.m_CompressionMode);
      break;
  }

  return std::move(reader);
}

void xiiArchiveUtils::ConfigureRawMemoryStreamReader(const xiiArchiveEntry& entry, const void* pStartOfArchiveData, xiiRawMemoryStreamReader& ref_memReader)
{
  ref_memReader.Reset(xiiMemoryUtils::AddByteOffset(pStartOfArchiveData, static_cast<std::ptrdiff_t>(entry.m_uiDataStartOffset)), entry.m_uiStoredDataSize);
}

static const char* szEndMarker = "XIIARCHIVE-END";

static xiiUInt32 GetEndMarkerSize(xiiUInt8 uiFileVersion)
{
  if (uiFileVersion == 1)
    return 0;

  return 15;
}

static xiiUInt32 GetTocMetaSize(xiiUInt8 uiFileVersion)
{
  if (uiFileVersion == 1)
    return sizeof(xiiUInt32); // TOC size

  return sizeof(xiiUInt32) /* TOC size */ + sizeof(xiiUInt64) /* TOC hash */;
}

struct TocMetaData
{
  xiiUInt32 m_uiSize = 0;
  xiiUInt64 m_uiHash = 0;
};

xiiResult xiiArchiveUtils::AppendTOC(xiiStreamWriter& ref_stream, const xiiArchiveTOC& toc)
{
  xiiDefaultMemoryStreamStorage storage;
  xiiMemoryStreamWriter         writer(&storage);

  XII_SUCCEED_OR_RETURN(toc.Serialize(writer));

  XII_SUCCEED_OR_RETURN(storage.CopyToStream(ref_stream));

  TocMetaData tocMeta;

  xiiHashStreamWriter64 hashStream(tocMeta.m_uiSize);
  XII_SUCCEED_OR_RETURN(storage.CopyToStream(hashStream));

  // Added in file version 2: hash of the TOC
  tocMeta.m_uiSize = storage.GetStorageSize32();
  tocMeta.m_uiHash = hashStream.GetHashValue();

  // append the TOC meta data
  ref_stream << tocMeta.m_uiSize;
  ref_stream << tocMeta.m_uiHash;

  // write an 'end' marker
  return ref_stream.WriteBytes(szEndMarker, 15);
}

static xiiResult VerifyEndMarker(xiiUInt64 uiArchiveDataSize, const void* pArchiveDataBuffer, xiiUInt8 uiArchiveVersion)
{
  const xiiUInt32 uiEndMarkerSize = GetEndMarkerSize(uiArchiveVersion);

  if (uiEndMarkerSize == 0)
  {
    return XII_SUCCESS;
  }

  if (uiEndMarkerSize > uiArchiveDataSize)
  {
    xiiLog::Error("Archive is too small. End-marker not found.");
    return XII_FAILURE;
  }

  const void* pStart = xiiMemoryUtils::AddByteOffset(pArchiveDataBuffer, static_cast<ptrdiff_t>(uiArchiveDataSize - uiEndMarkerSize));

  xiiRawMemoryStreamReader reader(pStart, uiEndMarkerSize);

  char szMarker[32] = "";
  if (reader.ReadBytes(szMarker, uiEndMarkerSize) != uiEndMarkerSize || !xiiStringUtils::IsEqual(szMarker, szEndMarker))
  {
    xiiLog::Error("Archive is corrupt or cut off. End-marker not found.");
    return XII_FAILURE;
  }

  return XII_SUCCESS;
}

xiiResult xiiArchiveUtils::ExtractTOCMeta(xiiUInt64 uiArchiveEndingDataSize, const void* pArchiveEndingDataBuffer, TOCMeta& ref_tocMeta, xiiUInt8 uiArchiveVersion)
{
  XII_SUCCEED_OR_RETURN(VerifyEndMarker(uiArchiveEndingDataSize, pArchiveEndingDataBuffer, uiArchiveVersion));

  const xiiUInt32 uiEndMarkerSize = GetEndMarkerSize(uiArchiveVersion);
  const xiiUInt32 uiTocMetaSize   = GetTocMetaSize(uiArchiveVersion);

  xiiUInt32 uiTocSize         = 0;
  xiiUInt64 uiExpectedTocHash = 0;

  // read the TOC meta data
  {
    XII_ASSERT_DEV(uiEndMarkerSize + uiTocMetaSize <= ArchiveTOCMetaMaxFooterSize, "");

    if (uiEndMarkerSize + uiTocMetaSize > uiArchiveEndingDataSize)
    {
      xiiLog::Error("Unable to extract Archive TOC. File size too small: {0}", xiiArgFileSize(uiArchiveEndingDataSize));
      return XII_FAILURE;
    }

    const void* pTocMetaStart = xiiMemoryUtils::AddByteOffset(pArchiveEndingDataBuffer, static_cast<ptrdiff_t>(uiArchiveEndingDataSize - uiEndMarkerSize - uiTocMetaSize));

    xiiRawMemoryStreamReader tocMetaReader(pTocMetaStart, uiTocMetaSize);

    tocMetaReader >> uiTocSize;

    if (uiTocSize > 1024 * 1024 * 1024) // 1GB of TOC is enough for ~16M entries...
    {
      xiiLog::Error("Archive TOC is probably corrupted. Unreasonable TOC size: {0}", xiiArgFileSize(uiTocSize));
      return XII_FAILURE;
    }

    if (uiArchiveVersion >= 2)
    {
      tocMetaReader >> uiExpectedTocHash;
    }
  }

  // output the result
  {
    ref_tocMeta                             = TOCMeta();
    ref_tocMeta.m_uiTocSize                 = uiTocSize;
    ref_tocMeta.m_uiExpectedTocHash         = uiExpectedTocHash;
    ref_tocMeta.m_uiTocOffsetFromArchiveEnd = uiTocSize + uiTocMetaSize + uiEndMarkerSize;
  }

  return XII_SUCCESS;
}

xiiResult xiiArchiveUtils::ExtractTOCMeta(const xiiMemoryMappedFile& memFile, TOCMeta& ref_tocMeta, xiiUInt8 uiArchiveVersion)
{
  return ExtractTOCMeta(memFile.GetFileSize(), memFile.GetReadPointer(), ref_tocMeta, uiArchiveVersion);
}

xiiResult xiiArchiveUtils::ExtractTOC(xiiUInt64 uiArchiveEndingDataSize, const void* pArchiveEndingDataBuffer, xiiArchiveTOC& ref_toc, xiiUInt8 uiArchiveVersion)
{
  // get toc meta
  TOCMeta tocMeta;
  if (ExtractTOCMeta(uiArchiveEndingDataSize, pArchiveEndingDataBuffer, tocMeta, uiArchiveVersion).Failed())
  {
    return XII_FAILURE;
  }

  // verify meta is valid
  if (tocMeta.m_uiTocOffsetFromArchiveEnd > uiArchiveEndingDataSize)
  {
    xiiLog::Error("Archive TOC offset is corrupted.");
    return XII_FAILURE;
  }

  // get toc data ptr
  const void* pTocStart = xiiMemoryUtils::AddByteOffset(pArchiveEndingDataBuffer, static_cast<ptrdiff_t>(uiArchiveEndingDataSize - tocMeta.m_uiTocOffsetFromArchiveEnd));

  // validate the TOC hash
  if (uiArchiveVersion >= 2)
  {
    const xiiUInt64 uiActualTocHash = xiiHashingUtils::xxHash64(pTocStart, tocMeta.m_uiTocSize);
    if (tocMeta.m_uiExpectedTocHash != uiActualTocHash)
    {
      xiiLog::Error("Archive TOC is corrupted. Hashes do not match.");
      return XII_FAILURE;
    }
  }

  // read the actual TOC data
  {
    xiiRawMemoryStreamReader tocReader(pTocStart, tocMeta.m_uiTocSize);

    if (ref_toc.Deserialize(tocReader, uiArchiveVersion).Failed())
    {
      xiiLog::Error("Failed to deserialize xiiArchive TOC");
      return XII_FAILURE;
    }
  }

  return XII_SUCCESS;
}

xiiResult xiiArchiveUtils::ExtractTOC(const xiiMemoryMappedFile& memFile, xiiArchiveTOC& ref_toc, xiiUInt8 uiArchiveVersion)
{
  return ExtractTOC(memFile.GetFileSize(), memFile.GetReadPointer(), ref_toc, uiArchiveVersion);
}

XII_STATICLINK_FILE(Foundation, Foundation_IO_Archive_Implementation_ArchiveUtils);

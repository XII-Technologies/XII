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

xiiResult xiiArchiveUtils::WriteEntry(xiiStreamWriter& ref_stream, xiiStringView sAbsSourcePath, xiiUInt32 uiPathStringOffset, xiiArchiveCompressionMode compression, xiiInt32 iCompressionLevel, xiiArchiveEntry& ref_tocEntry, xiiUInt64& inout_uiCurrentStreamPosition, FileWriteProgressCallback progress /*= FileWriteProgressCallback()*/)
{
  xiiFileReader file;
  XII_SUCCEED_OR_RETURN(file.Open(sAbsSourcePath, 1024 * 1024));

  const xiiUInt64 uiMaxBytes = file.GetFileSize();

  xiiUInt8 uiTemp[1024 * 8];

  ref_tocEntry.m_uiPathStringOffset     = uiPathStringOffset;
  ref_tocEntry.m_uiDataStartOffset      = inout_uiCurrentStreamPosition;
  ref_tocEntry.m_uiUncompressedDataSize = 0;

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
      constexpr xiiUInt32 uiMaxNumWorkerThreads = 12u;
      zstdWriter.SetOutputStream(&ref_stream, uiMaxNumWorkerThreads, (xiiCompressedStreamWriterZstd::Compression)iCompressionLevel);
      pWriter = &zstdWriter;
    }
    break;
#endif

    default:
      compression = xiiArchiveCompressionMode::Uncompressed;
      break;
  }

  ref_tocEntry.m_CompressionMode = compression;

  xiiUInt64 uiRead = 0;
  while (true)
  {
    uiRead = file.ReadBytes(uiTemp, XII_ARRAY_SIZE(uiTemp));

    if (uiRead == 0)
      break;

    ref_tocEntry.m_uiUncompressedDataSize += uiRead;

    if (progress.IsValid())
    {
      if (!progress(ref_tocEntry.m_uiUncompressedDataSize, uiMaxBytes))
        return XII_FAILURE;
    }

    XII_SUCCEED_OR_RETURN(pWriter->WriteBytes(uiTemp, uiRead));
  }

  switch (compression)
  {
#ifdef BUILDSYSTEM_ENABLE_ZSTD_SUPPORT
    case xiiArchiveCompressionMode::Compressed_zstd:
    {
      XII_SUCCEED_OR_RETURN(zstdWriter.FinishCompressedStream());
      ref_tocEntry.m_uiStoredDataSize = zstdWriter.GetWrittenBytes();
    }
    break;
#endif

    case xiiArchiveCompressionMode::Uncompressed:
    default:
      ref_tocEntry.m_uiStoredDataSize = ref_tocEntry.m_uiUncompressedDataSize;
      break;
  }

  inout_uiCurrentStreamPosition += ref_tocEntry.m_uiStoredDataSize;

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

static xiiResult VerifyEndMarker(xiiMemoryMappedFile& ref_memFile, xiiUInt8 uiArchiveVersion)
{
  const xiiUInt32 uiEndMarkerSize = GetEndMarkerSize(uiArchiveVersion);

  if (uiEndMarkerSize == 0)
    return XII_SUCCESS;

  const void* pStart = ref_memFile.GetReadPointer(uiEndMarkerSize, xiiMemoryMappedFile::OffsetBase::End);

  xiiRawMemoryStreamReader reader(pStart, uiEndMarkerSize);

  char szMarker[32] = "";
  if (reader.ReadBytes(szMarker, uiEndMarkerSize) != uiEndMarkerSize || !xiiStringUtils::IsEqual(szMarker, szEndMarker))
  {
    xiiLog::Error("Archive is corrupt or cut off. End-marker not found.");
    return XII_FAILURE;
  }

  return XII_SUCCESS;
}

xiiResult xiiArchiveUtils::ExtractTOC(xiiMemoryMappedFile& ref_memFile, xiiArchiveTOC& ref_toc, xiiUInt8 uiArchiveVersion)
{
  XII_SUCCEED_OR_RETURN(VerifyEndMarker(ref_memFile, uiArchiveVersion));

  const xiiUInt32 uiEndMarkerSize = GetEndMarkerSize(uiArchiveVersion);
  const xiiUInt32 uiTocMetaSize   = GetTocMetaSize(uiArchiveVersion);

  xiiUInt32 uiTocSize         = 0;
  xiiUInt64 uiExpectedTocHash = 0;

  // read the TOC meta data
  {
    const void* pTocMetaStart = ref_memFile.GetReadPointer(uiEndMarkerSize + uiTocMetaSize, xiiMemoryMappedFile::OffsetBase::End);

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

  const void* pTocStart = ref_memFile.GetReadPointer(uiTocSize + uiTocMetaSize + uiEndMarkerSize, xiiMemoryMappedFile::OffsetBase::End);

  // validate the TOC hash
  if (uiArchiveVersion >= 2)
  {
    const xiiUInt64 uiActualTocHash = xiiHashingUtils::xxHash64(pTocStart, uiTocSize);
    if (uiExpectedTocHash != uiActualTocHash)
    {
      xiiLog::Error("Archive TOC is corrupted. Hashes do not match.");
      return XII_FAILURE;
    }
  }

  // read the actual TOC data
  {
    xiiRawMemoryStreamReader tocReader(pTocStart, uiTocSize);

    if (ref_toc.Deserialize(tocReader, uiArchiveVersion).Failed())
    {
      xiiLog::Error("Failed to deserialize xiiArchive TOC");
      return XII_FAILURE;
    }
  }

  return XII_SUCCESS;
}

namespace ZipFormat
{
  constexpr xiiUInt32 EndOfCDMagicSignature = 0x06054b50;
  constexpr xiiUInt32 EndOfCDHeaderLength   = 22;

  constexpr xiiUInt32 MaxCommentLength       = 65535;
  constexpr xiiUInt64 MaxEndOfCDSearchLength = MaxCommentLength + EndOfCDHeaderLength;

  constexpr xiiUInt32 LocalFileMagicSignature = 0x04034b50;
  constexpr xiiUInt32 LocalFileHeaderLength   = 30;

  constexpr xiiUInt32 CDFileMagicSignature = 0x02014b50;
  constexpr xiiUInt32 CDFileHeaderLength   = 46;

  enum CompressionType
  {
    Uncompressed = 0,
    Deflate      = 8,

  };

  struct EndOfCDHeader
  {
    xiiUInt32 signature;
    xiiUInt16 diskNumber;
    xiiUInt16 diskWithCD;
    xiiUInt16 diskEntries;
    xiiUInt16 totalEntries;
    xiiUInt32 cdSize;
    xiiUInt32 cdOffset;
    xiiUInt16 commentLength;
  };

  xiiStreamReader& operator>>(xiiStreamReader& ref_stream, EndOfCDHeader& ref_value)
  {
    ref_stream >> ref_value.signature >> ref_value.diskNumber >> ref_value.diskWithCD >> ref_value.diskEntries >> ref_value.totalEntries >> ref_value.cdSize;
    ref_stream >> ref_value.cdOffset >> ref_value.commentLength;
    XII_ASSERT_DEBUG(ref_value.signature == EndOfCDMagicSignature, "ZIP: Corrupt end of central directory header.");
    return ref_stream;
  }

  struct CDFileHeader
  {
    xiiUInt32 signature;
    xiiUInt16 version;
    xiiUInt16 versionNeeded;
    xiiUInt16 flags;
    xiiUInt16 compression;
    xiiUInt16 modTime;
    xiiUInt16 modDate;
    xiiUInt32 crc32;
    xiiUInt32 compressedSize;
    xiiUInt32 uncompressedSize;
    xiiUInt16 fileNameLength;
    xiiUInt16 extraFieldLength;
    xiiUInt16 fileCommentLength;
    xiiUInt16 diskNumStart;
    xiiUInt16 internalAttr;
    xiiUInt32 externalAttr;
    xiiUInt32 offsetLocalHeader;
  };

  xiiStreamReader& operator>>(xiiStreamReader& ref_stream, CDFileHeader& ref_value)
  {
    ref_stream >> ref_value.signature >> ref_value.version >> ref_value.versionNeeded >> ref_value.flags >> ref_value.compression >> ref_value.modTime >> ref_value.modDate;
    ref_stream >> ref_value.crc32 >> ref_value.compressedSize >> ref_value.uncompressedSize >> ref_value.fileNameLength >> ref_value.extraFieldLength;
    ref_stream >> ref_value.fileCommentLength >> ref_value.diskNumStart >> ref_value.internalAttr >> ref_value.externalAttr >> ref_value.offsetLocalHeader;
    XII_ASSERT_DEBUG(ref_value.signature == CDFileMagicSignature, "ZIP: Corrupt central directory file entry header.");
    return ref_stream;
  }

  struct LocalFileHeader
  {
    xiiUInt32 signature;
    xiiUInt16 version;
    xiiUInt16 flags;
    xiiUInt16 compression;
    xiiUInt16 modTime;
    xiiUInt16 modDate;
    xiiUInt32 crc32;
    xiiUInt32 compressedSize;
    xiiUInt32 uncompressedSize;
    xiiUInt16 fileNameLength;
    xiiUInt16 extraFieldLength;
  };

  xiiStreamReader& operator>>(xiiStreamReader& ref_stream, LocalFileHeader& ref_value)
  {
    ref_stream >> ref_value.signature >> ref_value.version >> ref_value.flags >> ref_value.compression >> ref_value.modTime >> ref_value.modDate >> ref_value.crc32;
    ref_stream >> ref_value.compressedSize >> ref_value.uncompressedSize >> ref_value.fileNameLength >> ref_value.extraFieldLength;
    XII_ASSERT_DEBUG(ref_value.signature == LocalFileMagicSignature, "ZIP: Corrupt local file entry header.");
    return ref_stream;
  }
}; // namespace ZipFormat

xiiResult xiiArchiveUtils::ReadZipHeader(xiiStreamReader& ref_stream, xiiUInt8& out_uiVersion)
{
  using namespace ZipFormat;

  xiiUInt32 header;
  ref_stream >> header;
  if (header == LocalFileMagicSignature)
  {
    out_uiVersion = 0;
    return XII_SUCCESS;
  }
  return XII_SUCCESS;
}

xiiResult xiiArchiveUtils::ExtractZipTOC(xiiMemoryMappedFile& ref_memFile, xiiArchiveTOC& ref_toc)
{
  using namespace ZipFormat;

  const xiiUInt8* pEndOfCDStart = nullptr;
  {
    // Find End of CD signature by searching from the end of the file.
    // As a comment can come after it we have to potentially walk max comment length backwards.
    const xiiUInt64 SearchEnd    = ref_memFile.GetFileSize() - xiiMath::Min(MaxEndOfCDSearchLength, ref_memFile.GetFileSize());
    const xiiUInt8* pSearchEnd   = static_cast<const xiiUInt8*>(ref_memFile.GetReadPointer(SearchEnd, xiiMemoryMappedFile::OffsetBase::End));
    const xiiUInt8* pSearchStart = static_cast<const xiiUInt8*>(ref_memFile.GetReadPointer(EndOfCDHeaderLength, xiiMemoryMappedFile::OffsetBase::End));
    while (pSearchStart >= pSearchEnd)
    {
      if (*reinterpret_cast<const xiiUInt32*>(pSearchStart) == EndOfCDMagicSignature)
      {
        pEndOfCDStart = pSearchStart;
        break;
      }
      pSearchStart--;
    }
    if (pEndOfCDStart == nullptr)
      return XII_FAILURE;
  }

  xiiRawMemoryStreamReader tocReader(pEndOfCDStart, EndOfCDHeaderLength);
  EndOfCDHeader            ecdHeader;
  tocReader >> ecdHeader;

  ref_toc.m_Entries.Reserve(ecdHeader.diskEntries);
  ref_toc.m_PathToEntryIndex.Reserve(ecdHeader.diskEntries);

  xiiStringBuilder sLowerCaseHash;
  xiiUInt64        uiEntryOffset = 0;
  for (xiiUInt16 uiEntry = 0; uiEntry < ecdHeader.diskEntries; ++uiEntry)
  {
    // First, read the current file's header from the central directory
    const void*              pCdfStart = ref_memFile.GetReadPointer(ecdHeader.cdOffset + uiEntryOffset, xiiMemoryMappedFile::OffsetBase::Start);
    xiiRawMemoryStreamReader cdfReader(pCdfStart, ecdHeader.cdSize - uiEntryOffset);
    CDFileHeader             cdfHeader;
    cdfReader >> cdfHeader;

    if (cdfHeader.compression == CompressionType::Uncompressed || cdfHeader.compression == CompressionType::Deflate)
    {
      auto& entry                    = ref_toc.m_Entries.ExpandAndGetRef();
      entry.m_uiUncompressedDataSize = cdfHeader.uncompressedSize;
      entry.m_uiStoredDataSize       = cdfHeader.compressedSize;
      entry.m_uiPathStringOffset     = ref_toc.m_AllPathStrings.GetCount();
      entry.m_CompressionMode        = cdfHeader.compression == CompressionType::Uncompressed ? xiiArchiveCompressionMode::Uncompressed : xiiArchiveCompressionMode::Compressed_zip;

      auto nameBuffer = xiiArrayPtr<const xiiUInt8>(static_cast<const xiiUInt8*>(pCdfStart) + CDFileHeaderLength, cdfHeader.fileNameLength);
      ref_toc.m_AllPathStrings.PushBackRange(nameBuffer);
      ref_toc.m_AllPathStrings.PushBack(0);
      const char* szName = reinterpret_cast<const char*>(ref_toc.m_AllPathStrings.GetData() + entry.m_uiPathStringOffset);
      sLowerCaseHash     = szName;
      sLowerCaseHash.ToLower();
      ref_toc.m_PathToEntryIndex.Insert(xiiArchiveStoredString(xiiHashingUtils::StringHash(sLowerCaseHash), entry.m_uiPathStringOffset), ref_toc.m_Entries.GetCount() - 1);

      // Compute data stream start location. We need to skip past the local (and redundant) file header to find it.
      const void*              pLfStart = ref_memFile.GetReadPointer(cdfHeader.offsetLocalHeader, xiiMemoryMappedFile::OffsetBase::Start);
      xiiRawMemoryStreamReader lfReader(pLfStart, ref_memFile.GetFileSize() - cdfHeader.offsetLocalHeader);
      LocalFileHeader          lfHeader;
      lfReader >> lfHeader;
      entry.m_uiDataStartOffset = cdfHeader.offsetLocalHeader + LocalFileHeaderLength + lfHeader.fileNameLength + lfHeader.extraFieldLength;
    }
    // Compute next file header location.
    uiEntryOffset += CDFileHeaderLength + cdfHeader.fileNameLength + cdfHeader.extraFieldLength + cdfHeader.fileCommentLength;
  }

  return XII_SUCCESS;
}

XII_STATICLINK_FILE(Foundation, Foundation_IO_Archive_Implementation_ArchiveUtils);

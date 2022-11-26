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

bool xiiArchiveUtils::IsAcceptedArchiveFileExtensions(xiiStringView extension)
{
  for (const auto& ext : GetAcceptedArchiveFileExtensions())
  {
    if (extension.IsEqual_NoCase(ext.GetView()))
      return true;
  }

  return false;
}

xiiResult xiiArchiveUtils::WriteHeader(xiiStreamWriter& stream)
{
  const char* szTag = "XIIARCHIVE";
  XII_SUCCEED_OR_RETURN(stream.WriteBytes(szTag, 11));

  const xiiUInt8 uiArchiveVersion = 4;

  // Version 2: Added end-of-file marker for file corruption (cutoff) detection
  // Version 3: HashedStrings changed from MurmurHash to xxHash
  // Version 4: use 64 Bit string hashes
  stream << uiArchiveVersion;

  const xiiUInt8 uiPadding[5] = {0, 0, 0, 0, 0};
  XII_SUCCEED_OR_RETURN(stream.WriteBytes(uiPadding, 5));

  return XII_SUCCESS;
}

xiiResult xiiArchiveUtils::ReadHeader(xiiStreamReader& stream, xiiUInt8& out_uiVersion)
{
  char szTag[11];
  if (stream.ReadBytes(szTag, 11) != 11 || !xiiStringUtils::IsEqual(szTag, "XIIARCHIVE"))
  {
    xiiLog::Error("Invalid or corrupted archive. Archive-marker not found.");
    return XII_FAILURE;
  }

  out_uiVersion = 0;
  stream >> out_uiVersion;

  if (out_uiVersion != 1 && out_uiVersion != 2 && out_uiVersion != 3 && out_uiVersion != 4)
  {
    xiiLog::Error("Unsupported archive version '{}'.", out_uiVersion);
    return XII_FAILURE;
  }

  xiiUInt8 uiPadding[5] = {255, 255, 255, 255, 255};
  if (stream.ReadBytes(uiPadding, 5) != 5)
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

xiiResult xiiArchiveUtils::WriteEntry(xiiStreamWriter& stream, const char* szAbsSourcePath, xiiUInt32 uiPathStringOffset, xiiArchiveCompressionMode compression, xiiArchiveEntry& tocEntry, xiiUInt64& inout_uiCurrentStreamPosition, FileWriteProgressCallback progress /*= FileWriteProgressCallback()*/)
{
  xiiFileReader file;
  XII_SUCCEED_OR_RETURN(file.Open(szAbsSourcePath, 1024 * 1024));

  const xiiUInt64 uiMaxBytes = file.GetFileSize();

  xiiUInt8 uiTemp[1024 * 8];

  tocEntry.m_uiPathStringOffset     = uiPathStringOffset;
  tocEntry.m_uiDataStartOffset      = inout_uiCurrentStreamPosition;
  tocEntry.m_uiUncompressedDataSize = 0;

  xiiStreamWriter* pWriter = &stream;

#ifdef BUILDSYSTEM_ENABLE_ZSTD_SUPPORT
  xiiCompressedStreamWriterZstd zstdWriter;
#endif

  switch (compression)
  {
    case xiiArchiveCompressionMode::Uncompressed:
      break;

    case xiiArchiveCompressionMode::Compressed_zstd:
#ifdef BUILDSYSTEM_ENABLE_ZSTD_SUPPORT
      zstdWriter.SetOutputStream(&stream);
      pWriter = &zstdWriter;
#else
      compression = xiiArchiveCompressionMode::Uncompressed;
#endif
      break;

    default:
      XII_ASSERT_NOT_IMPLEMENTED;
  }

  tocEntry.m_CompressionMode = compression;

  xiiUInt64 uiRead = 0;
  while (true)
  {
    uiRead = file.ReadBytes(uiTemp, XII_ARRAY_SIZE(uiTemp));

    if (uiRead == 0)
      break;

    tocEntry.m_uiUncompressedDataSize += uiRead;

    if (progress.IsValid())
    {
      if (!progress(tocEntry.m_uiUncompressedDataSize, uiMaxBytes))
        return XII_FAILURE;
    }

    XII_SUCCEED_OR_RETURN(pWriter->WriteBytes(uiTemp, uiRead));
  }


  switch (compression)
  {
#ifdef BUILDSYSTEM_ENABLE_ZSTD_SUPPORT
    case xiiArchiveCompressionMode::Compressed_zstd:
      XII_SUCCEED_OR_RETURN(zstdWriter.FinishCompressedStream());
      tocEntry.m_uiStoredDataSize = zstdWriter.GetWrittenBytes();
      break;
#endif

    case xiiArchiveCompressionMode::Uncompressed:
    default:
      tocEntry.m_uiStoredDataSize = tocEntry.m_uiUncompressedDataSize;
      break;
  }

  inout_uiCurrentStreamPosition += tocEntry.m_uiStoredDataSize;

  return XII_SUCCESS;
}

xiiResult xiiArchiveUtils::WriteEntryOptimal(xiiStreamWriter& stream, const char* szAbsSourcePath, xiiUInt32 uiPathStringOffset, xiiArchiveCompressionMode compression, xiiArchiveEntry& tocEntry, xiiUInt64& inout_uiCurrentStreamPosition, FileWriteProgressCallback progress /*= FileWriteProgressCallback()*/)
{
  if (compression == xiiArchiveCompressionMode::Uncompressed)
  {
    return WriteEntry(stream, szAbsSourcePath, uiPathStringOffset, xiiArchiveCompressionMode::Uncompressed, tocEntry, inout_uiCurrentStreamPosition, progress);
  }
  else
  {
    xiiDefaultMemoryStreamStorage storage;
    xiiMemoryStreamWriter         writer(&storage);

    xiiUInt64 streamPos = inout_uiCurrentStreamPosition;
    XII_SUCCEED_OR_RETURN(WriteEntry(writer, szAbsSourcePath, uiPathStringOffset, compression, tocEntry, streamPos, progress));

    if (tocEntry.m_uiStoredDataSize * 12 >= tocEntry.m_uiUncompressedDataSize * 10)
    {
      // less than 20% size saving -> go uncompressed
      return WriteEntry(stream, szAbsSourcePath, uiPathStringOffset, xiiArchiveCompressionMode::Uncompressed, tocEntry, inout_uiCurrentStreamPosition, progress);
    }
    else
    {
      auto res                      = storage.CopyToStream(stream);
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
      break;
    }

#ifdef BUILDSYSTEM_ENABLE_ZSTD_SUPPORT
    case xiiArchiveCompressionMode::Compressed_zstd:
    {
      reader                                              = XII_DEFAULT_NEW(xiiCompressedStreamReaderZstdWithSource);
      xiiCompressedStreamReaderZstdWithSource* pRawReader = static_cast<xiiCompressedStreamReaderZstdWithSource*>(reader.Borrow());
      ConfigureRawMemoryStreamReader(entry, pStartOfArchiveData, pRawReader->m_Source);
      pRawReader->SetInputStream(&pRawReader->m_Source);
      break;
    }
#endif

    default:
      XII_REPORT_FAILURE("Archive entry compression mode '{}' is not supported by xiiArchiveReader", (int)entry.m_CompressionMode);
      break;
  }

  return std::move(reader);
}

void xiiArchiveUtils::ConfigureRawMemoryStreamReader(const xiiArchiveEntry& entry, const void* pStartOfArchiveData, xiiRawMemoryStreamReader& memReader)
{
  memReader.Reset(xiiMemoryUtils::AddByteOffset(pStartOfArchiveData, static_cast<ptrdiff_t>(entry.m_uiDataStartOffset)), entry.m_uiStoredDataSize);
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

xiiResult xiiArchiveUtils::AppendTOC(xiiStreamWriter& stream, const xiiArchiveTOC& toc)
{
  xiiDefaultMemoryStreamStorage storage;
  xiiMemoryStreamWriter         writer(&storage);

  XII_SUCCEED_OR_RETURN(toc.Serialize(writer));

  XII_SUCCEED_OR_RETURN(storage.CopyToStream(stream));

  TocMetaData tocMeta;

  xiiHashStreamWriter64 hashStream(tocMeta.m_uiSize);
  XII_SUCCEED_OR_RETURN(storage.CopyToStream(hashStream));

  // Added in file version 2: hash of the TOC
  tocMeta.m_uiSize = storage.GetStorageSize32();
  tocMeta.m_uiHash = hashStream.GetHashValue();

  // append the TOC meta data
  stream << tocMeta.m_uiSize;
  stream << tocMeta.m_uiHash;

  // write an 'end' marker
  return stream.WriteBytes(szEndMarker, 15);
}

static xiiResult VerifyEndMarker(xiiMemoryMappedFile& memFile, xiiUInt8 uiArchiveVersion)
{
  const xiiUInt32 uiEndMarkerSize = GetEndMarkerSize(uiArchiveVersion);

  if (uiEndMarkerSize == 0)
    return XII_SUCCESS;

  const void* pStart = memFile.GetReadPointer(uiEndMarkerSize, xiiMemoryMappedFile::OffsetBase::End);

  xiiRawMemoryStreamReader reader(pStart, uiEndMarkerSize);

  char szMarker[32] = "";
  if (reader.ReadBytes(szMarker, uiEndMarkerSize) != uiEndMarkerSize || !xiiStringUtils::IsEqual(szMarker, szEndMarker))
  {
    xiiLog::Error("Archive is corrupt or cut off. End-marker not found.");
    return XII_FAILURE;
  }

  return XII_SUCCESS;
}

xiiResult xiiArchiveUtils::ExtractTOC(xiiMemoryMappedFile& memFile, xiiArchiveTOC& toc, xiiUInt8 uiArchiveVersion)
{
  XII_SUCCEED_OR_RETURN(VerifyEndMarker(memFile, uiArchiveVersion));

  const xiiUInt32 uiEndMarkerSize = GetEndMarkerSize(uiArchiveVersion);
  const xiiUInt32 uiTocMetaSize   = GetTocMetaSize(uiArchiveVersion);

  xiiUInt32 uiTocSize         = 0;
  xiiUInt64 uiExpectedTocHash = 0;

  // read the TOC meta data
  {
    const void* pTocMetaStart = memFile.GetReadPointer(uiEndMarkerSize + uiTocMetaSize, xiiMemoryMappedFile::OffsetBase::End);

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

  const void* pTocStart = memFile.GetReadPointer(uiTocSize + uiTocMetaSize + uiEndMarkerSize, xiiMemoryMappedFile::OffsetBase::End);

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

    if (toc.Deserialize(tocReader, uiArchiveVersion).Failed())
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

  xiiStreamReader& operator>>(xiiStreamReader& Stream, EndOfCDHeader& Value)
  {
    Stream >> Value.signature >> Value.diskNumber >> Value.diskWithCD >> Value.diskEntries >> Value.totalEntries >> Value.cdSize;
    Stream >> Value.cdOffset >> Value.commentLength;
    XII_ASSERT_DEBUG(Value.signature == EndOfCDMagicSignature, "ZIP: Corrupt end of central directory header.");
    return Stream;
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

  xiiStreamReader& operator>>(xiiStreamReader& Stream, CDFileHeader& Value)
  {
    Stream >> Value.signature >> Value.version >> Value.versionNeeded >> Value.flags >> Value.compression >> Value.modTime >> Value.modDate;
    Stream >> Value.crc32 >> Value.compressedSize >> Value.uncompressedSize >> Value.fileNameLength >> Value.extraFieldLength;
    Stream >> Value.fileCommentLength >> Value.diskNumStart >> Value.internalAttr >> Value.externalAttr >> Value.offsetLocalHeader;
    XII_ASSERT_DEBUG(Value.signature == CDFileMagicSignature, "ZIP: Corrupt central directory file entry header.");
    return Stream;
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

  xiiStreamReader& operator>>(xiiStreamReader& Stream, LocalFileHeader& Value)
  {
    Stream >> Value.signature >> Value.version >> Value.flags >> Value.compression >> Value.modTime >> Value.modDate >> Value.crc32;
    Stream >> Value.compressedSize >> Value.uncompressedSize >> Value.fileNameLength >> Value.extraFieldLength;
    XII_ASSERT_DEBUG(Value.signature == LocalFileMagicSignature, "ZIP: Corrupt local file entry header.");
    return Stream;
  }
}; // namespace ZipFormat

xiiResult xiiArchiveUtils::ReadZipHeader(xiiStreamReader& stream, xiiUInt8& out_uiVersion)
{
  using namespace ZipFormat;

  xiiUInt32 header;
  stream >> header;
  if (header == LocalFileMagicSignature)
  {
    out_uiVersion = 0;
    return XII_SUCCESS;
  }
  return XII_SUCCESS;
}

xiiResult xiiArchiveUtils::ExtractZipTOC(xiiMemoryMappedFile& memFile, xiiArchiveTOC& toc)
{
  using namespace ZipFormat;

  const xiiUInt8* pEndOfCDStart = nullptr;
  {
    // Find End of CD signature by searching from the end of the file.
    // As a comment can come after it we have to potentially walk max comment length backwards.
    const xiiUInt64 SearchEnd    = memFile.GetFileSize() - xiiMath::Min(MaxEndOfCDSearchLength, memFile.GetFileSize());
    const xiiUInt8* pSearchEnd   = static_cast<const xiiUInt8*>(memFile.GetReadPointer(SearchEnd, xiiMemoryMappedFile::OffsetBase::End));
    const xiiUInt8* pSearchStart = static_cast<const xiiUInt8*>(memFile.GetReadPointer(EndOfCDHeaderLength, xiiMemoryMappedFile::OffsetBase::End));
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

  toc.m_Entries.Reserve(ecdHeader.diskEntries);
  toc.m_PathToEntryIndex.Reserve(ecdHeader.diskEntries);

  xiiStringBuilder sLowerCaseHash;
  xiiUInt64        uiEntryOffset = 0;
  for (xiiUInt16 uiEntry = 0; uiEntry < ecdHeader.diskEntries; ++uiEntry)
  {
    // First, read the current file's header from the central directory
    const void*              pCdfStart = memFile.GetReadPointer(ecdHeader.cdOffset + uiEntryOffset, xiiMemoryMappedFile::OffsetBase::Start);
    xiiRawMemoryStreamReader cdfReader(pCdfStart, ecdHeader.cdSize - uiEntryOffset);
    CDFileHeader             cdfHeader;
    cdfReader >> cdfHeader;

    if (cdfHeader.compression == CompressionType::Uncompressed || cdfHeader.compression == CompressionType::Deflate)
    {
      auto& entry                    = toc.m_Entries.ExpandAndGetRef();
      entry.m_uiUncompressedDataSize = cdfHeader.uncompressedSize;
      entry.m_uiStoredDataSize       = cdfHeader.compressedSize;
      entry.m_uiPathStringOffset     = toc.m_AllPathStrings.GetCount();
      entry.m_CompressionMode        = cdfHeader.compression == CompressionType::Uncompressed ? xiiArchiveCompressionMode::Uncompressed : xiiArchiveCompressionMode::Compressed_zip;

      auto nameBuffer = xiiArrayPtr<const xiiUInt8>(static_cast<const xiiUInt8*>(pCdfStart) + CDFileHeaderLength, cdfHeader.fileNameLength);
      toc.m_AllPathStrings.PushBackRange(nameBuffer);
      toc.m_AllPathStrings.PushBack(0);
      const char* szName = reinterpret_cast<const char*>(toc.m_AllPathStrings.GetData() + entry.m_uiPathStringOffset);
      sLowerCaseHash     = szName;
      sLowerCaseHash.ToLower();
      toc.m_PathToEntryIndex.Insert(xiiArchiveStoredString(xiiHashingUtils::StringHash(sLowerCaseHash), entry.m_uiPathStringOffset), toc.m_Entries.GetCount() - 1);

      // Compute data stream start location. We need to skip past the local (and redundant) file header to find it.
      const void*              pLfStart = memFile.GetReadPointer(cdfHeader.offsetLocalHeader, xiiMemoryMappedFile::OffsetBase::Start);
      xiiRawMemoryStreamReader lfReader(pLfStart, memFile.GetFileSize() - cdfHeader.offsetLocalHeader);
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

/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/IO/Archive/Archive.h>
#include <Foundation/Types/Delegate.h>
#include <Foundation/Types/UniquePtr.h>

class xiiStreamReader;
class xiiStreamWriter;
class xiiMemoryMappedFile;
class xiiArchiveTOC;
class xiiArchiveEntry;
class xiiRawMemoryStreamReader;

/// Utilities for working with xiiArchive files
namespace xiiArchiveUtils
{
  using FileWriteProgressCallback                 = xiiDelegate<bool(xiiUInt64, xiiUInt64)>;
  constexpr xiiUInt32 ArchiveHeaderSize           = 17;
  constexpr xiiUInt32 ArchiveTOCMetaMaxFooterSize = 15 + 12; //< note that it's the MAX size, i.e. toc meta can be smaller

  struct TOCMeta
  {
    xiiUInt32 m_uiTocSize                 = 0;
    xiiUInt64 m_uiExpectedTocHash         = 0;
    xiiUInt32 m_uiTocOffsetFromArchiveEnd = 0;
  };

  /// Returns a modifiable array of file extensions that the engine considers to be valid xiiArchive file extensions.
  ///
  /// By default it always contains 'xiiArchive'.
  /// Add or overwrite the values, if you want custom file extensions to be handled as xiiArchives.
  XII_FOUNDATION_DLL xiiHybridArray<xiiString, 4, xiiStaticAllocatorWrapper>& GetAcceptedArchiveFileExtensions();

  /// Checks case insensitive, whether the given extension is in the list of GetAcceptedArchiveFileExtensions().
  XII_FOUNDATION_DLL bool IsAcceptedArchiveFileExtensions(xiiStringView sExtension);

  /// Writes the header that identifies the xiiArchive file and version to the stream
  XII_FOUNDATION_DLL xiiResult WriteHeader(xiiStreamWriter& ref_stream);

  /// Reads the xiiArchive header. Returns success and the version, if the stream is a valid xiiArchive file.
  XII_FOUNDATION_DLL xiiResult ReadHeader(xiiStreamReader& ref_stream, xiiUInt8& out_uiVersion);

  /// Writes the archive TOC to the stream. This must be the last thing in the stream, if ExtractTOC() is supposed to work.
  XII_FOUNDATION_DLL xiiResult AppendTOC(xiiStreamWriter& ref_stream, const xiiArchiveTOC& toc);

  /// Deserializes the TOC meta from archive ending. Assumes the TOC is the very last data in the file.
  XII_FOUNDATION_DLL xiiResult ExtractTOCMeta(xiiUInt64 uiArchiveEndingDataSize, const void* pArchiveEndingDataBuffer, TOCMeta& ref_tocMeta, xiiUInt8 uiArchiveVersion);

  /// Deserializes the TOC meta from the memory mapped file. Assumes the TOC is the very last data in the file.
  XII_FOUNDATION_DLL xiiResult ExtractTOCMeta(const xiiMemoryMappedFile& memFile, TOCMeta& ref_tocMeta, xiiUInt8 uiArchiveVersion);

  /// Deserializes the TOC from from archive ending. Assumes the TOC is the very last data in the file and reads it from the back.
  XII_FOUNDATION_DLL xiiResult ExtractTOC(xiiUInt64 uiArchiveEndingDataSize, const void* pArchiveEndingDataBuffer, xiiArchiveTOC& ref_toc, xiiUInt8 uiArchiveVersion);

  /// Deserializes the TOC from the memory mapped file. Assumes the TOC is the very last data in the file and reads it from the back.
  XII_FOUNDATION_DLL xiiResult ExtractTOC(const xiiMemoryMappedFile& memFile, xiiArchiveTOC& ref_toc, xiiUInt8 uiArchiveVersion);

  /// Writes a single file entry to a xiiArchive stream with the given compression level.
  ///
  /// Appends information to the TOC for finding the data in the stream. Reads and updates inout_uiCurrentStreamPosition with the data byte
  /// offset. The progress callback is executed for every couple of KB of data that were written.
  XII_FOUNDATION_DLL xiiResult WriteEntry(xiiStreamWriter& ref_stream, xiiStringView sAbsSourcePath, xiiUInt32 uiPathStringOffset, xiiArchiveCompressionMode compression, xiiInt32 iCompressionLevel, xiiArchiveEntry& ref_tocEntry, xiiUInt64& inout_uiCurrentStreamPosition, FileWriteProgressCallback progress = FileWriteProgressCallback());

  /// Writes a single file entry to a xiiArchive stream with the given compression level.
  ///
  /// Appends information to the TOC for finding the data in the stream. Reads and updates inout_uiCurrentStreamPosition with the data byte
  /// offset. Compression parameter indicate compression that the entry data already have applied.
  XII_FOUNDATION_DLL xiiResult WriteEntryPreprocessed(xiiStreamWriter& ref_stream, xiiConstByteArrayPtr entryData, xiiUInt32 uiPathStringOffset, xiiArchiveCompressionMode compression, xiiUInt32 uiUncompressedEntryDataSize, xiiArchiveEntry& ref_tocEntry, xiiUInt64& inout_uiCurrentStreamPosition);

  /// Similar to WriteEntry, but if compression is enabled, checks that compression makes enough of a difference.
  /// If compression does not reduce file size enough, the file is stored uncompressed instead.
  XII_FOUNDATION_DLL xiiResult WriteEntryOptimal(xiiStreamWriter& ref_stream, xiiStringView sAbsSourcePath, xiiUInt32 uiPathStringOffset, xiiArchiveCompressionMode compression, xiiInt32 iCompressionLevel, xiiArchiveEntry& ref_tocEntry, xiiUInt64& inout_uiCurrentStreamPosition, FileWriteProgressCallback progress = FileWriteProgressCallback());

  /// Configures \a memReader as a view into the data stored for \a entry in the archive file.
  ///
  /// The raw memory stream may be compressed or uncompressed. This only creates a view for the stored data, it does not interpret it.
  XII_FOUNDATION_DLL void ConfigureRawMemoryStreamReader(const xiiArchiveEntry& entry, const void* pStartOfArchiveData, xiiRawMemoryStreamReader& ref_memReader);

  /// Creates a new stream reader which allows to read the uncompressed data for the given archive entry.
  ///
  /// Under the hood it may create different types of stream readers to uncompress or decode the data.
  XII_FOUNDATION_DLL xiiUniquePtr<xiiStreamReader> CreateEntryReader(const xiiArchiveEntry& entry, const void* pStartOfArchiveData);

} // namespace xiiArchiveUtils

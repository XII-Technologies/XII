/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/IO/MemoryStream.h>

/// A file writer that caches all written data and only opens and writes to the output file when everything is finished.
/// Useful to ensure that only complete files are written, or nothing at all, in case of a crash.
class XII_FOUNDATION_DLL xiiDeferredFileWriter : public xiiStreamWriter
{
  XII_DISALLOW_COPY_AND_ASSIGN(xiiDeferredFileWriter);

public:
  xiiDeferredFileWriter();

  /// Upon destruction the file is closed and thus written, unless Discard was called before.
  ~xiiDeferredFileWriter() { Close().IgnoreResult(); }

  /// This must be configured before anything is written to the file.
  void SetOutput(xiiStringView sFileToWriteTo, bool bOnlyWriteIfDifferent = false); // [tested]

  virtual xiiResult WriteBytes(const void* pWriteBuffer, xiiUInt64 uiBytesToWrite) override; // [tested]

  /// Upon calling this the content is written to the file specified with SetOutput().
  /// The return value is XII_FAILURE if the file could not be opened or not completely written.
  xiiResult Close(bool* out_pWasWrittenTo = nullptr); // [tested]

  /// Calling this abandons the content and a later Close or destruction of the instance
  /// will no longer write anything to file.
  void Discard(); // [tested]

private:
  bool                          m_bOnlyWriteIfDifferent = false;
  bool                          m_bAlreadyClosed        = false;
  xiiString                     m_sOutputFile;
  xiiDefaultMemoryStreamStorage m_Storage;
  xiiMemoryStreamWriter         m_Writer;
};

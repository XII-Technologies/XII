/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/IO/Stream.h>
#include <Foundation/Strings/HashedString.h>

/// Simple class to handle asset file headers (the very first bytes in all transformed asset files)
class XII_FOUNDATION_DLL xiiAssetFileHeader
{
public:
  xiiAssetFileHeader();

  /// Reads the hash from file. If the file is outdated, the hash is set to 0xFFFFFFFFFFFFFFFF.
  xiiResult Read(xiiStreamReader& ref_stream);

  /// Writes the asset hash to file (plus a little version info)
  xiiResult Write(xiiStreamWriter& ref_stream) const;

  /// Checks whether the stored file contains the same hash.
  bool IsFileUpToDate(xiiUInt64 uiExpectedHash, xiiUInt16 uiVersion) const { return (m_uiHash == uiExpectedHash && m_uiVersion == uiVersion); }

  /// Returns the asset file hash
  xiiUInt64 GetFileHash() const { return m_uiHash; }

  /// Sets the asset file hash
  void SetFileHashAndVersion(xiiUInt64 uiHash, xiiUInt16 v)
  {
    m_uiHash    = uiHash;
    m_uiVersion = v;
  }

  /// Returns the asset type version
  xiiUInt16 GetFileVersion() const { return m_uiVersion; }

  /// Returns the generator which was used to produce the asset file
  const xiiHashedString& GetGenerator() { return m_sGenerator; }

  /// Allows to set the generator string
  void SetGenerator(xiiStringView sGenerator) { m_sGenerator.Assign(sGenerator); }

private:
  // initialize to a 'valid' hash
  // this may get stored, unless someone sets the hash
  xiiUInt64       m_uiHash    = 0;
  xiiUInt16       m_uiVersion = 0;
  xiiHashedString m_sGenerator;
};

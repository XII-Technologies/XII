#include <Core/CorePCH.h>

#include <Core/Assets/AssetFileHeader.h>
#include <Foundation/IO/MemoryStream.h>

static const char* g_szAssetTag = "xiiAsset";

xiiAssetFileHeader::xiiAssetFileHeader()
{
  // initialize to a 'valid' hash
  // this may get stored, unless someone sets the hash
  m_uiHash    = 0;
  m_uiVersion = 0;
}

enum xiiAssetFileHeaderVersion : xiiUInt8
{
  Version1 = 1,
  Version2,
  Version3,

  VersionCount,
  VersionCurrent = VersionCount - 1
};

xiiResult xiiAssetFileHeader::Write(xiiStreamWriter& ref_stream) const
{
  XII_ASSERT_DEBUG(m_uiHash != 0xFFFFFFFFFFFFFFFF, "Cannot write an invalid hash to file");

  // 9 Bytes for identification + version
  XII_SUCCEED_OR_RETURN(ref_stream.WriteBytes(g_szAssetTag, 8));

  const xiiUInt8 uiVersion = xiiAssetFileHeaderVersion::VersionCurrent;
  ref_stream << uiVersion;

  // 8 Bytes for the hash
  ref_stream << m_uiHash;
  // 2 for the type version
  ref_stream << m_uiVersion;

  ref_stream << m_sGenerator;
  return XII_SUCCESS;
}

xiiResult xiiAssetFileHeader::Read(xiiStreamReader& ref_stream)
{
  // initialize to 'invalid'
  m_uiHash    = 0xFFFFFFFFFFFFFFFF;
  m_uiVersion = 0;

  char szTag[9] = {0};
  if (ref_stream.ReadBytes(szTag, 8) < 8)
  {
    XII_REPORT_FAILURE("The stream does not contain a valid asset file header");
    return XII_FAILURE;
  }

  szTag[8] = '\0';

  // invalid asset file ... this is not going to end well
  XII_ASSERT_DEBUG(xiiStringUtils::IsEqual(szTag, g_szAssetTag), "The stream does not contain a valid asset file header");

  if (!xiiStringUtils::IsEqual(szTag, g_szAssetTag))
    return XII_FAILURE;

  xiiUInt8 uiVersion = 0;
  ref_stream >> uiVersion;

  xiiUInt64 uiHash = 0;
  ref_stream >> uiHash;

  // future version?
  XII_ASSERT_DEV(uiVersion <= xiiAssetFileHeaderVersion::VersionCurrent, "Unknown asset header version {0}", uiVersion);

  if (uiVersion >= xiiAssetFileHeaderVersion::Version2)
  {
    ref_stream >> m_uiVersion;
  }

  if (uiVersion >= xiiAssetFileHeaderVersion::Version3)
  {
    ref_stream >> m_sGenerator;
  }

  // older version? set the hash to 'invalid'
  if (uiVersion != xiiAssetFileHeaderVersion::VersionCurrent)
    return XII_FAILURE;

  m_uiHash = uiHash;

  return XII_SUCCESS;
}

XII_STATICLINK_FILE(Core, Core_Assets_Implementation_AssetFileHeader);

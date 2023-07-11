#include <Foundation/FoundationPCH.h>

#include <Foundation/IO/FileSystem/DeferredFileWriter.h>
#include <Foundation/IO/FileSystem/FileReader.h>

xiiDeferredFileWriter::xiiDeferredFileWriter() :
  m_Writer(&m_Storage)
{
}

void xiiDeferredFileWriter::SetOutput(xiiStringView sFileToWriteTo, bool bOnlyWriteIfDifferent)
{
  m_bOnlyWriteIfDifferent = bOnlyWriteIfDifferent;
  m_sOutputFile           = sFileToWriteTo;
}

xiiResult xiiDeferredFileWriter::WriteBytes(const void* pWriteBuffer, xiiUInt64 uiBytesToWrite)
{
  XII_ASSERT_DEBUG(!m_sOutputFile.IsEmpty(), "Output file has not been configured");

  return m_Writer.WriteBytes(pWriteBuffer, uiBytesToWrite);
}

xiiResult xiiDeferredFileWriter::Close(bool* out_pWasWrittenTo /*= nullptr*/)
{
  if (out_pWasWrittenTo)
  {
    *out_pWasWrittenTo = false;
  }

  if (m_bAlreadyClosed)
    return XII_SUCCESS;

  if (m_sOutputFile.IsEmpty())
    return XII_FAILURE;

  m_bAlreadyClosed = true;

  if (m_bOnlyWriteIfDifferent)
  {
    xiiFileReader fileIn;
    if (fileIn.Open(m_sOutputFile).Succeeded() && fileIn.GetFileSize() == m_Storage.GetStorageSize64())
    {
      xiiUInt8 tmp1[1024 * 4];
      xiiUInt8 tmp2[1024 * 4];

      xiiMemoryStreamReader storageReader(&m_Storage);

      while (true)
      {
        const xiiUInt64 readBytes1 = fileIn.ReadBytes(tmp1, XII_ARRAY_SIZE(tmp1));
        const xiiUInt64 readBytes2 = storageReader.ReadBytes(tmp2, XII_ARRAY_SIZE(tmp2));

        if (readBytes1 != readBytes2)
          goto write_data;

        if (readBytes1 == 0)
          break;

        if (xiiMemoryUtils::RawByteCompare(tmp1, tmp2, xiiMath::SafeConvertToSizeT(readBytes1)) != 0)
          goto write_data;
      }

      // content is already the same as what we would write -> skip the write (do not modify file write date)
      return XII_SUCCESS;
    }
  }

write_data:
  xiiFileWriter file;
  XII_SUCCEED_OR_RETURN(file.Open(m_sOutputFile, 0)); // use the minimum cache size, we want to pass data directly through to disk

  if (out_pWasWrittenTo)
  {
    *out_pWasWrittenTo = false;
  }

  m_sOutputFile.Clear();
  return m_Storage.CopyToStream(file);
}

void xiiDeferredFileWriter::Discard()
{
  m_sOutputFile.Clear();
}

XII_STATICLINK_FILE(Foundation, Foundation_IO_FileSystem_Implementation_DeferredFileWriter);

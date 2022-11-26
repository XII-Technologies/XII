#include <Foundation/IO/MemoryMappedFile.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Strings/PathUtils.h>

///#TODO: Implement. Under some restrictions, UWP supports
/// CreateFileMappingFromApp, OpenFileMappingFromApp, MapViewOfFileFromApp
/// Needs adding codeGeneration capability to the app manifest.

struct xiiMemoryMappedFileImpl
{
  xiiMemoryMappedFile::Mode m_Mode           = xiiMemoryMappedFile::Mode::None;
  void*                     m_pMappedFilePtr = nullptr;
  xiiUInt64                 m_uiFileSize     = 0;

  ~xiiMemoryMappedFileImpl() {}
};

xiiMemoryMappedFile::xiiMemoryMappedFile()
{
  m_pImpl = XII_DEFAULT_NEW(xiiMemoryMappedFileImpl);
}

xiiMemoryMappedFile::~xiiMemoryMappedFile()
{
  Close();
}

void xiiMemoryMappedFile::Close()
{
  m_pImpl = XII_DEFAULT_NEW(xiiMemoryMappedFileImpl);
}

xiiMemoryMappedFile::Mode xiiMemoryMappedFile::GetMode() const
{
  return m_pImpl->m_Mode;
}

const void* xiiMemoryMappedFile::GetReadPointer(xiiUInt64 uiOffset /*= 0*/, OffsetBase base /*= OffsetBase::Start*/) const
{
  XII_ASSERT_DEBUG(m_pImpl->m_Mode >= Mode::ReadOnly, "File must be opened with read access before accessing it for reading.");
  return m_pImpl->m_pMappedFilePtr;
}

void* xiiMemoryMappedFile::GetWritePointer(xiiUInt64 uiOffset /*= 0*/, OffsetBase base /*= OffsetBase::Start*/)
{
  XII_ASSERT_DEBUG(m_pImpl->m_Mode >= Mode::ReadWrite, "File must be opened with read/write access before accessing it for writing.");
  return m_pImpl->m_pMappedFilePtr;
}

xiiUInt64 xiiMemoryMappedFile::GetFileSize() const
{
  return m_pImpl->m_uiFileSize;
}

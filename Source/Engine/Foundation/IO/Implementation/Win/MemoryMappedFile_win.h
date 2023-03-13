#include <Foundation/IO/Implementation/Win/DosDevicePath_win.h>
#include <Foundation/IO/MemoryMappedFile.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Strings/PathUtils.h>
#include <Foundation/Strings/StringConversion.h>

struct xiiMemoryMappedFileImpl
{
  xiiMemoryMappedFile::Mode m_Mode           = xiiMemoryMappedFile::Mode::None;
  void*                     m_pMappedFilePtr = nullptr;
  xiiUInt64                 m_uiFileSize     = 0;
  HANDLE                    m_hFile          = INVALID_HANDLE_VALUE;
  HANDLE                    m_hMapping       = INVALID_HANDLE_VALUE;

  ~xiiMemoryMappedFileImpl()
  {
    if (m_pMappedFilePtr != nullptr)
    {
      UnmapViewOfFile(m_pMappedFilePtr);
      m_pMappedFilePtr = nullptr;
    }

    if (m_hMapping != INVALID_HANDLE_VALUE)
    {
      CloseHandle(m_hMapping);
      m_hMapping = INVALID_HANDLE_VALUE;
    }

    if (m_hFile != INVALID_HANDLE_VALUE)
    {
      CloseHandle(m_hFile);
      m_hFile = INVALID_HANDLE_VALUE;
    }
  }
};

xiiMemoryMappedFile::xiiMemoryMappedFile()
{
  m_pImpl = XII_DEFAULT_NEW(xiiMemoryMappedFileImpl);
}

xiiMemoryMappedFile::~xiiMemoryMappedFile()
{
  Close();
}

xiiResult xiiMemoryMappedFile::Open(xiiStringView sAbsolutePath, Mode mode)
{
  XII_ASSERT_DEV(mode != Mode::None, "Invalid mode to open the memory mapped file");
  XII_ASSERT_DEV(xiiPathUtils::IsAbsolutePath(sAbsolutePath), "xiiMemoryMappedFile::Open() can only be used with absolute file paths");

  XII_LOG_BLOCK("MemoryMapFile", sAbsolutePath);


  Close();

  m_pImpl->m_Mode = mode;

  DWORD access = GENERIC_READ;

  if (mode == Mode::ReadWrite)
  {
    access |= GENERIC_WRITE;
  }

  m_pImpl->m_hFile = CreateFileW(xiiDosDevicePath(sAbsolutePath), access, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);

  DWORD errorCode = GetLastError();

  if (m_pImpl->m_hFile == nullptr || m_pImpl->m_hFile == INVALID_HANDLE_VALUE)
  {
    xiiLog::Error("Could not open file for memory mapping - {}", xiiArgErrorCode(errorCode));
    Close();
    return XII_FAILURE;
  }

  if (GetFileSizeEx(m_pImpl->m_hFile, reinterpret_cast<LARGE_INTEGER*>(&m_pImpl->m_uiFileSize)) == FALSE || m_pImpl->m_uiFileSize == 0)
  {
    xiiLog::Error("File for memory mapping is empty");
    Close();
    return XII_FAILURE;
  }

  m_pImpl->m_hMapping = CreateFileMappingW(m_pImpl->m_hFile, nullptr, m_pImpl->m_Mode == Mode::ReadOnly ? PAGE_READONLY : PAGE_READWRITE, 0, 0, nullptr);

  if (m_pImpl->m_hMapping == nullptr || m_pImpl->m_hMapping == INVALID_HANDLE_VALUE)
  {
    errorCode = GetLastError();

    xiiLog::Error("Could not create memory mapping of file - {}", xiiArgErrorCode(errorCode));
    Close();
    return XII_FAILURE;
  }

  m_pImpl->m_pMappedFilePtr = MapViewOfFile(m_pImpl->m_hMapping, mode == Mode::ReadOnly ? FILE_MAP_READ : FILE_MAP_WRITE, 0, 0, 0);

  if (m_pImpl->m_pMappedFilePtr == nullptr)
  {
    errorCode = GetLastError();

    xiiLog::Error("Could not create memory mapping view of file - {}", xiiArgErrorCode(errorCode));
    Close();
    return XII_FAILURE;
  }

  return XII_SUCCESS;
}

xiiResult xiiMemoryMappedFile::OpenShared(xiiStringView sSharedName, xiiUInt64 uiSize, Mode mode)
{
  XII_ASSERT_DEV(mode != Mode::None, "Invalid mode to open the memory mapped file");
  XII_ASSERT_DEV(uiSize > 0, "xiiMemoryMappedFile::OpenShared() needs a valid file size to map");

  XII_LOG_BLOCK("MemoryMapFile", sSharedName);

  Close();

  m_pImpl->m_Mode = mode;

  DWORD errorCode = 0;
  DWORD sizeHigh  = static_cast<DWORD>((uiSize >> 32) & 0xFFFFFFFFu);
  DWORD sizeLow   = static_cast<DWORD>(uiSize & 0xFFFFFFFFu);

  m_pImpl->m_hMapping = CreateFileMappingW(INVALID_HANDLE_VALUE, nullptr, m_pImpl->m_Mode == Mode::ReadOnly ? PAGE_READONLY : PAGE_READWRITE, sizeHigh,
                                           sizeLow, xiiStringWChar(sSharedName).GetData());

  if (m_pImpl->m_hMapping == nullptr || m_pImpl->m_hMapping == INVALID_HANDLE_VALUE)
  {
    errorCode = GetLastError();

    xiiLog::Error("Could not create memory mapping of file - {}", xiiArgErrorCode(errorCode));
    Close();
    return XII_FAILURE;
  }

  m_pImpl->m_pMappedFilePtr = MapViewOfFile(m_pImpl->m_hMapping, mode == Mode::ReadOnly ? FILE_MAP_READ : FILE_MAP_WRITE, 0, 0, 0);

  if (m_pImpl->m_pMappedFilePtr == nullptr)
  {
    errorCode = GetLastError();

    xiiLog::Error("Could not create memory mapping view of file - {}", xiiArgErrorCode(errorCode));
    Close();
    return XII_FAILURE;
  }

  return XII_SUCCESS;
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
  XII_ASSERT_DEBUG(uiOffset <= m_pImpl->m_uiFileSize, "Read offset must be smaller than mapped file size");

  if (base == OffsetBase::Start)
  {
    return xiiMemoryUtils::AddByteOffset(m_pImpl->m_pMappedFilePtr, static_cast<ptrdiff_t>(uiOffset));
  }
  else
  {
    return xiiMemoryUtils::AddByteOffset(m_pImpl->m_pMappedFilePtr, static_cast<ptrdiff_t>(m_pImpl->m_uiFileSize - uiOffset));
  }
}

void* xiiMemoryMappedFile::GetWritePointer(xiiUInt64 uiOffset /*= 0*/, OffsetBase base /*= OffsetBase::Start*/)
{
  XII_ASSERT_DEBUG(m_pImpl->m_Mode >= Mode::ReadWrite, "File must be opened with read/write access before accessing it for writing.");
  XII_ASSERT_DEBUG(uiOffset <= m_pImpl->m_uiFileSize, "Read offset must be smaller than mapped file size");

  if (base == OffsetBase::Start)
  {
    return xiiMemoryUtils::AddByteOffset(m_pImpl->m_pMappedFilePtr, static_cast<ptrdiff_t>(uiOffset));
  }
  else
  {
    return xiiMemoryUtils::AddByteOffset(m_pImpl->m_pMappedFilePtr, static_cast<ptrdiff_t>(m_pImpl->m_uiFileSize - uiOffset));
  }
}

xiiUInt64 xiiMemoryMappedFile::GetFileSize() const
{
  return m_pImpl->m_uiFileSize;
}

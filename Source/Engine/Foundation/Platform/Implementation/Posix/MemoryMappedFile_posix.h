#include <Foundation/FoundationPCH.h>
XII_FOUNDATION_INTERNAL_HEADER

#include <Foundation/IO/MemoryMappedFile.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Strings/PathUtils.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>

#if XII_ENABLED(XII_PLATFORM_LINUX)
#  include <linux/version.h>
#endif

#if XII_ENABLED(XII_PLATFORM_OSX)
#  include <unistd.h>
#endif

struct xiiMemoryMappedFileImpl
{
  xiiMemoryMappedFile::Mode m_Mode           = xiiMemoryMappedFile::Mode::None;
  void*                     m_pMappedFilePtr = nullptr;
  xiiUInt64                 m_uiFileSize     = 0;
  xiiInt32                  m_hFile          = -1;
  xiiString                 m_sSharedMemoryName;

  ~xiiMemoryMappedFileImpl()
  {
#if XII_ENABLED(XII_SUPPORTS_MEMORY_MAPPED_FILE)
    if (m_pMappedFilePtr != nullptr)
    {
      munmap(m_pMappedFilePtr, m_uiFileSize);
      m_pMappedFilePtr = nullptr;
    }
    if (m_hFile != -1)
    {
      close(m_hFile);
      m_hFile = -1;
    }

    if (!m_sSharedMemoryName.IsEmpty())
    {
      shm_unlink(m_sSharedMemoryName);
      m_sSharedMemoryName.Clear();
    }

    m_uiFileSize = 0;
#endif
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

#if XII_ENABLED(XII_SUPPORTS_MEMORY_MAPPED_FILE)
xiiResult xiiMemoryMappedFile::Open(xiiStringView sAbsolutePath, Mode mode)
{
  XII_ASSERT_DEV(mode != Mode::None, "Invalid mode to open the memory mapped file");
  XII_ASSERT_DEV(xiiPathUtils::IsAbsolutePath(sAbsolutePath), "xiiMemoryMappedFile::Open() can only be used with absolute file paths");

  XII_LOG_BLOCK("MemoryMapFile", sAbsolutePath);

  const xiiStringBuilder sPath = sAbsolutePath;

  Close();

  m_pImpl->m_Mode = mode;

  xiiInt32 access = O_RDONLY;
  xiiInt32 prot   = PROT_READ;
  xiiInt32 flags  = MAP_PRIVATE;
  if (mode == Mode::ReadWrite)
  {
    access = O_RDWR;
    prot |= PROT_WRITE;
    flags = MAP_SHARED;
  }
#  if XII_ENABLED(XII_PLATFORM_LINUX)
#    if LINUX_VERSION_CODE > KERNEL_VERSION(2, 6, 22)
  flags |= MAP_POPULATE;
#    endif
#  endif
  m_pImpl->m_hFile = open(sPath, access | O_CLOEXEC, 0);
  if (m_pImpl->m_hFile == -1)
  {
    xiiLog::Error("Could not open file for memory mapping - {}", strerror(errno));
    Close();
    return XII_FAILURE;
  }
  struct stat sb;
  if (stat(sPath, &sb) == -1 || sb.st_size == 0)
  {
    xiiLog::Error("File for memory mapping is empty - {}", strerror(errno));
    Close();
    return XII_FAILURE;
  }
  m_pImpl->m_uiFileSize = sb.st_size;

  m_pImpl->m_pMappedFilePtr = mmap(nullptr, m_pImpl->m_uiFileSize, prot, flags, m_pImpl->m_hFile, 0);
  if (m_pImpl->m_pMappedFilePtr == nullptr)
  {
    xiiLog::Error("Could not create memory mapping of file - {}", strerror(errno));
    Close();
    return XII_FAILURE;
  }

  return XII_SUCCESS;
}
#endif

#if XII_ENABLED(XII_SUPPORTS_SHARED_MEMORY)
xiiResult xiiMemoryMappedFile::OpenShared(xiiStringView sSharedName, xiiUInt64 uiSize, Mode mode)
{
  XII_ASSERT_DEV(mode != Mode::None, "Invalid mode to open the memory mapped file");
  XII_ASSERT_DEV(uiSize > 0, "xiiMemoryMappedFile::OpenShared() needs a valid file size to map");

  const xiiStringBuilder sName = sSharedName;

  XII_LOG_BLOCK("MemoryMapFile", sName);

  Close();

  m_pImpl->m_Mode = mode;

  xiiInt32 prot  = PROT_READ;
  xiiInt32 oflag = O_RDONLY;
  xiiInt32 flags = MAP_SHARED;
#  if XII_ENABLED(XII_PLATFORM_LINUX)
#    if LINUX_VERSION_CODE > KERNEL_VERSION(2, 6, 22)
  flags |= MAP_POPULATE;
#    endif
#  endif

  if (mode == Mode::ReadWrite)
  {
    oflag = O_RDWR;
    prot |= PROT_WRITE;
  }
  oflag |= O_CREAT;

  m_pImpl->m_hFile = shm_open(sName, oflag, 0666);
  if (m_pImpl->m_hFile == -1)
  {
    xiiLog::Error("Could not open shared memory mapping - {}", strerror(errno));
    Close();
    return XII_FAILURE;
  }
  m_pImpl->m_sSharedMemoryName = sName;

  if (ftruncate(m_pImpl->m_hFile, uiSize) == -1)
  {
    xiiLog::Error("Could not open shared memory mapping - {}", strerror(errno));
    Close();
    return XII_FAILURE;
  }
  m_pImpl->m_uiFileSize = uiSize;

  m_pImpl->m_pMappedFilePtr = mmap(nullptr, m_pImpl->m_uiFileSize, prot, flags, m_pImpl->m_hFile, 0);
  if (m_pImpl->m_pMappedFilePtr == nullptr)
  {
    xiiLog::Error("Could not create memory mapping of file - {}", strerror(errno));
    Close();
    return XII_FAILURE;
  }
  return XII_SUCCESS;
}
#endif

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
    return xiiMemoryUtils::AddByteOffset(m_pImpl->m_pMappedFilePtr, uiOffset);
  }
  else
  {
    return xiiMemoryUtils::AddByteOffset(m_pImpl->m_pMappedFilePtr, m_pImpl->m_uiFileSize - uiOffset);
  }
}

void* xiiMemoryMappedFile::GetWritePointer(xiiUInt64 uiOffset /*= 0*/, OffsetBase base /*= OffsetBase::Start*/)
{
  XII_ASSERT_DEBUG(m_pImpl->m_Mode >= Mode::ReadWrite, "File must be opened with read/write access before accessing it for writing.");
  XII_ASSERT_DEBUG(uiOffset <= m_pImpl->m_uiFileSize, "Read offset must be smaller than mapped file size");

  if (base == OffsetBase::Start)
  {
    return xiiMemoryUtils::AddByteOffset(m_pImpl->m_pMappedFilePtr, uiOffset);
  }
  else
  {
    return xiiMemoryUtils::AddByteOffset(m_pImpl->m_pMappedFilePtr, m_pImpl->m_uiFileSize - uiOffset);
  }
}

xiiUInt64 xiiMemoryMappedFile::GetFileSize() const
{
  return m_pImpl->m_uiFileSize;
}

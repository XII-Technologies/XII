#include <Core/CorePCH.h>

#include <Core/ResourceManager/Resource.h>
#include <Core/ResourceManager/ResourceTypeLoader.h>
#include <Foundation/Containers/Blob.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/MemoryStream.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Profiling/Profiling.h>

struct FileResourceLoadData
{
  xiiBlob                  m_Storage;
  xiiRawMemoryStreamReader m_Reader;
};

xiiResourceLoadData xiiResourceLoaderFromFile::OpenDataStream(const xiiResource* pResource)
{
  XII_PROFILE_SCOPE("ReadResourceFile");

  xiiResourceLoadData res;

  xiiFileReader File;
  if (File.Open(pResource->GetResourceID().GetData()).Failed())
    return res;

  res.m_sResourceDescription = File.GetFilePathRelative().GetData();

#if XII_ENABLED(XII_SUPPORTS_FILE_STATS)
  xiiFileStats stat;
  if (xiiFileSystem::GetFileStats(pResource->GetResourceID(), stat).Succeeded())
  {
    res.m_LoadedFileModificationDate = stat.m_LastModificationTime;
  }

#endif

  FileResourceLoadData* pData = XII_DEFAULT_NEW(FileResourceLoadData);

  const xiiUInt64 uiFileSize = File.GetFileSize();

  const xiiUInt64 uiBlobCapacity = uiFileSize + File.GetFilePathAbsolute().GetElementCount() + 8; // +8 for the string overhead
  pData->m_Storage.SetCountUninitialized(uiBlobCapacity);

  xiiUInt8* pBlobPtr = pData->m_Storage.GetBlobPtr<xiiUInt8>().GetPtr();

  xiiRawMemoryStreamWriter w(pBlobPtr, uiBlobCapacity);

  // write the absolute path to the read file into the memory stream
  w << File.GetFilePathAbsolute();

  const xiiUInt64 uiOffset = w.GetNumWrittenBytes();

  File.ReadBytes(pBlobPtr + uiOffset, uiFileSize);

  pData->m_Reader.Reset(pBlobPtr, w.GetNumWrittenBytes() + uiFileSize);
  res.m_pDataStream       = &pData->m_Reader;
  res.m_pCustomLoaderData = pData;

  return res;
}

void xiiResourceLoaderFromFile::CloseDataStream(const xiiResource* pResource, const xiiResourceLoadData& LoaderData)
{
  FileResourceLoadData* pData = static_cast<FileResourceLoadData*>(LoaderData.m_pCustomLoaderData);

  XII_DEFAULT_DELETE(pData);
}

bool xiiResourceLoaderFromFile::IsResourceOutdated(const xiiResource* pResource) const
{
  // if we cannot find the target file, there is no point in trying to reload it -> claim it's up to date
  if (xiiFileSystem::ResolvePath(pResource->GetResourceID(), nullptr, nullptr).Failed())
    return false;

#if XII_ENABLED(XII_SUPPORTS_FILE_STATS)

  if (pResource->GetLoadedFileModificationTime().IsValid())
  {
    xiiFileStats stat;
    if (xiiFileSystem::GetFileStats(pResource->GetResourceID(), stat).Failed())
      return false;

    return !stat.m_LastModificationTime.Compare(pResource->GetLoadedFileModificationTime(), xiiTimestamp::CompareMode::FileTimeEqual);
  }

#endif

  return true;
}

//////////////////////////////////////////////////////////////////////////

xiiResourceLoadData xiiResourceLoaderFromMemory::OpenDataStream(const xiiResource* pResource)
{
  m_Reader.SetStorage(&m_CustomData);
  m_Reader.SetReadPosition(0);

  xiiResourceLoadData res;

  res.m_sResourceDescription       = m_sResourceDescription;
  res.m_LoadedFileModificationDate = m_ModificationTimestamp;
  res.m_pDataStream                = &m_Reader;
  res.m_pCustomLoaderData          = nullptr;

  return res;
}

void xiiResourceLoaderFromMemory::CloseDataStream(const xiiResource* pResource, const xiiResourceLoadData& LoaderData)
{
  m_Reader.SetStorage(nullptr);
}

bool xiiResourceLoaderFromMemory::IsResourceOutdated(const xiiResource* pResource) const
{
  if (pResource->GetLoadedFileModificationTime().IsValid() && m_ModificationTimestamp.IsValid())
  {
    if (!m_ModificationTimestamp.Compare(pResource->GetLoadedFileModificationTime(), xiiTimestamp::CompareMode::FileTimeEqual))
      return true;

    return false;
  }

  return true;
}



XII_STATICLINK_FILE(Core, Core_ResourceManager_Implementation_ResourceTypeLoader);

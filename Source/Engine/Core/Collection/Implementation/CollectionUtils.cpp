#include <Core/CorePCH.h>

#include <Core/Collection/CollectionUtils.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/IO/OSFile.h>

void xiiCollectionUtils::AddFiles(xiiCollectionResourceDescriptor& collection, const char* szAssetTypeName, const char* szAbsPathToFolder, const char* szFileExtension, const char* szStripPrefix, const char* szPrependPrefix)
{
#if XII_ENABLED(XII_SUPPORTS_FILE_ITERATORS)

  const xiiUInt32 uiStripPrefixLength = xiiStringUtils::GetCharacterCount(szStripPrefix);

  xiiFileSystemIterator fsIt;
  fsIt.StartSearch(szAbsPathToFolder, xiiFileSystemIteratorFlags::ReportFilesRecursive);

  if (!fsIt.IsValid())
    return;

  xiiStringBuilder sFullPath;
  xiiHashedString  sAssetTypeName;
  sAssetTypeName.Assign(szAssetTypeName);

  for (; fsIt.IsValid(); fsIt.Next())
  {
    const auto& stats = fsIt.GetStats();

    if (xiiPathUtils::HasExtension(stats.m_sName, szFileExtension))
    {
      stats.GetFullPath(sFullPath);

      sFullPath.Shrink(uiStripPrefixLength, 0);
      sFullPath.Prepend(szPrependPrefix);
      sFullPath.MakeCleanPath();

      auto& entry            = collection.m_Resources.ExpandAndGetRef();
      entry.m_sAssetTypeName = sAssetTypeName;
      entry.m_sResourceID    = sFullPath;
      entry.m_uiFileSize     = stats.m_uiFileSize;
    }
  }

#else
  XII_ASSERT_NOT_IMPLEMENTED;
#endif
}


XII_CORE_DLL void xiiCollectionUtils::MergeCollections(xiiCollectionResourceDescriptor& result, xiiArrayPtr<const xiiCollectionResourceDescriptor*> inputCollections)
{
  xiiMap<xiiString, const xiiCollectionEntry*> firstEntryOfID;

  for (const xiiCollectionResourceDescriptor* inputDesc : inputCollections)
  {
    for (const xiiCollectionEntry& inputEntry : inputDesc->m_Resources)
    {
      if (!firstEntryOfID.Contains(inputEntry.m_sResourceID))
      {
        firstEntryOfID.Insert(inputEntry.m_sResourceID, &inputEntry);
        result.m_Resources.PushBack(inputEntry);
      }
    }
  }
}


XII_CORE_DLL void xiiCollectionUtils::DeDuplicateEntries(xiiCollectionResourceDescriptor& result, const xiiCollectionResourceDescriptor& input)
{
  const xiiCollectionResourceDescriptor* firstInput = &input;
  MergeCollections(result, xiiArrayPtr<const xiiCollectionResourceDescriptor*>(&firstInput, 1));
}

void xiiCollectionUtils::AddResourceHandle(xiiCollectionResourceDescriptor& collection, xiiTypelessResourceHandle handle, const char* szAssetTypeName, const char* szAbsFolderpath)
{
  if (!handle.IsValid())
    return;

  const char* resID = handle.GetResourceID();

  auto& entry = collection.m_Resources.ExpandAndGetRef();

  entry.m_sAssetTypeName.Assign(szAssetTypeName);
  entry.m_sResourceID = resID;

  xiiStringBuilder absFilename;

  // if a folder path is specified, replace the root (for testing filesize below)
  if (szAbsFolderpath != nullptr)
  {
    xiiStringView root, relFile;
    xiiPathUtils::GetRootedPathParts(resID, root, relFile);
    absFilename = szAbsFolderpath;
    absFilename.AppendPath(relFile.GetStartPointer());
    absFilename.MakeCleanPath();

    xiiFileStats stats;
    if (!absFilename.IsEmpty() && absFilename.IsAbsolutePath() && xiiFileSystem::GetFileStats(absFilename, stats).Succeeded())
    {
      entry.m_uiFileSize = stats.m_uiFileSize;
    }
  }
}

XII_STATICLINK_FILE(Core, Core_Collection_Implementation_CollectionUtils);

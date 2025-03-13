#include <Core/CorePCH.h>

#include <Core/Collection/CollectionUtils.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/IO/OSFile.h>

void xiiCollectionUtils::AddFiles(xiiCollectionResourceDescriptor& ref_collection, xiiStringView sAssetTypeNameView, xiiStringView sAbsPathToFolder, xiiStringView sFileExtension, xiiStringView sStripPrefix, xiiStringView sPrependPrefix)
{
#if XII_ENABLED(XII_SUPPORTS_FILE_ITERATORS)

  const xiiUInt32 uiStripPrefixLength = xiiStringUtils::GetCharacterCount(sStripPrefix.GetStartPointer(), sStripPrefix.GetEndPointer());

  xiiFileSystemIterator fsIt;
  fsIt.StartSearch(sAbsPathToFolder, xiiFileSystemIteratorFlags::ReportFilesRecursive);

  if (!fsIt.IsValid())
    return;

  xiiStringBuilder sFullPath;
  xiiHashedString  sAssetTypeName;
  sAssetTypeName.Assign(sAssetTypeNameView);

  for (; fsIt.IsValid(); fsIt.Next())
  {
    const auto& stats = fsIt.GetStats();

    if (xiiPathUtils::HasExtension(stats.m_sName, sFileExtension))
    {
      stats.GetFullPath(sFullPath);

      sFullPath.Shrink(uiStripPrefixLength, 0);
      sFullPath.Prepend(sPrependPrefix);
      sFullPath.MakeCleanPath();

      auto& entry            = ref_collection.m_Resources.ExpandAndGetRef();
      entry.m_sAssetTypeName = sAssetTypeName;
      entry.m_sResourceID    = sFullPath;
      entry.m_uiFileSize     = stats.m_uiFileSize;
    }
  }

#else
  XII_IGNORE_UNUSED(ref_collection);
  XII_IGNORE_UNUSED(sAssetTypeNameView);
  XII_IGNORE_UNUSED(sAbsPathToFolder);
  XII_IGNORE_UNUSED(sFileExtension);
  XII_IGNORE_UNUSED(sStripPrefix);
  XII_IGNORE_UNUSED(sPrependPrefix);
  XII_ASSERT_NOT_IMPLEMENTED;
#endif
}


XII_CORE_DLL void xiiCollectionUtils::MergeCollections(xiiCollectionResourceDescriptor& ref_result, xiiArrayPtr<const xiiCollectionResourceDescriptor*> inputCollections)
{
  xiiMap<xiiString, const xiiCollectionEntry*> firstEntryOfID;

  for (const xiiCollectionResourceDescriptor* inputDesc : inputCollections)
  {
    for (const xiiCollectionEntry& inputEntry : inputDesc->m_Resources)
    {
      if (!firstEntryOfID.Contains(inputEntry.m_sResourceID))
      {
        firstEntryOfID.Insert(inputEntry.m_sResourceID, &inputEntry);
        ref_result.m_Resources.PushBack(inputEntry);
      }
    }
  }
}


XII_CORE_DLL void xiiCollectionUtils::DeDuplicateEntries(xiiCollectionResourceDescriptor& ref_result, const xiiCollectionResourceDescriptor& input)
{
  const xiiCollectionResourceDescriptor* firstInput = &input;
  MergeCollections(ref_result, xiiArrayPtr<const xiiCollectionResourceDescriptor*>(&firstInput, 1));
}

void xiiCollectionUtils::AddResourceHandle(xiiCollectionResourceDescriptor& ref_collection, xiiTypelessResourceHandle hHandle, xiiStringView sAssetTypeName, xiiStringView sAbsFolderpath)
{
  if (!hHandle.IsValid())
    return;

  const xiiStringView resID = hHandle.GetResourceID();

  auto& entry = ref_collection.m_Resources.ExpandAndGetRef();

  entry.m_sAssetTypeName.Assign(sAssetTypeName);
  entry.m_sResourceID = resID;

  xiiStringBuilder absFilename;

  // if a folder path is specified, replace the root (for testing filesize below)
  if (!sAbsFolderpath.IsEmpty())
  {
    xiiStringView root, relFile;
    xiiPathUtils::GetRootedPathParts(resID, root, relFile);
    absFilename = sAbsFolderpath;
    absFilename.AppendPath(relFile);
    absFilename.MakeCleanPath();

    xiiFileStats stats;
    if (!absFilename.IsEmpty() && absFilename.IsAbsolutePath() && xiiFileSystem::GetFileStats(absFilename, stats).Succeeded())
    {
      entry.m_uiFileSize = stats.m_uiFileSize;
    }
  }
}

XII_STATICLINK_FILE(Core, Core_Collection_Implementation_CollectionUtils);

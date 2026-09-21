/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Core/Collection/CollectionResource.h>

class xiiHashedString;

namespace xiiCollectionUtils
{
  /// Adds all files from \a szAbsPathToFolder and \a szFileExtension to \a collection
  ///
  /// The files are added as new entries using szAssetTypeName as the resource type identifier (see xiiResourceManager::RegisterResourceForAssetType).
  /// \a szStripPrefix is stripped from the file system paths and \a szPrependPrefix is prepended.
  XII_CORE_DLL void AddFiles(xiiCollectionResourceDescriptor& ref_collection, xiiStringView sAssetTypeName, xiiStringView sAbsPathToFolder, xiiStringView sFileExtension, xiiStringView sStripPrefix, xiiStringView sPrependPrefix);

  /// Merges all collections from the input array into the target result collection. Resource entries will be de-duplicated by resource ID
  /// string.
  XII_CORE_DLL void MergeCollections(xiiCollectionResourceDescriptor& ref_result, xiiArrayPtr<const xiiCollectionResourceDescriptor*> inputCollections);

  /// Special case of xiiCollectionUtils::MergeCollections which outputs unique entries from input collection into the result collection
  XII_CORE_DLL void DeDuplicateEntries(xiiCollectionResourceDescriptor& ref_result, const xiiCollectionResourceDescriptor& input);

  /// Extracts info (i.e. resource ID as file path) from the passed handle and adds it as a new resource entry. Does not add an entry if the
  /// resource handle is not valid.
  ///
  /// The resource type identifier must be passed explicity as szAssetTypeName (see xiiResourceManager::RegisterResourceForAssetType). To determine the
  /// file size, the resource ID is used as a filename passed to xiiFileSystem::GetFileStats. In case the resource's path root is not mounted, the path
  /// root can be replaced by passing non-NULL string to szAbsFolderpath, which will replace the root, e.g. with an absolute file path. This is just
  /// for the file size check within the scope of the function, it will not modify the resource Id.
  XII_CORE_DLL void AddResourceHandle(xiiCollectionResourceDescriptor& ref_collection, xiiTypelessResourceHandle hHandle, xiiStringView sAssetTypeName, xiiStringView sAbsFolderpath);

}; // namespace xiiCollectionUtils

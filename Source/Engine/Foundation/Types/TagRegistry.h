/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

class xiiHashedString;
class xiiTempHashedString;
class xiiTag;
class xiiStreamWriter;
class xiiStreamReader;

#include <Foundation/Containers/Map.h>
#include <Foundation/Threading/Mutex.h>

/// The tag registry for tags in tag sets.
///
/// Normal usage of the tag registry is to get the global tag registry instance via xiiTagRegistry::GetGlobalRegistry()
/// and to use this instance to register and get tags.
/// Certain special cases (e.g. tests) may actually need their own instance of the tag registry.
/// Note however that tags which were registered with one registry shouldn't be used with tag sets filled
/// with tags from another registry since there may be conflicting tag assignments.
/// The tag registry registration and tag retrieval functions are thread safe due to a mutex.
class XII_FOUNDATION_DLL xiiTagRegistry
{
public:
  xiiTagRegistry();

  static xiiTagRegistry& GetGlobalRegistry();

  /// Ensures the tag with the given name exists and returns a pointer to it.
  const xiiTag& RegisterTag(xiiStringView sTagString); // [tested]

  /// Ensures the tag with the given name exists and returns a pointer to it.
  const xiiTag& RegisterTag(const xiiHashedString& sTagString); // [tested]

  /// Searches for a tag with the given name and returns a pointer to it
  const xiiTag* GetTagByName(const xiiTempHashedString& sTagString) const; // [tested]

  /// Searches for a tag with the given murmur hash. This function is only for backwards compatibility.
  const xiiTag* GetTagByMurmurHash(xiiUInt32 uiMurmurHash) const;

  /// Returns the tag with the given index.
  const xiiTag* GetTagByIndex(xiiUInt32 uiIndex) const;

  /// Returns the number of registered tags.
  xiiUInt32 GetNumTags() const;

  /// Loads the saved state and integrates it into this registry. Does not discard previously registered tag information. This function is only
  /// for backwards compatibility.
  xiiResult Load(xiiStreamReader& ref_stream);

protected:
  mutable xiiMutex m_TagRegistryMutex;

  xiiMap<xiiTempHashedString, xiiTag> m_RegisteredTags;
  xiiDeque<xiiTag*>                   m_TagsByIndex;
};

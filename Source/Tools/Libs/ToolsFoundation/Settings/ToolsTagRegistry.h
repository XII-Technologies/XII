#pragma once

#include <Foundation/Containers/Map.h>
#include <Foundation/Types/Status.h>
#include <Foundation/Types/Variant.h>
#include <ToolsFoundation/ToolsFoundationDLL.h>

struct XII_TOOLSFOUNDATION_DLL xiiToolsTag
{
  xiiToolsTag() = default;
  xiiToolsTag(xiiStringView sCategory, xiiStringView sName, bool bBuiltIn = false) :
    m_sCategory(sCategory), m_sName(sName), m_bBuiltInTag(bBuiltIn)
  {
  }

  xiiString m_sCategory;
  xiiString m_sName;
  bool      m_bBuiltInTag = false; ///< If set to true, this is a tag created by code that the user is not allowed to remove
};

class XII_TOOLSFOUNDATION_DLL xiiToolsTagRegistry
{
public:
  /// \brief Removes all tags that are not specified as 'built-in'.
  static void Clear();

  /// \brief Serializes all tags to a DDL stream.
  static void WriteToDDL(xiiStreamWriter& inout_stream);

  /// \brief Reads tags from a DDL stream.
  static xiiStatus ReadFromDDL(xiiStreamReader& inout_stream);


  /// \brief Adds a tag to the registry. Returns true if the tag was valid.
  static bool AddTag(const xiiToolsTag& tag);

  /// \brief Removes a tag by name. Returns true if the tag was removed.
  static bool RemoveTag(xiiStringView sName);


  /// \brief Retrieves all tags in the registry.
  static void GetAllTags(xiiHybridArray<const xiiToolsTag*, 16>& out_tags);

  /// \brief Retrieves all tags in the given categories.
  static void GetTagsByCategory(const xiiArrayPtr<xiiStringView>& categories, xiiHybridArray<const xiiToolsTag*, 16>& out_tags);

private:
  static xiiMap<xiiString, xiiToolsTag> s_NameToTags;
};

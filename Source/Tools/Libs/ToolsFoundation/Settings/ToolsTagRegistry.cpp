#include <ToolsFoundation/ToolsFoundationPCH.h>

#include <Foundation/Configuration/Startup.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/IO/OpenDdlReader.h>
#include <Foundation/IO/OpenDdlUtils.h>
#include <Foundation/IO/OpenDdlWriter.h>
#include <Foundation/Logging/Log.h>
#include <ToolsFoundation/Settings/ToolsTagRegistry.h>

struct TagComparer
{
  XII_ALWAYS_INLINE bool Less(const xiiToolsTag* a, const xiiToolsTag* b) const
  {
    if (a->m_sCategory != b->m_sCategory)
      return a->m_sCategory < b->m_sCategory;

    return a->m_sName < b->m_sName;
    ;
  }
};
////////////////////////////////////////////////////////////////////////
// xiiToolsTagRegistry public functions
////////////////////////////////////////////////////////////////////////

xiiMap<xiiString, xiiToolsTag> xiiToolsTagRegistry::s_NameToTags;

void xiiToolsTagRegistry::Clear()
{
  for (auto it = s_NameToTags.GetIterator(); it.IsValid();)
  {
    if (!it.Value().m_bBuiltInTag)
    {
      it = s_NameToTags.Remove(it);
    }
    else
    {
      ++it;
    }
  }
}

void xiiToolsTagRegistry::WriteToDDL(xiiStreamWriter& stream)
{
  xiiOpenDdlWriter writer;
  writer.SetOutputStream(&stream);
  writer.SetCompactMode(false);
  writer.SetPrimitiveTypeStringMode(xiiOpenDdlWriter::TypeStringMode::ShortenedUnsignedInt);

  for (auto it = s_NameToTags.GetIterator(); it.IsValid(); ++it)
  {
    writer.BeginObject("Tag");

    writer.BeginPrimitiveList(xiiOpenDdlPrimitiveType::String, "Name");
    writer.WriteString(it.Value().m_sName);
    writer.EndPrimitiveList();

    writer.BeginPrimitiveList(xiiOpenDdlPrimitiveType::String, "Category");
    writer.WriteString(it.Value().m_sCategory);
    writer.EndPrimitiveList();

    writer.EndObject();
  }
}

xiiStatus xiiToolsTagRegistry::ReadFromDDL(xiiStreamReader& stream)
{
  xiiOpenDdlReader reader;
  if (reader.ParseDocument(stream).Failed())
  {
    return xiiStatus("Failed to read data from ToolsTagRegistry stream!");
  }

  // Makes sure not to remove the built-in tags
  Clear();

  const xiiOpenDdlReaderElement* pRoot = reader.GetRootElement();

  for (const xiiOpenDdlReaderElement* pTags = pRoot->GetFirstChild(); pTags != nullptr; pTags = pTags->GetSibling())
  {
    if (!pTags->IsCustomType("Tag"))
      continue;

    const xiiOpenDdlReaderElement* pName     = pTags->FindChildOfType(xiiOpenDdlPrimitiveType::String, "Name");
    const xiiOpenDdlReaderElement* pCategory = pTags->FindChildOfType(xiiOpenDdlPrimitiveType::String, "Category");

    if (!pName || !pCategory)
    {
      xiiLog::Error("Incomplete tag declaration!");
      continue;
    }

    xiiToolsTag tag;
    tag.m_sName     = pName->GetPrimitivesString()[0];
    tag.m_sCategory = pCategory->GetPrimitivesString()[0];

    if (!xiiToolsTagRegistry::AddTag(tag))
    {
      xiiLog::Error("Failed to add tag '{0}'", tag.m_sName);
    }
  }

  return xiiStatus(XII_SUCCESS);
}

bool xiiToolsTagRegistry::AddTag(const xiiToolsTag& tag)
{
  if (tag.m_sName.IsEmpty())
    return false;

  auto it = s_NameToTags.Find(tag.m_sName);
  if (it.IsValid())
  {
    if (tag.m_bBuiltInTag)
    {
      // Make sure to pass this on, as it is not stored in the DDL file (because we don't want to rely on that)
      it.Value().m_bBuiltInTag = true;
    }

    return true;
  }
  else
  {
    s_NameToTags[tag.m_sName] = tag;
    return true;
  }
}

bool xiiToolsTagRegistry::RemoveTag(const char* szName)
{
  auto it = s_NameToTags.Find(szName);
  if (it.IsValid())
  {
    s_NameToTags.Remove(it);
    return true;
  }
  else
  {
    return false;
  }
}

void xiiToolsTagRegistry::GetAllTags(xiiHybridArray<const xiiToolsTag*, 16>& out_tags)
{
  out_tags.Clear();
  for (auto it = s_NameToTags.GetIterator(); it.IsValid(); ++it)
  {
    out_tags.PushBack(&it.Value());
  }

  out_tags.Sort(TagComparer());
}

void xiiToolsTagRegistry::GetTagsByCategory(const xiiArrayPtr<xiiStringView>& categories, xiiHybridArray<const xiiToolsTag*, 16>& out_tags)
{
  out_tags.Clear();
  for (auto it = s_NameToTags.GetIterator(); it.IsValid(); ++it)
  {
    if (std::any_of(cbegin(categories), cend(categories), [&it](const xiiStringView& cat) { return it.Value().m_sCategory == cat; }))
    {
      out_tags.PushBack(&it.Value());
    }
  }
  out_tags.Sort(TagComparer());
}

#include <ToolsFoundation/ToolsFoundationPCH.h>

#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/MemoryStream.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Serialization/DdlSerializer.h>
#include <ToolsFoundation/Document/PrefabCache.h>
#include <ToolsFoundation/Document/PrefabUtils.h>
#include <ToolsFoundation/Project/ToolsProject.h>

XII_IMPLEMENT_SINGLETON(xiiPrefabCache);

// clang-format off
XII_BEGIN_SUBSYSTEM_DECLARATION(ToolsFoundation, xiiPrefabCache)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "Foundation"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    XII_DEFAULT_NEW(xiiPrefabCache);
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    xiiPrefabCache* pDummy = xiiPrefabCache::GetSingleton();
    XII_DEFAULT_DELETE(pDummy);
  }

  ON_HIGHLEVELSYSTEMS_STARTUP
  {
  }

  ON_HIGHLEVELSYSTEMS_SHUTDOWN
  {
  }

XII_END_SUBSYSTEM_DECLARATION;
// clang-format on

xiiPrefabCache::xiiPrefabCache() :
  m_SingletonRegistrar(this)
{
}

const xiiStringBuilder& xiiPrefabCache::GetCachedPrefabDocument(const xiiUuid& documentGuid)
{
  PrefabData& data = xiiPrefabCache::GetOrCreatePrefabCache(documentGuid);
  return data.m_sDocContent;
}

const xiiAbstractObjectGraph* xiiPrefabCache::GetCachedPrefabGraph(const xiiUuid& documentGuid)
{
  PrefabData& data = xiiPrefabCache::GetOrCreatePrefabCache(documentGuid);
  if (data.m_sAbsPath.IsEmpty())
    return nullptr;
  return &data.m_Graph;
}

void xiiPrefabCache::LoadGraph(xiiAbstractObjectGraph& out_graph, xiiStringView sGraph)
{
  xiiUInt64 uiHash = xiiHashingUtils::xxHash64(sGraph.GetStartPointer(), sGraph.GetElementCount());
  auto      it     = m_CachedGraphs.Find(uiHash);
  if (!it.IsValid())
  {
    it = m_CachedGraphs.Insert(uiHash, xiiUniquePtr<xiiAbstractObjectGraph>(XII_DEFAULT_NEW(xiiAbstractObjectGraph)));

    xiiRawMemoryStreamReader             stringReader(sGraph.GetStartPointer(), sGraph.GetElementCount());
    xiiUniquePtr<xiiAbstractObjectGraph> header;
    xiiUniquePtr<xiiAbstractObjectGraph> types;
    xiiAbstractGraphDdlSerializer::ReadDocument(stringReader, header, it.Value(), types, true).IgnoreResult();
  }

  it.Value()->Clone(out_graph);
}

xiiPrefabCache::PrefabData& xiiPrefabCache::GetOrCreatePrefabCache(const xiiUuid& documentGuid)
{
  auto it = m_PrefabData.Find(documentGuid);

#if XII_ENABLED(XII_SUPPORTS_FILE_STATS)
  if (it.IsValid())
  {
    xiiFileStats Stats;
    if (xiiOSFile::GetFileStats(it.Value()->m_sAbsPath, Stats).Succeeded() && !Stats.m_LastModificationTime.Compare(it.Value()->m_fileModifiedTime, xiiTimestamp::CompareMode::FileTimeEqual))
    {
      UpdatePrefabData(*it.Value().Borrow());
    }
  }
  else
  {
    it = m_PrefabData.Insert(documentGuid, xiiUniquePtr<PrefabData>(XII_DEFAULT_NEW(PrefabData)));

    it.Value()->m_documentGuid = documentGuid;
    it.Value()->m_sAbsPath     = xiiToolsProject::GetSingleton()->GetPathForDocumentGuid(documentGuid);
    if (it.Value()->m_sAbsPath.IsEmpty())
    {
      xiiStringBuilder sGuid;
      xiiConversionUtils::ToString(documentGuid, sGuid);
      xiiLog::Error("Can't resolve prefab document guid '{0}'. The resolved path is empty", sGuid);
    }
    else
      UpdatePrefabData(*it.Value().Borrow());
  }
#else
  XII_ASSERT_NOT_IMPLEMENTED;
#endif

  return *it.Value().Borrow();
}

void xiiPrefabCache::UpdatePrefabData(PrefabData& data)
{
#if XII_ENABLED(XII_SUPPORTS_FILE_STATS)
  if (data.m_sAbsPath.IsEmpty())
  {
    data.m_sAbsPath = xiiToolsProject::GetSingleton()->GetPathForDocumentGuid(data.m_documentGuid);
    if (data.m_sAbsPath.IsEmpty())
    {
      xiiStringBuilder sGuid;
      xiiConversionUtils::ToString(data.m_documentGuid, sGuid);
      xiiLog::Error("Can't resolve prefab document guid '{0}'. The resolved path is empty", sGuid);
      return;
    }
  }

  xiiFileStats Stats;
  bool         bStat = xiiOSFile::GetFileStats(data.m_sAbsPath, Stats).Succeeded();

  if (!bStat)
  {
    xiiLog::Error("Can't update prefab file '{0}', the file can't be opened.", data.m_sAbsPath);
    return;
  }

  data.m_sDocContent = xiiPrefabUtils::ReadDocumentAsString(data.m_sAbsPath);

  if (data.m_sDocContent.IsEmpty())
    return;

  data.m_fileModifiedTime = Stats.m_LastModificationTime;
  data.m_Graph.Clear();
  xiiPrefabUtils::LoadGraph(data.m_Graph, data.m_sDocContent);
#else
  XII_ASSERT_NOT_IMPLEMENTED;
#endif
}

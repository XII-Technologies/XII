#pragma once

#include <Foundation/Configuration/Singleton.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <Foundation/Time/Timestamp.h>
#include <Foundation/Types/UniquePtr.h>
#include <ToolsFoundation/ToolsFoundationDLL.h>

class xiiAbstractObjectGraph;

class XII_TOOLSFOUNDATION_DLL xiiPrefabCache
{
  XII_DECLARE_SINGLETON(xiiPrefabCache);

public:
  xiiPrefabCache();

  const xiiStringBuilder&       GetCachedPrefabDocument(const xiiUuid& documentGuid);
  const xiiAbstractObjectGraph* GetCachedPrefabGraph(const xiiUuid& documentGuid);
  void                          LoadGraph(xiiAbstractObjectGraph& out_graph, xiiStringView sGraph);

private:
  XII_MAKE_SUBSYSTEM_STARTUP_FRIEND(ToolsFoundation, xiiPrefabCache);

  struct PrefabData
  {
    PrefabData() = default;

    xiiUuid   m_documentGuid;
    xiiString m_sAbsPath;

    xiiAbstractObjectGraph m_Graph;
    xiiStringBuilder       m_sDocContent;
    xiiTimestamp           m_fileModifiedTime;
  };
  PrefabData& GetOrCreatePrefabCache(const xiiUuid& documentGuid);
  void        UpdatePrefabData(PrefabData& data);

  xiiMap<xiiUInt64, xiiUniquePtr<xiiAbstractObjectGraph>> m_CachedGraphs;
  xiiMap<xiiUuid, xiiUniquePtr<PrefabData>>               m_PrefabData;
};

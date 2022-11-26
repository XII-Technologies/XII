#include <EditorPluginRecast/EditorPluginRecastPCH.h>

#include <EditorEngineProcessFramework/LongOps/LongOps.h>

class xiiLongOpProxy_BuildNavMesh : public xiiLongOpProxy
{
  XII_ADD_DYNAMIC_REFLECTION(xiiLongOpProxy_BuildNavMesh, xiiLongOpProxy);

public:
  virtual void        InitializeRegistered(const xiiUuid& documentGuid, const xiiUuid& componentGuid) override;
  virtual const char* GetDisplayName() const override { return "Generate NavMesh"; }
  virtual void        GetReplicationInfo(xiiStringBuilder& out_sReplicationOpType, xiiStreamWriter& description) override;
  virtual void        Finalize(xiiResult result, const xiiDataBuffer& resultData) override;

private:
  xiiUuid m_DocumentGuid;
  xiiUuid m_ComponentGuid;
};

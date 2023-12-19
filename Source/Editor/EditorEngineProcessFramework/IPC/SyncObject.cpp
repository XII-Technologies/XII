#include <EditorEngineProcessFramework/EditorEngineProcessFrameworkPCH.h>

#include <EditorEngineProcessFramework/IPC/SyncObject.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiEditorEngineSyncObject, 1, xiiRTTINoAllocator)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("SyncGuid", m_SyncObjectGuid),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiEditorEngineSyncObject::xiiEditorEngineSyncObject()
{
  m_SyncObjectGuid = xiiUuid::MakeUuid();
  m_bModified      = true;
}

xiiEditorEngineSyncObject::~xiiEditorEngineSyncObject()
{
  if (m_OnDestruction.IsValid())
  {
    m_OnDestruction(this);
  }
}

void xiiEditorEngineSyncObject::Configure(xiiUuid ownerGuid, xiiDelegate<void(xiiEditorEngineSyncObject*)> onDestruction)
{
  m_OwnerGuid     = ownerGuid;
  m_OnDestruction = onDestruction;
}

xiiUuid xiiEditorEngineSyncObject::GetDocumentGuid() const
{
  return m_OwnerGuid;
}

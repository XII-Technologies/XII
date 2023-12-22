#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/IPC/EngineProcessConnection.h>
#include <Foundation/IO/FileSystem/DeferredFileWriter.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/OpenDdlReader.h>
#include <Foundation/Serialization/ReflectionSerializer.h>

const xiiPlatformProfile* xiiAssetCurator::GetDevelopmentAssetProfile() const
{
  return m_AssetProfiles[0];
}

const xiiPlatformProfile* xiiAssetCurator::GetActiveAssetProfile() const
{
  return m_AssetProfiles[m_uiActiveAssetProfile];
}

xiiUInt32 xiiAssetCurator::GetActiveAssetProfileIndex() const
{
  return m_uiActiveAssetProfile;
}

xiiUInt32 xiiAssetCurator::FindAssetProfileByName(const char* szPlatform)
{
  XII_LOCK(m_CuratorMutex);

  XII_ASSERT_DEV(!m_AssetProfiles.IsEmpty(), "Need to have a valid asset platform config");

  for (xiiUInt32 i = 0; i < m_AssetProfiles.GetCount(); ++i)
  {
    if (m_AssetProfiles[i]->m_sName.IsEqual_NoCase(szPlatform))
    {
      return i;
    }
  }

  return xiiInvalidIndex;
}

xiiUInt32 xiiAssetCurator::GetNumAssetProfiles() const
{
  return m_AssetProfiles.GetCount();
}

const xiiPlatformProfile* xiiAssetCurator::GetAssetProfile(xiiUInt32 uiIndex) const
{
  if (uiIndex >= m_AssetProfiles.GetCount())
    return m_AssetProfiles[0]; // fall back to default platform

  return m_AssetProfiles[uiIndex];
}

xiiPlatformProfile* xiiAssetCurator::GetAssetProfile(xiiUInt32 uiIndex)
{
  if (uiIndex >= m_AssetProfiles.GetCount())
    return m_AssetProfiles[0]; // fall back to default platform

  return m_AssetProfiles[uiIndex];
}

xiiPlatformProfile* xiiAssetCurator::CreateAssetProfile()
{
  xiiPlatformProfile* pProfile = XII_DEFAULT_NEW(xiiPlatformProfile);
  m_AssetProfiles.PushBack(pProfile);

  return pProfile;
}

xiiResult xiiAssetCurator::DeleteAssetProfile(xiiPlatformProfile* pProfile)
{
  if (m_AssetProfiles.GetCount() <= 1)
    return XII_FAILURE;

  // do not allow to delete element 0 !

  for (xiiUInt32 i = 1; i < m_AssetProfiles.GetCount(); ++i)
  {
    if (m_AssetProfiles[i] == pProfile)
    {
      if (m_uiActiveAssetProfile == i)
        return XII_FAILURE;

      if (i < m_uiActiveAssetProfile)
        --m_uiActiveAssetProfile;

      XII_DEFAULT_DELETE(pProfile);
      m_AssetProfiles.RemoveAtAndCopy(i);

      return XII_SUCCESS;
    }
  }

  return XII_FAILURE;
}

void xiiAssetCurator::SetActiveAssetProfileByIndex(xiiUInt32 uiIndex, bool bForceReevaluation /*= false*/)
{
  if (uiIndex >= m_AssetProfiles.GetCount())
    uiIndex = 0; // fall back to default platform

  if (!bForceReevaluation && m_uiActiveAssetProfile == uiIndex)
    return;

  XII_LOG_BLOCK("Switch Active Asset Platform", m_AssetProfiles[uiIndex]->GetConfigName());

  m_uiActiveAssetProfile = uiIndex;

  CheckFileSystem();

  {
    xiiAssetCuratorEvent e;
    e.m_Type = xiiAssetCuratorEvent::Type::ActivePlatformChanged;
    m_Events.Broadcast(e);
  }

  {
    xiiSimpleConfigMsgToEngine msg;
    msg.m_sWhatToDo = "ChangeActivePlatform";
    msg.m_sPayload  = GetActiveAssetProfile()->GetConfigName();
    xiiEditorEngineProcessConnection::GetSingleton()->SendMessage(&msg);
  }
}

void xiiAssetCurator::SaveRuntimeProfiles()
{
  for (xiiUInt32 i = 0; i < GetNumAssetProfiles(); ++i)
  {
    xiiStringBuilder sProfileRuntimeDataFile;

    xiiPlatformProfile* pProfile = GetAssetProfile(i);

    sProfileRuntimeDataFile.Set(":project/RuntimeConfigs/", pProfile->GetConfigName(), ".xiiProfile");

    pProfile->SaveForRuntime(sProfileRuntimeDataFile).IgnoreResult();
  }
}

xiiResult xiiAssetCurator::SaveAssetProfiles()
{
  xiiDeferredFileWriter file;
  file.SetOutput(":project/Editor/AssetProfiles.ddl");

  xiiOpenDdlWriter ddl;
  ddl.SetOutputStream(&file);

  ddl.BeginObject("AssetProfiles");

  for (const auto* pCfg : m_AssetProfiles)
  {
    ddl.BeginObject("Config", pCfg->m_sName);

    // make sure to create the same GUID every time, otherwise the serialized file changes all the time
    const xiiUuid guid = xiiUuid::StableUuidForString(pCfg->GetConfigName());

    xiiReflectionSerializer::WriteObjectToDDL(ddl, pCfg->GetDynamicRTTI(), pCfg, guid);

    ddl.EndObject();
  }

  ddl.EndObject();

  return file.Close();
}

xiiResult xiiAssetCurator::LoadAssetProfiles()
{
  XII_LOG_BLOCK("LoadAssetProfiles", ":project/Editor/PlatformProfiles.ddl");

  xiiFileReader file;
  if (file.Open(":project/Editor/AssetProfiles.ddl").Failed())
    return XII_FAILURE;

  xiiOpenDdlReader ddl;
  if (ddl.ParseDocument(file).Failed())
    return XII_FAILURE;

  const xiiOpenDdlReaderElement* pRootElement = ddl.GetRootElement()->FindChildOfType("AssetProfiles");

  if (!pRootElement)
    return XII_FAILURE;

  if (pRootElement->FindChildOfType("Config") == nullptr)
    return XII_FAILURE;

  ClearAssetProfiles();

  for (auto pChild = pRootElement->GetFirstChild(); pChild != nullptr; pChild = pChild->GetSibling())
  {
    if (pChild->IsCustomType("Config"))
    {
      const xiiRTTI* pRtti      = nullptr;
      void*          pConfigObj = xiiReflectionSerializer::ReadObjectFromDDL(pChild, pRtti);

      auto pProfile = static_cast<xiiPlatformProfile*>(pConfigObj);

      pProfile->AddMissingConfigs();

      m_AssetProfiles.PushBack(pProfile);
    }
  }

  return XII_SUCCESS;
}

void xiiAssetCurator::ClearAssetProfiles()
{
  for (auto pCfg : m_AssetProfiles)
  {
    pCfg->GetDynamicRTTI()->GetAllocator()->Deallocate(pCfg);
  }

  m_AssetProfiles.Clear();
}

void xiiAssetCurator::SetupDefaultAssetProfiles()
{
  ClearAssetProfiles();

  {
    xiiPlatformProfile* pCfg = XII_DEFAULT_NEW(xiiPlatformProfile);
    pCfg->m_sName            = "PC";
    pCfg->AddMissingConfigs();
    m_AssetProfiles.PushBack(pCfg);
  }
}

void xiiAssetCurator::ComputeAllDocumentManagerAssetProfileHashes()
{
  for (auto pMan : xiiDocumentManager::GetAllDocumentManagers())
  {
    if (auto pAssMan = xiiDynamicCast<xiiAssetDocumentManager*>(pMan))
    {
      pAssMan->ComputeAssetProfileHash(GetActiveAssetProfile());
    }
  }
}

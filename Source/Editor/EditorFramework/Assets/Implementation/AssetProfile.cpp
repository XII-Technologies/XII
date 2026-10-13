/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/IPC/EngineProcessConnection.h>
#include <Foundation/Algorithm/HashingUtils.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/MemoryStream.h>
#include <Foundation/IO/OpenDdlReader.h>
#include <Foundation/Serialization/DdlSerializer.h>
#include <Foundation/Serialization/ReflectionSerializer.h>
#include <Foundation/Serialization/RttiConverter.h>
#include <Foundation/Types/ScopeExit.h>

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
    if (m_AssetProfiles[i]->GetConfigName().IsEqual_NoCase(szPlatform))
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

xiiResult xiiAssetCurator::SaveRuntimeProfiles()
{
  for (xiiUInt32 i = 0; i < GetNumAssetProfiles(); ++i)
  {
    xiiStringBuilder sProfileRuntimeDataFile;

    xiiPlatformProfile* pProfile = GetAssetProfile(i);

    sProfileRuntimeDataFile.Set(":project/RuntimeConfigs/", pProfile->GetConfigName(), ".xiiProfile");

    XII_SUCCEED_OR_RETURN(pProfile->SaveForRuntime(sProfileRuntimeDataFile));
  }
  return XII_SUCCESS;
}

xiiResult xiiAssetCurator::ReloadAssetProfiles()
{
  XII_LOG_BLOCK("ReloadAssetProfiles", ":project/Editor/AssetProfiles.ddl");

  xiiFileReader file;
  if (file.Open(":project/Editor/AssetProfiles.ddl").Failed())
    return XII_FAILURE;

  if (file.GetFileSize() > xiiMath::MaxValue<xiiUInt32>())
    return XII_FAILURE;
  xiiContiguousMemoryStreamStorage contents;
  contents.ReadAll(file, file.GetFileSize());
  if (contents.GetStorageSize64() != file.GetFileSize())
    return XII_FAILURE;
  file.Close();

  const xiiUInt64 hash = xiiHashingUtils::xxHash64(contents.GetData(), contents.GetStorageSize64());
  if (m_bAssetProfilesLoaded && hash == m_uiLoadedAssetProfilesHash)
    return XII_SUCCESS;

  xiiMemoryStreamReader reader(&contents);
  xiiOpenDdlReader      ddl;
  if (ddl.ParseDocument(reader).Failed())
    return XII_FAILURE;

  const xiiOpenDdlReaderElement* pRootElement = ddl.GetRootElement()->FindChildOfType("AssetProfiles");

  if (!pRootElement)
    return XII_FAILURE;

  if (pRootElement->FindChildOfType("Config") == nullptr)
    return XII_FAILURE;

  xiiHybridArray<xiiPlatformProfile*, 8> profiles;
  XII_SCOPE_EXIT(for (auto* profile : profiles) profile->GetDynamicRTTI()->GetAllocator()->Deallocate(profile));

  for (auto pChild = pRootElement->GetFirstChild(); pChild != nullptr; pChild = pChild->GetSibling())
  {
    if (pChild->IsCustomType("Config"))
    {
      if (pChild->FindChildOfType("Objects") == nullptr)
        return XII_FAILURE;
      xiiAbstractObjectGraph graph;
      XII_SUCCEED_OR_RETURN(xiiAbstractGraphDdlSerializer::Read(pChild, &graph));
      const auto* root = graph.GetNodeByName("root");
      if (root == nullptr || root->GetType() != xiiGetStaticRTTI<xiiPlatformProfile>()->GetTypeName())
        return XII_FAILURE;

      // Validate owned configuration types before the reflection reader constructs them.
      if (const auto* configs = root->FindProperty("Configs"))
      {
        if (!configs->m_Value.IsA<xiiVariantArray>())
          return XII_FAILURE;
        xiiSet<const xiiRTTI*> types;
        for (const auto& id : configs->m_Value.Get<xiiVariantArray>())
        {
          if (!id.IsA<xiiUuid>())
            return XII_FAILURE;
          const auto* node = graph.GetNode(id.Get<xiiUuid>());
          const auto* type = node != nullptr ? xiiRTTI::FindTypeByName(node->GetType()) : nullptr;
          if (type == nullptr || !type->IsDerivedFrom<xiiProfileConfigData>() || !type->GetAllocator()->CanAllocate() || types.Contains(type))
            return XII_FAILURE;
          types.Insert(type);
        }
      }

      xiiRttiConverterContext context;
      xiiRttiConverterReader  converter(&graph, &context);
      xiiPlatformProfile*     pProfile = context.CreateObject(root->GetGuid(), xiiGetStaticRTTI<xiiPlatformProfile>()).Cast<xiiPlatformProfile>();
      if (pProfile == nullptr)
        return XII_FAILURE;
      profiles.PushBack(pProfile);
      converter.ApplyPropertiesToObject(root, xiiGetStaticRTTI<xiiPlatformProfile>(), pProfile);
      if (!xiiStringUtils::IsValidIdentifierName(pProfile->GetConfigName().GetStartPointer(), pProfile->GetConfigName().GetEndPointer()))
        return XII_FAILURE;
      for (xiiUInt32 i = 0; i + 1 < profiles.GetCount(); ++i)
      {
        if (profiles[i]->GetConfigName().IsEqual_NoCase(pProfile->GetConfigName()))
          return XII_FAILURE;
      }

      pProfile->AddMissingConfigs();
    }
  }

  xiiUInt32 defaultIndex = xiiInvalidIndex;
  for (xiiUInt32 i = 0; i < profiles.GetCount(); ++i)
  {
    if (profiles[i]->GetConfigName().IsEqual_NoCase("Default"))
    {
      profiles[i]->SetConfigName("Default");
      defaultIndex = i;
      break;
    }
  }
  if (defaultIndex == xiiInvalidIndex)
  {
    xiiPlatformProfile* pCfg = XII_DEFAULT_NEW(xiiPlatformProfile);
    pCfg->SetConfigName("Default");
    pCfg->SetTargetPlatform("Windows");

    pCfg->AddMissingConfigs();
    profiles.InsertAt(0, pCfg);
  }
  else if (defaultIndex != 0)
  {
    auto* profile = profiles[defaultIndex];
    profiles.RemoveAtAndCopy(defaultIndex);
    profiles.InsertAt(0, profile);
  }

  xiiUInt32       activeIndex = 0;
  const xiiString activeName  = m_AssetProfiles.IsEmpty() ? xiiString() : xiiString(GetActiveAssetProfile()->GetConfigName());
  for (xiiUInt32 i = 0; i < profiles.GetCount(); ++i)
    if (profiles[i]->GetConfigName().IsEqual_NoCase(activeName))
      activeIndex = i;
  PublishAssetProfiles(profiles, hash, activeIndex);

  return XII_SUCCESS;
}

void xiiAssetCurator::ClearAssetProfiles()
{
  for (auto pCfg : m_AssetProfiles)
  {
    pCfg->GetDynamicRTTI()->GetAllocator()->Deallocate(pCfg);
  }

  m_AssetProfiles.Clear();
  m_uiActiveAssetProfile      = 0;
  m_uiLoadedAssetProfilesHash = 0;
  m_bAssetProfilesLoaded      = false;
}

void xiiAssetCurator::SetupDefaultAssetProfiles()
{
  ClearAssetProfiles();

  {
    xiiPlatformProfile* pCfg = XII_DEFAULT_NEW(xiiPlatformProfile);
    pCfg->SetConfigName("Default");
    pCfg->SetTargetPlatform("Windows");
    pCfg->AddMissingConfigs();
    m_AssetProfiles.PushBack(pCfg);
  }
}

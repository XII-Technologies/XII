/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <Foundation/IO/FileSystem/DeferredFileWriter.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/IO/OpenDdlReader.h>
#include <Foundation/IO/OpenDdlUtils.h>
#include <Foundation/IO/OpenDdlWriter.h>
#include <Foundation/Profiling/Profiling.h>

void xiiPluginBundle::WriteStateToDDL(xiiOpenDdlWriter& ref_ddl, xiiStringView sOwnName) const
{
  ref_ddl.BeginObject("PluginState");
  xiiOpenDdlUtils::StoreString(ref_ddl, sOwnName, "ID");
  xiiOpenDdlUtils::StoreBool(ref_ddl, m_bSelected, "Selected");
  xiiOpenDdlUtils::StoreBool(ref_ddl, m_bLoadCopy, "LoadCopy");
  ref_ddl.EndObject();
}

void xiiPluginBundle::ReadStateFromDDL(xiiOpenDdlReader& ref_ddl, xiiStringView sOwnName)
{
  m_bSelected = false;

  auto pState = ref_ddl.GetRootElement()->FindChildOfType("PluginState");
  while (pState)
  {
    auto pName = pState->FindChildOfType(xiiOpenDdlPrimitiveType::String, "ID");
    if (!pName || pName->GetPrimitivesString()[0] != sOwnName)
    {
      pState = pState->GetSibling();
      continue;
    }

    if (auto pVal = pState->FindChildOfType(xiiOpenDdlPrimitiveType::Bool, "Selected"))
      m_bSelected = pVal->GetPrimitivesBool()[0];
    if (auto pVal = pState->FindChildOfType(xiiOpenDdlPrimitiveType::Bool, "LoadCopy"))
      m_bLoadCopy = pVal->GetPrimitivesBool()[0];

    break;
  }
}

void xiiPluginBundleSet::SetFromTemplate(xiiStringView sTemplateName)
{
  for (auto it : m_Plugins)
  {
    xiiPluginBundle& bundle = it.Value();

    bundle.m_bSelected = bundle.m_EnabledInTemplates.Contains(sTemplateName);
  }
}

void xiiPluginBundleSet::WriteStateToDDL(xiiOpenDdlWriter& ref_ddl) const
{
  for (const auto& it : m_Plugins)
  {
    if (it.Value().m_bSelected)
    {
      it.Value().WriteStateToDDL(ref_ddl, it.Key());
    }
  }
}

void xiiPluginBundleSet::ReadStateFromDDL(xiiOpenDdlReader& ref_ddl)
{
  for (auto& it : m_Plugins)
  {
    it.Value().ReadStateFromDDL(ref_ddl, it.Key());
  }
}

bool xiiPluginBundleSet::IsStateEqual(const xiiPluginBundleSet& rhs) const
{
  if (m_Plugins.GetCount() != rhs.m_Plugins.GetCount())
    return false;

  for (auto it : m_Plugins)
  {
    auto it2 = rhs.m_Plugins.Find(it.Key());

    if (!it2.IsValid())
      return false;

    if (!it.Value().IsStateEqual(it2.Value()))
      return false;
  }

  return true;
}

xiiResult xiiPluginBundle::ReadBundleFromDDL(xiiOpenDdlReader& ref_ddl)
{
  XII_LOG_BLOCK("Reading plugin info file");

  auto pInfo = ref_ddl.GetRootElement()->FindChildOfType("PluginInfo");

  if (pInfo == nullptr)
  {
    xiiLog::Error("'PluginInfo' root object is missing");
    return XII_FAILURE;
  }

  if (auto pElement = pInfo->FindChildOfType(xiiOpenDdlPrimitiveType::Bool, "Mandatory"))
    m_bMandatory = pElement->GetPrimitivesBool()[0];

  if (auto pElement = pInfo->FindChildOfType(xiiOpenDdlPrimitiveType::String, "DisplayName"))
    m_sDisplayName = pElement->GetPrimitivesString()[0];

  if (auto pElement = pInfo->FindChildOfType(xiiOpenDdlPrimitiveType::String, "Description"))
    m_sDescription = pElement->GetPrimitivesString()[0];

  if (auto pElement = pInfo->FindChildOfType(xiiOpenDdlPrimitiveType::String, "EditorPlugins"))
  {
    m_EditorPlugins.SetCount(pElement->GetNumPrimitives());
    for (xiiUInt32 i = 0; i < pElement->GetNumPrimitives(); ++i)
      m_EditorPlugins[i] = pElement->GetPrimitivesString()[i];
  }

  if (auto pElement = pInfo->FindChildOfType(xiiOpenDdlPrimitiveType::String, "EditorEnginePlugins"))
  {
    m_EditorEnginePlugins.SetCount(pElement->GetNumPrimitives());
    for (xiiUInt32 i = 0; i < pElement->GetNumPrimitives(); ++i)
      m_EditorEnginePlugins[i] = pElement->GetPrimitivesString()[i];
  }

  if (auto pElement = pInfo->FindChildOfType(xiiOpenDdlPrimitiveType::String, "RuntimePlugins"))
  {
    m_RuntimePlugins.SetCount(pElement->GetNumPrimitives());
    for (xiiUInt32 i = 0; i < pElement->GetNumPrimitives(); ++i)
      m_RuntimePlugins[i] = pElement->GetPrimitivesString()[i];
  }

  if (auto pElement = pInfo->FindChildOfType(xiiOpenDdlPrimitiveType::String, "PackageDependencies"))
  {
    m_PackageDependencies.SetCount(pElement->GetNumPrimitives());
    for (xiiUInt32 i = 0; i < pElement->GetNumPrimitives(); ++i)
      m_PackageDependencies[i] = pElement->GetPrimitivesString()[i];
  }

  if (auto pElement = pInfo->FindChildOfType(xiiOpenDdlPrimitiveType::String, "RequiredPlugins"))
  {
    m_RequiredBundles.SetCount(pElement->GetNumPrimitives());
    for (xiiUInt32 i = 0; i < pElement->GetNumPrimitives(); ++i)
      m_RequiredBundles[i] = pElement->GetPrimitivesString()[i];
  }

  if (auto pElement = pInfo->FindChildOfType(xiiOpenDdlPrimitiveType::String, "ExclusiveFeatures"))
  {
    m_ExclusiveFeatures.SetCount(pElement->GetNumPrimitives());
    for (xiiUInt32 i = 0; i < pElement->GetNumPrimitives(); ++i)
      m_ExclusiveFeatures[i] = pElement->GetPrimitivesString()[i];
  }

  if (auto pElement = pInfo->FindChildOfType(xiiOpenDdlPrimitiveType::String, "EnabledInTemplates"))
  {
    m_EnabledInTemplates.SetCount(pElement->GetNumPrimitives());
    for (xiiUInt32 i = 0; i < pElement->GetNumPrimitives(); ++i)
      m_EnabledInTemplates[i] = pElement->GetPrimitivesString()[i];
  }

  m_bMissing = false;

  return XII_SUCCESS;
}

void xiiQtEditorApp::DetectAvailablePluginBundles(xiiStringView sSearchDirectory)
{
#if XII_ENABLED(XII_SUPPORTS_FILE_ITERATORS)
  // find all xiiPluginBundle files
  {
    xiiStringBuilder sSearch = sSearchDirectory;

    sSearch.AppendPath("*.xiiPluginBundle");

    xiiStringBuilder sPath, sPlugin;

    xiiFileSystemIterator fsit;
    for (fsit.StartSearch(sSearch.GetData(), xiiFileSystemIteratorFlags::ReportFiles); fsit.IsValid(); fsit.Next())
    {
      sPlugin = fsit.GetStats().m_sName;
      sPlugin.RemoveFileExtension();

      fsit.GetStats().GetFullPath(sPath);

      xiiFileReader file;
      if (file.Open(sPath).Succeeded())
      {
        xiiOpenDdlReader ddl;
        if (ddl.ParseDocument(file).Failed())
        {
          xiiLog::Error("Failed to parse plugin bundle file: '{}'", sPath);
        }
        else
        {
          m_PluginBundles.m_Plugins[sPlugin].ReadBundleFromDDL(ddl).IgnoreResult();
        }
      }
    }
  }

  // additionally, find all *Plugin.dll files that are not mentioned in any xiiPluginBundle and treat them as fake plugin bundles
  if (false) // sometimes useful, but not how it's supposed to be
  {
    xiiStringBuilder sSearch = xiiOSFile::GetApplicationDirectory();

    sSearch.AppendPath("*Plugin.dll");

    xiiStringBuilder sPlugin;

    auto isUsedInBundle = [this](const xiiStringBuilder& sPlugin) -> bool {
      for (auto pit : m_PluginBundles.m_Plugins)
      {
        if (pit.Key().IsEqual_NoCase(sPlugin))
          return true;

        const xiiPluginBundle& val = pit.Value();

        for (const auto& rt : val.m_RuntimePlugins)
        {
          if (rt.IsEqual_NoCase(sPlugin))
            return true;
        }
      }

      return false;
    };

    xiiFileSystemIterator fsit;
    for (fsit.StartSearch(sSearch.GetData(), xiiFileSystemIteratorFlags::ReportFiles); fsit.IsValid(); fsit.Next())
    {
      sPlugin = fsit.GetStats().m_sName;
      sPlugin.RemoveFileExtension();

      if (isUsedInBundle(sPlugin))
        continue;

      auto& newp = m_PluginBundles.m_Plugins[sPlugin];
      newp.m_RuntimePlugins.PushBack(sPlugin);
      newp.m_sDescription = "No xiiPluginBundle file is present for this plugin.";

      sPlugin.Shrink(0, 6);
      newp.m_sDisplayName = sPlugin;
    }
  }
#else
  XII_ASSERT_NOT_IMPLEMENTED;
#endif
}

void xiiQtEditorApp::LoadEditorPlugins()
{
  XII_PROFILE_SCOPE("LoadEditorPlugins");
  DetectAvailablePluginBundles(xiiOSFile::GetApplicationDirectory());

  xiiPlugin::InitializeStaticallyLinkedPlugins();
}

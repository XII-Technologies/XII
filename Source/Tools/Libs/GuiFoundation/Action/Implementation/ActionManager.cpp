#include <GuiFoundation/GuiFoundationPCH.h>

#include <Foundation/Configuration/Startup.h>
#include <Foundation/IO/FileSystem/DeferredFileWriter.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/IO/OpenDdlReader.h>
#include <Foundation/IO/OpenDdlWriter.h>
#include <Foundation/Logging/Log.h>
#include <GuiFoundation/Action/ActionManager.h>
#include <GuiFoundation/Action/CommandHistoryActions.h>
#include <GuiFoundation/Action/DocumentActions.h>
#include <GuiFoundation/Action/EditActions.h>
#include <GuiFoundation/Action/StandardMenus.h>
#include <ToolsFoundation/Application/ApplicationServices.h>

// clang-format off
XII_BEGIN_SUBSYSTEM_DECLARATION(GuiFoundation, ActionManager)

  BEGIN_SUBSYSTEM_DEPENDENCIES
  "ToolsFoundation"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    xiiActionManager::Startup();
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    xiiActionManager::Shutdown();
  }

XII_END_SUBSYSTEM_DECLARATION;
// clang-format on

xiiEvent<const xiiActionManager::Event&>          xiiActionManager::s_Events;
xiiIdTable<xiiActionId, xiiActionDescriptor*>     xiiActionManager::s_ActionTable;
xiiMap<xiiString, xiiActionManager::CategoryData> xiiActionManager::s_CategoryPathToActions;
xiiMap<xiiString, xiiString>                      xiiActionManager::s_ShortcutOverride;

////////////////////////////////////////////////////////////////////////
// xiiActionManager public functions
////////////////////////////////////////////////////////////////////////

xiiActionDescriptorHandle xiiActionManager::RegisterAction(const xiiActionDescriptor& desc)
{
  xiiActionDescriptorHandle hType = GetActionHandle(desc.m_sCategoryPath, desc.m_sActionName);
  XII_ASSERT_DEV(hType.IsInvalidated(), "The action '{0}' in category '{1}' was already registered!", desc.m_sActionName, desc.m_sCategoryPath);

  xiiActionDescriptor* pDesc = CreateActionDesc(desc);

  // apply shortcut override
  {
    auto ovride = s_ShortcutOverride.Find(desc.m_sActionName);
    if (ovride.IsValid())
      pDesc->m_sShortcut = ovride.Value();
  }

  hType           = xiiActionDescriptorHandle(s_ActionTable.Insert(pDesc));
  pDesc->m_Handle = hType;

  auto it = s_CategoryPathToActions.FindOrAdd(pDesc->m_sCategoryPath);
  it.Value().m_Actions.Insert(hType);
  it.Value().m_ActionNameToHandle[pDesc->m_sActionName] = hType;

  {
    Event msg;
    msg.m_Type   = Event::Type::ActionAdded;
    msg.m_pDesc  = pDesc;
    msg.m_Handle = hType;
    s_Events.Broadcast(msg);
  }
  return hType;
}

bool xiiActionManager::UnregisterAction(xiiActionDescriptorHandle& ref_hAction)
{
  xiiActionDescriptor* pDesc = nullptr;
  if (!s_ActionTable.TryGetValue(ref_hAction, pDesc))
  {
    ref_hAction.Invalidate();
    return false;
  }

  auto it = s_CategoryPathToActions.Find(pDesc->m_sCategoryPath);
  XII_ASSERT_DEV(it.IsValid(), "Action is present but not mapped in its category path!");
  XII_VERIFY(it.Value().m_Actions.Remove(ref_hAction), "Action is present but not in its category data!");
  XII_VERIFY(it.Value().m_ActionNameToHandle.Remove(pDesc->m_sActionName), "Action is present but its name is not in the map!");
  if (it.Value().m_Actions.IsEmpty())
  {
    s_CategoryPathToActions.Remove(it);
  }

  s_ActionTable.Remove(ref_hAction);
  DeleteActionDesc(pDesc);
  ref_hAction.Invalidate();
  return true;
}

const xiiActionDescriptor* xiiActionManager::GetActionDescriptor(xiiActionDescriptorHandle hAction)
{
  xiiActionDescriptor* pDesc = nullptr;
  if (s_ActionTable.TryGetValue(hAction, pDesc))
    return pDesc;

  return nullptr;
}

const xiiIdTable<xiiActionId, xiiActionDescriptor*>::ConstIterator xiiActionManager::GetActionIterator()
{
  return s_ActionTable.GetIterator();
}

xiiActionDescriptorHandle xiiActionManager::GetActionHandle(xiiStringView sCategoryPath, xiiStringView sActionName)
{
  xiiActionDescriptorHandle hAction;
  auto                      it = s_CategoryPathToActions.Find(sCategoryPath);
  if (!it.IsValid())
    return hAction;

  it.Value().m_ActionNameToHandle.TryGetValue(sActionName, hAction);

  return hAction;
}

xiiString xiiActionManager::FindActionCategory(xiiStringView sActionName)
{
  for (auto itCat : s_CategoryPathToActions)
  {
    if (itCat.Value().m_ActionNameToHandle.Contains(sActionName))
      return itCat.Key();
  }

  return xiiString();
}

xiiResult xiiActionManager::ExecuteAction(xiiStringView sCategory, xiiStringView sActionName, const xiiActionContext& context, const xiiVariant& value /*= xiiVariant()*/)
{
  if (sCategory.IsEmpty())
  {
    sCategory = FindActionCategory(sActionName);
  }

  auto hAction = xiiActionManager::GetActionHandle(sCategory, sActionName);

  if (hAction.IsInvalidated())
    return XII_FAILURE;

  const xiiActionDescriptor* pDesc = xiiActionManager::GetActionDescriptor(hAction);

  if (pDesc == nullptr)
    return XII_FAILURE;

  xiiAction* pAction = pDesc->CreateAction(context);

  if (pAction == nullptr)
    return XII_FAILURE;

  pAction->Execute(value);
  pDesc->DeleteAction(pAction);

  return XII_SUCCESS;
}

void xiiActionManager::SaveShortcutAssignment()
{
  xiiStringBuilder sFile = xiiApplicationServices::GetSingleton()->GetApplicationPreferencesFolder();
  sFile.AppendPath("Settings/Shortcuts.ddl");

  XII_LOG_BLOCK("LoadShortcutAssignment", sFile.GetData());

  xiiDeferredFileWriter file;
  file.SetOutput(sFile);

  xiiOpenDdlWriter writer;
  writer.SetOutputStream(&file);
  writer.SetCompactMode(false);
  writer.SetPrimitiveTypeStringMode(xiiOpenDdlWriter::TypeStringMode::Compliant);

  xiiStringBuilder sKey;

  for (auto it = GetActionIterator(); it.IsValid(); ++it)
  {
    auto pAction = it.Value();

    if (pAction->m_Type != xiiActionType::Action)
      continue;

    if (pAction->m_sShortcut == pAction->m_sDefaultShortcut)
      sKey.Set("default: ", pAction->m_sShortcut);
    else
      sKey = pAction->m_sShortcut;

    writer.BeginPrimitiveList(xiiOpenDdlPrimitiveType::String, pAction->m_sActionName);
    writer.WriteString(sKey);
    writer.EndPrimitiveList();
  }

  if (file.Close().Failed())
  {
    xiiLog::Error("Failed to write shortcuts config file '{0}'", sFile);
  }
}

void xiiActionManager::LoadShortcutAssignment()
{
  xiiStringBuilder sFile = xiiApplicationServices::GetSingleton()->GetApplicationPreferencesFolder();
  sFile.AppendPath("Settings/Shortcuts.ddl");

  XII_LOG_BLOCK("LoadShortcutAssignment", sFile.GetData());

  xiiFileReader file;
  if (file.Open(sFile).Failed())
  {
    xiiLog::Dev("No shortcuts file '{0}' was found", sFile);
    return;
  }

  xiiOpenDdlReader reader;
  if (reader.ParseDocument(file, 0, xiiLog::GetThreadLocalLogSystem()).Failed())
    return;

  const auto obj = reader.GetRootElement();

  xiiStringBuilder sKey, sValue;

  for (auto pElement = obj->GetFirstChild(); pElement != nullptr; pElement = pElement->GetSibling())
  {
    if (!pElement->HasName() || !pElement->HasPrimitives(xiiOpenDdlPrimitiveType::String))
      continue;

    sKey   = pElement->GetName();
    sValue = pElement->GetPrimitivesString()[0];

    if (sValue.FindSubString_NoCase("default") != nullptr)
      continue;

    s_ShortcutOverride[sKey] = sValue;
  }

  // apply overrides
  for (auto it = GetActionIterator(); it.IsValid(); ++it)
  {
    auto pAction = it.Value();

    if (pAction->m_Type != xiiActionType::Action)
      continue;

    auto ovride = s_ShortcutOverride.Find(pAction->m_sActionName);
    if (ovride.IsValid())
      pAction->m_sShortcut = ovride.Value();
  }
}

////////////////////////////////////////////////////////////////////////
// xiiActionManager private functions
////////////////////////////////////////////////////////////////////////

void xiiActionManager::Startup()
{
  xiiDocumentActions::RegisterActions();
  xiiStandardMenus::RegisterActions();
  xiiCommandHistoryActions::RegisterActions();
  xiiEditActions::RegisterActions();
}

void xiiActionManager::Shutdown()
{
  xiiDocumentActions::UnregisterActions();
  xiiStandardMenus::UnregisterActions();
  xiiCommandHistoryActions::UnregisterActions();
  xiiEditActions::UnregisterActions();

  XII_ASSERT_DEV(s_ActionTable.IsEmpty(), "Some actions were registered but not unregistred.");
  XII_ASSERT_DEV(s_CategoryPathToActions.IsEmpty(), "Some actions were registered but not unregistred.");

  s_ActionTable.Clear();
  s_CategoryPathToActions.Clear();
  s_ShortcutOverride.Clear();
}

xiiActionDescriptor* xiiActionManager::CreateActionDesc(const xiiActionDescriptor& desc)
{
  xiiActionDescriptor* pDesc = XII_DEFAULT_NEW(xiiActionDescriptor);
  *pDesc                     = desc;
  return pDesc;
}

void xiiActionManager::DeleteActionDesc(xiiActionDescriptor* pDesc)
{
  XII_DEFAULT_DELETE(pDesc);
}

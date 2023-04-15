#include <EditorPluginProcGen/EditorPluginProcGenPCH.h>

#include <EditorPluginProcGen/Actions/ProcGenActions.h>
#include <EditorPluginProcGen/ProcGenGraphAsset/ProcGenGraphAsset.h>
#include <GuiFoundation/Action/ActionManager.h>

xiiActionDescriptorHandle xiiProcGenActions::s_hCategory;
xiiActionDescriptorHandle xiiProcGenActions::s_hDumpAST;
xiiActionDescriptorHandle xiiProcGenActions::s_hDumpDisassembly;

void xiiProcGenActions::RegisterActions()
{
  s_hCategory        = XII_REGISTER_CATEGORY("ProcGen");
  s_hDumpAST         = XII_REGISTER_ACTION_1("ProcGen.DumpAST", xiiActionScope::Document, "ProcGen Graph", "", xiiProcGenAction, xiiProcGenAction::ActionType::DumpAST);
  s_hDumpDisassembly = XII_REGISTER_ACTION_1("ProcGen.DumpDisassembly", xiiActionScope::Document, "ProcGen Graph", "", xiiProcGenAction, xiiProcGenAction::ActionType::DumpDisassembly);
}

void xiiProcGenActions::UnregisterActions()
{
  xiiActionManager::UnregisterAction(s_hCategory);
  xiiActionManager::UnregisterAction(s_hDumpAST);
  xiiActionManager::UnregisterAction(s_hDumpDisassembly);
}

void xiiProcGenActions::MapMenuActions()
{
  xiiActionMap* pMap = xiiActionMapManager::GetActionMap("ProcGenAssetMenuBar");
  XII_ASSERT_DEV(pMap != nullptr, "Mapping the actions failed!");

  pMap->MapAction(s_hCategory, "Menu.Tools", 9.0f);
  pMap->MapAction(s_hDumpAST, "Menu.Tools", 10.0f);
  pMap->MapAction(s_hDumpDisassembly, "Menu.Tools", 11.0f);

  pMap = xiiActionMapManager::GetActionMap("ProcGenAssetToolBar");
  XII_ASSERT_DEV(pMap != nullptr, "Mapping the actions failed!");

  pMap->MapAction(s_hCategory, "", 12.0f);
  pMap->MapAction(s_hDumpAST, "ProcGen", 0.0f);
  pMap->MapAction(s_hDumpDisassembly, "ProcGen", 0.0f);
}

//////////////////////////////////////////////////////////////////////////

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiProcGenAction, 0, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiProcGenAction::xiiProcGenAction(const xiiActionContext& context, const char* szName, ActionType type) :
  xiiButtonAction(context, szName, false, ""), m_Type(type)
{
}

xiiProcGenAction::~xiiProcGenAction() {}

void xiiProcGenAction::Execute(const xiiVariant& value)
{
  if (auto pAssetDocument = xiiDynamicCast<xiiProcGenGraphAssetDocument*>(GetContext().m_pDocument))
  {
    pAssetDocument->DumpSelectedOutput(m_Type == ActionType::DumpAST, m_Type == ActionType::DumpDisassembly);
  }
}

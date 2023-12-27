#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorPluginAssets/MaterialAsset/MaterialAsset.h>
#include <EditorPluginAssets/VisualShader/VisualShaderActions.h>
#include <GuiFoundation/Action/ActionMapManager.h>

xiiActionDescriptorHandle xiiVisualShaderActions::s_hCleanGraph;

void xiiVisualShaderActions::RegisterActions()
{
  s_hCleanGraph = XII_REGISTER_ACTION_0("VisualShader.CleanGraph", xiiActionScope::Document, "Visual Shader", "", xiiVisualShaderAction);
}

void xiiVisualShaderActions::UnregisterActions()
{
  xiiActionManager::UnregisterAction(s_hCleanGraph);
}

void xiiVisualShaderActions::MapActions(xiiStringView sMapping)
{
  xiiActionMap* pMap = xiiActionMapManager::GetActionMap(sMapping);
  XII_ASSERT_DEV(pMap != nullptr, "The given mapping ('{0}') does not exist, mapping the actions failed!", sMapping);

  pMap->MapAction(s_hCleanGraph, "", 30.0f);
}


XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiVisualShaderAction, 0, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiVisualShaderAction::xiiVisualShaderAction(const xiiActionContext& context, const char* szName) :
  xiiButtonAction(context, szName, false, "")
{
  SetIconPath(":/EditorPluginAssets/Cleanup.svg");
}

xiiVisualShaderAction::~xiiVisualShaderAction() = default;

void xiiVisualShaderAction::Execute(const xiiVariant& value)
{
  xiiMaterialAssetDocument* pMaterial = (xiiMaterialAssetDocument*)(m_Context.m_pDocument);

  pMaterial->RemoveDisconnectedNodes();
}

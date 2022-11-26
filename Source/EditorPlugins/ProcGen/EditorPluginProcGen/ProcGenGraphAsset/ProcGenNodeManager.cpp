#include <EditorPluginProcGen/EditorPluginProcGenPCH.h>

#include <EditorPluginProcGen/ProcGenGraphAsset/ProcGenGraphQt.h>
#include <EditorPluginProcGen/ProcGenGraphAsset/ProcGenNodeManager.h>
#include <EditorPluginProcGen/ProcGenGraphAsset/ProcGenNodes.h>
#include <Foundation/Configuration/Startup.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiProcGenPin, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_SUBSYSTEM_DECLARATION(EditorPluginProcGen, ProcGen)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "ReflectedTypeManager"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    const xiiRTTI* pBaseType = xiiGetStaticRTTI<xiiProcGenNodeBase>();

    xiiQtNodeScene::GetPinFactory().RegisterCreator(xiiGetStaticRTTI<xiiProcGenPin>(), [](const xiiRTTI* pRtti)->xiiQtPin* { return new xiiQtProcGenPin(); });
    xiiQtNodeScene::GetNodeFactory().RegisterCreator(pBaseType, [](const xiiRTTI* pRtti)->xiiQtNode* { return new xiiQtProcGenNode(); });
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    const xiiRTTI* pBaseType = xiiGetStaticRTTI<xiiProcGenNodeBase>();

    xiiQtNodeScene::GetPinFactory().UnregisterCreator(xiiGetStaticRTTI<xiiProcGenPin>());
    xiiQtNodeScene::GetNodeFactory().UnregisterCreator(pBaseType);
  }

XII_END_SUBSYSTEM_DECLARATION;
// clang-format on

//////////////////////////////////////////////////////////////////////////

bool xiiProcGenNodeManager::InternalIsNode(const xiiDocumentObject* pObject) const
{
  return pObject->GetType()->IsDerivedFrom(xiiGetStaticRTTI<xiiProcGenNodeBase>());
}

void xiiProcGenNodeManager::InternalCreatePins(const xiiDocumentObject* pObject, NodeInternal& node)
{
  const xiiRTTI* pNodeBaseType = xiiGetStaticRTTI<xiiProcGenNodeBase>();

  auto pType = pObject->GetTypeAccessor().GetType();
  if (!pType->IsDerivedFrom(pNodeBaseType))
    return;

  xiiHybridArray<xiiAbstractProperty*, 32> properties;
  pType->GetAllProperties(properties);

  for (xiiAbstractProperty* pProp : properties)
  {
    if (pProp->GetCategory() != xiiPropertyCategory::Member)
      continue;

    const xiiRTTI* pPropType = pProp->GetSpecificType();
    if (!pPropType->IsDerivedFrom<xiiRenderPipelineNodePin>())
      continue;

    xiiColor pinColor = xiiColorScheme::DarkUI(xiiColorScheme::Gray);
    if (const xiiColorAttribute* pAttr = pProp->GetAttributeByType<xiiColorAttribute>())
    {
      pinColor = pAttr->GetColor();
    }

    if (pPropType->IsDerivedFrom<xiiRenderPipelineNodeInputPin>())
    {
      auto pPin = XII_DEFAULT_NEW(xiiProcGenPin, xiiPin::Type::Input, pProp->GetPropertyName(), pinColor, pObject);
      node.m_Inputs.PushBack(pPin);
    }
    else if (pPropType->IsDerivedFrom<xiiRenderPipelineNodeOutputPin>())
    {
      auto pPin = XII_DEFAULT_NEW(xiiProcGenPin, xiiPin::Type::Output, pProp->GetPropertyName(), pinColor, pObject);
      node.m_Outputs.PushBack(pPin);
    }
  }
}

void xiiProcGenNodeManager::GetCreateableTypes(xiiHybridArray<const xiiRTTI*, 32>& Types) const
{
  const xiiRTTI* pNodeBaseType = xiiGetStaticRTTI<xiiProcGenNodeBase>();

  for (auto it = xiiRTTI::GetFirstInstance(); it != nullptr; it = it->GetNextInstance())
  {
    if (it->IsDerivedFrom(pNodeBaseType) && !it->GetTypeFlags().IsSet(xiiTypeFlags::Abstract))
      Types.PushBack(it);
  }
}

const char* xiiProcGenNodeManager::GetTypeCategory(const xiiRTTI* pRtti) const
{
  if (const xiiCategoryAttribute* pAttr = pRtti->GetAttributeByType<xiiCategoryAttribute>())
  {
    return pAttr->GetCategory();
  }

  return nullptr;
}

xiiStatus xiiProcGenNodeManager::InternalCanConnect(const xiiPin& source, const xiiPin& target, CanConnectResult& out_Result) const
{
  out_Result = CanConnectResult::ConnectNto1;
  return xiiStatus(XII_SUCCESS);
}

#include <EditorPluginScene/EditorPluginScenePCH.h>

#include "Foundation/Serialization/GraphPatch.h"
#include <Core/World/GameObject.h>
#include <EditorPluginScene/Objects/SceneObjectManager.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiSceneDocumentSettingsBase, 1, xiiRTTINoAllocator)
{
}
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiPrefabDocumentSettings, 1, xiiRTTIDefaultAllocator<xiiPrefabDocumentSettings>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ARRAY_MEMBER_PROPERTY("ExposedProperties", m_ExposedProperties),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiLayerDocumentSettings, 1, xiiRTTIDefaultAllocator<xiiLayerDocumentSettings>)
{
}
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiSceneDocumentRoot, 2, xiiRTTINoAllocator)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Settings", m_pSettings)->AddFlags(xiiPropertyFlags::PointerOwner),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

//////////////////////////////////////////////////////////////////////////

xiiSceneObjectManager::xiiSceneObjectManager() :
  xiiDocumentObjectManager(xiiGetStaticRTTI<xiiSceneDocumentRoot>())
{
}

void xiiSceneObjectManager::GetCreateableTypes(xiiHybridArray<const xiiRTTI*, 32>& Types) const
{
  Types.PushBack(xiiRTTI::FindTypeByName(xiiGetStaticRTTI<xiiGameObject>()->GetTypeName()));

  const xiiRTTI* pComponentType = xiiRTTI::FindTypeByName(xiiGetStaticRTTI<xiiComponent>()->GetTypeName());

  for (auto it = xiiRTTI::GetFirstInstance(); it != nullptr; it = it->GetNextInstance())
  {
    if (it->IsDerivedFrom(pComponentType) && !it->GetTypeFlags().IsSet(xiiTypeFlags::Abstract))
      Types.PushBack(it);
  }
}

xiiStatus xiiSceneObjectManager::InternalCanAdd(
  const xiiRTTI*           pRtti,
  const xiiDocumentObject* pParent,
  const char*              szParentProperty,
  const xiiVariant&        index) const
{
  if (IsUnderRootProperty("Children", pParent, szParentProperty))
  {
    if (pParent == nullptr)
    {
      bool bIsDerived = pRtti->IsDerivedFrom<xiiGameObject>();
      if (!bIsDerived)
      {
        return xiiStatus("Only xiiGameObject can be added to the root of the world!");
      }
    }
    else
    {
      // only prevent adding game objects (as children) to objects that already have a prefab component
      // do allow to attach components to objects with prefab components
      // if (pRtti->IsDerivedFrom<xiiGameObject>())
      //{
      //  if (pParent->GetTypeAccessor().GetType()->IsDerivedFrom<xiiGameObject>())
      //  {
      //    auto children = pParent->GetChildren();
      //    for (auto pChild : children)
      //    {
      //      if (pChild->GetType()->IsDerivedFrom<xiiPrefabReferenceComponent>())
      //        return xiiStatus("Cannot add objects to a prefab node.");
      //    }
      //  }
      //}

      // in case prefab component should be the only component on a node
      // if (pRtti->IsDerivedFrom<xiiPrefabReferenceComponent>())
      //{
      //  if (!pParent->GetChildren().IsEmpty())
      //    return xiiStatus("Prefab components can only be added to empty nodes.");
      //}
    }
  }
  return xiiStatus(XII_SUCCESS);
}

xiiStatus xiiSceneObjectManager::InternalCanMove(
  const xiiDocumentObject* pObject,
  const xiiDocumentObject* pNewParent,
  const char*              szParentProperty,
  const xiiVariant&        index) const
{
  // code to disallow attaching nodes to a prefab node
  // if (pNewParent != nullptr)
  //{
  //  if (pNewParent->GetTypeAccessor().GetType()->IsDerivedFrom<xiiGameObject>())
  //  {
  //    auto children = pNewParent->GetChildren();
  //    for (auto pChild : children)
  //    {
  //      if (pChild->GetType()->IsDerivedFrom<xiiPrefabReferenceComponent>())
  //        return xiiStatus("Cannot move objects into a prefab node.");
  //    }
  //  }
  //}

  return xiiStatus(XII_SUCCESS);
}

xiiStatus xiiSceneObjectManager::InternalCanSelect(const xiiDocumentObject* pObject) const
{
  /*if (pObject->GetTypeAccessor().GetType() != xiiGetStaticRTTI<xiiGameObject>())
  {
    return xiiStatus(
      xiiFmt("Object of type '{0}' is not a 'xiiGameObject' and can't be selected.", pObject->GetTypeAccessor().GetType()->GetTypeName()));
  }*/
  return xiiStatus(XII_SUCCESS);
}

namespace
{
  /// Patch class
  class xiiSceneDocumentSettings_1_2 : public xiiGraphPatch
  {
  public:
    xiiSceneDocumentSettings_1_2() :
      xiiGraphPatch("xiiSceneDocumentSettings", 2)
    {
    }
    virtual void Patch(xiiGraphPatchContext& context, xiiAbstractObjectGraph* pGraph, xiiAbstractObjectNode* pNode) const override
    {
      // Previously, xiiSceneDocumentSettings only contained prefab settings. As these only apply to prefab documents, we switch the old version to prefab.
      context.RenameClass("xiiPrefabDocumentSettings", 1);
      xiiVersionKey bases[] = {{"xiiSceneDocumentSettingsBase", 1}, {"xiiReflectedClass", 1}};
      context.ChangeBaseClass(bases);
    }
  };
  xiiSceneDocumentSettings_1_2 g_xiiSceneDocumentSettings_1_2;
} // namespace

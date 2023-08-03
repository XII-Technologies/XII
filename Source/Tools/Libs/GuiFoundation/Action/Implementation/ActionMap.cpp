#include <GuiFoundation/GuiFoundationPCH.h>

#include <Foundation/Logging/Log.h>
#include <GuiFoundation/Action/ActionManager.h>
#include <GuiFoundation/Action/ActionMap.h>
#include <ToolsFoundation/Reflection/PhantomRttiManager.h>

// clang-format off
XII_BEGIN_STATIC_REFLECTED_TYPE(xiiActionMapDescriptor, xiiNoBase, 0, xiiRTTINoAllocator);
//  XII_BEGIN_PROPERTIES
//  XII_END_PROPERTIES;
XII_END_STATIC_REFLECTED_TYPE;
// clang-format on

////////////////////////////////////////////////////////////////////////
// xiiActionMap public functions
////////////////////////////////////////////////////////////////////////

xiiActionMap::xiiActionMap() = default;

xiiActionMap::~xiiActionMap() = default;

void xiiActionMap::MapAction(xiiActionDescriptorHandle hAction, xiiStringView sPath, xiiStringView sSubPath, float fOrder)
{
  xiiStringBuilder sFullPath = sPath;

  if (!sPath.IsEmpty() && sPath.FindSubString("/") == nullptr)
  {
    if (SearchPathForAction(sPath, sFullPath).Failed())
    {
      sFullPath = sPath;
    }
  }

  sFullPath.AppendPath(sSubPath);

  MapAction(hAction, sFullPath, fOrder);
}

void xiiActionMap::MapAction(xiiActionDescriptorHandle hAction, xiiStringView sPath, float fOrder)
{
  xiiStringBuilder sCleanPath = sPath;
  sCleanPath.MakeCleanPath();
  sCleanPath.Trim("/");
  xiiActionMapDescriptor d;
  d.m_hAction = hAction;
  d.m_sPath   = sCleanPath;
  d.m_fOrder  = fOrder;

  if (!d.m_sPath.IsEmpty() && d.m_sPath.FindSubString("/") == nullptr)
  {
    xiiStringBuilder sFullPath;
    if (SearchPathForAction(d.m_sPath, sFullPath).Succeeded())
    {
      d.m_sPath = sFullPath;
    }
  }

  XII_VERIFY(MapAction(d).IsValid(), "Mapping Failed");
}

xiiUuid xiiActionMap::MapAction(const xiiActionMapDescriptor& desc)
{
  xiiUuid ParentGUID;
  if (!FindObjectByPath(desc.m_sPath, ParentGUID))
  {
    return xiiUuid();
  }

  auto it = m_Descriptors.Find(ParentGUID);

  xiiTreeNode<xiiActionMapDescriptor>* pParent = nullptr;
  if (it.IsValid())
  {
    pParent = it.Value();
  }

  if (desc.m_sPath.IsEmpty())
  {
    pParent = &m_Root;
  }
  else
  {
    const xiiActionMapDescriptor* pDesc = GetDescriptor(pParent);
    if (pDesc->m_hAction.GetDescriptor()->m_Type == xiiActionType::Action)
    {
      xiiLog::Error("Can't map descriptor '{0}' as its parent is an action itself and thus can't have any children.",
                    desc.m_hAction.GetDescriptor()->m_sActionName);
      return xiiUuid();
    }
  }

  if (GetChildByName(pParent, desc.m_hAction.GetDescriptor()->m_sActionName) != nullptr)
  {
    xiiLog::Error("Can't map descriptor as its name is already present: {0}", desc.m_hAction.GetDescriptor()->m_sActionName);
    return xiiUuid();
  }

  xiiInt32 iIndex = 0;
  for (iIndex = 0; iIndex < (xiiInt32)pParent->GetChildren().GetCount(); ++iIndex)
  {
    const xiiTreeNode<xiiActionMapDescriptor>* pChild = pParent->GetChildren()[iIndex];
    const xiiActionMapDescriptor*              pDesc  = GetDescriptor(pChild);

    if (desc.m_fOrder < pDesc->m_fOrder)
      break;
  }

  xiiTreeNode<xiiActionMapDescriptor>* pChild = pParent->InsertChild(desc, iIndex);

  m_Descriptors.Insert(pChild->GetGuid(), pChild);

  return pChild->GetGuid();
}

xiiResult xiiActionMap::UnmapAction(const xiiUuid& guid)
{
  auto it = m_Descriptors.Find(guid);
  if (!it.IsValid())
    return XII_FAILURE;

  xiiTreeNode<xiiActionMapDescriptor>* pNode = it.Value();
  if (xiiTreeNode<xiiActionMapDescriptor>* pParent = pNode->GetParent())
  {
    pParent->RemoveChild(pNode->GetParentIndex());
  }
  m_Descriptors.Remove(it);
  return XII_SUCCESS;
}

xiiResult xiiActionMap::UnmapAction(xiiActionDescriptorHandle hAction, xiiStringView sPath)
{
  xiiStringBuilder sCleanPath = sPath;
  sCleanPath.MakeCleanPath();
  sCleanPath.Trim("/");

  xiiActionMapDescriptor d;
  d.m_hAction = hAction;
  d.m_sPath   = sCleanPath;
  d.m_fOrder  = 0.0f; // unused.

  if (!d.m_sPath.IsEmpty() && d.m_sPath.FindSubString("/") == nullptr)
  {
    xiiStringBuilder sFullPath;
    if (SearchPathForAction(d.m_sPath, sFullPath).Succeeded())
    {
      d.m_sPath = sFullPath;
    }
  }

  return UnmapAction(d);
}

xiiResult xiiActionMap::UnmapAction(const xiiActionMapDescriptor& desc)
{
  xiiTreeNode<xiiActionMapDescriptor>* pParent = nullptr;
  if (desc.m_sPath.IsEmpty())
  {
    pParent = &m_Root;
  }
  else
  {
    xiiUuid ParentGUID;
    if (!FindObjectByPath(desc.m_sPath, ParentGUID))
      return XII_FAILURE;

    auto it = m_Descriptors.Find(ParentGUID);
    if (!it.IsValid())
      return XII_FAILURE;

    pParent = it.Value();
  }

  if (auto* pChild = GetChildByName(pParent, desc.m_hAction.GetDescriptor()->m_sActionName))
  {
    return UnmapAction(pChild->GetGuid());
  }
  return XII_FAILURE;
}

bool xiiActionMap::FindObjectByPath(xiiStringView sPath, xiiUuid& out_guid) const
{
  out_guid = xiiUuid();
  if (sPath.IsEmpty())
    return true;

  xiiStringBuilder                 sPathBuilder(sPath);
  xiiHybridArray<xiiStringView, 8> parts;
  sPathBuilder.Split(false, parts, "/");

  const xiiTreeNode<xiiActionMapDescriptor>* pParent = &m_Root;
  for (const xiiStringView& name : parts)
  {
    pParent = GetChildByName(pParent, name);
    if (pParent == nullptr)
      return false;
  }

  out_guid = pParent->GetGuid();
  return true;
}

xiiResult xiiActionMap::SearchPathForAction(xiiStringView sUniqueName, xiiStringBuilder& out_sPath) const
{
  out_sPath.Clear();

  if (FindObjectPathByName(&m_Root, sUniqueName, out_sPath))
  {
    return XII_SUCCESS;
  }

  return XII_FAILURE;
}

bool xiiActionMap::FindObjectPathByName(const xiiTreeNode<xiiActionMapDescriptor>* pObject, xiiStringView sName, xiiStringBuilder& out_sPath) const
{
  xiiStringView sObjectName;

  if (!pObject->m_Data.m_hAction.IsInvalidated())
  {
    sObjectName = pObject->m_Data.m_hAction.GetDescriptor()->m_sActionName;
  }

  out_sPath.AppendPath(sObjectName);

  if (sObjectName == sName)
    return true;

  for (const xiiTreeNode<xiiActionMapDescriptor>* pChild : pObject->GetChildren())
  {
    const xiiActionMapDescriptor& pDesc = pChild->m_Data;

    if (FindObjectPathByName(pChild, sName, out_sPath))
      return true;
  }

  out_sPath.PathParentDirectory();
  return false;
}

const xiiActionMapDescriptor* xiiActionMap::GetDescriptor(const xiiUuid& guid) const
{
  auto it = m_Descriptors.Find(guid);
  if (!it.IsValid())
    return nullptr;
  return GetDescriptor(it.Value());
}

const xiiActionMapDescriptor* xiiActionMap::GetDescriptor(const xiiTreeNode<xiiActionMapDescriptor>* pObject) const
{
  if (pObject == nullptr)
    return nullptr;

  return &pObject->m_Data;
}

const xiiTreeNode<xiiActionMapDescriptor>* xiiActionMap::GetChildByName(const xiiTreeNode<xiiActionMapDescriptor>* pObject, xiiStringView sName) const
{
  for (const xiiTreeNode<xiiActionMapDescriptor>* pChild : pObject->GetChildren())
  {
    const xiiActionMapDescriptor& pDesc = pChild->m_Data;
    if (sName.IsEqual_NoCase(pDesc.m_hAction.GetDescriptor()->m_sActionName.GetData()))
    {
      return pChild;
    }
  }
  return nullptr;
}

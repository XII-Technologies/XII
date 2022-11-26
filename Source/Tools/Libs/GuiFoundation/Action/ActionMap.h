#pragma once

#include <GuiFoundation/Action/Action.h>
#include <GuiFoundation/GuiFoundationDLL.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>

class xiiDocument;

struct XII_GUIFOUNDATION_DLL xiiActionMapDescriptor
{
  xiiActionDescriptorHandle m_hAction; ///< Action to be mapped
  xiiString
    m_sPath;      ///< Path where the action should be mapped excluding the action's name, e.g. "File/New" for a menu item "File -> New -> Project..." .
  float m_fOrder; ///< Ordering key to sort actions in the mapping path.
};
XII_DECLARE_REFLECTABLE_TYPE(XII_NO_LINKAGE, xiiActionMapDescriptor);

template <typename T>
class xiiTreeNode
{
public:
  xiiTreeNode() :
    m_pParent(nullptr)
  {
  }
  xiiTreeNode(const T& data) :
    m_Data(data), m_pParent(nullptr)
  {
  }
  ~xiiTreeNode()
  {
    while (!m_Children.IsEmpty())
    {
      RemoveChild(0);
    }
  }

  const xiiUuid&                            GetGuid() const { return m_Guid; }
  const xiiTreeNode<T>*                     GetParent() const { return m_pParent; }
  xiiTreeNode<T>*                           GetParent() { return m_pParent; }
  const xiiHybridArray<xiiTreeNode<T>*, 8>& GetChildren() const { return m_Children; }
  xiiHybridArray<xiiTreeNode<T>*, 8>&       GetChildren() { return m_Children; }

  xiiTreeNode<T>* InsertChild(const T& data, xiiUInt32 iIndex)
  {
    xiiTreeNode<T>* pNode = XII_DEFAULT_NEW(xiiTreeNode<T>, data);
    pNode->m_Guid.CreateNewUuid();
    m_Children.Insert(pNode, iIndex);
    pNode->m_pParent = this;
    return pNode;
  }

  bool RemoveChild(xiiUInt32 iIndex)
  {
    if (iIndex > m_Children.GetCount())
      return false;

    xiiTreeNode<T>* pChild = m_Children[iIndex];
    m_Children.RemoveAtAndCopy(iIndex);
    XII_DEFAULT_DELETE(pChild);
    return true;
  }

  xiiUInt32 GetParentIndex() const
  {
    XII_ASSERT_DEV(m_pParent != nullptr, "Can't compute parent index if no parent is present!");
    for (xiiUInt32 i = 0; i < m_pParent->GetChildren().GetCount(); i++)
    {
      if (m_pParent->GetChildren()[i] == this)
        return i;
    }
    XII_REPORT_FAILURE("Couldn't find oneself in own parent!");
    return -1;
  }

  T       m_Data;
  xiiUuid m_Guid;

private:
  xiiTreeNode<T>*                    m_pParent;
  xiiHybridArray<xiiTreeNode<T>*, 8> m_Children;
};


class XII_GUIFOUNDATION_DLL xiiActionMap
{
public:
  typedef xiiTreeNode<xiiActionMapDescriptor> TreeNode;
  xiiActionMap();
  ~xiiActionMap();

  void      MapAction(xiiActionDescriptorHandle hAction, const char* szPath, float m_fOrder);
  xiiUuid   MapAction(const xiiActionMapDescriptor& desc);
  xiiResult UnmapAction(xiiActionDescriptorHandle hAction, const char* szPath);
  xiiResult UnmapAction(const xiiActionMapDescriptor& desc);
  xiiResult UnmapAction(const xiiUuid& guid);

  const TreeNode*               GetRootObject() const { return &m_Root; }
  const xiiActionMapDescriptor* GetDescriptor(const xiiUuid& guid) const;
  const xiiActionMapDescriptor* GetDescriptor(const xiiTreeNode<xiiActionMapDescriptor>* pObject) const;

private:
  bool                                       FindObjectByPath(const xiiStringView& sPath, xiiUuid& out_guid) const;
  const xiiTreeNode<xiiActionMapDescriptor>* GetChildByName(const xiiTreeNode<xiiActionMapDescriptor>* pObject, const xiiStringView& sName) const;

private:
  TreeNode                                              m_Root;
  xiiMap<xiiUuid, xiiTreeNode<xiiActionMapDescriptor>*> m_Descriptors;
};

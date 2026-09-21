/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GuiFoundation/Action/Action.h>
#include <GuiFoundation/GuiFoundationDLL.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>

class xiiDocument;

struct XII_GUIFOUNDATION_DLL xiiActionMapDescriptor
{
  xiiActionDescriptorHandle m_hAction; ///< Action to be mapped
  xiiString                 m_sPath;   ///< Path where the action should be mapped excluding the action's name, e.g. "File/New" for a menu item "File -> New -> Project..." .
  float                     m_fOrder;  ///< Ordering key to sort actions in the mapping path.
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

  xiiTreeNode<T>* InsertChild(const T& data, xiiUInt32 uiIndex)
  {
    xiiTreeNode<T>* pNode = XII_DEFAULT_NEW(xiiTreeNode<T>, data);
    pNode->m_Guid         = xiiUuid::MakeUuid();
    m_Children.InsertAt(uiIndex, pNode);
    pNode->m_pParent = this;
    return pNode;
  }

  bool RemoveChild(xiiUInt32 uiIndex)
  {
    if (uiIndex > m_Children.GetCount())
      return false;

    xiiTreeNode<T>* pChild = m_Children[uiIndex];
    m_Children.RemoveAtAndCopy(uiIndex);
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

/// Defines the structure of how actions are organized in a particular context.
///
/// Actions are usually commands that are exposed through UI.
/// For instance a button in a toolbar or a menu entry.
///
/// Actions are unique. Each action only exists once in xiiActionManager.
///
/// An action map defines where in a menu an action shows up.
/// Actions are usually grouped by categories. So for example all actions related to opening, closing
/// or saving a document may be in one group. Their position within that group is defined through
/// an 'order' value. This allows plugins to insert actions easily.
///
/// A window might use multiple action maps to build different structures.
/// For example, usually there is one action map for a window menu, and another map for a toolbar.
/// These will contain different actions, and they are organized differently.
///
/// Action maps are created through xiiActionMapManager and are simply identified by name.
class XII_GUIFOUNDATION_DLL xiiActionMap
{
public:
  using TreeNode = xiiTreeNode<xiiActionMapDescriptor>;
  xiiActionMap(xiiStringView sParentMapping);
  ~xiiActionMap();

  /// Adds the given action to into the category or menu identified by sPath.
  ///
  /// All actions added to the same path will be sorted by 'fOrder' and the ones with the smaller values show up at the top.
  ///
  /// sPath must either be a fully qualified path OR the name of a uniquely named category or menu.
  /// If sPath is empty, the action (which may be a category itself) will be mapped into the root.
  /// This is common for top-level menus and for toolbars.
  ///
  /// If sPath is a fully qualified path, the segments are separated by slashes (/)
  /// and each segment must name either a category (see XII_REGISTER_CATEGORY) or a menu (see XII_REGISTER_MENU).
  ///
  /// sPath may also name a category or menu WITHOUT it being a full path. In this case the name must be unique.
  /// If sPath isn't empty and doesn't contain a slash, the system searches all available actions that are already in the action map.
  /// This allows you to insert an action into a category, without knowing the full path to that category.
  /// By convention, categories that are meant to be used that way are named "G.Something". The idea is, that where that category
  /// really shows up (and whether it is its own menu or just an area somewhere) may change in the future, or may be different
  /// in different contexts.
  ///
  /// To make it easier to use 'global' category names combined with an additional relative path, there is an overload of this function
  /// that takes an additional sSubPath argument.
  void MapAction(xiiActionDescriptorHandle hAction, xiiStringView sPath, float fOrder);

  /// An overload of MapAction that takes a dedicated sPath and sSubPath argument for convenience.
  ///
  /// If sPath is a 'global' name of a category, it is searched for (see SearchPathForAction()).
  /// Afterwards sSubPath is appended and the result is forwarded to MapAction() as a single path string.
  void MapAction(xiiActionDescriptorHandle hAction, xiiStringView sPath, xiiStringView sSubPath, float fOrder);

  /// Hides an action from the action map. The same rules for 'global' names apply as for MapAction().
  /// If the target action is in this mapping, prefer not calling MapAction in the first place. Use this for actions to be removed that might be in a parent mapping and thus can't be modified directly.
  void HideAction(xiiActionDescriptorHandle hAction, xiiStringView sPath);

  /// Builds an action tree out of all mapped actions of this and any parent mappings.
  const TreeNode*               BuildActionTree();
  const xiiActionMapDescriptor* GetDescriptor(const xiiTreeNode<xiiActionMapDescriptor>* pObject) const;

private:
  struct TempActionMapDescriptor
  {
    xiiActionDescriptorHandle m_hAction;
    xiiString                 m_sPath;
    xiiString                 m_sSubPath;
    float                     m_fOrder;
  };

  /// Searches for an action with the given name and returns the full path to it.
  ///
  /// This is mainly meant to be used with (unique) names to categories (or menus).
  xiiResult SearchPathForAction(xiiStringView sUniqueName, xiiStringBuilder& out_sPath) const;

  void      MapActionInternal(xiiActionDescriptorHandle hAction, xiiStringView sPath, float fOrder);
  void      MapActionInternal(xiiActionDescriptorHandle hAction, xiiStringView sPath, xiiStringView sSubPath, float fOrder);
  xiiResult UnmapActionInternal(xiiActionDescriptorHandle hAction, xiiStringView sPath);

  xiiUuid   MapActionInternal(const xiiActionMapDescriptor& desc);
  xiiResult UnmapActionInternal(const xiiActionMapDescriptor& desc);
  xiiResult UnmapActionInternal(const xiiUuid& guid);

  const xiiActionMapDescriptor* GetDescriptor(const xiiUuid& guid) const;

  bool                                       FindObjectByPath(xiiStringView sPath, xiiUuid& out_guid) const;
  bool                                       FindObjectPathByName(const xiiTreeNode<xiiActionMapDescriptor>* pObject, xiiStringView sName, xiiStringBuilder& out_sPath) const;
  const xiiTreeNode<xiiActionMapDescriptor>* GetChildByName(const xiiTreeNode<xiiActionMapDescriptor>* pObject, xiiStringView sName) const;

private:
  xiiString                                m_sParentMapping;
  xiiDynamicArray<TempActionMapDescriptor> m_TempActions;
  xiiDynamicArray<TempActionMapDescriptor> m_TempHiddenActions;
  xiiUInt32                                m_uiEditCounter = 0;

  mutable xiiUInt32                                             m_uiTransitiveEditCounterOfRoot = 0;
  mutable TreeNode                                              m_Root;
  mutable xiiMap<xiiUuid, xiiTreeNode<xiiActionMapDescriptor>*> m_Descriptors;
};

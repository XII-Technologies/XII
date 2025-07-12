#include <ToolsFoundation/ToolsFoundationPCH.h>

#include <Foundation/IO/MemoryStream.h>
#include <Foundation/Serialization/DdlSerializer.h>
#include <ToolsFoundation/Command/TreeCommands.h>
#include <ToolsFoundation/Document/PrefabUtils.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>
#include <ToolsFoundation/Reflection/PhantomRttiManager.h>
#include <ToolsFoundation/Serialization/DocumentObjectConverter.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiAddObjectCommand, 1, xiiRTTIDefaultAllocator<xiiAddObjectCommand>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ACCESSOR_PROPERTY("Type", GetType, SetType),
    XII_MEMBER_PROPERTY("ParentGuid", m_Parent),
    XII_MEMBER_PROPERTY("ParentProperty", m_sParentProperty),
    XII_MEMBER_PROPERTY("Index", m_Index),
    XII_MEMBER_PROPERTY("NewGuid", m_NewObjectGuid),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiPasteObjectsCommand, 1, xiiRTTIDefaultAllocator<xiiPasteObjectsCommand>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("ParentGuid", m_Parent),
    XII_MEMBER_PROPERTY("TextGraph", m_sGraphTextFormat),
    XII_MEMBER_PROPERTY("Mime", m_sMimeType),
    XII_MEMBER_PROPERTY("AllowPickedPosition", m_bAllowPickedPosition),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiInstantiatePrefabCommand, 1, xiiRTTIDefaultAllocator<xiiInstantiatePrefabCommand>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("ParentGuid", m_Parent),
    XII_MEMBER_PROPERTY("CreateFromPrefab", m_CreateFromPrefab),
    XII_MEMBER_PROPERTY("BaseGraph", m_sBasePrefabGraph),
    XII_MEMBER_PROPERTY("ObjectGraph", m_sObjectGraph),
    XII_MEMBER_PROPERTY("RemapGuid", m_RemapGuid),
    XII_MEMBER_PROPERTY("CreatedObjects", m_CreatedRootObject),
    XII_MEMBER_PROPERTY("AllowPickedPos", m_bAllowPickedPosition),
    XII_MEMBER_PROPERTY("Index", m_Index),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiUnlinkPrefabCommand, 1, xiiRTTIDefaultAllocator<xiiUnlinkPrefabCommand>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Object", m_Object),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiRemoveObjectCommand, 1, xiiRTTIDefaultAllocator<xiiRemoveObjectCommand>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("ObjectGuid", m_Object),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiMoveObjectCommand, 1, xiiRTTIDefaultAllocator<xiiMoveObjectCommand>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("ObjectGuid", m_Object),
    XII_MEMBER_PROPERTY("NewParentGuid", m_NewParent),
    XII_MEMBER_PROPERTY("ParentProperty", m_sParentProperty),
    XII_MEMBER_PROPERTY("Index", m_Index),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiSetObjectPropertyCommand, 1, xiiRTTIDefaultAllocator<xiiSetObjectPropertyCommand>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("ObjectGuid", m_Object),
    XII_MEMBER_PROPERTY("NewValue", m_NewValue),
    XII_MEMBER_PROPERTY("Index", m_Index),
    XII_MEMBER_PROPERTY("Property", m_sProperty),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiResizeAndSetObjectPropertyCommand, 1, xiiRTTIDefaultAllocator<xiiResizeAndSetObjectPropertyCommand>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("ObjectGuid", m_Object),
    XII_MEMBER_PROPERTY("NewValue", m_NewValue),
    XII_MEMBER_PROPERTY("Index", m_Index),
    XII_MEMBER_PROPERTY("Property", m_sProperty),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiInsertObjectPropertyCommand, 1, xiiRTTIDefaultAllocator<xiiInsertObjectPropertyCommand>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("ObjectGuid", m_Object),
    XII_MEMBER_PROPERTY("NewValue", m_NewValue),
    XII_MEMBER_PROPERTY("Index", m_Index),
    XII_MEMBER_PROPERTY("Property", m_sProperty),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiRemoveObjectPropertyCommand, 1, xiiRTTIDefaultAllocator<xiiRemoveObjectPropertyCommand>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("ObjectGuid", m_Object),
    XII_MEMBER_PROPERTY("Index", m_Index),
    XII_MEMBER_PROPERTY("Property", m_sProperty),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiMoveObjectPropertyCommand, 1, xiiRTTIDefaultAllocator<xiiMoveObjectPropertyCommand>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("ObjectGuid", m_Object),
    XII_MEMBER_PROPERTY("OldIndex", m_OldIndex),
    XII_MEMBER_PROPERTY("NewIndex", m_NewIndex),
    XII_MEMBER_PROPERTY("Property", m_sProperty),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

////////////////////////////////////////////////////////////////////////
// xiiAddObjectCommand
////////////////////////////////////////////////////////////////////////

xiiAddObjectCommand::xiiAddObjectCommand() = default;

xiiStringView xiiAddObjectCommand::GetType() const
{
  if (m_pType == nullptr)
    return {};

  return m_pType->GetTypeName();
}

void xiiAddObjectCommand::SetType(xiiStringView sType)
{
  m_pType = xiiRTTI::FindTypeByName(sType);
}

xiiStatus xiiAddObjectCommand::DoInternal(bool bRedo)
{
  xiiDocument* pDocument = GetDocument();

  if (!bRedo)
  {
    if (!m_NewObjectGuid.IsValid())
      m_NewObjectGuid = xiiUuid::MakeUuid();
  }

  xiiDocumentObject* pParent = nullptr;
  if (m_Parent.IsValid())
  {
    pParent = pDocument->GetObjectManager()->GetObject(m_Parent);
    if (pParent == nullptr)
      return xiiStatus("Add Object: The given parent does not exist!");
  }

  XII_SUCCEED_OR_RETURN(pDocument->GetObjectManager()->CanAdd(m_pType, pParent, m_sParentProperty, m_Index));

  if (!bRedo)
  {
    m_pObject = pDocument->GetObjectManager()->CreateObject(m_pType, m_NewObjectGuid);
  }

  pDocument->GetObjectManager()->AddObject(m_pObject, pParent, m_sParentProperty, m_Index);
  return XII_SUCCESS;
}

xiiStatus xiiAddObjectCommand::UndoInternal(bool bFireEvents)
{
  XII_ASSERT_DEV(bFireEvents, "This command does not support temporary commands");

  xiiDocument* pDocument = GetDocument();
  XII_SUCCEED_OR_RETURN(pDocument->GetObjectManager()->CanRemove(m_pObject));

  pDocument->GetObjectManager()->RemoveObject(m_pObject);
  return XII_SUCCESS;
}

void xiiAddObjectCommand::CleanupInternal(CommandState state)
{
  if (state == CommandState::WasUndone)
  {
    GetDocument()->GetObjectManager()->DestroyObject(m_pObject);
    m_pObject = nullptr;
  }
}


////////////////////////////////////////////////////////////////////////
// xiiPasteObjectsCommand
////////////////////////////////////////////////////////////////////////

xiiPasteObjectsCommand::xiiPasteObjectsCommand() = default;

xiiStatus xiiPasteObjectsCommand::DoInternal(bool bRedo)
{
  xiiDocument* pDocument = GetDocument();

  xiiDocumentObject* pParent = nullptr;
  if (m_Parent.IsValid())
  {
    pParent = pDocument->GetObjectManager()->GetObject(m_Parent);
    if (pParent == nullptr)
      return xiiStatus("Paste Objects: The given parent does not exist!");
  }

  if (!bRedo)
  {
    xiiAbstractObjectGraph graph;

    {
      // Deserialize
      xiiRawMemoryStreamReader memoryReader(m_sGraphTextFormat.GetData(), m_sGraphTextFormat.GetElementCount());
      XII_SUCCEED_OR_RETURN(xiiAbstractGraphDdlSerializer::Read(memoryReader, &graph));
    }

    // Remap
    graph.ReMapNodeGuids(xiiUuid::MakeUuid());

    xiiDocumentObjectConverterReader reader(&graph, pDocument->GetObjectManager(), xiiDocumentObjectConverterReader::Mode::CreateOnly);

    xiiHybridArray<xiiAbstractObjectNode*, 16> RootNodes;
    auto&                                      nodes = graph.GetAllNodes();
    for (auto it = nodes.GetIterator(); it.IsValid(); ++it)
    {
      auto* pNode = it.Value();
      if (pNode->GetNodeName() == "root")
      {
        RootNodes.PushBack(pNode);
      }
    }

    RootNodes.Sort([](const xiiAbstractObjectNode* a, const xiiAbstractObjectNode* b) {
      auto* pOrderA = a->FindProperty("__Order");
      auto* pOrderB = b->FindProperty("__Order");
      if (pOrderA && pOrderB && pOrderA->m_Value.CanConvertTo<xiiUInt32>() && pOrderB->m_Value.CanConvertTo<xiiUInt32>())
      {
        return pOrderA->m_Value.ConvertTo<xiiUInt32>() < pOrderB->m_Value.ConvertTo<xiiUInt32>();
      }
      return a < b;
    });

    xiiHybridArray<xiiDocument::PasteInfo, 16> ToBePasted;
    for (xiiAbstractObjectNode* pNode : RootNodes)
    {
      auto* pNewObject = reader.CreateObjectFromNode(pNode);

      if (pNewObject)
      {
        reader.ApplyPropertiesToObject(pNode, pNewObject);

        auto& ref     = ToBePasted.ExpandAndGetRef();
        ref.m_pObject = pNewObject;
        ref.m_pParent = pParent;
      }
    }

    if (pDocument->Paste(ToBePasted, graph, m_bAllowPickedPosition, m_sMimeType))
    {
      for (const auto& item : ToBePasted)
      {
        auto& po             = m_PastedObjects.ExpandAndGetRef();
        po.m_pObject         = item.m_pObject;
        po.m_pParent         = item.m_pParent;
        po.m_Index           = item.m_pObject->GetPropertyIndex();
        po.m_sParentProperty = item.m_pObject->GetParentProperty();
      }
    }
    else
    {
      for (const auto& item : ToBePasted)
      {
        pDocument->GetObjectManager()->DestroyObject(item.m_pObject);
      }
    }

    if (m_PastedObjects.IsEmpty())
      return xiiStatus("Paste Objects: nothing was pasted!");
  }
  else
  {
    // Re-add at recorded place.
    for (auto& po : m_PastedObjects)
    {
      pDocument->GetObjectManager()->AddObject(po.m_pObject, po.m_pParent, po.m_sParentProperty, po.m_Index);
    }
  }
  return XII_SUCCESS;
}

xiiStatus xiiPasteObjectsCommand::UndoInternal(bool bFireEvents)
{
  XII_ASSERT_DEV(bFireEvents, "This command does not support temporary commands");
  xiiDocument* pDocument = GetDocument();

  for (auto& po : m_PastedObjects)
  {
    XII_SUCCEED_OR_RETURN(pDocument->GetObjectManager()->CanRemove(po.m_pObject));

    pDocument->GetObjectManager()->RemoveObject(po.m_pObject);
  }

  return XII_SUCCESS;
}

void xiiPasteObjectsCommand::CleanupInternal(CommandState state)
{
  if (state == CommandState::WasUndone)
  {
    for (auto& po : m_PastedObjects)
    {
      GetDocument()->GetObjectManager()->DestroyObject(po.m_pObject);
    }
    m_PastedObjects.Clear();
  }
}

////////////////////////////////////////////////////////////////////////
// xiiInstantiatePrefabCommand
////////////////////////////////////////////////////////////////////////

xiiInstantiatePrefabCommand::xiiInstantiatePrefabCommand()
{
  m_bAllowPickedPosition = true;
}

xiiStatus xiiInstantiatePrefabCommand::DoInternal(bool bRedo)
{
  xiiDocument* pDocument = GetDocument();

  xiiDocumentObject* pParent = nullptr;
  if (m_Parent.IsValid())
  {
    pParent = pDocument->GetObjectManager()->GetObject(m_Parent);
    if (pParent == nullptr)
      return xiiStatus("Instantiate Prefab: The given parent does not exist!");
  }

  if (!bRedo)
  {
    // TODO: this is hard-coded, it only works for scene documents !
    const xiiRTTI* pRootObjectType = xiiRTTI::FindTypeByName("xiiGameObject");
    xiiStringView  sParentProperty = "Children"_xiisv;

    xiiDocumentObject*                         pRootObject = nullptr;
    xiiHybridArray<xiiDocument::PasteInfo, 16> ToBePasted;
    xiiAbstractObjectGraph                     graph;

    // create root object
    {
      XII_SUCCEED_OR_RETURN(pDocument->GetObjectManager()->CanAdd(pRootObjectType, pParent, sParentProperty, m_Index));

      // use the same GUID for the root object ID as the remap GUID, this way the object ID is deterministic and reproducible
      m_CreatedRootObject = m_RemapGuid;

      pRootObject = pDocument->GetObjectManager()->CreateObject(pRootObjectType, m_CreatedRootObject);

      auto& ref     = ToBePasted.ExpandAndGetRef();
      ref.m_pObject = pRootObject;
      ref.m_pParent = pParent;
      ref.m_Index   = m_Index;
    }

    // update meta data
    // this is read when Paste is executed, to determine a good node name
    {
      // if prefabs are not allowed in this document, just create this as a regular object, with no link to the prefab template
      if (pDocument->ArePrefabsAllowed())
      {
        auto pMeta                = pDocument->m_DocumentObjectMetaData->BeginModifyMetaData(m_CreatedRootObject);
        pMeta->m_CreateFromPrefab = m_CreateFromPrefab;
        pMeta->m_PrefabSeedGuid   = m_RemapGuid;
        pMeta->m_sBasePrefab      = m_sBasePrefabGraph;
        pDocument->m_DocumentObjectMetaData->EndModifyMetaData(xiiDocumentObjectMetaData::PrefabFlag);
      }
      else
      {
        pDocument->ShowDocumentStatus("Nested prefabs are not allowed. Instantiated object will not be linked to prefab template.");
      }
    }

    if (pDocument->Paste(ToBePasted, graph, m_bAllowPickedPosition, "application/xiiEditor.xiiAbstractGraph"))
    {
      for (const auto& item : ToBePasted)
      {
        auto& po             = m_PastedObjects.ExpandAndGetRef();
        po.m_pObject         = item.m_pObject;
        po.m_pParent         = item.m_pParent;
        po.m_Index           = item.m_pObject->GetPropertyIndex();
        po.m_sParentProperty = item.m_pObject->GetParentProperty();
      }
    }
    else
    {
      for (const auto& item : ToBePasted)
      {
        pDocument->GetObjectManager()->DestroyObject(item.m_pObject);
      }

      ToBePasted.Clear();
    }

    if (m_PastedObjects.IsEmpty())
      return xiiStatus("Paste Objects: nothing was pasted!");

    if (!m_sObjectGraph.IsEmpty())
      xiiPrefabUtils::LoadGraph(graph, m_sObjectGraph);
    else
      xiiPrefabUtils::LoadGraph(graph, m_sBasePrefabGraph);

    graph.ReMapNodeGuids(m_RemapGuid);

    // a prefab can have multiple top level nodes
    xiiHybridArray<xiiAbstractObjectNode*, 4> rootNodes;
    xiiPrefabUtils::GetRootNodes(graph, rootNodes);

    for (auto* pPrefabRoot : rootNodes)
    {
      xiiDocumentObjectConverterReader reader(&graph, pDocument->GetObjectManager(), xiiDocumentObjectConverterReader::Mode::CreateOnly);

      if (auto* pNewObject = reader.CreateObjectFromNode(pPrefabRoot))
      {
        reader.ApplyPropertiesToObject(pPrefabRoot, pNewObject);

        // attach all prefab nodes to the main group node
        pDocument->GetObjectManager()->AddObject(pNewObject, pRootObject, sParentProperty, -1);
      }
    }
  }
  else
  {
    // Re-add at recorded place.
    for (auto& po : m_PastedObjects)
    {
      pDocument->GetObjectManager()->AddObject(po.m_pObject, po.m_pParent, po.m_sParentProperty, po.m_Index);
    }
  }

  return XII_SUCCESS;
}

xiiStatus xiiInstantiatePrefabCommand::UndoInternal(bool bFireEvents)
{
  XII_ASSERT_DEV(bFireEvents, "This command does not support temporary commands");
  xiiDocument* pDocument = GetDocument();

  for (auto& po : m_PastedObjects)
  {
    XII_SUCCEED_OR_RETURN(pDocument->GetObjectManager()->CanRemove(po.m_pObject));

    pDocument->GetObjectManager()->RemoveObject(po.m_pObject);
  }

  return XII_SUCCESS;
}

void xiiInstantiatePrefabCommand::CleanupInternal(CommandState state)
{
  if (state == CommandState::WasUndone)
  {
    for (auto& po : m_PastedObjects)
    {
      GetDocument()->GetObjectManager()->DestroyObject(po.m_pObject);
    }
    m_PastedObjects.Clear();
  }
}


//////////////////////////////////////////////////////////////////////////
// xiiUnlinkPrefabCommand
//////////////////////////////////////////////////////////////////////////

xiiStatus xiiUnlinkPrefabCommand::DoInternal(bool bRedo)
{
  xiiDocument*       pDocument = GetDocument();
  xiiDocumentObject* pObject   = pDocument->GetObjectManager()->GetObject(m_Object);

  if (pObject == nullptr)
    return xiiStatus("Unlink Prefab: The given object does not exist!");

  // store previous values
  if (!bRedo)
  {
    auto pMeta            = pDocument->m_DocumentObjectMetaData->BeginReadMetaData(m_Object);
    m_OldCreateFromPrefab = pMeta->m_CreateFromPrefab;
    m_OldRemapGuid        = pMeta->m_PrefabSeedGuid;
    m_sOldGraphTextFormat = pMeta->m_sBasePrefab;
    pDocument->m_DocumentObjectMetaData->EndReadMetaData();
  }

  // unlink
  {
    auto pMeta                = pDocument->m_DocumentObjectMetaData->BeginModifyMetaData(m_Object);
    pMeta->m_CreateFromPrefab = xiiUuid();
    pMeta->m_PrefabSeedGuid   = xiiUuid();
    pMeta->m_sBasePrefab.Clear();
    pDocument->m_DocumentObjectMetaData->EndModifyMetaData(xiiDocumentObjectMetaData::PrefabFlag);
  }

  return XII_SUCCESS;
}

xiiStatus xiiUnlinkPrefabCommand::UndoInternal(bool bFireEvents)
{
  xiiDocument*       pDocument = GetDocument();
  xiiDocumentObject* pObject   = pDocument->GetObjectManager()->GetObject(m_Object);

  if (pObject == nullptr)
    return xiiStatus("Unlink Prefab: The given object does not exist!");

  // restore link
  {
    auto pMeta                = pDocument->m_DocumentObjectMetaData->BeginModifyMetaData(m_Object);
    pMeta->m_CreateFromPrefab = m_OldCreateFromPrefab;
    pMeta->m_PrefabSeedGuid   = m_OldRemapGuid;
    pMeta->m_sBasePrefab      = m_sOldGraphTextFormat;
    pDocument->m_DocumentObjectMetaData->EndModifyMetaData(xiiDocumentObjectMetaData::PrefabFlag);
  }

  return XII_SUCCESS;
}


////////////////////////////////////////////////////////////////////////
// xiiRemoveObjectCommand
////////////////////////////////////////////////////////////////////////

xiiRemoveObjectCommand::xiiRemoveObjectCommand() = default;

xiiStatus xiiRemoveObjectCommand::DoInternal(bool bRedo)
{
  xiiDocument* pDocument = GetDocument();

  if (!bRedo)
  {
    if (m_Object.IsValid())
    {
      m_pObject = pDocument->GetObjectManager()->GetObject(m_Object);
      if (m_pObject == nullptr)
        return xiiStatus("Remove Object: The given object does not exist!");
    }
    else
      return xiiStatus("Remove Object: The given object does not exist!");

    XII_SUCCEED_OR_RETURN(pDocument->GetObjectManager()->CanRemove(m_pObject));

    m_pParent                                 = const_cast<xiiDocumentObject*>(m_pObject->GetParent());
    m_sParentProperty                         = m_pObject->GetParentProperty();
    const xiiIReflectedTypeAccessor& accessor = m_pObject->GetParent()->GetTypeAccessor();
    m_Index                                   = accessor.GetPropertyChildIndex(m_pObject->GetParentProperty(), m_pObject->GetGuid());
  }

  pDocument->GetObjectManager()->RemoveObject(m_pObject);
  return XII_SUCCESS;
}

xiiStatus xiiRemoveObjectCommand::UndoInternal(bool bFireEvents)
{
  XII_ASSERT_DEV(bFireEvents, "This command does not support temporary commands");

  xiiDocument* pDocument = GetDocument();
  XII_SUCCEED_OR_RETURN(pDocument->GetObjectManager()->CanAdd(m_pObject->GetTypeAccessor().GetType(), m_pParent, m_sParentProperty, m_Index));

  pDocument->GetObjectManager()->AddObject(m_pObject, m_pParent, m_sParentProperty, m_Index);
  return XII_SUCCESS;
}

void xiiRemoveObjectCommand::CleanupInternal(CommandState state)
{
  if (state == CommandState::WasDone)
  {
    GetDocument()->GetObjectManager()->DestroyObject(m_pObject);
    m_pObject = nullptr;
  }
}


////////////////////////////////////////////////////////////////////////
// xiiMoveObjectCommand
////////////////////////////////////////////////////////////////////////

xiiMoveObjectCommand::xiiMoveObjectCommand()
{
  m_pObject    = nullptr;
  m_pOldParent = nullptr;
  m_pNewParent = nullptr;
}

xiiStatus xiiMoveObjectCommand::DoInternal(bool bRedo)
{
  xiiDocument* pDocument = GetDocument();

  if (!bRedo)
  {
    {
      m_pObject = pDocument->GetObjectManager()->GetObject(m_Object);
      if (m_pObject == nullptr)
        return xiiStatus("Move Object: The given object does not exist!");
    }

    if (m_NewParent.IsValid())
    {
      m_pNewParent = pDocument->GetObjectManager()->GetObject(m_NewParent);
      if (m_pNewParent == nullptr)
        return xiiStatus("Move Object: The new parent does not exist!");
    }

    m_pOldParent                              = const_cast<xiiDocumentObject*>(m_pObject->GetParent());
    m_sOldParentProperty                      = m_pObject->GetParentProperty();
    const xiiIReflectedTypeAccessor& accessor = m_pOldParent->GetTypeAccessor();
    m_OldIndex                                = accessor.GetPropertyChildIndex(m_pObject->GetParentProperty(), m_pObject->GetGuid());

    XII_SUCCEED_OR_RETURN(pDocument->GetObjectManager()->CanMove(m_pObject, m_pNewParent, m_sParentProperty, m_Index));
  }

  pDocument->GetObjectManager()->MoveObject(m_pObject, m_pNewParent, m_sParentProperty, m_Index);
  return XII_SUCCESS;
}

xiiStatus xiiMoveObjectCommand::UndoInternal(bool bFireEvents)
{
  XII_ASSERT_DEV(bFireEvents, "This command does not support temporary commands");

  xiiDocument* pDocument = GetDocument();

  xiiVariant FinalOldPosition = m_OldIndex;

  if (m_Index.CanConvertTo<xiiInt32>() && m_pOldParent == m_pNewParent)
  {
    // If we are moving an object downwards, we must move by more than 1 (+1 would be behind the same object, which is still the same
    // position) so an object must always be moved by at least +2 moving UP can be done by -1, so when we undo that, we must ensure to move
    // +2

    xiiInt32 iNew = m_Index.ConvertTo<xiiInt32>();
    xiiInt32 iOld = m_OldIndex.ConvertTo<xiiInt32>();

    if (iNew < iOld)
    {
      FinalOldPosition = iOld + 1;
    }
  }

  XII_SUCCEED_OR_RETURN(pDocument->GetObjectManager()->CanMove(m_pObject, m_pOldParent, m_sOldParentProperty, FinalOldPosition));

  pDocument->GetObjectManager()->MoveObject(m_pObject, m_pOldParent, m_sOldParentProperty, FinalOldPosition);

  return XII_SUCCESS;
}


////////////////////////////////////////////////////////////////////////
// xiiSetObjectPropertyCommand
////////////////////////////////////////////////////////////////////////

xiiSetObjectPropertyCommand::xiiSetObjectPropertyCommand()
{
  m_pObject = nullptr;
}

xiiStatus xiiSetObjectPropertyCommand::DoInternal(bool bRedo)
{
  xiiDocument* pDocument = GetDocument();

  if (!bRedo)
  {
    // if this assert triggers because of a stringview, check the caller and make sure to copy the stringview into a string first
    // something like this:
    // const xiiVariantType::Enum storageType = xiiToolsReflectionUtils::GetStorageType(pProp);
    // if (op.m_Value.GetType() != storageType)
    // {
    //   op.m_Value = op.m_Value.ConvertTo(storageType);
    // }
    XII_ASSERT_DEBUG(m_NewValue.GetType() != xiiVariantType::StringView && m_NewValue.GetType() != xiiVariantType::TypedPointer, "Variants that are stored in the command history must hold ownership of their value.");

    if (m_Object.IsValid())
    {
      m_pObject = pDocument->GetObjectManager()->GetObject(m_Object);
      if (m_pObject == nullptr)
        return xiiStatus("Set Property: The given object does not exist!");
    }
    else
      return xiiStatus("Set Property: The given object does not exist!");

    xiiIReflectedTypeAccessor& accessor0 = m_pObject->GetTypeAccessor();

    xiiStatus res(XII_SUCCESS);
    m_OldValue = accessor0.GetValue(m_sProperty, m_Index, &res);
    if (res.Failed())
      return res;
    const xiiAbstractProperty* pProp = accessor0.GetType()->FindPropertyByName(m_sProperty);
    if (pProp == nullptr)
      return xiiStatus(xiiFmt("Set Property: The property '{0}' does not exist", m_sProperty));

    if (pProp->GetFlags().IsSet(xiiPropertyFlags::PointerOwner))
    {
      return xiiStatus(xiiFmt("Set Property: The property '{0}' is a PointerOwner, use xiiAddObjectCommand instead", m_sProperty));
    }

    if (pProp->GetAttributeByType<xiiTemporaryAttribute>())
    {
      // if we modify a 'temporary' property, ie. one that is not serialized,
      // don't mark the document as modified
      m_bModifiedDocument = false;
    }
  }

  return pDocument->GetObjectManager()->SetValue(m_pObject, m_sProperty, m_NewValue, m_Index);
}

xiiStatus xiiSetObjectPropertyCommand::UndoInternal(bool bFireEvents)
{
  if (bFireEvents)
  {
    return GetDocument()->GetObjectManager()->SetValue(m_pObject, m_sProperty, m_OldValue, m_Index);
  }
  else
  {
    xiiIReflectedTypeAccessor& accessor = m_pObject->GetTypeAccessor();
    if (!accessor.SetValue(m_sProperty, m_OldValue, m_Index))
    {
      return xiiStatus(xiiFmt("Set Property: The property '{0}' does not exist", m_sProperty));
    }
  }
  return XII_SUCCESS;
}

////////////////////////////////////////////////////////////////////////
// xiiSetObjectPropertyCommand
////////////////////////////////////////////////////////////////////////

xiiResizeAndSetObjectPropertyCommand::xiiResizeAndSetObjectPropertyCommand()
{
  m_pObject = nullptr;
}

xiiStatus xiiResizeAndSetObjectPropertyCommand::DoInternal(bool bRedo)
{
  xiiDocument* pDocument = GetDocument();

  if (!bRedo)
  {
    if (m_Object.IsValid())
    {
      m_pObject = pDocument->GetObjectManager()->GetObject(m_Object);
      if (m_pObject == nullptr)
        return xiiStatus("Set Property: The given object does not exist!");
    }
    else
      return xiiStatus("Set Property: The given object does not exist!");

    const xiiInt32 uiIndex = m_Index.ConvertTo<xiiInt32>();

    xiiIReflectedTypeAccessor& accessor0 = m_pObject->GetTypeAccessor();

    const xiiInt32 iCount = accessor0.GetCount(m_sProperty);

    for (xiiInt32 i = iCount; i <= uiIndex; ++i)
    {
      xiiInsertObjectPropertyCommand ins;
      ins.m_Object    = m_Object;
      ins.m_sProperty = m_sProperty;
      ins.m_Index     = i;
      ins.m_NewValue  = xiiReflectionUtils::GetDefaultVariantFromType(m_NewValue.GetType());

      AddSubCommand(ins).AssertSuccess();
    }

    xiiSetObjectPropertyCommand set;
    set.m_sProperty = m_sProperty;
    set.m_Index     = m_Index;
    set.m_NewValue  = m_NewValue;
    set.m_Object    = m_Object;

    AddSubCommand(set).AssertSuccess();
  }

  return XII_SUCCESS;
}

////////////////////////////////////////////////////////////////////////
// xiiInsertObjectPropertyCommand
////////////////////////////////////////////////////////////////////////

xiiInsertObjectPropertyCommand::xiiInsertObjectPropertyCommand()
{
  m_pObject = nullptr;
}

xiiStatus xiiInsertObjectPropertyCommand::DoInternal(bool bRedo)
{
  xiiDocument* pDocument = GetDocument();

  if (!bRedo)
  {
    if (m_Object.IsValid())
    {
      m_pObject = pDocument->GetObjectManager()->GetObject(m_Object);
      if (m_pObject == nullptr)
        return xiiStatus("Insert Property: The given object does not exist!");
    }
    else
      return xiiStatus("Insert Property: The given object does not exist!");

    if (m_Index.CanConvertTo<xiiInt32>() && m_Index.ConvertTo<xiiInt32>() == -1)
    {
      xiiIReflectedTypeAccessor& accessor = m_pObject->GetTypeAccessor();
      m_Index                             = accessor.GetCount(m_sProperty);
    }
  }

  return pDocument->GetObjectManager()->InsertValue(m_pObject, m_sProperty, m_NewValue, m_Index);
}

xiiStatus xiiInsertObjectPropertyCommand::UndoInternal(bool bFireEvents)
{
  if (bFireEvents)
  {
    return GetDocument()->GetObjectManager()->RemoveValue(m_pObject, m_sProperty, m_Index);
  }
  else
  {
    xiiIReflectedTypeAccessor& accessor = m_pObject->GetTypeAccessor();
    if (!accessor.RemoveValue(m_sProperty, m_Index))
    {
      return xiiStatus(xiiFmt("Insert Property: The property '{0}' does not exist", m_sProperty));
    }
  }

  return XII_SUCCESS;
}


////////////////////////////////////////////////////////////////////////
// xiiRemoveObjectPropertyCommand
////////////////////////////////////////////////////////////////////////

xiiRemoveObjectPropertyCommand::xiiRemoveObjectPropertyCommand()
{
  m_pObject = nullptr;
}

xiiStatus xiiRemoveObjectPropertyCommand::DoInternal(bool bRedo)
{
  xiiDocument* pDocument = GetDocument();

  if (!bRedo)
  {
    if (m_Object.IsValid())
    {
      m_pObject = pDocument->GetObjectManager()->GetObject(m_Object);
      if (m_pObject == nullptr)
        return xiiStatus("Remove Property: The given object does not exist!");

      xiiStatus res(XII_SUCCESS);

      m_OldValue = m_pObject->GetTypeAccessor().GetValue(m_sProperty, m_Index, &res);
      if (res.Failed())
        return res;
    }
    else
    {
      return xiiStatus("Remove Property: The given object does not exist!");
    }
  }

  return pDocument->GetObjectManager()->RemoveValue(m_pObject, m_sProperty, m_Index);
}

xiiStatus xiiRemoveObjectPropertyCommand::UndoInternal(bool bFireEvents)
{
  if (bFireEvents)
  {
    return GetDocument()->GetObjectManager()->InsertValue(m_pObject, m_sProperty, m_OldValue, m_Index);
  }
  else
  {
    xiiIReflectedTypeAccessor& accessor = m_pObject->GetTypeAccessor();
    if (!accessor.InsertValue(m_sProperty, m_Index, m_OldValue))
    {
      return xiiStatus(xiiFmt("Remove Property: Undo failed! The index '{0}' in property '{1}' does not exist", m_Index.ConvertTo<xiiString>(), m_sProperty));
    }
  }
  return XII_SUCCESS;
}


////////////////////////////////////////////////////////////////////////
// xiiMoveObjectPropertyCommand
////////////////////////////////////////////////////////////////////////

xiiMoveObjectPropertyCommand::xiiMoveObjectPropertyCommand()
{
  m_pObject = nullptr;
}

xiiStatus xiiMoveObjectPropertyCommand::DoInternal(bool bRedo)
{
  xiiDocument* pDocument = GetDocument();

  if (!bRedo)
  {
    m_pObject = pDocument->GetObjectManager()->GetObject(m_Object);
    if (m_pObject == nullptr)
      return xiiStatus("Move Property: The given object does not exist.");
  }

  return GetDocument()->GetObjectManager()->MoveValue(m_pObject, m_sProperty, m_OldIndex, m_NewIndex);
}

xiiStatus xiiMoveObjectPropertyCommand::UndoInternal(bool bFireEvents)
{
  XII_ASSERT_DEV(bFireEvents, "This command does not support temporary commands");

  xiiVariant FinalOldPosition = m_OldIndex;
  xiiVariant FinalNewPosition = m_NewIndex;

  if (m_OldIndex.CanConvertTo<xiiInt32>())
  {
    // If we are moving an object downwards, we must move by more than 1 (+1 would be behind the same object, which is still the same
    // position) so an object must always be moved by at least +2 moving UP can be done by -1, so when we undo that, we must ensure to move
    // +2

    xiiInt32 iNew = m_NewIndex.ConvertTo<xiiInt32>();
    xiiInt32 iOld = m_OldIndex.ConvertTo<xiiInt32>();

    if (iNew < iOld)
    {
      FinalOldPosition = iOld + 1;
    }

    // The new position is relative to the original array, so we need to substract one to account for
    // the removal of the same element at the lower index.
    if (iNew > iOld)
    {
      FinalNewPosition = iNew - 1;
    }
  }

  return GetDocument()->GetObjectManager()->MoveValue(m_pObject, m_sProperty, FinalNewPosition, FinalOldPosition);
}

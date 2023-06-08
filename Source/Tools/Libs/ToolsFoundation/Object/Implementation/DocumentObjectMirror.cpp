#include <ToolsFoundation/ToolsFoundationPCH.h>

#include <Foundation/IO/MemoryStream.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Serialization/BinarySerializer.h>
#include <ToolsFoundation/Object/DocumentObjectMirror.h>
#include <ToolsFoundation/Serialization/DocumentObjectConverter.h>

// clang-format off
XII_BEGIN_STATIC_REFLECTED_TYPE(xiiObjectChange, xiiNoBase, 1, xiiRTTIDefaultAllocator<xiiObjectChange>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Change", m_Change),
    XII_MEMBER_PROPERTY("Root", m_Root),
    XII_ARRAY_MEMBER_PROPERTY("Steps", m_Steps),
    XII_MEMBER_PROPERTY("Graph", m_GraphData),
  }
  XII_END_PROPERTIES;
}
XII_END_STATIC_REFLECTED_TYPE;
// clang-format on

xiiObjectChange::xiiObjectChange(const xiiObjectChange&)
{
  XII_REPORT_FAILURE("Not supported!");
}

void xiiObjectChange::GetGraph(xiiAbstractObjectGraph& ref_graph) const
{
  ref_graph.Clear();

  xiiRawMemoryStreamReader reader(m_GraphData);
  xiiAbstractGraphBinarySerializer::Read(reader, &ref_graph);
}

void xiiObjectChange::SetGraph(xiiAbstractObjectGraph& ref_graph)
{
  xiiContiguousMemoryStreamStorage storage;
  xiiMemoryStreamWriter            writer(&storage);
  xiiAbstractGraphBinarySerializer::Write(writer, &ref_graph);

  m_GraphData = {storage.GetData(), storage.GetStorageSize32()};
}

xiiObjectChange::xiiObjectChange(xiiObjectChange&& rhs)
{
  m_Change    = std::move(rhs.m_Change);
  m_Root      = rhs.m_Root;
  m_Steps     = std::move(rhs.m_Steps);
  m_GraphData = std::move(rhs.m_GraphData);
}

void xiiObjectChange::operator=(xiiObjectChange&& rhs)
{
  m_Change    = std::move(rhs.m_Change);
  m_Root      = rhs.m_Root;
  m_Steps     = std::move(rhs.m_Steps);
  m_GraphData = std::move(rhs.m_GraphData);
}

void xiiObjectChange::operator=(xiiObjectChange& rhs)
{
  XII_REPORT_FAILURE("Not supported!");
}


xiiDocumentObjectMirror::xiiDocumentObjectMirror()
{
  m_pContext = nullptr;
  m_pManager = nullptr;
}

xiiDocumentObjectMirror::~xiiDocumentObjectMirror()
{
  XII_ASSERT_DEV(m_pManager == nullptr && m_pContext == nullptr, "Need to call DeInit before d-tor!");
}

void xiiDocumentObjectMirror::InitSender(const xiiDocumentObjectManager* pManager)
{
  m_pManager = pManager;
  m_pManager->m_StructureEvents.AddEventHandler(xiiMakeDelegate(&xiiDocumentObjectMirror::TreeStructureEventHandler, this));
  m_pManager->m_PropertyEvents.AddEventHandler(xiiMakeDelegate(&xiiDocumentObjectMirror::TreePropertyEventHandler, this));
}

void xiiDocumentObjectMirror::InitReceiver(xiiRttiConverterContext* pContext)
{
  m_pContext = pContext;
}

void xiiDocumentObjectMirror::DeInit()
{
  if (m_pManager)
  {
    m_pManager->m_StructureEvents.RemoveEventHandler(xiiMakeDelegate(&xiiDocumentObjectMirror::TreeStructureEventHandler, this));
    m_pManager->m_PropertyEvents.RemoveEventHandler(xiiMakeDelegate(&xiiDocumentObjectMirror::TreePropertyEventHandler, this));
    m_pManager = nullptr;
  }

  if (m_pContext)
  {
    m_pContext = nullptr;
  }
}

void xiiDocumentObjectMirror::SetFilterFunction(FilterFunction filter)
{
  m_Filter = filter;
}

void xiiDocumentObjectMirror::SendDocument()
{
  const auto* pRoot = m_pManager->GetRootObject();
  for (auto* pChild : pRoot->GetChildren())
  {
    if (IsDiscardedByFilter(pRoot, pChild->GetParentProperty()))
      continue;

    xiiObjectChange change;
    change.m_Change.m_Operation = xiiObjectChangeType::NodeAdded;
    change.m_Change.m_Value     = pChild->GetGuid();

    xiiAbstractObjectGraph           graph;
    xiiDocumentObjectConverterWriter objectConverter(&graph, m_pManager);
    objectConverter.AddObjectToGraph(pChild, "Object");
    change.SetGraph(graph);

    ApplyOp(change);
  }
}

void xiiDocumentObjectMirror::Clear()
{
  if (m_pManager)
  {
    const auto* pRoot = m_pManager->GetRootObject();
    for (auto* pChild : pRoot->GetChildren())
    {
      xiiObjectChange change;
      change.m_Change.m_Operation = xiiObjectChangeType::NodeRemoved;
      change.m_Change.m_Value     = pChild->GetGuid();

      /*xiiAbstractObjectGraph graph;
      xiiDocumentObjectConverterWriter objectConverter(&graph, m_pManager);
      xiiAbstractObjectNode* pNode = objectConverter.AddObjectToGraph(pChild, "Object");
      change.SetGraph(graph);*/

      ApplyOp(change);
    }
  }

  if (m_pContext)
  {
    m_pContext->Clear();
  }
}

void xiiDocumentObjectMirror::TreeStructureEventHandler(const xiiDocumentObjectStructureEvent& e)
{
  if (e.m_pNewParent && IsDiscardedByFilter(e.m_pNewParent, e.m_sParentProperty))
    return;
  if (e.m_pPreviousParent && IsDiscardedByFilter(e.m_pPreviousParent, e.m_sParentProperty))
    return;

  switch (e.m_EventType)
  {
    case xiiDocumentObjectStructureEvent::Type::AfterObjectMoved:
    {
      if (IsHeapAllocated(e.m_pNewParent, e.m_sParentProperty))
      {
        if (e.m_pNewParent == nullptr || e.m_pNewParent == m_pManager->GetRootObject())
        {
          // Object is now a root object, nothing to do to attach it to its new parent.
          break;
        }

        if (e.GetProperty()->GetCategory() == xiiPropertyCategory::Set && e.m_pPreviousParent == e.m_pNewParent)
        {
          // Sets only have ordering in the editor. We can ignore set order changes in the mirror.
          break;
        }
        xiiObjectChange change;
        CreatePath(change, e.m_pNewParent, e.m_sParentProperty);

        change.m_Change.m_Operation = xiiObjectChangeType::PropertyInserted;
        change.m_Change.m_Index     = e.getInsertIndex();
        change.m_Change.m_Value     = e.m_pObject->GetGuid();

        ApplyOp(change);
        break;
      }
      // Intended falltrough as non ptr object might as well be destroyed and rebuild.
    }
      // case xiiDocumentObjectStructureEvent::Type::BeforeObjectAdded:
    case xiiDocumentObjectStructureEvent::Type::AfterObjectAdded:
    {
      xiiObjectChange change;
      CreatePath(change, e.m_pNewParent, e.m_sParentProperty);

      change.m_Change.m_Operation = xiiObjectChangeType::NodeAdded;
      change.m_Change.m_Index     = e.getInsertIndex();
      change.m_Change.m_Value     = e.m_pObject->GetGuid();

      xiiAbstractObjectGraph           graph;
      xiiDocumentObjectConverterWriter objectConverter(&graph, m_pManager);
      objectConverter.AddObjectToGraph(e.m_pObject, "Object");
      change.SetGraph(graph);

      ApplyOp(change);
    }
    break;
    case xiiDocumentObjectStructureEvent::Type::BeforeObjectMoved:
    {
      if (IsHeapAllocated(e.m_pPreviousParent, e.m_sParentProperty))
      {
        XII_ASSERT_DEBUG(IsHeapAllocated(e.m_pNewParent, e.m_sParentProperty), "Old and new parent must have the same heap allocation state!");
        if (e.m_pPreviousParent == nullptr || e.m_pPreviousParent == m_pManager->GetRootObject())
        {
          // Object is currently a root object, nothing to do to detach it from its parent.
          break;
        }

        if (e.GetProperty()->GetCategory() == xiiPropertyCategory::Set && e.m_pPreviousParent == e.m_pNewParent)
        {
          // Sets only have ordering in the editor. We can ignore set order changes in the mirror.
          break;
        }

        xiiObjectChange change;
        CreatePath(change, e.m_pPreviousParent, e.m_sParentProperty);

        // Do not delete heap object, just remove it from its owner.
        change.m_Change.m_Operation = xiiObjectChangeType::PropertyRemoved;
        change.m_Change.m_Index     = e.m_OldPropertyIndex;
        change.m_Change.m_Value     = e.m_pObject->GetGuid();

        ApplyOp(change);
        break;
      }
      else
      {
        xiiObjectChange change;
        CreatePath(change, e.m_pPreviousParent, e.m_sParentProperty);

        change.m_Change.m_Operation = xiiObjectChangeType::PropertyRemoved;
        change.m_Change.m_Index     = e.m_OldPropertyIndex;
        change.m_Change.m_Value     = e.m_pObject->GetGuid();

        ApplyOp(change);
        break;
      }
    }
      // case xiiDocumentObjectStructureEvent::Type::AfterObjectRemoved:
    case xiiDocumentObjectStructureEvent::Type::BeforeObjectRemoved:
    {
      xiiObjectChange change;
      CreatePath(change, e.m_pPreviousParent, e.m_sParentProperty);

      change.m_Change.m_Operation = xiiObjectChangeType::NodeRemoved;
      change.m_Change.m_Index     = e.m_OldPropertyIndex;
      change.m_Change.m_Value     = e.m_pObject->GetGuid();

      ApplyOp(change);
    }
    break;

    default:
      break;
  }
}

void xiiDocumentObjectMirror::TreePropertyEventHandler(const xiiDocumentObjectPropertyEvent& e)
{
  if (IsDiscardedByFilter(e.m_pObject, e.m_sProperty))
    return;

  switch (e.m_EventType)
  {
    case xiiDocumentObjectPropertyEvent::Type::PropertySet:
    {
      xiiObjectChange change;
      CreatePath(change, e.m_pObject, e.m_sProperty);

      change.m_Change.m_Operation = xiiObjectChangeType::PropertySet;
      change.m_Change.m_Index     = e.m_NewIndex;
      change.m_Change.m_Value     = e.m_NewValue;
      ApplyOp(change);
    }
    break;
    case xiiDocumentObjectPropertyEvent::Type::PropertyInserted:
    {
      xiiObjectChange change;
      CreatePath(change, e.m_pObject, e.m_sProperty);

      change.m_Change.m_Operation = xiiObjectChangeType::PropertyInserted;
      change.m_Change.m_Index     = e.m_NewIndex;
      change.m_Change.m_Value     = e.m_NewValue;
      ApplyOp(change);
    }
    break;
    case xiiDocumentObjectPropertyEvent::Type::PropertyRemoved:
    {
      xiiObjectChange change;
      CreatePath(change, e.m_pObject, e.m_sProperty);

      change.m_Change.m_Operation = xiiObjectChangeType::PropertyRemoved;
      change.m_Change.m_Index     = e.m_OldIndex;
      change.m_Change.m_Value     = e.m_OldValue;
      ApplyOp(change);
    }
    break;
    case xiiDocumentObjectPropertyEvent::Type::PropertyMoved:
    {
      xiiUInt32 uiOldIndex = e.m_OldIndex.ConvertTo<xiiUInt32>();
      xiiUInt32 uiNewIndex = e.m_NewIndex.ConvertTo<xiiUInt32>();
      XII_ASSERT_DEBUG(e.m_NewValue.IsValid(), "Value must be valid");

      {
        xiiObjectChange change;
        CreatePath(change, e.m_pObject, e.m_sProperty);

        change.m_Change.m_Operation = xiiObjectChangeType::PropertyRemoved;
        change.m_Change.m_Index     = uiOldIndex;
        change.m_Change.m_Value     = e.m_NewValue;
        ApplyOp(change);
      }

      if (uiNewIndex > uiOldIndex)
      {
        uiNewIndex -= 1;
      }

      {
        xiiObjectChange change;
        CreatePath(change, e.m_pObject, e.m_sProperty);

        change.m_Change.m_Operation = xiiObjectChangeType::PropertyInserted;
        change.m_Change.m_Index     = uiNewIndex;
        change.m_Change.m_Value     = e.m_NewValue;
        ApplyOp(change);
      }

      return;
    }
    break;
  }
}

void* xiiDocumentObjectMirror::GetNativeObjectPointer(const xiiDocumentObject* pObject)
{
  auto object = m_pContext->GetObjectByGUID(pObject->GetGuid());
  return object.m_pObject;
}

const void* xiiDocumentObjectMirror::GetNativeObjectPointer(const xiiDocumentObject* pObject) const
{
  auto object = m_pContext->GetObjectByGUID(pObject->GetGuid());
  return object.m_pObject;
}

bool xiiDocumentObjectMirror::IsRootObject(const xiiDocumentObject* pParent)
{
  return (pParent == nullptr || pParent == m_pManager->GetRootObject());
}

bool xiiDocumentObjectMirror::IsHeapAllocated(const xiiDocumentObject* pParent, const char* szParentProperty)
{
  if (pParent == nullptr || pParent == m_pManager->GetRootObject())
    return true;

  const xiiRTTI* pRtti = pParent->GetTypeAccessor().GetType();

  auto* pProp = pRtti->FindPropertyByName(szParentProperty);
  return pProp->GetFlags().IsSet(xiiPropertyFlags::PointerOwner);
}


bool xiiDocumentObjectMirror::IsDiscardedByFilter(const xiiDocumentObject* pObject, const char* szProperty) const
{
  if (m_Filter.IsValid())
  {
    return !m_Filter(pObject, szProperty);
  }
  return false;
}

void xiiDocumentObjectMirror::CreatePath(xiiObjectChange& out_change, const xiiDocumentObject* pRoot, const char* szProperty)
{
  if (pRoot && pRoot->GetDocumentObjectManager()->GetRootObject() != pRoot)
  {
    xiiHybridArray<const xiiDocumentObject*, 8> path;
    out_change.m_Root = FindRootOpObject(pRoot, path);
    FlattenSteps(path, out_change.m_Steps);
  }

  out_change.m_Change.m_sProperty = szProperty;
}

xiiUuid xiiDocumentObjectMirror::FindRootOpObject(const xiiDocumentObject* pParent, xiiHybridArray<const xiiDocumentObject*, 8>& path)
{
  path.PushBack(pParent);

  if (!pParent->IsOnHeap())
  {
    return FindRootOpObject(pParent->GetParent(), path);
  }
  else
  {
    return pParent->GetGuid();
  }
}

void xiiDocumentObjectMirror::FlattenSteps(const xiiArrayPtr<const xiiDocumentObject* const> path, xiiHybridArray<xiiPropertyPathStep, 2>& out_steps)
{
  xiiUInt32 uiCount = path.GetCount();
  XII_ASSERT_DEV(uiCount > 0, "Path must not be empty!");
  XII_ASSERT_DEV(path[uiCount - 1]->IsOnHeap(), "Root of steps must be on heap!");

  // Only root object? Then there is no path from it.
  if (uiCount == 1)
    return;

  for (xiiInt32 i = (xiiInt32)uiCount - 2; i >= 0; --i)
  {
    const xiiDocumentObject* pObject = path[i];
    out_steps.PushBack(xiiPropertyPathStep({pObject->GetParentProperty(), pObject->GetPropertyIndex()}));
  }
}

void xiiDocumentObjectMirror::ApplyOp(xiiObjectChange& change)
{
  xiiRttiConverterObject object;
  if (change.m_Root.IsValid())
  {
    object = m_pContext->GetObjectByGUID(change.m_Root);
    if (!object.m_pObject)
      return;
    // XII_ASSERT_DEV(object.m_pObject != nullptr, "Root object does not exist in mirrored native object!");
  }

  xiiPropertyPath propPath;
  if (propPath.InitializeFromPath(*object.m_pType, change.m_Steps).Failed())
  {
    xiiLog::Error("Failed to init property path on object of type '{0}'.", object.m_pType->GetTypeName());
    return;
  }
  propPath.WriteToLeafObject(
            object.m_pObject, *object.m_pType, [this, &change](void* pLeaf, const xiiRTTI& type) { ApplyOp(xiiRttiConverterObject(&type, pLeaf), change); })
    .IgnoreResult();
}

void xiiDocumentObjectMirror::ApplyOp(xiiRttiConverterObject object, const xiiObjectChange& change)
{
  xiiAbstractProperty* pProp = nullptr;

  if (object.m_pType != nullptr)
  {
    pProp = object.m_pType->FindPropertyByName(change.m_Change.m_sProperty);
    if (pProp == nullptr)
    {
      xiiLog::Error("Property '{0}' not found, can't apply mirror op!", change.m_Change.m_sProperty);
      return;
    }
  }

  switch (change.m_Change.m_Operation)
  {
    case xiiObjectChangeType::NodeAdded:
    {
      xiiAbstractObjectGraph graph;
      change.GetGraph(graph);
      xiiRttiConverterReader       reader(&graph, m_pContext);
      const xiiAbstractObjectNode* pNode  = graph.GetNodeByName("Object");
      const xiiRTTI*               pType  = xiiRTTI::FindTypeByName(pNode->GetType());
      void*                        pValue = reader.CreateObjectFromNode(pNode);
      if (!pValue)
      {
        // Can't create object, exiting.
        return;
      }

      if (!change.m_Root.IsValid())
      {
        // Create without parent (root element)
        return;
      }

      if (pProp->GetCategory() == xiiPropertyCategory::Member)
      {
        auto pSpecificProp = static_cast<xiiAbstractMemberProperty*>(pProp);
        if (pProp->GetFlags().IsSet(xiiPropertyFlags::Pointer))
        {
          pSpecificProp->SetValuePtr(object.m_pObject, &pValue);
        }
        else
        {
          pSpecificProp->SetValuePtr(object.m_pObject, pValue);
        }
      }
      else if (pProp->GetCategory() == xiiPropertyCategory::Array)
      {
        auto pSpecificProp = static_cast<xiiAbstractArrayProperty*>(pProp);
        if (pProp->GetFlags().IsSet(xiiPropertyFlags::Pointer))
        {
          pSpecificProp->Insert(object.m_pObject, change.m_Change.m_Index.ConvertTo<xiiUInt32>(), &pValue);
        }
        else
        {
          pSpecificProp->Insert(object.m_pObject, change.m_Change.m_Index.ConvertTo<xiiUInt32>(), pValue);
        }
      }
      else if (pProp->GetCategory() == xiiPropertyCategory::Set)
      {
        XII_ASSERT_DEV(pProp->GetFlags().IsSet(xiiPropertyFlags::Pointer), "Set object must always be pointers!");
        auto pSpecificProp = static_cast<xiiAbstractSetProperty*>(pProp);
        xiiReflectionUtils::InsertSetPropertyValue(pSpecificProp, object.m_pObject, xiiVariant(pValue, pType));
      }
      else if (pProp->GetCategory() == xiiPropertyCategory::Map)
      {
        auto pSpecificProp = static_cast<xiiAbstractMapProperty*>(pProp);
        if (pProp->GetFlags().IsSet(xiiPropertyFlags::Pointer))
        {
          pSpecificProp->Insert(object.m_pObject, change.m_Change.m_Index.Get<xiiString>(), &pValue);
        }
        else
        {
          pSpecificProp->Insert(object.m_pObject, change.m_Change.m_Index.Get<xiiString>(), pValue);
        }
      }

      if (!pProp->GetFlags().AreAllSet(xiiPropertyFlags::Pointer | xiiPropertyFlags::PointerOwner))
      {
        m_pContext->DeleteObject(pNode->GetGuid());
      }
    }
    break;
    case xiiObjectChangeType::NodeRemoved:
    {
      if (!change.m_Root.IsValid())
      {
        // Delete root object
        m_pContext->DeleteObject(change.m_Change.m_Value.Get<xiiUuid>());
        return;
      }

      if (pProp->GetCategory() == xiiPropertyCategory::Member)
      {
        auto pSpecificProp = static_cast<xiiAbstractMemberProperty*>(pProp);
        if (!pProp->GetFlags().AreAllSet(xiiPropertyFlags::Pointer | xiiPropertyFlags::PointerOwner))
        {
          xiiLog::Error("Property '{0}' not a pointer, can't remove object!", change.m_Change.m_sProperty);
          return;
        }

        void* pValue = nullptr;
        pSpecificProp->SetValuePtr(object.m_pObject, &pValue);
      }
      else if (pProp->GetCategory() == xiiPropertyCategory::Array)
      {
        auto pSpecificProp = static_cast<xiiAbstractArrayProperty*>(pProp);
        xiiReflectionUtils::RemoveArrayPropertyValue(pSpecificProp, object.m_pObject, change.m_Change.m_Index.ConvertTo<xiiUInt32>());
      }
      else if (pProp->GetCategory() == xiiPropertyCategory::Set)
      {
        XII_ASSERT_DEV(pProp->GetFlags().IsSet(xiiPropertyFlags::Pointer), "Set object must always be pointers!");
        auto pSpecificProp = static_cast<xiiAbstractSetProperty*>(pProp);
        auto valueObject   = m_pContext->GetObjectByGUID(change.m_Change.m_Value.Get<xiiUuid>());
        xiiReflectionUtils::RemoveSetPropertyValue(pSpecificProp, object.m_pObject, xiiVariant(valueObject.m_pObject, valueObject.m_pType));
      }
      else if (pProp->GetCategory() == xiiPropertyCategory::Map)
      {
        auto pSpecificProp = static_cast<xiiAbstractMapProperty*>(pProp);
        pSpecificProp->Remove(object.m_pObject, change.m_Change.m_Index.Get<xiiString>());
      }

      if (pProp->GetFlags().AreAllSet(xiiPropertyFlags::Pointer | xiiPropertyFlags::PointerOwner))
      {
        m_pContext->DeleteObject(change.m_Change.m_Value.Get<xiiUuid>());
      }
    }
    break;
    case xiiObjectChangeType::PropertySet:
    {
      if (pProp->GetCategory() == xiiPropertyCategory::Member)
      {
        auto pSpecificProp = static_cast<xiiAbstractMemberProperty*>(pProp);
        xiiReflectionUtils::SetMemberPropertyValue(pSpecificProp, object.m_pObject, change.m_Change.m_Value);
      }
      else if (pProp->GetCategory() == xiiPropertyCategory::Array)
      {
        auto pSpecificProp = static_cast<xiiAbstractArrayProperty*>(pProp);
        xiiReflectionUtils::SetArrayPropertyValue(
          pSpecificProp, object.m_pObject, change.m_Change.m_Index.ConvertTo<xiiUInt32>(), change.m_Change.m_Value);
      }
      else if (pProp->GetCategory() == xiiPropertyCategory::Set)
      {
        auto pSpecificProp = static_cast<xiiAbstractSetProperty*>(pProp);
        xiiReflectionUtils::InsertSetPropertyValue(pSpecificProp, object.m_pObject, change.m_Change.m_Value);
      }
      else if (pProp->GetCategory() == xiiPropertyCategory::Map)
      {
        auto pSpecificProp = static_cast<xiiAbstractMapProperty*>(pProp);
        xiiReflectionUtils::SetMapPropertyValue(pSpecificProp, object.m_pObject, change.m_Change.m_Index.Get<xiiString>(), change.m_Change.m_Value);
      }
    }
    break;
    case xiiObjectChangeType::PropertyInserted:
    {
      xiiVariant value = change.m_Change.m_Value;
      if (pProp->GetFlags().IsSet(xiiPropertyFlags::Pointer))
      {
        auto valueObject = m_pContext->GetObjectByGUID(change.m_Change.m_Value.Get<xiiUuid>());
        value            = xiiTypedPointer(valueObject.m_pObject, valueObject.m_pType);
      }

      if (pProp->GetCategory() == xiiPropertyCategory::Array)
      {
        auto pSpecificProp = static_cast<xiiAbstractArrayProperty*>(pProp);
        xiiReflectionUtils::InsertArrayPropertyValue(pSpecificProp, object.m_pObject, value, change.m_Change.m_Index.ConvertTo<xiiUInt32>());
      }
      else if (pProp->GetCategory() == xiiPropertyCategory::Set)
      {
        auto pSpecificProp = static_cast<xiiAbstractSetProperty*>(pProp);
        xiiReflectionUtils::InsertSetPropertyValue(pSpecificProp, object.m_pObject, value);
      }
      else if (pProp->GetCategory() == xiiPropertyCategory::Map)
      {
        auto pSpecificProp = static_cast<xiiAbstractMapProperty*>(pProp);
        xiiReflectionUtils::SetMapPropertyValue(pSpecificProp, object.m_pObject, change.m_Change.m_Index.Get<xiiString>(), value);
      }
    }
    break;
    case xiiObjectChangeType::PropertyRemoved:
    {
      if (pProp->GetCategory() == xiiPropertyCategory::Array)
      {
        auto pSpecificProp = static_cast<xiiAbstractArrayProperty*>(pProp);
        xiiReflectionUtils::RemoveArrayPropertyValue(pSpecificProp, object.m_pObject, change.m_Change.m_Index.ConvertTo<xiiUInt32>());
      }
      else if (pProp->GetCategory() == xiiPropertyCategory::Set)
      {
        xiiVariant value = change.m_Change.m_Value;
        if (pProp->GetFlags().IsSet(xiiPropertyFlags::Pointer))
        {
          auto valueObject = m_pContext->GetObjectByGUID(change.m_Change.m_Value.Get<xiiUuid>());
          value            = xiiTypedPointer(valueObject.m_pObject, valueObject.m_pType);
        }

        auto pSpecificProp = static_cast<xiiAbstractSetProperty*>(pProp);
        xiiReflectionUtils::RemoveSetPropertyValue(pSpecificProp, object.m_pObject, value);
      }
      else if (pProp->GetCategory() == xiiPropertyCategory::Map)
      {
        auto pSpecificProp = static_cast<xiiAbstractMapProperty*>(pProp);
        pSpecificProp->Remove(object.m_pObject, change.m_Change.m_Index.Get<xiiString>());
      }
    }
    break;
  }
}

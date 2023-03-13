#include <Core/CorePCH.h>

#include <Core/WorldSerializer/WorldWriter.h>
#include <Foundation/IO/MemoryStream.h>
#include <Foundation/IO/StringDeduplicationContext.h>

void xiiWorldWriter::Clear()
{
  m_AllRootObjects.Clear();
  m_AllChildObjects.Clear();
  m_AllComponents.Clear();

  m_pStream  = nullptr;
  m_pExclude = nullptr;

  // invalid handles
  {
    m_WrittenGameObjectHandles.Clear();
    m_WrittenGameObjectHandles[xiiGameObjectHandle()] = 0;
  }
}

void xiiWorldWriter::WriteWorld(xiiStreamWriter& stream, xiiWorld& world, const xiiTagSet* pExclude)
{
  Clear();

  m_pStream  = &stream;
  m_pExclude = pExclude;

  XII_LOCK(world.GetReadMarker());

  world.Traverse(xiiMakeDelegate(&xiiWorldWriter::ObjectTraverser, this), xiiWorld::TraversalMethod::DepthFirst);

  WriteToStream().IgnoreResult();
}

void xiiWorldWriter::WriteObjects(xiiStreamWriter& stream, const xiiDeque<const xiiGameObject*>& rootObjects)
{
  Clear();

  m_pStream = &stream;

  for (const xiiGameObject* pObject : rootObjects)
  {
    // traversal function takes a non-const object, but we only read it anyway
    Traverse(const_cast<xiiGameObject*>(pObject));
  }

  WriteToStream().IgnoreResult();
}

void xiiWorldWriter::WriteObjects(xiiStreamWriter& stream, xiiArrayPtr<const xiiGameObject*> rootObjects)
{
  Clear();

  m_pStream = &stream;

  for (const xiiGameObject* pObject : rootObjects)
  {
    // traversal function takes a non-const object, but we only read it anyway
    Traverse(const_cast<xiiGameObject*>(pObject));
  }

  WriteToStream().IgnoreResult();
}

xiiResult xiiWorldWriter::WriteToStream()
{
  const xiiUInt8 uiVersion = 10;
  *m_pStream << uiVersion;

  // version 8: use string dedup instead of handle writer
  xiiStringDeduplicationWriteContext stringDedupWriteContext(*m_pStream);
  m_pStream = &stringDedupWriteContext.Begin();

  IncludeAllComponentBaseTypes();

  xiiUInt32 uiNumRootObjects    = m_AllRootObjects.GetCount();
  xiiUInt32 uiNumChildObjects   = m_AllChildObjects.GetCount();
  xiiUInt32 uiNumComponentTypes = m_AllComponents.GetCount();

  *m_pStream << uiNumRootObjects;
  *m_pStream << uiNumChildObjects;
  *m_pStream << uiNumComponentTypes;

  // this is used to sort all component types by name, to make the file serialization deterministic
  xiiMap<xiiString, const xiiRTTI*> sortedTypes;

  for (auto it = m_AllComponents.GetIterator(); it.IsValid(); ++it)
  {
    sortedTypes[it.Key()->GetTypeName()] = it.Key();
  }

  AssignGameObjectIndices();
  AssignComponentHandleIndices(sortedTypes);

  for (const auto* pObject : m_AllRootObjects)
  {
    WriteGameObject(pObject);
  }

  for (const auto* pObject : m_AllChildObjects)
  {
    WriteGameObject(pObject);
  }

  for (auto it = sortedTypes.GetIterator(); it.IsValid(); ++it)
  {
    WriteComponentTypeInfo(it.Value());
  }

  for (auto it = sortedTypes.GetIterator(); it.IsValid(); ++it)
  {
    WriteComponentCreationData(m_AllComponents[it.Value()].m_Components);
  }

  for (auto it = sortedTypes.GetIterator(); it.IsValid(); ++it)
  {
    WriteComponentSerializationData(m_AllComponents[it.Value()].m_Components);
  }

  XII_SUCCEED_OR_RETURN(stringDedupWriteContext.End());
  m_pStream = &stringDedupWriteContext.GetOriginalStream();

  return XII_SUCCESS;
}


void xiiWorldWriter::AssignGameObjectIndices()
{
  xiiUInt32 uiGameObjectIndex = 1;
  for (const auto* pObject : m_AllRootObjects)
  {
    m_WrittenGameObjectHandles[pObject->GetHandle()] = uiGameObjectIndex;
    ++uiGameObjectIndex;
  }

  for (const auto* pObject : m_AllChildObjects)
  {
    m_WrittenGameObjectHandles[pObject->GetHandle()] = uiGameObjectIndex;
    ++uiGameObjectIndex;
  }
}

void xiiWorldWriter::AssignComponentHandleIndices(const xiiMap<xiiString, const xiiRTTI*>& sortedTypes)
{
  xiiUInt16 uiTypeIndex = 0;

  XII_ASSERT_DEV(m_AllComponents.GetCount() <= xiiMath::MaxValue<xiiUInt16>(), "Too many types for world writer");

  // assign the component handle indices in the order in which the components are written
  for (auto it = sortedTypes.GetIterator(); it.IsValid(); ++it)
  {
    auto& components = m_AllComponents[it.Value()];

    components.m_uiSerializedTypeIndex = uiTypeIndex;
    ++uiTypeIndex;

    xiiUInt32 uiComponentIndex                       = 1;
    components.m_HandleToIndex[xiiComponentHandle()] = 0;

    for (const xiiComponent* pComp : components.m_Components)
    {
      components.m_HandleToIndex[pComp->GetHandle()] = uiComponentIndex;
      ++uiComponentIndex;
    }
  }
}


void xiiWorldWriter::IncludeAllComponentBaseTypes()
{
  xiiDynamicArray<const xiiRTTI*> allNow;
  allNow.Reserve(m_AllComponents.GetCount());
  for (auto it = m_AllComponents.GetIterator(); it.IsValid(); ++it)
  {
    allNow.PushBack(it.Key());
  }

  for (auto pRtti : allNow)
  {
    IncludeAllComponentBaseTypes(pRtti->GetParentType());
  }
}


void xiiWorldWriter::IncludeAllComponentBaseTypes(const xiiRTTI* pRtti)
{
  if (pRtti == nullptr || !pRtti->IsDerivedFrom<xiiComponent>() || m_AllComponents.Contains(pRtti))
    return;

  // this is actually used to insert the type, but we have no component of this type
  m_AllComponents[pRtti];

  IncludeAllComponentBaseTypes(pRtti->GetParentType());
}


void xiiWorldWriter::Traverse(xiiGameObject* pObject)
{
  if (ObjectTraverser(pObject) == xiiVisitorExecution::Continue)
  {
    for (auto it = pObject->GetChildren(); it.IsValid(); ++it)
    {
      Traverse(&(*it));
    }
  }
}

void xiiWorldWriter::WriteGameObjectHandle(const xiiGameObjectHandle& hObject)
{
  auto it = m_WrittenGameObjectHandles.Find(hObject);

  xiiUInt32 uiIndex = 0;

  XII_ASSERT_DEV(it.IsValid(), "Referenced object does not exist in the scene. This can happen, if it was optimized away, because it had no name, no children and no essential components.");

  if (it.IsValid())
    uiIndex = it.Value();

  *m_pStream << uiIndex;
}

void xiiWorldWriter::WriteComponentHandle(const xiiComponentHandle& hComponent)
{
  xiiUInt16 uiTypeIndex = 0;
  xiiUInt32 uiIndex     = 0;

  xiiComponent* pComponent = nullptr;
  if (xiiWorld::GetWorld(hComponent)->TryGetComponent(hComponent, pComponent))
  {
    if (auto* components = m_AllComponents.GetValue(pComponent->GetDynamicRTTI()))
    {
      auto it = components->m_HandleToIndex.Find(hComponent);
      XII_ASSERT_DEBUG(it.IsValid(), "Handle should always be in the written map at this point");

      if (it.IsValid())
      {
        uiTypeIndex = components->m_uiSerializedTypeIndex;
        uiIndex     = it.Value();
      }
    }
  }

  *m_pStream << uiTypeIndex;
  *m_pStream << uiIndex;
}

xiiVisitorExecution::Enum xiiWorldWriter::ObjectTraverser(xiiGameObject* pObject)
{
  if (m_pExclude && pObject->GetTags().IsAnySet(*m_pExclude))
    return xiiVisitorExecution::Skip;

  if (pObject->GetParent())
    m_AllChildObjects.PushBack(pObject);
  else
    m_AllRootObjects.PushBack(pObject);

  auto components = pObject->GetComponents();

  for (const xiiComponent* pComp : components)
  {
    m_AllComponents[pComp->GetDynamicRTTI()].m_Components.PushBack(pComp);
  }

  return xiiVisitorExecution::Continue;
}

void xiiWorldWriter::WriteGameObject(const xiiGameObject* pObject)
{
  if (pObject->GetParent())
    WriteGameObjectHandle(pObject->GetParent()->GetHandle());
  else
    WriteGameObjectHandle(xiiGameObjectHandle());

  xiiStreamWriter& s = *m_pStream;

  s << pObject->GetName();
  s << pObject->GetGlobalKey();
  s << pObject->GetLocalPosition();
  s << pObject->GetLocalRotation();
  s << pObject->GetLocalScaling();
  s << pObject->GetLocalUniformScaling();
  s << pObject->GetActiveFlag();
  s << pObject->IsDynamic();
  pObject->GetTags().Save(s);
  s << pObject->GetTeamID();
  s << pObject->GetStableRandomSeed();
}

void xiiWorldWriter::WriteComponentTypeInfo(const xiiRTTI* pRtti)
{
  xiiStreamWriter& s = *m_pStream;

  s << pRtti->GetTypeName();
  s << pRtti->GetTypeVersion();
}

void xiiWorldWriter::WriteComponentCreationData(const xiiDeque<const xiiComponent*>& components)
{
  xiiDefaultMemoryStreamStorage storage;
  xiiMemoryStreamWriter         memWriter(&storage);

  xiiStreamWriter* pPrevStream = m_pStream;
  m_pStream                    = &memWriter;

  // write to memory stream
  {
    xiiStreamWriter& s = *m_pStream;
    s << components.GetCount();

    xiiUInt32 uiComponentIndex = 1;
    for (auto pComponent : components)
    {
      WriteGameObjectHandle(pComponent->GetOwner()->GetHandle());
      s << uiComponentIndex;
      ++uiComponentIndex;

      s << pComponent->GetActiveFlag();

      // version 7
      {
        xiiUInt8 userFlags = 0;
        for (xiiUInt8 i = 0; i < 8; ++i)
        {
          userFlags |= pComponent->GetUserFlag(i) ? XII_BIT(i) : 0;
        }

        s << userFlags;
      }
    }
  }

  m_pStream = pPrevStream;

  // write result to actual stream
  {
    xiiStreamWriter& s = *m_pStream;
    s << storage.GetStorageSize32();

    XII_ASSERT_ALWAYS(storage.GetStorageSize64() <= xiiMath::MaxValue<xiiUInt32>(), "Slight file format change and version increase needed to support > 4GB worlds.");

    storage.CopyToStream(s).IgnoreResult();
  }
}

void xiiWorldWriter::WriteComponentSerializationData(const xiiDeque<const xiiComponent*>& components)
{
  xiiDefaultMemoryStreamStorage storage;
  xiiMemoryStreamWriter         memWriter(&storage);

  xiiStreamWriter* pPrevStream = m_pStream;
  m_pStream                    = &memWriter;

  // write to memory stream
  for (auto pComp : components)
  {
    pComp->SerializeComponent(*this);
  }

  m_pStream = pPrevStream;

  // write result to actual stream
  {
    xiiStreamWriter& s = *m_pStream;
    s << storage.GetStorageSize32();

    XII_ASSERT_ALWAYS(storage.GetStorageSize64() <= xiiMath::MaxValue<xiiUInt32>(), "Slight file format change and version increase needed to support > 4GB worlds.");

    storage.CopyToStream(s).IgnoreResult();
  }
}

XII_STATICLINK_FILE(Core, Core_WorldSerializer_Implementation_WorldWriter);

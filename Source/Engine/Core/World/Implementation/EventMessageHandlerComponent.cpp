#include <Core/CorePCH.h>

#include <Core/World/EventMessageHandlerComponent.h>
#include <Core/World/World.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>

namespace
{
  static xiiStaticArray<xiiDynamicArray<xiiComponentHandle>*, 64> s_GlobalEventHandlerPerWorld;

  static void RegisterGlobalEventHandler(xiiComponent* pComponent)
  {
    const xiiUInt32 uiWorldIndex = pComponent->GetWorld()->GetIndex();
    s_GlobalEventHandlerPerWorld.EnsureCount(uiWorldIndex + 1);

    auto globalEventHandler = s_GlobalEventHandlerPerWorld[uiWorldIndex];
    if (globalEventHandler == nullptr)
    {
      globalEventHandler = XII_NEW(xiiStaticAllocatorWrapper::GetAllocator(), xiiDynamicArray<xiiComponentHandle>);

      s_GlobalEventHandlerPerWorld[uiWorldIndex] = globalEventHandler;
    }

    globalEventHandler->PushBack(pComponent->GetHandle());
  }

  static void DeregisterGlobalEventHandler(xiiComponent* pComponent)
  {
    xiiUInt32 uiWorldIndex       = pComponent->GetWorld()->GetIndex();
    auto      globalEventHandler = s_GlobalEventHandlerPerWorld[uiWorldIndex];
    XII_ASSERT_DEV(globalEventHandler != nullptr, "Implementation error.");

    globalEventHandler->RemoveAndSwap(pComponent->GetHandle());
  }
} // namespace

//////////////////////////////////////////////////////////////////////////

// clang-format off
XII_BEGIN_ABSTRACT_COMPONENT_TYPE(xiiEventMessageHandlerComponent, 3)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ACCESSOR_PROPERTY("HandleGlobalEvents", GetGlobalEventHandlerMode, SetGlobalEventHandlerMode),
    XII_ACCESSOR_PROPERTY("PassThroughUnhandledEvents", GetPassThroughUnhandledEvents, SetPassThroughUnhandledEvents),
  }
  XII_END_PROPERTIES;
}
XII_END_ABSTRACT_COMPONENT_TYPE;
// clang-format on

xiiEventMessageHandlerComponent::xiiEventMessageHandlerComponent()  = default;
xiiEventMessageHandlerComponent::~xiiEventMessageHandlerComponent() = default;

void xiiEventMessageHandlerComponent::SerializeComponent(xiiWorldWriter& ref_stream) const
{
  SUPER::SerializeComponent(ref_stream);
  auto& s = ref_stream.GetStream();

  // version 2
  s << m_bIsGlobalEventHandler;

  // version 3
  s << m_bPassThroughUnhandledEvents;
}

void xiiEventMessageHandlerComponent::DeserializeComponent(xiiWorldReader& ref_stream)
{
  SUPER::DeserializeComponent(ref_stream);
  const xiiUInt32 uiVersion = ref_stream.GetComponentTypeVersion(GetStaticRTTI());
  auto&           s         = ref_stream.GetStream();

  if (uiVersion >= 2)
  {
    bool bGlobalEH;
    s >> bGlobalEH;

    SetGlobalEventHandlerMode(bGlobalEH);
  }

  if (uiVersion >= 3)
  {
    s >> m_bPassThroughUnhandledEvents;
  }
}

void xiiEventMessageHandlerComponent::Deinitialize()
{
  SetGlobalEventHandlerMode(false);

  SUPER::Deinitialize();
}

void xiiEventMessageHandlerComponent::SetDebugOutput(bool bEnable)
{
  m_bDebugOutput = bEnable;
}

bool xiiEventMessageHandlerComponent::GetDebugOutput() const
{
  return m_bDebugOutput;
}

void xiiEventMessageHandlerComponent::SetGlobalEventHandlerMode(bool bEnable)
{
  if (m_bIsGlobalEventHandler == bEnable)
    return;

  m_bIsGlobalEventHandler = bEnable;

  if (bEnable)
  {
    RegisterGlobalEventHandler(this);
  }
  else
  {
    DeregisterGlobalEventHandler(this);
  }
}

void xiiEventMessageHandlerComponent::SetPassThroughUnhandledEvents(bool bPassThrough)
{
  m_bPassThroughUnhandledEvents = bPassThrough;
}

// static
xiiArrayPtr<xiiComponentHandle> xiiEventMessageHandlerComponent::GetAllGlobalEventHandler(const xiiWorld* pWorld)
{
  xiiUInt32 uiWorldIndex = pWorld->GetIndex();

  if (uiWorldIndex < s_GlobalEventHandlerPerWorld.GetCount())
  {
    if (auto globalEventHandler = s_GlobalEventHandlerPerWorld[uiWorldIndex])
    {
      return globalEventHandler->GetArrayPtr();
    }
  }

  return xiiArrayPtr<xiiComponentHandle>();
}


void xiiEventMessageHandlerComponent::ClearGlobalEventHandlersForWorld(const xiiWorld* pWorld)
{
  xiiUInt32 uiWorldIndex = pWorld->GetIndex();

  if (uiWorldIndex < s_GlobalEventHandlerPerWorld.GetCount())
  {
    s_GlobalEventHandlerPerWorld[uiWorldIndex]->Clear();
  }
}

XII_STATICLINK_FILE(Core, Core_World_Implementation_EventMessageHandlerComponent);

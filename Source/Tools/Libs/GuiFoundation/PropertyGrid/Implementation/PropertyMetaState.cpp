#include <GuiFoundation/GuiFoundationPCH.h>

#include <GuiFoundation/PropertyGrid/PropertyMetaState.h>
#include <ToolsFoundation/Serialization/DocumentObjectConverter.h>

// clang-format off
XII_IMPLEMENT_SINGLETON(xiiPropertyMetaState);

XII_BEGIN_SUBSYSTEM_DECLARATION(GuiFoundation, PropertyMetaState)

  ON_CORESYSTEMS_STARTUP
  {
    XII_DEFAULT_NEW(xiiPropertyMetaState);
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    if (xiiPropertyMetaState::GetSingleton())
    {
      auto ptr = xiiPropertyMetaState::GetSingleton();
      XII_DEFAULT_DELETE(ptr);
    }
  }

XII_END_SUBSYSTEM_DECLARATION;
// clang-format on

xiiPropertyMetaState::xiiPropertyMetaState() :
  m_SingletonRegistrar(this)
{
}

void xiiPropertyMetaState::GetTypePropertiesState(const xiiDocumentObject* pObject, xiiMap<xiiString, xiiPropertyUiState>& out_propertyStates)
{
  xiiPropertyMetaStateEvent eventData;
  eventData.m_pPropertyStates = &out_propertyStates;
  eventData.m_pObject         = pObject;

  m_Events.Broadcast(eventData);
}

void xiiPropertyMetaState::GetTypePropertiesState(const xiiHybridArray<xiiPropertySelection, 8>& items, xiiMap<xiiString, xiiPropertyUiState>& out_propertyStates)
{
  for (const auto& sel : items)
  {
    m_Temp.Clear();
    GetTypePropertiesState(sel.m_pObject, m_Temp);

    for (auto it = m_Temp.GetIterator(); it.IsValid(); ++it)
    {
      auto& curState = out_propertyStates[it.Key()];

      curState.m_Visibility    = xiiMath::Max(curState.m_Visibility, it.Value().m_Visibility);
      curState.m_sNewLabelText = it.Value().m_sNewLabelText;
    }
  }
}

void xiiPropertyMetaState::GetContainerElementsState(const xiiDocumentObject* pObject, const char* szProperty, xiiHashTable<xiiVariant, xiiPropertyUiState>& out_propertyStates)
{
  xiiContainerElementMetaStateEvent eventData;
  eventData.m_pContainerElementStates = &out_propertyStates;
  eventData.m_pObject                 = pObject;
  eventData.m_szProperty              = szProperty;

  m_ContainerEvents.Broadcast(eventData);
}

void xiiPropertyMetaState::GetContainerElementsState(const xiiHybridArray<xiiPropertySelection, 8>& items, const char* szProperty, xiiHashTable<xiiVariant, xiiPropertyUiState>& out_propertyStates)
{
  for (const auto& sel : items)
  {
    m_Temp2.Clear();
    GetContainerElementsState(sel.m_pObject, szProperty, m_Temp2);

    for (auto it = m_Temp2.GetIterator(); it.IsValid(); ++it)
    {
      auto& curState = out_propertyStates[it.Key()];

      curState.m_Visibility    = xiiMath::Max(curState.m_Visibility, it.Value().m_Visibility);
      curState.m_sNewLabelText = it.Value().m_sNewLabelText;
    }
  }
}

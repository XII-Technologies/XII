#include <GameEngine/GameEnginePCH.h>

#include <Core/World/GameObject.h>
#include <Core/World/World.h>
#include <GameEngine/VisualScript/Nodes/VisualScriptReferenceNodes.h>
#include <GameEngine/VisualScript/VisualScriptInstance.h>

//////////////////////////////////////////////////////////////////////////

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiVisualScriptNode_GetScriptOwner, 2, xiiRTTIDefaultAllocator<xiiVisualScriptNode_GetScriptOwner>)
{
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("References")
  }
    XII_END_ATTRIBUTES;
    XII_BEGIN_PROPERTIES
  {
    // Data Pins (Output)
    XII_OUTPUT_DATA_PIN("Object", 0, xiiVisualScriptDataPinType::GameObjectHandle),
    XII_OUTPUT_DATA_PIN("Component", 1, xiiVisualScriptDataPinType::ComponentHandle),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiVisualScriptNode_GetScriptOwner::xiiVisualScriptNode_GetScriptOwner()  = default;
xiiVisualScriptNode_GetScriptOwner::~xiiVisualScriptNode_GetScriptOwner() = default;

void xiiVisualScriptNode_GetScriptOwner::Execute(xiiVisualScriptInstance* pInstance, xiiUInt8 uiExecPin)
{
  // we have no input values here that could change, but this will still be executed once after initial startup
  // after that the value will not change, so no need to re-execute
  if (m_bInputValuesChanged)
  {
    xiiGameObjectHandle hObject = pInstance->GetOwner();
    pInstance->SetOutputPinValue(this, 0, &hObject);

    xiiComponentHandle hComponent = pInstance->GetOwnerComponent();
    pInstance->SetOutputPinValue(this, 1, &hComponent);
  }
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiVisualScriptNode_GetComponentOwner, 1, xiiRTTIDefaultAllocator<xiiVisualScriptNode_GetComponentOwner>)
{
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("References")
  }
    XII_END_ATTRIBUTES;
    XII_BEGIN_PROPERTIES
  {
    // Data Pins (Input)
    XII_INPUT_DATA_PIN("Component", 0, xiiVisualScriptDataPinType::ComponentHandle),
    // Data Pins (Output)
    XII_OUTPUT_DATA_PIN("Object", 0, xiiVisualScriptDataPinType::GameObjectHandle),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiVisualScriptNode_GetComponentOwner::xiiVisualScriptNode_GetComponentOwner()  = default;
xiiVisualScriptNode_GetComponentOwner::~xiiVisualScriptNode_GetComponentOwner() = default;

void xiiVisualScriptNode_GetComponentOwner::Execute(xiiVisualScriptInstance* pInstance, xiiUInt8 uiExecPin)
{
  if (m_bInputValuesChanged)
  {
    xiiComponent*       pComponent = nullptr;
    xiiGameObjectHandle hObject;

    if (pInstance->GetWorld()->TryGetComponent(m_hComponent, pComponent))
    {
      hObject = pComponent->GetOwner()->GetHandle();
    }

    pInstance->SetOutputPinValue(this, 0, &hObject);
  }
}

void* xiiVisualScriptNode_GetComponentOwner::GetInputPinDataPointer(xiiUInt8 uiPin)
{
  switch (uiPin)
  {
    case 0:
      return &m_hComponent;
  }

  return nullptr;
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiVisualScriptNode_FindChildObject, 1, xiiRTTIDefaultAllocator<xiiVisualScriptNode_FindChildObject>)
{
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("References"),
    new xiiTitleAttribute("Child '{Name}'"),
  }
  XII_END_ATTRIBUTES;
    XII_BEGIN_PROPERTIES
  {
    // Data Pins (Input)
    XII_INPUT_DATA_PIN("Object", 0, xiiVisualScriptDataPinType::GameObjectHandle),
    // Data Pins (Output)
    XII_OUTPUT_DATA_PIN("Child", 0, xiiVisualScriptDataPinType::GameObjectHandle),
    // Exposed Properties
    XII_MEMBER_PROPERTY("Name", m_sChildObjectName),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiVisualScriptNode_FindChildObject::xiiVisualScriptNode_FindChildObject()  = default;
xiiVisualScriptNode_FindChildObject::~xiiVisualScriptNode_FindChildObject() = default;

void xiiVisualScriptNode_FindChildObject::Execute(xiiVisualScriptInstance* pInstance, xiiUInt8 uiExecPin)
{
  if (m_bInputValuesChanged)
  {
    if (m_hObject.IsInvalidated())
    {
      m_hObject = pInstance->GetOwner();
    }

    xiiGameObject* pParent = nullptr;
    if (!pInstance->GetWorld()->TryGetObject(m_hObject, pParent))
    {
      xiiLog::Warning("Script: FindChildObject: Cannot find child '{0}', parent object is already invalid.", m_sChildObjectName);
      return;
    }

    xiiGameObject* pChild = pParent->SearchForChildByNameSequence(m_sChildObjectName.GetData());

    if (pChild == nullptr)
    {
      xiiLog::Warning("Script: Child-Object with Name '{0}' does not exist at node '{1}'.", m_sChildObjectName, pParent->GetName());
      return;
    }

    xiiGameObjectHandle hResult = pChild->GetHandle();
    pInstance->SetOutputPinValue(this, 0, &hResult);
  }
}


void* xiiVisualScriptNode_FindChildObject::GetInputPinDataPointer(xiiUInt8 uiPin)
{
  switch (uiPin)
  {
    case 0:
      return &m_hObject;
  }

  return nullptr;
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiVisualScriptNode_FindComponent, 1, xiiRTTIDefaultAllocator<xiiVisualScriptNode_FindComponent>)
{
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("References"),
    new xiiTitleAttribute("Component '{Type}'"),
  }
  XII_END_ATTRIBUTES;
    XII_BEGIN_PROPERTIES
  {
    // Data Pins (Input)
    XII_INPUT_DATA_PIN("Object", 0, xiiVisualScriptDataPinType::GameObjectHandle),
    // Data Pins (Output)
    XII_OUTPUT_DATA_PIN("Component", 0, xiiVisualScriptDataPinType::ComponentHandle),
    // Exposed Properties
    XII_MEMBER_PROPERTY("Type", m_sType)->AddAttributes(new xiiDynamicStringEnumAttribute("ComponentTypes")),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiVisualScriptNode_FindComponent::xiiVisualScriptNode_FindComponent()  = default;
xiiVisualScriptNode_FindComponent::~xiiVisualScriptNode_FindComponent() = default;

void xiiVisualScriptNode_FindComponent::Execute(xiiVisualScriptInstance* pInstance, xiiUInt8 uiExecPin)
{
  if (m_bInputValuesChanged)
  {
    if (m_hObject.IsInvalidated())
    {
      m_hObject = pInstance->GetOwner();
    }

    xiiGameObject* pParent = nullptr;
    if (!pInstance->GetWorld()->TryGetObject(m_hObject, pParent))
    {
      xiiLog::Warning("Script: FindComponent: Cannot find component '{0}', parent object is already invalid.", m_sType);
      return;
    }

    const xiiRTTI* pRtti = xiiRTTI::FindTypeByName(m_sType);
    if (pRtti == nullptr)
    {
      xiiLog::Error("Script: FindComponent: Component type '{0}' is unknown.", m_sType);
      return;
    }

    xiiComponent* pComponent = nullptr;
    if (!pParent->TryGetComponentOfBaseType(pRtti, pComponent))
    {
      xiiLog::Warning("Script: Component of type '{0}' does not exist at node '{1}'.", m_sType, pParent->GetName());
      return;
    }

    xiiComponentHandle hResult = pComponent->GetHandle();
    pInstance->SetOutputPinValue(this, 0, &hResult);
  }
}

void* xiiVisualScriptNode_FindComponent::GetInputPinDataPointer(xiiUInt8 uiPin)
{
  switch (uiPin)
  {
    case 0:
      return &m_hObject;
  }

  return nullptr;
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiVisualScriptNode_QueryGlobalObject, 1, xiiRTTIDefaultAllocator<xiiVisualScriptNode_QueryGlobalObject>)
{
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("References"),
    new xiiTitleAttribute("Global Object '{Name}'"),
  }
  XII_END_ATTRIBUTES;
  XII_BEGIN_PROPERTIES
  {
    // Data Pins (Input)
    XII_INPUT_DATA_PIN_AND_PROPERTY("Name", 0, xiiVisualScriptDataPinType::String, m_sObjectName),
    // Data Pins (Output)
    XII_OUTPUT_DATA_PIN("Object", 0, xiiVisualScriptDataPinType::GameObjectHandle),    
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiVisualScriptNode_QueryGlobalObject::xiiVisualScriptNode_QueryGlobalObject()  = default;
xiiVisualScriptNode_QueryGlobalObject::~xiiVisualScriptNode_QueryGlobalObject() = default;

void xiiVisualScriptNode_QueryGlobalObject::Execute(xiiVisualScriptInstance* pInstance, xiiUInt8 uiExecPin)
{
  if (m_bInputValuesChanged)
  {
    const xiiTempHashedString name(m_sObjectName.GetData());
    xiiGameObject*            pObject;

    if (!pInstance->GetWorld()->TryGetObjectWithGlobalKey(name, pObject))
    {
      xiiLog::Warning("Script: Object with Global Key '{0}' does not exist.", m_sObjectName);
      return;
    }

    xiiGameObjectHandle hResult = pObject->GetHandle();
    pInstance->SetOutputPinValue(this, 0, &hResult);
  }
}

void* xiiVisualScriptNode_QueryGlobalObject::GetInputPinDataPointer(xiiUInt8 uiPin)
{
  switch (uiPin)
  {
    case 0:
      return &m_sObjectName;
  }

  return nullptr;
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiVisualScriptNode_FindParent, 1, xiiRTTIDefaultAllocator<xiiVisualScriptNode_FindParent>)
{
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("References"),
    new xiiTitleAttribute("Parent '{Name}'"),
  }
    XII_END_ATTRIBUTES;
    XII_BEGIN_PROPERTIES
  {
    // Data Pins (Input)
    XII_INPUT_DATA_PIN("Object", 0, xiiVisualScriptDataPinType::GameObjectHandle),
    // Exposed Properties
    XII_MEMBER_PROPERTY("Name", m_sObjectName),
    // Data Pins (Output)
    XII_OUTPUT_DATA_PIN("Parent", 0, xiiVisualScriptDataPinType::GameObjectHandle),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiVisualScriptNode_FindParent::xiiVisualScriptNode_FindParent()  = default;
xiiVisualScriptNode_FindParent::~xiiVisualScriptNode_FindParent() = default;

void xiiVisualScriptNode_FindParent::Execute(xiiVisualScriptInstance* pInstance, xiiUInt8 uiExecPin)
{
  if (m_bInputValuesChanged)
  {
    const xiiTempHashedString name(m_sObjectName.GetData());
    xiiGameObject*            pObject;

    // 'self' if nothing is connected
    if (m_hObject.IsInvalidated())
    {
      m_hObject = pInstance->GetOwner();
    }

    if (pInstance->GetWorld()->TryGetObject(m_hObject, pObject))
    {
      // skip starting object
      pObject = pObject->GetParent();

      // search for a parent with the given name
      while (pObject != nullptr)
      {
        if (m_sObjectName.IsEmpty() || pObject->HasName(name))
        {
          xiiGameObjectHandle hResult = pObject->GetHandle();
          pInstance->SetOutputPinValue(this, 0, &hResult);
          return;
        }

        pObject = pObject->GetParent();
      }
    }

    xiiLog::Warning("Script: Parent Object with Name '{0}' could not be found.", m_sObjectName);
  }
}

void* xiiVisualScriptNode_FindParent::GetInputPinDataPointer(xiiUInt8 uiPin)
{
  switch (uiPin)
  {
    case 0:
      return &m_hObject;
  }

  return nullptr;
}

//////////////////////////////////////////////////////////////////////////



XII_STATICLINK_FILE(GameEngine, GameEngine_VisualScript_Nodes_Implementation_VisualScriptReferenceNodes);

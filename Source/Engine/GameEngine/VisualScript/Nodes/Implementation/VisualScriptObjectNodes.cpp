#include <GameEngine/GameEnginePCH.h>

#include <Core/World/GameObject.h>
#include <Core/World/World.h>
#include <GameEngine/VisualScript/Nodes/VisualScriptObjectNodes.h>
#include <GameEngine/VisualScript/VisualScriptInstance.h>

//////////////////////////////////////////////////////////////////////////

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiVisualScriptNode_DeleteObject, 1, xiiRTTIDefaultAllocator<xiiVisualScriptNode_DeleteObject>)
{
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("Objects")
  }
  XII_END_ATTRIBUTES;
  XII_BEGIN_PROPERTIES
  {
    // Execution Pins (Input)
    XII_INPUT_EXECUTION_PIN("run", 0),
    // Execution Pins (Output)
    XII_OUTPUT_EXECUTION_PIN("then", 0),
    // Data Pins (Input)
    XII_INPUT_DATA_PIN("Object", 0, xiiVisualScriptDataPinType::GameObjectHandle),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiVisualScriptNode_DeleteObject::xiiVisualScriptNode_DeleteObject() {}

void xiiVisualScriptNode_DeleteObject::Execute(xiiVisualScriptInstance* pInstance, xiiUInt8 uiExecPin)
{
  if (!m_hObject.IsInvalidated())
  {
    pInstance->GetWorld()->DeleteObjectDelayed(m_hObject);
  }
  else
  {
    pInstance->GetWorld()->DeleteObjectDelayed(pInstance->GetOwner());
  }

  pInstance->ExecuteConnectedNodes(this, 0);
}

void* xiiVisualScriptNode_DeleteObject::GetInputPinDataPointer(xiiUInt8 uiPin)
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
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiVisualScriptNode_ActivateObject, 1, xiiRTTIDefaultAllocator<xiiVisualScriptNode_ActivateObject>)
{
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("Objects")
  }
  XII_END_ATTRIBUTES;
  XII_BEGIN_PROPERTIES
  {
    // Execution Pins (Input)
    XII_INPUT_EXECUTION_PIN("Activate", 0),
    XII_INPUT_EXECUTION_PIN("Deactivate", 1),
    // Execution Pins (Output)
    XII_OUTPUT_EXECUTION_PIN("then", 0),
    // Data Pins (Input)
    XII_INPUT_DATA_PIN("Object", 0, xiiVisualScriptDataPinType::GameObjectHandle),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiVisualScriptNode_ActivateObject::xiiVisualScriptNode_ActivateObject() = default;

void xiiVisualScriptNode_ActivateObject::Execute(xiiVisualScriptInstance* pInstance, xiiUInt8 uiExecPin)
{
  xiiGameObject* pObject = nullptr;
  if (pInstance->GetWorld()->TryGetObject(m_hObject, pObject))
  {
    if (uiExecPin == 0)
    {
      pObject->SetActiveFlag(true);
    }
    else
    {
      pObject->SetActiveFlag(false);
    }
  }

  pInstance->ExecuteConnectedNodes(this, 0);
}

void* xiiVisualScriptNode_ActivateObject::GetInputPinDataPointer(xiiUInt8 uiPin)
{
  return &m_hObject;
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiVisualScriptNode_ActivateComponent, 1, xiiRTTIDefaultAllocator<xiiVisualScriptNode_ActivateComponent>)
{
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("Components")
  }
    XII_END_ATTRIBUTES;
    XII_BEGIN_PROPERTIES
  {
    // Execution Pins (Input)
    XII_INPUT_EXECUTION_PIN("Activate", 0),
    XII_INPUT_EXECUTION_PIN("Deactivate", 1),
    // Execution Pins (Output)
    XII_OUTPUT_EXECUTION_PIN("then", 0),
    // Data Pins (Input)
    XII_INPUT_DATA_PIN("Component", 0, xiiVisualScriptDataPinType::ComponentHandle),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiVisualScriptNode_ActivateComponent::xiiVisualScriptNode_ActivateComponent() {}

void xiiVisualScriptNode_ActivateComponent::Execute(xiiVisualScriptInstance* pInstance, xiiUInt8 uiExecPin)
{
  if (!m_hComponent.IsInvalidated())
  {
    xiiComponent* pComponent = nullptr;
    if (pInstance->GetWorld()->TryGetComponent(m_hComponent, pComponent))
    {
      if (uiExecPin == 0)
      {
        pComponent->SetActiveFlag(true);
      }
      else
      {
        pComponent->SetActiveFlag(false);
      }
    }
  }

  pInstance->ExecuteConnectedNodes(this, 0);
}

void* xiiVisualScriptNode_ActivateComponent::GetInputPinDataPointer(xiiUInt8 uiPin)
{
  return &m_hComponent;
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiVisualScriptNode_HasName, 1, xiiRTTIDefaultAllocator<xiiVisualScriptNode_HasName>)
{
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("Objects"),
    new xiiTitleAttribute("HasName '{ObjectName}'")
  }
  XII_END_ATTRIBUTES;
  XII_BEGIN_PROPERTIES
  {
    // Execution Pins
    XII_INPUT_EXECUTION_PIN("run", 0),
    XII_OUTPUT_EXECUTION_PIN("OnTrue", 0),
    XII_OUTPUT_EXECUTION_PIN("OnFalse", 1),
    // Data Pins (Input)
    XII_INPUT_DATA_PIN("Object", 0, xiiVisualScriptDataPinType::GameObjectHandle),
    XII_INPUT_DATA_PIN_AND_PROPERTY("ObjectName", 1, xiiVisualScriptDataPinType::String, m_sObjectName)
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiVisualScriptNode_HasName::xiiVisualScriptNode_HasName() {}

void xiiVisualScriptNode_HasName::Execute(xiiVisualScriptInstance* pInstance, xiiUInt8 uiExecPin)
{
  xiiGameObjectHandle hObject = m_hObject.IsInvalidated() ? pInstance->GetOwner() : m_hObject;

  xiiGameObject* pObject = nullptr;
  if (pInstance->GetWorld()->TryGetObject(hObject, pObject))
  {
    if (m_sObjectName == pObject->GetName())
    {
      pInstance->ExecuteConnectedNodes(this, 0);
      return;
    }
  }

  pInstance->ExecuteConnectedNodes(this, 1);
}

void* xiiVisualScriptNode_HasName::GetInputPinDataPointer(xiiUInt8 uiPin)
{
  switch (uiPin)
  {
    case 0:
      return &m_hObject;
    case 1:
      return &m_sObjectName;
  }

  return nullptr;
}

//////////////////////////////////////////////////////////////////////////



XII_STATICLINK_FILE(GameEngine, GameEngine_VisualScript_Nodes_VisualScriptObjectNodes);

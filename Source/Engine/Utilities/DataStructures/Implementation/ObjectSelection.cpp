/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Utilities/UtilitiesPCH.h>

#include <Utilities/DataStructures/ObjectSelection.h>

xiiObjectSelection::xiiObjectSelection()
{
  m_pWorld = nullptr;
}

void xiiObjectSelection::SetWorld(xiiWorld* pWorld)
{
  XII_ASSERT_DEV((m_pWorld == nullptr) || (m_pWorld == pWorld) || m_Objects.IsEmpty(), "You cannot change the world for this selection.");

  m_pWorld = pWorld;
}

void xiiObjectSelection::RemoveDeadObjects()
{
  XII_ASSERT_DEV(m_pWorld != nullptr, "The world has not been set.");

  for (xiiUInt32 i = m_Objects.GetCount(); i > 0; --i)
  {
    xiiGameObject* pObject;
    if (!m_pWorld->TryGetObject(m_Objects[i - 1], pObject))
    {
      m_Objects.RemoveAtAndCopy(i - 1); // keep the order
    }
  }
}

void xiiObjectSelection::AddObject(xiiGameObjectHandle hObject, bool bDontAddTwice)
{
  XII_ASSERT_DEV(m_pWorld != nullptr, "The world has not been set.");

  // only insert valid objects
  xiiGameObject* pObject;
  if (!m_pWorld->TryGetObject(hObject, pObject))
    return;

  if (m_Objects.IndexOf(hObject) != xiiInvalidIndex)
    return;

  m_Objects.PushBack(hObject);
}

bool xiiObjectSelection::RemoveObject(xiiGameObjectHandle hObject)
{
  return m_Objects.RemoveAndCopy(hObject);
}

void xiiObjectSelection::ToggleSelection(xiiGameObjectHandle hObject)
{
  for (xiiUInt32 i = 0; i < m_Objects.GetCount(); ++i)
  {
    if (m_Objects[i] == hObject)
    {
      m_Objects.RemoveAtAndCopy(i); // keep the order
      return;
    }
  }

  // ensures invalid objects don't get added
  AddObject(hObject);
}



XII_STATICLINK_FILE(Utilities, Utilities_DataStructures_Implementation_ObjectSelection);

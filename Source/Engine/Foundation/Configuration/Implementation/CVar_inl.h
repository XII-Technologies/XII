/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Configuration/CVar.h>

template <typename Type, xiiCVarType::Enum CVarType>
xiiTypedCVar<Type, CVarType>::xiiTypedCVar(xiiStringView sName, const Type& value, xiiBitflags<xiiCVarFlags> flags, xiiStringView sDescription) :
  xiiCVar(sName, flags, sDescription)
{
  XII_ASSERT_DEBUG(sName.FindSubString(" ") == nullptr, "CVar names must not contain whitespace");

  for (xiiUInt32 i = 0; i < xiiCVarValue::ENUM_COUNT; ++i)
    m_Values[i] = value;
}

template <typename Type, xiiCVarType::Enum CVarType>
xiiTypedCVar<Type, CVarType>::operator const Type&() const
{
  return (m_Values[xiiCVarValue::Current]);
}

template <typename Type, xiiCVarType::Enum CVarType>
xiiCVarType::Enum xiiTypedCVar<Type, CVarType>::GetType() const
{
  return CVarType;
}

template <typename Type, xiiCVarType::Enum CVarType>
void xiiTypedCVar<Type, CVarType>::SetToDelayedSyncValue()
{
  if (m_Values[xiiCVarValue::Current] == m_Values[xiiCVarValue::DelayedSync])
    return;

  // This will NOT trigger a 'restart value changed' event.
  m_Values[xiiCVarValue::Current] = m_Values[xiiCVarValue::DelayedSync];

  xiiCVarEvent e(this);
  e.m_EventType = xiiCVarEvent::ValueChanged;
  m_CVarEvents.Broadcast(e);

  // Broadcast the same to the 'all cvars' event handlers.
  s_AllCVarEvents.Broadcast(e);
}

template <typename Type, xiiCVarType::Enum CVarType>
const Type& xiiTypedCVar<Type, CVarType>::GetValue(xiiCVarValue::Enum val) const
{
  return (m_Values[val]);
}

template <typename Type, xiiCVarType::Enum CVarType>
void xiiTypedCVar<Type, CVarType>::operator=(const Type& value)
{
  xiiCVarEvent e(this);

  if (GetFlags().IsAnySet(xiiCVarFlags::RequiresDelayedSync))
  {
    if (value == m_Values[xiiCVarValue::DelayedSync]) // No change
      return;

    e.m_EventType = xiiCVarEvent::DelayedSyncValueChanged;
  }
  else
  {
    if (m_Values[xiiCVarValue::Current] == value) // No change
      return;

    m_Values[xiiCVarValue::Current] = value;
    e.m_EventType                   = xiiCVarEvent::ValueChanged;
  }

  m_Values[xiiCVarValue::DelayedSync] = value;

  m_CVarEvents.Broadcast(e);

  // Broadcast the same to the 'all cvars' event handlers.
  s_AllCVarEvents.Broadcast(e);
}

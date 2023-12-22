#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorFramework/Gizmos/SnapProvider.h>
#include <EditorFramework/Preferences/EditorPreferences.h>
#include <Foundation/Configuration/SubSystem.h>

xiiAngle               xiiSnapProvider::s_RotationSnapValue      = xiiAngle::Degree(15.0f);
float                  xiiSnapProvider::s_fScaleSnapValue        = 0.125f;
float                  xiiSnapProvider::s_fTranslationSnapValue  = 0.25f;
xiiEventSubscriptionID xiiSnapProvider::s_UserPreferencesChanged = 0;

xiiEvent<const xiiSnapProviderEvent&> xiiSnapProvider::s_Events;

// clang-format off
XII_BEGIN_SUBSYSTEM_DECLARATION(EditorFramework, SnapProvider)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "EditorFrameworkMain"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    xiiSnapProvider::Startup();
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    xiiSnapProvider::Shutdown();
  }

XII_END_SUBSYSTEM_DECLARATION;
// clang-format on

void xiiSnapProvider::Startup()
{
  xiiQtEditorApp::m_Events.AddEventHandler(xiiMakeDelegate(&xiiSnapProvider::EditorEventHandler));
}

void xiiSnapProvider::Shutdown()
{
  if (s_UserPreferencesChanged)
  {
    xiiEditorPreferencesUser* pPreferences = xiiPreferences::QueryPreferences<xiiEditorPreferencesUser>();
    pPreferences->m_ChangedEvent.RemoveEventHandler(s_UserPreferencesChanged);
  }
  xiiQtEditorApp::m_Events.RemoveEventHandler(xiiMakeDelegate(&xiiSnapProvider::EditorEventHandler));
}

void xiiSnapProvider::EditorEventHandler(const xiiEditorAppEvent& e)
{
  if (e.m_Type == xiiEditorAppEvent::Type::EditorStarted)
  {
    xiiEditorPreferencesUser* pPreferences = xiiPreferences::QueryPreferences<xiiEditorPreferencesUser>();
    PreferenceChangedEventHandler(pPreferences);
    s_UserPreferencesChanged = pPreferences->m_ChangedEvent.AddEventHandler(xiiMakeDelegate(&xiiSnapProvider::PreferenceChangedEventHandler));
  }
}

void xiiSnapProvider::PreferenceChangedEventHandler(xiiPreferences* pPreferenceBase)
{
  auto* pPreferences = static_cast<xiiEditorPreferencesUser*>(pPreferenceBase);
  SetRotationSnapValue(pPreferences->m_RotationSnapValue);
  SetScaleSnapValue(pPreferences->m_fScaleSnapValue);
  SetTranslationSnapValue(pPreferences->m_fTranslationSnapValue);
}

xiiAngle xiiSnapProvider::GetRotationSnapValue()
{
  return s_RotationSnapValue;
}

float xiiSnapProvider::GetScaleSnapValue()
{
  return s_fScaleSnapValue;
}

float xiiSnapProvider::GetTranslationSnapValue()
{
  return s_fTranslationSnapValue;
}

void xiiSnapProvider::SetRotationSnapValue(xiiAngle angle)
{
  if (s_RotationSnapValue == angle)
    return;

  s_RotationSnapValue = angle;

  xiiEditorPreferencesUser* pPreferences = xiiPreferences::QueryPreferences<xiiEditorPreferencesUser>();
  pPreferences->m_RotationSnapValue      = angle;
  pPreferences->TriggerPreferencesChangedEvent();

  xiiSnapProviderEvent e;
  e.m_Type = xiiSnapProviderEvent::Type::RotationSnapChanged;
  s_Events.Broadcast(e);
}

void xiiSnapProvider::SetScaleSnapValue(float fPercentage)
{
  if (s_fScaleSnapValue == fPercentage)
    return;

  s_fScaleSnapValue = fPercentage;

  xiiEditorPreferencesUser* pPreferences = xiiPreferences::QueryPreferences<xiiEditorPreferencesUser>();
  pPreferences->m_fScaleSnapValue        = fPercentage;
  pPreferences->TriggerPreferencesChangedEvent();

  xiiSnapProviderEvent e;
  e.m_Type = xiiSnapProviderEvent::Type::ScaleSnapChanged;
  s_Events.Broadcast(e);
}

void xiiSnapProvider::SetTranslationSnapValue(float fUnits)
{
  if (s_fTranslationSnapValue == fUnits)
    return;

  s_fTranslationSnapValue = fUnits;

  xiiEditorPreferencesUser* pPreferences = xiiPreferences::QueryPreferences<xiiEditorPreferencesUser>();
  pPreferences->m_fTranslationSnapValue  = fUnits;
  pPreferences->TriggerPreferencesChangedEvent();

  xiiSnapProviderEvent e;
  e.m_Type = xiiSnapProviderEvent::Type::TranslationSnapChanged;
  s_Events.Broadcast(e);
}

void xiiSnapProvider::SnapTranslation(xiiVec3& value)
{
  if (s_fTranslationSnapValue <= 0.0f)
    return;

  value.x = xiiMath::RoundToMultiple(value.x, s_fTranslationSnapValue);
  value.y = xiiMath::RoundToMultiple(value.y, s_fTranslationSnapValue);
  value.z = xiiMath::RoundToMultiple(value.z, s_fTranslationSnapValue);
}

void xiiSnapProvider::SnapTranslationInLocalSpace(const xiiQuat& qRotation, xiiVec3& ref_vTranslation)
{
  if (s_fTranslationSnapValue <= 0.0f)
    return;

  const xiiQuat mInvRot = qRotation.GetInverse();

  xiiVec3 vLocalTranslation = mInvRot * ref_vTranslation;
  vLocalTranslation.x       = xiiMath::RoundToMultiple(vLocalTranslation.x, s_fTranslationSnapValue);
  vLocalTranslation.y       = xiiMath::RoundToMultiple(vLocalTranslation.y, s_fTranslationSnapValue);
  vLocalTranslation.z       = xiiMath::RoundToMultiple(vLocalTranslation.z, s_fTranslationSnapValue);

  ref_vTranslation = qRotation * vLocalTranslation;
}

void xiiSnapProvider::SnapRotation(xiiAngle& ref_rotation)
{
  if (s_RotationSnapValue.GetRadian() != 0.0f)
  {
    ref_rotation = xiiAngle::MakeFromRadian(xiiMath::RoundToMultiple(ref_rotation.GetRadian(), s_RotationSnapValue.GetRadian()));
  }
}

void xiiSnapProvider::SnapScale(float& ref_fScale)
{
  if (s_fScaleSnapValue > 0.0f)
  {
    ref_fScale = xiiMath::RoundToMultiple(ref_fScale, s_fScaleSnapValue);
  }
}

void xiiSnapProvider::SnapScale(xiiVec3& ref_vScale)
{
  if (s_fScaleSnapValue > 0.0f)
  {
    SnapScale(ref_vScale.x);
    SnapScale(ref_vScale.y);
    SnapScale(ref_vScale.z);
  }
}

xiiVec3 xiiSnapProvider::GetScaleSnapped(const xiiVec3& vScale)
{
  xiiVec3 res = vScale;
  SnapScale(res);
  return res;
}

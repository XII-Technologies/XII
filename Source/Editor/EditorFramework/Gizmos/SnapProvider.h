/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>

#include <Foundation/Communication/Event.h>
#include <Foundation/Math/Declarations.h>

struct xiiEditorAppEvent;
class xiiPreferences;

struct xiiSnapProviderEvent
{
  enum class Type
  {
    RotationSnapChanged,
    ScaleSnapChanged,
    TranslationSnapChanged
  };

  Type m_Type;
};

class XII_EDITORFRAMEWORK_DLL xiiSnapProvider
{
public:
  static void Startup();
  static void Shutdown();

  static xiiAngle GetRotationSnapValue();
  static float    GetScaleSnapValue();
  static float    GetTranslationSnapValue();

  static void SetRotationSnapValue(xiiAngle angle);
  static void SetScaleSnapValue(float fPercentage);
  static void SetTranslationSnapValue(float fUnits);

  /// Rounds each component to the closest translation snapping value
  static void SnapTranslation(xiiVec3& value);

  /// Inverts the rotation, applies that to the translation, snaps it and then transforms it back into the original space
  static void SnapTranslationInLocalSpace(const xiiQuat& qRotation, xiiVec3& ref_vTranslation);

  static void SnapRotation(xiiAngle& ref_rotation);

  static void SnapScale(float& ref_fScale);
  static void SnapScale(xiiVec3& ref_vScale);

  static xiiVec3 GetScaleSnapped(const xiiVec3& vScale);

  static xiiEvent<const xiiSnapProviderEvent&> s_Events;

private:
  static void EditorEventHandler(const xiiEditorAppEvent& e);
  static void PreferenceChangedEventHandler(xiiPreferences* pPreferenceBase);

  static xiiAngle               s_RotationSnapValue;
  static float                  s_fScaleSnapValue;
  static float                  s_fTranslationSnapValue;
  static xiiEventSubscriptionID s_UserPreferencesChanged;
};

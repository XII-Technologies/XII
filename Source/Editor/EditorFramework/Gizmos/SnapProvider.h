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

  /// \brief Rounds each component to the closest translation snapping value
  static void SnapTranslation(xiiVec3& value);

  /// \brief Inverts the rotation, applies that to the translation, snaps it and then transforms it back into the original space
  static void SnapTranslationInLocalSpace(const xiiQuat& rotation, xiiVec3& translation);

  static void SnapRotation(xiiAngle& rotation);

  static void SnapScale(float& scale);
  static void SnapScale(xiiVec3& scale);

  static xiiVec3 GetScaleSnapped(const xiiVec3& scale);

  static xiiEvent<const xiiSnapProviderEvent&> s_Events;

private:
  static void EditorEventHandler(const xiiEditorAppEvent& e);
  static void PreferenceChangedEventHandler(xiiPreferences* pPreferenceBase);

  static xiiAngle               s_RotationSnapValue;
  static float                  s_fScaleSnapValue;
  static float                  s_fTranslationSnapValue;
  static xiiEventSubscriptionID s_UserPreferencesChanged;
};

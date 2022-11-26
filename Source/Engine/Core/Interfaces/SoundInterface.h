#pragma once

#include <Foundation/Basics.h>

class xiiSoundInterface
{
public:
  /// \brief Can be called before startup to load the fmod configs from a different file.
  /// Otherwise will automatically be loaded by fmod startup with the default path ":project/FmodConfig.ddl"
  virtual void LoadConfiguration(const char* szFile) = 0;

  /// \brief By default the fmod integration will auto-detect the platform (and thus the config) to use.
  /// Calling this before startup allows to override which configuration is used.
  virtual void SetOverridePlatform(const char* szPlatform) = 0;

  /// \brief Has to be called once per frame to update all sounds
  virtual void UpdateSound() = 0;

  /// \brief Adjusts the master volume. This affects all sounds, with no exception. Value must be between 0.0f and 1.0f.
  virtual void  SetMasterChannelVolume(float volume) = 0;
  virtual float GetMasterChannelVolume() const       = 0;

  /// \brief Allows to mute all sounds. Useful for when the application goes to a background state.
  virtual void SetMasterChannelMute(bool mute) = 0;
  virtual bool GetMasterChannelMute() const    = 0;

  /// \brief Allows to pause all sounds. Useful for when the application goes to a background state and you want to pause all sounds, instead of mute
  /// them.
  virtual void SetMasterChannelPaused(bool paused) = 0;
  virtual bool GetMasterChannelPaused() const      = 0;

  /// \brief Specifies the volume for a VCA ('Voltage Control Amplifier').
  ///
  /// This is used to control the volume of high level sound groups, such as 'Effects', 'Music', 'Ambiance' or 'Speech'.
  /// Note that the fmod strings banks are never loaded, so the given string must be a GUID (fmod Studio -> Copy GUID).
  virtual void  SetSoundGroupVolume(const char* szVcaGroupGuid, float volume) = 0;
  virtual float GetSoundGroupVolume(const char* szVcaGroupGuid) const         = 0;

  /// \brief Default is 1. Allows to set how many virtual listeners the sound is mixed for (split screen game play).
  virtual void     SetNumListeners(xiiUInt8 uiNumListeners) = 0;
  virtual xiiUInt8 GetNumListeners()                        = 0;

  /// \brief The editor activates this to ignore the listener positions from the listener components, and instead use the editor camera as the
  /// listener position.
  virtual void SetListenerOverrideMode(bool enabled) = 0;

  /// \brief Sets the position for listener N. Index -1 is used for the override mode listener.
  virtual void SetListener(xiiInt32 iIndex, const xiiVec3& vPosition, const xiiVec3& vForward, const xiiVec3& vUp, const xiiVec3& vVelocity) = 0;
};

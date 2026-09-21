/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Core/CoreDLL.h>
#include <Foundation/Basics.h>

class XII_CORE_DLL xiiSoundInterface
{
public:
  /// Can be called before startup to load the configs from a different file.
  /// Otherwise will automatically be loaded by the sound system startup with the default path.
  virtual void LoadConfiguration(xiiStringView sFile) = 0;

  /// By default the integration should auto-detect the platform (and thus the config) to use.
  /// Calling this before startup allows to override which configuration is used.
  virtual void SetOverridePlatform(xiiStringView sPlatform) = 0;

  /// Has to be called once per frame to update all sounds
  virtual void UpdateSound() = 0;

  /// Adjusts the master volume. This affects all sounds, with no exception. Value must be between 0.0f and 1.0f.
  virtual void  SetMasterChannelVolume(float fVolume) = 0;
  virtual float GetMasterChannelVolume() const        = 0;

  /// Allows to mute all sounds. Useful for when the application goes to a background state.
  virtual void SetMasterChannelMute(bool bMute) = 0;
  virtual bool GetMasterChannelMute() const     = 0;

  /// Allows to pause all sounds. Useful for when the application goes to a background state and you want to pause all sounds, instead of mute
  /// them.
  virtual void SetMasterChannelPaused(bool bPaused) = 0;
  virtual bool GetMasterChannelPaused() const       = 0;

  /// Specifies the volume for a VCA ('Voltage Control Amplifier').
  ///
  /// This is used to control the volume of high level sound groups, such as 'Effects', 'Music', 'Ambiance' or 'Speech'.
  /// Note that the FMOD strings banks are never loaded, so the given string must be a GUID (FMOD Studio -> Copy GUID).
  virtual void  SetSoundGroupVolume(xiiStringView sVcaGroupGuid, float fVolume) = 0;
  virtual float GetSoundGroupVolume(xiiStringView sVcaGroupGuid) const          = 0;

  /// Default is 1. Allows to set how many virtual listeners the sound is mixed for (split screen game play).
  virtual void     SetNumListeners(xiiUInt8 uiNumListeners) = 0;
  virtual xiiUInt8 GetNumListeners()                        = 0;

  /// The editor activates this to ignore the listener positions from the listener components, and instead use the editor camera as the
  /// listener position.
  virtual void SetListenerOverrideMode(bool bEnabled) = 0;

  /// Sets the position for listener N. Index -1 is used for the override mode listener.
  virtual void SetListener(xiiInt32 iIndex, const xiiVec3& vPosition, const xiiVec3& vForward, const xiiVec3& vUp, const xiiVec3& vVelocity) = 0;

  /// Plays a sound once. Callced by xiiSoundInterface::PlaySound().
  virtual xiiResult OneShotSound(xiiStringView sResourceID, const xiiTransform& globalPosition, float fPitch = 1.0f, float fVolume = 1.0f, bool bBlockIfNotLoaded = true) = 0;

  /// Plays a sound once.
  ///
  /// Convenience function to call OneShotSound() without having to retrieve the xiiSoundInterface first.
  ///
  /// Which sound to play is specified through a resource ID ('Asset GUID').
  /// This is not the most efficient way to load a sound, as there is no way to preload the resource.
  /// If preloading is desired, you need to access the implementation-specific resource type directly (e.g. xiiFmodSoundEventResource).
  /// Also see xiiFmodSoundEventResource::PlayOnce().
  /// In practice, though, sounds are typically loaded in bulk from sound-banks, and preloading is not necessary.
  ///
  /// Be aware that this does not allow to adjust volume, pitch or position after creation. Stopping is also not possible.
  /// Use a sound component, if that is necessary.
  ///
  /// Also by default a pitch of 1 is always used. If the game speed is not 1 (xiiWorld clock), a custom pitch would need to be provided,
  /// if the sound should play at the same speed.
  static xiiResult PlaySound(xiiStringView sResourceID, const xiiTransform& globalPosition, float fPitch = 1.0f, float fVolume = 1.0f, bool bBlockIfNotLoaded = true);
};

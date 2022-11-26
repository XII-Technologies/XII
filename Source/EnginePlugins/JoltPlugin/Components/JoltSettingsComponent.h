#pragma once

#include <Core/World/SettingsComponent.h>
#include <Core/World/SettingsComponentManager.h>
#include <JoltPlugin/Declarations.h>

using xiiJoltSettingsComponentManager = xiiSettingsComponentManager<class xiiJoltSettingsComponent>;

class XII_JOLTPLUGIN_DLL xiiJoltSettingsComponent : public xiiSettingsComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiJoltSettingsComponent, xiiSettingsComponent, xiiJoltSettingsComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // xiiComponent

public:
  virtual void SerializeComponent(xiiWorldWriter& stream) const override;
  virtual void DeserializeComponent(xiiWorldReader& stream) override;


  //////////////////////////////////////////////////////////////////////////
  // xiiJoltSettingsComponent

public:
  xiiJoltSettingsComponent();
  ~xiiJoltSettingsComponent();

  const xiiJoltSettings& GetSettings() const { return m_Settings; }

  const xiiVec3& GetObjectGravity() const { return m_Settings.m_vObjectGravity; } // [ property ]
  void           SetObjectGravity(const xiiVec3& v);                              // [ property ]

  const xiiVec3& GetCharacterGravity() const { return m_Settings.m_vCharacterGravity; } // [ property ]
  void           SetCharacterGravity(const xiiVec3& v);                                 // [ property ]

  xiiJoltSteppingMode::Enum GetSteppingMode() const { return m_Settings.m_SteppingMode; } // [ property ]
  void                      SetSteppingMode(xiiJoltSteppingMode::Enum mode);              // [ property ]

  float GetFixedFrameRate() const { return m_Settings.m_fFixedFrameRate; } // [ property ]
  void  SetFixedFrameRate(float fFixedFrameRate);                          // [ property ]

  xiiUInt32 GetMaxSubSteps() const { return m_Settings.m_uiMaxSubSteps; } // [ property ]
  void      SetMaxSubSteps(xiiUInt32 uiMaxSubSteps);                      // [ property ]

  xiiUInt32 GetMaxBodies() const { return m_Settings.m_uiMaxBodies; } // [ property ]
  void      SetMaxBodies(xiiUInt32 uiMaxBodies);                      // [ property ]

protected:
  xiiJoltSettings m_Settings;
};

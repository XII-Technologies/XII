#pragma once

#include <Core/World/SettingsComponentManager.h>
#include <GameEngine/GameEngineDLL.h>
#include <GameEngine/XR/XRInterface.h>

//////////////////////////////////////////////////////////////////////////

using xiiStageSpaceComponentManager = class xiiStageSpaceComponent;

/// \brief Singleton to set the type of stage space and its global transform in the world.
///
/// The global transform of the owner and the set stage space are read out by the XR
/// implementation every frame.
class XII_GAMEENGINE_DLL xiiStageSpaceComponent : public xiiComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiStageSpaceComponent, xiiComponent, xiiStageSpaceComponentManager);

public:
  xiiStageSpaceComponent();
  ~xiiStageSpaceComponent();

  //
  // xiiDeviceTrackingComponent Interface
  //

  /// \brief Sets the stage space used by the XR experience.
  void                     SetStageSpace(xiiEnum<xiiXRStageSpace> space);
  xiiEnum<xiiXRStageSpace> GetStageSpace() const;

protected:
  //
  // xiiComponent Interface
  //
  virtual void SerializeComponent(xiiWorldWriter& stream) const override;
  virtual void DeserializeComponent(xiiWorldReader& stream) override;
  virtual void OnActivated() override;
  virtual void OnDeactivated() override;

private:
  xiiEnum<xiiXRStageSpace> m_Space;
};

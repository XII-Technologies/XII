#pragma once

#include <Core/World/Component.h>
#include <Core/World/World.h>
#include <GameEngine/GameEngineDLL.h>
#include <GameEngine/XR/XRSpatialAnchorsInterface.h>

//////////////////////////////////////////////////////////////////////////

using xiiSpatialAnchorComponentManager = xiiComponentManagerSimple<class xiiSpatialAnchorComponent, xiiComponentUpdateType::WhenSimulating>;

class XII_GAMEENGINE_DLL xiiSpatialAnchorComponent : public xiiComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiSpatialAnchorComponent, xiiComponent, xiiSpatialAnchorComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // xiiComponent

public:
  virtual void SerializeComponent(xiiWorldWriter& ref_stream) const override;
  virtual void DeserializeComponent(xiiWorldReader& ref_stream) override;

protected:
  virtual void OnSimulationStarted() override;

  //////////////////////////////////////////////////////////////////////////
  // xiiSpatialAnchorComponent

public:
  xiiSpatialAnchorComponent();
  ~xiiSpatialAnchorComponent();

  /// \brief Attempts to create a new anchor at the given location.
  ///
  /// On failure, the existing anchor will continue to be used.
  /// On success, the new anchor will be used and the new location.
  xiiResult RecreateAnchorAt(const xiiTransform& position);

protected:
  void Update();

private:
  xiiXRSpatialAnchorID m_AnchorID;
};

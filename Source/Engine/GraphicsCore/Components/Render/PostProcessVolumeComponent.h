/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>

#include <Core/World/ComponentManager.h>

using xiiPostProcessVolumeComponentManager = xiiComponentManager<class xiiPostProcessVolumeComponent, xiiBlockStorageType::Compact>;

class XII_GRAPHICSCORE_DLL xiiPostProcessVolumeComponent : public xiiComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiPostProcessVolumeComponent, xiiComponent, xiiPostProcessVolumeComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // xiiComponent

public:
  virtual void SerializeComponent(xiiWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(xiiWorldReader& inout_stream) override;

  //////////////////////////////////////////////////////////////////////////
  // xiiPostProcessVolumeComponent

public:
  xiiPostProcessVolumeComponent();
  ~xiiPostProcessVolumeComponent();

protected:
  xiiColor m_ColourTint;
};

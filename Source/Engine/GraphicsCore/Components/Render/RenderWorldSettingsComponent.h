/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>

#include <Core/World/SettingsComponent.h>
#include <Core/World/SettingsComponentManager.h>
#include <GraphicsCore/Pipeline/Declarations.h>

using xiiRenderWorldSettingsComponentManager = xiiSettingsComponentManager<class xiiRenderWorldSettingsComponent>;

class XII_GRAPHICSCORE_DLL xiiRenderWorldSettingsComponent : public xiiSettingsComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiRenderWorldSettingsComponent, xiiSettingsComponent, xiiRenderWorldSettingsComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // xiiComponent

public:
  virtual void SerializeComponent(xiiWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(xiiWorldReader& inout_stream) override;


  //////////////////////////////////////////////////////////////////////////
  // xiiRenderWorldSettingsComponent

public:
  xiiRenderWorldSettingsComponent();
  ~xiiRenderWorldSettingsComponent();

protected:
  xiiEnum<xiiShadingQualityLevel> m_ShadingQualityLevel;
};

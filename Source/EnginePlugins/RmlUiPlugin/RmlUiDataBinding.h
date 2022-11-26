#pragma once

#include <RmlUiPlugin/RmlUiPluginDLL.h>

#include <RmlUi/Include/RmlUi/Core.h>

class XII_RMLUIPLUGIN_DLL xiiRmlUiDataBinding
{
public:
  virtual ~xiiRmlUiDataBinding() {}

  virtual xiiResult Initialize(Rml::Context& context)   = 0;
  virtual void      Deinitialize(Rml::Context& context) = 0;

  virtual void Update() = 0;
};

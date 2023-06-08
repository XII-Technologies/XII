#pragma once

#include <RmlUiPlugin/RmlUiPluginDLL.h>

#include <RmlUi/Include/RmlUi/Core.h>

class XII_RMLUIPLUGIN_DLL xiiRmlUiDataBinding
{
public:
  virtual ~xiiRmlUiDataBinding() = default;

  virtual xiiResult Initialize(Rml::Context& ref_context)   = 0;
  virtual void      Deinitialize(Rml::Context& ref_context) = 0;

  virtual void Update() = 0;
};

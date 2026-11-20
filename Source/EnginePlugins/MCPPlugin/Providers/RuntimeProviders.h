/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <MCPPlugin/MCPPluginDLL.h>

#include <MCPPlugin/Registry.h>

namespace xiiMCPRuntimeProviders
{
  void                    RegisterApplication(xiiMCPRegistry& ref_registry);
  void                    RegisterInput(xiiMCPRegistry& ref_registry);
  void                    RegisterWorld(xiiMCPRegistry& ref_registry);
  void                    UpdatePhysics(xiiMCPRegistry& ref_registry, bool& ref_bRegistered);
  void                    RegisterResources(xiiMCPRegistry& ref_registry);
  void                    RegisterReflection(xiiMCPRegistry& ref_registry);
  [[nodiscard]] xiiStatus WriteResource(xiiStringView sUri, const xiiVariantDictionary& data, xiiVariantDictionary& out_result);
} // namespace xiiMCPRuntimeProviders

/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Core/CorePCH.h>

#include <Core/World/SettingsComponent.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiSettingsComponent, 1, xiiRTTINoAllocator)
{
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("Settings"),
  }
  XII_END_ATTRIBUTES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiSettingsComponent::xiiSettingsComponent()
{
  SetModified();
}

xiiSettingsComponent::~xiiSettingsComponent() = default;


XII_STATICLINK_FILE(Core, Core_World_Implementation_SettingsComponent);

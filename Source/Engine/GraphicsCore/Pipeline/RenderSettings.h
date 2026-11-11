/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>

#include <Foundation/Configuration/Settings.h>
#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Reflection/PropertyPath.h>
#include <Foundation/Strings/String.h>

struct xiiRenderSettingsChange
{
  using StorageType = xiiUInt8;
  enum Enum : StorageType
  {
    None      = 0,
    History   = XII_BIT(0),
    Resources = XII_BIT(1),
    Pipeline  = XII_BIT(2),
    Shaders   = XII_BIT(3),
    Output    = XII_BIT(4),
    Default   = None
  };
  struct Bits
  {
    StorageType History : 1;
    StorageType Resources : 1;
    StorageType Pipeline : 1;
    StorageType Shaders : 1;
    StorageType Output : 1;
  };
};
XII_DECLARE_FLAGS_OPERATORS(xiiRenderSettingsChange);

/// Opt-in mapping from a typed subsystem property to an existing authored pass property.
/// Bindings are resolved at configuration publication, never during graph execution.
class XII_GRAPHICSCORE_DLL xiiRenderSettingsBindingAttribute : public xiiPropertyAttribute
{
  XII_ADD_DYNAMIC_REFLECTION(xiiRenderSettingsBindingAttribute, xiiPropertyAttribute);

public:
  xiiRenderSettingsBindingAttribute() = default;
  xiiRenderSettingsBindingAttribute(xiiStringView pass, xiiStringView property, xiiUInt32 changes = 0);
  xiiRenderSettingsBindingAttribute(xiiStringView pass, xiiStringView property, xiiBitflags<xiiRenderSettingsChange> changes) : xiiRenderSettingsBindingAttribute(pass, property, changes.GetValue()) {}
  const char* GetPass() const { return m_sPass; }
  const char* GetProperty() const { return m_sProperty; }
  xiiUInt32   GetChanges() const { return m_uiChanges; }

private:
  xiiString m_sPass;
  xiiString m_sProperty;
  xiiUInt32 m_uiChanges = 0;
};

/// For controls consumed by a view itself, rather than a specific pipeline operation.
class XII_GRAPHICSCORE_DLL xiiViewRenderSettingsAttribute : public xiiPropertyAttribute
{
  XII_ADD_DYNAMIC_REFLECTION(xiiViewRenderSettingsAttribute, xiiPropertyAttribute);
};

struct XII_GRAPHICSCORE_DLL xiiRenderSettingProperty
{
  xiiString                  m_sPath;
  const xiiAbstractProperty* m_pProperty = nullptr;
};

/// Subsystem-owned typed values with an explicit independent override mask.
/// Deriving types use normal RTTI, categories, ranges and binding attributes. Constructors
/// have no rendering side effects. Owners publish validated copies at a CPU frame boundary.
class XII_GRAPHICSCORE_DLL xiiRenderSubsystemSettings : public xiiSettings
{
  XII_ADD_DYNAMIC_REFLECTION(xiiRenderSubsystemSettings, xiiSettings);

public:
  xiiDynamicArray<xiiString> m_Overrides;

  xiiResult        Validate() const override;
  bool             IsOverridden(xiiStringView path) const;
  xiiResult        SetOverride(xiiStringView path, const xiiVariant& value);
  void             RemoveOverride(xiiStringView path);
  void             GetProperties(xiiDynamicArray<xiiRenderSettingProperty>& out_properties) const;
  static void      GetProperties(const xiiRTTI* type, xiiDynamicArray<xiiRenderSettingProperty>& out_properties);
  static xiiResult ReadProperty(const xiiRenderSubsystemSettings& settings, xiiStringView path, xiiVariant& out_value);
};

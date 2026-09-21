/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Configuration/Singleton.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Time/Timestamp.h>

struct xiiPhantomRttiManagerEvent;
class xiiExposedParameters;
struct xiiAssetCuratorEvent;

/// Lazily converts xiiExposedParameters into phantom types.
/// Call GetExposedParametersType to create a type for a sub-asset ID.
class xiiExposedParametersTypeRegistry
{
  XII_DECLARE_SINGLETON(xiiExposedParametersTypeRegistry);

public:
  xiiExposedParametersTypeRegistry();
  ~xiiExposedParametersTypeRegistry();
  /// Returns null if the curator can find the asset or if the asset
  /// does not have any xiiExposedParameters meta data.
  const xiiRTTI* GetExposedParametersType(xiiStringView sResource);
  /// All exposed parameter types derive from this.
  const xiiRTTI* GetExposedParametersBaseType() const { return m_pBaseType; }

private:
  XII_MAKE_SUBSYSTEM_STARTUP_FRIEND(EditorFramework, ExposedParametersTypeRegistry);

  struct ParamData
  {
    ParamData() = default;

    xiiUuid        m_SubAssetGuid;
    bool           m_bUpToDate = true;
    const xiiRTTI* m_pType     = nullptr;
  };
  void UpdateExposedParametersType(ParamData& data, const xiiExposedParameters& params);
  void AssetCuratorEventHandler(const xiiAssetCuratorEvent& e);
  void PhantomTypeRegistryEventHandler(const xiiPhantomRttiManagerEvent& e);

  xiiMap<xiiUuid, ParamData> m_ShaderTypes;
  const xiiRTTI*             m_pBaseType;
  ParamData*                 m_pAboutToBeRegistered = nullptr;
};

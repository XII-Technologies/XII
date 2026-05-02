/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Core/ResourceManager/ResourceHandle.h>
#include <Core/World/World.h>
#include <Foundation/Containers/ArrayMap.h>
#include <Foundation/Strings/HashedString.h>
#include <Foundation/Types/RangeView.h>
#include <Foundation/Types/Variant.h>
#include <GameEngine/GameEngineDLL.h>

using xiiPrefabResourceHandle = xiiTypedResourceHandle<class xiiPrefabResource>;

using xiiPlayerStartPointComponentManager = xiiComponentManager<class xiiPlayerStartPointComponent, xiiBlockStorageType::Compact>;

class XII_GAMEENGINE_DLL xiiPlayerStartPointComponent : public xiiComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiPlayerStartPointComponent, xiiComponent, xiiPlayerStartPointComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // xiiComponent

public:
  virtual void SerializeComponent(xiiWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(xiiWorldReader& inout_stream) override;

  //////////////////////////////////////////////////////////////////////////
  // xiiPlayerStartPointComponent

public:
  xiiPlayerStartPointComponent();
  ~xiiPlayerStartPointComponent();

  void                           SetPlayerPrefab(const xiiPrefabResourceHandle& hPrefab); // [ property ]
  const xiiPrefabResourceHandle& GetPlayerPrefab() const;                                 // [ property ]

  const xiiRangeView<xiiStringView, xiiUInt32> GetParameters() const;                                         // [ property ] (exposed parameter)
  void                                         SetParameter(xiiStringView sKey, const xiiVariant& value);     // [ property ] (exposed parameter)
  void                                         RemoveParameter(xiiStringView sKey);                           // [ property ] (exposed parameter)
  bool                                         GetParameter(xiiStringView sKey, xiiVariant& out_value) const; // [ property ] (exposed parameter)

  xiiArrayMap<xiiHashedString, xiiVariant> m_Parameters;

  // TODO:
  //  add properties to differentiate use cases, such as
  //  single player vs. multi-player spawn points
  //  team number

protected:
  xiiPrefabResourceHandle m_hPlayerPrefab;
};

#pragma once

#include <Core/ResourceManager/ResourceHandle.h>
#include <Core/World/World.h>
#include <Foundation/Containers/ArrayMap.h>
#include <Foundation/Strings/HashedString.h>
#include <Foundation/Types/RangeView.h>
#include <Foundation/Types/Variant.h>
#include <GameEngine/GameEngineDLL.h>

using xiiPrefabResourceHandle = xiiTypedResourceHandle<class xiiPrefabResource>;

using xiiPlayerStartPointComponentManager = class xiiPlayerStartPointComponent;

class XII_GAMEENGINE_DLL xiiPlayerStartPointComponent : public xiiComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiPlayerStartPointComponent, xiiComponent, xiiPlayerStartPointComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // xiiComponent

public:
  virtual void SerializeComponent(xiiWorldWriter& ref_stream) const override;
  virtual void DeserializeComponent(xiiWorldReader& ref_stream) override;

  //////////////////////////////////////////////////////////////////////////
  // xiiPlayerStartPointComponent

public:
  xiiPlayerStartPointComponent();
  ~xiiPlayerStartPointComponent();

  void        SetPlayerPrefabFile(const char* szFile); // [ property ]
  const char* GetPlayerPrefabFile() const;             // [ property ]

  void                           SetPlayerPrefab(const xiiPrefabResourceHandle& hPrefab); // [ property ]
  const xiiPrefabResourceHandle& GetPlayerPrefab() const;                                 // [ property ]

  const xiiRangeView<const char*, xiiUInt32> GetParameters() const;                                        // [ property ] (exposed parameter)
  void                                       SetParameter(const char* szKey, const xiiVariant& value);     // [ property ] (exposed parameter)
  void                                       RemoveParameter(const char* szKey);                           // [ property ] (exposed parameter)
  bool                                       GetParameter(const char* szKey, xiiVariant& out_value) const; // [ property ] (exposed parameter)

  xiiArrayMap<xiiHashedString, xiiVariant> m_Parameters;

  // TODO:
  //  add properties to differentiate use cases, such as
  //  single player vs. multi-player spawn points
  //  team number

protected:
  xiiPrefabResourceHandle m_hPlayerPrefab;
};

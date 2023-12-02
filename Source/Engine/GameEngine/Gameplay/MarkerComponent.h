#pragma once

#include <GameEngine/GameEngineDLL.h>

#include <Core/World/Component.h>
#include <Core/World/World.h>

using xiiMarkerComponentManager = xiiComponentManager<class xiiMarkerComponent, xiiBlockStorageType::Compact>;

class XII_GAMEENGINE_DLL xiiMarkerComponent : public xiiComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiMarkerComponent, xiiComponent, xiiMarkerComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // xiiComponent

public:
  virtual void SerializeComponent(xiiWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(xiiWorldReader& inout_stream) override;

protected:
  virtual void OnActivated() override;
  virtual void OnDeactivated() override;


  //////////////////////////////////////////////////////////////////////////
  // xiiMarkerComponent

public:
  xiiMarkerComponent();
  ~xiiMarkerComponent();

  void        SetMarkerType(const char* szType); // [ property ]
  const char* GetMarkerType() const;             // [ property ]

  void  SetRadius(float fRadius); // [ property ]
  float GetRadius() const;        // [ property ]

protected:
  void OnMsgUpdateLocalBounds(xiiMsgUpdateLocalBounds& msg) const; // [ msg handler ]
  void UpdateMarker();

  float           m_fRadius = 0.1f; // [ property ]
  xiiHashedString m_sMarkerType;    // [ property ]

  xiiSpatialData::Category m_SpatialCategory = xiiInvalidSpatialDataCategory;
};

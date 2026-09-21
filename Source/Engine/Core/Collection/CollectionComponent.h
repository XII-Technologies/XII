/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Core/Collection/CollectionResource.h>
#include <Core/CoreDLL.h>
#include <Core/World/Component.h>
#include <Core/World/World.h>

using xiiCollectionComponentManager = xiiComponentManager<class xiiCollectionComponent, xiiBlockStorageType::Compact>;

/// A xiiCollectionComponent references a xiiCollectionResource and triggers resource preloading when needed
///
/// Placing a xiiCollectionComponent in a scene or a model makes it possible to tell the engine to preload certain resources
/// that are likely to be needed soon.
///
/// If a deactivated xiiCollectionComponent is part of the scene, it will not trigger a preload, but will do so once
/// the component is activated.
class XII_CORE_DLL xiiCollectionComponent : public xiiComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiCollectionComponent, xiiComponent, xiiCollectionComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // xiiComponent
public:
  virtual void SerializeComponent(xiiWorldWriter& ref_stream) const override;
  virtual void DeserializeComponent(xiiWorldReader& ref_stream) override;

protected:
  virtual void OnSimulationStarted() override;

  //////////////////////////////////////////////////////////////////////////
  // xiiCollectionComponent
public:
  xiiCollectionComponent();
  ~xiiCollectionComponent();

  void                                                 SetCollection(const xiiCollectionResourceHandle& hPrefab); // [ property ]
  XII_ALWAYS_INLINE const xiiCollectionResourceHandle& GetCollection() const { return m_hCollection; }            // [ property ]

protected:
  /// Triggers the preload on the referenced xiiCollectionResource
  void InitiatePreload();

  bool                        m_bRegisterNames = false;
  xiiCollectionResourceHandle m_hCollection;
};

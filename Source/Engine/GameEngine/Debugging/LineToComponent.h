#pragma once

#include <GameEngine/GameEngineDLL.h>

#include <Core/World/Component.h>
#include <Core/World/ComponentManager.h>

class xiiLineToComponentManager : public xiiComponentManager<class xiiLineToComponent, xiiBlockStorageType::FreeList>
{
  using SUPER = xiiComponentManager<class xiiLineToComponent, xiiBlockStorageType::FreeList>;

public:
  xiiLineToComponentManager(xiiWorld* pWorld);

protected:
  void Initialize() override;
  void Update(const xiiWorldModule::UpdateContext& context);
};

/// \brief Draws a line from its own position to the target object position
class XII_GAMEENGINE_DLL xiiLineToComponent : public xiiComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiLineToComponent, xiiComponent, xiiLineToComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // xiiComponent

public:
  virtual void SerializeComponent(xiiWorldWriter& stream) const override;
  virtual void DeserializeComponent(xiiWorldReader& stream) override;

  //////////////////////////////////////////////////////////////////////////
  // xiiLineToComponent

public:
  xiiLineToComponent();
  ~xiiLineToComponent();

  const char* GetLineToTargetGuid() const;                   // [ property ]
  void        SetLineToTargetGuid(const char* szTargetGuid); // [ property ]

  void                       SetLineToTarget(const xiiGameObjectHandle& hTargetObject); // [ property ]
  const xiiGameObjectHandle& GetLineToTarget() const { return m_hTargetObject; }        // [ property ]

  xiiColor m_LineColor; // [ property ]

protected:
  void Update();

  xiiGameObjectHandle m_hTargetObject;
};

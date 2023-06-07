#pragma once

#include <Core/ResourceManager/ResourceHandle.h>
#include <Core/World/EventMessageHandlerComponent.h>
#include <Core/World/World.h>
#include <Foundation/Types/RangeView.h>
#include <GameEngine/GameEngineDLL.h>

class xiiVisualScriptComponent;
class xiiVisualScriptInstance;
struct xiiVisualScriptInstanceActivity;

using xiiVisualScriptResourceHandle = xiiTypedResourceHandle<class xiiVisualScriptResource>;

struct XII_GAMEENGINE_DLL xiiVisualScriptComponentActivityEvent
{
  xiiVisualScriptComponent*        m_pComponent = nullptr;
  xiiVisualScriptInstanceActivity* m_pActivity  = nullptr;
};

class XII_GAMEENGINE_DLL xiiVisualScriptComponentManager : public xiiComponentManager<xiiVisualScriptComponent, xiiBlockStorageType::Compact>
{
public:
  xiiVisualScriptComponentManager(xiiWorld* pWorld);
  ~xiiVisualScriptComponentManager();

  virtual void Initialize() override;

  void Update(const xiiWorldModule::UpdateContext& context);

private:
  void ResourceEventHandler(const xiiResourceEvent& e);

  xiiSet<xiiComponentHandle> m_ComponentsToUpdate;
};

class XII_GAMEENGINE_DLL xiiVisualScriptComponent : public xiiEventMessageHandlerComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiVisualScriptComponent, xiiEventMessageHandlerComponent, xiiVisualScriptComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // xiiComponent

public:
  virtual void SerializeComponent(xiiWorldWriter& ref_stream) const override;
  virtual void DeserializeComponent(xiiWorldReader& ref_stream) override;

protected:
  virtual bool OnUnhandledMessage(xiiMessage& msg, bool bWasPostedMsg) override;
  virtual bool OnUnhandledMessage(xiiMessage& msg, bool bWasPostedMsg) const override;

  virtual void Initialize() override;

  //////////////////////////////////////////////////////////////////////////
  // xiiEventMessageHandlerComponent

protected:
  virtual bool HandlesMessage(const xiiMessage& msg) const override;

  //////////////////////////////////////////////////////////////////////////
  // xiiVisualScriptComponent

public:
  xiiVisualScriptComponent();
  xiiVisualScriptComponent(xiiVisualScriptComponent&& other);
  ~xiiVisualScriptComponent();

  xiiVisualScriptComponent& operator=(xiiVisualScriptComponent&& other);

  void        SetScriptFile(const char* szFile); // [ property ]
  const char* GetScriptFile() const;             // [ property ]

  void                    SetScript(const xiiVisualScriptResourceHandle& hResource);
  XII_ALWAYS_INLINE const xiiVisualScriptResourceHandle& GetScript() const { return m_hResource; }

  const xiiRangeView<const char*, xiiUInt32> GetParameters() const;                                        // [ property ]
  void                                       SetParameter(const char* szKey, const xiiVariant& value);     // [ property ]
  void                                       RemoveParameter(const char* szKey);                           // [ property ]
  bool                                       GetParameter(const char* szKey, xiiVariant& out_value) const; // [ property ]

  static const xiiEvent<const xiiVisualScriptComponentActivityEvent&>& GetActivityEvents() { return s_ActivityEvents; }

protected:
  void Update();
  void InitScriptInstance();

  static xiiEvent<const xiiVisualScriptComponentActivityEvent&> s_ActivityEvents;

  struct Param
  {
    xiiHashedString m_sName;
    xiiVariant      m_Value;
  };

  xiiHybridArray<Param, 4> m_Params;

  xiiVisualScriptResourceHandle         m_hResource;
  xiiUniquePtr<xiiVisualScriptInstance> m_pScriptInstance;

  bool m_bHadEmptyActivity = true;
  bool m_bParamsChanged    = false;

  xiiUniquePtr<xiiVisualScriptInstanceActivity> m_pActivity;
};

#pragma once

#include <Core/ResourceManager/ResourceHandle.h>
#include <RendererCore/Components/RenderComponent.h>
#include <RmlUiPlugin/Components/RmlUiMessages.h>

struct xiiMsgExtractRenderData;
class xiiRmlUiContext;
class xiiRmlUiDataBinding;
class xiiBlackboard;

using xiiRmlUiResourceHandle = xiiTypedResourceHandle<class xiiRmlUiResource>;

using xiiRmlUiCanvas2DComponentManager = xiiComponentManagerSimple<class xiiRmlUiCanvas2DComponent, xiiComponentUpdateType::Always, xiiBlockStorageType::Compact>;

class XII_RMLUIPLUGIN_DLL xiiRmlUiCanvas2DComponent : public xiiRenderComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiRmlUiCanvas2DComponent, xiiRenderComponent, xiiRmlUiCanvas2DComponentManager);

public:
  xiiRmlUiCanvas2DComponent();
  ~xiiRmlUiCanvas2DComponent();

  xiiRmlUiCanvas2DComponent& operator=(xiiRmlUiCanvas2DComponent&& rhs);

  virtual void Initialize() override;
  virtual void Deinitialize() override;

  virtual void OnActivated() override;
  virtual void OnDeactivated() override;

  void Update();

  void        SetRmlFile(const char* szFile); // [ property ]
  const char* GetRmlFile() const;             // [ property ]

  void                          SetRmlResource(const xiiRmlUiResourceHandle& hResource);
  const xiiRmlUiResourceHandle& GetRmlResource() const { return m_hResource; }

  void              SetOffset(const xiiVec2I32& offset);    // [ property ]
  const xiiVec2I32& GetOffset() const { return m_vOffset; } // [ property ]

  void              SetSize(const xiiVec2U32& size);    // [ property ]
  const xiiVec2U32& GetSize() const { return m_vSize; } // [ property ]

  void           SetAnchorPoint(const xiiVec2& anchorPoint);       // [ property ]
  const xiiVec2& GetAnchorPoint() const { return m_vAnchorPoint; } // [ property ]

  void SetPassInput(bool bPassInput);                // [ property ]
  bool GetPassInput() const { return m_bPassInput; } // [ property ]

  /// \brief Look for a blackboard component on the owner object and its parent and bind their blackboards during initialization of this component.
  void SetAutobindBlackboards(bool bAutobind);                           // [ property ]
  bool GetAutobindBlackboards() const { return m_bAutobindBlackboards; } // [ property ]

  xiiUInt32 AddDataBinding(xiiUniquePtr<xiiRmlUiDataBinding>&& dataBinding);
  void      RemoveDataBinding(xiiUInt32 uiDataBindingIndex);

  /// \brief Adds the given blackboard as data binding. The name of the board is used as model name for the binding.
  xiiUInt32 AddBlackboardBinding(const xiiSharedPtr<xiiBlackboard>& pBlackboard);
  void      RemoveBlackboardBinding(xiiUInt32 uiDataBindingIndex);

  xiiRmlUiContext* GetOrCreateRmlContext();
  xiiRmlUiContext* GetRmlContext() { return m_pContext; }

  virtual void SerializeComponent(xiiWorldWriter& stream) const override;
  virtual void DeserializeComponent(xiiWorldReader& stream) override;

  virtual xiiResult GetLocalBounds(xiiBoundingBoxSphere& bounds, bool& bAlwaysVisible, xiiMsgUpdateLocalBounds& msg) override;

protected:
  void OnMsgExtractRenderData(xiiMsgExtractRenderData& msg) const;
  void OnMsgReload(xiiMsgRmlUiReload& msg);
  void UpdateCachedValues();
  void UpdateAutobinding();

  xiiRmlUiResourceHandle                                    m_hResource;
  xiiEvent<const xiiResourceEvent&, xiiMutex>::Unsubscriber m_ResourceEventUnsubscriber;

  xiiVec2I32 m_vOffset              = xiiVec2I32::ZeroVector();
  xiiVec2U32 m_vSize                = xiiVec2U32::ZeroVector();
  xiiVec2    m_vAnchorPoint         = xiiVec2::ZeroVector();
  xiiVec2U32 m_vReferenceResolution = xiiVec2U32::ZeroVector();
  bool       m_bPassInput           = true;
  bool       m_bAutobindBlackboards = true;

  xiiRmlUiContext* m_pContext = nullptr;

  xiiDynamicArray<xiiUniquePtr<xiiRmlUiDataBinding>> m_DataBindings;
  xiiDynamicArray<xiiUInt32>                         m_AutoBindings;
};

#include <GameEngine/GameEnginePCH.h>

#include <Core/ResourceManager/ResourceManager.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GameEngine/VisualScript/VisualScriptComponent.h>
#include <GameEngine/VisualScript/VisualScriptInstance.h>
#include <GameEngine/VisualScript/VisualScriptResource.h>

xiiEvent<const xiiVisualScriptComponentActivityEvent&> xiiVisualScriptComponent::s_ActivityEvents;

//////////////////////////////////////////////////////////////////////////

xiiVisualScriptComponentManager::xiiVisualScriptComponentManager(xiiWorld* pWorld) :
  xiiComponentManager<ComponentType, xiiBlockStorageType::Compact>(pWorld)
{
  xiiResourceManager::GetResourceEvents().AddEventHandler(xiiMakeDelegate(&xiiVisualScriptComponentManager::ResourceEventHandler, this));
}

xiiVisualScriptComponentManager::~xiiVisualScriptComponentManager()
{
  xiiResourceManager::GetResourceEvents().RemoveEventHandler(xiiMakeDelegate(&xiiVisualScriptComponentManager::ResourceEventHandler, this));
}

void xiiVisualScriptComponentManager::Initialize()
{
  auto desc = XII_CREATE_MODULE_UPDATE_FUNCTION_DESC(xiiVisualScriptComponentManager::Update, this);

  RegisterUpdateFunction(desc);
}

void xiiVisualScriptComponentManager::ResourceEventHandler(const xiiResourceEvent& e)
{
  // Don't handle resource reload events during play the game since that would mess up script state
  if (GetWorld()->GetWorldSimulationEnabled())
    return;

  if (e.m_Type == xiiResourceEvent::Type::ResourceContentUnloading && e.m_pResource->GetDynamicRTTI()->IsDerivedFrom<xiiVisualScriptResource>())
  {
    xiiVisualScriptResourceHandle hScript((xiiVisualScriptResource*)(e.m_pResource));

    for (auto it = GetComponents(); it.IsValid(); it.Next())
    {
      if (it->m_hResource == hScript)
      {
        m_ComponentsToUpdate.Insert(it->GetHandle());
      }
    }
  }
}

void xiiVisualScriptComponentManager::Update(const xiiWorldModule::UpdateContext& context)
{
  {
    for (auto hComp : m_ComponentsToUpdate)
    {
      xiiVisualScriptComponent* pComponent = nullptr;
      if (!TryGetComponent(hComp, pComponent))
        continue;

      pComponent->InitScriptInstance();
    }

    m_ComponentsToUpdate.Clear();
  }

  if (GetWorld()->GetWorldSimulationEnabled())
  {
    for (auto it = this->m_ComponentStorage.GetIterator(context.m_uiFirstComponentIndex, context.m_uiComponentCount); it.IsValid(); ++it)
    {
      ComponentType* pComponent = it;
      if (pComponent->IsActiveAndInitialized())
      {
        pComponent->Update();
      }
    }
  }
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
XII_BEGIN_COMPONENT_TYPE(xiiVisualScriptComponent, 5, xiiComponentMode::Static);
{
  XII_BEGIN_PROPERTIES
  {
    XII_ACCESSOR_PROPERTY("Script", GetScriptFile, SetScriptFile)->AddAttributes(new xiiAssetBrowserAttribute("CompatibleAsset_Code_VisualScript", xiiDependencyFlags::Package)),
    XII_MAP_ACCESSOR_PROPERTY("Parameters", GetParameters, GetParameter, SetParameter, RemoveParameter)->AddAttributes(new xiiExposedParametersAttribute("Script"), new xiiExposeColorAlphaAttribute),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("Scripting"),
  }
  XII_END_ATTRIBUTES;
}
XII_END_COMPONENT_TYPE
// clang-format on

xiiVisualScriptComponent::xiiVisualScriptComponent()                                 = default;
xiiVisualScriptComponent::xiiVisualScriptComponent(xiiVisualScriptComponent&& other) = default;
xiiVisualScriptComponent::~xiiVisualScriptComponent()                                = default;

xiiVisualScriptComponent& xiiVisualScriptComponent::operator=(xiiVisualScriptComponent&& other) = default;

void xiiVisualScriptComponent::SerializeComponent(xiiWorldWriter& ref_stream) const
{
  SUPER::SerializeComponent(ref_stream);
  auto& s = ref_stream.GetStream();

  s << m_hResource;
  /// \todo Store the current script state

  // Version 5
  s << m_Params.GetCount();
  for (xiiUInt32 i = 0; i < m_Params.GetCount(); ++i)
  {
    s << m_Params[i].m_sName;
    s << m_Params[i].m_Value;
  }
}

void xiiVisualScriptComponent::DeserializeComponent(xiiWorldReader& ref_stream)
{
  SUPER::DeserializeComponent(ref_stream);
  const xiiUInt32 uiVersion = ref_stream.GetComponentTypeVersion(GetStaticRTTI());
  auto&           s         = ref_stream.GetStream();

  s >> m_hResource;

  if (uiVersion == 3)
  {
    bool globalEventHandler = false; // dummy to prevent early out in SetIsGlobalEventHandler
    s >> globalEventHandler;
    SetGlobalEventHandlerMode(globalEventHandler);
  }

  if (uiVersion >= 3 && uiVersion < 5)
  {
    m_Params.Clear();

    xiiUInt32 numNums, numBools;

    s >> numNums;
    for (xiiUInt32 i = 0; i < numNums; ++i)
    {
      auto& param = m_Params.ExpandAndGetRef();
      s >> param.m_sName;

      double value;
      s >> value;
      param.m_Value = value;
    }

    s >> numBools;
    for (xiiUInt32 i = 0; i < numBools; ++i)
    {
      auto& param = m_Params.ExpandAndGetRef();
      s >> param.m_sName;

      bool value;
      s >> value;
      param.m_Value = value;
    }

    m_bParamsChanged = !m_Params.IsEmpty();
  }

  if (uiVersion >= 5)
  {
    xiiUInt32 numParams = 0;
    s >> numParams;
    m_Params.SetCount(numParams);

    for (xiiUInt32 i = 0; i < m_Params.GetCount(); ++i)
    {
      s >> m_Params[i].m_sName;
      s >> m_Params[i].m_Value;
    }
  }

  /// \todo Read script state
}

void xiiVisualScriptComponent::SetScriptFile(const char* szFile)
{
  xiiVisualScriptResourceHandle hResource;

  if (!xiiStringUtils::IsNullOrEmpty(szFile))
  {
    hResource = xiiResourceManager::LoadResource<xiiVisualScriptResource>(szFile);
  }

  SetScript(hResource);
}

const char* xiiVisualScriptComponent::GetScriptFile() const
{
  if (!m_hResource.IsValid())
    return "";

  return m_hResource.GetResourceID();
}

void xiiVisualScriptComponent::SetScript(const xiiVisualScriptResourceHandle& hResource)
{
  m_hResource = hResource;

  if (m_pScriptInstance != nullptr)
  {
    InitScriptInstance();
  }
}

bool xiiVisualScriptComponent::HandlesMessage(const xiiMessage& msg) const
{
  if (m_pScriptInstance)
  {
    return m_pScriptInstance->HandlesMessage(msg);
  }

  return false;
}

void xiiVisualScriptComponent::Update()
{
  /// \todo Do we really need to tick scripts every frame?

  XII_ASSERT_DEV(m_pScriptInstance != nullptr, "Script instance should have been created at this point");

  const bool bEnableDebugOutput = GetDebugOutput();

  if (bEnableDebugOutput != (m_pActivity != nullptr))
  {
    if (bEnableDebugOutput)
      m_pActivity = XII_DEFAULT_NEW(xiiVisualScriptInstanceActivity);
    else
      m_pActivity.Clear();
  }

  // Script Parameters
  {
    if (m_bParamsChanged)
    {
      m_bParamsChanged = false;

      for (auto& param : m_Params)
      {
        if (param.m_Value.IsA<bool>())
        {
          m_pScriptInstance->GetLocalVariables().StoreBool(param.m_sName, param.m_Value.Get<bool>());
        }
        else if (param.m_Value.IsA<xiiString>())
        {
          m_pScriptInstance->GetLocalVariables().StoreString(param.m_sName, param.m_Value.Get<xiiString>());
        }
        else if (param.m_Value.IsNumber())
        {
          m_pScriptInstance->GetLocalVariables().StoreDouble(param.m_sName, param.m_Value.ConvertTo<double>());
        }
        else
        {
          XII_ASSERT_NOT_IMPLEMENTED;
        }
      }

      // TODO: atm we clear this, because it stores only the initial state, and any mutated state is stored
      // in the VS instance, which we do not sync with
      // therefore if we don't clear, modifying ANY value would reset ALL values to the start value
      m_Params.Clear();
    }
  }

  m_pScriptInstance->ExecuteScript(m_pActivity.Borrow());

  if (bEnableDebugOutput && (!m_pActivity->IsEmpty() || !m_bHadEmptyActivity))
  {
    xiiVisualScriptComponentActivityEvent e;
    e.m_pComponent = this;
    e.m_pActivity  = m_pActivity.Borrow();

    s_ActivityEvents.Broadcast(e);

    // this is to send one 'empty' activity event (but not more), every time a script becomes inactive
    m_bHadEmptyActivity = m_pActivity->IsEmpty();
  }
}

void xiiVisualScriptComponent::InitScriptInstance()
{
  m_pScriptInstance = XII_DEFAULT_NEW(xiiVisualScriptInstance);

  if (m_hResource.IsValid())
  {
    m_pScriptInstance->Configure(m_hResource, this);
  }

  m_bParamsChanged = true;
}

bool xiiVisualScriptComponent::OnUnhandledMessage(xiiMessage& msg, bool bWasPostedMsg)
{
  return m_pScriptInstance->HandleMessage(msg);
}

bool xiiVisualScriptComponent::OnUnhandledMessage(xiiMessage& msg, bool bWasPostedMsg) const
{
  return m_pScriptInstance->HandleMessage(msg);
}

void xiiVisualScriptComponent::Initialize()
{
  SUPER::Initialize();

  EnableUnhandledMessageHandler(true);

  InitScriptInstance();
}

const xiiRangeView<const char*, xiiUInt32> xiiVisualScriptComponent::GetParameters() const
{
  return xiiRangeView<const char*, xiiUInt32>([]() -> xiiUInt32 { return 0; },
                                              [this]() -> xiiUInt32 { return m_Params.GetCount(); }, [](xiiUInt32& ref_uiIt) { ++ref_uiIt; },
                                              [this](const xiiUInt32& uiIt) -> const char* { return m_Params[uiIt].m_sName.GetData(); });
}

void xiiVisualScriptComponent::SetParameter(const char* szKey, const xiiVariant& value)
{
  const xiiTempHashedString th(szKey);

  for (auto& param : m_Params)
  {
    if (param.m_sName == th)
    {
      if (param.m_Value != value)
      {
        m_bParamsChanged = true;
        param.m_Value    = value;
      }
      return;
    }
  }

  m_bParamsChanged = true;
  auto& param      = m_Params.ExpandAndGetRef();
  param.m_sName.Assign(szKey);
  param.m_Value = value;
}

void xiiVisualScriptComponent::RemoveParameter(const char* szKey)
{
  const xiiTempHashedString th(szKey);

  for (xiiUInt32 i = 0; i < m_Params.GetCount(); ++i)
  {
    if (m_Params[i].m_sName == th)
    {
      m_Params.RemoveAtAndSwap(i);
      return;
    }
  }
}

bool xiiVisualScriptComponent::GetParameter(const char* szKey, xiiVariant& out_value) const
{
  const xiiTempHashedString th(szKey);

  for (const auto& param : m_Params)
  {
    if (param.m_sName == th)
    {
      out_value = param.m_Value;
      return true;
    }
  }

  return false;
}

XII_STATICLINK_FILE(GameEngine, GameEngine_VisualScript_Implementation_VisualScriptComponent);

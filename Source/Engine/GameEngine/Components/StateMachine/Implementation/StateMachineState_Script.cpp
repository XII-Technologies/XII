/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GameEngine/GameEnginePCH.h>

#include <Core/Scripting/ScriptWorldModule.h>
#include <Foundation/IO/TypeVersionContext.h>
#include <GameEngine/Components/StateMachine/StateMachineState_Script.h>

namespace
{
  struct ScriptInstanceData
  {
    xiiReflectedClass* m_pOwner = nullptr;

    xiiStateMachineInstance*                        m_pStateMachineInstance = nullptr;
    const xiiArrayMap<xiiHashedString, xiiVariant>* m_pParameters           = nullptr;
    xiiScriptClassResourceHandle                    m_hScriptClass;

    xiiSharedPtr<xiiScriptRTTI>     m_pScriptType;
    xiiUniquePtr<xiiScriptInstance> m_pInstance;

    ~ScriptInstanceData()
    {
      ClearInstance();
    }

    void InstantiateScript(const xiiStateMachineState* pFromState = nullptr)
    {
      ClearInstance();

      xiiResourceLock<xiiScriptClassResource> pScript(m_hScriptClass, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
      if (pScript.GetAcquireResult() != xiiResourceAcquireResult::Final)
      {
        xiiLog::Error("Failed to load script '{}'", m_hScriptClass.GetResourceID());
        return;
      }

      auto pScriptType = pScript->GetType();
      if (pScriptType == nullptr || pScriptType->IsDerivedFrom(xiiGetStaticRTTI<xiiStateMachineState>()) == false)
      {
        xiiLog::Error("Script type '{}' is not a state machine state", pScriptType != nullptr ? pScriptType->GetTypeName() : "NULL");
        return;
      }

      m_pScriptType = pScriptType;

      m_pInstance = pScript->Instantiate(*m_pOwner, m_pStateMachineInstance->GetOwnerWorld());
      if (m_pInstance != nullptr)
      {
        m_pInstance->SetInstanceVariables(*m_pParameters);
      }

      if (xiiWorld* pWorld = m_pStateMachineInstance->GetOwnerWorld())
      {
        pWorld->AddResourceReloadFunction(m_hScriptClass, xiiComponentHandle(), this,
                                          [](xiiWorld::ResourceReloadContext& context) {
                                            static_cast<ScriptInstanceData*>(context.m_pUserData)->ReloadScript();
                                          });
      }

      CallOnEnter(pFromState);
    }

    void ClearInstance()
    {
      CallOnExit(nullptr);

      if (m_pStateMachineInstance != nullptr)
      {
        if (xiiWorld* pWorld = m_pStateMachineInstance->GetOwnerWorld())
        {
          auto pModule = pWorld->GetOrCreateModule<xiiScriptWorldModule>();
          pModule->StopAndDeleteAllCoroutines(m_pInstance.Borrow());

          pWorld->RemoveResourceReloadFunction(m_hScriptClass, xiiComponentHandle(), this);
        }
      }

      m_pInstance   = nullptr;
      m_pScriptType = nullptr;
    }

    void ReloadScript()
    {
      InstantiateScript();
    }

    const xiiAbstractFunctionProperty* GetScriptFunction(xiiUInt32 uiFunctionIndex)
    {
      if (m_pScriptType != nullptr && m_pInstance != nullptr)
      {
        return m_pScriptType->GetFunctionByIndex(uiFunctionIndex);
      }

      return nullptr;
    }

    void CallOnEnter(const xiiStateMachineState* pFromState)
    {
      if (auto pFunction = GetScriptFunction(xiiStateMachineState_ScriptBaseClassFunctions::OnEnter))
      {
        xiiVariant args[] = {m_pStateMachineInstance, pFromState};
        xiiVariant returnValue;
        pFunction->Execute(m_pInstance.Borrow(), xiiMakeArrayPtr(args), returnValue);
      }
    }

    void CallOnExit(const xiiStateMachineState* pToState)
    {
      if (auto pFunction = GetScriptFunction(xiiStateMachineState_ScriptBaseClassFunctions::OnExit))
      {
        xiiVariant args[] = {m_pStateMachineInstance, pToState};
        xiiVariant returnValue;
        pFunction->Execute(m_pInstance.Borrow(), xiiMakeArrayPtr(args), returnValue);
      }
    }

    void CallUpdate(xiiTime deltaTime)
    {
      if (auto pFunction = GetScriptFunction(xiiStateMachineState_ScriptBaseClassFunctions::Update))
      {
        xiiVariant args[] = {m_pStateMachineInstance, deltaTime};
        xiiVariant returnValue;
        pFunction->Execute(m_pInstance.Borrow(), xiiMakeArrayPtr(args), returnValue);
      }
    }
  };
} // namespace

//////////////////////////////////////////////////////////////////////////

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiStateMachineState_Script, 1, xiiRTTIDefaultAllocator<xiiStateMachineState_Script>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ACCESSOR_PROPERTY("ScriptClass", GetScriptClassFile, SetScriptClassFile)->AddAttributes(new xiiAssetBrowserAttribute("CompatibleAsset_ScriptClass")),
    XII_MAP_ACCESSOR_PROPERTY("Parameters", GetParameters, GetParameter, SetParameter, RemoveParameter)->AddAttributes(new xiiExposedParametersAttribute("ScriptClass")),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiStateMachineState_Script::xiiStateMachineState_Script(xiiStringView sName) :
  xiiStateMachineState(sName)
{
}

xiiStateMachineState_Script::~xiiStateMachineState_Script() = default;

void xiiStateMachineState_Script::OnEnter(xiiStateMachineInstance& ref_instance, void* pInstanceData, const xiiStateMachineState* pFromState) const
{
  auto& instanceData = *static_cast<ScriptInstanceData*>(pInstanceData);

  if (instanceData.m_pInstance == nullptr)
  {
    instanceData.m_pOwner                = const_cast<xiiStateMachineState_Script*>(this);
    instanceData.m_pStateMachineInstance = &ref_instance;
    instanceData.m_pParameters           = &m_Parameters;
    instanceData.m_hScriptClass          = xiiResourceManager::LoadResource<xiiScriptClassResource>(m_sScriptClassFile);

    instanceData.InstantiateScript(pFromState);
  }
  else
  {
    instanceData.CallOnEnter(pFromState);
  }
}

void xiiStateMachineState_Script::OnExit(xiiStateMachineInstance& ref_instance, void* pInstanceData, const xiiStateMachineState* pToState) const
{
  auto& instanceData = *static_cast<ScriptInstanceData*>(pInstanceData);
  instanceData.CallOnExit(pToState);
}

void xiiStateMachineState_Script::Update(xiiStateMachineInstance& ref_instance, void* pInstanceData, xiiTime deltaTime) const
{
  auto& instanceData = *static_cast<ScriptInstanceData*>(pInstanceData);
  instanceData.CallUpdate(deltaTime);
}

xiiResult xiiStateMachineState_Script::Serialize(xiiStreamWriter& inout_stream) const
{
  XII_SUCCEED_OR_RETURN(SUPER::Serialize(inout_stream));

  inout_stream << m_sScriptClassFile;

  xiiUInt16 uiNumParams = static_cast<xiiUInt16>(m_Parameters.GetCount());
  inout_stream << uiNumParams;

  for (xiiUInt32 p = 0; p < uiNumParams; ++p)
  {
    inout_stream << m_Parameters.GetKey(p);
    inout_stream << m_Parameters.GetValue(p);
  }

  return XII_SUCCESS;
}

xiiResult xiiStateMachineState_Script::Deserialize(xiiStreamReader& inout_stream)
{
  XII_SUCCEED_OR_RETURN(SUPER::Deserialize(inout_stream));
  const xiiUInt32 uiVersion = xiiTypeVersionReadContext::GetContext()->GetTypeVersion(GetStaticRTTI());
  XII_IGNORE_UNUSED(uiVersion);
  inout_stream >> m_sScriptClassFile;

  xiiUInt16 uiNumParams = 0;
  inout_stream >> uiNumParams;
  m_Parameters.Reserve(uiNumParams);

  xiiHashedString key;
  xiiVariant      value;
  for (xiiUInt32 p = 0; p < uiNumParams; ++p)
  {
    inout_stream >> key;
    inout_stream >> value;

    m_Parameters.Insert(key, value);
  }

  return XII_SUCCESS;
}

bool xiiStateMachineState_Script::GetInstanceDataDesc(xiiInstanceDataDesc& out_desc)
{
  out_desc.FillFromType<ScriptInstanceData>();
  return true;
}

void xiiStateMachineState_Script::SetScriptClassFile(xiiStringView sFile)
{
  m_sScriptClassFile = sFile;

  // Note that we can't load the resource here directly. State machine states are instantiated during
  // state machine asset transform but the script class resource overwrites are not known there so the resource load would fail.
}

const char* xiiStateMachineState_Script::GetScriptClassFile() const
{
  return m_sScriptClassFile;
}

const xiiRangeView<xiiStringView, xiiUInt32> xiiStateMachineState_Script::GetParameters() const
{
  return xiiRangeView<xiiStringView, xiiUInt32>([]() -> xiiUInt32 { return 0; },
                                                [this]() -> xiiUInt32 { return m_Parameters.GetCount(); },
                                                [](xiiUInt32& ref_uiIt) { ++ref_uiIt; },
                                                [this](const xiiUInt32& uiIt) -> xiiStringView { return m_Parameters.GetKey(uiIt).GetString().GetView(); });
}

void xiiStateMachineState_Script::SetParameter(xiiStringView sKey, const xiiVariant& value)
{
  xiiHashedString hs;
  hs.Assign(sKey);

  auto it = m_Parameters.Find(hs);
  if (it != xiiInvalidIndex && m_Parameters.GetValue(it) == value)
    return;

  m_Parameters[hs] = value;
}

void xiiStateMachineState_Script::RemoveParameter(xiiStringView sKey)
{
  if (m_Parameters.RemoveAndCopy(sKey))
  {
  }
}

bool xiiStateMachineState_Script::GetParameter(xiiStringView sKey, xiiVariant& out_value) const
{
  xiiUInt32 it = m_Parameters.Find(sKey);

  if (it == xiiInvalidIndex)
    return false;

  out_value = m_Parameters.GetValue(it);
  return true;
}

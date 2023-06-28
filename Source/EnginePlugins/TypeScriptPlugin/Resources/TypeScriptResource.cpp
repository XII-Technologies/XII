#include <TypeScriptPlugin/TypeScriptPluginPCH.h>

#include <Core/Assets/AssetFileHeader.h>
#include <Core/Scripting/DuktapeContext.h>
#include <Foundation/Configuration/Startup.h>
#include <TypeScriptPlugin/Components/TypeScriptComponent.h>
#include <TypeScriptPlugin/Resources/TypeScriptResource.h>

namespace
{
  class TypeScriptFunctionProperty : public xiiAbstractFunctionProperty
  {
  public:
    TypeScriptFunctionProperty(const char* szPropertyName) :
      xiiAbstractFunctionProperty(szPropertyName)
    {
    }

    virtual xiiFunctionType::Enum         GetFunctionType() const override { return xiiFunctionType::Member; }
    virtual const xiiRTTI*                GetReturnType() const override { return nullptr; }
    virtual xiiBitflags<xiiPropertyFlags> GetReturnFlags() const override { return xiiPropertyFlags::Void; }
    virtual xiiUInt32                     GetArgumentCount() const override { return 0; }
    virtual const xiiRTTI*                GetArgumentType(xiiUInt32 uiParamIndex) const override { return nullptr; }
    virtual xiiBitflags<xiiPropertyFlags> GetArgumentFlags(xiiUInt32 uiParamIndex) const override { return xiiPropertyFlags::Void; }

    virtual void Execute(void* pInstance, xiiArrayPtr<xiiVariant> arguments, xiiVariant& ref_returnValue) const override
    {
      auto                  pTypeScriptInstance = static_cast<xiiTypeScriptInstance*>(pInstance);
      xiiTypeScriptBinding& binding             = pTypeScriptInstance->GetBinding();

      xiiDuktapeHelper duk(binding.GetDukTapeContext());

      // TODO: this needs to be more generic to work with other things besides components
      binding.DukPutComponentObject(&pTypeScriptInstance->GetComponent()); // [ comp ]

      if (duk.PrepareMethodCall(GetPropertyName()).Succeeded()) // [ comp func comp ]
      {
        duk.CallPreparedMethod().IgnoreResult(); // [ comp result ]
        duk.PopStack(2);                         // [ ]

        XII_DUK_RETURN_VOID_AND_VERIFY_STACK(duk, 0);
      }
      else
      {
        // remove 'this'   [ comp ]
        duk.PopStack(); // [ ]

        XII_DUK_RETURN_VOID_AND_VERIFY_STACK(duk, 0);
      }
    }
  };
} // namespace

//////////////////////////////////////////////////////////////////////////

xiiTypeScriptInstance::xiiTypeScriptInstance(xiiComponent& ref_owner, xiiTypeScriptBinding& ref_binding) :
  m_Binding(ref_binding), m_Component(ref_owner)
{
}

void xiiTypeScriptInstance::ApplyParameters(const xiiArrayMap<xiiHashedString, xiiVariant>& parameters)
{
  xiiDuktapeHelper duk(m_Binding.GetDukTapeContext());

  m_Binding.DukPutComponentObject(&m_Component); // [ comp ]

  for (xiiUInt32 p = 0; p < parameters.GetCount(); ++p)
  {
    const auto& pair = parameters.GetPair(p);

    xiiTypeScriptBinding::SetVariantProperty(duk, pair.key.GetString(), -1, pair.value); // [ comp ]
  }

  duk.PopStack(); // [ ]

  XII_DUK_RETURN_VOID_AND_VERIFY_STACK(duk, 0);
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiTypeScriptClassResource, 1, xiiRTTIDefaultAllocator<xiiTypeScriptClassResource>)
XII_END_DYNAMIC_REFLECTED_TYPE;
XII_RESOURCE_IMPLEMENT_COMMON_CODE(xiiTypeScriptClassResource);

XII_BEGIN_SUBSYSTEM_DECLARATION(TypeScript, ClassResource)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "ResourceManager" 
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP 
  {
    xiiResourceManager::RegisterResourceOverrideType(xiiGetStaticRTTI<xiiTypeScriptClassResource>(), [](const xiiStringBuilder& sResourceID) -> bool  {
        return sResourceID.HasExtension(".xiiTypeScriptRes");
    });
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    xiiResourceManager::UnregisterResourceOverrideType(xiiGetStaticRTTI<xiiTypeScriptClassResource>());
  }

XII_END_SUBSYSTEM_DECLARATION;
// clang-format on

xiiTypeScriptClassResource::xiiTypeScriptClassResource()  = default;
xiiTypeScriptClassResource::~xiiTypeScriptClassResource() = default;

xiiResourceLoadDesc xiiTypeScriptClassResource::UnloadData(Unload WhatToUnload)
{
  DeleteScriptType();

  xiiResourceLoadDesc ld;
  ld.m_State                      = xiiResourceState::Unloaded;
  ld.m_uiQualityLevelsDiscardable = 0;
  ld.m_uiQualityLevelsLoadable    = 0;

  return ld;
}

xiiResourceLoadDesc xiiTypeScriptClassResource::UpdateContent(xiiStreamReader* pStream)
{
  xiiResourceLoadDesc ld;
  ld.m_uiQualityLevelsDiscardable = 0;
  ld.m_uiQualityLevelsLoadable    = 0;

  if (pStream == nullptr)
  {
    ld.m_State = xiiResourceState::LoadedResourceMissing;
    return ld;
  }

  // skip the absolute file path data that the standard file reader writes into the stream
  {
    xiiString sAbsFilePath;
    (*pStream) >> sAbsFilePath;
  }

  // skip the asset file header at the start of the file
  xiiAssetFileHeader AssetHash;
  AssetHash.Read(*pStream).IgnoreResult();

  xiiString sTypeName;
  (*pStream) >> sTypeName;
  (*pStream) >> m_Guid;

  xiiScriptRTTI::FunctionList       functions;
  xiiScriptRTTI::MessageHandlerList messageHandlers;

  // TODO: this list should be generated during asset transform and stored in the resource
  const char* szFunctionNames[] = {"Initialize", "Deinitialize", "OnActivated", "OnDeactivated", "OnSimulationStarted", "Tick"};

  for (xiiUInt32 i = 0; i < XII_ARRAY_SIZE(szFunctionNames); ++i)
  {
    functions.PushBack(XII_DEFAULT_NEW(TypeScriptFunctionProperty, szFunctionNames[i]));
  }

  const xiiRTTI* pParentType = xiiGetStaticRTTI<xiiComponent>();
  CreateScriptType(sTypeName, pParentType, std::move(functions), std::move(messageHandlers));

  ld.m_State = xiiResourceState::Loaded;

  return ld;
}

void xiiTypeScriptClassResource::UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage)
{
  out_NewMemoryUsage.m_uiMemoryCPU = (xiiUInt32)sizeof(xiiTypeScriptClassResource);
  out_NewMemoryUsage.m_uiMemoryGPU = 0;
}

xiiUniquePtr<xiiScriptInstance> xiiTypeScriptClassResource::Instantiate(xiiReflectedClass& owner, xiiWorld* pWorld) const
{
  auto pComponent = xiiStaticCast<xiiComponent*>(&owner);

  // TODO: typescript context needs to be moved to a world module
  auto  pTypeScriptComponentManager = static_cast<xiiTypeScriptComponentManager*>(pWorld->GetManagerForComponentType(xiiGetStaticRTTI<xiiTypeScriptComponent>()));
  auto& binding                     = pTypeScriptComponentManager->GetTsBinding();

  xiiTypeScriptBinding::TsComponentTypeInfo componentTypeInfo;
  if (binding.LoadComponent(m_Guid, componentTypeInfo).Failed())
  {
    xiiLog::Error("Failed to load TS component type.");
    return nullptr;
  }

  xiiUInt32 uiStashIdx = 0;
  if (binding.RegisterComponent(m_pType->GetTypeName(), pComponent->GetHandle(), uiStashIdx, false).Failed())
  {
    xiiLog::Error("Failed to register TS component type '{}'. Class may not exist under that name.", m_pType->GetTypeName());
    return nullptr;
  }

  return XII_DEFAULT_NEW(xiiTypeScriptInstance, *pComponent, binding);
}

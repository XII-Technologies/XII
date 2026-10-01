/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsFoundation/GraphicsFoundationPCH.h>

#include <Foundation/Configuration/Startup.h>
#include <GraphicsFoundation/Device/DeviceFactory.h>

struct xiiGALDeviceFactory::CreatorFuncInfo
{
  CreatorFunc                       m_Func;
  xiiEnum<xiiGALGraphicsDeviceType> m_APIType = xiiGALGraphicsDeviceType::Null;
  xiiString                         m_sShaderModel;
  xiiString                         m_sShaderCompiler;
};

class xiiGALDeviceFactory::State
{
public:
  xiiMutex                                  m_Mutex;
  xiiHashTable<xiiString, CreatorFuncInfo>  m_CreatorFunctions;
};

xiiUniquePtr<xiiGALDeviceFactory::State> xiiGALDeviceFactory::s_pState;

// clang-format off
XII_BEGIN_SUBSYSTEM_DECLARATION(GraphicsFoundation, DeviceFactoryRegistry)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "Foundation",
    "DeviceRegistry"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    xiiGALDeviceFactory::Startup();
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    xiiGALDeviceFactory::Shutdown();
  }

XII_END_SUBSYSTEM_DECLARATION;
// clang-format on

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiGALDeviceImplementationDescription, xiiNoBase, 1, xiiRTTIDefaultAllocator<xiiGALDeviceImplementationDescription>)
  {
    XII_BEGIN_PROPERTIES
    {
      XII_ENUM_MEMBER_PROPERTY("APIType", xiiGALGraphicsDeviceType, m_APIType),
      XII_MEMBER_PROPERTY("Name", m_sName),
      XII_MEMBER_PROPERTY("ShaderModel", m_sShaderModel),
      XII_MEMBER_PROPERTY("ShaderCompiler", m_sShaderCompiler),
    } XII_END_PROPERTIES;
  }
XII_END_STATIC_REFLECTED_TYPE;

bool xiiGALDeviceFactory::IsInitialized()
{
  return s_pState != nullptr;
}

bool xiiGALDeviceFactory::GetCreatorFuncInfo(xiiStringView sImplementationName, CreatorFuncInfo& out_info)
{
  XII_ASSERT_DEV(s_pState != nullptr, "The graphics device factory subsystem is not started.");
  if (s_pState == nullptr)
    return false;

  {
    XII_LOCK(s_pState->m_Mutex);
    if (s_pState->m_CreatorFunctions.TryGetValue(sImplementationName, out_info))
      return true;
  }

  // Loading invokes the backend subsystem, which registers its factory through this class.
  // Do not hold the registry mutex while plugin startup runs.
  xiiStringBuilder sPluginName = "xiiGraphics";
  sPluginName.Append(sImplementationName);

  if (xiiPlugin::LoadPlugin(sPluginName).Failed())
  {
    xiiLog::Error("Graphics API plugin '{}' was not found.", sPluginName);
    return false;
  }

  XII_LOCK(s_pState->m_Mutex);
  const bool bRegistered = s_pState->m_CreatorFunctions.TryGetValue(sImplementationName, out_info);
  XII_ASSERT_DEV(bRegistered, "Graphics API plugin '{}' is not registered.", sImplementationName);
  return bRegistered;
}

xiiSharedPtr<xiiGALDevice> xiiGALDeviceFactory::CreateDevice(xiiStringView sImplementationName, xiiAllocator* pAllocator, const xiiGALDeviceCreationDescription& description)
{
  CreatorFuncInfo info;
  if (GetCreatorFuncInfo(sImplementationName, info))
  {
    return info.m_Func(pAllocator, description);
  }
  return xiiInternal::NewInstance<xiiGALDevice>(nullptr, pAllocator);
}

void xiiGALDeviceFactory::RegisterImplementation(xiiStringView sImplementationName, const CreatorFunc& func, const xiiGALDeviceImplementationDescription& description)
{
  XII_ASSERT_DEV(s_pState != nullptr, "The graphics device factory subsystem is not started.");
  if (s_pState == nullptr)
    return;

  CreatorFuncInfo info;
  info.m_Func            = func;
  info.m_APIType         = description.m_APIType;
  info.m_sShaderModel    = description.m_sShaderModel;
  info.m_sShaderCompiler = description.m_sShaderCompiler;

  XII_LOCK(s_pState->m_Mutex);
  XII_VERIFY(s_pState->m_CreatorFunctions.Insert(sImplementationName, info) == false, "Graphics API implementation is already registered.");
}

void xiiGALDeviceFactory::UnregisterImplementation(xiiStringView sImplementationName)
{
  if (s_pState == nullptr)
    return;

  XII_LOCK(s_pState->m_Mutex);
  XII_VERIFY(s_pState->m_CreatorFunctions.Remove(sImplementationName), "Graphics API implementation is not registered.");
}

void xiiGALDeviceFactory::GetShaderModelAndCompiler(xiiStringView sRendererName, xiiStringView& ref_sShaderModel, xiiStringView& ref_sShaderCompiler)
{
  CreatorFuncInfo info;
  if (!GetCreatorFuncInfo(sRendererName, info))
    return;

  XII_LOCK(s_pState->m_Mutex);
  const CreatorFuncInfo* pRegisteredInfo = s_pState->m_CreatorFunctions.GetValue(sRendererName);
  XII_ASSERT_DEV(pRegisteredInfo != nullptr, "Graphics API implementation '{}' was unregistered during lookup.", sRendererName);
  if (pRegisteredInfo == nullptr)
    return;

  ref_sShaderModel    = pRegisteredInfo->m_sShaderModel;
  ref_sShaderCompiler = pRegisteredInfo->m_sShaderCompiler;
}

void xiiGALDeviceFactory::Startup()
{
  XII_ASSERT_DEV(s_pState == nullptr, "Graphics device factory started twice.");
  s_pState = XII_DEFAULT_NEW(State);
}

void xiiGALDeviceFactory::Shutdown()
{
  if (s_pState == nullptr)
    return;

  {
    XII_LOCK(s_pState->m_Mutex);
    XII_ASSERT_DEV(s_pState->m_CreatorFunctions.IsEmpty(), "Graphics API implementations must unregister before the device factory shuts down.");
    s_pState->m_CreatorFunctions.Clear();
  }
  s_pState.Clear();
}

XII_STATICLINK_FILE(GraphicsFoundation, GraphicsFoundation_Device_Implementation_DeviceFactory);

#include <RendererFoundation/RendererFoundationPCH.h>

#include <RendererFoundation/Device/DeviceFactory.h>

struct CreatorFuncInfo
{
  xiiGALDeviceFactory::CreatorFunc m_Func;
  xiiString                        m_sShaderModel;
  xiiString                        m_sShaderCompiler;
};

static xiiHashTable<xiiString, CreatorFuncInfo> s_CreatorFuncs;
static xiiHashTable<xiiString, const char*>     s_LibraryNames;

CreatorFuncInfo* GetCreatorFuncInfo(const char* szRendererName)
{
  auto pFuncInfo = s_CreatorFuncs.GetValue(szRendererName);
  if (pFuncInfo == nullptr)
  {
    xiiStringBuilder sLibraryName = *s_LibraryNames.GetValue(szRendererName);
    XII_ASSERT_DEV(sLibraryName != nullptr, "Renderer library name is unknown");
    XII_ASSERT_DEV(!sLibraryName.IsEmpty(), "Renderer library name must not be empty");

    XII_VERIFY(xiiPlugin::LoadPlugin(sLibraryName).Succeeded(), "Renderer plugin '{}' not found", sLibraryName);

    pFuncInfo = s_CreatorFuncs.GetValue(szRendererName);
    XII_ASSERT_DEV(pFuncInfo != nullptr, "Renderer '{}' is not registered", szRendererName);
  }

  return pFuncInfo;
}

xiiGraphicsDeviceType::Enum xiiGALDeviceFactory::GetGraphicsDevice(const char* szRendererName)
{
  if (szRendererName == "D3D11")
    return xiiGraphicsDeviceType::D3D11;
  if (szRendererName == "D3D12")
    return xiiGraphicsDeviceType::D3D12;
  if (szRendererName == "Vulkan")
    return xiiGraphicsDeviceType::Vulkan;
  if (szRendererName == "Metal")
    return xiiGraphicsDeviceType::Metal;
  if (szRendererName == "OpenGL")
    return xiiGraphicsDeviceType::OpenGL;
  if (szRendererName == "OpenGLES")
    return xiiGraphicsDeviceType::OpenGLES;

  return xiiGraphicsDeviceType::Undefined;
}

xiiInternal::NewInstance<xiiGALDevice> xiiGALDeviceFactory::CreateDevice(const char* szRendererName, xiiAllocatorBase* pAllocator, const xiiGALDeviceCreationDescription& desc)
{
  if (auto pFuncInfo = GetCreatorFuncInfo(szRendererName))
  {
    return pFuncInfo->m_Func(pAllocator, desc);
  }

  return xiiInternal::NewInstance<xiiGALDevice>(nullptr, pAllocator);
}

void xiiGALDeviceFactory::GetShaderModelAndCompiler(const char* szRendererName, const char*& szShaderModel, const char*& szShaderCompiler)
{
  if (auto pFuncInfo = GetCreatorFuncInfo(szRendererName))
  {
    szShaderModel    = pFuncInfo->m_sShaderModel;
    szShaderCompiler = pFuncInfo->m_sShaderCompiler;
  }
}

void xiiGALDeviceFactory::RegisterCreatorFunc(const char* szRendererName, const CreatorFunc& func, const char* szShaderModel, const char* szShaderCompiler)
{
  CreatorFuncInfo funcInfo;
  funcInfo.m_Func            = func;
  funcInfo.m_sShaderModel    = szShaderModel;
  funcInfo.m_sShaderCompiler = szShaderCompiler;

  XII_VERIFY(s_CreatorFuncs.Insert(szRendererName, funcInfo) == false, "Creator func already registered");
}

void xiiGALDeviceFactory::UnregisterCreatorFunc(const char* szRendererName)
{
  XII_VERIFY(s_CreatorFuncs.Remove(szRendererName), "Creator func is not registered");
  XII_VERIFY(s_LibraryNames.Remove(szRendererName), "Library name is not registered");
}

void xiiGALDeviceFactory::ConfigureLibraryName(const char* szRendererName, const char* szLibraryName)
{
  XII_VERIFY(s_LibraryNames.Insert(szRendererName, szLibraryName) == false, "Library name already registered");
}


XII_STATICLINK_FILE(RendererFoundation, RendererFoundation_Device_Implementation_DeviceFactory);

#include <RendererFoundation/RendererFoundationPCH.h>

#include <RendererFoundation/Device/DeviceFactory.h>

struct CreatorFuncInfo
{
  xiiGALDeviceFactory::CreatorFunc m_Func;
  xiiString                        m_sShaderModel;
  xiiString                        m_sShaderCompiler;
};

static xiiHashTable<xiiString, CreatorFuncInfo> s_CreatorFuncs;

CreatorFuncInfo* GetCreatorFuncInfo(const char* szRendererName)
{
  auto pFuncInfo = s_CreatorFuncs.GetValue(szRendererName);
  if (pFuncInfo == nullptr)
  {
    xiiStringBuilder sPluginName = "xiiRenderer";
    sPluginName.Append(szRendererName);

    XII_VERIFY(xiiPlugin::LoadPlugin(sPluginName).Succeeded(), "Renderer plugin '{}' not found", sPluginName);

    pFuncInfo = s_CreatorFuncs.GetValue(szRendererName);
    XII_ASSERT_DEV(pFuncInfo != nullptr, "Renderer '{}' is not registered", szRendererName);
  }

  return pFuncInfo;
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
  XII_VERIFY(s_CreatorFuncs.Remove(szRendererName), "Creator func not registered");
}

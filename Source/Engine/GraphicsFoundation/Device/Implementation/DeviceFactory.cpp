#include <GraphicsFoundation/GraphicsFoundationPCH.h>

#include <GraphicsFoundation/Device/DeviceFactory.h>

struct CreatorFuncInfo
{
  xiiGALDeviceFactory::CreatorFunc m_Func;
  xiiString                        m_sShaderModel;
  xiiString                        m_sShaderCompiler;
};

static xiiHashTable<xiiString, CreatorFuncInfo> s_CreatorFunctions;

CreatorFuncInfo* GetCreatorFuncInfo(xiiStringView sRendererName)
{
  auto pFuncInfo = s_CreatorFunctions.GetValue(sRendererName);
  if (pFuncInfo == nullptr)
  {
    xiiStringBuilder sPluginName = "xiiGraphics";
    sPluginName.Append(sRendererName);

    XII_VERIFY(xiiPlugin::LoadPlugin(sPluginName).Succeeded(), "Graphics API plugin '{}' not found.", sPluginName);

    pFuncInfo = s_CreatorFunctions.GetValue(sRendererName);
    XII_ASSERT_DEV(pFuncInfo != nullptr, "Graphics API plugin '{}' is not registered.", sRendererName);
  }

  return pFuncInfo;
}

xiiInternal::NewInstance<xiiGALDevice> xiiGALDeviceFactory::CreateDevice(xiiStringView sRendererName, xiiAllocatorBase* pAllocator, const xiiGALDeviceCreationDescription& description)
{
  if (auto pFuncInfo = GetCreatorFuncInfo(sRendererName))
  {
    return pFuncInfo->m_Func(pAllocator, description);
  }

  return xiiInternal::NewInstance<xiiGALDevice>(nullptr, pAllocator);
}

void xiiGALDeviceFactory::GetShaderModelAndCompiler(xiiStringView sRendererName, const char*& ref_szShaderModel, const char*& ref_szShaderCompiler)
{
  if (auto pFuncInfo = GetCreatorFuncInfo(sRendererName))
  {
    ref_szShaderModel    = pFuncInfo->m_sShaderModel;
    ref_szShaderCompiler = pFuncInfo->m_sShaderCompiler;
  }
}

void xiiGALDeviceFactory::RegisterCreatorFunc(const char* szRendererName, const CreatorFunc& func, const char* szShaderModel, const char* szShaderCompiler)
{
  CreatorFuncInfo funcInfo;
  funcInfo.m_Func            = func;
  funcInfo.m_sShaderModel    = szShaderModel;
  funcInfo.m_sShaderCompiler = szShaderCompiler;

  XII_VERIFY(s_CreatorFunctions.Insert(szRendererName, funcInfo) == false, "Creator function is already registered");
}

void xiiGALDeviceFactory::UnregisterCreatorFunc(const char* szRendererName)
{
  XII_VERIFY(s_CreatorFunctions.Remove(szRendererName), "Creator function is not registered.");
}

XII_STATICLINK_FILE(GraphicsFoundation, GraphicsFoundation_Device_Implementation_DeviceFactory);

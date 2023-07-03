#include <RendererFoundation/RendererFoundationPCH.h>

#include <Foundation/Logging/Log.h>
#include <RendererFoundation/Device/DeviceFactory.h>

#include <Foundation/Logging/Log.h>

struct CreatorFuncInfo
{
  xiiGALDeviceFactory::CreatorFunc m_Func;
  xiiString                        m_sShaderModel;
  xiiString                        m_sShaderCompiler;
};

static xiiHashTable<xiiString, CreatorFuncInfo> s_CreatorFuncs;
static xiiHashTable<xiiString, const char*>     s_LibraryNames;

CreatorFuncInfo* GetCreatorFuncInfo(xiiStringView sRendererName)
{
  auto pFuncInfo = s_CreatorFuncs.GetValue(sRendererName);
  if (pFuncInfo == nullptr)
  {
    xiiStringBuilder sLibraryName = *s_LibraryNames.GetValue(sRendererName);
    XII_ASSERT_DEV(sLibraryName != nullptr, "Renderer library name is unknown");
    XII_ASSERT_DEV(!sLibraryName.IsEmpty(), "Renderer library name must not be empty");

    XII_VERIFY(xiiPlugin::LoadPlugin(sLibraryName).Succeeded(), "Renderer plugin '{}' not found", sLibraryName);

    pFuncInfo = s_CreatorFuncs.GetValue(sRendererName);
    XII_ASSERT_DEV(pFuncInfo != nullptr, "Renderer '{}' is not registered", sRendererName);
  }

  return pFuncInfo;
}

xiiInternal::NewInstance<xiiGALDevice> xiiGALDeviceFactory::CreateDevice(xiiStringView sRendererName, xiiAllocatorBase* pAllocator, const xiiGALDeviceCreationDescription& desc)
{
  if (auto pFuncInfo = GetCreatorFuncInfo(sRendererName))
  {
    return pFuncInfo->m_Func(pAllocator, desc);
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

void xiiGALDeviceFactory::RegisterCreatorFunc(xiiStringView sRendererName, const CreatorFunc& func, xiiStringView sShaderModel, xiiStringView sShaderCompiler)
{
  CreatorFuncInfo funcInfo;
  funcInfo.m_Func            = func;
  funcInfo.m_sShaderModel    = sShaderModel;
  funcInfo.m_sShaderCompiler = sShaderCompiler;

  XII_VERIFY(s_CreatorFuncs.Insert(sRendererName, funcInfo) == false, "Creator func already registered.");
}

void xiiGALDeviceFactory::UnregisterCreatorFunc(xiiStringView sRendererName)
{
  XII_VERIFY(s_CreatorFuncs.Remove(sRendererName), "Creator func is not registered.");
}

void xiiGALDeviceFactory::RegisterLibraryName(xiiStringView sRendererName, xiiStringView sLibraryName)
{
  XII_VERIFY(s_LibraryNames.Insert(sRendererName, sLibraryName) == false, "Library name already registered.");

  xiiLog::Info("Registered Renderer Name '{0}' with library '{1}'.", sRendererName, sLibraryName);
}

void xiiGALDeviceFactory::UnregisterLibraryName(xiiStringView sRendererName)
{
  XII_VERIFY(s_LibraryNames.Remove(sRendererName), "Library name is not registered.");

  xiiLog::Info("Unregistered Renderer Name '{}'.", sRendererName);
}

XII_STATICLINK_FILE(RendererFoundation, RendererFoundation_Device_Implementation_DeviceFactory);

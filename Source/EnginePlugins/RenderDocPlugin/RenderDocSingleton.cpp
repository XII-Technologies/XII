/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <RenderDocPlugin/RenderDocPluginPCH.h>

#include <Foundation/Basics/Platform/Windows/IncludeWindows.h>
#include <Foundation/Configuration/CVar.h>
#include <Foundation/Utilities/CommandLineOptions.h>
#include <Foundation/Utilities/CommandLineUtils.h>

#include <RenderDocPlugin/RenderDocSingleton.h>
#include <RenderDocPlugin/ThirdParty/renderdoc_app.h>

XII_IMPLEMENT_SINGLETON(xiiRenderDoc);

static xiiCommandLineOptionBool opt_NoCaptures("RenderDoc", "-NoCaptures", "Disables RenderDoc capture support.", false);

static xiiRenderDoc g_RenderDocSingleton;

xiiRenderDoc::xiiRenderDoc() :
  m_SingletonRegistrar(this)
{
  if (opt_NoCaptures.GetOptionValue(xiiCommandLineOption::LogMode::AlwaysIfSpecified))
  {
    xiiLog::Info("RenderDoc plugin: Initialization suppressed via -NoCaptures.");
    m_bInitAttempted = true;
    return;
  }

  TryInitializeInternal();
}

xiiRenderDoc::~xiiRenderDoc()
{
  m_pRenderDocAPI = nullptr;

  if (m_pHandleToFree)
  {
    FreeLibrary(xiiMinWindows::ToNative(m_pHandleToFree));
    m_pHandleToFree = nullptr;
  }
}

bool xiiRenderDoc::IsInitialized() const
{
  return m_pRenderDocAPI != nullptr;
}

void xiiRenderDoc::SetAbsCaptureFilePathTemplate(xiiStringView sFilePathTemplate)
{
  if (m_pRenderDocAPI)
  {
    xiiStringBuilder tmp;
    m_pRenderDocAPI->SetCaptureFilePathTemplate(sFilePathTemplate.GetData(tmp));
  }
}

xiiStringView xiiRenderDoc::GetAbsCaptureFilePathTemplate() const
{
  if (m_pRenderDocAPI)
  {
    return m_pRenderDocAPI->GetCaptureFilePathTemplate();
  }

  return {};
}

void xiiRenderDoc::StartFrameCapture(xiiWindowHandle hWnd)
{
  if (m_pRenderDocAPI)
  {
    m_pRenderDocAPI->StartFrameCapture(nullptr, hWnd);
  }
}

bool xiiRenderDoc::IsFrameCapturing() const
{
  return m_pRenderDocAPI ? m_pRenderDocAPI->IsFrameCapturing() : false;
}

void xiiRenderDoc::EndFrameCaptureAndWriteOutput(xiiWindowHandle hWnd)
{
  if (m_pRenderDocAPI)
  {
    m_pRenderDocAPI->EndFrameCapture(nullptr, hWnd);
  }
}

void xiiRenderDoc::EndFrameCaptureAndDiscardResult(xiiWindowHandle hWnd)
{
  if (m_pRenderDocAPI)
  {
    m_pRenderDocAPI->DiscardFrameCapture(nullptr, hWnd);
  }
}

xiiResult xiiRenderDoc::GetLastAbsCaptureFileName(xiiStringBuilder& out_sFileName) const
{
  if (m_pRenderDocAPI && m_pRenderDocAPI->GetNumCaptures() > 0)
  {
    xiiUInt32 uiNumCaptures    = m_pRenderDocAPI->GetNumCaptures();
    xiiUInt32 uiFilePathLength = 0;
    if (m_pRenderDocAPI->GetCapture(uiNumCaptures - 1, nullptr, &uiFilePathLength, nullptr))
    {
      xiiHybridArray<char, 128> filePathBuffer;
      filePathBuffer.SetCount(uiFilePathLength);
      m_pRenderDocAPI->GetCapture(uiNumCaptures - 1, filePathBuffer.GetArrayPtr().GetPtr(), nullptr, nullptr);
      out_sFileName = filePathBuffer.GetArrayPtr().GetPtr();

      return XII_SUCCESS;
    }
  }

  return XII_FAILURE;
}

bool xiiRenderDoc::TryInitializeInternal()
{
  if (m_bInitAttempted)
    return m_pRenderDocAPI != nullptr;

  m_bInitAttempted = true;

  HMODULE hHandle = TryFindOrLoadRenderDocDll();
  if (!hHandle)
  {
    xiiLog::Info("RenderDoc plugin: 'renderdoc.dll' not found. Capture disabled.");
    return false;
  }

  if (!AcquireApiFromDll(hHandle))
  {
    xiiLog::Warning("RenderDoc plugin: Failed to acquire RenderDoc API.");
    if (m_pHandleToFree)
    {
      FreeLibrary(xiiMinWindows::ToNative(m_pHandleToFree));
      m_pHandleToFree = nullptr;
    }
    return false;
  }

  // Configure RenderDoc defaults
  m_pRenderDocAPI->SetCaptureKeys(nullptr, 0);
  m_pRenderDocAPI->SetFocusToggleKeys(nullptr, 0);
  m_pRenderDocAPI->MaskOverlayBits(0, 0);

  xiiLog::Info("RenderDoc plugin: Successfully initialized.");
  return true;
}

HMODULE xiiRenderDoc::TryFindOrLoadRenderDocDll()
{
  if (HMODULE hHandle = TryUseExistingModule())
    return hHandle;

  if (HMODULE hHandle = TryLoadFromEnvVar())
    return hHandle;

  if (HMODULE hHandle = TryLoadFromRegistry())
    return hHandle;

  if (HMODULE hHandle = TryLoadFromFallbackPath())
    return hHandle;

  return nullptr;
}

HMODULE xiiRenderDoc::TryUseExistingModule()
{
  HMODULE dll = GetModuleHandleW(L"renderdoc.dll");
  if (dll)
  {
    xiiLog::Info("RenderDoc plugin: Found injected 'renderdoc.dll'.");
    return dll;
  }
  return nullptr;
}

HMODULE xiiRenderDoc::TryLoadFromEnvVar()
{
  wchar_t path[MAX_PATH];
  DWORD   uiLength = GetEnvironmentVariableW(L"RENDERDOC_DLL_PATH", path, MAX_PATH);

  if (uiLength == 0 || uiLength >= MAX_PATH)
    return nullptr;

  xiiStringBuilder sPath;
  sPath = xiiStringWChar(path);
  xiiLog::Info("RenderDoc plugin: Trying DLL from RENDERDOC_DLL_PATH: '{0}'", sPath);

  HMODULE hHandle = LoadLibraryW(path);
  if (hHandle)
  {
    m_pHandleToFree = xiiMinWindows::FromNative(hHandle);
    return hHandle;
  }

  xiiLog::Warning("RenderDoc plugin: Failed to load DLL from RENDERDOC_DLL_PATH.");
  return nullptr;
}

HMODULE xiiRenderDoc::TryLoadFromRegistry()
{
  HKEY hKey    = nullptr;
  LONG iResult = RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\RenderDoc", 0, KEY_READ, &hKey);
  if (iResult != ERROR_SUCCESS)
    return nullptr;

  wchar_t installPath[MAX_PATH];
  DWORD   uiSize = sizeof(installPath);
  iResult        = RegGetValueW(hKey, nullptr, L"InstallPath", RRF_RT_REG_SZ, nullptr, installPath, &uiSize);
  RegCloseKey(hKey);

  if (iResult != ERROR_SUCCESS)
    return nullptr;

  xiiStringBuilder sDLLPath;
  sDLLPath = xiiStringWChar(installPath);
  sDLLPath.AppendPath("renderdoc.dll");

  xiiLog::Info("RenderDoc plugin: Trying DLL from registry: '{0}'", sDLLPath);

  HMODULE hHandle = LoadLibraryW(xiiStringWChar(sDLLPath));
  if (hHandle)
  {
    m_pHandleToFree = xiiMinWindows::FromNative(hHandle);
    return hHandle;
  }

  xiiLog::Warning("RenderDoc plugin: Failed to load DLL from registry path.");
  return nullptr;
}

HMODULE xiiRenderDoc::TryLoadFromFallbackPath()
{
#if XII_ENABLED(XII_PLATFORM_64BIT)
  const wchar_t* paths[] = {
    L"C:\\Program Files\\RenderDoc\\renderdoc.dll",
  };
#else
  const wchar_t* paths[] = {
    L"C:\\Program Files\\RenderDoc\\x86\\renderdoc.dll",
  };
#endif

  xiiStringBuilder sBasePath;
  for (auto p : paths)
  {
    sBasePath = xiiStringWChar(p);
    xiiLog::Info("RenderDoc plugin: Trying fallback DLL path: '{0}'", sBasePath);

    HMODULE hHandle = LoadLibraryW(p);
    if (hHandle)
    {
      m_pHandleToFree = xiiMinWindows::FromNative(hHandle);
      return hHandle;
    }
  }

  return nullptr;
}

bool xiiRenderDoc::AcquireApiFromDll(HMODULE hHandle)
{
  auto GetAPI = (pRENDERDOC_GetAPI)GetProcAddress(hHandle, "RENDERDOC_GetAPI");
  if (!GetAPI)
  {
    xiiLog::Warning("RenderDoc plugin: RENDERDOC_GetAPI not found.");
    return false;
  }

  void*    pApi         = nullptr;
  xiiInt32 iBestVersion = 0;

  for (xiiInt32 v = eRENDERDOC_API_Version_1_6_0; v > 0; --v)
  {
    if (GetAPI((RENDERDOC_Version)v, &pApi))
    {
      iBestVersion = v;
      break;
    }
  }

  if (!iBestVersion || !pApi)
  {
    xiiLog::Warning("RenderDoc plugin: No compatible API version found.");
    return false;
  }

  m_pRenderDocAPI = (RENDERDOC_API_1_6_0*)pApi;

  xiiLog::Info("RenderDoc plugin: Using RenderDoc API version {0}.", iBestVersion);
  return true;
}

XII_STATICLINK_FILE(RenderDocPlugin, RenderDocPlugin_RenderDocSingleton);

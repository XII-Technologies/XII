#include <RenderDocPlugin/RenderDocPluginPCH.h>

#include <Foundation/Basics/Platform/Win/IncludeWindows.h>
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
    xiiLog::Info("RenderDoc plugin: Initialization suppressed via command-line.");
    return;
  }

  HMODULE dllHandle = GetModuleHandleW(L"renderdoc.dll");
  if (!dllHandle)
  {
    dllHandle       = LoadLibraryW(L"renderdoc.dll");
    m_pHandleToFree = xiiMinWindows::FromNative(dllHandle);
  }

  if (!dllHandle)
  {
    xiiLog::Info("RenderDoc plugin: 'renderdoc.dll' could not be found. Frame captures aren't possible.");
    return;
  }

  if (pRENDERDOC_GetAPI RenderDoc_GetAPI = (pRENDERDOC_GetAPI)GetProcAddress(dllHandle, "RENDERDOC_GetAPI"))
  {
    void* pApi = nullptr;
    RenderDoc_GetAPI(eRENDERDOC_API_Version_1_4_0, &pApi);
    m_pRenderDocAPI = (RENDERDOC_API_1_4_1*)pApi;
  }

  if (m_pRenderDocAPI)
  {
    m_pRenderDocAPI->SetCaptureKeys(nullptr, 0);
    m_pRenderDocAPI->SetFocusToggleKeys(nullptr, 0);
    m_pRenderDocAPI->MaskOverlayBits(0, 0);
  }
  else
  {
    xiiLog::Warning("RenderDoc plugin: Unable to retrieve API pointer from DLL. Potentially outdated version. Frame captures aren't possible.");
  }
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

void xiiRenderDoc::SetAbsCaptureFilePathTemplate(const char* szFilePathTemplate)
{
  if (m_pRenderDocAPI)
  {
    m_pRenderDocAPI->SetCaptureFilePathTemplate(szFilePathTemplate);
  }
}

const char* xiiRenderDoc::GetAbsCaptureFilePathTemplate() const
{
  if (m_pRenderDocAPI)
  {
    return m_pRenderDocAPI->GetCaptureFilePathTemplate();
  }
  return nullptr;
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

XII_STATICLINK_FILE(RenderDocPlugin, RenderDocPlugin_RenderDocSingleton);

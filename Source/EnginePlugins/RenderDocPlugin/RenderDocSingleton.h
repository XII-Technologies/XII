#pragma once

#include <Core/Interfaces/FrameCaptureInterface.h>
#include <Foundation/Configuration/Plugin.h>
#include <Foundation/Configuration/Singleton.h>
#include <RenderDocPlugin/RenderDocPluginDLL.h>

struct RENDERDOC_API_1_4_1;

/// \brief RenderDoc implementation of the xiiFrameCaptureInterface interface
///
/// Adds support for capturing frames through RenderDoc.
/// When the plugin gets loaded, a xiiRenderDoc instance is created and initialized.
/// It tries to find a RenderDoc DLL dynamically, so for initialization to succeed,
/// the DLL has to be available in some search directory (e.g. binary folder or PATH).
/// If an outdated RenderDoc DLL is found, initialization will fail and the plugin will be deactivated.
///
/// For interface documentation see \ref xiiFrameCaptureInterface
class XII_RENDERDOCPLUGIN_DLL xiiRenderDoc : public xiiFrameCaptureInterface
{
  XII_DECLARE_SINGLETON_OF_INTERFACE(xiiRenderDoc, xiiFrameCaptureInterface);

public:
  xiiRenderDoc();
  virtual ~xiiRenderDoc();

  virtual bool        IsInitialized() const override;
  virtual void          SetAbsCaptureFilePathTemplate(xiiStringView sFilePathTemplate) override;
  virtual xiiStringView GetAbsCaptureFilePathTemplate() const override;
  virtual void        StartFrameCapture(xiiWindowHandle hWnd) override;
  virtual bool        IsFrameCapturing() const override;
  virtual void        EndFrameCaptureAndWriteOutput(xiiWindowHandle hWnd) override;
  virtual void        EndFrameCaptureAndDiscardResult(xiiWindowHandle hWnd) override;
  virtual xiiResult   GetLastAbsCaptureFileName(xiiStringBuilder& out_sFileName) const override;

private:
  RENDERDOC_API_1_4_1*   m_pRenderDocAPI = nullptr;
  xiiMinWindows::HMODULE m_pHandleToFree = nullptr;
};

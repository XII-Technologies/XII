/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Core/CorePCH.h>

#include <Core/System/Window.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/IO/OpenDdlReader.h>
#include <Foundation/IO/OpenDdlUtils.h>
#include <Foundation/IO/OpenDdlWriter.h>
#include <Foundation/System/Screen.h>

xiiUInt8 xiiWindow::s_uiNextUnusedWindowNumber = 0;

xiiResult xiiWindowCreationDescription::AdjustWindowSizeAndPosition()
{
  xiiHybridArray<xiiScreenInfo, 2> screens;
  if (xiiScreen::EnumerateScreens(screens).Failed() || screens.IsEmpty())
    return XII_FAILURE;

  xiiInt32 iShowOnMonitor = m_iMonitor;

  if (iShowOnMonitor >= (xiiInt32)screens.GetCount())
    iShowOnMonitor = -1;

  const xiiScreenInfo* pScreen = nullptr;

  // This means 'pick the primary screen'
  if (iShowOnMonitor < 0)
  {
    pScreen = &screens[0];

    for (xiiUInt32 i = 0; i < screens.GetCount(); ++i)
    {
      if (screens[i].m_bIsPrimary)
      {
        pScreen = &screens[i];
        break;
      }
    }
  }
  else
  {
    pScreen = &screens[iShowOnMonitor];
  }

  if (m_WindowMode == xiiWindowMode::FullscreenBorderlessNativeResolution)
  {
    m_Resolution.width  = pScreen->m_iResolutionX;
    m_Resolution.height = pScreen->m_iResolutionY;
  }
  else
  {
    // Clamp the resolution to the native resolution ?
    // m_ClientAreaSize.width = xiiMath::Min<xiiUInt32>(m_ClientAreaSize.width, pScreen->m_iResolutionX);
    // m_ClientAreaSize.height= xiiMath::Min<xiiUInt32>(m_ClientAreaSize.height,pScreen->m_iResolutionY);
  }

  if (m_bCenterWindowOnDisplay)
  {
    m_Position.Set(pScreen->m_iOffsetX + (pScreen->m_iResolutionX - (xiiInt32)m_Resolution.width) / 2, pScreen->m_iOffsetY + (pScreen->m_iResolutionY - (xiiInt32)m_Resolution.height) / 2);
  }
  else
  {
    m_Position.Set(pScreen->m_iOffsetX, pScreen->m_iOffsetY);
  }

  return XII_SUCCESS;
}

void xiiWindowCreationDescription::SaveToDDL(xiiOpenDdlWriter& ref_writer)
{
  ref_writer.BeginObject("WindowDesc");

  xiiOpenDdlUtils::StoreString(ref_writer, m_Title, "Title");

  switch (m_WindowMode.GetValue())
  {
    case xiiWindowMode::FullscreenBorderlessNativeResolution:
      xiiOpenDdlUtils::StoreString(ref_writer, "Borderless", "Mode");
      break;
    case xiiWindowMode::FullscreenFixedResolution:
      xiiOpenDdlUtils::StoreString(ref_writer, "Fullscreen", "Mode");
      break;
    case xiiWindowMode::WindowFixedResolution:
      xiiOpenDdlUtils::StoreString(ref_writer, "Window", "Mode");
      break;
    case xiiWindowMode::WindowResizable:
      xiiOpenDdlUtils::StoreString(ref_writer, "ResizableWindow", "Mode");
      break;
  }

  if (m_uiWindowNumber != 0)
    xiiOpenDdlUtils::StoreUInt8(ref_writer, m_uiWindowNumber, "Index");

  if (m_iMonitor >= 0)
    xiiOpenDdlUtils::StoreInt8(ref_writer, m_iMonitor, "Monitor");

  if (m_Position != xiiVec2I32(0x80000000, 0x80000000))
  {
    xiiOpenDdlUtils::StoreVec2I(ref_writer, m_Position, "Position");
  }

  xiiOpenDdlUtils::StoreVec2U(ref_writer, xiiVec2U32(m_Resolution.width, m_Resolution.height), "Resolution");

  xiiOpenDdlUtils::StoreBool(ref_writer, m_bClipMouseCursor, "ClipMouseCursor");
  xiiOpenDdlUtils::StoreBool(ref_writer, m_bShowMouseCursor, "ShowMouseCursor");
  xiiOpenDdlUtils::StoreBool(ref_writer, m_bSetForegroundOnInit, "SetForegroundOnInit");
  xiiOpenDdlUtils::StoreBool(ref_writer, m_bCenterWindowOnDisplay, "CenterWindowOnDisplay");

  ref_writer.EndObject();
}


xiiResult xiiWindowCreationDescription::SaveToDDL(xiiStringView sFile)
{
  xiiFileWriter file;
  XII_SUCCEED_OR_RETURN(file.Open(sFile));

  xiiOpenDdlWriter writer;
  writer.SetOutputStream(&file);

  SaveToDDL(writer);

  return XII_SUCCESS;
}

void xiiWindowCreationDescription::LoadFromDDL(const xiiOpenDdlReaderElement* pParentElement)
{
  if (const xiiOpenDdlReaderElement* pDesc = pParentElement->FindChildOfType("WindowDesc"))
  {
    if (const xiiOpenDdlReaderElement* pTitle = pDesc->FindChildOfType(xiiOpenDdlPrimitiveType::String, "Title"))
      m_Title = pTitle->GetPrimitivesString()[0];

    if (const xiiOpenDdlReaderElement* pMode = pDesc->FindChildOfType(xiiOpenDdlPrimitiveType::String, "Mode"))
    {
      auto mode = pMode->GetPrimitivesString()[0];

      if (mode == "Borderless")
        m_WindowMode = xiiWindowMode::FullscreenBorderlessNativeResolution;
      else if (mode == "Fullscreen")
        m_WindowMode = xiiWindowMode::FullscreenFixedResolution;
      else if (mode == "Window")
        m_WindowMode = xiiWindowMode::WindowFixedResolution;
      else if (mode == "ResizableWindow")
        m_WindowMode = xiiWindowMode::WindowResizable;
    }

    if (const xiiOpenDdlReaderElement* pIndex = pDesc->FindChildOfType(xiiOpenDdlPrimitiveType::UInt8, "Index"))
    {
      m_uiWindowNumber = pIndex->GetPrimitivesUInt8()[0];
    }

    if (const xiiOpenDdlReaderElement* pMonitor = pDesc->FindChildOfType(xiiOpenDdlPrimitiveType::Int8, "Monitor"))
    {
      m_iMonitor = pMonitor->GetPrimitivesInt8()[0];
    }

    if (const xiiOpenDdlReaderElement* pPosition = pDesc->FindChild("Position"))
    {
      xiiOpenDdlUtils::ConvertToVec2I(pPosition, m_Position).IgnoreResult();
    }

    if (const xiiOpenDdlReaderElement* pPosition = pDesc->FindChild("Resolution"))
    {
      xiiVec2U32 res;
      xiiOpenDdlUtils::ConvertToVec2U(pPosition, res).IgnoreResult();
      m_Resolution.width  = res.x;
      m_Resolution.height = res.y;
    }

    if (const xiiOpenDdlReaderElement* pClipMouseCursor = pDesc->FindChildOfType(xiiOpenDdlPrimitiveType::Bool, "ClipMouseCursor"))
      m_bClipMouseCursor = pClipMouseCursor->GetPrimitivesBool()[0];

    if (const xiiOpenDdlReaderElement* pShowMouseCursor = pDesc->FindChildOfType(xiiOpenDdlPrimitiveType::Bool, "ShowMouseCursor"))
      m_bShowMouseCursor = pShowMouseCursor->GetPrimitivesBool()[0];

    if (const xiiOpenDdlReaderElement* pSetForegroundOnInit = pDesc->FindChildOfType(xiiOpenDdlPrimitiveType::Bool, "SetForegroundOnInit"))
      m_bSetForegroundOnInit = pSetForegroundOnInit->GetPrimitivesBool()[0];

    if (const xiiOpenDdlReaderElement* pCenterWindowOnDisplay = pDesc->FindChildOfType(xiiOpenDdlPrimitiveType::Bool, "CenterWindowOnDisplay"))
      m_bCenterWindowOnDisplay = pCenterWindowOnDisplay->GetPrimitivesBool()[0];
  }
}

xiiResult xiiWindowCreationDescription::LoadFromDDL(xiiStringView sFile)
{
  xiiFileReader file;
  XII_SUCCEED_OR_RETURN(file.Open(sFile));

  xiiOpenDdlReader reader;
  XII_SUCCEED_OR_RETURN(reader.ParseDocument(file));

  LoadFromDDL(reader.GetRootElement());

  return XII_SUCCESS;
}

xiiWindow::xiiWindow()
{
  ++s_uiNextUnusedWindowNumber;
}

xiiWindow::~xiiWindow()
{
  if (m_bInitialized)
  {
    Destroy().IgnoreResult();
  }
  XII_ASSERT_DEV(m_iReferenceCount == 0, "The window is still being referenced, probably by a swapchain. Make sure to destroy all swapchains and call xiiGALDevice::WaitIdle before destroying a window.");
}

#if XII_ENABLED(XII_PLATFORM_WINDOWS)
void xiiWindow::OnWindowMessage(xiiMinWindows::HWND hWnd, xiiMinWindows::UINT msg, xiiMinWindows::WPARAM wparam, xiiMinWindows::LPARAM lparam)
{
  XII_IGNORE_UNUSED(hWnd);
  XII_IGNORE_UNUSED(msg);
  XII_IGNORE_UNUSED(wparam);
  XII_IGNORE_UNUSED(lparam);
}
#endif

xiiUInt8 xiiWindow::GetNextUnusedWindowNumber()
{
  return s_uiNextUnusedWindowNumber;
}

XII_STATICLINK_FILE(Core, Core_System_Implementation_Window);

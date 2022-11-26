#include <Foundation/FoundationInternal.h>
XII_FOUNDATION_INTERNAL_HEADER

#include <GLFW/glfw3.h>

namespace {
  xiiResult xiiGlfwError(const char* file, size_t line)
  {
    const char* desc;
    int errorCode = glfwGetError(&desc);
    if(errorCode != GLFW_NO_ERROR)
    {
      xiiLog::Error("GLFW error {} ({}): {} - {}", file, line, errorCode, desc);
      return XII_FAILURE;
    }
    return XII_SUCCESS;
  }
}

#define XII_GLFW_RETURN_FAILURE_ON_ERROR() do { if(xiiGlfwError(__FILE__, __LINE__).Failed()) return XII_FAILURE; } while(false)

xiiResult xiiScreen::EnumerateScreens(xiiHybridArray<xiiScreenInfo, 2>& out_Screens)
{
  out_Screens.Clear();

  int iMonitorCount = 0;
  GLFWmonitor** pMonitors = glfwGetMonitors(&iMonitorCount);
  XII_GLFW_RETURN_FAILURE_ON_ERROR();
  if(iMonitorCount == 0)
  {
    return XII_FAILURE;
  }

  GLFWmonitor* pPrimaryMonitor = glfwGetPrimaryMonitor();
  XII_GLFW_RETURN_FAILURE_ON_ERROR();
  if(pPrimaryMonitor == nullptr)
  {
    return XII_FAILURE;
  }

  for(int i=0; i < iMonitorCount; ++i)
  {
    xiiScreenInfo& screen = out_Screens.ExpandAndGetRef();
    screen.m_sDisplayName = glfwGetMonitorName(pMonitors[i]);
    XII_GLFW_RETURN_FAILURE_ON_ERROR();

    const GLFWvidmode* mode = glfwGetVideoMode(pMonitors[i]);
    XII_GLFW_RETURN_FAILURE_ON_ERROR();
    if(mode == nullptr)
    {
      return XII_FAILURE;
    }
    screen.m_iResolutionX = mode->width;
    screen.m_iResolutionY = mode->height;

    glfwGetMonitorPos(pMonitors[i], &screen.m_iOffsetX, &screen.m_iOffsetY);
    XII_GLFW_RETURN_FAILURE_ON_ERROR();
    
    screen.m_bIsPrimary = pMonitors[i] == pPrimaryMonitor;
  }

  return XII_SUCCESS;
}
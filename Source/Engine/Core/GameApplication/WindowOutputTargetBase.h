#pragma once

#include <Core/CoreDLL.h>

class xiiImage;

/// \brief Base class for window output targets
///
/// A window output target is usually tied tightly to a window (\sa xiiWindowBase) and represents the
/// graphics APIs side of the render output.
/// E.g. in a DirectX implementation this would be a swapchain.
///
/// This interface provides the high level functionality that is needed by xiiGameApplication to work with
/// the render output.
class XII_CORE_DLL xiiWindowOutputTargetBase
{
public:
  virtual ~xiiWindowOutputTargetBase()                = default;
  virtual void      Present(bool bEnableVSync)        = 0;
  virtual xiiResult CaptureImage(xiiImage& out_image) = 0;
};

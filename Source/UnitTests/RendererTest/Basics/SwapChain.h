#pragma once

#include "../TestClass/TestClass.h"
#include <RendererCore/Textures/Texture2DResource.h>

class xiiRendererTestSwapChain : public xiiGraphicsTest
{
public:
  virtual const char* GetTestName() const override { return "SwapChain"; }

private:
  enum SubTests
  {
    ST_ColorOnly,
    ST_D16,
    ST_D24S8,
    ST_D32,
    ST_NoVSync,
    ST_ResizeWindow,
  };

  virtual void SetupSubTests() override
  {
    AddSubTest("Color Only", SubTests::ST_ColorOnly);
    AddSubTest("Depth D16", SubTests::ST_D16);
    AddSubTest("Depth D24S8", SubTests::ST_D24S8);
    AddSubTest("Depth D32", SubTests::ST_D32);
    AddSubTest("No VSync", SubTests::ST_NoVSync);
    AddSubTest("Resize Window", SubTests::ST_ResizeWindow);
  }

  virtual xiiResult InitializeSubTest(xiiInt32 iIdentifier) override;
  virtual xiiResult DeInitializeSubTest(xiiInt32 iIdentifier) override;

  void          ResizeTest(xiiUInt32 uiInvocationCount);
  xiiTestAppRun BasicRenderLoop(xiiInt32 iIdentifier, xiiUInt32 uiInvocationCount);

  virtual xiiTestAppRun RunSubTest(xiiInt32 iIdentifier, xiiUInt32 uiInvocationCount) override
  {
    ++m_iFrame;

    switch (iIdentifier)
    {
      case SubTests::ST_ResizeWindow:
        ResizeTest(uiInvocationCount);
        [[fallthrough]];
      case SubTests::ST_ColorOnly:
      case SubTests::ST_D16:
      case SubTests::ST_D24S8:
      case SubTests::ST_D32:
      case SubTests::ST_NoVSync:
        return BasicRenderLoop(iIdentifier, uiInvocationCount);
      default:
        XII_ASSERT_NOT_IMPLEMENTED;
        break;
    }
    return xiiTestAppRun::Quit;
  }

  xiiSizeU32 m_CurrentWindowSize = xiiSizeU32(320, 240);
  xiiInt32   m_iFrame;
};

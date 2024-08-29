#include <Foundation/FoundationInternal.h>
XII_FOUNDATION_INTERNAL_HEADER

#include <Foundation/Basics/Platform/uwp/UWPUtils.h>
#include <windows.graphics.display.h>

xiiResult xiiScreen::EnumerateScreens(xiiHybridArray<xiiScreenInfo, 2>& out_Screens)
{
  out_Screens.Clear();

  ComPtr<ABI::Windows::Graphics::Display::IDisplayInformationStatics> displayInformationStatics;
  XII_HRESULT_TO_FAILURE_LOG(ABI::Windows::Foundation::GetActivationFactory(HStringReference(RuntimeClass_Windows_Graphics_Display_DisplayInformation).Get(), &displayInformationStatics));

  // Get information for current screen. Todo: How to get information for secondary screen?
  ComPtr<ABI::Windows::Graphics::Display::IDisplayInformation> currentDisplayInformation;
  XII_HRESULT_TO_FAILURE_LOG(displayInformationStatics->GetForCurrentView(currentDisplayInformation.GetAddressOf()));
  ComPtr<ABI::Windows::Graphics::Display::IDisplayInformation4> currentDisplayInformation4;
  XII_HRESULT_TO_FAILURE(currentDisplayInformation.As(&currentDisplayInformation4));

  xiiScreenInfo& currentScreen = out_Screens.ExpandAndGetRef();
  currentScreen.m_sDisplayName = "Current Display";
  currentScreen.m_iOffsetX     = 0;
  currentScreen.m_iOffsetY     = 0;
  currentScreen.m_iResolutionX = 0;
  currentScreen.m_iResolutionY = 0;
  currentScreen.m_bIsPrimary   = true;

  UINT rawPixelWidth, rawPixelHeight;
  XII_HRESULT_TO_FAILURE(currentDisplayInformation4->get_ScreenHeightInRawPixels(&rawPixelHeight));
  XII_HRESULT_TO_FAILURE(currentDisplayInformation4->get_ScreenWidthInRawPixels(&rawPixelWidth));

  currentScreen.m_iResolutionX = static_cast<xiiInt32>(rawPixelWidth);
  currentScreen.m_iResolutionY = static_cast<xiiInt32>(rawPixelHeight);

  return XII_SUCCESS;
}

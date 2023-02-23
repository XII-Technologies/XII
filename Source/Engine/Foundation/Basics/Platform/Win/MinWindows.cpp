#include <Foundation/FoundationPCH.h>

#if XII_ENABLED(XII_PLATFORM_WINDOWS)
#  include <Foundation/Basics/Platform/Win/IncludeWindows.h>
#  include <Foundation/Basics/Platform/Win/MinWindows.h>
#  include <type_traits>

template <typename xiiType, typename WindowsType, bool mustBeConvertible>
void xiiVerifyWindowsType()
{
  static_assert(sizeof(xiiType) == sizeof(WindowsType), "XII <=> windows.h size mismatch");
  static_assert(alignof(xiiType) == alignof(WindowsType), "XII <=> windows.h alignment mismatch");
  static_assert(std::is_pointer<xiiType>::value == std::is_pointer<WindowsType>::value, "XII <=> windows.h pointer type mismatch");
  static_assert(!mustBeConvertible || xiiConversionTest<xiiType, WindowsType>::exists == 1, "XII <=> windows.h conversion failure");
  static_assert(!mustBeConvertible || xiiConversionTest<WindowsType, xiiType>::exists == 1, "windows.h <=> XII conversion failure");
};

void CALLBACK             WindowsCallbackTest1();
void XII_WINDOWS_CALLBACK WindowsCallbackTest2();
void WINAPI               WindowsWinapiTest1();
void XII_WINDOWS_WINAPI   WindowsWinapiTest2();

// Will never be called and thus removed by the linker
void xiiCheckWindowsTypeSizes()
{
  xiiVerifyWindowsType<xiiMinWindows::DWORD, DWORD, true>();
  xiiVerifyWindowsType<xiiMinWindows::UINT, UINT, true>();
  xiiVerifyWindowsType<xiiMinWindows::BOOL, BOOL, true>();
  xiiVerifyWindowsType<xiiMinWindows::LPARAM, LPARAM, true>();
  xiiVerifyWindowsType<xiiMinWindows::WPARAM, WPARAM, true>();
  xiiVerifyWindowsType<xiiMinWindows::HINSTANCE, HINSTANCE, false>();
  xiiVerifyWindowsType<xiiMinWindows::HMODULE, HMODULE, false>();
  xiiVerifyWindowsType<xiiMinWindows::LPSTR, LPSTR, true>();
  xiiVerifyWindowsType<xiiMinWindows::HWND, HWND, false>();
  xiiVerifyWindowsType<xiiMinWindows::HRESULT, HRESULT, true>();

  static_assert(std::is_same<decltype(&WindowsCallbackTest1), decltype(&WindowsCallbackTest2)>::value, "XII_WINDOWS_CALLBACK does not match CALLBACK");
  static_assert(std::is_same<decltype(&WindowsWinapiTest1), decltype(&WindowsWinapiTest2)>::value, "XII_WINDOWS_WINAPI does not match WINAPI");

  // Clang doesn't allow us to do this check at compile time
#  if XII_DISABLED(XII_COMPILER_CLANG)
  static_assert(XII_WINDOWS_INVALID_HANDLE_VALUE == INVALID_HANDLE_VALUE, "XII_WINDOWS_INVALID_HANDLE_VALUE does not match INVALID_HANDLE_VALUE");
#  endif
}
#endif


XII_STATICLINK_FILE(Foundation, Foundation_Basics_Platform_Win_MinWindows);

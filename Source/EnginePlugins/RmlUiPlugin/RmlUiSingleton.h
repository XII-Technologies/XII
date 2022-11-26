#pragma once

#include <RmlUiPlugin/RmlUiPluginDLL.h>

#include <Foundation/Configuration/Singleton.h>
#include <Foundation/Types/UniquePtr.h>

class xiiRmlUiContext;
struct xiiMsgExtractRenderData;

/// \brief The fmod configuration to be used on a specific platform
struct XII_RMLUIPLUGIN_DLL xiiRmlUiConfiguration
{
  xiiDynamicArray<xiiString> m_Fonts;

  xiiResult Save(const char* szFile) const;
  xiiResult Load(const char* szFile);

  bool operator==(const xiiRmlUiConfiguration& rhs) const;
  bool operator!=(const xiiRmlUiConfiguration& rhs) const { return !operator==(rhs); }
};

class XII_RMLUIPLUGIN_DLL xiiRmlUi
{
  XII_DECLARE_SINGLETON(xiiRmlUi);

public:
  xiiRmlUi();
  ~xiiRmlUi();

  xiiRmlUiContext* CreateContext(const char* szName, const xiiVec2U32& initialSize);
  void             DeleteContext(xiiRmlUiContext* pContext);

  bool AnyContextWantsInput();

  void ExtractContext(xiiRmlUiContext& context, xiiMsgExtractRenderData& msg);

private:
  struct Data;
  xiiUniquePtr<Data> m_pData;
};

#pragma once

#include <RmlUiPlugin/RmlUiPluginDLL.h>

#include <Foundation/Configuration/Singleton.h>
#include <Foundation/Types/UniquePtr.h>

class xiiRmlUiContext;
struct xiiMsgExtractRenderData;

/// \brief The RML configuration to be used on a specific platform
struct XII_RMLUIPLUGIN_DLL xiiRmlUiConfiguration
{
  xiiDynamicArray<xiiString> m_Fonts;

  static constexpr const xiiStringView s_sConfigFile = ":project/RuntimeConfigs/RmlUiConfig.ddl"_xiisv;

  xiiResult Save(xiiStringView sFile = s_sConfigFile) const;
  xiiResult Load(xiiStringView sFile = s_sConfigFile);

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

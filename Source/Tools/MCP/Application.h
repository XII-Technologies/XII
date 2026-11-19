/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Application/Application.h>
#include <MCP/Gateway.h>

#include <cstdio>

class xiiMCPApplication final : public xiiApplication
{
public:
  xiiMCPApplication();
  ~xiiMCPApplication() override;

private:
  xiiResult BeforeCoreSystemsStartup() override;
  void AfterCoreSystemsStartup() override;
  void BeforeCoreSystemsShutdown() override;
  Execution Run() override;

  xiiMCPGatewayConfiguration m_Configuration;
  xiiUniquePtr<xiiMCPGateway> m_pGateway;
  xiiMCPSession m_StdioSession;
  xiiDynamicArray<char> m_Input;
  std::FILE* m_pProtocolOutput = nullptr;
  bool m_bStdio = false;
  bool m_bGui = false;
  bool m_bDiscardLine = false;
};

#ifdef XII_USE_QT
int RunMCPWindow(const xiiMCPGatewayConfiguration& configuration);
#endif

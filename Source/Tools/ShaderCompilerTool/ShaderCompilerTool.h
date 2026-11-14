/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GameEngine/GameApplication/GameApplication.h>
#include <GraphicsFoundation/ShaderCompiler/PermutationGenerator.h>

class xiiShaderCompilerApplication : public xiiGameApplication
{
public:
  using SUPER = xiiGameApplication;

  xiiShaderCompilerApplication();

  xiiApplication::Execution Run() override;

private:
  void      PrintConfig();
  xiiResult CompileShader(xiiStringView sShaderFile);
  xiiResult ExtractPermutationVarValues(xiiStringView sShaderFile);

  xiiResult BeforeCoreSystemsStartup() override;
  void      AfterCoreSystemsStartup() override;
  void      Init_LoadProjectPlugins() override {}
  void      Init_SetupDefaultResources() override {}
  void      Init_ConfigureInput() override {}
  void      Init_ConfigureTags() override {}
  bool      Run_ProcessApplicationInput() override { return true; }

  xiiGALPermutationGenerator                      m_PermutationGenerator;
  xiiString                                       m_sPlatforms;
  xiiString                                       m_sShaderFiles;
  xiiMap<xiiString, xiiHybridArray<xiiString, 4>> m_FixedPermVars;
};

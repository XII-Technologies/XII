#pragma once

#include <GameEngine/GameApplication/GameApplication.h>
#include <GraphicsCore/ShaderCompiler/PermutationGenerator.h>

class xiiShaderCompilerApplication : public xiiGameApplication
{
public:
  using SUPER = xiiGameApplication;

  xiiShaderCompilerApplication();

  virtual xiiApplication::Execution Run() override;

private:
  void      PrintConfig();
  xiiResult CompileShader(xiiStringView sShaderFile);
  xiiResult ExtractPermutationVarValues(xiiStringView sShaderFile);

  virtual xiiResult BeforeCoreSystemsStartup() override;
  virtual void      AfterCoreSystemsStartup() override;
  virtual void      Init_LoadProjectPlugins() override {}
  virtual void      Init_SetupDefaultResources() override {}
  virtual void      Init_ConfigureInput() override {}
  virtual void      Init_ConfigureTags() override {}
  virtual bool      Run_ProcessApplicationInput() override { return true; }

  xiiPermutationGenerator                         m_PermutationGenerator;
  xiiString                                       m_sPlatforms;
  xiiString                                       m_sShaderFiles;
  xiiMap<xiiString, xiiHybridArray<xiiString, 4>> m_FixedPermVars;
};

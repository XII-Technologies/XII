/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsFoundation/GraphicsFoundationPCH.h>

#include <Foundation/Configuration/Startup.h>
#include <GraphicsFoundation/ShaderCompiler/ShaderManager.h>
#include <GraphicsFoundation/ShaderCompiler/ShaderParser.h>
#include <GraphicsFoundation/ShaderCompiler/ShaderStageBinary.h>

// clang-format off
XII_BEGIN_SUBSYSTEM_DECLARATION(GraphicsFoundation, ShaderCompiler)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "Foundation"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    xiiGALShaderManager::Startup();
    xiiGALShaderParser::OnEngineStartup();
    xiiGALShaderStageBinary::OnEngineStartup();
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    xiiGALShaderStageBinary::OnEngineShutdown();
    xiiGALShaderParser::OnEngineShutdown();
    xiiGALShaderManager::Shutdown();
  }

  ON_HIGHLEVELSYSTEMS_STARTUP
  {
  }

  ON_HIGHLEVELSYSTEMS_SHUTDOWN
  {
  }

XII_END_SUBSYSTEM_DECLARATION;
// clang-format on

XII_STATICLINK_FILE(GraphicsFoundation, GraphicsFoundation_Startup);

/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Configuration/Singleton.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Time/Timestamp.h>

struct xiiPhantomRttiManagerEvent;

class xiiShaderTypeRegistry
{
  XII_DECLARE_SINGLETON(xiiShaderTypeRegistry);

public:
  xiiShaderTypeRegistry();
  ~xiiShaderTypeRegistry();

  const xiiRTTI* GetShaderType(xiiStringView sShaderPath);
  const xiiRTTI* GetShaderBaseType() const { return m_pBaseType; }

private:
  XII_MAKE_SUBSYSTEM_STARTUP_FRIEND(EditorFramework, ShaderTypeRegistry);

  struct ShaderData
  {
    ShaderData() = default;

    xiiString      m_sShaderPath;
    xiiString      m_sAbsShaderPath;
    xiiTimestamp   m_fileModifiedTime;
    const xiiRTTI* m_pType = nullptr;
  };
  void UpdateShaderType(ShaderData& data);
  void PhantomTypeRegistryEventHandler(const xiiPhantomRttiManagerEvent& e);

  xiiMap<xiiString, ShaderData> m_ShaderTypes;
  const xiiRTTI*                m_pBaseType;
};

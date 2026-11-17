/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Containers/HybridArray.h>
#include <Foundation/Strings/String.h>

struct xiiRemoteShaderPermutation
{
  xiiString m_sName;
  xiiString m_sValue;
};

struct xiiRemoteShaderCompileRequest
{
  xiiString                                      m_sShader;
  xiiString                                      m_sPlatform;
  xiiHybridArray<xiiRemoteShaderPermutation, 16> m_Permutations;
};

/// Remote shader compilation capability used by the rendering backend.
class XII_FOUNDATION_DLL xiiRemoteToolingInterface
{
public:
  virtual bool      IsShaderCompilationAvailable() const                                                    = 0;
  virtual xiiResult CompileShader(const xiiRemoteShaderCompileRequest& request, xiiStringBuilder& out_sLog) = 0;

protected:
  xiiRemoteToolingInterface();
  ~xiiRemoteToolingInterface();
};

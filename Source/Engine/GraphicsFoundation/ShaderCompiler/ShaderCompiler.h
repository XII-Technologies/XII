/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GraphicsFoundation/GraphicsFoundationDLL.h>

#include <Foundation/CodeUtils/Preprocessor.h>

#include <GraphicsFoundation/ShaderCompiler/Descriptors.h>

class xiiRemoteMessage;

/// Shader compiler interface.
/// Custom shader compilers need to derive from this class and implement the pure virtual interface functions.
/// Instances are created via reflection, so each implementation must be properly reflected.
class XII_GRAPHICSFOUNDATION_DLL xiiGALShaderProgramCompiler : public xiiReflectedClass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiGALShaderProgramCompiler, xiiReflectedClass);

public:
  /// Returns the platforms that this shader compiler supports.
  /// \param out_platforms Filled with the platforms this compiler supports.
  virtual void GetSupportedPlatforms(xiiHybridArray<xiiString, 4>& out_platforms) = 0;

  /// Allows the shader compiler to modify the shader source before hashing and compiling. This allows it to implement custom features by injecting code before the compile process. Mostly used to define resource bindings that do not cause conflicts across shader stages.
  /// \param inout_data The state of the shader compiler. Only m_sShaderSource should be modified by the implementation.
  /// \param pLog Logging interface to be used when outputting any errors.
  /// \return Returns whether the shader could be modified. On failure, the shader won't be compiled.
  virtual xiiResult ModifyShaderSource(xiiGALShaderProgramData& inout_data, xiiLogInterface* pLog) = 0;

  /// Compiles the shader comprised of multiple stages defined in inout_data.
  /// \param inout_data The state of the shader compiler. m_Resources and m_ByteCode should be written to on successful return code.
  /// \param pLog Logging interface to be used when outputting any errors.
  /// \return Returns whether the shader was compiled successfully. On failure, errors should be written to pLog.
  virtual xiiResult Compile(xiiGALShaderProgramData& inout_data, xiiLogInterface* pLog) = 0;
};

class XII_GRAPHICSFOUNDATION_DLL xiiGALShaderCompiler
{
public:
  xiiResult CompileShaderPermutationForPlatforms(xiiStringView sFile, const xiiArrayPtr<const xiiGALPermutationVariable>& permutationVariables, xiiLogInterface* pLog, xiiStringView sPlatform = "ALL", xiiTokenizedFileCache* pFileCache = nullptr);

private:
  xiiResult RunShaderCompiler(xiiStringView sFile, xiiStringView sPlatform, xiiGALShaderProgramCompiler* pCompiler, xiiLogInterface* pLog, xiiTokenizedFileCache* pFileCache = nullptr);

  void WriteFailedShaderSource(xiiGALShaderProgramData& spd, xiiLogInterface* pLog);

  bool PassThroughUnknownCommandCB(xiiStringView sCmd) { return sCmd == "version"; }

  void ShaderCompileMsg(xiiRemoteMessage& msg);

  xiiResult FileOpen(xiiStringView sAbsoluteFile, xiiDynamicArray<xiiUInt8>& fileContent, xiiTimestamp& out_fileModification);

  struct ShaderData
  {
    xiiString                                     m_sPlatform;
    xiiHybridArray<xiiGALPermutationVariable, 16> m_Permutations;
    xiiHybridArray<xiiGALPermutationVariable, 16> m_FixedPermutationVariables;
    xiiString                                     m_StateSource;
    xiiMap<xiiGALShaderType::Enum, xiiString>     m_ShaderStageSource;
  };

  xiiMap<xiiGALShaderType::Enum, xiiStringBuilder> m_StageSourceFile;

  xiiTokenizedFileCache m_FileCache;
  ShaderData            m_ShaderData;

  xiiSet<xiiString> m_IncludeFiles;

  bool      m_bCompileShaderRemotely    = false;
  xiiResult m_RemoteShaderCompileResult = XII_FAILURE;
};

#pragma once

#include <Foundation/CodeUtils/Preprocessor.h>
#include <Foundation/Containers/Set.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Reflection/Reflection.h>
#include <GraphicsCore/ShaderCompiler/Declarations.h>
#include <GraphicsCore/ShaderCompiler/ShaderParser.h>
#include <GraphicsCore/Shader/Implementation/Helper.h>
#include <GraphicsCore/Shader/ShaderPermutationBinary.h>
#include <GraphicsCore/ShaderCompiler/PermutationGenerator.h>

/// \brief Shader compiler interface.
/// Custom shader compiles need to derive from this class and implement the pure virtual interface functions. Instances are created via reflection so each implementation must be properly reflected.
class XII_GRAPHICSCORE_DLL xiiShaderProgramCompiler : public xiiReflectedClass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiShaderProgramCompiler, xiiReflectedClass);

public:
  /// \brief Returns the platforms that this shader compiler supports.
  /// \param out_platforms Filled with the platforms this compiler supports.
  virtual void GetSupportedPlatforms(xiiHybridArray<xiiString, 4>& out_platforms) = 0;

  /// Allows the shader compiler to modify the shader source before hashing and compiling. This allows it to implement custom features by injecting code before the compile process. Mostly used to define resource bindings that do not cause conflicts across shader stages.
  /// \param inout_data The state of the shader compiler. Only m_sShaderSource should be modified by the implementation.
  /// \param pLog Logging interface to be used when outputting any errors.
  /// \return Returns whether the shader could be modified. On failure, the shader won't be compiled.
  virtual xiiResult ModifyShaderSource(xiiShaderProgramData& inout_data, xiiLogInterface* pLog) = 0;

  /// Compiles the shader comprised of multiple stages defined in inout_data.
  /// \param inout_data The state of the shader compiler. m_Resources and m_ByteCode should be written to on successful return code.
  /// \param pLog Logging interface to be used when outputting any errors.
  /// \return Returns whether the shader was compiled successfully. On failure, errors should be written to pLog.
  virtual xiiResult Compile(xiiShaderProgramData& inout_data, xiiLogInterface* pLog) = 0;
};

class XII_GRAPHICSCORE_DLL xiiShaderCompiler
{
public:
  xiiResult CompileShaderPermutationForPlatforms(xiiStringView sFile, const xiiArrayPtr<const xiiPermutationVar>& permutationVars, xiiLogInterface* pLog, xiiStringView sPlatform = "ALL");

private:
  xiiResult RunShaderCompiler(xiiStringView sFile, xiiStringView sPlatform, xiiShaderProgramCompiler* pCompiler, xiiLogInterface* pLog);

  void WriteFailedShaderSource(xiiShaderProgramData& spd, xiiLogInterface* pLog);

  bool PassThroughUnknownCommandCB(xiiStringView sCmd) { return sCmd == "version"; }

  struct xiiShaderData
  {
    xiiString                             m_Platforms;
    xiiHybridArray<xiiPermutationVar, 16> m_Permutations;
    xiiHybridArray<xiiPermutationVar, 16> m_FixedPermVars;
    xiiString                             m_StateSource;
    xiiString                             m_ShaderStageSource[xiiGALShaderStage::ENUM_COUNT];
  };

  xiiResult FileOpen(xiiStringView sAbsoluteFile, xiiDynamicArray<xiiUInt8>& FileContent, xiiTimestamp& out_FileModification);

  xiiStringBuilder m_StageSourceFile[xiiGALShaderStage::ENUM_COUNT];

  xiiTokenizedFileCache m_FileCache;
  xiiShaderData         m_ShaderData;

  xiiSet<xiiString> m_IncludeFiles;
};

#pragma once

#include <GraphicsFoundation/GraphicsFoundationDLL.h>

#include <Foundation/Reflection/Reflection.h>

/// \brief Shader compiler interface.
/// Custom shader compilers need to derive from this class and implement the pure virtual interface functions.
/// Instances are created via reflection, so each implementation must be properly reflected.
class XII_GRAPHICSFOUNDATION_DLL xiiShaderProgramCompiler : public xiiReflectedClass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiShaderProgramCompiler, xiiReflectedClass);

public:
  /// \brief Returns the platforms that this shader compiler supports.
  /// \param out_platforms Filled with the platforms this compiler supports.
  virtual void GetSupportedPlatforms(xiiHybridArray<xiiString, 4>& out_platforms) = 0;
};

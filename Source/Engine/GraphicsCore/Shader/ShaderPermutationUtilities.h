#include <GraphicsCore/GraphicsCoreDLL.h>

#include <GraphicsCore/Shader/ShaderPermutationResource.h>
#include <GraphicsCore/Shader/ShaderResource.h>
#include <GraphicsFoundation/ShaderCompiler/Descriptors.h>

class XII_GRAPHICSCORE_DLL xiiGALShaderPermutationUtilities
{
public:
  static void                               PreloadPermutations(xiiShaderResourceHandle hShader, const xiiHashTable<xiiHashedString, xiiHashedString>& permVars, xiiTime shouldBeAvailableIn);
  static xiiShaderPermutationResourceHandle PreloadSinglePermutation(xiiShaderResourceHandle hShader, const xiiHashTable<xiiHashedString, xiiHashedString>& permVars, bool bAllowFallback);

private:
  static xiiShaderPermutationResourceHandle PreloadSinglePermutationInternal(xiiStringView sResourceId, xiiUInt64 uiResourceIdHash, xiiUInt32 uiPermutationHash, xiiArrayPtr<xiiGALPermutationVariable> filteredPermutationVariables);
};

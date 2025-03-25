#include <GraphicsFoundation/GraphicsFoundationPCH.h>

#include <GraphicsFoundation/ShaderCompiler/ShaderPermutationBinary.h>

struct xiiGALShaderPermutationBinaryVersion
{
  using StorageType = xiiUInt8;

  enum Enum : StorageType
  {
    Version1 = 1,

    // Increase this version number to trigger shader recompilation

    ENUM_COUNT,
    Current = ENUM_COUNT - 1
  };
};

xiiGALShaderPermutationBinary::xiiGALShaderPermutationBinary() = default;

xiiResult xiiGALShaderPermutationBinary::Write(xiiStreamWriter& inout_stream)
{
  // write this at the beginning so that the file can be read as a xiiDependencyFile.
  m_DependencyFile.StoreCurrentTimeStamp();
  XII_SUCCEED_OR_RETURN(m_DependencyFile.WriteDependencyFile(inout_stream));

  const xiiUInt8 uiVersion = xiiGALShaderPermutationBinaryVersion::Current;
  if (inout_stream.WriteBytes(&uiVersion, sizeof(xiiUInt8)).Failed())
    return XII_FAILURE;

  const xiiUInt32 uiShaderStageHashCount = m_ShaderStageHashes.GetCount();
  if (inout_stream.WriteDWordValue(&uiShaderStageHashCount).Failed())
    return XII_FAILURE;

  for (auto it : m_ShaderStageHashes)
  {
    if (inout_stream.WriteDWordValue(&it.Key()).Failed())
      return XII_FAILURE;

    if (inout_stream.WriteDWordValue(&it.Value()).Failed())
      return XII_FAILURE;
  }

  m_StateDescriptor.Save(inout_stream);

  const xiiUInt32 uiPermutationVariableCount = m_ShaderStageHashes.GetCount();
  if (inout_stream.WriteDWordValue(&uiPermutationVariableCount).Failed())
    return XII_FAILURE;

  for (const auto& var : m_PermutationVariables)
  {
    if (inout_stream.WriteString(var.m_sName).Failed())
      return XII_FAILURE;

    if (inout_stream.WriteString(var.m_sValue).Failed())
      return XII_FAILURE;
  }

  return XII_SUCCESS;
}

xiiResult xiiGALShaderPermutationBinary::Read(xiiStreamReader& inout_stream, bool& out_bOldVersion)
{
  XII_SUCCEED_OR_RETURN(m_DependencyFile.ReadDependencyFile(inout_stream));

  xiiUInt8 uiVersion = 0;
  if (inout_stream.ReadBytes(&uiVersion, sizeof(xiiUInt8)) != sizeof(xiiUInt8))
    return XII_FAILURE;

  XII_ASSERT_DEV(uiVersion <= xiiGALShaderPermutationBinaryVersion::Current, "Invalid Version {0}.", uiVersion);

  out_bOldVersion = uiVersion != xiiGALShaderPermutationBinaryVersion::Current;

  xiiUInt32 uiShaderStageHashCount = 0;
  if (inout_stream.ReadDWordValue(&uiShaderStageHashCount).Failed())
    return XII_FAILURE;

  for (xiiUInt32 i = 0; i < uiShaderStageHashCount; ++i)
  {
    xiiUInt32 uiStage = 0;
    if (inout_stream.ReadDWordValue(&uiStage).Failed())
      return XII_FAILURE;

    xiiUInt32 uiHash = 0;
    if (inout_stream.ReadDWordValue(&uiHash).Failed())
      return XII_FAILURE;

    m_ShaderStageHashes.Insert((xiiGALShaderType::Enum)uiStage, uiHash);
  }

  m_StateDescriptor.Load(inout_stream);

  {
    xiiUInt32 uiPermutationCount = 0U;
    if (inout_stream.ReadDWordValue(&uiPermutationCount).Failed())
      return XII_FAILURE;

    m_PermutationVariables.SetCount(uiPermutationCount);

    xiiStringBuilder tmp;
    for (xiiUInt32 i = 0; i < uiPermutationCount; ++i)
    {
      auto& var = m_PermutationVariables[i];

      if (inout_stream.ReadString(tmp).Failed())
        return XII_FAILURE;

      var.m_sName.Assign(tmp.GetView());

      if (inout_stream.ReadString(tmp).Failed())
        return XII_FAILURE;

      var.m_sValue.Assign(tmp.GetView());
    }
  }

  return XII_SUCCESS;
}

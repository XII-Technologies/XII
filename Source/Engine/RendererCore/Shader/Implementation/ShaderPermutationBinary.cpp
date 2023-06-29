#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/Shader/ShaderPermutationBinary.h>

struct xiiShaderPermutationBinaryVersion
{
  enum Enum : xiiUInt32
  {
    Version1 = 1,
    Version2 = 2,
    Version3 = 3,
    Version4 = 4,
    Version5 = 5,

    // Increase this version number to trigger shader recompilation

    ENUM_COUNT,
    Current = ENUM_COUNT - 1
  };
};

xiiShaderPermutationBinary::xiiShaderPermutationBinary()
{
  for (xiiUInt32 stage = 0; stage < xiiGALShaderStage::ENUM_COUNT; ++stage)
    m_uiShaderStageHashes[stage] = 0;
}

xiiResult xiiShaderPermutationBinary::Write(xiiStreamWriter& ref_stream)
{
  // write this at the beginning so that the file can be read as a xiiDependencyFile
  m_DependencyFile.StoreCurrentTimeStamp();
  XII_SUCCEED_OR_RETURN(m_DependencyFile.WriteDependencyFile(ref_stream));

  const xiiUInt8 uiVersion = xiiShaderPermutationBinaryVersion::Current;

  if (ref_stream.WriteBytes(&uiVersion, sizeof(xiiUInt8)).Failed())
    return XII_FAILURE;

  for (xiiUInt32 stage = 0; stage < xiiGALShaderStage::ENUM_COUNT; ++stage)
  {
    if (ref_stream.WriteDWordValue(&m_uiShaderStageHashes[stage]).Failed())
      return XII_FAILURE;
  }

  m_StateDescriptor.Save(ref_stream);

  ref_stream << m_PermutationVars.GetCount();

  for (auto& var : m_PermutationVars)
  {
    ref_stream << var.m_sName.GetString();
    ref_stream << var.m_sValue.GetString();
  }

  return XII_SUCCESS;
}

xiiResult xiiShaderPermutationBinary::Read(xiiStreamReader& ref_stream, bool& out_bOldVersion)
{
  XII_SUCCEED_OR_RETURN(m_DependencyFile.ReadDependencyFile(ref_stream));

  xiiUInt8 uiVersion = 0;

  if (ref_stream.ReadBytes(&uiVersion, sizeof(xiiUInt8)) != sizeof(xiiUInt8))
    return XII_FAILURE;

  XII_ASSERT_DEV(uiVersion <= xiiShaderPermutationBinaryVersion::Current, "Wrong Version {0}", uiVersion);

  out_bOldVersion = uiVersion != xiiShaderPermutationBinaryVersion::Current;

  for (xiiUInt32 stage = 0; stage < xiiGALShaderStage::ENUM_COUNT; ++stage)
  {
    if (ref_stream.ReadDWordValue(&m_uiShaderStageHashes[stage]).Failed())
      return XII_FAILURE;
  }

  m_StateDescriptor.Load(ref_stream);

  if (uiVersion >= xiiShaderPermutationBinaryVersion::Version2)
  {
    xiiUInt32 uiPermutationCount;
    ref_stream >> uiPermutationCount;

    m_PermutationVars.SetCount(uiPermutationCount);

    xiiStringBuilder tmp;
    for (xiiUInt32 i = 0; i < uiPermutationCount; ++i)
    {
      auto& var = m_PermutationVars[i];

      ref_stream >> tmp;
      var.m_sName.Assign(tmp.GetData());
      ref_stream >> tmp;
      var.m_sValue.Assign(tmp.GetData());
    }
  }

  return XII_SUCCESS;
}

XII_STATICLINK_FILE(RendererCore, RendererCore_Shader_Implementation_ShaderPermutationBinary);

#include <GraphicsCore/GraphicsCorePCH.h>

#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <GraphicsCore/Shader/ShaderStageBinary.h>
#include <GraphicsCore/ShaderCompiler/ShaderManager.h>
#include <GraphicsFoundation/Shader/Types.h>

xiiMap<xiiUInt32, xiiShaderStageBinary> xiiShaderStageBinary::s_ShaderStageBinaries[xiiGALShaderType::ENUM_COUNT];

xiiShaderStageBinary::xiiShaderStageBinary() = default;

xiiShaderStageBinary::~xiiShaderStageBinary()
{
  m_pGALByteCode = nullptr;
}

xiiResult xiiShaderStageBinary::Write(xiiStreamWriter& inout_stream) const
{
  const xiiUInt8 uiVersion = xiiShaderStageBinary::VersionCurrent;

  // xiiShaderStageBinary
  inout_stream << uiVersion;
  inout_stream << m_uiSourceHash;

  // xiiGALShaderByteCode
  inout_stream << m_pGALByteCode->m_ShaderStage;
  inout_stream << m_pGALByteCode->m_bWasCompiledWithDebug;

  // m_ByteCode
  const xiiUInt32 uiByteCodeSize = m_pGALByteCode->m_ByteCode.GetCount();
  inout_stream << uiByteCodeSize;

  if (!m_pGALByteCode->m_ByteCode.IsEmpty() && inout_stream.WriteBytes(&m_pGALByteCode->m_ByteCode[0], uiByteCodeSize).Failed())
    return XII_FAILURE;

  // m_ShaderResourceBindings
  xiiUInt32 uiResourceCount = m_pGALByteCode->m_ShaderResourceBindings.GetCount();
  inout_stream << uiResourceCount;

  for (const auto& resource : m_pGALByteCode->m_ShaderResourceBindings)
  {
    inout_stream << resource.m_Type;
    inout_stream << resource.m_TextureType;
    inout_stream << resource.m_uiArraySize;
    inout_stream << resource.m_uiBindIndex;
    inout_stream << resource.m_uiDescriptorSet;
    inout_stream << resource.m_ShaderStages;
    inout_stream << resource.m_uiTotalSize;
    inout_stream << resource.m_sName.GetData();

    const bool bHasMembers = !resource.m_Variables.IsEmpty();
    inout_stream << bHasMembers;

    if (bHasMembers)
    {
      XII_SUCCEED_OR_RETURN(Write(inout_stream, resource.m_Variables));
    }
  }

  // m_VertexInputLayout
  const xiiUInt32 uiVertexInputLayoutCount = m_pGALByteCode->m_VertexInputLayout.GetCount();
  inout_stream << uiVertexInputLayoutCount;

  for (const auto& input : m_pGALByteCode->m_VertexInputLayout)
  {
    inout_stream << input.m_Semantic;
    inout_stream << input.m_uiSemanticIndex;
    inout_stream << input.m_Format;
  }

  return XII_SUCCESS;
}

xiiResult xiiShaderStageBinary::Read(xiiStreamReader& inout_stream)
{
  XII_ASSERT_DEBUG(m_pGALByteCode == nullptr, "Expected an empty bytecode reference.");
  m_pGALByteCode = XII_DEFAULT_NEW(xiiGALShaderByteCode);

  xiiUInt8 uiVersion = 0;

  if (inout_stream.ReadBytes(&uiVersion, sizeof(xiiUInt8)) != sizeof(xiiUInt8))
    return XII_FAILURE;

  if (uiVersion < xiiShaderStageBinary::VersionCurrent)
  {
    xiiLog::Error("Unsupported shader binary, please recompile the shader.");
    return XII_FAILURE;
  }

  inout_stream >> m_uiSourceHash;

  // xiiGALShaderByteCode
  inout_stream >> m_pGALByteCode->m_ShaderStage;
  inout_stream >> m_pGALByteCode->m_bWasCompiledWithDebug;

  // m_ByteCode
  {
    xiiUInt32 uiByteCodeSize = 0U;
    inout_stream >> uiByteCodeSize;

    m_pGALByteCode->m_ByteCode.SetCountUninitialized(uiByteCodeSize);
    if (!m_pGALByteCode->m_ByteCode.IsEmpty() && inout_stream.ReadBytes(&m_pGALByteCode->m_ByteCode[0], uiByteCodeSize) != uiByteCodeSize)
      return XII_FAILURE;
  }

  // m_ShaderResourceBinding
  {
    xiiUInt32 uiResourceCount = 0U;
    inout_stream >> uiResourceCount;

    m_pGALByteCode->m_ShaderResourceBindings.SetCount(uiResourceCount);

    xiiString sTemp;

    for (auto& resource : m_pGALByteCode->m_ShaderResourceBindings)
    {
      inout_stream >> resource.m_Type;
      inout_stream >> resource.m_TextureType;
      inout_stream >> resource.m_uiArraySize;
      inout_stream >> resource.m_uiBindIndex;
      inout_stream >> resource.m_uiDescriptorSet;
      inout_stream >> resource.m_ShaderStages;
      inout_stream >> resource.m_uiTotalSize;
      inout_stream >> sTemp;

      resource.m_sName.Assign(sTemp.GetData());

      bool bHasMembers = false;
      inout_stream >> bHasMembers;

      if (bHasMembers)
      {
        XII_SUCCEED_OR_RETURN(Read(inout_stream, resource.m_Variables));
      }
    }
  }

  // m_VertexInputLayout
  {
    xiiUInt32 uiVertexInputLayoutCount = 0;
    inout_stream >> uiVertexInputLayoutCount;
    m_pGALByteCode->m_VertexInputLayout.SetCount(uiVertexInputLayoutCount);

    for (auto& input : m_pGALByteCode->m_VertexInputLayout)
    {
      inout_stream >> input.m_Semantic;
      inout_stream >> input.m_uiSemanticIndex;
      inout_stream >> input.m_Format;
    }
  }

  return XII_SUCCESS;
}

xiiResult xiiShaderStageBinary::Write(xiiStreamWriter& inout_stream, const xiiDynamicArray<xiiGALShaderVariableDescription>& layout) const
{
  const xiiUInt32 uiResourceVariableCount = layout.GetCount();
  inout_stream << uiResourceVariableCount;

  if (!layout.IsEmpty())
  {
    for (xiiUInt32 i = 0; i < uiResourceVariableCount; ++i)
    {
      const xiiGALShaderVariableDescription& variableDescription = layout[i];

      inout_stream << variableDescription.m_Class;
      inout_stream << variableDescription.m_PrimitiveType;
      inout_stream << variableDescription.m_uiRowCount;
      inout_stream << variableDescription.m_uiColumnCount;
      inout_stream << variableDescription.m_uiOffset;
      inout_stream << variableDescription.m_uiArraySize;
      inout_stream << variableDescription.m_sName.GetData();

      if (Write(inout_stream, variableDescription.m_Members).Failed())
        return XII_FAILURE;
    }
  }
  return XII_SUCCESS;
}

xiiResult xiiShaderStageBinary::Read(xiiStreamReader& inout_stream, xiiDynamicArray<xiiGALShaderVariableDescription>& out_layout)
{
  xiiUInt32 uiResourceVariableCount = 0U;
  inout_stream >> uiResourceVariableCount;

  if (uiResourceVariableCount > 0)
  {
    out_layout.SetCount(uiResourceVariableCount);

    xiiString sTemp;

    for (xiiUInt32 i = 0; i < uiResourceVariableCount; ++i)
    {
      xiiGALShaderVariableDescription& variableDescription = out_layout[i];

      inout_stream >> variableDescription.m_Class;
      inout_stream >> variableDescription.m_PrimitiveType;
      inout_stream >> variableDescription.m_uiRowCount;
      inout_stream >> variableDescription.m_uiColumnCount;
      inout_stream >> variableDescription.m_uiOffset;
      inout_stream >> variableDescription.m_uiArraySize;
      inout_stream >> sTemp;

      variableDescription.m_sName.Assign(sTemp.GetData());

      if (Read(inout_stream, variableDescription.m_Members).Failed())
        return XII_FAILURE;
    }
  }
  return XII_SUCCESS;
}

xiiSharedPtr<const xiiGALShaderByteCode> xiiShaderStageBinary::GetByteCode() const
{
  return m_pGALByteCode;
}

xiiResult xiiShaderStageBinary::WriteStageBinary(xiiLogInterface* pLog, xiiStringView sPlatform) const
{
  xiiStringBuilder sShaderStageFile = xiiShaderManager::GetCacheDirectory();

  sShaderStageFile.AppendPath(sPlatform);
  sShaderStageFile.AppendFormat("/{0}_{1}.xiiShaderStage", xiiGALShaderType::Names[xiiGALShaderType::GetStageIndex((xiiGALShaderType::Enum)m_pGALByteCode->m_ShaderStage.GetValue())], xiiArgU(m_uiSourceHash, 8, true, 16, true));

  xiiFileWriter StageFileOut;
  if (StageFileOut.Open(sShaderStageFile).Failed())
  {
    xiiLog::Error(pLog, "Could not open shader stage file '{0}' for writing", sShaderStageFile);
    return XII_FAILURE;
  }

  if (Write(StageFileOut).Failed())
  {
    xiiLog::Error(pLog, "Could not write shader stage file '{0}'", sShaderStageFile);
    return XII_FAILURE;
  }

  return XII_SUCCESS;
}

// static
xiiShaderStageBinary* xiiShaderStageBinary::LoadStageBinary(xiiGALShaderType::Enum Stage, xiiUInt32 uiHash, xiiStringView sPlatform)
{
  auto itStage = s_ShaderStageBinaries[xiiGALShaderType::GetStageIndex(Stage)].Find(uiHash);

  if (!itStage.IsValid())
  {
    xiiStringBuilder sShaderStageFile = xiiShaderManager::GetCacheDirectory();

    sShaderStageFile.AppendPath(sPlatform);
    sShaderStageFile.AppendFormat("/{0}_{1}.xiiShaderStage", xiiGALShaderType::Names[xiiGALShaderType::GetStageIndex(Stage)], xiiArgU(uiHash, 8, true, 16, true));

    xiiFileReader StageFileIn;
    if (StageFileIn.Open(sShaderStageFile.GetData()).Failed())
    {
      xiiLog::Debug("Could not open shader stage file '{0}' for reading", sShaderStageFile);
      return nullptr;
    }

    xiiShaderStageBinary shaderStageBinary;
    if (shaderStageBinary.Read(StageFileIn).Failed())
    {
      xiiLog::Error("Could not read shader stage file '{0}'", sShaderStageFile);
      return nullptr;
    }

    itStage = xiiShaderStageBinary::s_ShaderStageBinaries[xiiGALShaderType::GetStageIndex(Stage)].Insert(uiHash, shaderStageBinary);
  }

  xiiShaderStageBinary* pShaderStageBinary = &itStage.Value();

  return pShaderStageBinary;
}

// static
void xiiShaderStageBinary::OnEngineShutdown()
{
  for (xiiUInt32 stage = 0; stage < xiiGALShaderType::ENUM_COUNT; ++stage)
  {
    s_ShaderStageBinaries[stage].Clear();
  }
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Shader_Implementation_ShaderStageBinary);

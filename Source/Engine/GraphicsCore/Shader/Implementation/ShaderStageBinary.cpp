#include <GraphicsCore/GraphicsCorePCH.h>

#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <GraphicsCore/Shader/ShaderStageBinary.h>
#include <GraphicsCore/ShaderCompiler/ShaderManager.h>
#include <GraphicsFoundation/Shader/Types.h>

xiiMap<xiiUInt32, xiiShaderStageBinary> xiiShaderStageBinary::s_ShaderStageBinaries[xiiGALShaderStage::ENUM_COUNT];

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
  xiiUInt8 uiVersion = 0;

  if (inout_stream.ReadBytes(&uiVersion, sizeof(xiiUInt8)) != sizeof(xiiUInt8))
    return XII_FAILURE;

  XII_ASSERT_DEV(uiVersion <= xiiShaderStageBinary::VersionCurrent, "Wrong Version {0}", uiVersion);

  if (inout_stream.ReadDWordValue(&m_uiSourceHash).Failed())
    return XII_FAILURE;

  xiiUInt8 uiStage = xiiGALShaderStage::ENUM_COUNT;

  if (inout_stream.ReadBytes(&uiStage, sizeof(xiiUInt8)) != sizeof(xiiUInt8))
    return XII_FAILURE;

  m_Stage = xiiGALShaderStage::GetStageFlag(uiStage);

  xiiUInt32 uiByteCodeSize = 0;

  if (inout_stream.ReadDWordValue(&uiByteCodeSize).Failed())
    return XII_FAILURE;

  m_ByteCode.SetCountUninitialized(uiByteCodeSize);

  if (!m_ByteCode.IsEmpty() && inout_stream.ReadBytes(&m_ByteCode[0], uiByteCodeSize) != uiByteCodeSize)
    return XII_FAILURE;

  {
    xiiUInt16 uiResources = 0;
    inout_stream >> uiResources;

    m_ShaderResourceBindings.SetCount(uiResources);

    xiiString sTemp;

    for (auto& r : m_ShaderResourceBindings)
    {
      inout_stream >> sTemp;
      r.m_sName.Assign(sTemp.GetData());
      inout_stream >> r.m_iSlot;

      xiiUInt8 uiType = 0;
      inout_stream >> uiType;
      r.m_Type = (xiiGALShaderResourceType::Enum)uiType;

      if (r.m_Type == xiiGALShaderResourceType::ConstantBuffer)
      {
        auto pLayout = XII_DEFAULT_NEW(xiiShaderConstantBufferLayout);
        XII_SUCCEED_OR_RETURN(pLayout->Read(inout_stream));

        r.m_pLayout = pLayout;
      }
    }
  }

  {
    inout_stream >> m_bWasCompiledWithDebug;
  }

  return XII_SUCCESS;
}

xiiResult xiiShaderStageBinary::Write(xiiStreamWriter& inout_stream, const xiiDynamicArray<xiiGALShaderVariableDescription>& layout) const
{
  return xiiResult();
}

xiiResult xiiShaderStageBinary::Read(xiiStreamReader& inout_stream, xiiDynamicArray<xiiGALShaderVariableDescription>& out_layout)
{
  return xiiResult();
}


xiiDynamicArray<xiiUInt8>& xiiShaderStageBinary::GetByteCode()
{
  return m_ByteCode;
}

void xiiShaderStageBinary::AddShaderResourceBinding(const xiiShaderResourceBinding& binding)
{
  m_ShaderResourceBindings.PushBack(binding);
}


xiiArrayPtr<const xiiShaderResourceBinding> xiiShaderStageBinary::GetShaderResourceBindings() const
{
  return m_ShaderResourceBindings;
}

const xiiShaderResourceBinding* xiiShaderStageBinary::GetShaderResourceBinding(const xiiTempHashedString& sName) const
{
  for (auto& binding : m_ShaderResourceBindings)
  {
    if (binding.m_sName == sName)
    {
      return &binding;
    }
  }

  return nullptr;
}

xiiShaderConstantBufferLayout* xiiShaderStageBinary::CreateConstantBufferLayout() const
{
  return XII_DEFAULT_NEW(xiiShaderConstantBufferLayout);
}

xiiResult xiiShaderStageBinary::WriteStageBinary(xiiLogInterface* pLog) const
{
  xiiStringBuilder sShaderStageFile = xiiShaderManager::GetCacheDirectory();

  sShaderStageFile.AppendPath(xiiShaderManager::GetActivePlatform().GetData());
  sShaderStageFile.AppendFormat("/{0}_{1}.xiiShaderStage", xiiGALShaderStage::Names[xiiGALShaderStage::GetStageIndex(m_Stage)], xiiArgU(m_uiSourceHash, 8, true, 16, true));

  xiiFileWriter StageFileOut;
  if (StageFileOut.Open(sShaderStageFile.GetData()).Failed())
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
xiiShaderStageBinary* xiiShaderStageBinary::LoadStageBinary(xiiBitflags<xiiGALShaderStage> Stage, xiiUInt32 uiHash)
{
  auto itStage = s_ShaderStageBinaries[xiiGALShaderStage::GetStageIndex(Stage)].Find(uiHash);

  if (!itStage.IsValid())
  {
    xiiStringBuilder sShaderStageFile = xiiShaderManager::GetCacheDirectory();

    sShaderStageFile.AppendPath(xiiShaderManager::GetActivePlatform().GetData());
    sShaderStageFile.AppendFormat("/{0}_{1}.xiiShaderStage", xiiGALShaderStage::Names[xiiGALShaderStage::GetStageIndex(Stage)], xiiArgU(uiHash, 8, true, 16, true));

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

    itStage = xiiShaderStageBinary::s_ShaderStageBinaries[xiiGALShaderStage::GetStageIndex(Stage)].Insert(uiHash, shaderStageBinary);
  }

  if (!itStage.IsValid())
  {
    return nullptr;
  }

  xiiShaderStageBinary* pShaderStageBinary = &itStage.Value();

  if (pShaderStageBinary->m_GALByteCode == nullptr && !pShaderStageBinary->m_ByteCode.IsEmpty())
  {
    pShaderStageBinary->m_GALByteCode = XII_DEFAULT_NEW(xiiGALShaderByteCode, pShaderStageBinary->m_ByteCode);
  }

  return pShaderStageBinary;
}

// static
void xiiShaderStageBinary::OnEngineShutdown()
{
  for (xiiUInt32 stage = 0; stage < xiiGALShaderStage::ENUM_COUNT; ++stage)
  {
    s_ShaderStageBinaries[stage].Clear();
  }
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Shader_Implementation_ShaderStageBinary);

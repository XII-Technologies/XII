#include <GraphicsCore/GraphicsCorePCH.h>

#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <GraphicsCore/Shader/ShaderStageBinary.h>
#include <GraphicsCore/Shader/Types.h>
#include <GraphicsCore/ShaderCompiler/ShaderManager.h>

xiiUInt32 xiiShaderConstantBufferLayout::Constant::s_TypeSize[(xiiUInt32)Type::ENUM_COUNT] = {0, sizeof(float) * 1, sizeof(float) * 2, sizeof(float) * 3, sizeof(float) * 4, sizeof(int) * 1, sizeof(int) * 2, sizeof(int) * 3, sizeof(int) * 4, sizeof(xiiUInt32) * 1, sizeof(xiiUInt32) * 2,
                                                                                              sizeof(xiiUInt32) * 3, sizeof(xiiUInt32) * 4, sizeof(xiiShaderMat3), sizeof(xiiMat4), sizeof(xiiShaderTransform), sizeof(xiiShaderBool)};

void xiiShaderConstantBufferLayout::Constant::CopyDataFormVariant(xiiUInt8* pDest, xiiVariant* pValue) const
{
  XII_ASSERT_DEV(m_uiArrayElements == 1, "Array constants are not supported");

  xiiResult conversionResult = XII_FAILURE;

  if (pValue != nullptr)
  {
    switch (m_Type)
    {
      case Type::Float1:
        *reinterpret_cast<float*>(pDest) = pValue->ConvertTo<float>(&conversionResult);
        break;
      case Type::Float2:
        *reinterpret_cast<xiiVec2*>(pDest) = pValue->Get<xiiVec2>();
        return;
      case Type::Float3:
        *reinterpret_cast<xiiVec3*>(pDest) = pValue->Get<xiiVec3>();
        return;
      case Type::Float4:
        if (pValue->GetType() == xiiVariant::Type::Color || pValue->GetType() == xiiVariant::Type::ColorGamma)
        {
          const xiiColor tmp                 = pValue->ConvertTo<xiiColor>();
          *reinterpret_cast<xiiVec4*>(pDest) = *reinterpret_cast<const xiiVec4*>(&tmp);
        }
        else
        {
          *reinterpret_cast<xiiVec4*>(pDest) = pValue->Get<xiiVec4>();
        }
        return;

      case Type::Int1:
        *reinterpret_cast<xiiInt32*>(pDest) = pValue->ConvertTo<xiiInt32>(&conversionResult);
        break;
      case Type::Int2:
        *reinterpret_cast<xiiVec2I32*>(pDest) = pValue->Get<xiiVec2I32>();
        return;
      case Type::Int3:
        *reinterpret_cast<xiiVec3I32*>(pDest) = pValue->Get<xiiVec3I32>();
        return;
      case Type::Int4:
        *reinterpret_cast<xiiVec4I32*>(pDest) = pValue->Get<xiiVec4I32>();
        return;

      case Type::UInt1:
        *reinterpret_cast<xiiUInt32*>(pDest) = pValue->ConvertTo<xiiUInt32>(&conversionResult);
        break;
      case Type::UInt2:
        *reinterpret_cast<xiiVec2U32*>(pDest) = pValue->Get<xiiVec2U32>();
        return;
      case Type::UInt3:
        *reinterpret_cast<xiiVec3U32*>(pDest) = pValue->Get<xiiVec3U32>();
        return;
      case Type::UInt4:
        *reinterpret_cast<xiiVec4U32*>(pDest) = pValue->Get<xiiVec4U32>();
        return;

      case Type::Mat3x3:
        *reinterpret_cast<xiiShaderMat3*>(pDest) = pValue->Get<xiiMat3>();
        return;
      case Type::Mat4x4:
        *reinterpret_cast<xiiMat4*>(pDest) = pValue->Get<xiiMat4>();
        return;
      case Type::Transform:
        *reinterpret_cast<xiiShaderTransform*>(pDest) = pValue->Get<xiiTransform>();
        return;

      case Type::Bool:
        *reinterpret_cast<xiiShaderBool*>(pDest) = pValue->ConvertTo<bool>(&conversionResult);
        break;

      default:
        XII_ASSERT_NOT_IMPLEMENTED;
    }
  }

  if (conversionResult.Succeeded())
  {
    return;
  }

  // xiiLog::Error("Constant '{0}' is not set, invalid or couldn't be converted to target type and will be set to zero.", m_sName);
  const xiiUInt32 uiSize = s_TypeSize[m_Type];
  xiiMemoryUtils::ZeroFill(pDest, uiSize);
}

xiiShaderConstantBufferLayout::xiiShaderConstantBufferLayout()
{
  m_uiTotalSize = 0;
}

xiiShaderConstantBufferLayout::~xiiShaderConstantBufferLayout() = default;

xiiResult xiiShaderConstantBufferLayout::Write(xiiStreamWriter& inout_stream) const
{
  inout_stream << m_uiTotalSize;

  xiiUInt16 uiConstants = static_cast<xiiUInt16>(m_Constants.GetCount());
  inout_stream << uiConstants;

  for (auto& constant : m_Constants)
  {
    inout_stream << constant.m_sName;
    inout_stream << constant.m_Type;
    inout_stream << constant.m_uiArrayElements;
    inout_stream << constant.m_uiOffset;
  }

  return XII_SUCCESS;
}

xiiResult xiiShaderConstantBufferLayout::Read(xiiStreamReader& inout_stream)
{
  inout_stream >> m_uiTotalSize;

  xiiUInt16 uiConstants = 0;
  inout_stream >> uiConstants;

  m_Constants.SetCount(uiConstants);

  for (auto& constant : m_Constants)
  {
    inout_stream >> constant.m_sName;
    inout_stream >> constant.m_Type;
    inout_stream >> constant.m_uiArrayElements;
    inout_stream >> constant.m_uiOffset;
  }

  return XII_SUCCESS;
}

//////////////////////////////////////////////////////////////////////////

xiiShaderResourceBinding::xiiShaderResourceBinding()
{
  m_Type    = xiiShaderResourceType::Unknown;
  m_iSlot   = -1;
  m_pLayout = nullptr;
}

xiiShaderResourceBinding::~xiiShaderResourceBinding() = default;

//////////////////////////////////////////////////////////////////////////

xiiMap<xiiUInt32, xiiShaderStageBinary> xiiShaderStageBinary::s_ShaderStageBinaries[xiiGALShaderStage::ENUM_COUNT];

xiiShaderStageBinary::xiiShaderStageBinary() = default;

xiiShaderStageBinary::~xiiShaderStageBinary()
{
  if (m_GALByteCode)
  {
    xiiGALShaderByteCode* pByteCode = m_GALByteCode;
    m_GALByteCode                   = nullptr;

    if (pByteCode->GetRefCount() == 0)
      XII_DEFAULT_DELETE(pByteCode);
  }

  for (auto& binding : m_ShaderResourceBindings)
  {
    if (binding.m_pLayout != nullptr)
    {
      xiiShaderConstantBufferLayout* pLayout = binding.m_pLayout;
      binding.m_pLayout                      = nullptr;

      if (pLayout->GetRefCount() == 0)
        XII_DEFAULT_DELETE(pLayout);
    }
  }
}

xiiResult xiiShaderStageBinary::Write(xiiStreamWriter& inout_stream) const
{
  const xiiUInt8 uiVersion = xiiShaderStageBinary::VersionCurrent;

  if (inout_stream.WriteBytes(&uiVersion, sizeof(xiiUInt8)).Failed())
    return XII_FAILURE;

  if (inout_stream.WriteDWordValue(&m_uiSourceHash).Failed())
    return XII_FAILURE;

  const xiiUInt8 uiStage = (xiiUInt8)m_Stage;

  if (inout_stream.WriteBytes(&uiStage, sizeof(xiiUInt8)).Failed())
    return XII_FAILURE;

  const xiiUInt32 uiByteCodeSize = m_ByteCode.GetCount();

  if (inout_stream.WriteDWordValue(&uiByteCodeSize).Failed())
    return XII_FAILURE;

  if (!m_ByteCode.IsEmpty() && inout_stream.WriteBytes(&m_ByteCode[0], uiByteCodeSize).Failed())
    return XII_FAILURE;

  xiiUInt16 uiResources = static_cast<xiiUInt16>(m_ShaderResourceBindings.GetCount());
  inout_stream << uiResources;

  for (const auto& r : m_ShaderResourceBindings)
  {
    inout_stream << r.m_sName.GetData();
    inout_stream << r.m_iSlot;
    inout_stream << (xiiUInt8)r.m_Type;

    if (r.m_Type == xiiShaderResourceType::ConstantBuffer)
    {
      XII_SUCCEED_OR_RETURN(r.m_pLayout->Write(inout_stream));
    }
  }

  inout_stream << m_bWasCompiledWithDebug;

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

  m_Stage = (xiiGALShaderStage::Enum)uiStage;

  xiiUInt32 uiByteCodeSize = 0;

  if (inout_stream.ReadDWordValue(&uiByteCodeSize).Failed())
    return XII_FAILURE;

  m_ByteCode.SetCountUninitialized(uiByteCodeSize);

  if (!m_ByteCode.IsEmpty() && inout_stream.ReadBytes(&m_ByteCode[0], uiByteCodeSize) != uiByteCodeSize)
    return XII_FAILURE;

  if (uiVersion >= xiiShaderStageBinary::Version2)
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
      r.m_Type = (xiiShaderResourceType::Enum)uiType;

      if (r.m_Type == xiiShaderResourceType::ConstantBuffer && uiVersion >= xiiShaderStageBinary::Version4)
      {
        auto pLayout = XII_DEFAULT_NEW(xiiShaderConstantBufferLayout);
        XII_SUCCEED_OR_RETURN(pLayout->Read(inout_stream));

        r.m_pLayout = pLayout;
      }
    }
  }

  if (uiVersion >= xiiShaderStageBinary::Version5)
  {
    inout_stream >> m_bWasCompiledWithDebug;
  }

  return XII_SUCCESS;
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
  sShaderStageFile.AppendFormat("/{0}_{1}.xiiShaderStage", xiiGALShaderStage::Names[m_Stage], xiiArgU(m_uiSourceHash, 8, true, 16, true));

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
xiiShaderStageBinary* xiiShaderStageBinary::LoadStageBinary(xiiGALShaderStage::Enum Stage, xiiUInt32 uiHash)
{
  auto itStage = s_ShaderStageBinaries[Stage].Find(uiHash);

  if (!itStage.IsValid())
  {
    xiiStringBuilder sShaderStageFile = xiiShaderManager::GetCacheDirectory();

    sShaderStageFile.AppendPath(xiiShaderManager::GetActivePlatform().GetData());
    sShaderStageFile.AppendFormat("/{0}_{1}.xiiShaderStage", xiiGALShaderStage::Names[Stage], xiiArgU(uiHash, 8, true, 16, true));

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

    itStage = xiiShaderStageBinary::s_ShaderStageBinaries[Stage].Insert(uiHash, shaderStageBinary);
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

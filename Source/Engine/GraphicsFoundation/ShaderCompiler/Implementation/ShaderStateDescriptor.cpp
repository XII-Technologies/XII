#include <GraphicsFoundation/GraphicsFoundationPCH.h>

#include <GraphicsFoundation/ShaderCompiler/ShaderPermutationBinary.h>
#include <GraphicsFoundation/Utilities/DescriptorHash.h>

struct xiiGALShaderStateVersion
{
  using StorageType = xiiUInt32;

  enum Enum : StorageType
  {
    Version1 = 1,

    ENUM_COUNT,
    Current = ENUM_COUNT - 1
  };
};

void xiiGALShaderStateResourceDescriptor::Save(xiiStreamWriter& inout_stream) const
{
  inout_stream << (xiiUInt32)xiiGALShaderStateVersion::Current;

  // Blend State
  {
    inout_stream << m_BlendDescription.m_bAlphaToCoverage;
    inout_stream << m_BlendDescription.m_bIndependentBlend;
    inout_stream << m_BlendDescription.m_LogicOperationEnable;
    inout_stream << (xiiUInt8)m_BlendDescription.m_LogicOperation.GetValue();

    const xiiUInt8 iBlends = (xiiUInt8)m_BlendDescription.m_RenderTargets.GetCount();
    inout_stream << iBlends;

    for (xiiUInt32 b = 0; b < iBlends; ++b)
    {
      inout_stream << m_BlendDescription.m_RenderTargets[b].m_bBlendEnable;
      inout_stream << (xiiUInt8)m_BlendDescription.m_RenderTargets[b].m_SourceBlend.GetValue();
      inout_stream << (xiiUInt8)m_BlendDescription.m_RenderTargets[b].m_DestinationBlend.GetValue();
      inout_stream << (xiiUInt8)m_BlendDescription.m_RenderTargets[b].m_BlendOperation.GetValue();
      inout_stream << (xiiUInt8)m_BlendDescription.m_RenderTargets[b].m_SourceBlendAlpha.GetValue();
      inout_stream << (xiiUInt8)m_BlendDescription.m_RenderTargets[b].m_DestinationBlendAlpha.GetValue();
      inout_stream << (xiiUInt8)m_BlendDescription.m_RenderTargets[b].m_BlendOperationAlpha.GetValue();
      inout_stream << (xiiUInt8)m_BlendDescription.m_RenderTargets[b].m_ColorMask.GetValue();
    }
  }

  // Depth Stencil State
  {
    inout_stream << m_DepthStencilDescription.m_bDepthEnable;
    inout_stream << m_DepthStencilDescription.m_bDepthWriteEnable;
    inout_stream << (xiiInt8)m_DepthStencilDescription.m_ComparisonDepthFunction.GetValue();
    inout_stream << m_DepthStencilDescription.m_bStencilEnable;
    inout_stream << m_DepthStencilDescription.m_uiStencilReadMask;
    inout_stream << m_DepthStencilDescription.m_uiStencilWriteMask;
    inout_stream << (xiiUInt8)m_DepthStencilDescription.m_FrontFace.m_StencilFailOperation.GetValue();
    inout_stream << (xiiUInt8)m_DepthStencilDescription.m_FrontFace.m_StencilDepthFailOperation.GetValue();
    inout_stream << (xiiUInt8)m_DepthStencilDescription.m_FrontFace.m_StencilPassOperation.GetValue();
    inout_stream << (xiiInt8)m_DepthStencilDescription.m_FrontFace.m_ComparisonFunction.GetValue();
    inout_stream << (xiiUInt8)m_DepthStencilDescription.m_BackFace.m_StencilFailOperation.GetValue();
    inout_stream << (xiiUInt8)m_DepthStencilDescription.m_BackFace.m_StencilDepthFailOperation.GetValue();
    inout_stream << (xiiUInt8)m_DepthStencilDescription.m_BackFace.m_StencilPassOperation.GetValue();
    inout_stream << (xiiInt8)m_DepthStencilDescription.m_BackFace.m_ComparisonFunction.GetValue();
  }

  // Rasterizer State
  {
    inout_stream << (xiiUInt8)m_RasterizerDescription.m_FillMode.GetValue();
    inout_stream << (xiiUInt8)m_RasterizerDescription.m_CullMode.GetValue();
    inout_stream << m_RasterizerDescription.m_bFrontCounterClockwise;
    inout_stream << m_RasterizerDescription.m_bDepthClipEnable;
    inout_stream << m_RasterizerDescription.m_bScissorEnable;
    inout_stream << m_RasterizerDescription.m_bAntialiasedLineEnable;
    inout_stream << m_RasterizerDescription.m_iDepthBias;
    inout_stream << m_RasterizerDescription.m_fDepthBiasClamp;
    inout_stream << m_RasterizerDescription.m_fSlopeScaledDepthBias;
  }
}

void xiiGALShaderStateResourceDescriptor::Load(xiiStreamReader& inout_stream)
{
  xiiUInt32 uiVersion = 0;
  inout_stream >> uiVersion;

  XII_ASSERT_DEV(uiVersion >= xiiGALShaderStateVersion::Version1 && uiVersion <= xiiGALShaderStateVersion::Current, "Invalid version {0}", uiVersion);

  // Blend State
  {
    inout_stream >> m_BlendDescription.m_bAlphaToCoverage;
    inout_stream >> m_BlendDescription.m_bIndependentBlend;
    inout_stream >> m_BlendDescription.m_LogicOperationEnable;
    {
      xiiUInt8 uiTemp;
      inout_stream >> uiTemp;
      m_BlendDescription.m_LogicOperation = (xiiGALLogicOperation::Enum)uiTemp;
    }

    xiiUInt8 uiBlends = 0;
    inout_stream >> uiBlends;

    m_BlendDescription.m_RenderTargets.SetCount(uiBlends);

    for (xiiUInt32 b = 0; b < uiBlends; ++b)
    {
      inout_stream >> m_BlendDescription.m_RenderTargets[b].m_bBlendEnable;

      xiiUInt8 uiTemp;
      inout_stream >> uiTemp;
      m_BlendDescription.m_RenderTargets[b].m_SourceBlend = (xiiGALBlendFactor::Enum)uiTemp;
      inout_stream >> uiTemp;
      m_BlendDescription.m_RenderTargets[b].m_DestinationBlend = (xiiGALBlendFactor::Enum)uiTemp;
      inout_stream >> uiTemp;
      m_BlendDescription.m_RenderTargets[b].m_BlendOperation = (xiiGALBlendOperation::Enum)uiTemp;
      inout_stream >> uiTemp;
      m_BlendDescription.m_RenderTargets[b].m_SourceBlendAlpha = (xiiGALBlendFactor::Enum)uiTemp;
      inout_stream >> uiTemp;
      m_BlendDescription.m_RenderTargets[b].m_DestinationBlendAlpha = (xiiGALBlendFactor::Enum)uiTemp;
      inout_stream >> uiTemp;
      m_BlendDescription.m_RenderTargets[b].m_BlendOperationAlpha = (xiiGALBlendOperation::Enum)uiTemp;
      inout_stream >> uiTemp;
      m_BlendDescription.m_RenderTargets[b].m_ColorMask = (xiiGALColorMask::Enum)uiTemp;
    }
  }

  // Depth Stencil State
  {
    inout_stream >> m_DepthStencilDescription.m_bDepthEnable;
    inout_stream >> m_DepthStencilDescription.m_bDepthWriteEnable;

    xiiInt8 iTemp = 0;
    inout_stream >> iTemp;
    m_DepthStencilDescription.m_ComparisonDepthFunction = (xiiGALComparisonFunction::Enum)iTemp;

    inout_stream >> m_DepthStencilDescription.m_bStencilEnable;
    inout_stream >> m_DepthStencilDescription.m_uiStencilReadMask;
    inout_stream >> m_DepthStencilDescription.m_uiStencilWriteMask;

    xiiUInt8 uiTemp = 0;
    inout_stream >> uiTemp;
    m_DepthStencilDescription.m_FrontFace.m_StencilFailOperation = (xiiGALStencilOperation::Enum)uiTemp;
    inout_stream >> uiTemp;
    m_DepthStencilDescription.m_FrontFace.m_StencilDepthFailOperation = (xiiGALStencilOperation::Enum)uiTemp;
    inout_stream >> uiTemp;
    m_DepthStencilDescription.m_FrontFace.m_StencilPassOperation = (xiiGALStencilOperation::Enum)uiTemp;
    inout_stream >> iTemp;
    m_DepthStencilDescription.m_FrontFace.m_ComparisonFunction = (xiiGALComparisonFunction::Enum)iTemp;
    inout_stream >> uiTemp;
    m_DepthStencilDescription.m_BackFace.m_StencilFailOperation = (xiiGALStencilOperation::Enum)uiTemp;
    inout_stream >> uiTemp;
    m_DepthStencilDescription.m_BackFace.m_StencilDepthFailOperation = (xiiGALStencilOperation::Enum)uiTemp;
    inout_stream >> uiTemp;
    m_DepthStencilDescription.m_BackFace.m_StencilPassOperation = (xiiGALStencilOperation::Enum)uiTemp;
    inout_stream >> iTemp;
    m_DepthStencilDescription.m_BackFace.m_ComparisonFunction = (xiiGALComparisonFunction::Enum)iTemp;
  }

  // Rasterizer State
  {
    xiiUInt8 uiTemp = 0;
    inout_stream >> uiTemp;
    m_RasterizerDescription.m_FillMode = (xiiGALFillMode::Enum)uiTemp;
    inout_stream >> uiTemp;
    m_RasterizerDescription.m_CullMode = (xiiGALCullMode::Enum)uiTemp;
    inout_stream >> m_RasterizerDescription.m_bFrontCounterClockwise;
    inout_stream >> m_RasterizerDescription.m_bDepthClipEnable;
    inout_stream >> m_RasterizerDescription.m_bScissorEnable;
    inout_stream >> m_RasterizerDescription.m_bAntialiasedLineEnable;
    inout_stream >> m_RasterizerDescription.m_iDepthBias;
    inout_stream >> m_RasterizerDescription.m_fDepthBiasClamp;
    inout_stream >> m_RasterizerDescription.m_fSlopeScaledDepthBias;
  }
}

xiiUInt32 xiiGALShaderStateResourceDescriptor::CalculateHash() const
{
  return xiiGALDescriptorHash::Hash(m_BlendDescription) + m_RasterizerDescription.CalculateHash() + m_DepthStencilDescription.CalculateHash();
}

static xiiStringView InsertNumber(const char* szString, xiiUInt32 uiNumber, xiiStringBuilder& ref_sTemp)
{
  ref_sTemp.SetFormat(szString, uiNumber);
  return ref_sTemp.GetView();
}

#if XII_ENABLED(XII_COMPILE_FOR_DEBUG)
static xiiSet<xiiString> s_AllAllowedVariables;
#endif

static bool GetBoolStateVariable(const xiiMap<xiiString, xiiString>& variables, xiiStringView sVariable, bool bDefValue)
{
#if XII_ENABLED(XII_COMPILE_FOR_DEBUG)
  s_AllAllowedVariables.Insert(sVariable);
#endif

  auto it = variables.Find(sVariable);

  if (!it.IsValid())
    return bDefValue;

  if (it.Value() == "true")
    return true;
  if (it.Value() == "false")
    return false;

  xiiLog::Error("Shader state variable '{0}' is set to invalid value '{1}'. Should be 'true' or 'false'", sVariable, it.Value());
  return bDefValue;
}

static xiiInt32 GetEnumStateVariable(const xiiMap<xiiString, xiiString>& variables, const xiiMap<xiiString, xiiInt32>& values, xiiStringView sVariable, xiiInt32 iDefValue)
{
#if XII_ENABLED(XII_COMPILE_FOR_DEBUG)
  s_AllAllowedVariables.Insert(sVariable);
#endif

  auto it = variables.Find(sVariable);

  if (!it.IsValid())
    return iDefValue;

  auto itVal = values.Find(it.Value());
  if (!itVal.IsValid())
  {
    xiiStringBuilder valid;
    for (auto vv = values.GetIterator(); vv.IsValid(); ++vv)
    {
      valid.Append(" ", vv.Key());
    }

    xiiLog::Error("Shader state variable '{0}' is set to invalid value '{1}'. Valid values are:{2}", sVariable, it.Value(), valid);
    return iDefValue;
  }

  return itVal.Value();
}

static float GetFloatStateVariable(const xiiMap<xiiString, xiiString>& variables, xiiStringView sVariable, float fDefValue)
{
#if XII_ENABLED(XII_COMPILE_FOR_DEBUG)
  s_AllAllowedVariables.Insert(sVariable);
#endif

  auto it = variables.Find(sVariable);

  if (!it.IsValid())
    return fDefValue;

  double result = 0;
  if (xiiConversionUtils::StringToFloat(it.Value(), result).Failed())
  {
    xiiLog::Error("Shader state variable '{0}' is not a valid float value: '{1}'.", sVariable, it.Value());
    return fDefValue;
  }

  return (float)result;
}

static xiiInt32 GetIntStateVariable(const xiiMap<xiiString, xiiString>& variables, xiiStringView sVariable, xiiInt32 iDefValue)
{
#if XII_ENABLED(XII_COMPILE_FOR_DEBUG)
  s_AllAllowedVariables.Insert(sVariable);
#endif

  auto it = variables.Find(sVariable);

  if (!it.IsValid())
    return iDefValue;

  xiiInt32 result = 0;
  if (xiiConversionUtils::StringToInt(it.Value(), result).Failed())
  {
    xiiLog::Error("Shader state variable '{0}' is not a valid int value: '{1}'.", sVariable, it.Value());
    return iDefValue;
  }

  return result;
}

// Global variables don't use memory tracking, so these won't reported as memory leaks.
static xiiMutex                    StateValuesLock;
static xiiMap<xiiString, xiiInt32> StateValuesBlendFactor;
static xiiMap<xiiString, xiiInt32> StateValuesBlendOperation;
static xiiMap<xiiString, xiiInt32> StateValuesFillMode;
static xiiMap<xiiString, xiiInt32> StateValuesCullMode;
static xiiMap<xiiString, xiiInt32> StateValuesComparisonFunction;
static xiiMap<xiiString, xiiInt32> StateValuesStencilOperation;
static xiiMap<xiiString, xiiInt32> StateValuesLogicOperation;

xiiResult xiiGALShaderStateResourceDescriptor::Parse(xiiStringView sSource)
{
  xiiMap<xiiString, xiiString> VariableValues;

  // extract all state assignments
  {
    xiiStringBuilder sSourceBuilder = sSource;

    xiiHybridArray<xiiStringView, 32> allAssignments;
    xiiHybridArray<xiiStringView, 4>  components;
    sSourceBuilder.Split(false, allAssignments, "\n", ";", "\r");

    xiiStringBuilder temp;
    for (const xiiStringView& assignment : allAssignments)
    {
      temp = assignment;
      temp.Trim(" \t\r\n;");
      if (temp.IsEmpty())
        continue;

      temp.Split(false, components, " ", "\t", "=", "\r");

      if (components.GetCount() != 2)
      {
        xiiLog::Error("Malformed shader state assignment: '{0}'", temp);
        continue;
      }

      VariableValues[components[0]] = components[1];
    }
  }

  {
    XII_LOCK(StateValuesLock);

    if (StateValuesBlendFactor.IsEmpty())
    {
      // xiiGALBlendFactor
      {
        StateValuesBlendFactor["BlendFactor_Zero"]                    = xiiGALBlendFactor::Zero;
        StateValuesBlendFactor["BlendFactor_One"]                     = xiiGALBlendFactor::One;
        StateValuesBlendFactor["BlendFactor_SourceColor"]             = xiiGALBlendFactor::SourceColor;
        StateValuesBlendFactor["BlendFactor_InverseSourceColor"]      = xiiGALBlendFactor::InverseSourceColor;
        StateValuesBlendFactor["BlendFactor_SourceAlpha"]             = xiiGALBlendFactor::SourceAlpha;
        StateValuesBlendFactor["BlendFactor_InverseSourceAlpha"]      = xiiGALBlendFactor::InverseSourceAlpha;
        StateValuesBlendFactor["BlendFactor_DestinationAlpha"]        = xiiGALBlendFactor::DestinationAlpha;
        StateValuesBlendFactor["BlendFactor_InverseDestinationAlpha"] = xiiGALBlendFactor::InverseDestinationAlpha;
        StateValuesBlendFactor["BlendFactor_DestinationColor"]        = xiiGALBlendFactor::DestinationColor;
        StateValuesBlendFactor["BlendFactor_InverseDestinationColor"] = xiiGALBlendFactor::InverseDestinationColor;
        StateValuesBlendFactor["BlendFactor_SourceAlphaSaturate"]     = xiiGALBlendFactor::SourceAlphaSaturate;
        StateValuesBlendFactor["BlendFactor_BlendFactor"]             = xiiGALBlendFactor::BlendFactor;
        StateValuesBlendFactor["BlendFactor_InverseBlendFactor"]      = xiiGALBlendFactor::InverseBlendFactor;
        StateValuesBlendFactor["BlendFactor_SourceOneColor"]          = xiiGALBlendFactor::SourceOneColor;
        StateValuesBlendFactor["BlendFactor_InverseSourceOneColor"]   = xiiGALBlendFactor::InverseSourceOneColor;
        StateValuesBlendFactor["BlendFactor_SourceOneAlpha"]          = xiiGALBlendFactor::SourceOneAlpha;
        StateValuesBlendFactor["BlendFactor_InverseSourceOneAlpha"]   = xiiGALBlendFactor::InverseSourceOneAlpha;
      }

      // xiiGALBlendOperation
      {
        StateValuesBlendOperation["BlendOperation_Add"]             = xiiGALBlendOperation::Add;
        StateValuesBlendOperation["BlendOperation_Subtract"]        = xiiGALBlendOperation::Subtract;
        StateValuesBlendOperation["BlendOperation_ReverseSubtract"] = xiiGALBlendOperation::ReverseSubtract;
        StateValuesBlendOperation["BlendOperation_Min"]             = xiiGALBlendOperation::Min;
        StateValuesBlendOperation["BlendOperation_Max"]             = xiiGALBlendOperation::Max;
      }

      // xiiGALFillMode
      {
        StateValuesFillMode["FillMode_Wireframe"] = xiiGALFillMode::Wireframe;
        StateValuesFillMode["FillMode_Solid"]     = xiiGALFillMode::Solid;
      }

      // xiiGALCullMode
      {
        StateValuesCullMode["CullMode_None"]  = xiiGALCullMode::None;
        StateValuesCullMode["CullMode_Front"] = xiiGALCullMode::Front;
        StateValuesCullMode["CullMode_Back"]  = xiiGALCullMode::Back;
      }

      // xiiGALComparisonFunction
      {
        StateValuesComparisonFunction["ComparisonFunction_Never"]        = xiiGALComparisonFunction::Never;
        StateValuesComparisonFunction["ComparisonFunction_Less"]         = xiiGALComparisonFunction::Less;
        StateValuesComparisonFunction["ComparisonFunction_Equal"]        = xiiGALComparisonFunction::Equal;
        StateValuesComparisonFunction["ComparisonFunction_LessEqual"]    = xiiGALComparisonFunction::LessEqual;
        StateValuesComparisonFunction["ComparisonFunction_Greater"]      = xiiGALComparisonFunction::Greater;
        StateValuesComparisonFunction["ComparisonFunction_NotEqual"]     = xiiGALComparisonFunction::NotEqual;
        StateValuesComparisonFunction["ComparisonFunction_GreaterEqual"] = xiiGALComparisonFunction::GreaterEqual;
        StateValuesComparisonFunction["ComparisonFunction_Always"]       = xiiGALComparisonFunction::Always;
      }

      // xiiGALStencilOperation
      {
        StateValuesStencilOperation["StencilOperation_Keep"]              = xiiGALStencilOperation::Keep;
        StateValuesStencilOperation["StencilOperation_Zero"]              = xiiGALStencilOperation::Zero;
        StateValuesStencilOperation["StencilOperation_Replace"]           = xiiGALStencilOperation::Replace;
        StateValuesStencilOperation["StencilOperation_IncrementSaturate"] = xiiGALStencilOperation::IncrementSaturate;
        StateValuesStencilOperation["StencilOperation_DecrementSaturate"] = xiiGALStencilOperation::DecrementSaturate;
        StateValuesStencilOperation["StencilOperation_Invert"]            = xiiGALStencilOperation::Invert;
        StateValuesStencilOperation["StencilOperation_IncrementWrap"]     = xiiGALStencilOperation::IncrementWrap;
        StateValuesStencilOperation["StencilOperation_DecrementWrap"]     = xiiGALStencilOperation::DecrementWrap;
      }

      // xiiGALLogicOperation
      {
        StateValuesLogicOperation["LogicOperation_Clear"]        = xiiGALLogicOperation::Clear;
        StateValuesLogicOperation["LogicOperation_Set"]          = xiiGALLogicOperation::Set;
        StateValuesLogicOperation["LogicOperation_Copy"]         = xiiGALLogicOperation::Copy;
        StateValuesLogicOperation["LogicOperation_CopyInverted"] = xiiGALLogicOperation::CopyInverted;
        StateValuesLogicOperation["LogicOperation_NoOperation"]  = xiiGALLogicOperation::NoOperation;
        StateValuesLogicOperation["LogicOperation_Invert"]       = xiiGALLogicOperation::Invert;
        StateValuesLogicOperation["LogicOperation_AND"]          = xiiGALLogicOperation::AND;
        StateValuesLogicOperation["LogicOperation_NAND"]         = xiiGALLogicOperation::NAND;
        StateValuesLogicOperation["LogicOperation_OR"]           = xiiGALLogicOperation::OR;
        StateValuesLogicOperation["LogicOperation_NOR"]          = xiiGALLogicOperation::NOR;
        StateValuesLogicOperation["LogicOperation_XOR"]          = xiiGALLogicOperation::XOR;
        StateValuesLogicOperation["LogicOperation_Equivalent"]   = xiiGALLogicOperation::Equivalent;
        StateValuesLogicOperation["LogicOperation_AndReversed"]  = xiiGALLogicOperation::AndReversed;
        StateValuesLogicOperation["LogicOperation_AndInverted"]  = xiiGALLogicOperation::AndInverted;
        StateValuesLogicOperation["LogicOperation_OrReversed"]   = xiiGALLogicOperation::OrReversed;
        StateValuesLogicOperation["LogicOperation_OrInverted"]   = xiiGALLogicOperation::OrInverted;
      }
    }
  }

  // Retrieve Blend State
  {
    m_BlendDescription.m_bAlphaToCoverage     = GetBoolStateVariable(VariableValues, "AlphaToCoverage", m_BlendDescription.m_bAlphaToCoverage);
    m_BlendDescription.m_bIndependentBlend    = GetBoolStateVariable(VariableValues, "IndependentBlend", m_BlendDescription.m_bIndependentBlend);
    m_BlendDescription.m_LogicOperationEnable = GetBoolStateVariable(VariableValues, "LogicOperationEnable", m_BlendDescription.m_LogicOperationEnable);
    m_BlendDescription.m_LogicOperation       = (xiiGALLogicOperation::Enum)GetEnumStateVariable(VariableValues, StateValuesLogicOperation, "LogicOperation", m_BlendDescription.m_LogicOperation);

    xiiStringBuilder s;

    m_BlendDescription.m_RenderTargets.SetCount(8U);
    for (xiiUInt32 i = 0; i < m_BlendDescription.m_RenderTargets.GetCount(); ++i)
    {
      m_BlendDescription.m_RenderTargets[i].m_bBlendEnable          = GetBoolStateVariable(VariableValues, InsertNumber("BlendEnable{0}", i, s), m_BlendDescription.m_RenderTargets[0].m_bBlendEnable);
      m_BlendDescription.m_RenderTargets[i].m_SourceBlend           = (xiiGALBlendFactor::Enum)GetEnumStateVariable(VariableValues, StateValuesBlendFactor, InsertNumber("SourceBlend{0}", i, s), m_BlendDescription.m_RenderTargets[0].m_SourceBlend);
      m_BlendDescription.m_RenderTargets[i].m_DestinationBlend      = (xiiGALBlendFactor::Enum)GetEnumStateVariable(VariableValues, StateValuesBlendFactor, InsertNumber("DestinationBlend{0}", i, s), m_BlendDescription.m_RenderTargets[0].m_DestinationBlend);
      m_BlendDescription.m_RenderTargets[i].m_BlendOperation        = (xiiGALBlendOperation::Enum)GetEnumStateVariable(VariableValues, StateValuesBlendOperation, InsertNumber("BlendOperation{0}", i, s), m_BlendDescription.m_RenderTargets[0].m_BlendOperation);
      m_BlendDescription.m_RenderTargets[i].m_SourceBlendAlpha      = (xiiGALBlendFactor::Enum)GetEnumStateVariable(VariableValues, StateValuesBlendFactor, InsertNumber("SourceBlendAlpha{0}", i, s), m_BlendDescription.m_RenderTargets[0].m_SourceBlendAlpha);
      m_BlendDescription.m_RenderTargets[i].m_DestinationBlendAlpha = (xiiGALBlendFactor::Enum)GetEnumStateVariable(VariableValues, StateValuesBlendFactor, InsertNumber("DestinationBlendAlpha{0}", i, s), m_BlendDescription.m_RenderTargets[0].m_DestinationBlendAlpha);
      m_BlendDescription.m_RenderTargets[i].m_ColorMask             = (xiiGALColorMask::Enum)GetIntStateVariable(VariableValues, InsertNumber("ColorMask{0}", i, s), m_BlendDescription.m_RenderTargets[0].m_ColorMask.GetValue());
    }
  }

  // Retrieve Rasterizer State
  {
    m_RasterizerDescription.m_FillMode               = (xiiGALFillMode::Enum)GetEnumStateVariable(VariableValues, StateValuesFillMode, "FillMode", m_RasterizerDescription.m_FillMode);
    m_RasterizerDescription.m_CullMode               = (xiiGALCullMode::Enum)GetEnumStateVariable(VariableValues, StateValuesCullMode, "CullMode", m_RasterizerDescription.m_CullMode);
    m_RasterizerDescription.m_bFrontCounterClockwise = GetBoolStateVariable(VariableValues, "FrontCounterClockwise", m_RasterizerDescription.m_bFrontCounterClockwise);
    m_RasterizerDescription.m_bDepthClipEnable       = GetBoolStateVariable(VariableValues, "DepthClipEnable", m_RasterizerDescription.m_bDepthClipEnable);
    m_RasterizerDescription.m_bScissorEnable         = GetBoolStateVariable(VariableValues, "ScissorEnable", m_RasterizerDescription.m_bScissorEnable);
    m_RasterizerDescription.m_bAntialiasedLineEnable = GetBoolStateVariable(VariableValues, "AntialiasedLineEnable", m_RasterizerDescription.m_bAntialiasedLineEnable);
    m_RasterizerDescription.m_iDepthBias             = GetIntStateVariable(VariableValues, "DepthBias", m_RasterizerDescription.m_iDepthBias);
    m_RasterizerDescription.m_fDepthBiasClamp        = GetFloatStateVariable(VariableValues, "DepthBiasClamp", m_RasterizerDescription.m_fDepthBiasClamp);
    m_RasterizerDescription.m_fSlopeScaledDepthBias  = GetFloatStateVariable(VariableValues, "SlopeScaledDepthBias", m_RasterizerDescription.m_fSlopeScaledDepthBias);
  }

  // Retrieve Depth-Stencil State
  {
    m_DepthStencilDescription.m_bDepthEnable            = GetBoolStateVariable(VariableValues, "DepthEnable", m_DepthStencilDescription.m_bDepthEnable);
    m_DepthStencilDescription.m_bDepthWriteEnable       = GetBoolStateVariable(VariableValues, "DepthWriteEnable", m_DepthStencilDescription.m_bDepthWriteEnable);
    m_DepthStencilDescription.m_ComparisonDepthFunction = (xiiGALComparisonFunction::Enum)GetEnumStateVariable(VariableValues, StateValuesComparisonFunction, "ComparisonDepthFunction", m_DepthStencilDescription.m_ComparisonDepthFunction);
    m_DepthStencilDescription.m_bStencilEnable          = GetBoolStateVariable(VariableValues, "StencilEnable", m_DepthStencilDescription.m_bStencilEnable);
    m_DepthStencilDescription.m_uiStencilReadMask       = static_cast<xiiUInt8>(GetIntStateVariable(VariableValues, "StencilReadMask", m_DepthStencilDescription.m_uiStencilReadMask));
    m_DepthStencilDescription.m_uiStencilWriteMask      = static_cast<xiiUInt8>(GetIntStateVariable(VariableValues, "StencilWriteMask", m_DepthStencilDescription.m_uiStencilWriteMask));

    m_DepthStencilDescription.m_FrontFace.m_StencilFailOperation      = (xiiGALStencilOperation::Enum)GetEnumStateVariable(VariableValues, StateValuesStencilOperation, "FrontFaceStencilFailOperation", m_DepthStencilDescription.m_FrontFace.m_StencilFailOperation);
    m_DepthStencilDescription.m_FrontFace.m_StencilDepthFailOperation = (xiiGALStencilOperation::Enum)GetEnumStateVariable(VariableValues, StateValuesStencilOperation, "FrontFaceStencilDepthFailOperation", m_DepthStencilDescription.m_FrontFace.m_StencilDepthFailOperation);
    m_DepthStencilDescription.m_FrontFace.m_StencilPassOperation      = (xiiGALStencilOperation::Enum)GetEnumStateVariable(VariableValues, StateValuesStencilOperation, "FrontFaceStencilPassOperation", m_DepthStencilDescription.m_FrontFace.m_StencilPassOperation);
    m_DepthStencilDescription.m_FrontFace.m_ComparisonFunction        = (xiiGALComparisonFunction::Enum)GetEnumStateVariable(VariableValues, StateValuesComparisonFunction, "FrontFaceComparisonFunction", m_DepthStencilDescription.m_FrontFace.m_ComparisonFunction);

    m_DepthStencilDescription.m_BackFace.m_StencilFailOperation      = (xiiGALStencilOperation::Enum)GetEnumStateVariable(VariableValues, StateValuesStencilOperation, "BackFaceStencilFailOperation", m_DepthStencilDescription.m_BackFace.m_StencilFailOperation);
    m_DepthStencilDescription.m_BackFace.m_StencilDepthFailOperation = (xiiGALStencilOperation::Enum)GetEnumStateVariable(VariableValues, StateValuesStencilOperation, "BackFaceStencilDepthFailOperation", m_DepthStencilDescription.m_BackFace.m_StencilDepthFailOperation);
    m_DepthStencilDescription.m_BackFace.m_StencilPassOperation      = (xiiGALStencilOperation::Enum)GetEnumStateVariable(VariableValues, StateValuesStencilOperation, "BackFaceStencilPassOperation", m_DepthStencilDescription.m_BackFace.m_StencilPassOperation);
    m_DepthStencilDescription.m_BackFace.m_ComparisonFunction        = (xiiGALComparisonFunction::Enum)GetEnumStateVariable(VariableValues, StateValuesComparisonFunction, "BackFaceComparisonFunction", m_DepthStencilDescription.m_BackFace.m_ComparisonFunction);
  }

#if XII_ENABLED(XII_COMPILE_FOR_DEBUG)
  // check for invalid variable names
  {
    for (auto it = VariableValues.GetIterator(); it.IsValid(); ++it)
    {
      if (!s_AllAllowedVariables.Contains(it.Key()))
      {
        xiiLog::Error("The shader state variable '{0}' does not exist.", it.Key());
      }
    }
  }
#endif

  return XII_SUCCESS;
}

XII_STATICLINK_FILE(GraphicsFoundation, GraphicsFoundation_Shader_Implementation_ShaderStateDescriptor);

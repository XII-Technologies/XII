#include <GraphicsCore/GraphicsCorePCH.h>

#include <Foundation/CodeUtils/TokenParseUtils.h>
#include <Foundation/CodeUtils/Tokenizer.h>
#include <Foundation/Types/Variant.h>
#include <Foundation/Utilities/ConversionUtils.h>
#include <GraphicsCore/Shader/Implementation/Helper.h>
#include <GraphicsCore/ShaderCompiler/ShaderManager.h>
#include <GraphicsCore/ShaderCompiler/ShaderParser.h>

using namespace xiiTokenParseUtils;

namespace
{
  static xiiHashTable<xiiStringView, const xiiRTTI*>                    s_NameToTypeTable;
  static xiiHashTable<xiiStringView, xiiEnum<xiiGALShaderResourceType>> s_NameToDescriptorTable;
  static xiiHashTable<xiiStringView, xiiEnum<xiiGALShaderTextureType>>  s_NameToTextureTable;

  void InitializeTables()
  {
    if (!s_NameToTypeTable.IsEmpty())
      return;

    s_NameToTypeTable.Insert("float", xiiGetStaticRTTI<float>());
    s_NameToTypeTable.Insert("float2", xiiGetStaticRTTI<xiiVec2>());
    s_NameToTypeTable.Insert("float3", xiiGetStaticRTTI<xiiVec3>());
    s_NameToTypeTable.Insert("float4", xiiGetStaticRTTI<xiiVec4>());
    s_NameToTypeTable.Insert("int", xiiGetStaticRTTI<int>());
    s_NameToTypeTable.Insert("int2", xiiGetStaticRTTI<xiiVec2I32>());
    s_NameToTypeTable.Insert("int3", xiiGetStaticRTTI<xiiVec3I32>());
    s_NameToTypeTable.Insert("int4", xiiGetStaticRTTI<xiiVec4I32>());
    s_NameToTypeTable.Insert("uint", xiiGetStaticRTTI<xiiUInt32>());
    s_NameToTypeTable.Insert("uint2", xiiGetStaticRTTI<xiiVec2U32>());
    s_NameToTypeTable.Insert("uint3", xiiGetStaticRTTI<xiiVec3U32>());
    s_NameToTypeTable.Insert("uint4", xiiGetStaticRTTI<xiiVec4U32>());
    s_NameToTypeTable.Insert("bool", xiiGetStaticRTTI<bool>());
    s_NameToTypeTable.Insert("Color", xiiGetStaticRTTI<xiiColor>());
    /// \todo Are we going to support linear UB colors ?
    s_NameToTypeTable.Insert("Texture2D", xiiGetStaticRTTI<xiiString>());
    s_NameToTypeTable.Insert("Texture3D", xiiGetStaticRTTI<xiiString>());
    s_NameToTypeTable.Insert("TextureCube", xiiGetStaticRTTI<xiiString>());

    s_NameToDescriptorTable.Insert("cbuffer"_xiisv, xiiGALShaderResourceType::ConstantBuffer);
    s_NameToDescriptorTable.Insert("ConstantBuffer"_xiisv, xiiGALShaderResourceType::ConstantBuffer);
    s_NameToDescriptorTable.Insert("SamplerState"_xiisv, xiiGALShaderResourceType::Sampler);
    s_NameToDescriptorTable.Insert("SamplerComparisonState"_xiisv, xiiGALShaderResourceType::Sampler);
    s_NameToDescriptorTable.Insert("Texture1D"_xiisv, xiiGALShaderResourceType::TextureSRV);
    s_NameToDescriptorTable.Insert("Texture1DArray"_xiisv, xiiGALShaderResourceType::TextureSRV);
    s_NameToDescriptorTable.Insert("Texture2D"_xiisv, xiiGALShaderResourceType::TextureSRV);
    s_NameToDescriptorTable.Insert("Texture2DArray"_xiisv, xiiGALShaderResourceType::TextureSRV);
    s_NameToDescriptorTable.Insert("Texture2DMS"_xiisv, xiiGALShaderResourceType::TextureSRV);
    s_NameToDescriptorTable.Insert("Texture2DMSArray"_xiisv, xiiGALShaderResourceType::TextureSRV);
    s_NameToDescriptorTable.Insert("Texture3D"_xiisv, xiiGALShaderResourceType::TextureSRV);
    s_NameToDescriptorTable.Insert("TextureCube"_xiisv, xiiGALShaderResourceType::TextureSRV);
    s_NameToDescriptorTable.Insert("TextureCubeArray"_xiisv, xiiGALShaderResourceType::TextureSRV);
    s_NameToDescriptorTable.Insert("Buffer"_xiisv, xiiGALShaderResourceType::BufferSRV);
    s_NameToDescriptorTable.Insert("StructuredBuffer"_xiisv, xiiGALShaderResourceType::BufferSRV);
    s_NameToDescriptorTable.Insert("ByteAddressBuffer"_xiisv, xiiGALShaderResourceType::BufferSRV);
    s_NameToDescriptorTable.Insert("RWTexture1D"_xiisv, xiiGALShaderResourceType::TextureUAV);
    s_NameToDescriptorTable.Insert("RWTexture1DArray"_xiisv, xiiGALShaderResourceType::TextureUAV);
    s_NameToDescriptorTable.Insert("RWTexture2D"_xiisv, xiiGALShaderResourceType::TextureUAV);
    s_NameToDescriptorTable.Insert("RWTexture2DArray"_xiisv, xiiGALShaderResourceType::TextureUAV);
    s_NameToDescriptorTable.Insert("RWTexture3D"_xiisv, xiiGALShaderResourceType::TextureUAV);
    s_NameToDescriptorTable.Insert("RWBuffer"_xiisv, xiiGALShaderResourceType::BufferUAV);
    s_NameToDescriptorTable.Insert("RWStructuredBuffer"_xiisv, xiiGALShaderResourceType::BufferUAV);
    s_NameToDescriptorTable.Insert("RWByteAddressBuffer"_xiisv, xiiGALShaderResourceType::BufferUAV);
    s_NameToDescriptorTable.Insert("AppendStructuredBuffer"_xiisv, xiiGALShaderResourceType::BufferUAV);
    s_NameToDescriptorTable.Insert("ConsumeStructuredBuffer"_xiisv, xiiGALShaderResourceType::BufferUAV);

    s_NameToTextureTable.Insert("Texture1D"_xiisv, xiiGALShaderTextureType::Texture1D);
    s_NameToTextureTable.Insert("Texture1DArray"_xiisv, xiiGALShaderTextureType::Texture1DArray);
    s_NameToTextureTable.Insert("Texture2D"_xiisv, xiiGALShaderTextureType::Texture2D);
    s_NameToTextureTable.Insert("Texture2DArray"_xiisv, xiiGALShaderTextureType::Texture2DArray);
    s_NameToTextureTable.Insert("Texture2DMS"_xiisv, xiiGALShaderTextureType::Texture2DMS);
    s_NameToTextureTable.Insert("Texture2DMSArray"_xiisv, xiiGALShaderTextureType::Texture2DMSArray);
    s_NameToTextureTable.Insert("Texture3D"_xiisv, xiiGALShaderTextureType::Texture3D);
    s_NameToTextureTable.Insert("TextureCube"_xiisv, xiiGALShaderTextureType::TextureCube);
    s_NameToTextureTable.Insert("TextureCubeArray"_xiisv, xiiGALShaderTextureType::TextureCubeArray);
    s_NameToTextureTable.Insert("RWTexture1D"_xiisv, xiiGALShaderTextureType::Texture1D);
    s_NameToTextureTable.Insert("RWTexture1DArray"_xiisv, xiiGALShaderTextureType::Texture1DArray);
    s_NameToTextureTable.Insert("RWTexture2D"_xiisv, xiiGALShaderTextureType::Texture2D);
    s_NameToTextureTable.Insert("RWTexture2DArray"_xiisv, xiiGALShaderTextureType::Texture2DArray);
    s_NameToTextureTable.Insert("RWTexture3D"_xiisv, xiiGALShaderTextureType::Texture3D);
  }

  const xiiRTTI* GetType(xiiStringView sType)
  {
    InitializeTables();

    const xiiRTTI* pType = nullptr;
    s_NameToTypeTable.TryGetValue(sType, pType);
    return pType;
  }

  xiiVariant ParseValue(const TokenStream& tokens, xiiUInt32& ref_uiCurToken)
  {
    xiiUInt32 uiValueToken = ref_uiCurToken;

    if (Accept(tokens, ref_uiCurToken, xiiTokenType::String1, &uiValueToken) || Accept(tokens, ref_uiCurToken, xiiTokenType::String2, &uiValueToken))
    {
      xiiStringBuilder sValue = tokens[uiValueToken]->m_DataView;
      sValue.Trim("\"'");

      return xiiVariant(sValue.GetData());
    }

    if (Accept(tokens, ref_uiCurToken, xiiTokenType::Integer, &uiValueToken))
    {
      xiiString sValue = tokens[uiValueToken]->m_DataView;

      xiiInt64 iValue = 0;
      if (sValue.StartsWith_NoCase("0x"))
      {
        xiiUInt32 uiValue32 = 0;
        xiiConversionUtils::ConvertHexStringToUInt32(sValue, uiValue32).IgnoreResult();

        iValue = uiValue32;
      }
      else
      {
        xiiConversionUtils::StringToInt64(sValue, iValue).IgnoreResult();
      }

      return xiiVariant(iValue);
    }

    if (Accept(tokens, ref_uiCurToken, xiiTokenType::Float, &uiValueToken))
    {
      xiiString sValue = tokens[uiValueToken]->m_DataView;

      double fValue = 0;
      xiiConversionUtils::StringToFloat(sValue, fValue).IgnoreResult();

      return xiiVariant(fValue);
    }

    if (Accept(tokens, ref_uiCurToken, "true", &uiValueToken) || Accept(tokens, ref_uiCurToken, "false", &uiValueToken))
    {
      bool bValue = tokens[uiValueToken]->m_DataView == "true";
      return xiiVariant(bValue);
    }

    auto& dataView = tokens[ref_uiCurToken]->m_DataView;
    if (tokens[ref_uiCurToken]->m_iType == xiiTokenType::Identifier && xiiStringUtils::IsValidIdentifierName(dataView.GetStartPointer(), dataView.GetEndPointer()))
    {
      // complex type constructor
      const xiiRTTI* pType = nullptr;
      if (!s_NameToTypeTable.TryGetValue(dataView, pType))
      {
        xiiLog::Error("Invalid type name '{}'", dataView);
        return xiiVariant();
      }

      ++ref_uiCurToken;
      Accept(tokens, ref_uiCurToken, "(");

      xiiHybridArray<xiiVariant, 8> constructorArgs;

      while (!Accept(tokens, ref_uiCurToken, ")"))
      {
        xiiVariant value = ParseValue(tokens, ref_uiCurToken);
        if (value.IsValid())
        {
          constructorArgs.PushBack(value);
        }
        else
        {
          xiiLog::Error("Invalid arguments for constructor '{}'", pType->GetTypeName());
          return XII_FAILURE;
        }

        Accept(tokens, ref_uiCurToken, ",");
      }

      // find matching constructor
      auto functions = pType->GetFunctions();
      for (auto pFunc : functions)
      {
        if (pFunc->GetFunctionType() == xiiFunctionType::Constructor && pFunc->GetArgumentCount() == constructorArgs.GetCount())
        {
          xiiHybridArray<xiiVariant, 8> convertedArgs;
          bool                          bAllArgsValid = true;

          for (xiiUInt32 uiArg = 0; uiArg < pFunc->GetArgumentCount(); ++uiArg)
          {
            const xiiRTTI* pArgType         = pFunc->GetArgumentType(uiArg);
            xiiResult      conversionResult = XII_FAILURE;
            convertedArgs.PushBack(constructorArgs[uiArg].ConvertTo(pArgType->GetVariantType(), &conversionResult));
            if (conversionResult.Failed())
            {
              bAllArgsValid = false;
              break;
            }
          }

          if (bAllArgsValid)
          {
            xiiVariant result;
            pFunc->Execute(nullptr, convertedArgs, result);

            if (result.IsValid())
            {
              return result;
            }
          }
        }
      }
    }

    return xiiVariant();
  }

  xiiResult ParseAttribute(const TokenStream& tokens, xiiUInt32& ref_uiCurToken, xiiShaderParser::ParameterDefinition& out_parameterDefinition)
  {
    if (!Accept(tokens, ref_uiCurToken, "@"))
    {
      return XII_FAILURE;
    }

    xiiUInt32 uiTypeToken = ref_uiCurToken;
    if (!Accept(tokens, ref_uiCurToken, xiiTokenType::Identifier, &uiTypeToken))
    {
      return XII_FAILURE;
    }

    xiiShaderParser::AttributeDefinition& attributeDef = out_parameterDefinition.m_Attributes.ExpandAndGetRef();
    attributeDef.m_sType                               = tokens[uiTypeToken]->m_DataView;

    Accept(tokens, ref_uiCurToken, "(");

    while (!Accept(tokens, ref_uiCurToken, ")"))
    {
      xiiVariant value = ParseValue(tokens, ref_uiCurToken);
      if (value.IsValid())
      {
        attributeDef.m_Values.PushBack(value);
      }
      else
      {
        xiiLog::Error("Invalid arguments for attribute '{}'", attributeDef.m_sType);
        return XII_FAILURE;
      }

      Accept(tokens, ref_uiCurToken, ",");
    }

    return XII_SUCCESS;
  }

  xiiResult ParseParameter(const TokenStream& tokens, xiiUInt32& ref_uiCurToken, xiiShaderParser::ParameterDefinition& out_parameterDefinition)
  {
    xiiUInt32 uiTypeToken = ref_uiCurToken;
    if (!Accept(tokens, ref_uiCurToken, xiiTokenType::Identifier, &uiTypeToken))
    {
      return XII_FAILURE;
    }

    out_parameterDefinition.m_sType = tokens[uiTypeToken]->m_DataView;
    out_parameterDefinition.m_pType = GetType(out_parameterDefinition.m_sType);

    xiiUInt32 uiNameToken = ref_uiCurToken;
    if (!Accept(tokens, ref_uiCurToken, xiiTokenType::Identifier, &uiNameToken))
    {
      return XII_FAILURE;
    }

    out_parameterDefinition.m_sName = tokens[uiNameToken]->m_DataView;

    while (!Accept(tokens, ref_uiCurToken, ";"))
    {
      if (ParseAttribute(tokens, ref_uiCurToken, out_parameterDefinition).Failed())
      {
        return XII_FAILURE;
      }
    }

    return XII_SUCCESS;
  }

  xiiResult ParseEnum(const TokenStream& tokens, xiiUInt32& ref_uiCurToken, xiiShaderParser::EnumDefinition& out_enumDefinition, bool bCheckPrefix)
  {
    if (!Accept(tokens, ref_uiCurToken, "enum"))
    {
      return XII_FAILURE;
    }

    xiiUInt32 uiNameToken = ref_uiCurToken;
    if (!Accept(tokens, ref_uiCurToken, xiiTokenType::Identifier, &uiNameToken))
    {
      return XII_FAILURE;
    }

    out_enumDefinition.m_sName = tokens[uiNameToken]->m_DataView;
    xiiStringBuilder sEnumPrefix(out_enumDefinition.m_sName, "_");

    if (!Accept(tokens, ref_uiCurToken, "{"))
    {
      xiiLog::Error("Opening bracket expected for enum definition.");
      return XII_FAILURE;
    }

    xiiUInt32 uiDefaultValue = 0;
    xiiUInt32 uiCurrentValue = 0;

    while (true)
    {
      xiiUInt32 uiValueNameToken = ref_uiCurToken;
      if (!Accept(tokens, ref_uiCurToken, xiiTokenType::Identifier, &uiValueNameToken))
      {
        return XII_FAILURE;
      }

      xiiStringView sValueName = tokens[uiValueNameToken]->m_DataView;

      if (Accept(tokens, ref_uiCurToken, "="))
      {
        xiiUInt32 uiValueToken = ref_uiCurToken;
        Accept(tokens, ref_uiCurToken, xiiTokenType::Integer, &uiValueToken);

        xiiInt32 iValue = 0;
        if (xiiConversionUtils::StringToInt(tokens[uiValueToken]->m_DataView, iValue).Succeeded() && iValue >= 0)
        {
          uiCurrentValue = iValue;
        }
        else
        {
          xiiLog::Error("Invalid enum value '{0}'. Only positive numbers are allowed.", tokens[uiValueToken]->m_DataView);
        }
      }

      if (sValueName.IsEqual_NoCase("default"))
      {
        uiDefaultValue = uiCurrentValue;
      }
      else
      {
        if (bCheckPrefix && !sValueName.StartsWith(sEnumPrefix))
        {
          xiiLog::Error("Enum value does not start with the expected enum name as prefix: '{0}'", sEnumPrefix);
        }

        auto& ev = out_enumDefinition.m_Values.ExpandAndGetRef();

        const xiiStringBuilder sFinalName = sValueName;
        ev.m_sValueName.Assign(sFinalName.GetData());
        ev.m_iValueValue = static_cast<xiiInt32>(uiCurrentValue);
      }

      if (Accept(tokens, ref_uiCurToken, ","))
      {
        ++uiCurrentValue;
      }
      else
      {
        break;
      }

      if (Accept(tokens, ref_uiCurToken, "}"))
        goto after_braces;
    }

    if (!Accept(tokens, ref_uiCurToken, "}"))
    {
      xiiLog::Error("Closing bracket expected for enum definition.");
      return XII_FAILURE;
    }

  after_braces:

    out_enumDefinition.m_uiDefaultValue = uiDefaultValue;

    Accept(tokens, ref_uiCurToken, ";");

    return XII_SUCCESS;
  }

  void SkipWhitespace(xiiStringView& s)
  {
    while (s.IsValid() && xiiStringUtils::IsWhiteSpace(s.GetCharacter()))
    {
      ++s;
    }
  }
} // namespace

// static
void xiiShaderParser::ParseMaterialParameterSection(xiiStreamReader& inout_stream, xiiHybridArray<ParameterDefinition, 16>& out_parameter, xiiHybridArray<EnumDefinition, 4>& out_enumDefinitions)
{
  xiiString sContent;
  sContent.ReadAll(inout_stream);

  xiiShaderHelper::xiiTextSectionizer Sections;
  xiiShaderHelper::GetShaderSections(sContent.GetData(), Sections);

  xiiUInt32     uiFirstLine = 0;
  xiiStringView s           = Sections.GetSectionContent(xiiShaderHelper::xiiShaderSections::MATERIALPARAMETER, uiFirstLine);

  xiiTokenizer tokenizer;
  tokenizer.Tokenize(xiiArrayPtr<const xiiUInt8>((const xiiUInt8*)s.GetStartPointer(), s.GetElementCount()), xiiLog::GetThreadLocalLogSystem());

  TokenStream tokens;
  tokenizer.GetAllLines(tokens);

  xiiUInt32 uiCurToken = 0;

  while (!Accept(tokens, uiCurToken, xiiTokenType::EndOfFile))
  {
    EnumDefinition enumDef;
    if (ParseEnum(tokens, uiCurToken, enumDef, false).Succeeded())
    {
      XII_ASSERT_DEV(!enumDef.m_sName.IsEmpty(), "");

      out_enumDefinitions.PushBack(std::move(enumDef));
      continue;
    }

    ParameterDefinition paramDef;
    if (ParseParameter(tokens, uiCurToken, paramDef).Succeeded())
    {
      out_parameter.PushBack(std::move(paramDef));
      continue;
    }

    xiiLog::Error("Invalid token in material parameter section '{}'", tokens[uiCurToken]->m_DataView);
    break;
  }
}

// static
void xiiShaderParser::ParsePermutationSection(xiiStreamReader& inout_stream, xiiHybridArray<xiiHashedString, 16>& out_permVars, xiiHybridArray<xiiPermutationVar, 16>& out_fixedPermVars)
{
  xiiString sContent;
  sContent.ReadAll(inout_stream);

  xiiShaderHelper::xiiTextSectionizer Sections;
  xiiShaderHelper::GetShaderSections(sContent.GetData(), Sections);

  xiiUInt32     uiFirstLine   = 0;
  xiiStringView sPermutations = Sections.GetSectionContent(xiiShaderHelper::xiiShaderSections::PERMUTATIONS, uiFirstLine);
  ParsePermutationSection(sPermutations, out_permVars, out_fixedPermVars);
}

// static
void xiiShaderParser::ParsePermutationSection(xiiStringView s, xiiHybridArray<xiiHashedString, 16>& out_permVars, xiiHybridArray<xiiPermutationVar, 16>& out_fixedPermVars)
{
  out_permVars.Clear();
  out_fixedPermVars.Clear();

  xiiTokenizer tokenizer;
  tokenizer.Tokenize(xiiArrayPtr<const xiiUInt8>((const xiiUInt8*)s.GetStartPointer(), s.GetElementCount()), xiiLog::GetThreadLocalLogSystem());

  enum class State
  {
    Idle,
    HasName,
    HasEqual,
    HasValue
  };

  State            state = State::Idle;
  xiiStringBuilder sToken, sVarName;

  for (const auto& token : tokenizer.GetTokens())
  {
    if (token.m_iType == xiiTokenType::Whitespace || token.m_iType == xiiTokenType::BlockComment || token.m_iType == xiiTokenType::LineComment)
      continue;

    if (token.m_iType == xiiTokenType::String1 || token.m_iType == xiiTokenType::String2 || token.m_iType == xiiTokenType::RawString1)
    {
      sToken = token.m_DataView;
      xiiLog::Error("Strings are not allowed in the permutation section: '{0}'", sToken);
      return;
    }

    if (token.m_iType == xiiTokenType::Newline || token.m_iType == xiiTokenType::EndOfFile)
    {
      if (state == State::HasEqual)
      {
        xiiLog::Error("Missing assignment value in permutation section");
        return;
      }

      if (state == State::HasName)
      {
        out_permVars.ExpandAndGetRef().Assign(sVarName.GetData());
      }

      state = State::Idle;
      continue;
    }

    sToken = token.m_DataView;

    if (token.m_iType == xiiTokenType::NonIdentifier)
    {
      if (sToken == "=" && state == State::HasName)
      {
        state = State::HasEqual;
        continue;
      }
    }
    else if (token.m_iType == xiiTokenType::Identifier)
    {
      if (state == State::Idle)
      {
        sVarName = sToken;
        state    = State::HasName;
        continue;
      }

      if (state == State::HasEqual)
      {
        auto& res = out_fixedPermVars.ExpandAndGetRef();
        res.m_sName.Assign(sVarName.GetData());
        res.m_sValue.Assign(sToken.GetData());
        state = State::HasValue;
        continue;
      }
    }

    xiiLog::Error("Invalid permutation section at token '{0}'", sToken);
  }
}

// static
void xiiShaderParser::ParsePermutationVarConfig(xiiStringView s, xiiVariant& out_defaultValue, EnumDefinition& out_enumDefinition)
{
  SkipWhitespace(s);

  xiiStringBuilder name;

  if (s.StartsWith("bool"))
  {
    bool bDefaultValue = false;

    const char* szDefaultValue = s.FindSubString("=");
    if (szDefaultValue != nullptr)
    {
      name.SetSubString_FromTo(s.GetStartPointer() + 4, szDefaultValue);

      ++szDefaultValue;
      xiiConversionUtils::StringToBool(szDefaultValue, bDefaultValue).IgnoreResult();
    }
    else
    {
      name.SetSubString_FromTo(s.GetStartPointer() + 4, s.GetEndPointer());
    }

    name.Trim(" \t\r\n");
    out_enumDefinition.m_sName = name;
    out_defaultValue           = bDefaultValue;
  }
  else if (s.StartsWith("enum"))
  {
    xiiTokenizer tokenizer;
    tokenizer.Tokenize(xiiArrayPtr<const xiiUInt8>((const xiiUInt8*)s.GetStartPointer(), s.GetElementCount()), xiiLog::GetThreadLocalLogSystem());

    TokenStream tokens;
    tokenizer.GetAllLines(tokens);

    xiiUInt32 uiCurToken = 0;
    if (ParseEnum(tokens, uiCurToken, out_enumDefinition, true).Failed())
    {
      xiiLog::Error("Invalid enum PermutationVar definition.");
    }
    else
    {
      XII_ASSERT_DEV(!out_enumDefinition.m_sName.IsEmpty(), "");

      out_defaultValue = out_enumDefinition.m_uiDefaultValue;
    }
  }
  else
  {
    xiiLog::Error("Unknown permutation var type");
  }
}

xiiResult ParseResource(const TokenStream& tokens, xiiUInt32& ref_uiCurToken, xiiShaderResourceDefinition& out_resourceDefinition)
{
  // Match type
  xiiUInt32 uiTypeToken = ref_uiCurToken;
  if (!Accept(tokens, ref_uiCurToken, xiiTokenType::Identifier, &uiTypeToken))
  {
    return XII_FAILURE;
  }
  if (!s_NameToDescriptorTable.TryGetValue(tokens[uiTypeToken]->m_DataView, out_resourceDefinition.m_ResourceDescription.m_Type))
    return XII_FAILURE;
  s_NameToTextureTable.TryGetValue(tokens[uiTypeToken]->m_DataView, out_resourceDefinition.m_ResourceDescription.m_TextureType);

  // Skip optional template
  TokenMatch                   templatePattern[] = {"<"_xiisv, xiiTokenType::Identifier, ">"_xiisv};
  xiiHybridArray<xiiUInt32, 8> acceptedTokens;
  Accept(tokens, ref_uiCurToken, templatePattern, &acceptedTokens);

  // Match name
  xiiUInt32 uiNameToken = ref_uiCurToken;
  if (!Accept(tokens, ref_uiCurToken, xiiTokenType::Identifier, &uiNameToken))
  {
    return XII_FAILURE;
  }
  out_resourceDefinition.m_ResourceDescription.m_sName.Assign(tokens[uiNameToken]->m_DataView);
  xiiUInt32 uiEndToken = uiNameToken;

  // Match optional array
  TokenMatch arrayPattern[]    = {"["_xiisv, xiiTokenType::Integer, "]"_xiisv};
  TokenMatch bindlessPattern[] = {"["_xiisv, "]"_xiisv};
  if (Accept(tokens, ref_uiCurToken, arrayPattern, &acceptedTokens))
  {
    xiiConversionUtils::StringToUInt(tokens[acceptedTokens[1]]->m_DataView, out_resourceDefinition.m_ResourceDescription.m_uiArraySize).AssertSuccess("Tokenizer error");
    uiEndToken = acceptedTokens.PeekBack();
  }
  else if (Accept(tokens, ref_uiCurToken, bindlessPattern, &acceptedTokens))
  {
    out_resourceDefinition.m_ResourceDescription.m_uiArraySize = 0;
    uiEndToken                                                 = acceptedTokens.PeekBack();
  }
  out_resourceDefinition.m_sDeclaration = xiiStringView(tokens[uiTypeToken]->m_DataView.GetStartPointer(), tokens[uiEndToken]->m_DataView.GetEndPointer());

  // Match optional register
  TokenMatch slotPattern[]       = {":"_xiisv, "register"_xiisv, "("_xiisv, xiiTokenType::Identifier, ")"_xiisv};
  TokenMatch slotAndSetPattern[] = {":"_xiisv, "register"_xiisv, "("_xiisv, xiiTokenType::Identifier, ","_xiisv, xiiTokenType::Identifier, ")"_xiisv};
  if (Accept(tokens, ref_uiCurToken, slotPattern, &acceptedTokens))
  {
    xiiStringView sSlot = tokens[acceptedTokens[3]]->m_DataView;
    sSlot.Trim("tsubx");
    if (sSlot.IsEqual_NoCase("AUTO")) // See shader macros in StandardMacros.h
    {
      out_resourceDefinition.m_ResourceDescription.m_uiBindIndex = -1;
    }
    else
    {
      xiiInt32 iSlot;
      xiiConversionUtils::StringToInt(sSlot, iSlot).AssertSuccess("Failed to parse slot index of shader resource");
      out_resourceDefinition.m_ResourceDescription.m_uiBindIndex = iSlot;
    }
    uiEndToken = acceptedTokens.PeekBack();
  }
  else if (Accept(tokens, ref_uiCurToken, slotAndSetPattern, &acceptedTokens))
  {
    xiiStringView sSlot = tokens[acceptedTokens[3]]->m_DataView;
    sSlot.Trim("tsubx");
    if (sSlot.IsEqual_NoCase("AUTO")) // See shader macros in StandardMacros.h
    {
      out_resourceDefinition.m_ResourceDescription.m_uiBindIndex = xiiInvalidIndex;
    }
    else
    {
      xiiInt32 iSlot;
      xiiConversionUtils::StringToInt(sSlot, iSlot).AssertSuccess("Failed to parse slot index of shader resource");
      out_resourceDefinition.m_ResourceDescription.m_uiBindIndex = iSlot;
    }
    xiiStringView sSet = tokens[acceptedTokens[5]]->m_DataView;
    sSet.TrimWordStart("space"_xiisv);
    xiiInt32 iSet;
    xiiConversionUtils::StringToInt(sSet, iSet).AssertSuccess("Failed to parse set index of shader resource");
    out_resourceDefinition.m_ResourceDescription.m_uiDescriptorSet = iSet;
    uiEndToken                                                     = acceptedTokens.PeekBack();
  }

  out_resourceDefinition.m_sDeclarationAndRegister = xiiStringView(tokens[uiTypeToken]->m_DataView.GetStartPointer(), tokens[uiEndToken]->m_DataView.GetEndPointer());
  // Match ; (resource declaration done) or { (constant buffer member declaration starts)
  if (!Accept(tokens, ref_uiCurToken, ";"_xiisv) && !Accept(tokens, ref_uiCurToken, "{"_xiisv))
    return XII_FAILURE;

  return XII_SUCCESS;
}

void xiiShaderParser::ParseShaderResources(xiiStringView sShaderStageSource, xiiDynamicArray<xiiShaderResourceDefinition>& out_resources)
{
  if (sShaderStageSource.IsEmpty())
  {
    out_resources.Clear();
    return;
  }

  InitializeTables();

  xiiTokenizer tokenizer;
  tokenizer.SetTreatHashSignAsLineComment(true);
  tokenizer.Tokenize(xiiArrayPtr<const xiiUInt8>((const xiiUInt8*)sShaderStageSource.GetStartPointer(), sShaderStageSource.GetElementCount()), xiiLog::GetThreadLocalLogSystem(), false);

  TokenStream tokens;
  tokenizer.GetAllLines(tokens);

  xiiUInt32 uiCurToken = 0;

  while (!Accept(tokens, uiCurToken, xiiTokenType::EndOfFile))
  {
    xiiShaderResourceDefinition resourceDef;
    if (ParseResource(tokens, uiCurToken, resourceDef).Succeeded())
    {
      out_resources.PushBack(std::move(resourceDef));
      continue;
    }
    ++uiCurToken;
  }
}

xiiResult xiiShaderParser::MergeShaderResourceBindings(const xiiShaderProgramData& spd, xiiHashTable<xiiHashedString, xiiGALShaderResourceDescription>& out_bindings, xiiLogInterface* pLog)
{
  xiiUInt32 uiSize = 0;
  for (xiiUInt32 stage = xiiGALShaderType::GetStageIndex(xiiGALShaderType::Vertex); stage < xiiGALShaderType::ENUM_COUNT; ++stage)
  {
    uiSize += spd.m_Resources[stage].GetCount();
  }

  out_bindings.Clear();
  out_bindings.Reserve(uiSize);

  xiiMap<xiiHashedString, const xiiShaderResourceDefinition*> resourceFirstOccurence;

  for (xiiUInt32 stage = xiiGALShaderType::GetStageIndex(xiiGALShaderType::Vertex); stage < xiiGALShaderType::ENUM_COUNT; ++stage)
  {
    for (const xiiShaderResourceDefinition& res : spd.m_Resources[stage])
    {
      xiiHashedString sName = res.m_ResourceDescription.m_sName;
      auto            it    = out_bindings.Find(sName);
      if (it.IsValid())
      {
        xiiGALShaderResourceDescription& current = it.Value();
        if (current.m_Type != res.m_ResourceDescription.m_Type || current.m_TextureType != res.m_ResourceDescription.m_TextureType || current.m_uiArraySize != res.m_ResourceDescription.m_uiArraySize)
        {
          xiiLog::Error(pLog, "A shared shader resource '{}' has a mismatching signatures between stages: '{}' vs '{}'", sName, resourceFirstOccurence.Find(sName).Value()->m_sDeclarationAndRegister, res.m_sDeclarationAndRegister);
          return XII_FAILURE;
        }

        current.m_ShaderStages |= xiiGALShaderType::GetStageFlag(stage);
      }
      else
      {
        out_bindings.Insert(sName, res.m_ResourceDescription);
        resourceFirstOccurence.Insert(sName, &res);
        out_bindings.Find(sName).Value().m_ShaderStages |= xiiGALShaderType::GetStageFlag(stage);
      }
    }
  }
  return XII_SUCCESS;
}

xiiResult xiiShaderParser::SanityCheckShaderResourceBindings(const xiiHashTable<xiiHashedString, xiiGALShaderResourceDescription>& bindings, xiiLogInterface* pLog)
{
  for (auto it : bindings)
  {
    if (it.Value().m_uiDescriptorSet == xiiInvalidIndex)
    {
      xiiLog::Error(pLog, "Shader resource '{}' does not have a set defined.", it.Key());
      return XII_FAILURE;
    }
    if (it.Value().m_uiDescriptorSet == xiiInvalidIndex)
    {
      xiiLog::Error(pLog, "Shader resource '{}' does not have a slot defined.", it.Key());
      return XII_FAILURE;
    }
  }
  return XII_SUCCESS;
}

void xiiShaderParser::ApplyShaderResourceBindings(xiiStringView sPlatform, xiiStringView sShaderStageSource, const xiiDynamicArray<xiiShaderResourceDefinition>& resources, const xiiHashTable<xiiHashedString, xiiGALShaderResourceDescription>& bindings, const CreateResourceDeclaration& createDeclaration, xiiStringBuilder& out_sShaderStageSource)
{
  xiiDeque<xiiString>               partStorage;
  xiiHybridArray<xiiStringView, 16> parts;

  xiiStringBuilder sDeclaration;
  const char*      szStart = sShaderStageSource.GetStartPointer();
  for (xiiUInt32 i = 0; i < resources.GetCount(); ++i)
  {
    parts.PushBack(xiiStringView(szStart, resources[i].m_sDeclarationAndRegister.GetStartPointer()));

    xiiGALShaderResourceDescription* pBinding = nullptr;
    XII_ASSERT_DEV(bindings.TryGetValue(resources[i].m_ResourceDescription.m_sName, pBinding), "Every resource should be present in the map.");
    XII_ASSERT_DEV(pBinding->m_uiBindIndex != xiiInvalidIndex && pBinding->m_uiDescriptorSet != xiiInvalidIndex, "Unbound shader resource binding found: '{}', slot: {}, set: {}", pBinding->m_sName, pBinding->m_uiBindIndex, pBinding->m_uiDescriptorSet);

    createDeclaration(sPlatform, resources[i].m_sDeclaration, *pBinding, sDeclaration);

    xiiString& sStorage = partStorage.ExpandAndGetRef();
    sStorage            = sDeclaration;
    parts.PushBack(sStorage);
    szStart = resources[i].m_sDeclarationAndRegister.GetEndPointer();
  }
  parts.PushBack(xiiStringView(szStart, sShaderStageSource.GetEndPointer()));

  xiiUInt32 uiSize = 0;
  for (const xiiStringView& sPart : parts)
    uiSize += sPart.GetElementCount();

  out_sShaderStageSource.Clear();
  out_sShaderStageSource.Reserve(uiSize);

  for (const xiiStringView& sPart : parts)
  {
    out_sShaderStageSource.Append(sPart);
  }
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_ShaderCompiler_Implementation_ShaderParser);

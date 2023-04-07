#include <RendererCore/RendererCorePCH.h>

#include <Foundation/CodeUtils/TokenParseUtils.h>
#include <Foundation/CodeUtils/Tokenizer.h>
#include <Foundation/Types/Variant.h>
#include <Foundation/Utilities/ConversionUtils.h>
#include <RendererCore/Shader/Implementation/Helper.h>
#include <RendererCore/ShaderCompiler/ShaderManager.h>
#include <RendererCore/ShaderCompiler/ShaderParser.h>

using namespace xiiTokenParseUtils;

namespace
{
  static xiiHashTable<xiiStringView, const xiiRTTI*> s_NameToTypeTable;

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
  }

  const xiiRTTI* GetType(xiiStringView sType)
  {
    InitializeTables();

    const xiiRTTI* pType = nullptr;
    s_NameToTypeTable.TryGetValue(sType, pType);
    return pType;
  }

  xiiVariant ParseValue(const TokenStream& Tokens, xiiUInt32& uiCurToken)
  {
    xiiUInt32 uiValueToken = uiCurToken;

    if (Accept(Tokens, uiCurToken, xiiTokenType::String1, &uiValueToken) || Accept(Tokens, uiCurToken, xiiTokenType::String2, &uiValueToken))
    {
      xiiStringBuilder sValue = Tokens[uiValueToken]->m_DataView;
      sValue.Trim("\"'");

      return xiiVariant(sValue.GetData());
    }

    if (Accept(Tokens, uiCurToken, xiiTokenType::Integer, &uiValueToken))
    {
      xiiString sValue = Tokens[uiValueToken]->m_DataView;

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

    if (Accept(Tokens, uiCurToken, xiiTokenType::Float, &uiValueToken))
    {
      xiiString sValue = Tokens[uiValueToken]->m_DataView;

      double fValue = 0;
      xiiConversionUtils::StringToFloat(sValue, fValue).IgnoreResult();

      return xiiVariant(fValue);
    }

    if (Accept(Tokens, uiCurToken, "true", &uiValueToken) || Accept(Tokens, uiCurToken, "false", &uiValueToken))
    {
      bool bValue = Tokens[uiValueToken]->m_DataView == "true";
      return xiiVariant(bValue);
    }

    auto& dataView = Tokens[uiCurToken]->m_DataView;
    if (Tokens[uiCurToken]->m_iType == xiiTokenType::Identifier && xiiStringUtils::IsValidIdentifierName(dataView.GetStartPointer(), dataView.GetEndPointer()))
    {
      // complex type constructor
      const xiiRTTI* pType = nullptr;
      if (!s_NameToTypeTable.TryGetValue(dataView, pType))
      {
        xiiLog::Error("Invalid type name '{}'", dataView);
        return xiiVariant();
      }

      ++uiCurToken;
      Accept(Tokens, uiCurToken, "(");

      xiiHybridArray<xiiVariant, 8> constructorArgs;

      while (!Accept(Tokens, uiCurToken, ")"))
      {
        xiiVariant value = ParseValue(Tokens, uiCurToken);
        if (value.IsValid())
        {
          constructorArgs.PushBack(value);
        }
        else
        {
          xiiLog::Error("Invalid arguments for constructor '{}'", pType->GetTypeName());
          return XII_FAILURE;
        }

        Accept(Tokens, uiCurToken, ",");
      }

      // find matching constructor
      auto& functions = pType->GetFunctions();
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

  xiiResult ParseAttribute(const TokenStream& Tokens, xiiUInt32& uiCurToken, xiiShaderParser::ParameterDefinition& out_ParameterDefinition)
  {
    if (!Accept(Tokens, uiCurToken, "@"))
    {
      return XII_FAILURE;
    }

    xiiUInt32 uiTypeToken = uiCurToken;
    if (!Accept(Tokens, uiCurToken, xiiTokenType::Identifier, &uiTypeToken))
    {
      return XII_FAILURE;
    }

    xiiShaderParser::AttributeDefinition& attributeDef = out_ParameterDefinition.m_Attributes.ExpandAndGetRef();
    attributeDef.m_sType                               = Tokens[uiTypeToken]->m_DataView;

    Accept(Tokens, uiCurToken, "(");

    while (!Accept(Tokens, uiCurToken, ")"))
    {
      xiiVariant value = ParseValue(Tokens, uiCurToken);
      if (value.IsValid())
      {
        attributeDef.m_Values.PushBack(value);
      }
      else
      {
        xiiLog::Error("Invalid arguments for attribute '{}'", attributeDef.m_sType);
        return XII_FAILURE;
      }

      Accept(Tokens, uiCurToken, ",");
    }

    return XII_SUCCESS;
  }

  xiiResult ParseParameter(const TokenStream& Tokens, xiiUInt32& uiCurToken, xiiShaderParser::ParameterDefinition& out_ParameterDefinition)
  {
    xiiUInt32 uiTypeToken = uiCurToken;
    if (!Accept(Tokens, uiCurToken, xiiTokenType::Identifier, &uiTypeToken))
    {
      return XII_FAILURE;
    }

    out_ParameterDefinition.m_sType = Tokens[uiTypeToken]->m_DataView;
    out_ParameterDefinition.m_pType = GetType(out_ParameterDefinition.m_sType);

    xiiUInt32 uiNameToken = uiCurToken;
    if (!Accept(Tokens, uiCurToken, xiiTokenType::Identifier, &uiNameToken))
    {
      return XII_FAILURE;
    }

    out_ParameterDefinition.m_sName = Tokens[uiNameToken]->m_DataView;

    while (!Accept(Tokens, uiCurToken, ";"))
    {
      if (ParseAttribute(Tokens, uiCurToken, out_ParameterDefinition).Failed())
      {
        return XII_FAILURE;
      }
    }

    return XII_SUCCESS;
  }

  xiiResult ParseEnum(const TokenStream& Tokens, xiiUInt32& uiCurToken, xiiShaderParser::EnumDefinition& out_EnumDefinition, bool bCheckPrefix)
  {
    if (!Accept(Tokens, uiCurToken, "enum"))
    {
      return XII_FAILURE;
    }

    xiiUInt32 uiNameToken = uiCurToken;
    if (!Accept(Tokens, uiCurToken, xiiTokenType::Identifier, &uiNameToken))
    {
      return XII_FAILURE;
    }

    out_EnumDefinition.m_sName = Tokens[uiNameToken]->m_DataView;
    xiiStringBuilder sEnumPrefix(out_EnumDefinition.m_sName, "_");

    if (!Accept(Tokens, uiCurToken, "{"))
    {
      xiiLog::Error("Opening bracket expected for enum definition.");
      return XII_FAILURE;
    }

    xiiUInt32 uiDefaultValue = 0;
    xiiUInt32 uiCurrentValue = 0;

    while (true)
    {
      xiiUInt32 uiValueNameToken = uiCurToken;
      if (!Accept(Tokens, uiCurToken, xiiTokenType::Identifier, &uiValueNameToken))
      {
        return XII_FAILURE;
      }

      xiiStringView sValueName = Tokens[uiValueNameToken]->m_DataView;

      if (Accept(Tokens, uiCurToken, "="))
      {
        xiiUInt32 uiValueToken = uiCurToken;
        Accept(Tokens, uiCurToken, xiiTokenType::Integer, &uiValueToken);

        xiiInt32 iValue = 0;
        if (xiiConversionUtils::StringToInt(Tokens[uiValueToken]->m_DataView.GetStartPointer(), iValue).Succeeded() && iValue >= 0)
        {
          uiCurrentValue = iValue;
        }
        else
        {
          xiiLog::Error("Invalid enum value '{0}'. Only positive numbers are allowed.", Tokens[uiValueToken]->m_DataView);
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

        auto& ev = out_EnumDefinition.m_Values.ExpandAndGetRef();

        const xiiStringBuilder sFinalName = sValueName;
        ev.m_sValueName.Assign(sFinalName.GetData());
        ev.m_iValueValue = static_cast<xiiInt32>(uiCurrentValue);
      }

      if (Accept(Tokens, uiCurToken, ","))
      {
        ++uiCurrentValue;
      }
      else
      {
        break;
      }

      if (Accept(Tokens, uiCurToken, "}"))
        goto after_braces;
    }

    if (!Accept(Tokens, uiCurToken, "}"))
    {
      xiiLog::Error("Closing bracket expected for enum definition.");
      return XII_FAILURE;
    }

  after_braces:

    out_EnumDefinition.m_uiDefaultValue = uiDefaultValue;

    Accept(Tokens, uiCurToken, ";");

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
void xiiShaderParser::ParseMaterialParameterSection(xiiStreamReader& stream, xiiHybridArray<ParameterDefinition, 16>& out_Parameter, xiiHybridArray<EnumDefinition, 4>& out_EnumDefinitions)
{
  xiiString sContent;
  sContent.ReadAll(stream);

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

      out_EnumDefinitions.PushBack(std::move(enumDef));
      continue;
    }

    ParameterDefinition paramDef;
    if (ParseParameter(tokens, uiCurToken, paramDef).Succeeded())
    {
      out_Parameter.PushBack(std::move(paramDef));
      continue;
    }

    xiiLog::Error("Invalid token in material parameter section '{}'", tokens[uiCurToken]->m_DataView);
    break;
  }
}

// static
void xiiShaderParser::ParsePermutationSection(xiiStreamReader& stream, xiiHybridArray<xiiHashedString, 16>& out_PermVars, xiiHybridArray<xiiPermutationVar, 16>& out_FixedPermVars)
{
  xiiString sContent;
  sContent.ReadAll(stream);

  xiiShaderHelper::xiiTextSectionizer Sections;
  xiiShaderHelper::GetShaderSections(sContent.GetData(), Sections);

  xiiUInt32     uiFirstLine   = 0;
  xiiStringView sPermutations = Sections.GetSectionContent(xiiShaderHelper::xiiShaderSections::PERMUTATIONS, uiFirstLine);
  ParsePermutationSection(sPermutations, out_PermVars, out_FixedPermVars);
}

// static
void xiiShaderParser::ParsePermutationSection(xiiStringView s, xiiHybridArray<xiiHashedString, 16>& out_PermVars, xiiHybridArray<xiiPermutationVar, 16>& out_FixedPermVars)
{
  out_PermVars.Clear();
  out_FixedPermVars.Clear();

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

    if (token.m_iType == xiiTokenType::String1 || token.m_iType == xiiTokenType::String2)
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
        out_PermVars.ExpandAndGetRef().Assign(sVarName.GetData());
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
        auto& res = out_FixedPermVars.ExpandAndGetRef();
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
void xiiShaderParser::ParsePermutationVarConfig(xiiStringView s, xiiVariant& out_DefaultValue, EnumDefinition& out_EnumDefinition)
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
    out_EnumDefinition.m_sName = name;
    out_DefaultValue           = bDefaultValue;
  }
  else if (s.StartsWith("enum"))
  {
    xiiTokenizer tokenizer;
    tokenizer.Tokenize(xiiArrayPtr<const xiiUInt8>((const xiiUInt8*)s.GetStartPointer(), s.GetElementCount()), xiiLog::GetThreadLocalLogSystem());

    TokenStream tokens;
    tokenizer.GetAllLines(tokens);

    xiiUInt32 uiCurToken = 0;
    if (ParseEnum(tokens, uiCurToken, out_EnumDefinition, true).Failed())
    {
      xiiLog::Error("Invalid enum PermutationVar definition.");
    }
    else
    {
      XII_ASSERT_DEV(!out_EnumDefinition.m_sName.IsEmpty(), "");

      out_DefaultValue = out_EnumDefinition.m_uiDefaultValue;
    }
  }
  else
  {
    xiiLog::Error("Unknown permutation var type");
  }
}

XII_STATICLINK_FILE(RendererCore, RendererCore_ShaderCompiler_Implementation_ShaderParser);

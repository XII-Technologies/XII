#pragma once

#include <GraphicsFoundation/GraphicsFoundationDLL.h>

#include <GraphicsFoundation/Shader/ShaderResourceVariable.h>

// clang-format off

XII_BEGIN_STATIC_REFLECTED_ENUM(xiiGALShaderResourceVariableType, 1)
  XII_ENUM_CONSTANT(xiiGALShaderResourceVariableType::Static),
  XII_ENUM_CONSTANT(xiiGALShaderResourceVariableType::Mutable),
  XII_ENUM_CONSTANT(xiiGALShaderResourceVariableType::Dynamic),
XII_END_STATIC_REFLECTED_ENUM;

XII_BEGIN_STATIC_REFLECTED_ENUM(xiiGALShaderResourceVariableTypeFlags, 1)
  XII_ENUM_CONSTANT(xiiGALShaderResourceVariableTypeFlags::Static),
  XII_ENUM_CONSTANT(xiiGALShaderResourceVariableTypeFlags::Mutable),
  XII_ENUM_CONSTANT(xiiGALShaderResourceVariableTypeFlags::Dynamic),
  XII_ENUM_CONSTANT(xiiGALShaderResourceVariableTypeFlags::MutableDynamic),
  XII_ENUM_CONSTANT(xiiGALShaderResourceVariableTypeFlags::All),
XII_END_STATIC_REFLECTED_ENUM;

XII_BEGIN_STATIC_REFLECTED_ENUM(xiiGALShaderResourceBindFlags, 1)
  XII_ENUM_CONSTANT(xiiGALShaderResourceBindFlags::Static),
  XII_ENUM_CONSTANT(xiiGALShaderResourceBindFlags::Mutable),
  XII_ENUM_CONSTANT(xiiGALShaderResourceBindFlags::Dynamic),
  XII_ENUM_CONSTANT(xiiGALShaderResourceBindFlags::All),
  XII_ENUM_CONSTANT(xiiGALShaderResourceBindFlags::KeepExisting),
  XII_ENUM_CONSTANT(xiiGALShaderResourceBindFlags::VerifyAllResolved),
  XII_ENUM_CONSTANT(xiiGALShaderResourceBindFlags::AllowOverWrite),
XII_END_STATIC_REFLECTED_ENUM;

XII_BEGIN_STATIC_REFLECTED_ENUM(xiiGALSetShaderResourceFlags, 1)
  XII_ENUM_CONSTANT(xiiGALSetShaderResourceFlags::None),
  XII_ENUM_CONSTANT(xiiGALSetShaderResourceFlags::AllowOverwrite),
XII_END_STATIC_REFLECTED_ENUM;

// clang-format on

xiiGALShaderResourceVariable::xiiGALShaderResourceVariable(const xiiGALShaderResourceVariableCreationDescription& creationDescription) :
  xiiGALObject<xiiGALShaderResourceVariableCreationDescription>(creationDescription)
{
}

xiiGALShaderResourceVariable::~xiiGALShaderResourceVariable() = default;

XII_STATICLINK_FILE(GraphicsFoundation, GraphicsFoundation_Shader_Implementation_ShaderResourceVariable);

#pragma once

#include <GraphicsFoundation/GraphicsFoundationDLL.h>

class XII_GRAPHICSFOUNDATION_DLL xiiGALShaderTextSectionizer
{
public:
  void Clear();

  void AddSection(xiiStringView sName);

  void Process(xiiStringView sText);

  xiiStringView GetSectionContent(xiiUInt32 uiSection, xiiUInt32& out_uiFirstLine) const;

private:
  struct xiiTextSection
  {
    xiiTextSection(xiiStringView sName) :
      m_sName(sName)
    {
    }

    void Reset()
    {
      m_szSectionStart = nullptr;
      m_sContent       = xiiStringView();
      m_uiFirstLine    = 0;
    }

    xiiString     m_sName;
    const char*   m_szSectionStart = nullptr;
    xiiStringView m_sContent;
    xiiUInt32     m_uiFirstLine = 0;
  };

  xiiStringBuilder                   m_sText;
  xiiHybridArray<xiiTextSection, 16> m_Sections;
};

struct XII_GRAPHICSFOUNDATION_DLL xiiGALShaderSections
{
  using StorageType = xiiUInt8;

  enum Enum : StorageType
  {
    Platforms = 0U,
    Permutations,
    MaterialConstants,
    MaterialParameter,
    MaterialConfiguration,
    RenderState,
    Shader,
    VertexShader,
    PixelShader,
    GeometryShader,
    HullShader,
    DomainShader,
    ComputeShader,
    AmplificationShader,
    MeshShader,
    RayGenerationShader,
    RayMissShader,
    RayClosestHitShader,
    RayAnyHitShader,
    RayIntersectionShader,
    CallableShader,
    TileShader,
    TemplateVariables,

    ENUM_COUNT,

    Default = Platforms
  };

  static void GetShaderSections(xiiStringView sContent, xiiGALShaderTextSectionizer& out_sections);
};

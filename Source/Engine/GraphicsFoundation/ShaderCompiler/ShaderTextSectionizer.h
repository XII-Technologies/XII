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
    PLATFORMS,
    PERMUTATIONS,
    MATERIALPARAMETER,
    MATERIALCONFIG,
    RENDERSTATE,
    SHADER,
    VERTEXSHADER,
    PIXELSHADER,
    GEOMETRYSHADER,
    HULLSHADER,
    DOMAINSHADER,
    COMPUTESHADER,
    AMPLIFICATIONSHADER,
    MESHSHADER,
    RAYGENERATIONSHADER,
    RAYMISSSHADER,
    RAYCLOSESTHITSHADER,
    RAYANYHITSHADER,
    RAYINTERSECTIONSHADER,
    CALLABLESHADER,
    TILESHADER,
    TEMPLATE_VARS
  };

  static void GetShaderSections(xiiStringView sContent, xiiGALShaderTextSectionizer& out_sections);
};

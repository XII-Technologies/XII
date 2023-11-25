#pragma once

#include <Foundation/Strings/String.h>
#include <GraphicsCore/Declarations.h>

namespace xiiShaderHelper
{
  class XII_GRAPHICSCORE_DLL xiiTextSectionizer
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
        m_Content        = xiiStringView();
        m_uiFirstLine    = 0;
      }

      xiiString     m_sName;
      const char*   m_szSectionStart = nullptr;
      xiiStringView m_Content;
      xiiUInt32     m_uiFirstLine = 0;
    };

    xiiStringBuilder                   m_sText;
    xiiHybridArray<xiiTextSection, 16> m_Sections;
  };

  struct xiiShaderSections
  {
    enum Enum
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
  };

  XII_GRAPHICSCORE_DLL void GetShaderSections(xiiStringView sContent, xiiTextSectionizer& out_sections);

  xiiUInt32 CalculateHash(const xiiArrayPtr<xiiPermutationVar>& vars);
} // namespace xiiShaderHelper

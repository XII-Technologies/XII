/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <EditorFramework/Assets/SimpleAssetDocument.h>
#include <Texture/TexConv/TexConvEnums.h>
#include <ToolsFoundation/Object/DocumentObjectBase.h>

struct xiiPropertyMetaStateEvent;

struct xiiTextureCubeChannelMappingEnum
{
  using StorageType = xiiInt8;

  enum Enum
  {
    RGB1,
    RGBA1,

    RGB1TO6,
    RGBA1TO6,

    Default = RGB1,
  };
};

XII_DECLARE_REFLECTABLE_TYPE(XII_NO_LINKAGE, xiiTextureCubeChannelMappingEnum);


class xiiTextureCubeAssetProperties : public xiiReflectedClass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiTextureCubeAssetProperties, xiiReflectedClass);

public:
  static void PropertyMetaStateEventHandler(xiiPropertyMetaStateEvent& e);

  const char* GetInputFile(xiiInt32 iInput) const { return m_Input[iInput]; }

  void        SetInputFile0(const char* szFile) { m_Input[0] = szFile; }
  const char* GetInputFile0() const { return m_Input[0]; }
  void        SetInputFile1(const char* szFile) { m_Input[1] = szFile; }
  const char* GetInputFile1() const { return m_Input[1]; }
  void        SetInputFile2(const char* szFile) { m_Input[2] = szFile; }
  const char* GetInputFile2() const { return m_Input[2]; }
  void        SetInputFile3(const char* szFile) { m_Input[3] = szFile; }
  const char* GetInputFile3() const { return m_Input[3]; }
  void        SetInputFile4(const char* szFile) { m_Input[4] = szFile; }
  const char* GetInputFile4() const { return m_Input[4]; }
  void        SetInputFile5(const char* szFile) { m_Input[5] = szFile; }
  const char* GetInputFile5() const { return m_Input[5]; }

  xiiString GetAbsoluteInputFilePath(xiiInt32 iInput) const;
  xiiInt32  GetNumInputFiles() const;

  xiiEnum<xiiTexConvCompressionMode> m_CompressionMode;
  xiiEnum<xiiTexConvMipmapMode>      m_MipmapMode;

  xiiEnum<xiiTextureFilterSetting>          m_TextureFilter;
  xiiEnum<xiiTextureConverterUsage>         m_TextureUsage;
  xiiEnum<xiiTextureCubeChannelMappingEnum> m_ChannelMapping;

  float m_fHdrExposureBias = 0;

private:
  xiiString m_Input[6];
};

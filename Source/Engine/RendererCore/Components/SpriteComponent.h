#pragma once

#include <Core/ResourceManager/ResourceHandle.h>
#include <Core/World/World.h>
#include <RendererCore/Components/RenderComponent.h>
#include <RendererCore/Pipeline/RenderData.h>

struct xiiMsgSetColor;
using xiiTexture2DResourceHandle = xiiTypedResourceHandle<class xiiTexture2DResource>;

struct xiiSpriteBlendMode
{
  typedef xiiUInt8 StorageType;

  enum Enum
  {
    Masked,
    Transparent,
    Additive,

    Default = Masked
  };

  static xiiTempHashedString GetPermutationValue(Enum blendMode);
};

XII_DECLARE_REFLECTABLE_TYPE(XII_RENDERERCORE_DLL, xiiSpriteBlendMode);

class XII_RENDERERCORE_DLL xiiSpriteRenderData : public xiiRenderData
{
  XII_ADD_DYNAMIC_REFLECTION(xiiSpriteRenderData, xiiRenderData);

public:
  void FillBatchIdAndSortingKey();

  xiiTexture2DResourceHandle m_hTexture;

  float                       m_fSize;
  float                       m_fMaxScreenSize;
  float                       m_fAspectRatio;
  xiiEnum<xiiSpriteBlendMode> m_BlendMode;

  xiiColor m_color;

  xiiVec2 m_texCoordScale;
  xiiVec2 m_texCoordOffset;

  xiiUInt32 m_uiUniqueID;
};

typedef xiiComponentManager<class xiiSpriteComponent, xiiBlockStorageType::Compact> xiiSpriteComponentManager;

class XII_RENDERERCORE_DLL xiiSpriteComponent : public xiiRenderComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiSpriteComponent, xiiRenderComponent, xiiSpriteComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // xiiComponent

public:
  virtual void SerializeComponent(xiiWorldWriter& stream) const override;
  virtual void DeserializeComponent(xiiWorldReader& stream) override;


  //////////////////////////////////////////////////////////////////////////
  // xiiRenderComponent

public:
  virtual xiiResult GetLocalBounds(xiiBoundingBoxSphere& bounds, bool& bAlwaysVisible, xiiMsgUpdateLocalBounds& msg) override;


  //////////////////////////////////////////////////////////////////////////
  // xiiSpriteComponent

public:
  xiiSpriteComponent();
  ~xiiSpriteComponent();

  void                              SetTexture(const xiiTexture2DResourceHandle& hTexture);
  const xiiTexture2DResourceHandle& GetTexture() const;

  void        SetTextureFile(const char* szFile); // [ property ]
  const char* GetTextureFile() const;             // [ property ]

  void     SetColor(xiiColor color); // [ property ]
  xiiColor GetColor() const;         // [ property ]

  void  SetSize(float fSize); // [ property ]
  float GetSize() const;      // [ property ]

  void  SetMaxScreenSize(float fSize); // [ property ]
  float GetMaxScreenSize() const;      // [ property ]

  void OnMsgSetColor(xiiMsgSetColor& msg); // [ property ]

private:
  void OnMsgExtractRenderData(xiiMsgExtractRenderData& msg) const;

  xiiTexture2DResourceHandle  m_hTexture;
  xiiEnum<xiiSpriteBlendMode> m_BlendMode;
  xiiColor                    m_Color = xiiColor::White;

  float m_fSize          = 1.0f;
  float m_fMaxScreenSize = 64.0f;
  float m_fAspectRatio   = 1.0f;
};

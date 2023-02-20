#pragma once

#include <Core/World/World.h>
#include <Foundation/Math/Float16.h>
#include <Foundation/Types/TagSet.h>
#include <ProcGenPlugin/Declarations.h>

using xiiImageDataResourceHandle = xiiTypedResourceHandle<class xiiImageDataResource>;

class XII_PROCGENPLUGIN_DLL xiiVolumeCollection : public xiiReflectedClass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiVolumeCollection, xiiReflectedClass);

public:
  struct ShapeType
  {
    typedef xiiUInt8 StorageType;

    enum Enum
    {
      Sphere,
      Box,
      Image,

      Default = Sphere
    };
  };

  struct Shape
  {
    xiiVec4                      m_GlobalToLocalTransform0;
    xiiVec4                      m_GlobalToLocalTransform1;
    xiiVec4                      m_GlobalToLocalTransform2;
    xiiEnum<ShapeType>           m_Type;
    xiiEnum<xiiProcGenBlendMode> m_BlendMode;
    xiiFloat16                   m_fValue;
    xiiUInt32                    m_uiSortingKey;

    XII_ALWAYS_INLINE bool operator<(const Shape& other) const { return m_uiSortingKey < other.m_uiSortingKey; }

    void         SetGlobalToLocalTransform(const xiiSimdMat4f& t);
    xiiSimdMat4f GetGlobalToLocalTransform() const;
  };

  struct Sphere : public Shape
  {
    float m_fFadeOutScale;
    float m_fFadeOutBias;
  };

  struct Box : public Shape
  {
    xiiVec3 m_vFadeOutScale;
    xiiVec3 m_vFadeOutBias;
  };

  struct Image : public Box
  {
    xiiImageDataResourceHandle m_Image;
    const xiiColor*            m_pPixelData    = nullptr;
    xiiUInt32                  m_uiImageWidth  = 0;
    xiiUInt32                  m_uiImageHeight = 0;
  };

  bool IsEmpty() { return m_Spheres.IsEmpty() && m_Boxes.IsEmpty(); }

  static xiiUInt32 ComputeSortingKey(float fSortOrder, float fMaxScale);

  float EvaluateAtGlobalPosition(const xiiSimdVec4f& vPosition, float fInitialValue, xiiProcVolumeImageMode::Enum imgMode, const xiiColor& refColor) const;

  static void ExtractVolumesInBox(const xiiWorld& world, const xiiBoundingBox& box, xiiSpatialData::Category spatialCategory, const xiiTagSet& includeTags, xiiVolumeCollection& out_Collection, const xiiRTTI* pComponentBaseType = nullptr);

  void AddSphere(const xiiSimdTransform& transform, float fRadius, xiiEnum<xiiProcGenBlendMode> blendMode, float fSortOrder, float fValue, float fFadeOutStart);

  void AddBox(const xiiSimdTransform& transform, const xiiVec3& vExtents, xiiEnum<xiiProcGenBlendMode> blendMode, float fSortOrder, float fValue, const xiiVec3& vFadeOutStart);

  void AddImage(const xiiSimdTransform& transform, const xiiVec3& vExtents, xiiEnum<xiiProcGenBlendMode> blendMode, float fSortOrder, float fValue, const xiiVec3& vFadeOutStart, const xiiImageDataResourceHandle& image);

private:
  xiiDynamicArray<Sphere, xiiAlignedAllocatorWrapper> m_Spheres;
  xiiDynamicArray<Box, xiiAlignedAllocatorWrapper>    m_Boxes;
  xiiDynamicArray<Image, xiiAlignedAllocatorWrapper>  m_Images;

  xiiDynamicArray<const Shape*> m_SortedShapes;
};

struct XII_PROCGENPLUGIN_DLL xiiMsgExtractVolumes : public xiiMessage
{
  XII_DECLARE_MESSAGE_TYPE(xiiMsgExtractVolumes, xiiMessage);

  xiiVolumeCollection* m_pCollection = nullptr;
};

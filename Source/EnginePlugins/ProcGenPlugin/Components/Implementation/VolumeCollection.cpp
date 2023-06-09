#include <ProcGenPlugin/ProcGenPluginPCH.h>

#include <GameEngine/Utils/ImageDataResource.h>
#include <ProcGenPlugin/Components/VolumeCollection.h>
#include <Texture/Image/ImageUtils.h>

namespace
{
  XII_FORCE_INLINE float ApplyValue(xiiProcGenBlendMode::Enum blendMode, float fInitialValue, float fNewValue)
  {
    switch (blendMode)
    {
      case xiiProcGenBlendMode::Add:
        return fInitialValue + fNewValue;
      case xiiProcGenBlendMode::Subtract:
        return fInitialValue - fNewValue;
      case xiiProcGenBlendMode::Multiply:
        return fInitialValue * fNewValue;
      case xiiProcGenBlendMode::Divide:
        return fInitialValue / fNewValue;
      case xiiProcGenBlendMode::Max:
        return xiiMath::Max(fInitialValue, fNewValue);
      case xiiProcGenBlendMode::Min:
        return xiiMath::Min(fInitialValue, fNewValue);
      case xiiProcGenBlendMode::Set:
        return fNewValue;
      default:
        return fInitialValue;
    }
  }
} // namespace

XII_CHECK_AT_COMPILETIME(sizeof(xiiVolumeCollection::Sphere) == 64);
XII_CHECK_AT_COMPILETIME(sizeof(xiiVolumeCollection::Box) == 80);

void xiiVolumeCollection::Shape::SetGlobalToLocalTransform(const xiiSimdMat4f& t)
{
  xiiSimdVec4f r0, r1, r2, r3;
  t.GetRows(r0, r1, r2, r3);

  m_GlobalToLocalTransform0 = xiiSimdConversion::ToVec4(r0);
  m_GlobalToLocalTransform1 = xiiSimdConversion::ToVec4(r1);
  m_GlobalToLocalTransform2 = xiiSimdConversion::ToVec4(r2);
}

xiiSimdMat4f xiiVolumeCollection::Shape::GetGlobalToLocalTransform() const
{
  xiiSimdMat4f m;
  m.SetRows(xiiSimdConversion::ToVec4(m_GlobalToLocalTransform0), xiiSimdConversion::ToVec4(m_GlobalToLocalTransform1),
            xiiSimdConversion::ToVec4(m_GlobalToLocalTransform2), xiiSimdVec4f(0, 0, 0, 1));

  return m;
}

//////////////////////////////////////////////////////////////////////////

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiVolumeCollection, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

// static
xiiUInt32 xiiVolumeCollection::ComputeSortingKey(float fSortOrder, float fMaxScale)
{
  xiiUInt32 uiSortingKey = (xiiUInt32)(xiiMath::Min(fSortOrder * 512.0f, 32767.0f) + 32768.0f);
  uiSortingKey           = (uiSortingKey << 16) | (0xFFFF - ((xiiUInt32)(fMaxScale * 100.0f) & 0xFFFF));
  return uiSortingKey;
}

float xiiVolumeCollection::EvaluateAtGlobalPosition(const xiiSimdVec4f& vPosition, float fInitialValue, xiiProcVolumeImageMode::Enum imgMode, const xiiColor& refColor) const
{
  float fValue = fInitialValue;

  for (auto pShape : m_SortedShapes)
  {
    if (pShape->m_Type == ShapeType::Sphere)
    {
      auto&              sphere      = *static_cast<const Sphere*>(pShape);
      const xiiSimdVec4f localPos    = sphere.GetGlobalToLocalTransform().TransformPosition(vPosition);
      const float        distSquared = localPos.GetLengthSquared<3>();
      if (distSquared <= 1.0f)
      {
        const float fNewValue = ApplyValue(sphere.m_BlendMode, fValue, sphere.m_fValue);
        const float fAlpha    = xiiMath::Saturate(xiiMath::Sqrt(distSquared) * sphere.m_fFadeOutScale + sphere.m_fFadeOutBias);
        fValue                = xiiMath::Lerp(fValue, fNewValue, fAlpha);
      }
    }
    else if (pShape->m_Type == ShapeType::Box)
    {
      auto&              box         = *static_cast<const Box*>(pShape);
      const xiiSimdVec4f absLocalPos = box.GetGlobalToLocalTransform().TransformPosition(vPosition).Abs();
      if ((absLocalPos <= xiiSimdVec4f(1.0f)).AllSet<3>())
      {
        const float  fNewValue = ApplyValue(box.m_BlendMode, fValue, box.m_fValue);
        xiiSimdVec4f vAlpha    = absLocalPos.CompMul(xiiSimdConversion::ToVec3(box.m_vFadeOutScale)) + xiiSimdConversion::ToVec3(box.m_vFadeOutBias);
        vAlpha                 = vAlpha.CompMin(xiiSimdVec4f(1.0f)).CompMax(xiiSimdVec4f::ZeroVector());
        const float fAlpha     = vAlpha.x() * vAlpha.y() * vAlpha.z();
        fValue                 = xiiMath::Lerp(fValue, fNewValue, fAlpha);
      }
    }
    else if (pShape->m_Type == ShapeType::Image)
    {
      auto& image = *static_cast<const Image*>(pShape);

      const xiiSimdVec4f localPos    = image.GetGlobalToLocalTransform().TransformPosition(vPosition);
      const xiiSimdVec4f absLocalPos = localPos.Abs();

      if ((absLocalPos <= xiiSimdVec4f(1.0f)).AllSet<3>() && image.m_pPixelData != nullptr)
      {
        xiiVec2 uv;
        uv.x = static_cast<float>(localPos.x()) * 0.5f + 0.5f;
        uv.y = static_cast<float>(localPos.y()) * 0.5f + 0.5f;

        const xiiColor col = xiiImageUtils::NearestSample(image.m_pPixelData, image.m_uiImageWidth, image.m_uiImageHeight, xiiImageAddressMode::Clamp, uv);

        float fValueToUse = image.m_fValue;

        switch (imgMode)
        {
          case xiiProcVolumeImageMode::ReferenceColor:
            fValueToUse = image.m_fValue;
            break;
          case xiiProcVolumeImageMode::ChannelR:
            fValueToUse = image.m_fValue * col.r;
            break;
          case xiiProcVolumeImageMode::ChannelG:
            fValueToUse = image.m_fValue * col.g;
            break;
          case xiiProcVolumeImageMode::ChannelB:
            fValueToUse = image.m_fValue * col.b;
            break;
          case xiiProcVolumeImageMode::ChannelA:
            fValueToUse = image.m_fValue * col.a;
            break;
        }

        if (imgMode != xiiProcVolumeImageMode::ReferenceColor || col.IsEqualRGBA(refColor, 0.1f))
        {
          const float  fNewValue = ApplyValue(image.m_BlendMode, fValue, fValueToUse);
          xiiSimdVec4f vAlpha    = absLocalPos.CompMul(xiiSimdConversion::ToVec3(image.m_vFadeOutScale)) + xiiSimdConversion::ToVec3(image.m_vFadeOutBias);
          vAlpha                 = vAlpha.CompMin(xiiSimdVec4f(1.0f)).CompMax(xiiSimdVec4f::ZeroVector());
          const float fAlpha     = vAlpha.x() * vAlpha.y() * vAlpha.z();
          fValue                 = xiiMath::Lerp(fValue, fNewValue, fAlpha);
        }
      }
    }
  }

  return fValue;
}

// static
void xiiVolumeCollection::ExtractVolumesInBox(const xiiWorld& world, const xiiBoundingBox& box, xiiSpatialData::Category spatialCategory, const xiiTagSet& includeTags, xiiVolumeCollection& out_collection, const xiiRTTI* pComponentBaseType)
{
  xiiMsgExtractVolumes msg;
  msg.m_pCollection = &out_collection;

  xiiSpatialSystem::QueryParams queryParams;
  queryParams.m_uiCategoryBitmask = spatialCategory.GetBitmask();
  queryParams.m_IncludeTags       = includeTags;

  world.GetSpatialSystem()->FindObjectsInBox(box, queryParams, [&](xiiGameObject* pObject) {
    if (pComponentBaseType != nullptr)
    {
      xiiHybridArray<const xiiComponent*, 8> components;
      pObject->TryGetComponentsOfBaseType(pComponentBaseType, components);

      for (auto pComponent : components)
      {
        pComponent->SendMessage(msg);
      }
    }
    else
    {
      pObject->SendMessage(msg);
    }

    return xiiVisitorExecution::Continue;
  });

  out_collection.m_Spheres.Sort();
  out_collection.m_Boxes.Sort();
  out_collection.m_Images.Sort();

  const xiiUInt32 uiNumSpheres = out_collection.m_Spheres.GetCount();
  const xiiUInt32 uiNumBoxes   = out_collection.m_Boxes.GetCount();
  const xiiUInt32 uiNumImages  = out_collection.m_Images.GetCount();

  out_collection.m_SortedShapes.Reserve(uiNumSpheres + uiNumBoxes + uiNumImages);

  xiiUInt32 uiCurrentSphere = 0;
  xiiUInt32 uiCurrentBox    = 0;
  xiiUInt32 uiCurrentImage  = 0;

  while (uiCurrentSphere < uiNumSpheres || uiCurrentBox < uiNumBoxes || uiCurrentImage < uiNumImages)
  {
    Sphere* pSphere = uiCurrentSphere < uiNumSpheres ? &out_collection.m_Spheres[uiCurrentSphere] : nullptr;
    Box*    pBox    = uiCurrentBox < uiNumBoxes ? &out_collection.m_Boxes[uiCurrentBox] : nullptr;
    Image*  pImage  = uiCurrentImage < uiNumImages ? &out_collection.m_Images[uiCurrentImage] : nullptr;

    Shape*    pSmallestShape = nullptr;
    xiiUInt32 uiSmallestKey  = 0xFFFFFFFF;

    if (pSphere && pSphere->m_uiSortingKey < uiSmallestKey)
    {
      pSmallestShape = pSphere;
      uiSmallestKey  = pSmallestShape->m_uiSortingKey;
    }

    if (pBox && pBox->m_uiSortingKey < uiSmallestKey)
    {
      pSmallestShape = pBox;
      uiSmallestKey  = pSmallestShape->m_uiSortingKey;
    }

    if (pImage && pImage->m_uiSortingKey < uiSmallestKey)
    {
      pSmallestShape = pImage;
      uiSmallestKey  = pSmallestShape->m_uiSortingKey;
    }

    XII_ASSERT_DEBUG(pSmallestShape != nullptr, "Error sorting proc-gen volumes.");

    out_collection.m_SortedShapes.PushBack(pSmallestShape);

    if (pSmallestShape == pSphere)
    {
      ++uiCurrentSphere;
    }
    else if (pSmallestShape == pBox)
    {
      ++uiCurrentBox;
    }
    else if (pSmallestShape == pImage)
    {
      ++uiCurrentImage;
    }
  }
}

void xiiVolumeCollection::AddSphere(const xiiSimdTransform& transform, float fRadius, xiiEnum<xiiProcGenBlendMode> blendMode, float fSortOrder, float fValue, float fFadeOutStart)
{
  xiiSimdTransform scaledTransform = transform;
  scaledTransform.m_Scale *= fRadius;

  auto& sphere = m_Spheres.ExpandAndGetRef();
  sphere.SetGlobalToLocalTransform(scaledTransform.GetAsMat4().GetInverse());
  sphere.m_Type          = ShapeType::Sphere;
  sphere.m_BlendMode     = blendMode;
  sphere.m_fValue        = fValue;
  sphere.m_uiSortingKey  = xiiVolumeCollection::ComputeSortingKey(fSortOrder, scaledTransform.GetMaxScale());
  sphere.m_fFadeOutScale = -1.0f / xiiMath::Max(1.0f - fFadeOutStart, 0.0001f);
  sphere.m_fFadeOutBias  = -sphere.m_fFadeOutScale;
}

void xiiVolumeCollection::AddBox(const xiiSimdTransform& transform, const xiiVec3& vExtents, xiiEnum<xiiProcGenBlendMode> blendMode, float fSortOrder, float fValue, const xiiVec3& vFadeOutStart)
{
  xiiSimdTransform scaledTransform = transform;
  scaledTransform.m_Scale          = scaledTransform.m_Scale.CompMul(xiiSimdConversion::ToVec3(vExtents)) * 0.5f;

  auto& box = m_Boxes.ExpandAndGetRef();
  box.SetGlobalToLocalTransform(scaledTransform.GetAsMat4().GetInverse());
  box.m_Type          = ShapeType::Box;
  box.m_BlendMode     = blendMode;
  box.m_fValue        = fValue;
  box.m_uiSortingKey  = xiiVolumeCollection::ComputeSortingKey(fSortOrder, scaledTransform.GetMaxScale());
  box.m_vFadeOutScale = xiiVec3(-1.0f).CompDiv((xiiVec3(1.0f) - vFadeOutStart).CompMax(xiiVec3(0.0001f)));
  box.m_vFadeOutBias  = -box.m_vFadeOutScale;
}

void xiiVolumeCollection::AddImage(const xiiSimdTransform& transform, const xiiVec3& vExtents, xiiEnum<xiiProcGenBlendMode> blendMode, float fSortOrder, float fValue, const xiiVec3& vFadeOutStart, const xiiImageDataResourceHandle& hImage)
{
  xiiSimdTransform scaledTransform = transform;
  scaledTransform.m_Scale          = scaledTransform.m_Scale.CompMul(xiiSimdConversion::ToVec3(vExtents)) * 0.5f;

  auto& shape = m_Images.ExpandAndGetRef();
  shape.SetGlobalToLocalTransform(scaledTransform.GetAsMat4().GetInverse());
  shape.m_Type          = ShapeType::Image;
  shape.m_BlendMode     = blendMode;
  shape.m_fValue        = fValue;
  shape.m_uiSortingKey  = xiiVolumeCollection::ComputeSortingKey(fSortOrder, scaledTransform.GetMaxScale());
  shape.m_vFadeOutScale = xiiVec3(-1.0f).CompDiv((xiiVec3(1.0f) - vFadeOutStart).CompMax(xiiVec3(0.0001f)));
  shape.m_vFadeOutBias  = -shape.m_vFadeOutScale;

  shape.m_Image = hImage;

  if (shape.m_Image.IsValid())
  {
    xiiResourceLock<xiiImageDataResource> pImage(shape.m_Image, xiiResourceAcquireMode::BlockTillLoaded);
    shape.m_pPixelData    = pImage->GetDescriptor().m_Image.GetPixelPointer<xiiColor>();
    shape.m_uiImageWidth  = pImage->GetDescriptor().m_Image.GetWidth();
    shape.m_uiImageHeight = pImage->GetDescriptor().m_Image.GetHeight();
  }
}

//////////////////////////////////////////////////////////////////////////

XII_IMPLEMENT_MESSAGE_TYPE(xiiMsgExtractVolumes);
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiMsgExtractVolumes, 1, xiiRTTIDefaultAllocator<xiiMsgExtractVolumes>)
XII_END_DYNAMIC_REFLECTED_TYPE;

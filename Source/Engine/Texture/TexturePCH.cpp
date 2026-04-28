/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Texture/TexturePCH.h>

XII_STATICLINK_LIBRARY(Texture)
{
  if (bReturn)
    return;

  XII_STATICLINK_REFERENCE(Texture_Image_Conversions_BC7EncConversions);
  XII_STATICLINK_REFERENCE(Texture_Image_Conversions_DXTConversions);
  XII_STATICLINK_REFERENCE(Texture_Image_Conversions_DXTexConversions);
  XII_STATICLINK_REFERENCE(Texture_Image_Conversions_DXTexCpuConversions);
  XII_STATICLINK_REFERENCE(Texture_Image_Conversions_PixelConversions);
  XII_STATICLINK_REFERENCE(Texture_Image_Conversions_PlanarConversions);
  XII_STATICLINK_REFERENCE(Texture_Image_Formats_BmpFileFormat);
  XII_STATICLINK_REFERENCE(Texture_Image_Formats_DdsFileFormat);
  XII_STATICLINK_REFERENCE(Texture_Image_Formats_ExrFileFormat);
  XII_STATICLINK_REFERENCE(Texture_Image_Formats_StbImageFileFormats);
  XII_STATICLINK_REFERENCE(Texture_Image_Formats_TgaFileFormat);
  XII_STATICLINK_REFERENCE(Texture_Image_Formats_WicFileFormat);
  XII_STATICLINK_REFERENCE(Texture_Image_Implementation_ImageEnums);
  XII_STATICLINK_REFERENCE(Texture_Image_Implementation_ImageFormat);
  XII_STATICLINK_REFERENCE(Texture_TexConv_Implementation_Processor);
}

#pragma once

#include <EditorFramework/Assets/SimpleAssetDocument.h>
#include <ToolsFoundation/Object/DocumentObjectBase.h>

class xiiImageDataAssetProperties : public xiiReflectedClass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiImageDataAssetProperties, xiiReflectedClass);

public:
  xiiString m_sInputFile;

  // TODO: more xiiImageData options
  // * maximum resolution
  // * 1, 2, 3, 4 channels
  // * compression: lossy (jpg), lossless (png), uncompressed
  // * HDR data ?
  // * combine from multiple images (channel mapping)
};

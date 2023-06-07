#pragma once

#include <Foundation/IO/MemoryStream.h>
#include <Foundation/IO/Stream.h>
#include <RendererCore/RendererCoreDLL.h>
#include <ozz/base/io/stream.h>

namespace ozz::animation
{
  class Skeleton;
  class Animation;
}; // namespace ozz::animation

/// \brief Stores or gather the data for an ozz file, for random access operations (seek / tell).
///
/// Since ozz::io::Stream requires seek/tell functionality, it cannot be implemented with basic xiiStreamReader / xiiStreamWriter.
/// Instead, we must have the entire ozz archive data in memory, to be able to jump around arbitrarily.
class XII_RENDERERCORE_DLL xiiOzzArchiveData
{
  XII_DISALLOW_COPY_AND_ASSIGN(xiiOzzArchiveData);

public:
  xiiOzzArchiveData();
  ~xiiOzzArchiveData();

  xiiResult FetchRegularFile(const char* szFile);
  xiiResult FetchEmbeddedArchive(xiiStreamReader& ref_stream);
  xiiResult StoreEmbeddedArchive(xiiStreamWriter& ref_stream) const;

  xiiDefaultMemoryStreamStorage m_Storage;
};

/// \brief Implements the ozz::io::Stream interface for reading. The data has to be present in an xiiOzzArchiveData object.
///
/// The class is implemented inline and not DLL exported because ozz is only available as a static library.
class XII_RENDERERCORE_DLL xiiOzzStreamReader : public ozz::io::Stream
{
public:
  xiiOzzStreamReader(const xiiOzzArchiveData& data);

  virtual bool opened() const override;

  virtual size_t Read(void* p_buffer, size_t ui_size) override;

  virtual size_t Write(const void* p_buffer, size_t ui_size) override;

  virtual int Seek(int i_offset, Origin _origin) override;

  virtual int Tell() const override;

  virtual size_t Size() const override;

private:
  xiiMemoryStreamReader m_Reader;
};

/// \brief Implements the ozz::io::Stream interface for writing. The data is gathered in an xiiOzzArchiveData object.
///
/// The class is implemented inline and not DLL exported because ozz is only available as a static library.
class XII_RENDERERCORE_DLL xiiOzzStreamWriter : public ozz::io::Stream
{
public:
  xiiOzzStreamWriter(xiiOzzArchiveData& ref_data);

  virtual bool opened() const override;

  virtual size_t Read(void* p_buffer, size_t ui_size) override;

  virtual size_t Write(const void* p_buffer, size_t ui_size) override;

  virtual int Seek(int i_offset, Origin _origin) override;

  virtual int Tell() const override;

  virtual size_t Size() const override;

private:
  xiiMemoryStreamWriter m_Writer;
};

namespace xiiOzzUtils
{
  XII_RENDERERCORE_DLL void CopyAnimation(ozz::animation::Animation* pDst, const ozz::animation::Animation* pSrc);
  XII_RENDERERCORE_DLL void CopySkeleton(ozz::animation::Skeleton* pDst, const ozz::animation::Skeleton* pSrc);
} // namespace xiiOzzUtils

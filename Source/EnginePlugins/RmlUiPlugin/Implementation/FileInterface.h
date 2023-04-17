#pragma once

#include <Foundation/IO/MemoryStream.h>
#include <Foundation/Types/UniquePtr.h>

#include <Foundation/Containers/IdTable.h>
#include <RmlUi/Core/FileInterface.h>

namespace xiiRmlUiInternal
{
  struct FileId : public xiiGenericId<24, 8>
  {
    using xiiGenericId::xiiGenericId;

    static FileId FromRml(Rml::FileHandle file) { return FileId(static_cast<xiiUInt32>(file)); }

    Rml::FileHandle ToRml() const { return m_Data; }
  };

  //////////////////////////////////////////////////////////////////////////

  class FileInterface final : public Rml::FileInterface
  {
  public:
    FileInterface();
    virtual ~FileInterface();

    virtual Rml::FileHandle Open(const Rml::String& sPath) override;
    virtual void            Close(Rml::FileHandle file) override;

    virtual size_t Read(void* pBuffer, size_t uiSize, Rml::FileHandle file) override;

    virtual bool   Seek(Rml::FileHandle file, long iOffset, int iOrigin) override;
    virtual size_t Tell(Rml::FileHandle file) override;

    virtual size_t Length(Rml::FileHandle file) override;

  private:
    struct OpenFile
    {
      xiiDefaultMemoryStreamStorage m_Storage;
      xiiMemoryStreamReader         m_Reader;
    };

    xiiIdTable<FileId, xiiUniquePtr<OpenFile>> m_OpenFiles;
  };
} // namespace xiiRmlUiInternal

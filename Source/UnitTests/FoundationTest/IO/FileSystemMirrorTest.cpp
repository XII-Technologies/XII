/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <FoundationTest/FoundationTestPCH.h>

#if XII_ENABLED(XII_SUPPORTS_FILE_ITERATORS)
#  include <Foundation/IO/Implementation/FileSystemMirror.h>
#  include <Foundation/Types/ScopeExit.h>

XII_CREATE_SIMPLE_TEST(IO, FileSystemMirror)
{
  xiiStringBuilder root(xiiTestFramework::GetInstance()->GetAbsOutputPath(), "/MirrorHierarchy");
  XII_TEST_RESULT(xiiOSFile::DeleteFolder(root));
  XII_TEST_RESULT(xiiOSFile::CreateDirectoryStructure(root));
  XII_SCOPE_EXIT(xiiOSFile::DeleteFolder(root).IgnoreResult());
  xiiFileSystemMirror<bool>       mirror;
  xiiStringBuilder                unknown;
  xiiFileSystemMirror<bool>::Type unknownType;
  XII_TEST_BOOL(mirror.GetPathCasing("", unknown, unknownType).Failed());
  XII_TEST_RESULT(mirror.AddDirectory(root));
  xiiStringBuilder parent(root, "/Mixed"), nested(parent, "/Nested"), sibling(root, "/Sibling"), file(nested, "/File.txt");
  XII_TEST_RESULT(mirror.AddDirectory(nested));
  XII_TEST_RESULT(mirror.AddDirectory(sibling));
  XII_TEST_RESULT(mirror.AddFile(file, true, nullptr, nullptr));
  xiiFileSystemMirror<bool>::Type type;
  XII_TEST_RESULT(mirror.GetType(nested, type));
  XII_TEST_BOOL(type == xiiFileSystemMirror<bool>::Type::Directory);
  xiiDynamicArray<xiiString> paths;
  XII_TEST_RESULT(mirror.Enumerate(root, [&](const xiiStringBuilder& path, xiiFileSystemMirror<bool>::Type) { paths.PushBack(path); }));
  XII_TEST_INT(paths.GetCount(), 4);
  XII_TEST_STRING(paths[0], file);
  XII_TEST_STRING(paths[1], nested);
  XII_TEST_STRING(paths[2], parent);
  XII_TEST_STRING(paths[3], sibling);
  xiiStringBuilder resolved;
  XII_TEST_RESULT(mirror.GetPathCasing(xiiStringBuilder(root, "/MIXED/NESTED/file.TXT"), resolved, type));
  XII_TEST_STRING(resolved, file);
  XII_TEST_BOOL(type == xiiFileSystemMirror<bool>::Type::File);
  XII_TEST_RESULT(mirror.GetPathCasing(xiiStringBuilder(root, "/mixed/nested/"), resolved, type));
  XII_TEST_STRING(resolved, nested);
  XII_TEST_BOOL(type == xiiFileSystemMirror<bool>::Type::Directory);
  XII_TEST_BOOL(mirror.GetPathCasing(xiiStringBuilder(root, "/Missing"), resolved, type).Failed());
  XII_TEST_BOOL(mirror.GetPathCasing(xiiStringBuilder(root, "Other/Mixed"), resolved, type).Failed());
  xiiStringBuilder renamed(nested, "/FILE.txt");
  XII_TEST_RESULT(mirror.MoveFile(file, renamed));
  XII_TEST_BOOL(mirror.GetType(file, type).Failed());
  XII_TEST_RESULT(mirror.GetType(renamed, type));
  bool existed = false, value = false;
  XII_TEST_RESULT(mirror.AddFile(renamed, false, &existed, &value));
  XII_TEST_BOOL(existed && value);
  XII_TEST_RESULT(mirror.MoveFile(renamed, renamed));
  XII_TEST_BOOL(mirror.MoveFile(file, renamed).Failed());
}


#endif

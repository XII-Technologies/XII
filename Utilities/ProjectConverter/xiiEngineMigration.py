"""
Converts the EZ namespace to the XII namespace.

Copyright (c) Theophilus Eriata. All rights reserved.
"""
import os
import sys
import time
import shutil
import string
import logging
import pathlib
import datetime
import argparse
import typing as t

########## Initialize Logger ##########

logger = logging.getLogger(__name__)
logger.addHandler(logging.NullHandler())

def GetLoggingLevel(target: str) -> int:
    """Returns the current logging level from string format."""
    if target not in logging._nameToLevel:
        return None
    return logging._nameToLevel[target]

#######################################

def GetResourcePath(*args) -> str:
    """
    Retrieves the full path to a resource given its relative path.
    """
    sPath: str = os.path.abspath(os.path.join(os.path.dirname(os.path.abspath(__file__)), '..'))
    if args:
        sPath = os.path.join(sPath, *args)
    return sPath

def SanitizeTextualData(sText: str) -> str:
    """
    Sanitizes a given string to be rid of new lines and trailing spaces.
    """
    # Clear trailing spaces on both ends of the text.
    sText = sText.strip()

    # Clear new lines from the text.
    lines: t.List[str] = sText.splitlines()
    while len(lines) > 1:
        sText  = "".join(lines)
        lines = sText.splitlines()

    return sText

def ScanDirectory(sDirectoryPath: str, extensions: t.List[str] = None) -> t.Tuple[t.List[str], t.List[str]]:
     """
     Scan sDirectoryPath and return all subfolders recursively.

     \param sDirectoryPath - The root directory to start the scan.
     \param extenstions    - A list of extensions to include in search results.
     """
     # https://stackoverflow.com/questions/18394147/how-to-do-a-recursive-sub-folder-search-and-return-files-in-a-list/59803793#59803793
     subfolders, files = [], []

     for path in os.scandir(sDirectoryPath):
         if path.is_dir():
            subfolders.append(path.path)
         if path.is_file():
            if not extensions:
                files.append(path.path)
            else:
                for ext in extensions:
                    if os.path.splitext(path.name)[1] == ext:
                        files.append(path.path)

     for sSubFolder in subfolders:
         recursedSubFolders, recursedFiles = ScanDirectory(sSubFolder, extensions)
         subfolders.extend(recursedSubFolders)
         files.extend(recursedFiles)

     return subfolders, files

class InstanceData:
    sSourcePath:         str = None
    bShouldRenameFiles: bool = True

    xiiExtensionsNoRename: set = set()
    xiiExtensionsNoEdit: set   = { ".jpg", ".png", ".svg", ".dds", ".pdn" }


def ResolveFileNames() -> None:
    if not InstanceData.bShouldRenameFiles:
        return

    subfolders, files = ScanDirectory(InstanceData.sSourcePath)
    # for subfolder in subfolders:
    #     logger.debug(subfolder)

    filesToRename: set = set()
    for file in files:
        # logger.debug(file)
        sFileName: str = os.path.basename(file)
        if sFileName.startswith('ez'):
            sExtension: str = os.path.splitext(sFileName)[1]

            if len(sExtension) > 0 and sExtension in InstanceData.xiiExtensionsNoRename:
                continue

            filesToRename.add(file)

    logger.info(f"Renaming {len(filesToRename)} files.")
    for file in filesToRename:
        sFileName: str = os.path.basename(file)
        sFileName      = 'xii' + sFileName.removeprefix('ez')
        sFilePath: str = os.path.join(os.path.dirname(file), sFileName)

        logger.info(f"--  {file} -> {sFilePath}")

        try:
            os.rename(file, sFilePath)
        except Exception as e:
            logger.error(f"Failed to rename file: {e}")


def main() -> int:
    parser = argparse.ArgumentParser(description="XII Migration Tool.")

    sEnginePath: str = "C:\\Development\\XII-Technologies\\XII-Migration\\Source\\Engine\\Foundation"
    parser.add_argument("--source",            type=str, default=sEnginePath, help="The full path to the engine source files. This does not have to be the engine source directory.")
    parser.add_argument("--consolelog", "-cl", type=int, default=1,           help="Should log to console instead of a file. Default is 1, use 0 to log to file.")
    parser.add_argument("--loglevel",   "-lv", type=str, default="DEBUG",     help="The logging level to use.", choices={"DEBUG", "INFO", "WARNING", "ERROR"})

    args = parser.parse_args()

    # Initialize Logger.
    if args.consolelog == 0:
        # Ensure that file is overwritten with 'w'.
        logging.basicConfig(level=GetLoggingLevel(args.loglevel), filename=GetResourcePath("Log.txt"), filemode="w")
    else:
        logging.basicConfig(level=GetLoggingLevel(args.loglevel))

    logger.info("XII Migration Tool")
    logger.info("Copyright (c) 2024 Theophilus Eriata. All Rights Reserved")
    logger.info("Please that all project files are backed up before using this tool.")

    InstanceData.sSourcePath = sEnginePath

    # First resolve file names.
    ResolveFileNames()

    return 0

if __name__ == '__main__':
    sys.exit(main())

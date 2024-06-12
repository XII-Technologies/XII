"""
Converts the EZ namespace to the XII namespace.

Copyright (c) Theophilus Eriata. All rights reserved.
"""
import os
import sys
import re
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

    xiiExtensionsNoRename: t.Set[str]    = set()
    xiiExtensionsNoEdit: t.Set[str]      = { ".jpg", ".png", ".svg", ".dds", ".pdn", ".ico" }
    xiiInvariantPunctuations: t.Set[str] = {'!', '"', '#', '$', '%', '&', "'", '(', ')', '*', '+', ',', '-', '.', '/', ':', ';', '<', '=', '>', '?', '@', '[', '\\', ']', '^', '_', '`', '{', '|', '}', '~'}

    xiiDebugMacroRegex: t.Set[str] = {
        r"XII_ASSERT_(DEBUG|DEV|RELEASE|ALWAYS)\($",
        r"[a-zA-Z0-9]+::+[a-zA-Z0-9_]+\($"
    }
    xiiIgnoreMacroRegex: t.Set[str] = {
        r"#define+\s+[a-zA-Z]+",
        r"^XII_BEGIN_DYNAMIC_REFLECTED_TYPE\($",
        r"^XII_END_DYNAMIC_REFLECTED_TYPE\($",
        r"XII_BEGIN_PROPERTIES",
        r"XII_END_PROPERTIES",
        r"XII_(ENUM|ARRAY|)_MEMBER_PROPERTY\($",
    }


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

def GetResolvedLineContent(sLineText: str) -> str:
    """
    Returns a the sLineText with modifications to move it into the XII namespace if needed.
    """
    # Perform rudimentary processing.
    sLineContent: str = sLineText
    sLineContent      = sLineContent.replace('\t', '  ')
    sLineContent      = sLineContent.replace(' an ez', ' a xii')
    sLineContent      = sLineContent.replace(' ez',  ' xii')
    sLineContent      = sLineContent.replace(' Ez',  ' XII')
    sLineContent      = sLineContent.replace(' EZ',  ' XII')
    sLineContent      = sLineContent.replace(' ez ', ' XII ')
    sLineContent      = sLineContent.replace(' Ez ', ' XII ')
    sLineContent      = sLineContent.replace(' eZ ', ' XII ')
    sLineContent      = sLineContent.replace(' EZ ', ' XII ')

    # Find namespace conversions hidden in punctuations.
    for sPunctuation in InstanceData.xiiInvariantPunctuations:
        sLineContent = sLineContent.replace(f"{sPunctuation}ez", f"{sPunctuation}xii")
        sLineContent = sLineContent.replace(f"{sPunctuation}Ez", f"{sPunctuation}XII")
        sLineContent = sLineContent.replace(f"{sPunctuation}EZ", f"{sPunctuation}XII")

    # Regex namespace conversions that can occur at the start of the file.
    if (rSearch := re.match(r"^ez", sLineContent)) and rSearch != None:
        sLineContent = 'xii' + sLineContent.removeprefix('ez')
    if (rSearch := re.match(r"^Ez", sLineContent)) and rSearch != None:
        sLineContent = 'XII' + sLineContent.removeprefix('Ez')
    if (rSearch := re.match(r"^EZ", sLineContent)) and rSearch != None:
        sLineContent = 'XII' + sLineContent.removeprefix('EZ')
    if (rSearch := re.match(r"^\s+ez", sLineContent)) and rSearch != None:
        uiIndex: int = sLineContent.find('ez')

        assert uiIndex != -1, "Implementation error. Index is -1, but had a successful regex match!"

        sLineContent = sLineContent[:uiIndex] + 'xii' + sLineContent[uiIndex:].removeprefix('ez')
    if (rSearch := re.match(r"^\s+Ez", sLineContent)) and rSearch != None:
        uiIndex: int = sLineContent.find('Ez')

        assert uiIndex != -1, "Implementation error. Index is -1, but had a successful regex match!"

        sLineContent = sLineContent[:uiIndex] + 'XII' + sLineContent[uiIndex:].removeprefix('Ez')
    if (rSearch := re.match(r"^\s+EZ", sLineContent)) and rSearch != None:
        uiIndex: int = sLineContent.find('EZ')

        assert uiIndex != -1, "Implementation error. Index is -1, but had a successful regex match!"

        sLineContent = sLineContent[:uiIndex] + 'XII' + sLineContent[uiIndex:].removeprefix('EZ')

    # TODO: Remove references to xiiEngine as a result of the transformation.
    # sLineContent = sLineContent.replace('xiiEngine', 'XII')

    return sLineContent

def ApplyCodeFormatRules(lineData: t.List[str]) -> t.Tuple[str, int]:
    """
    Returns a tuple containing string containing the formatted text, and the offset from the start of the buffer that was used.
    """
    pass

def ResolveNamespace() -> None:
    """
    Resolves the namespaces. That is, the ez -> xii conversion.

    The following conversion holds:
    - ez -> xii
    - Ez -> XII
    - EZ -> XII

    Steps are taken to ensure that:
    - Words like breeze, freeze, etc. do not get modified.
    - Words like 'an ezType' should be 'a xiiType'.
    - Methods should have all definitions on the same line.
    """
    subfolders, files = ScanDirectory(InstanceData.sSourcePath)
    for file in files:
        try:
            #logger.info(f"Transforming file: {file}")

            sExtension: str = os.path.splitext(file)[1]

            if len(sExtension) > 0 and sExtension in InstanceData.xiiExtensionsNoEdit:
                continue

            # Read file content, we will perform processing outside the file reading.
            fileContent: t.List[str] = []
            with open(file, mode='r') as filestream:
                for line in filestream:
                    fileContent.append(line)

            # Perform processing.
            processedFileContent: t.List[str] = []
            uiCurrentLine: int                = 0
            uiNextLinesToSkip: int            = 0
            for sLine in fileContent:
                if uiNextLinesToSkip > 0:
                    uiNextLinesToSkip -= 1
                    continue

                sStrippedLine: str = sLine.rstrip()

                # We do not act on empty lines, clang-format may restrict how many empty lines are possible.
                if len(sStrippedLine) == 0:
                    processedFileContent.append('\n')
                    uiCurrentLine += 1
                    continue

                # Perform rudimentary processing.
                sLineContent: str = GetResolvedLineContent(sStrippedLine)

                # Cleanup multiline function arguments, clang-format does not clean them up.
                if (rSearch := re.match(r"^[a-zA-Z0-9]+\s[a-zA-Z0-9]+::[a-zA-Z0-9_]+\($", sLineContent)) and rSearch != None:
                    sLineParse: str = sLineContent

                    uiIndexAdvancement: int = 1
                    while (uiCurrentLine + uiIndexAdvancement) < len(fileContent):
                        sProcessedLine: str = GetResolvedLineContent(fileContent[uiCurrentLine + uiIndexAdvancement].strip())

                        if len(sProcessedLine) == 0:
                            uiIndexAdvancement += 1
                            continue

                        sLineParse += sProcessedLine

                        if sProcessedLine.endswith('{'):
                            sLineContent       = sLineParse
                            uiCurrentLine     += uiIndexAdvancement
                            uiNextLinesToSkip += uiIndexAdvancement
                            logger.info(f"Applied code format rules: {sLineContent}")
                            break

                        # Max depth
                        if uiIndexAdvancement > 50:
                            logger.info(f"Failed to apply code format rules: {sLineContent}")
                            break

                        uiIndexAdvancement += 1

                # Cleanup multiline function arguments, clang-format does not clean them up.
                if (rSearch := re.search(r"^[a-zA-Z0-9]+\s+[a-zA-Z0-9_]+\($", sLineContent)) and rSearch != None and False:
                    sLineParse: str = sLineContent

                    uiIndexAdvancement: int = 1
                    while (uiCurrentLine + uiIndexAdvancement) < len(fileContent):
                        sProcessedLine: str = GetResolvedLineContent(fileContent[uiCurrentLine + uiIndexAdvancement].strip())

                        if len(sProcessedLine) == 0:
                            uiIndexAdvancement += 1
                            continue

                        sLineParse += sProcessedLine

                        if sProcessedLine.endswith('(') or sProcessedLine.endswith('{') or sProcessedLine.endswith(';') or sProcessedLine.endswith('[tested]'):
                            sLineContent       = sLineParse
                            uiCurrentLine     += uiIndexAdvancement
                            uiNextLinesToSkip += uiIndexAdvancement
                            logger.info(f"Applied code format rules: {sLineContent}")
                            break

                        # Max depth
                        if uiIndexAdvancement > 50:
                            logger.info(f"Failed to apply code format rules: {sLineContent}")
                            break

                        uiIndexAdvancement += 1

                # Cleanup multiline function arguments, clang-format does not clean them up.
                # We do not apply code format rules on comments. (Block comments are unhandled)
                if (rSearch := re.search(r"^[a-zA-Z0-9]+\s+[a-zA-Z0-9_]+\(+([a-zA-Z0-9\s<>&*:,.\(\)])+$", sLineContent)) and rSearch != None and False and not sLineContent.strip().startswith("//") and not sLineContent.strip().startswith("/*") and not sLineContent.strip().startswith("*") and not sLineContent.strip().startswith("#"):
                    sLineParse: str = sLineContent

                    uiIndexAdvancement: int = 1
                    while (uiCurrentLine + uiIndexAdvancement) < len(fileContent):
                        sProcessedLine: str = GetResolvedLineContent(fileContent[uiCurrentLine + uiIndexAdvancement].strip())

                        # Empty lines are ambiguous at the moment.
                        if len(sProcessedLine) == 0:
                            break

                        # We do not apply code format rules on comments. (Block comments are unhandled)
                        if sProcessedLine.strip().startswith("//"):
                            break

                        sLineParse += sProcessedLine

                        if sProcessedLine.endswith('(') or sProcessedLine.endswith('{') or sProcessedLine.endswith(';') or sProcessedLine.endswith('[tested]'):
                            sLineContent       = sLineParse
                            uiCurrentLine     += uiIndexAdvancement
                            uiNextLinesToSkip += uiIndexAdvancement
                            logger.info(f"Applied code format rules: {sLineContent}")
                            break

                        # Max depth
                        if uiIndexAdvancement > 50:
                            logger.info(f"Failed to apply code format rules: {sLineContent}")
                            break

                        uiIndexAdvancement += 1

                # Cleanup multiline function arguments, clang-format does not clean them up.
                # We do not apply code format rules on comments. (Block comments are unhandled)
                if (rSearch := re.search(r"^[a-zA-Z0-9]+::+[a-zA-Z0-9_]+\($", sLineContent)) and rSearch != None and False and not sLineContent.strip().startswith("//") and not sLineContent.strip().startswith("/*") and not sLineContent.strip().startswith("*") and not sLineContent.strip().startswith("#"):
                    sLineParse: str = sLineContent

                    uiIndexAdvancement: int = 1
                    while (uiCurrentLine + uiIndexAdvancement) < len(fileContent):
                        sProcessedLine: str = GetResolvedLineContent(fileContent[uiCurrentLine + uiIndexAdvancement].strip())

                        # Empty lines are ambiguous at the moment.
                        if len(sProcessedLine) == 0:
                            break

                        # We do not apply code format rules on comments. (Block comments are unhandled)
                        if sProcessedLine.strip().startswith("//"):
                            break

                        sLineParse += sProcessedLine

                        if sProcessedLine.endswith('(') or sProcessedLine.endswith('{') or sProcessedLine.endswith(';') or sProcessedLine.endswith('[tested]'):
                            sLineContent       = sLineParse
                            uiCurrentLine     += uiIndexAdvancement
                            uiNextLinesToSkip += uiIndexAdvancement
                            logger.info(f"Applied code format rules: {sLineContent}")
                            break

                        # Max depth
                        if uiIndexAdvancement > 50:
                            logger.info(f"Failed to apply code format rules: {sLineContent}")
                            break

                        uiIndexAdvancement += 1

                # Cleanup multiline function arguments, clang-format does not clean them up.
                # We do not apply code format rules on comments. (Block comments are unhandled)
                if (rSearch := re.search(r"XII_ASSERT_(DEBUG|DEV|RELEASE|ALWAYS)\($", sLineContent)) and rSearch != None and False and not sLineContent.strip().startswith("//") and not sLineContent.strip().startswith("/*") and not sLineContent.strip().startswith("*") and not sLineContent.strip().startswith("#"):
                    sLineParse: str = sLineContent

                    uiIndexAdvancement: int = 1
                    while (uiCurrentLine + uiIndexAdvancement) < len(fileContent):
                        sProcessedLine: str = GetResolvedLineContent(fileContent[uiCurrentLine + uiIndexAdvancement].strip())

                        # Empty lines are ambiguous at the moment.
                        if len(sProcessedLine) == 0:
                            break

                        # We do not apply code format rules on comments. (Block comments are unhandled)
                        if sProcessedLine.strip().startswith("//"):
                            break

                        sLineParse += sProcessedLine

                        if sProcessedLine.endswith(';'):
                            sLineContent       = sLineParse
                            uiCurrentLine     += uiIndexAdvancement
                            uiNextLinesToSkip += uiIndexAdvancement
                            logger.info(f"Applied code format rules: {sLineContent}")
                            break

                        # Max depth
                        if uiIndexAdvancement > 50:
                            logger.info(f"Failed to apply code format rules: {sLineContent}")
                            break

                        uiIndexAdvancement += 1

                sLineContent += '\n'

                processedFileContent.append(sLineContent)

                uiCurrentLine += 1

            # Write file contents back into file.
            with open(file, mode='w') as filestream:
                filestream.writelines(processedFileContent)
            #break
        except Exception as e:
            logger.error(f"Failed to transform source file {file}: {e}")

def main() -> int:
    parser = argparse.ArgumentParser(description="XII Migration Tool.")

    parser.add_argument("source",              type=str,                  help="The full path to the engine source files. This does not have to be the engine source directory.")
    parser.add_argument("--consolelog", "-cl", type=int, default=1,       help="Should log to console instead of a file. Default is 1, use 0 to log to file.")
    parser.add_argument("--loglevel",   "-lv", type=str, default="DEBUG", help="The logging level to use.", choices={"DEBUG", "INFO", "WARNING", "ERROR"})

    args = parser.parse_args()

    # Initialize Logger.
    if args.consolelog == 0:
        # Ensure that file is overwritten with 'w'.
        logging.basicConfig(level=GetLoggingLevel(args.loglevel), filename=GetResourcePath("Log.txt"), filemode="w")
    else:
        logging.basicConfig(level=GetLoggingLevel(args.loglevel))

    if len(args.source.strip()) == 0:
        logger.error("No valid engine path provided.")
        return 1

    path: pathlib.Path = pathlib.Path(args.source)
    if not path.exists() or not path.is_dir():
        logger.error(f"The engine path {path} does not exist or is not a valid directory.")
        return 1

    InstanceData.sSourcePath = str(path)

    logger.info("XII Migration Tool")
    logger.info("Copyright (c) 2024 Theophilus Eriata. All Rights Reserved")
    logger.info("Please ensure that all project files are backed up before using this tool.")

    # First resolve file names.
    ResolveFileNames()

    # Next, resolve the namespaces. That is, the ez -> xii conversion.
    ResolveNamespace()

    return 0

if __name__ == '__main__':
    sys.exit(main())

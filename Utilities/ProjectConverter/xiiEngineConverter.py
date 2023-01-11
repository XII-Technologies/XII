# XII Engine Project Converter
#
# Date:    October 4, 2022
# Author:  Theophilus Eriata
#
# Purpose:
#   Convert a project made with ezEngine to XII.
#
# Usage:
#   - Run with absoulute path to the project
#   - Ex. >>> python xiiEngineConverter.py C:\Dev\MyProj

import os
import sys
import time
import shutil
import string
import logging
import pathlib
import datetime
import typing as t

class Static:
    xiiExtensions : list = [
        ### Natives
        #   Extensions are expected to have a 1 : 1 Relationship. This may be changed in the future.
        #   The first extension in the set will be renamed to the second.
        #   Extensions that are the same will not be renamed

        # Native and Source Files
        [".h", ".h"],
        [".c", ".c"],
        [".py", ".py"],
        [".cs", ".cs"],
        [".cc", ".cc"],
        [".inl", ".inl"],
        [".cpp", ".cpp"],
        [".cxx", ".cxx"],
        
        # Build System Files
        [".sh", ".sh"],
        [".txt", ".txt"],
        [".ps1", ".ps1"],
        [".bat", ".bat"],
        [".json", ".json"],
        [".cmake", ".cmake"],
        [".BuildFilter", ".BuildFilter"],
        [".ezPluginBundle", ".xiiPluginBundle"],

        # Project
        [".ddl", ".ddl"],
        # ["ezProject", "xiiProject"], Should be deleted
        [".ezManifest", ".xiiManifest"],
        [".ezExportFilter", ".xiiExportFilter"],

        # Scripting
        [".ts", ".ts"],
        [".ezTypeScriptAsset", ".xiiTypeScriptAsset"],
        [".ezVisualScriptAsset", ".xiiVisualScriptAsset"],
        [".ezAnimationControllerAsset", ".xiiAnimationControllerAsset"],

        # Graphics
        [".ezScene", ".xiiScene"],
        [".ezPrefab", ".xiiPrefab"],
        [".ezShader", ".xiiShader"],
        [".ezPermVar", ".xiiPermVar"],
        [".ezMaterial", ".xiiMaterial"],
        [".ezMeshAsset", ".xiiMeshAsset"],
        [".ezDecalAsset", ".xiiDecalAsset"],
        [".ezTextureAsset", ".xiiTextureAsset"],
        [".ezMaterialAsset", ".xiiMaterialAsset"],
        [".ezSkeletonAsset", ".xiiSkeletonAsset"],
        [".ezShaderTemplate", ".xiiShaderTemplate"],
        [".ezTextureCubeAsset", ".xiiTextureCubeAsset"],
        [".ezRenderTargetAsset", ".xiiRenderTargetAsset"],
        [".ezAnimatedMeshAsset", ".xiiAnimatedMeshAsset"],
        [".ezAnimationClipAsset", ".xiiAnimationClipAsset"],
        [".ezRenderPipelineAsset", ".xiiRenderPipelineAsset"],

        # Data
        [".ezLUTAsset", ".xiiLUTAsset"],
        [".ezCurve1DAsset", ".xiiCurve1DAsset"],
        [".ezSurfaceAsset", ".xiiSurfaceAsset"],
        [".ezImageDataAsset", ".xiiImageDataAsset"],
        [".ezCollectionAsset", ".xiiCollectionAsset"],
        [".ezPropertyAnimAsset", ".xiiPropertyAnimAsset"],
        [".ezColorGradientAsset", ".xiiColorGradientAsset"],

        ### Plugins

        # Kraut Plugin
        [".ezKrautTreeAsset", ".xiiKrautTreeAsset"],

        # Particle Plugin
        [".ezParticleEffectAsset", ".xiiParticleEffectAsset"],

        # Fmod Plugin
        [".ezSoundBankAsset", ".xiiSoundBankAsset"],
        
        # ProcGen Plugin
        [".ezProcGenGraphAsset", ".xiiProcGenGraphAsset"],

        # Jolt Plugin
        [".ezJoltCollisionMeshAsset", ".xiiJoltCollisionMeshAsset"],
        [".ezJoltConvexCollisionMeshAsset", ".xiiJoltConvexCollisionMeshAsset"],

        # PhysX Plugin
        [".ezCollisionMeshAsset", ".xiiCollisionMeshAsset"],
        [".ezConvexCollisionMeshAsset", ".xiiConvexCollisionMeshAsset"],

        # RML UI Plugin
        [".ezRmlUiAsset", ".xiiRmlUiAsset"],

        # Other
        [".md", ".md"],
        [".yml", ".yml"],

        # QT
        [".ui", ".ui"],
        [".qrc", ".qrc"],

        [".jpg", ".jpg"],
        [".png", ".png"],
        [".svg", ".svg"],
        [".dds", ".dds"]
    ]

    xiiEnginePath: pathlib.Path = None
    xiiProjectPath: pathlib.Path = None

    # Stores replace regexes in order of [oldKey, newKey] in a 2D List
    xiiReplaceKeys: list = [
        ["ez", "xii"],
        ["Ez", "XII"],
        ["EZ", "XII"],
    ]

    # Store transformed filepaths given that there are repetitions in filepaths
    xiiSetStorage : set = set()

class Log:

    ENABLE_LOGGING = True

    # logging.basicConfig(
    #     stream=sys.stderr,
    #     level=logging.INFO,
    #     format="%(asctime)s %(name)s %(levelname)s - %(message)s",
    # )
    # logging.getLogger("py.warnings").setLevel(logging.ERROR)
    # logging.captureWarnings(True)
    rootLogger = logging.getLogger()

    logHandler = logging.StreamHandler()
    logHandler.setFormatter(logging.Formatter("[%(asctime)s] [%(levelname)s] %(name)s - %(message)s"))

    logFileHandler = None

    rootLogger.addHandler(logHandler)
    rootLogger.setLevel(logging.INFO)

    logger = logging.getLogger(__name__)
    logger.addHandler(logging.NullHandler())
    
    @staticmethod
    def Initialize():
        today = datetime.datetime.today()
        date_ = today.strftime("%d-%m-%Y")
        time_ = today.strftime("%H-%M-%S")
        
        fileName = "{0}/Output/Transform/Transform_{1}_{2}.txt".format(str(Static.xiiEnginePath), date_, time_)
        OSFile.CreateFile(fileName)

        Log.logFileHandler = logging.FileHandler(
            filename=fileName, mode="a", encoding="utf-8", delay=False
        )
        Log.rootLogger.addHandler(Log.logFileHandler)

    # Prints an info message to the console
    @staticmethod
    def Info(message):
        if Log.ENABLE_LOGGING:
            logging.info(message)

    # Prints a warning message to the console
    @staticmethod
    def Warning(message):
        if Log.ENABLE_LOGGING:
            logging.warning(message)

    # Prints an error message to the console
    @staticmethod
    def Error(message):
        if Log.ENABLE_LOGGING:
            logging.error(message)

    # Log Exception message to the console
    @staticmethod
    def Exception(message):
        if Log.ENABLE_LOGGING:
            logging.exception(message)

    # Prints a debug message to the console
    @staticmethod
    def Debug(message):
        if Log.ENABLE_LOGGING:
            logging.debug(message)


class OSFile:

    @staticmethod
    def PathExists(path: str):
        return os.path.exists(path)

    @staticmethod
    def IsDirectory(path: str):
        return os.path.isdir(path)

    # Deletes a file at a given filepath
    @staticmethod
    def DeleteFile(filepath: str):
        if os.path.isfile(filepath):
            os.remove(filepath)
            Log.Info("Deleted file at path {}".format(filepath))
        else:
            Log.Error(
                "Failed to delete file at path '{}'. File does not exist or may have been removed.".format(
                    filepath
                )
            )

    # Delets a specified directory
    @staticmethod
    def DeleteDirectory(directoryPath: str):
        if (os.path.exists(directoryPath)):
            shutil.rmtree(directoryPath)
            Log.Info("Deleted directory at path '{}'".format(directoryPath))
            return

        Log.Info("Failed to delete directory at path '{}' Directory does not exist or may have been removed.".format(directoryPath))

    @staticmethod
    def GetNextName(path: str) -> str:
        """Find the next numeric unused path.
        E.g. if dir is empty, GetNextName('dir/file') will return 'dir/file0',
        but if 'dir/file0', 'dir/file1' already exist, then 'dir/file2' will be returned.
        If the provided path has an extension (. character), indexes will be checked/added
        before the first period.
        """
        name, *extensions = path.split(".")
        extension = ".".join(extensions)
        # If an extension exists, prefix it with .
        if extension:
            extension = "." + extension
        index = 0
        while os.path.exists(f"{name}{index}{extension}"):
            index += 1
        return f"{name}{index}{extension}"

    @staticmethod
    def GetNextNameNoConflict(path: pathlib.Path) -> pathlib.Path:
        """Determine the next non-conflicting path name.
        Creates a non-conflicting name by incrementing a trailing number,
        directly before the first '.'.
        <name>.<ext> -> <name>1.<ext> -> <name>2.<ext> ...
        If the path does not already exist, it is returned unchanged.
        """

        # If the path doesn't exist, doing .parent.iterdir
        # will appropriately raise an error, but in that
        # case we can properly say existing is an empty set
        try:
            existing = set(path.parent.iterdir())
        except FileNotFoundError:
            existing = set()

        # We edit path until it no longer exists
        # If it doesn't exist in the first place we instantly return
        while path in existing:
            # Extract suffixes
            name, *suffixes = path.name.split(".")
            # Strip any existing numeral off
            bare_name = name.rstrip(string.digits)
            # Recover digits
            # Empty string will raise a value error
            try:
                old_value = int(name[len(bare_name) :])
            except ValueError:
                old_value = 0
            # Increment
            next_mark = str(old_value + 1)
            # need to do the '.' + '.'.join to get leading '.' when there is a extension
            new_name = (
                bare_name + next_mark + ("." if suffixes else "") + ".".join(suffixes)
            )
            path = path.with_name(new_name)

        return path

    @staticmethod
    def QueryFolder(query: t.Optional[str], generic: str, parent: str) -> pathlib.Path:
        """Construct a query-based folder for files based on given parameters.
        Makes a path with the query if given,
        otherwise increments a generic folder name.
        """

        # Query can be false if None, or empty ("")
        if query:
            data_folder = pathlib.Path(parent).joinpath(query)
        else:
            data_folder = OSFile.GetNextNameNoConflict(
                pathlib.Path(parent).joinpath(pathlib.Path(generic))
            )

        return data_folder

    @staticmethod
    def ScanDirectory(directory: str, extensions: list) -> str:
        """
        Scan directory and return all subfolders recursively.
        """
        # https://stackoverflow.com/questions/18394147/how-to-do-a-recursive-sub-folder-search-and-return-files-in-a-list/59803793#59803793
        subfolders, files = [], []

        for path in os.scandir(directory):
            if path.is_dir():
                subfolders.append(path.path)
            if path.is_file():
                if not extensions:
                    files.append(path.path)
                else:
                    for ext in extensions:
                        if os.path.splitext(path.name)[1] == ext:
                            files.append(path.path)

        for dir in list(subfolders):
            _subfolders, _files = OSFile.ScanDirectory(dir, extensions)
            subfolders.extend(_subfolders)
            files.extend(_files)

        return subfolders, files

    @staticmethod
    def CreateDirectory(directory: str):
        directory_exists = os.path.exists(directory)

        if directory_exists:
            Log.Info("Directory at '{}' alread exists!".format(directory))
            return

        os.mkdir(directory)
        Log.Info("Directory created at '{}'".format(directory))

    @staticmethod
    def CopyFileToDirectory(sourceFile: pathlib.Path, destinationFolder: pathlib.Path):
        # Copy File with Metadata
        shutil.copy2(str(sourceFile), str(destinationFolder))\
        
    @staticmethod
    def CreateFile(path: str):
        internalPath: pathlib.Path = pathlib.Path(path)
        internalPathParent = internalPath.parent

        if not OSFile.IsDirectory(str(internalPathParent)):
            internalPathParent.mkdir(parents=True)

        try:
            with open(str(internalPath), "x") as _:
                pass
        except Exception as e:
            Log.Error(
                "Encountered an error in creating file at path '{0}': '{1}'".format(
                    str(internalPath), e
                )
            )


class ProjectConverter:

    @staticmethod
    def ConvertEngineProject():
        """Converts Engine Project to the XII Convention"""

        # Replace File Extensions
        for ext in Static.xiiExtensions:
            # Get all files of extension
            folders, files = OSFile.ScanDirectory(
                directory=str(Static.xiiEnginePath), extensions=ext
            )

            # Replace file extension
            for replaceFile in files:
                filePath = pathlib.Path(replaceFile)

                if filePath.suffix != ext[1]:
                    fileName = filePath.stem

                    # Append proper extension
                    fileName = fileName + ext[1]

                    if fileName[0:2] == "ez":
                        fileName = "xii" + fileName[2: len(fileName)]

                    Log.Info(fileName)

                    # Get parent directory to rebuild the path.
                    filePath = filePath.parent

                    finalFileResolve = pathlib.Path(str(filePath) + str("/") + str(fileName))

                    try:
                        os.rename(replaceFile, str(finalFileResolve))
                    except FileNotFoundError:
                        Log.Info("File not found at '{}'".format(replaceFile))

                else:
                    fileName = filePath.stem + filePath.suffix

                    if fileName[0:2] == "ez":
                        fileName = "xii" + fileName[2: len(fileName)]

                        # Get parent directory to rebuild the path.
                        filePath = filePath.parent

                        finalFileResolve = pathlib.Path(str(filePath) + str("/") + str(fileName))
                        
                        try:
                            os.rename(replaceFile, str(finalFileResolve))
                        except FileNotFoundError:
                            Log.Info("File not found at '{}'".format(replaceFile))


            
        # Secondly, Get all files of supported XII extentions and perform file resolve
        fileExtensions: list = []

        for ext in Static.xiiExtensions:
            for xiiExt in Static.xiiExtensions:
                fileExtensions.append(xiiExt[1])

        folders, files = OSFile.ScanDirectory(
            directory=str(Static.xiiEnginePath), extensions=fileExtensions
        )

        for path in files:


            if path in Static.xiiSetStorage:
                # Path has already been transformed
                continue

            excludeList = [".png", ".jpg", ".svg"]
            filePath = pathlib.Path(path)
            found = False
            
            for e in excludeList:
                if filePath.suffix == e:
                    found = True

            if found:
                continue

            Log.Info("Resolving file {}".format(path))

            try:

                fileData = ""

                with open(path, "r") as fileHandle:
                    # Read content of file
                    fileData = fileHandle.read()

                # Replace keys
                for prevKey in Static.xiiReplaceKeys:
                    fileData = fileData.replace(prevKey[0], prevKey[1])

                with open(path, "w") as fileHandle:
                    fileHandle.write(fileData)

            except Exception as e:
                Log.Info("Failed to transform file '{}' due to exception '{}'".format(path, e))
            
            Static.xiiSetStorage.add(path)

    @staticmethod
    def ConvertProject():
        """Converts a game project into XII readable format"""

        try:
            # Delete Generated Folders and Directories
            OSFile.DeleteFile(str(pathlib.Path( str(Static.xiiProjectPath) + "/ezProject" )))
            OSFile.DeleteDirectory(str(pathlib.Path( str(Static.xiiProjectPath) + "/AssetCache" )))
            OSFile.DeleteDirectory(str(pathlib.Path( str(Static.xiiProjectPath) + "/RuntimeConfigs" )))

            # Copy necessary files
            OSFile.CopyFileToDirectory(pathlib.Path(  str(Static.xiiEnginePath) + "/Data/Base/xiiProject"), Static.xiiProjectPath)
        except Exception as e:
            Log.Info("Encountered an exception '{}' during Setup.".format(e))

        # Get all files from supported extensions and resolve with their counterparts
        # First, resolve file extensions

        for ext in Static.xiiExtensions:
            # Get all files of extension
            folders, files = OSFile.ScanDirectory(
                directory=str(Static.xiiProjectPath), extensions=ext
            )

            # Replace file extension
            for replaceFile in files:
                filePath = pathlib.Path(replaceFile)
                
                if filePath.suffix != ext[1]:
                    fileName = filePath.stem

                    # Append proper extension
                    fileName = fileName + ext[1]

                    # Get parent directory to rebuild the path.
                    filePath = filePath.parent

                    finalFileResolve = pathlib.Path(str(filePath) + str("/") + str(fileName))
                    os.rename(replaceFile, str(finalFileResolve))

        # Secondly, Get all files of supported XII extentions and perform file resolve
        fileExtensions: list = []

        for ext in Static.xiiExtensions:
            for xiiExt in Static.xiiExtensions:
                fileExtensions.append(xiiExt[1])

        folders, files = OSFile.ScanDirectory(
            directory=str(Static.xiiProjectPath), extensions=fileExtensions
        )

        for path in files:

            if path in Static.xiiSetStorage:
                # Path has already been transformed
                continue

            Log.Info("Resolving file {}".format(path))

            try:

                fileData = ""

                with open(path, "r") as fileHandle:
                    # Read content of file
                    fileData = fileHandle.read()

                # Replace keys
                for prevKey in Static.xiiReplaceKeys:
                    fileData = fileData.replace(prevKey[0], prevKey[1])

                # Resolve issues
                # Things with eeze like Freeze and Breeze transform into exiie that need to be resolved
                # fileData = fileData.replace("xiie", "eze")
                # fileData = fileData.replace("XIIE", "EZE")
                # fileData = fileData.replace("xiiie", "ezie")
                # fileData = fileData.replace("XIIIE", "EZIE")

                with open(path, "w") as fileHandle:
                    fileHandle.write(fileData)
            
            except Exception as e:
                Log.Info("Failed to transform file '{}' due to exception '{}'".format(path, e))

            Static.xiiSetStorage.add(path)

    @staticmethod
    def ConvertPictures():
        if fileName[0:2] == "ez":
            fileName = "xii" + fileName[2, len(fileName)]
        
        if fileName[0:2] == "EZ":
            fileName = "XII" + fileName[2, len(fileName)]

def PrintHelpMessage():
    Log.Info("Run program with arguments <Path to Engine Folder> <Path to Project Folder>")
    Log.Info("Note: Absolute paths are required.\n")
    Log.Info("Sample: \n\t python xiiEngineProject.py C:\Dev\XII C:\Dev\Projects\MyProj")


def main():

    startTime = time.perf_counter()

    Log.Info("XII Project Converter")
    Log.Info("Copyright (c) 2022 Theophilus Eriata. All Rights Reserved\n")
    Log.Info("Please that all project files are backed up before using this tool.\n")

    Log.Debug("Number of arguments: {}".format(len(sys.argv)))
    Log.Debug("Argument list: {}".format(sys.argv))

    # Engine Project should be situated at index 1, and the project folder at index 2
    if (len(sys.argv) != 3):
        Log.Error("Wrong arguments specified!")
        PrintHelpMessage()
        return

    Log.Debug("Engine Project {}".format(sys.argv[1]))
    Log.Debug("Project Folder {}".format(sys.argv[2]))

    if sys.argv[2] != "-xiiNow":

        # Try to resolve specified paths
        Static.xiiEnginePath = pathlib.Path(sys.argv[1])
        Static.xiiProjectPath = pathlib.Path(sys.argv[2])

        if (not Static.xiiEnginePath.exists()):
            Log.Error("Specified an invalid Engine Path")
            PrintHelpMessage()
            return

        if (not Static.xiiProjectPath.exists()):
            Log.Error("Specified an invalid Project Path")
            PrintHelpMessage()
            return

        # Log.Initialize()

        Log.Info("Engine  Path {}".format(str(Static.xiiEnginePath)))
        Log.Info("Project Path {}".format(str(Static.xiiProjectPath)))

        # Begin Project Conversion
        ProjectConverter.ConvertProject()

        Log.Info("")
        Log.Info("Project Conversion Complete.")
        Log.Info("Time Taken {}s".format(round(time.perf_counter() - startTime, 3)))

    else:
        
        Static.xiiEnginePath = pathlib.Path(sys.argv[1])

        # Log.Initialize()

        if (not Static.xiiEnginePath.exists()):
            Log.Error("Specified an invalid Engine Path")
            PrintHelpMessage()
            return
        
        Log.Info("Engine Path {}".format(str(Static.xiiEnginePath)))

        # Begin Project Conversion
        ProjectConverter.ConvertEngineProject()

        Log.Info("")
        Log.Info("Project Conversion Complete.")
        Log.Info("Time Taken {}s".format(round(time.perf_counter() - startTime, 3)))


if __name__ == "__main__":
    main()

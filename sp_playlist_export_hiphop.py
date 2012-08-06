import subprocess, os, sys, argparse, glob, shutil

OUTPUT_FOLDER = "E:\\"
LOCAL_LIBRARY_FOLDER = "Y:\\Music"

l_Parser = argparse.ArgumentParser()
l_Parser.add_argument('passwd')
l_Args = l_Parser.parse_args()

os.chdir(os.path.abspath(os.path.dirname(__file__)))

localLibrary = {}

from mutagen.easyid3 import EasyID3
from mutagen.m4a import M4A

def importLocalDirectory(dirPath):
    for fileName in os.listdir(dirPath):
        filePath = os.path.join(dirPath, fileName)
        if os.path.isdir(filePath):
            importLocalDirectory(filePath)

        entry = None
        if fileName.lower().endswith(".mp3"):
            audio = EasyID3(filePath)
            entry = fileName + audio["artist"][0] + audio["title"][0]

        if fileName.lower().endswith(".m4a"):
            audio = M4A(filePath)
            entry = fileName + audio['\xa9ART'] + audio['\xa9nam']

        if entry != None:
            localLibrary[filePath] = entry
            
def findInLibrary(fileName):
    objs = fileName.split(' - ')
    for key, value in localLibrary.items():
        found = True
        for obj in objs:
            if value.find(obj) == -1:
                found = False
                break
        if found:
            return key

    return None

importLocalDirectory(LOCAL_LIBRARY_FOLDER)
print "Local library:", len(localLibrary), "files"

l_PlaylistList = ['Exotic', 'Funky', 'Jazz Hip-Hop', 'Jazzy', 'New School', 'Old School', 'Trip Hop', 'Underground']

for l_Playlist in l_PlaylistList:
    
    l_DirPath = os.path.join(OUTPUT_FOLDER, l_Playlist)
    if not os.path.isdir(l_DirPath):
        os.mkdir(l_DirPath)
        
    l_FileToCopy = {}
    l_FileToRemove = {}
    while True:
        l_Process = subprocess.Popen(['sp_playlist_export', 'Benweet', l_Args.passwd, OUTPUT_FOLDER, l_Playlist], stdout=subprocess.PIPE)
        while True:
            l_Process.poll()
            l_Result = l_Process.returncode
            if l_Result != None:
                break

            l_Line = l_Process.stdout.readline()
            if l_Line.startswith('_ENCODE_'):
                print "__________________________________________"
                l_OutputFilename = "%s.m4a"%l_Line[9:].rstrip()
                print "ENCODE:", l_OutputFilename
                l_OutputFilePath = os.path.join(l_DirPath, l_OutputFilename)
                
                l_QaacProcess = subprocess.Popen(['qaac', '-i', '-o', l_OutputFilePath, '-'], stdin=l_Process.stdout)
                l_Process.stdout.close()
                l_QaacProcess.communicate()
                #fileName = os.path.split(l_Line[9:].rstrip())[1]
                #f = open("Y:\\Temp\\%s.wav"%fileName, 'wb')
                #while True:
                    #buffer = l_Process.stdout.read(4096)
                    #if len(buffer) == 0:
                        #f.close()
                        #sys.exit()
                    #f.write(buffer)
                
            if l_Line.startswith('_REMOVE_'):
                l_OutputFilename = l_Line[9:].rstrip() + ".*"
                for l_File in glob.glob(l_OutputFilename):
                    l_FileToRemove[l_File] = None
                
            if l_Line.startswith('_COPY_'):
                l_OutputDir, l_OutputFilename = os.path.split(l_Line[9:].rstrip())
                localFile = findInLibrary(l_OutputFilename)
                if localFile != None:
                    l_OutputFilename += localFile[-4:].lower()
                l_OutputFilename = os.path.join(l_OutputDir, l_OutputFilename)
                l_FileToCopy[l_OutputFilename] = localFile
                
        if l_Result != 1:
            break
    
    for dest, src in l_FileToCopy.items():
        print "__________________________________________"
        if src != None:
            print "COPY:", dest
            shutil.copy(src, dest)
        else:
            print "Not found in local library:", dest

    for filePath in l_FileToRemove.keys():
        print "__________________________________________"
        print "REMOVE:", filePath
        os.unlink(filePath)

    if l_Result:
        print("ERROR:", l_Result)
        sys.exit(l_Result)

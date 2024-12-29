// SPDX-License-Identifier: Apache-2.0
// Copyright 2021 - 2022, the Anboto author and contributors
#include <Core/Core.h>
#include "Functions4U.h"


#ifdef PLATFORM_WIN32 // || defined (PLATFORM_WIN64)
	#define Ptr Ptr_
	#define byte byte_
	#ifndef win32_CY_
		#define win32_CY_ long
	#endif
	#define CY win32_CY_
	
	#include <shellapi.h>
	#include <wincon.h>
	#include <shlobj.h>
	
	#undef Ptr
	#undef byte
	#undef CY
#endif

#define TFILE <Functions4U/Functions4U.t>
#include <Core/t.h>

	
/*
Hi Koldo,

I checked the functions in Functions4U. Here are some notes about trashing:

    * On older systems, the trash folder was $HOME/.Trash
    * Your implementation of disregards the folder $HOME/.local/share/trash/info. You should create 
    there a .trashinfo file when moving something in trash and delete it when deleting corresponding file permanently.
    * If you delete something on different partition than $HOME, you should also check if .Trash-XXXX 
    exists in root of that partition (XXXX is id of user who deleted the files in it). 

.local/share/Trash/files
.local/share/Trash/info

A file every time a file is removed with

KK.trashinfo
[Trash Info]
Path=/home/pubuntu/KK
DeletionDate=2010-05-19T18:00:52


You might also be interested in following:

    * Official trash specification from freedesktop.org
    * Project implementing command line access to trash (unfortunately in python) according to the specification mentioned above 


Hope this might help Smile It definitely learned me a lot of new things Very Happy

Best regards, 
Honza
*/

namespace Upp {


/////////////////////////////////////////////////////////////////////
// LaunchFile

#if defined(PLATFORM_WIN32) || defined (PLATFORM_WIN64)
bool LaunchFileCreateProcess(const char *file, const char *params, const char *directory) {
	STARTUPINFOW startInfo;
    PROCESS_INFORMATION procInfo;

    ZeroMemory(&startInfo, sizeof(startInfo));
    startInfo.cb = sizeof(startInfo);
    ZeroMemory(&procInfo, sizeof(procInfo));

	String command = Format("\"%s\" \"%s\" %s", GetExtExecutable(GetFileExt(file)), file, params);
	Vector<WCHAR> cmd = ToSystemCharsetW(command);
	
	if (!CreateProcessW(NULL, cmd, NULL, NULL, FALSE, 0, NULL, directory ? ToSystemCharsetW(directory) : (LPCWSTR)NULL, &startInfo, &procInfo))  
		return false;

   	WaitForSingleObject(procInfo.hProcess, 0);

	if (!CloseHandle(procInfo.hProcess))
		return false;
	if (!CloseHandle(procInfo.hThread))
		return false;	
	return true;
}

bool LaunchFileShellExecute(const char *file, const char *params, const char *directory) {
	uint64 ret = uint64(ShellExecuteW(NULL, L"open", ToSystemCharsetW(file), ToSystemCharsetW(params), ToSystemCharsetW(directory), SW_SHOWNORMAL));		 
	return 32 < ret;
} 

bool LaunchFile(const char *file, const char *params, const char *directory) {
	String _file = Trim(WinPath(file));
	String _params, _directory;
	if (params)
		_params = WinPath(params);
	if (directory)
		_directory = WinPath(directory);
	if (!LaunchFileShellExecute(_file, _params, _directory))			// First try
	   	return LaunchFileCreateProcess(_file, _params, _directory);		// Second try
	return true;
}
#endif

#ifdef PLATFORM_POSIX

String GetDesktopManagerNew() {
	if(GetEnv("GNOME_DESKTOP_SESSION_ID").GetCount() || GetEnv("GNOME_KEYRING_SOCKET").GetCount()) 
		return "gnome";
	else if(GetEnv("KDE_FULL_SESSION").GetCount() || GetEnv("KDEDIR").GetCount() || GetEnv("KDE_MULTIHEAD").GetCount()) 
        return "kde"; 
	else {
		StringParse desktopStr;
		if (Sys("xprop -root _DT_SAVE_MODE").Find("xfce") >= 0)
			return "xfce";
		else if ((desktopStr = Sys("xprop -root")).Find("ENLIGHTENMENT") >= 0) 
			return "enlightenment";
		else
			return GetEnv("DESKTOP_SESSION");
	}
}

bool LaunchFile(const char *_file, const char *_params, const char *) {
	String file = UnixPath(_file);
	String params = _params == nullptr ? "" : UnixPath(_params);
	int ret;
	if (GetDesktopManagerNew() == "gnome") 
		ret = system("gnome-open \"" + file + "\" " + params);
	else if (GetDesktopManagerNew() == "kde") 
		ret = system("kfmclient exec \"" + file + "\" " + params + " &"); 
	else if (GetDesktopManagerNew() == "enlightenment") {
		String mime = GetExtExecutable(GetFileExt(file));
		String program = mime.Left(mime.Find("."));		// Left side of mime executable is the program to run
		ret = system(program + " \"" + file + "\" " + params + " &"); 
	} else 
		ret = system("xdg-open \"" + String(file) + "\"");
	return (ret >= 0);
}
#endif

bool LaunchCommand(const char* command, const char* directory) {
#if defined(PLATFORM_WIN32) 
	STARTUPINFOW startInfo;
    PROCESS_INFORMATION procInfo;

    ZeroMemory(&startInfo, sizeof(startInfo));
    startInfo.cb = sizeof(startInfo);
    ZeroMemory(&procInfo, sizeof(procInfo));

	if (!CreateProcessW(NULL, ToSystemCharsetW(command), NULL, NULL, FALSE, DETACHED_PROCESS, NULL, directory ? ToSystemCharsetW(directory) : (LPCWSTR)NULL, &startInfo, &procInfo))
		return false;

    if (!CloseHandle(procInfo.hProcess))
        return false;
    if (!CloseHandle(procInfo.hThread))
        return false;
#else
	String actualDirectory;
	if (directory) {
		actualDirectory = GetCurrentDirectory();
		if (!SetCurrentDirectory(directory))
			return false;
	}
	system(S(command) + "&");
	if (directory) {
		if (!SetCurrentDirectory(actualDirectory))
			return false;
	}
#endif	
	return true;
}
/////////////////////////////////////////////////////////////////////
// General utilities

bool FileCat(const char *file, const char *appendFile) {
	if (!FileExists(file)) 
		return FileCopy(appendFile, file);
	FileAppend f(file);
	if(!f.IsOpen())
		return false;
	FileIn fi(appendFile);
	if(!fi.IsOpen())
		return false;
	CopyStream(f, fi, fi.GetLeft());
	fi.Close();
	f.Close();
	if(f.IsError()) 
		return false;
	return true;
}

bool FileStrAppend(const char *file, const char *str) {
	FileAppend f(file);
	if(!f.IsOpen())
		return false;
	f << str;
	f.Close();
	if(f.IsError()) 
		return false;
	return true;
}

bool AppendFile(const char *file, const char *str) {return FileStrAppend(file, str);}

String FormatLong(long a) { 
	return Sprintf("%ld", a);
}

String Replace(String str, String find, String replace) {
	String ret;
	
	int lenStr = str.GetCount();
	int lenFind = find.GetCount();
	int i = 0, j;
	while ((j = str.Find(find, i)) >= i) {
		ret += str.Mid(i, j-i) + replace;
		i = j + lenFind;
		if (i >= lenStr)
			break;
	}
	ret += str.Mid(i);
	return ret;
}

String Replace(String str, char find, char replace) {
	StringBuffer ret(str);
	for (int i = 0; i < ret.GetCount(); ++i) {
		if (ret[i] == find)
			ret[i] = replace;
	}
	return String(ret);
}

// Rename file or folder
bool FileMoveX(const char *oldpath, const char *newpath, EXT_FILE_FLAGS flags) {
	bool usr, grp, oth;
	if (flags & DELETE_READ_ONLY) {
		if (IsReadOnly(oldpath, usr, grp, oth))
			SetReadOnly(oldpath, false, false, false);
	}
	bool ret = FileMove(oldpath, newpath);
	if (flags & DELETE_READ_ONLY) 
		SetReadOnly(newpath, usr, grp, oth);
	return ret;
}

bool FileDeleteX(const char *path, EXT_FILE_FLAGS flags) {
	if (flags & USE_TRASH_BIN)
		return FileToTrashBin(path);
	else {
		if (flags & DELETE_READ_ONLY) 
			SetReadOnly(path, false, false, false);
		return FileDelete(path);
	}
}

bool FolderDeleteX(const char *path, EXT_FILE_FLAGS flags) {
	if (flags & USE_TRASH_BIN)
		return FileToTrashBin(path);
	else {
		if (flags & DELETE_READ_ONLY) 
			SetReadOnly(path, false, false, false);
		return DirectoryDelete(path);
	}
}

bool DirectoryCreateX(const char *path) {
	DirectoryCreate(path);
	return DirectoryExists(path);
}

bool DirectoryExistsX_Each(const char *name) {
#if defined(PLATFORM_WIN32)
	if(name[0] && name[1] == ':' && name[2] == '\\' && name[3] == 0 &&
	   GetDriveType(name) != DRIVE_NO_ROOT_DIR) 
	    return true;
	DWORD res = GetFileAttributes(ToSystemCharset(name));
	if (!(res & FILE_ATTRIBUTE_DIRECTORY))
		return false;
	if (res != INVALID_FILE_ATTRIBUTES)
		return true;
	if (!(name[0] && name[1] == ':'))
		return false;
	if (!(ERROR_PATH_NOT_FOUND == GetLastError()))
		return false;
	
	String localName = String(name, 2);
	char remoteName[256]; 
	DWORD lenRemoteName = sizeof(remoteName); 
	res = WNetGetConnection(localName, remoteName, &lenRemoteName);  
	if (res != ERROR_CONNECTION_UNAVAIL)
		return false;
	
	NETRESOURCE nr;
	memset(&nr, 0, sizeof(NETRESOURCE));
	nr.dwType = RESOURCETYPE_DISK;
	nr.lpLocalName = const_cast<char *>(localName.Begin());
	nr.lpRemoteName = remoteName;
	nr.lpProvider = NULL;
    DWORD dwFlags = CONNECT_UPDATE_PROFILE;
    res = WNetAddConnection2(&nr, NULL, NULL, dwFlags);
   	if (res != NO_ERROR)
   		return false;
   	
	res = GetFileAttributes(ToSystemCharset(name));
	return (res != INVALID_FILE_ATTRIBUTES && 
           (res & FILE_ATTRIBUTE_DIRECTORY));	
	
#else
	FindFile ff(name);
	return ff && ff.IsDirectory();
#endif
}

bool DirectoryExistsX(const char *path, EXT_FILE_FLAGS flags) {
	String spath = path;
	if (spath.EndsWith(DIR_SEPS))
		spath = spath.Left(spath.GetCount() - 1);
	if (!(flags & BROWSE_LINKS))
		return DirectoryExistsX_Each(spath);
	if (DirectoryExistsX_Each(spath))
		return true;
	if (!IsSymLink(spath))
		return false;
	String filePath;
	filePath = GetSymLinkPath(spath);
	if (filePath.IsEmpty())
		return false;
	return DirectoryExistsX_Each(filePath); 
}

bool IsFile(const char *fileName) {
	FindFile ff;
	if(ff.Search(fileName) && ff.IsFile()) 
		return true;
	return false;
}
	
bool IsFolder(const char *fileName) {
	FindFile ff;
	if(ff.Search(fileName) && ff.IsFolder()) 
		return true;
	return false;
}

String GetRelativePath(String from, String path, bool normalize) {
	String ret, dir_seps;
	String creplace = DIR_SEP == '\\' ? "/" : "\\";
	if (normalize) {
		from.Replace(creplace, DIR_SEPS);
		path.Replace(creplace, DIR_SEPS);
		if (!PLATFORM_PATH_HAS_CASE) {
			from = ToLower(from);
			path = ToLower(path);
		}
		dir_seps = DIR_SEPS;
	} else {
		bool seplinux = from.Find('/') >= 0 || path.Find('/') >= 0;
		bool sepwindows = from.Find('\\') >= 0 || path.Find('\\') >= 0;
		if (seplinux && sepwindows) {
			dir_seps = DIR_SEPS;	
			from.Replace(creplace, DIR_SEPS);
			from.Replace(creplace, DIR_SEPS);
		} else if (seplinux) 
			dir_seps = "/";	
		else
			dir_seps = "\\";	
	}
	int pos_from = 0, pos_path = 0;
	bool first = true;
	while (!IsNull(pos_from)) {
		String f_from = Tokenize2(from, dir_seps, pos_from);
		String f_path = Tokenize2(path, dir_seps, pos_path);
		if (f_from != f_path) {
			if (first) 
				return String::GetVoid();
			ret << f_path;	
			String fileName = path.Mid(pos_path);
			if (!fileName.IsEmpty()) 
				ret << dir_seps << fileName;	
			while (!IsNull(pos_from)) {
				ret.Insert(0, String("..") + dir_seps);
				Tokenize2(from, dir_seps, pos_from);		
			}
			ret.Insert(0, String("..") + dir_seps);
			return ret;
		}
		first = false;
	}
	return path.Mid(pos_path);
}

String GetAbsolutePath(String from, String relative) {
	from = Trim(from);
	relative = Trim(relative);
	if (!relative.StartsWith("."))
		return relative;
	while (!from.IsEmpty() && !relative.IsEmpty()) {
		if (relative.StartsWith("./") || relative.StartsWith(".\\")) 
			relative = relative.Mid(2);
		else if (relative.StartsWith("../") || relative.StartsWith("..\\")) {
			relative = relative.Mid(3);
			from = GetUpperFolder(from);
		} else
			break;
	}
	return AppendFileNameX(from, relative);
}

bool SetReadOnly(const char *path, bool readOnly) {
	return SetReadOnly(path, readOnly, readOnly, readOnly);
}

bool SetReadOnly(const char *path, bool usr, bool, bool) {
#if defined(PLATFORM_WIN32) || defined (PLATFORM_WIN64)
	DWORD attr = GetFileAttributesW(ToSystemCharsetW(path)); 
	
	if (attr == INVALID_FILE_ATTRIBUTES) 
		return false; 

	DWORD newattr;
	if (usr)
		newattr = attr | FILE_ATTRIBUTE_READONLY;
	else
		newattr = attr & ~FILE_ATTRIBUTE_READONLY;
	
	if (attr != newattr)
		return SetFileAttributesW(ToSystemCharsetW(path), newattr); 
	else
		return true;
#else
	struct stat buffer;
	//int status;

	if(0 != stat(ToSystemCharset(path), &buffer))
		return false;
	
	mode_t m = buffer.st_mode;
	mode_t newmode = (m & S_IRUSR) | (m & S_IRGRP) | (m & S_IROTH);
	
	if (newmode != buffer.st_mode)
		return 0 == chmod(ToSystemCharset(path), newmode);
	else
		return true;
#endif
}

bool IsReadOnly(const char *path, bool &usr, bool &grp, bool &oth) {
#if defined(PLATFORM_WIN32) || defined (PLATFORM_WIN64)
	DWORD attr = GetFileAttributesW(ToSystemCharsetW(path)); 
	
	if (attr == INVALID_FILE_ATTRIBUTES) 
		return false; 

	usr = grp = oth = attr & FILE_ATTRIBUTE_READONLY;
	return true;
#else
	struct stat buffer;

	if(0 != stat(ToSystemCharset(path), &buffer))
		return false;
	
	usr = buffer.st_mode & S_IRUSR;
	grp = buffer.st_mode & S_IRGRP;
	oth = buffer.st_mode & S_IROTH;
	
	return true;
#endif
}

#ifdef PLATFORM_POSIX

int GetUid() {
	String proc = LoadFile_Safe("/etc/passwd");
	int pos = proc.Find(GetUserName());
	if (pos < 0)
		return -1;
	pos = proc.Find(':', pos);
	if (pos < 0)
		return -1;
	pos = proc.Find(':', pos+1);
	if (pos < 0)
		return -1;
	int posend = proc.Find(':', pos+1);
	if (posend < 0)
		return -1;
	return ScanInt(proc.Mid(pos+1, posend-pos-1));
}

String GetMountDirectory(const String &path) {
	Vector<String> drives = GetDriveList();	
	for (int i = 0; i < drives.GetCount(); ++i) {		
		if (path.Find(drives[i]) == 0)
			return drives[i];
	}
	String localPath = AppendFileNameX(GetCurrentDirectory(), path);
	if (!FileExists(localPath) && !DirectoryExists(localPath))
		return "";
	for (int i = 0; i < drives.GetCount(); ++i) {
		if (localPath.Find(drives[i]) == 0)
			return drives[i];
	}
	return "";
}
	
String GetTrashBinDirectory()
{	
	String ret = GetEnv("XDG_DATA_HOME");
	if (ret.IsEmpty())
		ret = AppendFileNameX(GetHomeDirectory(), ".local/share/Trash");
	else
		ret = AppendFileNameX(ret, "Trash");
	return ret;
}

bool FileToTrashBin(const char *path)
{	
	String newPath = AppendFileNameX(GetTrashBinDirectory(), GetFileName(path));
	return FileMove(path, newPath);
}

int64 TrashBinGetCount()
{
	int64 ret = 0;
	FindFile ff;
	if(ff.Search(AppendFileNameX(GetTrashBinDirectory(), "*"))) {
		do {
			String name = ff.GetName();
			if (name != "." && name != "..")
				ret++;
		} while(ff.Next());
	}
	return ret;
}

bool TrashBinClear()
{
	FindFile ff;
	String trashBinDirectory = GetTrashBinDirectory();
	if(ff.Search(AppendFileNameX(trashBinDirectory, "*"))) {
		do {
			String name = ff.GetName();
			if (name != "." && name != "..") {
				String path = AppendFileNameX(trashBinDirectory, name);
				if (ff.IsFile())
					FileDelete(path);
				else if (ff.IsDirectory()) {
					DeleteFolderDeep(path);		Sleep(100);
				}
			}
		} while(ff.Next());
	}
	return true;
}

#endif
#if defined(PLATFORM_WIN32) || defined (PLATFORM_WIN64)

bool DirectoryMove(const char *dir, const char *newPlace) {
	if (strcmp(dir, newPlace) == 0)
		return true;
	
	WString wDir(dir), wNewPlace(newPlace);
    wDir.Cat(L'\0');	
    wNewPlace.Cat(L'\0');	
	
    SHFILEOPSTRUCTW fileOp = {};
  	fileOp.hwnd = NULL;
    fileOp.wFunc = FO_MOVE;
    fileOp.pFrom = (PCZZWSTR)~wDir;
    fileOp.pTo = (PCZZWSTR)~wNewPlace;
    fileOp.fFlags = FOF_ALLOWUNDO | FOF_NOCONFIRMATION | FOF_SILENT;
 
    int ret = SHFileOperationW(&fileOp);
 	return ret == 0;
}

bool FileToTrashBin(const char *path) {	
    if (!FileExists(path) && !DirectoryExists(path))
        return false;
	
    WString wpath(path);
    wpath.Cat(L'\0');	
		
    SHFILEOPSTRUCTW fileOp = {}; 
    fileOp.hwnd = NULL;
    fileOp.wFunc = FO_DELETE;
    fileOp.pFrom = (PCZZWSTR)~wpath;
    fileOp.pTo = NULL;
    fileOp.fFlags = FOF_ALLOWUNDO | FOF_NOCONFIRMATION | FOF_SILENT;

    int ret = SHFileOperationW(&fileOp);
 	return ret == 0;
}

int64 TrashBinGetCount() {
	SHQUERYRBINFO shqbi; 
 
 	shqbi.cbSize = sizeof(SHQUERYRBINFO);
	if (S_OK != SHQueryRecycleBin(0, &shqbi))
		return -1;
	return shqbi.i64NumItems;
}

bool TrashBinClear() {
	if (S_OK != SHEmptyRecycleBin(0, 0, SHERB_NOCONFIRMATION | SHERB_NOPROGRESSUI | SHERB_NOSOUND))
		return false;
	return true;
}

#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

String LoadFile_Safe(const String fileName)
{
#ifdef PLATFORM_POSIX
	int fid = open(fileName, O_RDONLY);
#else
	int fid = _wopen((const wchar_t *)fileName.ToWString().Begin(), O_RDONLY|O_BINARY);
#endif
	if (fid < 0) 
		return String();
	const int size = 1024;
	int nsize;
	StringBuffer s;
	char buf[size];
	while((nsize = read(fid, buf, size)) == size) 
		s.Cat(buf, size);
	if (nsize > 1)
		s.Cat(buf, nsize-1);
	close(fid);
	return String(s);
}

String LoadFile(const char *fileName, off_t from, size_t len)
{
#ifdef PLATFORM_POSIX
	int fid = open(fileName, O_RDONLY);
#else
	int fid = _wopen((const wchar_t *)String(fileName).ToWString().Begin(), O_RDONLY|O_BINARY);
#endif
	if (fid < 0) 
		return String();
	if (0 > lseek(fid, from, SEEK_SET))
		return String();
	size_t size = 1024;
	if (len != 0 && size > len)
		size = len;
	size_t nsize;
	StringBuffer s;
	Buffer<char> buf(size);
	size_t loaded;
	for (loaded = 0; (nsize = read(fid, buf, unsigned(size))) == size && (len == 0 || loaded < len); loaded += nsize) {
		if (len != 0 && loaded + size > len)
			size = len - loaded;
		s.Cat(buf, int(size));
	}
	if (nsize > 1 && (len == 0 || loaded < len))
		s.Cat(buf, int(nsize-1));
	close(fid);
	return String(s);
}

Array<SoftwareDetails> GetSoftwareDetails(String software) {
	software = ToLower(software);
	Array<SoftwareDetails> enumSoft = GetInstalledSoftware();
	for (int i = enumSoft.size()-1; i >= 0; --i) {
		if (!PatternMatch(software, ToLower(enumSoft[i].name)))
			enumSoft.Remove(i);
	}
#ifdef POSIX
	for (auto &s : enumSoft)
		GetSoftwarePath(s);
#endif
	return enumSoft;
}

#if defined(PLATFORM_WIN32) 
Array<SoftwareDetails> GetInstalledSoftware() {
	Array<SoftwareDetails> ret;
    char sAppKeyName[_MAX_PATH];
    char str[_MAX_PATH];
    dword dwType;
    
    struct Soft {
    	HKEY key;
    	String architecture;
    	String path; 
    };
    Array<Soft> paths = {{HKEY_LOCAL_MACHINE, "32", "SOFTWARE\\WOW6432Node\\Microsoft\\Windows\\CurrentVersion\\Uninstall"},
    					 {HKEY_LOCAL_MACHINE, "64", "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall"},
    					 {HKEY_LOCAL_MACHINE, "64", "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Installer\\UserData"},
    					 {HKEY_LOCAL_MACHINE, "64", "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Group Policy\\AppMgmt"},
						 {HKEY_LOCAL_MACHINE, "64", "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\ProfileList"},
						 {HKEY_LOCAL_MACHINE, "32", "SOFTWARE\\Wow6432Node\\Microsoft\\Windows NT\\CurrentVersion\\ProfileList"},
						 {HKEY_CLASSES_ROOT,  "64", "Installer\\Products"}};

  
    for (int ip = 0; ip < paths.size(); ++ip) {
        const Soft &path = paths[ip];
		HKEY hUninstKey = NULL;
	    if(RegOpenKeyEx(path.key, path.path, 0, KEY_READ, &hUninstKey) == ERROR_SUCCESS) {
			long lResult = ERROR_SUCCESS;
		    for(dword dwIndex = 0; lResult == ERROR_SUCCESS; dwIndex++) {
		        dword dwBufferSize = sizeof(sAppKeyName);
		        if((lResult = RegEnumKeyEx(hUninstKey, dwIndex, sAppKeyName,
		            &dwBufferSize, NULL, NULL, NULL, NULL)) == ERROR_SUCCESS) {
		            
		            String sSubKey = AppendFileName(path.path, sAppKeyName);
		            HKEY hAppKey = NULL;
		            if(RegOpenKeyEx(path.key, sSubKey, 0, KEY_READ, &hAppKey) == ERROR_SUCCESS) {
		                SoftwareDetails &data = ret.Add();
		                
		                dwType = KEY_ALL_ACCESS;
			            dwBufferSize = sizeof(str);
			            if(RegQueryValueEx(hAppKey, "DisplayName", NULL, &dwType, (unsigned char*)str, &dwBufferSize) == ERROR_SUCCESS)
			                data.name = str;
	
						if (data.name == "") 
							ret.Remove(ret.size()-1);
						else {
							dwType = KEY_ALL_ACCESS;
				            dwBufferSize = sizeof(str);
				            if(RegQueryValueEx(hAppKey, "Publisher", NULL, &dwType, (unsigned char*)str, &dwBufferSize) == ERROR_SUCCESS)
				                data.publisher = str;
	
							dwType = KEY_ALL_ACCESS;
				            dwBufferSize = sizeof(str);
				            if(RegQueryValueEx(hAppKey, "DisplayVersion", NULL, &dwType, (unsigned char*)str, &dwBufferSize) == ERROR_SUCCESS)
				                data.version = str;
	
							dwType = KEY_ALL_ACCESS;
				            dwBufferSize = sizeof(str);
				            if(RegQueryValueEx(hAppKey, "InstallLocation", NULL, &dwType, (unsigned char*)str, &dwBufferSize) == ERROR_SUCCESS)
				                data.path = str;
				            
				            data.architecture = path.architecture;
		            	}    
		            }
		            RegCloseKey(hAppKey);
		        }
		    }
	    }
	    RegCloseKey(hUninstKey);
    }
    return ret;
}

#endif

#ifdef PLATFORM_POSIX

static void GetInstalledSoftwareDpkg(Array<SoftwareDetails> &ret) {
	String str = Sys("dpkg --list --no-pager");
	StringStream stream(str);
	
	int idname = -1, idver, idarch, iddesc;
	while (!stream.IsEof()) {
		String line = stream.GetLine();
		if ((iddesc = line.Find("Description")) >= 0) {
			idname = line.Find("Name");
			idver = line.Find("Version");
			idarch = line.Find("Architecture");
			break;
		}
	}
	if (idname == -1)
		return;
	
	stream.GetLine();
	LineParser p;
	while (!stream.IsEof()) {	
		p.LoadFields(stream.GetLine(), {idname, idver, idarch, iddesc});

		SoftwareDetails &r = ret.Add();
		r.name = p.GetText(0);	
		r.version = p.GetText(1);
		r.architecture = p.GetText(2);
		r.description = p.GetText(3);
	}
}

static void GetInstalledSoftwareSnap(Array<SoftwareDetails> &ret) {
	String str = Sys("snap list");
	StringStream stream(str);
	
	int idname = -1, idver, idrev, idtrack, idpub, idnotes;
	while (!stream.IsEof()) {
		String line = stream.GetLine();
		if ((idpub = line.Find("Publisher")) >= 0) {
			idname = line.Find("Name");
			idver = line.Find("Version");
			idrev = line.Find("Rev");
			idtrack = line.Find("Tracking");
			idpub = line.Find("Publisher");
			idnotes = line.Find("Notes");
			break;
		}
	}
	if (idname == -1)
		return;
	
	stream.GetLine();
	LineParser p;
	while (!stream.IsEof()) {	
		p.LoadFields(stream.GetLine(), {idname, idver, idrev, idtrack, idpub, idnotes});
		SoftwareDetails &r = ret.Add();
		r.name = p.GetText(0);	
		r.version = p.GetText(1) + "." + p.GetText(2);
		r.publisher = p.GetText(4);
		
		//GetSoftwarePath(r);
	}
}

Array<SoftwareDetails> GetInstalledSoftware() {
	Array<SoftwareDetails> ret;
	
	GetInstalledSoftwareDpkg(ret);
	GetInstalledSoftwareSnap(ret);
	
	return ret;
}


#endif

String GetExtExecutable(const String _ext)
{
	String ext = _ext;
	String exeFile = "";
	if (ext[0] != '.')
		ext = String(".") + ext;
#if defined(PLATFORM_WIN32) || defined (PLATFORM_WIN64)
	String file = AppendFileNameX(GetHomeDirectory(), String("dummy") + ext); // Required by FindExecutableW
	SaveFile(file, "   ");
	if (!FileExists(file)) 
		return "";
	WCHAR exe[MAX_PATH];
	HINSTANCE ret = FindExecutableW(ToSystemCharsetW(file), NULL, exe);
	if (reinterpret_cast<uint64>(ret) > 32)
		exeFile = WString(exe).ToString();
	DeleteFile(file);
#endif
#ifdef PLATFORM_POSIX
	StringParse mime;
	//if (LaunchCommand(String("xdg-mime query filetype ") + file, mime) >= 0) 	// xdg-mime query filetype does not work properly in Enlightenment
	mime = LoadFile_Safe("/etc/mime.types");	// Search in /etc/mime.types the mime type for the extension
	if ((mime.GoAfter_Init(String(" ") + ext.Right(ext.GetCount()-1))) || (mime.GoAfter_Init(String("\t") + ext.Right(ext.GetCount()-1)))) {
		mime.GoBeginLine();
		mime = mime.GetText();
		exeFile = TrimRight(Sys(String("xdg-mime query default ") + mime));
	}
#endif
	return exeFile;
}

#if defined(PLATFORM_WIN32) || defined (PLATFORM_WIN64)
Vector<String> GetDriveList() {
	char drvStr[26*4+1];		// A, B, C, ...
	Vector<String> ret;
	
	int lenDrvStrs = ::GetLogicalDriveStrings(sizeof(drvStr), drvStr);
	// To get the error call GetLastError()
	if (lenDrvStrs == 0)
		return ret;
	
	ret.Add(drvStr);
	for (int i = 0; i < lenDrvStrs-1; ++i) {
		if (drvStr[i] == '\0') 
			ret.Add(drvStr + i + 1);
	}
	return ret;
}
#endif
#if defined(PLATFORM_POSIX)
Vector<String> GetDriveList() {
	Vector<String> ret;
	// Search for mountable file systems
	String mountableFS = "cofs.";
	StringParse sfileSystems(LoadFile_Safe("/proc/filesystems"));
	String str;
	while (true) {
		str = sfileSystems.GetText();	
		if (str == "")
			break;
		else if (str != "nodev")
			mountableFS << str << ".";
		else 
			str = sfileSystems.GetText();
	}
	// Get mounted drives
	StringParse smounts(LoadFile_Safe("/proc/mounts"));
	StringParse smountLine(Trim(smounts.GetText("\r\n")));
	do {
		String devPath 	 = smountLine.GetText();
		String mountPath = smountLine.GetText();
		String fs        = smountLine.GetText();
		if ((mountableFS.Find(fs) >= 0) && (mountPath.Find("/dev") < 0) 
		 && (mountPath.Find("/rofs") < 0) && (mountPath != "/"))	// Is mountable 
			ret.Add(mountPath);
		smountLine = Trim(smounts.GetText("\r\n"));
	} while (smountLine != "");
	ret.Add("/");	// Last but not least
	return ret;
}
#endif


#if defined(PLATFORM_WIN32) || defined (PLATFORM_WIN64)
String GetShellFolder2(int clsid) 
{
	wchar path[MAX_PATH];
	if(SHGetFolderPathW(NULL, clsid, NULL, //SHGFP_TYPE_CURRENT
											0, (LPWSTR)path) == S_OK)
		return FromUnicodeBuffer(path);
	return Null;
}

String GetShellFolder2(const char *local, const char *users) 
{
	String ret = FromSystemCharset(GetWinRegString(local, "Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders", 
									   HKEY_CURRENT_USER));
	if (ret == "" && *users != '\0')
		return FromSystemCharset(GetWinRegString(users, "Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders", 
									   HKEY_LOCAL_MACHINE));
	return ret;
}

String GetPersonalFolder()	{return GetShellFolder2("Personal", 0);}
String GetStartupFolder()	{return GetShellFolder2(CSIDL_STARTUP);}

String GetTempFolder()
{
	String ret;
	if ((ret = GetEnv("TEMP")) == "")	// One or the other one
		ret = GetEnv("TMP");
	return ret;
}

String GetOsFolder()
{
	char ret[MAX_PATH];
	::GetWindowsDirectory(ret, MAX_PATH);
	return String(ret);
}
String GetSystemFolder()
{
	char ret[MAX_PATH];
	::GetSystemDirectory(ret, MAX_PATH);
	return String(ret);
}

#ifdef PLATFORM_WIN32
String GetCommonAppDataFolder() { 
	wchar path[MAX_PATH];
	if(SHGetFolderPathW(NULL, CSIDL_COMMON_APPDATA, NULL, 0, (LPWSTR)path) == S_OK)
		return FromUnicodeBuffer(path);
	return Null;
}
#endif
/*
bool SetEnv(const char *id, const char *val) 
{
//	EnvMap().Put(WString(id), WString(val));
#ifdef PLATFORM_POSIX
	return setenv(id, val, 1) == 0;
#else
	WString str = WString(id) + "=" + WString(val);
	return _wputenv((const wchar_t *)str.Begin()) == 0;
#endif
}
*/

#endif
#ifdef PLATFORM_POSIX

String GetPathXdg2(String xdgConfigHome, String xdgConfigDirs)
{
	String ret;
	if (FileExists(ret = AppendFileNameX(xdgConfigHome, "user-dirs.dirs"))) 
		;
  	else if (FileExists(ret = AppendFileNameX(xdgConfigDirs, "user-dirs.defaults")))
  		;
  	else if (FileExists(ret = AppendFileNameX(xdgConfigDirs, "user-dirs.dirs")))
  		;
  	return ret;
}
String GetPathDataXdg2(String fileConfig, const char *folder) 
{
	StringParse fileData = LoadFile(fileConfig);
	
	if (!fileData.GoAfter(folder)) return "";
	if (!fileData.GoAfter("=")) return "";
	
	String ret = "";
	StringParse path = fileData.GetText();
	if(path.GoAfter("$HOME/")) 
		ret = AppendFileNameX(GetHomeDirectory(), path.Right());
	else if (!FileExists(path))
		ret = AppendFileNameX(GetHomeDirectory(), path);
	
	return ret;		
}
String GetShellFolder2(const char *local, const char *users) 
{
 	String xdgConfigHome = GetEnv("XDG_CONFIG_HOME");
  	if (xdgConfigHome == "")		// By default
  		xdgConfigHome = AppendFileNameX(GetHomeDirectory(), ".config");
  	String xdgConfigDirs = GetEnv("XDG_CONFIG_DIRS");
  	if (xdgConfigDirs == "")			// By default
  		xdgConfigDirs = "/etc/xdg";
  	String xdgFileConfigData = GetPathXdg2(xdgConfigHome, xdgConfigDirs);
  	String ret = GetPathDataXdg2(xdgFileConfigData, local);
  	if (ret == "" && *users != '\0')
  		return GetPathDataXdg2(xdgFileConfigData, users);
  	else
  		return ret;
}

String GetTempFolder()
{
	return GetHomeDirectory();		
}
String GetOsFolder()
{
	return String("/bin");
}
String GetSystemFolder()
{
	return String("");
}

String GetPersonalFolder()	{return GetShellFolder2("XDG_DOCUMENTS_DIR", "DOCUMENTS");}

#endif

struct StringNormalCompare__ {
	int operator()(char a, char b) const { return a - b; }
};

int Compare(const String& a, int i0, const String& b, int len) {
	return IterCompare(a.Begin() + i0, a.Begin() + i0 + len, b.Begin(), b.Begin() + len, StringNormalCompare__());
}

int ReverseFind(const String& s, const String& toFind, int from) {
	ASSERT(from >= 0 && from <= s.GetLength());
	int lc = toFind.GetLength();
	for (int i = from; i >= 0; --i) {
		if (Compare(s, i, toFind, lc) == 0)
			return i;
	}
	return -1;
}

Time StringToTime(const char *s) {
	Time ret;
	if (StrToTime(ret, s))
		return ret;
	else
		return Null;
}

Date StringToDate(const char *s) {
	Time ret;
	if (StrToDate(ret, s))
		return ret;
	else
		return Null;
}

Upp::Time StrToTime(const char *s)	{return StringToTime(s);}
Date StrToDate(const char *s)		{return StringToDate(s);}

void StringToHMS(const char *s, int &hour, int &min, double &seconds) {
	StringParse duration(s);
	String s1, s2, s3; 
	s1 = duration.GetText(":");
	s2 = duration.GetText(":");
	s3 = duration.GetText();
	
	if (s3 != "") {
		hour = ScanInt(s1);
		min = ScanInt(s2);
		seconds = ScanDouble(s3);
	} else if (s2 != "") {
		hour = 0;
		min = ScanInt(s1);
		seconds = ScanDouble(s2);
	} else {
		hour = 0;
		min = 0;
		seconds = ScanDouble(s1);		
	}
	if (IsNull(hour) || IsNull(min) || IsNull(seconds)) {
		hour = min = Null;
		seconds = Null;
	} else if (hour < 0 || min < 0 || seconds < 0) {
		hour = Neg(hour);
		min = Neg(min);
		seconds = Neg(seconds);
	}
}

double StringToSeconds(const char *s) {
	int hour, min;
	double secs;
	StringToHMS(s, hour, min, secs); 
	return 3600.*hour + 60.*min + secs;
}

String formatSeconds(double seconds, int dec, bool fill) {
	int iseconds = int(seconds);
	String ret;
	if (fill)
		ret = FormatIntDec(iseconds, 2, '0');
	else
		ret = FormatInt(iseconds);
	double decs = seconds - iseconds;
	if (decs > 0 && dec > 0) 
		ret << "." << FormatIntDec(static_cast<int>(decs*pow(10, dec)), dec, '0');
	return ret;
}

String HMSToString(int hour, int min, double seconds, int dec, bool units, bool space, bool tp, 
					bool longUnits, bool forcesec) {
	String ret;
	bool isneg = hour < 0 || min < 0 || seconds < 0;
	
	if (hour > 0) {
		ret << hour;
		if (space)
			ret << " ";
		if (tp)
			ret << ":";
		if (units)
			ret << (longUnits ? ((hour > 1) ? t_("hours") : t_("hour")) : t_("h"));
	}
	
	if (min > 0) {
		if (tp) {
			String fmt = hour > 0 ? "%02d" : "%d";
			ret << Format(fmt, min);
		} else
			ret << (ret.IsEmpty() ? "" : " ") << min;
		if (space)
			ret << " ";
		if (tp && forcesec)
			ret << ":";
		if (units)
			ret << (longUnits ? ((min > 1) ? t_("mins") : t_("min")) : t_("m"));
	} else if (tp) {
		if (hour > 0) {
			ret << "00";
			if (forcesec)
				ret << ":";
		}
	}
	
	if (forcesec || (hour == 0 && (seconds > 1 || (seconds > 0 && dec > 0)))) {
		ret << (ret.IsEmpty() || tp ? "" : " ") << formatSeconds(seconds, dec, tp && min > 0);
		if (space)
			ret << " ";
		if (units)
 			ret << (longUnits ? ((seconds > 1) ? t_("secs") : t_("sec")) : t_("s"));
	}
	
	if (isneg)
		ret = "-" + ret;
	return ret;
}

String SecondsToString(double seconds, int dec, bool units, bool space, bool tp, 
					bool longUnits, bool forcesec) {
	int hour, min;
	hour = static_cast<int>(seconds/3600.);
	seconds -= hour*3600;
	min = static_cast<int>(seconds/60.);
	seconds -= min*60;	
	return HMSToString(hour, min, seconds, dec, units, space, tp, longUnits, forcesec);
}

String SeasonName(int iseason) {
	static const char *season[] = {t_("winter"), t_("spring"), t_("summer"), t_("autumn")};
	return iseason >= 0 && iseason < 4 ? season[iseason] : "";
}

int GetSeason(const Date &date) {
	return int((date.month - 1)/3.);
}

String BytesToString(uint64 _bytes, bool units)
{
	String ret;
	uint64 bytes = _bytes;
	
	if (bytes >= 1024) {
		bytes /= 1024;
		if (bytes >= 1024) {
			bytes /= 1024;
			if (bytes >= 1024) {
				bytes /= 1024;
				if (bytes >= 1024) {
					//bytes /= 1024;
					ret = Format("%.1f %s", _bytes/(1024*1024*1024*1024.), units ? "Tb" : "");
				} else
					ret = Format("%.1f %s", _bytes/(1024*1024*1024.), units ? "Gb" : "");
			} else
				ret = Format("%.1f %s", _bytes/(1024*1024.), units ? "Mb" : "");
		} else
			ret = Format("%.1f %s", _bytes/1024., units ? "Kb" : "");
	} else
		ret << _bytes << (units ? "b" : "");
	return ret;
}

int NumAdvicedDigits(double range) {
	if 		(0.001 <= range && range < 0.01) 	return 7;
	else if (0.01  <= range && range < 0.1) 	return 6;
	else if (0.1   <= range && range < 10) 		return 5;
	else if (10	   <= range && range < 100) 	return 5;
	else if (100   <= range && range < 10000) 	return 6;
	else if (10000 <= range && range < 100000) 	return 7;
	else return 6;
}

String FormatDoubleAutosize(double d) {
	return FormatDoubleAutosize(d, d);
}
	
String FormatDoubleAutosize(double d, double range) {
	return FDS(d, NumAdvicedDigits(range));
}

inline bool IsNum_(double n) {return Upp::IsFin(n) && !IsNull(n);}

String FormatDoubleSize(double d, int fieldWidth, bool fillSpaces, const String &strNull) {
	String data;
	if (!IsNum_(d)) {
		if (fillSpaces)
			data = String(' ', fieldWidth - strNull.GetCount());
		data.Cat(strNull);
	} else {
		int actualWidth = fieldWidth;
		data = FormatDouble(d, fieldWidth, FD_CAP_E|FD_SPECIAL|FD_MINIMAL_EXP);
		while (data.GetCount() > fieldWidth && actualWidth >= 3) {
			actualWidth--;
			data = FormatDouble(d, actualWidth, FD_CAP_E|FD_SPECIAL|FD_MINIMAL_EXP);
		}
		if (fillSpaces && data.GetCount() <= fieldWidth) 
			data = String(' ', fieldWidth - data.GetCount()) + data;
	}
	return data;	
}

String FormatDoubleDecimals(double d, int maxDecimals) {
	String ret = FormatF(d, maxDecimals);
	
	int ichar;
	for (ichar = ret.GetCount()-1; ichar >= 0 && ret[ichar] == '0'; --ichar)
		;
	if (ret[ichar] == '.')
		ichar--;
	
	return ret.Left(ichar+1);
}
	
int GetExponent10(double d) {
	d = abs(d);
	if (d >= 1)
    	return int(log10(d));
	else if (d == 0)
		return 0;
	else
		return -int(log10(1/d))-1;
}

double NumberWithLeastSignificantDigits(double minVal, double maxVal) {
	ASSERT(minVal < maxVal);
	if (maxVal >= 0 && minVal <= 0)		// Crosses 0
		return 0;	
	if (maxVal - minVal < 10*std::numeric_limits<double>::epsilon())	// Almost 0
		return 0;
	
    double range = maxVal - minVal;
    
    //if (abs(maxVal) < range/10 || abs(minVal) < range/10)
    //    return 0;

	double val = Avg(minVal, maxVal);
	int emin = GetExponent10(val);
	double p10 = Pow10Int<double>(emin);
	minVal /= p10;
	maxVal /= p10;
	range  /= p10;
	
    int precision = 0;
    double result = minVal;

    while (true) {
        double multiplier = Pow10Int<double>(precision);
        double roundedResult = floor(result * multiplier)/multiplier;

        if (roundedResult >= maxVal) 
            return maxVal*p10;

        if (roundedResult != result) {
        	if (!Between(roundedResult, minVal, maxVal)) 
        		roundedResult += multiplier;
            return roundedResult*p10;
        }
        result += range / Pow10Int<double>(precision);
        precision++;
    }
    return Null;
}

double GetRangeMajorUnits(double minV, double maxV) {
	ASSERT(minV < maxV);
	double rgy = maxV - minV;
	double dy = rgy/6;
	dy = Pow10Int<double>(GetExponent10(rgy));
	if (int(rgy/dy) > 6) {
		if (int(rgy/(2*dy)) < 6) 
			dy *= 2;
		else
			dy *= 5;
	} else if (int(rgy/dy) < 3) {
		dy /= 10;
		
		if (int(rgy/dy) > 6) {
			if (int(rgy/(2*dy)) < 6) 
				dy *= 2;
			else
				dy *= 5;
		} 
	}
	return dy;
}

WString RemoveAccentW(wchar c) {
	WString wsret;

	if (IsAlNum(c) || IsSpace(c)) {
		wsret.Cat(c);
		return wsret;
	}
	//const WString accented = "ÂÃÀÁÇÈÉÊËẼÌÍÎÏÑÒÓÔÕÙÚÛÝàáâãçèéêëẽìíîïñòóôõøùúûýÿ";
	const WString accented = "\303\202\303\203\303\200\303\201\303\207\303\210\303\211\303\212\303\213\341\272\274\303\214\303\215\303\216\303\217\303\221\303\222\303\223\303\224\303\225\303\231\303\232\303\233\303\235\303\240\303\241\303\242\303\243\303\247\303\250\303\251\303\252\303\253\341\272\275\303\254\303\255\303\256\303\257\303\261\303\262\303\263\303\264\303\265\303\270\303\271\303\272\303\273\303\275\303\277ω";
	const char *unaccented = "AAAACEEEEEIIIINOOOOUUUYaaaaceeeeeiiiinooooouuuyyw";
	
	for (int i = 0; accented[i]; ++i) {
		if (*(accented.begin() + i) == c) {
			wsret.Cat(unaccented[i]);
			return wsret;	
		}
	}
	//const WString maccented = "ÅåÆæØøþÞßÐðÄäÖöÜü";
	const WString maccented = "\303\205\303\245\303\206\303\246\303\230\303\270\303\276\303\236\303\237\303\220\303\260\303\204\303\244\303\226\303\266\303\234\303\274∞";
	const char *unmaccented[] = {"AA", "aa", "AE", "ae", "OE", "oe", "TH", "th", "SS", "ETH", 
								 "eth", "AE", "ae", "OE", "oe", "UE", "ue", "inf"};
	for (int i = 0; maccented[i]; ++i) {
		if (*(maccented.begin() + i) == c) 
			return unmaccented[i];
	}
	wsret.Cat(c);
	return wsret;
}

String RemoveAccent(wchar c) {
	return RemoveAccentW(c).ToString();
}

bool IsPunctuation(wchar c) {
	//const WString punct = "\"’'()[]{}<>:;,‒–—―….,¡!¿?«»-‐‘’“”/\\&@*\\•^©¤฿¢$€ƒ£₦¥₩₪†‡〃#№ºª\%‰‱ ¶′®§℠℗~™|¦=";
	const WString punct = "\"\342\200\231'()[]{}<>:;,\342\200\222\342\200\223\342\200\224\342\200\225\342\200\246.,\302\241!\302\277?\302\253\302\273-\342\200\220\342\200\230\342\200\231\342\200\234\342\200\235/\\&@*\\\342\200\242^\302\251\302\244\340\270\277\302\242$\342\202\254\306\222\302\243\342\202\246\302\245\342\202\251\342\202\252\342\200\240\342\200\241\343\200\203#\342\204\226\302\272\302\252%\342\200\260\342\200\261∞"
     					  "\302\266\342\200\262\302\256\302\247\342\204\240\342\204\227~\342\204\242|\302\246=";
	for (int i = 0; punct[i]; ++i) {
		if (*(punct.begin() + i) == c) 
			return true;
	}
	return false;
}

const char *GreekSymbols(int i) {
	const char *greekSymbols[] = {"alpha", "Αα", "nu", "Νν", "beta", "Ββ", "xi", "Ξξ", "gamma", "Γγ", 
				"omicron", "Οο", "delta", "Δδ", "pi", "Ππ", "epsilon", "Εε", "rho", "Ρρ", "zeta", "Ζζ", 	 	
				"sigma", "Σσ", "eta", "Ηη", "tau", "Ττ", "theta", "Θθ", "upsilon", "Υυ", "iota", "Ιι", 	 	
				"phi", "Φφ", "kappa", "Κκ", "chi", "Χχ", "lambda", "Λλ", "psi", "Ψψ", "mu", "Μμ", "omega", "Ωω", ""};
	return greekSymbols[i];
}
			
String GreekToText(wchar c) {
	for (int i = 0; GreekSymbols(i)[0]; i += 2) {
		WString ws(GreekSymbols(i+1));
		if (c == *(ws.begin()))
			return InitCaps(GreekSymbols(i));		
		if (c == *(ws.begin() + 1))
			return GreekSymbols(i);		
	}
	return Null;
}

bool IsGreek(wchar c) {
	for (int i = 0; GreekSymbols(i)[0]; i += 2) {
		WString ws(GreekSymbols(i+1));
		if (c == *(ws.begin()))
			return true;		
		if (c == *(ws.begin() + 1))
			return true;		
	}
	return false;
}
		
String RemoveAccents(String str) {
	String ret;
	WString wstr = str.ToWString();
	for (int i = 0; i < wstr.GetCount(); ++i) {
		String schar = RemoveAccent(wstr[i]);
		if (i == 0 || ((i > 0) && ((IsSpace(wstr[i-1]) || IsPunctuation(wstr[i-1]))))) {
			if (schar.GetCount() > 1) {
				if (IsUpper(schar[0]) && IsLower(wstr[1]))
				 	schar = String(schar[0], 1) + ToLower(schar.Mid(1));
			}
		} 
		ret += schar;
	}
	return ret;
}

String RemovePunctuation(String str) {
	String ret;
	WString wstr = str.ToWString();
	for (int i = 0; i < wstr.GetCount(); ++i) {
		if (!IsPunctuation(wstr[i]))
		    ret += wstr[i];
	}
	return ret;
}

String RemoveGreek(String str) {
	String ret;
	WString wstr = str.ToWString();
	for (int i = 0; i < wstr.GetCount(); ++i) {
		if (!IsGreek(wstr[i]))
		    ret += wstr[i];
		else
			ret += GreekToText(wstr[i]);
	}
	return ret;	
}

String CharToSubSupScript(char c, bool subscript) {
	const Vector<WString> lower = {"aₐᵃ", "b_ᵇ", "c_ᶜ", "d_ᵈ", "e_ᵉ", "f_ᶠ", "g_ᵍ", "hₕʰ", "iᵢⁱ", "jⱼʲ", "kₖᵏ", "lₗˡ", "mₘᵐ", "nₙⁿ", "oₒᵒ", "pₚᵖ", "q__", "rᵣʳ", "sₛˢ", "tₜᵗ", "uᵤᵘ", "vᵥᵛ", "w_ʷ", "xₓˣ", "yᵧʸ", "z_ᶻ"}; 
	const Vector<WString> upper = {"Aᴀᴬ", "Bʙᴮ", "Cᴄ_", "Dᴅᴰ", "Eᴇᴱ", "Fғ_", "Gɢᴳ", "Hʜᴴ", "Iɪᴵ", "Jᴊᴶ", "Kᴋᴷ", "Lʟᴸ", "Mᴍᴹ", "Nɴᴺ", "O_ᴼ", "Pᴘᴾ", "Qǫ_", "Rʀᴿ", "Ss ", "Tᴛᵀ", "Uᴜᵁ", "Vᴠⱽ", "Wᴡᵂ", "Xx_", "Yʏ_", "Zᴢ_"};
	const Vector<WString> number= {"0₀⁰", "1₁¹", "2₂²", "3₃³", "4₄⁴", "5₅⁵", "6₆⁶", "7₇⁷", "8₈⁸", "9₉⁹"};
	const Vector<WString> symbol= {"+₊⁺", "-₋⁻", "=₌⁼", "(₍⁽", ")₎⁾", ""};
 
 	if (c >= 'a' && c <= 'z') 
 		return WString(subscript ? lower[c - 'a'][1]  : lower[c - 'a'][2], 1).ToString();
 	else if (c >= 'A' && c <= 'Z') 
 		return WString(subscript ? upper[c - 'A'][1]  : upper[c - 'A'][2], 1).ToString();
 	else if (c >= '0' && c <= '9') 
 		return WString(subscript ? number[c - '0'][1] : number[c - '0'][2], 1).ToString();
 	else {
 		for (int i = 0; symbol[i][0] != '\0'; ++i)
 			if (symbol[i][0] == c)
 				return WString(subscript ? symbol[i][1] : symbol[i][2], 1).ToString();
 	}
 	return "_";
}

String NumToSubSupScript(int d, bool subscript) {
	String str = FormatInt(d);
	String ret;
	for (int i = 0; i < str.GetCount(); ++i)
		ret << CharToSubSupScript(char(str[i]), subscript);
	return ret;
}


const char *Ordinal(int num) {
	num = abs(num);
    if (num % 100 >= 11 && num % 100 <= 13) 
        return "th";
    switch (num % 10) {
    case 1:		return "st";
    case 2:		return "nd";
    case 3:		return "rd";
    default:	return "th";
    }
}


String FitFileName(const String fileName, int len) {
	if (fileName.GetCount() <= len)
		return fileName;
	
	Vector<String> path;
	
	const char *s = fileName;
	char c;
	int pos0 = 0;
	for (int pos1 = 0; (c = s[pos1]) != '\0'; ++pos1) {
	#if defined(PLATFORM_WIN32) || defined (PLATFORM_WIN64)
		if(c == '\\' || c == '/') {
	#else
		if(c == '/') {
	#endif
			path.Add(fileName.Mid(pos0, pos1-pos0));
			pos0 = ++pos1;
		}
	}
	path.Add(fileName.Mid(pos0));

	String begin, end;
	int iBegin = 0;
	int iEnd = path.GetCount() - 1;
	
	if (path[iEnd].GetCount() >= len)
		return path[iEnd].Left(len);
	if (path[iEnd].GetCount() >= len-4)
		return path[iEnd];
	
	len -= 3;	// ...
	
	for (; iBegin <= iEnd; iBegin++, iEnd--) {
		if (path[iEnd].GetCount() + 1 > len)
			break;
		end = DIR_SEPS + path[iEnd] + end;
		len -= path[iEnd].GetCount() + 1;
		if (path[iBegin].GetCount() + 1 > len)
			break;
		begin += path[iBegin] + DIR_SEPS;
		len -= path[iBegin].GetCount() + 1;
	}
	return begin + "..." + end;
}

String Tokenize2(const String &str, const String &token, int &pos) {
	if (IsNull(pos) || pos >= str.GetCount()) {
		pos = Null;
		return Null;
	}
	int npos = str.Find(token, pos);
/*	for (int i = 0; i < token.GetCount(); ++i) {
		if ((npos = str.Find(token[i], pos)) >= 0) 
			break;
	}*/
	int oldpos = pos;
	if (npos < 0) {
		pos = Null;
		return str.Mid(oldpos);
	} else {
		pos = npos + token.GetCount();
		return str.Mid(oldpos, npos - oldpos);
	}
}

String Tokenize2(const String &str, const String &token) {
	int dummy = 0;
	return Tokenize2(str, token, dummy);
}

Vector<String> Tokenize(const String &str, const String &token, int pos) {
	Vector<String> ret;
	
	Tokenize(str, token, ret, pos);
	
	return ret;
}

void Tokenize(const String &str, const String &token, Vector<String> &ret, int pos) {
	int _pos = pos;
	while (true) {
		String strRet = Tokenize2(str, token, _pos);
		if (IsNull(_pos)) {
			if (!IsNull(strRet)) 
				ret << strRet;
			break;
		}
		ret << strRet;
	}
}

String GetLine(const String &str, int &pos) {
	ASSERT(pos >= 0);
	String ret;
	if (pos >= str.GetCount())
		return String::GetVoid();
	int npos = str.Find("\n", pos);
	if (npos == -1) {
		ret = str.Mid(pos);
		pos = -1;
	} else {
		ret = str.Mid(pos, npos - pos);
		pos = npos + 1;
	}
	return TrimBoth(ret);
}

String GetField(const String &str, int &pos, char separator) {
	ASSERT(separator != '\"');	
	String sret;
	int npos = str.Find(separator, pos);
	int spos = str.Find('\"', pos);
	if (spos < 0 || spos > npos) {
		if (npos < 0) {
			if (spos >= 0) {
				int lspos = str.Find('\"', spos + 1);
				if (lspos < 0) 
					sret = str.Mid(max(pos, spos));
				else
					sret = str.Mid(spos + 1, lspos - spos - 1);
			} else
				sret = Trim(str.Mid(pos));
			pos = -1;
		} else {
			sret = Trim(str.Mid(pos, npos - pos));
			pos = npos + 1;
		}
	} else {
		int lspos = str.Find('\"', spos + 1);
		if (lspos < 0) {
			sret = str.Mid(spos);
			pos = -1;
		} else {
			sret = str.Mid(spos + 1, lspos - spos - 1);
			npos = str.Find(separator, lspos);
			pos = npos + 1;
		}
	}	
	return sret;	
}

Value GetField(const String &str, int &pos, char separator, char decimalSign, bool onlyStrings) {
	String sret = GetField(str, pos, separator);
	if (onlyStrings)
		return sret;
	
	if (sret.IsEmpty())
		return Null;
	
	bool hasDecimal = false, hasLetter = false;
	for (int i = 0; i < sret.GetCount(); ++i) {
		if (sret[i] == decimalSign)
			hasDecimal = true;
		else if (!IsNumber(sret[i]))
			hasLetter = true;
	}
	if (!hasLetter) {
		if (hasDecimal) {
			double dbl = ScanDouble(sret, NULL, decimalSign == ',');
			if (IsNull(dbl))
				return sret;
			else 
				return dbl;
		} else {
			int64 it64 = ScanInt64(sret);
			if (IsNull(it64))
				return sret;
			int it = int(it64);
			if (it64 != it)
				return it64;
			else
				return it;
		}
	} else {
		Time t = ScanTime(sret);
		if (IsNull(t)) 
			return sret;
		else if (t.hour == 0 && t.minute == 0 && t.second == 0)
			return Date(t);
		else
			return t;
	}
}

Vector<Vector <Value> > ReadCSV(const String &strFile, char separator, bool bycols, bool removeRepeated, char decimalSign, bool onlyStrings, int fromRow) {
	Vector<Vector<Value> > result;

	if (strFile.IsEmpty())
		return result;
	
	int posLine = 0;
	for (int i = 0; i < fromRow; ++i)
		GetLine(strFile, posLine);
	
	String line;
	int pos = 0;
	if (bycols) {
		line = GetLine(strFile, posLine);
		while (pos >= 0) {
			Value name = GetField(line, pos, separator, decimalSign, onlyStrings);
			if (/*pos >= 0 && */!IsNull(name)) {
				Vector<Value> &data = result.Add();
				data.Add(name);
			}
		}
		while (posLine >= 0) {
			pos = 0;
			line = GetLine(strFile, posLine);
			if (!line.IsEmpty()) {
				bool repeated = removeRepeated;
				int row = result[0].GetCount() - 1;
				for (int col = 0; col < result.GetCount(); col++) {
					if (pos >= 0) {
						Value data = GetField(line, pos, separator, decimalSign, onlyStrings);
						result[col].Add(data);
						if (row > 0 && result[col][row] != data)
							repeated = false;
					} else
						result[col].Add();
				}
				if (row > 0 && repeated) {
					for (int col = 0; col < result.GetCount(); col++) 
						result[col].Remove(row+1);
				}
			} else
				break;
		}
	} else {
		int row = 0;
		while (posLine >= 0) {
			pos = 0;
			line = GetLine(strFile, posLine);
			bool repeated = removeRepeated;
			if (!line.IsEmpty()) {
				Vector <Value> &linedata = result.Add();
				int col = 0;
				while (pos >= 0) {
					Value val = GetField(line, pos, separator, decimalSign, onlyStrings);
					if (val.IsNull())
						linedata << "";
					else
						linedata << val;
					if (row > 0 && (result[row - 1].GetCount() <= col || result[row - 1][col] != val))
						repeated = false;
					col++;
				}
			} else
				break;
			if (row > 0 && repeated) 
				result.Remove(row);
			else
				row++;
		}
	}
	return result;
}

Vector<Vector <Value> > ReadCSVFile(const String &fileName, char separator, bool bycols, bool removeRepeated, char decimalSign, bool onlyStrings, int fromRow) {
	return ReadCSV(LoadFile(fileName), separator, bycols, removeRepeated,  decimalSign, onlyStrings, fromRow);
}

bool ReadCSVFileByLine(const String &fileName, Gate<int, Vector<Value>&, String &> WhenRow, char separator, char decimalSign, bool onlyStrings, int fromRow) {
	Vector<Value> result;

	FindFile ff(fileName);
	if(!ff || !ff.IsFile()) 
		return false;
	
	FileIn in(fileName);
	in.ClearError();
	
	for (int i = 0; i < fromRow; ++i)
		in.GetLine();
	
	for (int row = 0; true; row++) {
		String line = in.GetLine();
		if (line.IsVoid()) {
			WhenRow(row, result, line);
			return true;
		}
		int pos = 0;
		while (pos >= 0) {
			Value val = GetField(line, pos, separator, decimalSign, onlyStrings);
			if (val.IsNull())
				result << "";
			else
				result << val;
		}
		if (!WhenRow(row, result, line))
			return false;
		result.Clear();
	}
	return false;
}
	
String ToStringDecimalSign(Value &val, const String &decimalSign) {
	String ret = val.ToString();
	if (val.Is<double>() && decimalSign != ".") 
		ret.Replace(".", decimalSign);
	return ret;
}

bool GuessCSVStream(Stream &in, bool onlyNumbers, String &header, Vector<String> &parameters, char &separator, bool &repetition, char &decimalSign, int64 &beginData, int &beginDataRow) {
	const Array<char> separators = {',', ';', '\t', '|', '%', ' '};
	
	int numLinesToDiscard = 10, numLinesToCheck = 5;	
	
	// Get all lines and its positions
	Vector<String> lines;
	Vector<int64> linesPos;
	linesPos << 0;
	for (int i = 0; i < numLinesToDiscard+numLinesToCheck && !in.IsEof(); ++i) {
		lines << in.GetLine();
		linesPos << in.GetPos();
	}
	// Re adjust check window if the file is small
	if (lines.size() < numLinesToDiscard + numLinesToCheck) {
		numLinesToCheck = (numLinesToCheck*lines.size())/(numLinesToDiscard + numLinesToCheck);
		numLinesToCheck = max(numLinesToCheck, min(4, lines.size()));
		numLinesToDiscard = lines.size() - numLinesToCheck;
	}

	auto NumNum = [](const Vector<String> &a, char decimal)->int {	// Number of real numbers in a vector of strings
		const char *endptr;
		int num = 0;
		for (int i = 0; i < a.size(); ++i) {
			if (a[i].Find(decimal == '.' ? ',' : '.') < 0) {		// If ',' is the decimal, '.' is not allowed in a number, and the opposite
				if (!IsNull(ScanDouble(a[i], &endptr, decimal == ',')))
					num++;
				else {
					String str = ToLower(a[i]);
					if (str == "nan" || str == "null" || str == "true" || str == "false")
						num++;
				}
			}
		}
		return num;
	};
		
	auto CompareVector = [](const Vector<int> &a)->int {			// Checks if all the values are the same, and returns it
		if (a.IsEmpty())
			return -1;
		
		int n = a[0];
		
		for (int i = 1; i < a.size(); ++i) {
			if (n != a[i]) 
				return -1;
		}
		return n;
	};
	
	auto CompareVectors = [](const Vector<int> &a, const Vector<int> &b)->int {	// Checks if all the values in 2 vectors are the same, and returns it
		if (a.size() != b.size())
			return -1;
		if (a.IsEmpty() || b.IsEmpty())
			return -1;
		int n = a[0];
		if (n != b[0])
			return -1;
		
		for (int i = 1; i < a.size(); ++i) {
			if (n != a[i] || n != b[i]) 
				return -1;
		}
		return n;
	};
	
	// Gets the separator and decimal
	Vector<char> decimals = {'.', ','};
	Vector<bool> sepRepetition = {false, true};
	int numBest = -1;
	for (int irep = 0; irep < sepRepetition.size(); ++irep) {
		for (int idec = 0; idec < decimals.size(); ++idec) {
			for (int isep = 0; isep < separators.size(); ++isep) {
				Vector<int> numF, numNum;
				for (int row = 0; row < numLinesToCheck; ++row) {
					Vector<String> data = Split(lines[row+numLinesToDiscard], separators[isep], sepRepetition[irep]);
					numF << data.size();
					if (onlyNumbers)
						numNum << NumNum(data, decimals[idec]);
				}
				int num;
				if (onlyNumbers)
					num = CompareVectors(numF, numNum);
				else
					num = CompareVector(numF);
				
				if (num > numBest) {
					separator = separators[isep];
					decimalSign = decimals[idec];
					repetition = sepRepetition[irep];
					numBest = num;
				}
			}
		}
	}
	if (numBest < 0)
		return false;

	// Analyses the header
	int beginHeader = 0, endHeader = 0;
	for (int r = numLinesToDiscard-1; r >= 0; --r) {
		Vector<String> data = Split(lines[r], separator, repetition);
		if (data.size() != numBest) 
			break;

		beginHeader = r;
		
		if (onlyNumbers) {
			if (NumNum(data, decimalSign) == numBest)
				endHeader = r;
		} else
			endHeader = r;
	}
	
	beginData = linesPos[endHeader];
	beginDataRow = endHeader;
	
	header = "";
	for (int r = 0; r <= beginHeader && r < endHeader; ++r) {
		if (r > 0)
			header << "\n";	
		header << lines[r];
	}
	
	parameters.SetCount(numBest);
	for (int r = beginHeader; r < endHeader; ++r) {
		Vector<String> data = Split(lines[r], separator);
		for (int i = 0; i < numBest; ++i) {	
			if (r - beginHeader > 0)
				parameters[i] << "\n";
			if (i < data.size())
				parameters[i] << Trim(data[i]);
		}
	}
	
	return true;	
}

bool GuessCSV(const String &fileName, bool onlyNumbers, String &header, Vector<String> &parameters, char &separator, bool &repetition, char &decimalSign, int64 &beginData, int &beginDataRow) {
	FileIn in(fileName);
	if (!in)
		return false;
	return GuessCSVStream(in, onlyNumbers, header, parameters, separator, repetition, decimalSign, beginData, beginDataRow);
}

bool GuessCSV(const String &fileName, bool onlyNumbers, CSVParameters &param) {
	return GuessCSV(fileName, onlyNumbers, param.header, param.parameters, param.separator, param.repetition, param.decimalSign, param.beginData, param.beginDataRow);
}

bool GuessCSVStream(Stream &in, bool onlyNumbers, CSVParameters &param) {
	return GuessCSVStream(in, onlyNumbers, param.header, param.parameters, param.separator, param.repetition, param.decimalSign, param.beginData, param.beginDataRow);
}

String WriteCSV(Vector<Vector <Value> > &data, char separator, bool bycols, char decimalSign) {
	String ret;
	
	String _decimalSign(decimalSign, 1);
	
	if (bycols) {
		int maxr = 0;
		for (int c = 0; c < data.GetCount(); ++c) 
			maxr = max(maxr, data[c].GetCount());
			
		for (int r = 0; r < maxr; ++r) {
			if (r > 0)
				ret << "\n";
			for (int c = 0; c < data.GetCount(); ++c) {
				if (c > 0)
					ret << separator;
				if (r >= data[c].GetCount())
					continue;
				String str = ToStringDecimalSign(data[c][r], _decimalSign);
				if (str.Find(separator) >= 0)
					ret << '\"' << str << '\"';
				else
					ret << str;
			}
		}		
	} else {
		for (int r = 0; r < data.GetCount(); ++r) {
			if (r > 0)
				ret << "\n";
			for (int c = 0; c < data[r].GetCount(); ++c) {
				if (c > 0)
					ret << separator;
				String str = ToStringDecimalSign(data[r][c], _decimalSign);
				if (str.Find(separator) >= 0)
					ret << '\"' << str << '\"';
				else
					ret << str;
			}
		}
	}
	return ret;
}

bool WriteCSVFile(const String fileName, Vector<Vector <Value> > &data, char separator, bool bycols, char decimalSign) {
	String str = WriteCSV(data, separator, bycols, decimalSign);
	return SaveFile(fileName, str);
}


#ifdef PLATFORM_POSIX
String FileRealName(const char *_fileName) {
	String fileName = GetFullPath(_fileName);
	FindFile ff(fileName);
	if (!ff)
		return String(""); 
	else
		return fileName;
}
#endif
#if defined(PLATFORM_WIN32) || defined (PLATFORM_WIN64)
bool GetRealName_Next(String &real, String file) {
	bool ret;
	String old;
	int from = real.GetCount()+1;
	int to = file.Find(DIR_SEP, from);
	if (to >= 0) {
		old = file.Mid(from, to-from);
		ret = true;
	} else {
		old = file.Mid(from);
		ret = false;
	}
	real += DIR_SEP;
	FindFile ff(real + old);
	real += ff.GetName();
	return ret;
}

String FileRealName(const char *_fileName) {
	String fileName = GetFullPath(_fileName);
	int len = fileName.GetCount();
	
	if (len == 3) {
		FindFile ff(fileName + "*");
		if (!ff)
			return String(""); 	
		else
			return fileName;
	}
	FindFile ff(fileName);
	if (!ff)
		return String(""); 
	String ret;
	
	ret.Reserve(len);
	
	if (fileName.Left(2) == "\\\\")
		return String("");	// Not valid for UNC paths

	ret = ToUpper(fileName.Left(1)) + ":";
	
	while (GetRealName_Next(ret, fileName)) ;
	
	return ret;
}
#endif

#if defined(PLATFORM_WIN32) || defined (PLATFORM_WIN64)

#define Ptr Ptr_
#define byte byte_
#define CY win32_CY_

#include <winnls.h>
#include <winnetwk.h>

#include <wincon.h>
#include <shlobj.h>

#undef Ptr
#undef byte
#undef CY

#endif

bool IsSymLink(const char *path) {
#if defined(PLATFORM_WIN32) || defined (PLATFORM_WIN64)
	return GetFileExt(path) == ".lnk";
#else
	struct stat stf;
	lstat(path, &stf);
	return S_ISLNK(stf.st_mode);
#endif
}	

String GetNextFolder(const String &folder, const String &lastFolder) {
	int pos = lastFolder.Find(DIR_SEP, folder.GetCount()+1);
	if (pos >= 0)
		return lastFolder.Left(pos);
	else
		return lastFolder;
}

bool IsRootFolder(const char *folderName) {
	if (!folderName)
		return false;
	if (folderName[0] == '\0')
		return false;
#if defined(PLATFORM_WIN32) || defined (PLATFORM_WIN64)
	if (strlen(folderName) == 3 && folderName[1] == ':' && folderName[2] == DIR_SEP)
#else
	if (strlen(folderName) == 1 && folderName[0] == DIR_SEP)
#endif
		return true;
	return false;
}

String GetUpperFolder(const String &folderName) {
	if (IsRootFolder(folderName))
		return folderName;
	int len = folderName.GetCount();
	if (len == 0)
		return String();
	if (folderName[len-1] == DIR_SEP)
		len--;
	int pos = folderName.ReverseFind(DIR_SEP, len-1);
#if defined(PLATFORM_WIN32) || defined (PLATFORM_WIN64)
	if (pos == 2)
#else
	if (pos == 0)
#endif
		pos++;
	return folderName.Left(pos);
}

bool DeleteDeepWildcardsX(const char *pathwc, bool filefolder, EXT_FILE_FLAGS flags, bool deep)
{
	return DeleteDeepWildcardsX(GetFileFolder(pathwc), GetFileName(pathwc), filefolder, flags, deep);	
}

bool DeleteDeepWildcardsX(const char *path, const char *namewc, bool filefolder, EXT_FILE_FLAGS flags, bool deep)
{
	FindFile ff(AppendFileNameX(path, "*.*"));
	while(ff) {
		String name = ff.GetName();
		String full = AppendFileNameX(path, name);
		if (PatternMatch(namewc, name)) {
			if (ff.IsFolder() && !filefolder) {
				if (!DeleteFolderDeepX(full, flags)) 
					return false;
				Sleep(100);
			} else if (ff.IsFile() && filefolder) {
				if (!FileDeleteX(full, flags)) 
					return false;
			}
		} else if(deep && ff.IsFolder()) {
			if (!DeleteDeepWildcardsX(full, namewc, filefolder, flags))
				return false;
		}	
		ff.Next();
	}
	return true;
}

bool DeleteFolderDeepWildcardsX(const char *path, EXT_FILE_FLAGS flags) 	
{
	return DeleteDeepWildcardsX(path, false, flags, true);
}

bool DeleteFolderDeepWildcardsX(const char *path, const char *name, EXT_FILE_FLAGS flags) 	
{
	return DeleteDeepWildcardsX(path, name, false, flags, true);
}

bool DeleteFileDeepWildcardsX(const char *path, EXT_FILE_FLAGS flags) 	
{
	return DeleteDeepWildcardsX(path, true, flags, true);
}

bool DeleteFileWildcardsX(const char *path, EXT_FILE_FLAGS flags) 	
{
	return DeleteDeepWildcardsX(path, true, flags, false);
}

bool DeleteFolderDeepX_Folder(const char *dir, EXT_FILE_FLAGS flags)
{
	FindFile ff(AppendFileNameX(dir, "*.*"));
	while(ff) {
		String name = ff.GetName();
		String p = AppendFileNameX(dir, name);
		if(ff.IsFile())
			FileDeleteX(p, flags);
		else
		if(ff.IsFolder())
			DeleteFolderDeepX_Folder(p, flags);
		ff.Next();
	}
	return FolderDeleteX(dir, flags);
}

bool DeleteFolderDeepX(const char *path, EXT_FILE_FLAGS flags)
{
	if (flags & USE_TRASH_BIN)
		return FileToTrashBin(path);
	return DeleteFolderDeepX_Folder(path, flags);
}

bool RenameDeepWildcardsX(const char *path, const char *namewc, const char *newname, bool forfile, bool forfolder, EXT_FILE_FLAGS flags)
{
	FindFile ff(AppendFileNameX(path, "*.*"));
	while(ff) {
		String name = ff.GetName();
		String full = AppendFileNameX(path, name);
		if(ff.IsFolder()) {
			if (!RenameDeepWildcardsX(full, namewc, newname, forfile, forfolder, flags))
				return false;
		}
		if (PatternMatch(namewc, name)) {
			if ((ff.IsFolder() && forfolder) || (ff.IsFile() && forfile)) {
				if (!FileMoveX(full, AppendFileNameX(path, newname), flags)) 
					return false;
			}
		}
		ff.Next();
	}
	return true;
}

void DirectoryCopy_Each(const char *dir, const char *newPlace, String relPath, bool replaceOnlyNew, String filesToExclude, String &errorList)
{
	String dirPath = AppendFileNameX(dir, relPath);
	String newPath = AppendFileNameX(newPlace, relPath);
	LOG(dirPath);
	LOG(newPath);
	LOG (AppendFileNameX(dirPath, "*.*"));
	FindFile ff(AppendFileNameX(dirPath, "*.*"));
	while(ff) { 
		String name = ff.GetName();
		String newFullPath = AppendFileNameX(newPath, name);
		if(ff.IsFile()) {
			bool copy = !replaceOnlyNew;
			if (replaceOnlyNew) {
				Time newPathTime = FileGetTime(newFullPath);
				if (IsNull(newPathTime) || (newPathTime < Time(ff.GetLastWriteTime())))
					copy = true;
			}
			if (copy) {
				if (!PatternMatchMulti(filesToExclude, name)) {
					if (!FileCopy(ff.GetPath(), newFullPath))
						errorList << "\n" << Format(t_("Impossible to copy '%s' to '%s': %s"), ff.GetPath(), newFullPath, GetLastErrorMessage());
				}
			} 
		} else if (ff.IsFolder()) {
			if (!DirectoryExists(newFullPath)) {
				if (!DirectoryCreate(newFullPath))
					errorList << "\n" << Format(t_("Impossible to create directory '%s': %s"), newFullPath, GetLastErrorMessage());
			}
			DirectoryCopy_Each(dir, newPlace, AppendFileNameX(relPath, name), replaceOnlyNew, filesToExclude, errorList);
		}
		ff.Next();
	}
}

void DirectoryCopyX(const char *dir, const char *newPlace, bool replaceOnlyNew, String filesToExclude, String &errorList) {
	if (!DirectoryExists(dir)) 
		errorList << "\n" << Format(t_("Directory '%s' does not exist"), dir);
	else
		DirectoryCopy_Each(dir, newPlace, "", replaceOnlyNew, filesToExclude, errorList);
}

bool FolderIsEmpty(const char *path) {
	FindFile ff(AppendFileNameX(path, "*.*"));
	while(ff) {
		if(ff.IsFile() || ff.IsFolder())
			return false;
		ff.Next();
	}
	return true;
}

#if defined(__MINGW32__)
	#define _SH_DENYNO 0x40 
#endif

int FileCompare(const char *path1, const char *path2) {
	int fp1;
#ifdef PLATFORM_POSIX
	fp1 = open(ToSystemCharset(path1), O_RDONLY, S_IWRITE|S_IREAD);
#else
	fp1 = _wsopen(ToSystemCharsetW(path1), O_RDONLY|O_BINARY, _SH_DENYNO, _S_IREAD|_S_IWRITE);
#endif
	if (fp1 == -1)
		return -2;
	int fp2;
#ifdef PLATFORM_POSIX
	fp2 = open(ToSystemCharset(path2), O_RDONLY, S_IWRITE|S_IREAD);
#else
	fp2 = _wsopen(ToSystemCharsetW(path2), O_RDONLY|O_BINARY, _SH_DENYNO, _S_IREAD|_S_IWRITE);
#endif
	if (fp2 == -1) {
		close(fp1);
		return -2;	
	}
	Buffer <byte> c1(8192), c2(8192);
	int ret = -1;
	while (true) {
		int n1 = read(fp1, c1, 8192);
		int n2 = read(fp2, c2, 8192);
		if (n1 == -1 || n2 == -1) {
			ret = -2;
			break;
		}
		if (n1 != n2)
			break;
		if (memcmp(c1, c2, n1) != 0)
			break;
		if (n1 == 0) {
			ret = 1;
			break;
		}
	}
#ifdef PLATFORM_POSIX	
	if (-1 == close(fp1))
		ret = -2;
	if (-1 == close(fp2))
		ret = -2;
#else
	if (-1 == _close(fp1))
		ret = -2;
	if (-1 == _close(fp2))
		ret = -2;
#endif
	return ret;
}

int64 FindStringInFile(const char *file, const String text, int64 pos0) {
#ifdef PLATFORM_POSIX
	FILE *fp = fopen(file, "rb");
#else
	FILE *fp = _wfopen((const wchar_t *)String(file).ToWString().Begin(), L"rb");
#endif
	if (fp != NULL) {
		int64 pos = 0;
		if (pos0 > 0) {
			pos = pos0;
			if (0 == fseek(fp, long(pos0), SEEK_SET)) {
				fclose(fp);
				return -2;
			}
		}
		int i = 0, c;
		for (; (c = fgetc(fp)) != EOF; pos++) {
			if (c == text[i]) {
				++i;
				if (i == text.GetCount()) 
					return pos - i;
			} else {
				if (i != 0) 
					if (0 == fseek(fp, -(i-1), SEEK_CUR))
						return -2;
				i = 0;
			}
		}
		fclose(fp);
	} else
		return -2;
	return -1;	
}

bool MatchPathName(const char *name, const Vector<String> &cond, const Vector<String> &ext) {
	for (int i = 0; i < cond.GetCount(); ++i) {
		if(!PatternMatch(cond[i], name))
			return false;
	}
	for (int i = 0; i < ext.GetCount(); ++i) {
		if(PatternMatch(ext[i], name))
			return false;
	}
	return true;
}

void SearchFile_Each(String dir, const Vector<String> &condFiles, const Vector<String> &condFolders, 
								 const Vector<String> &extFiles,  const Vector<String> &extFolders, 
								 const String text, Vector<String> &files, Vector<String> &errorList) {
	FindFile ff;
	if (ff.Search(AppendFileNameX(dir, "*"))) {
		do {
			if(ff.IsFile()) {
				String name = AppendFileNameX(dir, ff.GetName());
				if (MatchPathName(ff.GetName(), condFiles, extFiles)) {
					if (text.IsEmpty())
						files.Add(name);
					else {
						switch(FindStringInFile(name, text)) {
						case 1:	files.Add(name);
								break;
						case -1:errorList.Add(AppendFileNameX(dir, ff.GetName()) + String(": ") + 
																	t_("Impossible to open file"));
								break;
						}
					}
				}
			} else if(ff.IsDirectory()) {
				String folder = ff.GetName();
				if (folder != "." && folder != "..") {
					String name = AppendFileNameX(dir, folder);
					if (MatchPathName(name, condFolders, extFolders)) 
						SearchFile_Each(name, condFiles, condFolders, extFiles, extFolders, text, files, errorList);
				}
			} 
		} while (ff.Next());
	}
}

Vector<String> SearchFile(String dir, const Vector<String> &condFiles, const Vector<String> &condFolders, 
								 const Vector<String> &extFiles,  const Vector<String> &extFolders, 
								 const String text, Vector<String> &errorList) {
	Vector<String> files;								     
	errorList.Clear();

	SearchFile_Each(dir, condFiles, condFolders, extFiles, extFolders, text, files, errorList);	
	
	return files;
}

Vector<String> SearchFile(String dir, String condFile, String text, Vector<String> &errorList)
{
	Vector<String> condFiles, condFolders, extFiles, extFolders, files;
	errorList.Clear();

	condFiles.Add(condFile);
	SearchFile_Each(dir, condFiles, condFolders, extFiles, extFolders, text, files, errorList);	

	return files;
}

Vector<String> SearchFile(String dir, String condFile, String text)
{
	Vector<String> errorList;
	Vector<String> condFiles, condFolders, extFiles, extFolders, files;
	
	condFiles.Add(condFile);
	SearchFile_Each(dir, condFiles, condFolders, extFiles, extFolders, text, files, errorList);	
	
	return files;
}
	
#if defined(PLATFORM_WIN32) || defined (PLATFORM_WIN64)
Value GetVARIANT(VARIANT &result)
{
	Value ret;
	switch(result.vt) {
	case VT_EMPTY:
	case VT_NULL:
	case VT_BLOB:
		break;
	case VT_BOOL:
         ret = result.boolVal;// ? "true" : "false";
         break;
   	case VT_I2:
         ret = result.iVal;
         break;
	case VT_I4:
		ret = static_cast<int64>(result.lVal); 
		break;
	case VT_R4:
		ret = AsString(result.fltVal);
		break;
	case VT_R8:
		ret = AsString(result.dblVal);
		break;
	case VT_BSTR:  
		ret = WideToString(result.bstrVal);
		break;
	case VT_LPSTR:
         //ret = result.pszVal;
         ret = "Unknown VT_LPSTR";
         break;
    case VT_DATE:
  		SYSTEMTIME stime;
     	VariantTimeToSystemTime(result.date, &stime);
     	{
	     	Time t;
	     	t.day    = static_cast<Upp::byte>(stime.wDay);
	     	t.month  = static_cast<Upp::byte>(stime.wMonth);
	     	t.year   = stime.wYear;
	     	t.hour   = static_cast<Upp::byte>(stime.wHour); 
	     	t.minute = static_cast<Upp::byte>(stime.wMinute);
	     	t.second = static_cast<Upp::byte>(stime.wSecond);		
			ret = t;
     	}
    	break;
 	case VT_CF:
     	ret = "(Clipboard format)";
     	break;
	}
	return ret;
}

String WideToString(LPCWSTR wcs, int len) {
	if (len == -1) {
		len = WideCharToMultiByte(CP_UTF8, 0, wcs, len, nullptr, 0, nullptr, nullptr);	
		if (len == 0)
			return Null;
	}
	Buffer<char> w(len);
	WideCharToMultiByte(CP_UTF8, 0, wcs, len, w, len, nullptr, nullptr);
	return ~w;	
}

bool StringToWide(String str, LPCWSTR &wcs) {
	wchar_t *buffer;
	DWORD size = MultiByteToWideChar(CP_UTF8, 0, str, -1, 0, 0);
	if (!(buffer = (wchar_t *)GlobalAlloc(GMEM_FIXED, sizeof(wchar_t) * size)))
		return false;

	MultiByteToWideChar(CP_UTF8, 0, str, -1, buffer, size);
	wcs = SysAllocString(buffer);
	GlobalFree(buffer);
	if (!wcs)
		return false;
	return true;
}

bool BSTRSet(const String str, BSTR &bstr) {
	wchar_t *buffer;
	DWORD size = MultiByteToWideChar(CP_UTF8, 0, str, -1, 0, 0);
	if (!(buffer = (wchar_t *)GlobalAlloc(GMEM_FIXED, sizeof(wchar_t) * size)))
		return false;

	MultiByteToWideChar(CP_UTF8, 0, str, -1, buffer, size);
	bstr = SysAllocString(buffer);
	GlobalFree(buffer);
	if (!bstr)
		return false;
	return true;
}

String BSTRGet(BSTR &bstr) {
	if (!bstr)
		return Null;
	
	char *buffer;
	DWORD size = SysStringLen(bstr);
	if (!(buffer = (char *)GlobalAlloc(GMEM_FIXED, sizeof(wchar_t) * size)))
		return Null;
	
	size_t i = wcstombs(buffer, bstr, size);
	buffer[i] = 0;
	
	String ret = buffer;
	GlobalFree(buffer);
	return ret;
}

#endif

#if defined(PLATFORM_WIN32) || defined (PLATFORM_WIN64)

Dl::~Dl() {
	if (hinstLib) 
		if (FreeLibrary(hinstLib) == 0)
			LOG(t_("Dl cannot be released"));
}

#ifndef LOAD_IGNORE_CODE_AUTHZ_LEVEL
	#define LOAD_IGNORE_CODE_AUTHZ_LEVEL	0x00000010
#endif

bool Dl::Load(const String &fileDll) {
	if (hinstLib) 
		if (FreeLibrary(hinstLib) == 0)
			return false;
	
	hinstLib = LoadLibraryEx(TEXT(ToSystemCharset(fileDll)), nullptr, LOAD_IGNORE_CODE_AUTHZ_LEVEL);
	return hinstLib != 0;
}

void Dl::Load_throw(const String &fileDll) {
	if (!Load(fileDll))
		throw Exc(GetErrorMessage(GetLastError()));
}

void *Dl::GetFunction(const String &functionName) const {
	if (!hinstLib) 
		return nullptr;
	return reinterpret_cast<void *>(GetProcAddress(hinstLib, functionName));
}

void *Dl::GetFunction_throw(const String &functionName) const {
	void *ret;
	if (!(ret = GetFunction(functionName)))
	 	throw Exc(Format("Dl::GetFunction: %s not found", functionName));
	return ret;
}
	

#else

#include <dlfcn.h>

Dl::~Dl() {
	if (hinstLib) 
		if (dlclose(hinstLib) == 0)
			;	// Dl cannot be released
}

bool Dl::Load(const String &fileDll) {
	if (hinstLib) 
		if (dlclose(hinstLib) == 0)
			return false;
	
	hinstLib = dlopen(fileDll, RTLD_LAZY);
	if (!hinstLib) 
		return false;
	return true;
}

void *Dl::GetFunction(const String &functionName) const {
	if (!hinstLib) 
		return nullptr;
	dlerror();				// Clear the errors
	void *ret = dlsym(hinstLib, functionName);
	if(!dlerror())			// If no error message, dlsym() went well
		return ret;
	else
		return nullptr;
}	

void *Dl::GetFunction_throw(const String &functionName) const {
	void *ret;
	if (!(ret = GetFunction(functionName)))
	 	throw Exc(Format("Dl::GetFunction: %s not found", functionName));
	return ret;
}

#endif

Color RandomColor() {
	return Color().FromRaw(Random());
}

Image GetRect(const Image& orig, const Rect &r) {
	if(r.IsEmpty())
		return Image(); 
	ImageBuffer ib(r.GetSize());
	for(int y = r.top; y < r.bottom; y++) {
		const RGBA *s = orig[y] + r.left;
		const RGBA *e = orig[y] + r.right;
		RGBA *t = ib[y - r.top];
		while(s < e) {
			*t = *s;
			t++;
			s++;
		}
	}
	return ib;
}


double tmGetTimeX() {
#ifdef __linux__
	timespec t;
	if (0 != clock_gettime(CLOCK_REALTIME, &t))
		return Null;
	return t.tv_sec + (double)t.tv_nsec/1000000000.;
#elif defined(_WIN32) || defined(WIN32)
	LARGE_INTEGER clock;
	LARGE_INTEGER freq;
	if(!QueryPerformanceCounter(&clock) || !QueryPerformanceFrequency(&freq))
	    return Null;
	return double(clock.QuadPart)/freq.QuadPart;
#else
	return double(time(0));		// Low resolution
#endif
}


int SysX(const char *cmd, String& out, String& err, double timeOut, 
			Gate3<double, String&, String&> progress, bool convertcharset) {
	out.Clear();
	LocalProcess p;
	p.ConvertCharset(convertcharset);
	double t0 = tmGetTimeX();
	if(!p.Start2(cmd))
		return -1;
	int ret = Null;
	String os, es;
	while(p.IsRunning()) {
		if (p.Read2(os, es)) {
			out.Cat(os);
			err.Cat(es);
		}
		double elapsed = tmGetTimeX() - t0;
		if (!IsNull(timeOut) && elapsed > timeOut) {
			ret = -2;
			break;
		}
		if (progress(elapsed, out, err)) {
			ret = -3;
			break;
		}
		Sleep(1);
	}
	out.Cat(os);
	err.Cat(es);
	if (!IsNull(ret))
		p.Kill();
		
	return IsNull(ret) ? 0 : ret;
}


int LevenshteinDistance(const char *s, const char *t) {
	int lens = int(strlen(s));
	int lent = int(strlen(t));
	
    Buffer<int> v0(lent + 1);
    Buffer<int> v1(lent + 1);

    for (int i = 0; i <= lent; ++i)
        v0[i] = i;

    for (int i = 0; i < lens; ++i) {
        v1[0] = i + 1;

        for (int j = 0; j < lent; ++j) {
            int deletionCost = v0[j + 1] + 1;
            int insertionCost = v1[j] + 1;
            int substitutionCost;
            if (s[i] == t[j])
                substitutionCost = v0[j];
            else
                substitutionCost = v0[j] + 1;

            v1[j + 1] = min(deletionCost, insertionCost, substitutionCost);
        }
        Swap(v0, v1);
   	}
    return v0[lent];
}

int DamerauLevenshteinDistance(const char *s, const char *t, int alphabetLength) {
	int lens = int(strlen(s));
	int lent = int(strlen(t));
	int lent2 = lent + 2;
	Buffer<int> H((lens+2)*lent2);  
	
    int infinity = lens + lent;
    H[0] = infinity;
    for(int i = 0; i <= lens; i++) {
		H[lent2*(i+1)+1] = i;
		H[lent2*(i+1)+0] = infinity;
    }
    for(int j = 0; j <= lent; j++) {
		H[lent2*1+(j+1)] = j;
		H[lent2*0+(j+1)] = infinity;
    }      
    Buffer<int> DA(alphabetLength, 0);
   
    for(int i = 1; i <= lens; i++) {
      	int DB = 0;
      	for(int j = 1; j <= lent; j++) {
	        int i1 = DA[int(t[j-1])];
	        int j1 = DB;
	        int cost = (s[i-1] == t[j-1]) ? 0 : 1;
	        if(cost == 0) 
	        	DB = j;
	        H[lent2*(i+1)+j+1] =
	          min(H[lent2*i     + j] + cost,
	              H[lent2*(i+1) + j] + 1,
	              H[lent2*i     + j+1] + 1, 
	              H[lent2*i1    + j1] + (i-i1-1) + 1 + (j-j1-1));
	  	}
      	DA[int(s[i-1])] = i;
    }
    return H[lent2*(lens+1)+lent+1];
}

int SentenceSimilitude(const char *s, const char *t) {
	int ls = int(strlen(s));
	int lt = int(strlen(t));
	if (ls > lt) {
		Swap(s, t);
		Swap(ls, lt);
	}
	int mind = ls;
	for (int i = 0; i < t - s; ++i) {
		int d = DamerauLevenshteinDistance(s, String(t).Mid(i, ls));
		if (d < mind)
			mind = d;
	}
	return (100*mind)/ls;
}
  
// Dummy functions added after TheIDE change
Upp::String GetCurrentMainPackage() {return "dummy";}
Upp::String GetCurrentBuildMethod()	{return "dummy";}
void IdePutErrorLine(const Upp::String& ) {}

size_t GetNumLines(Stream &stream) {
	size_t res = 0;
	int c;

	if ((c = stream.Get()) < 0)
		return 0;
	if (c == '\n')
		res++;
	while ((c = stream.Get()) > 0)
		if (c == '\n')
			res++;
	if (c != '\n')
		res++;	
	return res;	
}


bool SetConsoleColor(CONSOLE_COLOR color) {
	static Vector<int> colors;
	if (color == RESET)
		colors.Clear();
	else if(color == PREVIOUS) {
		int num = colors.size();
		if (num == 0)
			color = RESET;
		else {
			colors.Remove(num-1);
			if (num-2 < 0) 
				color = RESET;
			else
				color = static_cast<CONSOLE_COLOR>(colors[num-2]); 
		}
	} else
		colors << color;

#ifdef PLATFORM_WIN32
	static HANDLE hstdout = 0;
	//static CONSOLE_SCREEN_BUFFER_INFO csbiInfo = {};
	static WORD woldattrs;
	
	if (hstdout == 0) {
		hstdout = GetStdHandle(STD_OUTPUT_HANDLE);
		//if (!GetConsoleScreenBufferInfo(hstdout, &csbiInfo)) {
		//	hstdout = 0;
		//	return false;
		//}
		woldattrs = FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED;  //csbiInfo.wAttributes;
	}
	if (color == RESET) 
		return SetConsoleTextAttribute(hstdout, woldattrs);
	else
		return SetConsoleTextAttribute(hstdout, color);
#else
	if (color < 100)
		printf("\x1b[%dm", color);
	else
		printf("\x1b[1;%dm", color-100);	
	return true;
#endif
}

void ConsoleOutputDisable(bool disable) {
#ifdef PLATFORM_WIN32
	if (disable)
		freopen("nul", "w", stdout);
	else
		freopen("CONOUT$", "w", stdout); 
#else
	static int saved_id = Null;
	static fpos_t saved_pos;
	
	if (disable) {
		fflush(stdout);
		fgetpos(stdout, &saved_pos);
		saved_id = dup(fileno(stdout));
		close(fileno(stdout));
	} else {
		if (!IsNull(saved_id)) {
			fflush(stdout);
			dup2(saved_id, fileno(stdout));
			close(saved_id);
			clearerr(stdout);
			fsetpos(stdout, &saved_pos); 
		}
	}
#endif
}

static void ListArgsCFunction(const String &strargs, const Vector <String> &ctypes, 
						Vector<int> &argTypeId, Vector<String> &argVars) {
	Vector<String> args = Split(strargs, ",");
	argTypeId.Clear();
	argVars.Clear();
	for (auto &arg : args) {
		arg = Trim(arg);
		int j;
		for (j = 0; j < ctypes.size(); ++j) {
			if (arg.StartsWith(ctypes[j])) 
				break;
		}
		if (j >= ctypes.size())
			throw Exc(Format("Type in argument '%s' not found", arg));
		argTypeId << j;
		argVars   << Trim(arg.Mid(ctypes[j].GetCount()));
	}
}

static String ToArgs(const Vector<String> &args) {
	String ret;
	for (int i = 0; i < args.size(); ++i) {
		if (i > 0)
			ret << ", ";
		ret << args[i];
	}
	return ret;	
}

String GetPythonDeclaration(const String &name, const String &include) {
	static String str;
	const Vector<String> ctypes = {"double **", 									  "int *", 						 "int",    		  "double",   		 "const char *", 	"bool",   		 "const double *"}; 
	const Vector<String> ptypes = {"ctypes.POINTER(ctypes.POINTER(ctypes.c_double))", "ctypes.POINTER(ctypes.c_int)", "ctypes.c_int", "ctypes.c_double", "ctypes.c_char_p", "ctypes.c_bool", "np.ctypeslib.ndpointer(dtype=np.float64)"}; 
	const Vector<bool> isPy_C   = {false, 											  false, 						 false,    		  false, 	   		 false,          	false, 	 		 true};
	const Vector<bool> isC_Py   = {true, 											  false, 						 false,    		  false, 	   		 false,          	false, 	 		 false};
	
	str << "# " << name << " python functions list\n\n"
		   "class " << name << ":\n"
		   "    def __init__(self, path_dll):\n"
		   "        self.libc = ctypes.CDLL(dll)\n\n";
		   
	String strIn  = "        # INPUT TYPES\n";
	String strOut = "        # OUTPUT TYPES\n";
	String strFun;
	
	String cleaned = CleanCFromDeclaration(include);
					
	Vector<String> lines = Split(cleaned, "\n");
	for (const auto &line : lines) {
		int pospar = line.Find("(");
		String function;
		
		for (int i = 0; i < ctypes.size(); ++i) {
			const auto &type = ctypes[i];
			if (line.StartsWith(type)) {
				function = Trim(line.Mid(type.GetCount(), pospar - type.GetCount()));
				strOut << "        self.libc." << function << ".restype = " << ptypes[i] << "\n";
				break;
			} 
		}
		if (function.IsEmpty() && line.StartsWith("void")) 
			function = Trim(line.Mid(String("void").GetCount(), pospar - String("void").GetCount()));
		
		if (function.IsEmpty())
			continue;
			
		int posparout = line.Find(")");
		String strargs = line.Mid(pospar+1, posparout - pospar-1);
		
		Vector<int> argTypeId;
		Vector<String> argVars;
		ListArgsCFunction(strargs, ctypes, argTypeId, argVars);
							
		if (argTypeId.IsEmpty()) 
			continue;
		
		Vector<String> pargs, cargs, pargTypes;
		String pre, post, returns;
		int idata = 0;
		for (int i = 0; i < argTypeId.size(); ++i) {
			pargTypes << ptypes[argTypeId[i]];
			if (isC_Py[argTypeId[i]]) {
				cargs << Format("ctypes.byref(_data%d)", idata);
				//pargs << argVars[i];
				pre  << Format("        _data%d = ctypes.POINTER(ctypes.c_double)()\n", idata)
        			 << Format("        _size%d = ctypes.c_int()\n", idata);
        		post << Format("        _arraySize%d = ctypes.c_double * _size%d.value\n", idata, idata)
        			 << Format("        _data%d_pointer = ctypes.cast(_data%d, ctypes.POINTER(_arraySize%d))\n", idata, idata, idata)
        			 << Format("		%s = np.frombuffer(_data%d_pointer.contents)\n", argVars[i], idata);
        		returns << ", " << argVars[i];
			} else if (isPy_C[argTypeId[i]]) {
				cargs << argVars[i];
				pargs << argVars[i];
			} else {
				if (i > 0 && isC_Py[argTypeId[i-1]]) {
					cargs << Format("ctypes.byref(_size%d)", idata);
					idata++;
				} else if (i > 0 && isPy_C[argTypeId[i-1]])
					cargs << Format("len(%s)", argVars[i-1]);
				else {
					if (ctypes[argTypeId[i]] == "const char *")
						cargs << Format("str.encode(%s, 'UTF-8')", argVars[i]);
					else
						cargs << argVars[i];
					pargs << argVars[i];
				}
			}
		}
		strIn << Format("        self.libc.%s.argtypes = [%s]\n", function, ToArgs(pargTypes));
				
		String fname = function;
		fname.Replace("DLL_", "");
		strFun << "    def " << fname << "(self";
		if (!pargs.IsEmpty())
			strFun << ", " << ToArgs(pargs);
		strFun << "):\n";
		if (!pre.IsEmpty()) 
			strFun << "        # Argument preparation\n" 
				   << pre 
				   << "        # DLL function call\n"
				   << Format("        ret = self.libc.%s(%s)\n", function, ToArgs(cargs));
		else
			strFun << Format("        self.libc.%s(%s)\n", function, ToArgs(cargs));
		if (!post.IsEmpty()) 
			strFun << "        # Vector processing\n" 
				   << post 
				   << "        return ret" << returns << "\n";
		
		strFun << "\n";
	}
	str << strIn << "\n" << strOut << "\n" << strFun;
	
	return str = Trim(str);	
}

String CleanCFromDeclaration(const String &include, bool removeSemicolon) {
	String str = include;
	
	str.Replace("	__declspec(dllexport) ", "");
	str.Replace("extern \"C\" {", "");
	str.Replace("};", "");
	str.Replace("\r\n\r\n", "\r\n");
	str.Replace(" noexcept", "");
	str.Replace("  ", " ");
	str.Replace("\t", " ");
	str.Replace(" ;", ";");
	str.Replace(" (", "(");
	str.Replace(" )", ")");
	
	if (removeSemicolon) 
		str.Replace(");", ")");
	
	return str;
}

String S(const char *s) 	{return s;}
String S(const Value &v) 	{return v.ToString();}

bool CoutStreamX::noprint = false;

Stream& CoutX() {
	return Single<CoutStreamX>();
}


void Grid::ColWidths(const Vector<int> &colWidths) {
	widths = clone(colWidths);
}

Grid &Grid::AddCol(int colWidth) {
	widths << colWidth;
	actualCol = widths.size()-1;
	return *this;
}
	
Grid& Grid::Set(int row, int col, const Value &data) {
	if (!IsNull(col))
		actualCol = col;
	if (!IsNull(row))
		actualRow = row;
	
	if (actualCol >= columns.size()) {
		columns.SetCount(actualCol+1);
		widths.SetCount(actualCol+1, defaultWidth);
	}
	if (actualRow >= columns[actualCol].size()) {
		for (int c = 0; c < columns.size(); ++c)
			if (columns[c].size() <= actualRow)
				columns[c].SetCount(actualRow+1);
	}
	columns[actualCol][actualRow] = data;
	
	return *this;
}

Grid& Grid::Set(const Value &data) {
	Set(Null, Null, data);	
	actualCol++;
	return *this;
}

Grid& Grid::SetRow(const Vector<Value> &data) {
	actualCol = 0;
	
	for (int c = 0; c < data.size(); ++c) {
		Set(Null, Null, data[c]);
		actualCol++;
		if (widths.size() <= actualCol)
			widths.SetCount(actualCol+1, defaultWidth);
	}
	actualRow++;
	actualCol = 0;
	
	return *this;
}

const Value &Grid::Get(int row, int col) const {
	static const Value nil;
	if (col >= columns.size() || row >= columns[col].size())
		return nil;
	return columns[col][row];
}

int Grid::rows(int col) const {
	if (vnum > 0)
		return vnum;
	if (columns.IsEmpty())
		return 0;
	return columns[col].size();
}

int Grid::cols() const {
	if (isVirtual)
		return vheader.size();
	return columns.size();
}
	
String Grid::AsString(bool format, bool removeEmpty, const String &separator) {
	String ret;
	for (int r = 0; r < columns[0].size(); ++r) {
		bool printRow;
		if (removeEmpty) {			// Doesn't show empty rows
			printRow = false;
			for (int c = 0; c < columns.size(); ++c) {
				if (!columns[c][r].IsNull()) {
					printRow = true;	
					break;
				}
			}
		} else
			printRow = true;
		if (printRow) {
			for (int c = 0; c < columns.size(); ++c) {
				if (format) {		// Add spaces to maintain the position of each column
					String str = columns[c][r].ToString();
					if (widths[c] >= str.GetCount()) {
						String spaces = String(' ', widths[c] - str.GetCount());
						if (columns[c][r].Is<String>())
							ret << str << spaces;
						else
							ret << spaces << str;
					} else {
						int lenLeft = widths[c]/2;
						int lenRight = widths[c] - lenLeft;
						ret << str.Left(lenLeft - 1) << S("***") << str.Right(lenRight - 2);
					}
				} else
					ret << columns[c][r];
				if (c < columns.size()-1)
					ret << separator;
			}
			if (r < columns[0].size()-1)
				ret << "\n";
		}
	}
	return ret;
}

String Grid::AsLatex(bool removeEmpty, bool setGrid) {
	String ret;
	for (int r = 0; r < columns[0].size(); ++r) {
		bool printRow;
		if (removeEmpty) {			// Doesn't show empty rows
			printRow = false;
			for (int c = 0; c < columns.size(); ++c) {
				if (!columns[c][r].IsNull()) {
					printRow = true;	
					break;
				}
			}
		} else
			printRow = true;
		if (printRow) {
			for (int c = 0; c < columns.size(); ++c) {
				String str = columns[c][r].ToString();
				str = Replace(str, "%", "\\%");
				const Grid::Fmt &fmt = GetFormat(r, c);
				if (fmt.fnt.IsBold())
					str = "\\textbf{" + str + "}";
				if (fmt.fnt.IsItalic())
					str = "\\textit{" + str + "}";
				if (!IsNull(fmt.text)) {
					String col = FormatIntHex(fmt.text.GetR(), 2) + FormatIntHex(fmt.text.GetG(), 2) + FormatIntHex(fmt.text.GetB(), 2);
					str = "\\textcolor[HTML]{" + col + "}{" + str + "}";
				}
				if (!IsNull(fmt.back)) {
					String col = FormatIntHex(fmt.back.GetR(), 2) + FormatIntHex(fmt.back.GetG(), 2) + FormatIntHex(fmt.back.GetB(), 2);
					str += "\\cellcolor[HTML]{" + col + "}";
				}
				if (setGrid) {
					int al = GetAlignment(r, c);
					String align;
					switch (al) {
					case ALIGN_LEFT:	align = "l";	break;
					case ALIGN_CENTER:	align = "c";	break;
					case ALIGN_RIGHT:	align = "r";	break;
					default:			align = "l";	break;
					}
					ret << Format("\\multicolumn{1}{|" + align + "|}{%s}", str);
				} else
					ret << str;
				if (c < columns.size()-1)
					ret << " & ";
			}
			if (r < columns[0].size()-1)
				ret << " \\\\ \\hline\n";
		}
	}
	return ret;
}

Grid& Grid::Set(int row, int col, const Grid::Fmt &fmt) {
	if (!IsNull(col))
		actualCol = col;
	if (!IsNull(row))
		actualRow = row;
	
	if (actualCol >= columns.size()) {
		columns.SetCount(actualCol+1);
		widths.SetCount(actualCol+1, defaultWidth);
	}
	if (actualRow >= columns[actualCol].size()) {
		for (int c = 0; c < columns.size(); ++c)
			if (columns[c].size() <= actualRow)
				columns[c].SetCount(actualRow+1);
	}
	formatCell.GetAdd(Tuple<int, int>(actualRow, actualCol)) = clone(fmt);
	
	return *this;
}

Grid& Grid::Set(const Grid::Fmt &fmt) {
	Set(Null, Null, fmt);		// Grid is not changed
	return *this;
}

const Grid::Fmt &Grid::GetFormat(int row, int col) const {
	int id = formatCell.Find(Tuple<int, int>(row, col));
	if (id >= 0) 
		return formatCell[id];	
	for (int i = 0; i < formatRange.size(); ++i) {
		const Tuple<int, int, int, int> &tp = formatRange.GetKey(i);
		if ((IsNull(tp.a) || IsNull(tp.c) || Between(row, tp.a, tp.c)) && (IsNull(tp.b) || IsNull(tp.d) || Between(col, tp.b, tp.d)))
			return formatRange[i];
	}
	static const Grid::Fmt dummy;
	return dummy;
}

Grid& Grid::Set(int row, int col, Alignment align) {
	if (!IsNull(col))
		actualCol = col;
	if (!IsNull(row))
		actualRow = row;
	
	if (actualCol >= columns.size()) {
		columns.SetCount(actualCol+1);
		widths.SetCount(actualCol+1, defaultWidth);
	}
	if (actualRow >= columns[actualCol].size()) {
		for (int c = 0; c < columns.size(); ++c)
			if (columns[c].size() <= actualRow)
				columns[c].SetCount(actualRow+1);
	}
	alignCell.GetAdd(Tuple<int, int>(actualRow, actualCol)) = align;
	
	return *this;
}

Grid& Grid::Set(Alignment align) {
	Set(Null, Null, align);		// Grid is not changed
	return *this;
}

Alignment Grid::GetAlignment(int row, int col) const {
	int id = alignCell.Find(Tuple<int, int>(row, col));
	if (id >= 0) 
		return (Alignment)alignCell[id];	
	for (int i = 0; i < alignRange.size(); ++i) {
		const Tuple<int, int, int, int> &tp = alignRange.GetKey(i);
		if ((IsNull(tp.a) || IsNull(tp.c) || Between(row, tp.a, tp.c)) && (IsNull(tp.b) || IsNull(tp.d) || Between(col, tp.b, tp.d)))
			return (Alignment)alignRange[i];
	}
	return ALIGN_NULL;
}


}
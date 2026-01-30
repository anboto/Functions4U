// SPDX-License-Identifier: Apache-2.0
// Copyright 2021 - 2024, the Anboto author and contributors
#include <Core/Core.h>
#include "Functions4U.h"

#define TFILE <Functions4U/Functions4U.t>
#include <Core/t.h>


namespace Upp {

String GetSourceFolder() {
	String ret = GetEnv("UPP_MAIN__");
	if (ret.IsEmpty())
		return String::GetVoid();
	int c = ret[ret.GetCount()-1];
	if (c == '\\' || c == '/')
		return ret.Left(ret.GetCount()-1);
	return ret;
}

bool IsTheIDE() {return !GetEnv("UPP_MAIN__").IsEmpty();}

#if defined(PLATFORM_WIN32) || defined (PLATFORM_WIN64)
static String WinLastError() {
	LPVOID lpMsgBuf;
	String ret;
	
    FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
        		  FORMAT_MESSAGE_IGNORE_INSERTS,
        		  NULL, ::GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        		  reinterpret_cast<LPTSTR>(&lpMsgBuf), 0, NULL);
  	ret = static_cast<char *>(lpMsgBuf);
  	LocalFree(lpMsgBuf);	
	return ret;
}
#endif


FileDataArray::FileDataArray(bool _useId, int _fileFlags) {
	Clear();
	useId = _useId;
	fileFlags = _fileFlags;
}

bool FileDataArray::Init(FileDataArray &orig, FileDiffArray &diff) {
	basePath = orig.basePath;
	fileCount = orig.fileCount;
	folderCount = orig.folderCount;
	fileSize = orig.fileSize;
	useId = orig.useId;
	fileList.SetCount(orig.size());
	for (int i = 0; i < orig.size(); ++i)
		fileList[i] = orig[i];
	
	for (int i = 0; i < diff.GetCount(); ++i) {
		long id;
		if (diff[i].action != 'n') {
		 	id = Find(diff[i].relPath, diff[i].fileName, diff[i].isFolder);
			if (id < 0)
				return false;
		}
		switch(diff[i].action) {
		case 'u':
			fileList[id].id = diff[i].idMaster;
			fileList[id].length = diff[i].lengthMaster;
			fileList[id].t = diff[i].tMaster;
			break;
		case 'n':
			fileList.Add(FileData(diff[i].isFolder, diff[i].fileName, diff[i].relPath, diff[i].lengthMaster, diff[i].tMaster, diff[i].idMaster));
			if (diff[i].isFolder) 
				folderCount++;
			else
				fileCount++;
			break;
		case 'd':
			fileList.Remove(id);
			if (diff[i].isFolder) 
				folderCount--;
			else
				fileCount--;
			break;
		case 'p':
			SetLastError(t_("Problem found"));		// To Fix				
			//return false;
		}
	}
	return true;
}

void FileDataArray::Clear() {
	fileList.Clear();
	errorList.Clear();
	fileCount = folderCount = 0;
	fileSize = 0;
	basePath = "";
}

bool FileDataArray::Search(const String &dir, const String &condFile, bool recurse, const String &findText, Function<bool(uint64, uint64)> Print, bool files, bool folders) {
	Clear();
	Vector<String> list;
	
	if (fileFlags & BROWSE_LINKS) {
		if (IsSymLink(dir)) {
			basePath = GetSymLinkPath(dir);
			list << basePath;
		} else {
			basePath = dir;
			list << dir;
		}
	} else {
		basePath = dir;
		list << dir;
	}
	for (uint64 n = 0; !list.IsEmpty(); n++) {
		Search_Each(list, condFile, recurse, findText, files, folders);
		if (Print && !(n%100))
			Print(fileCount, folderCount); 
	}
	return errorList.IsEmpty();
}

void FileDataArray::Search_Each(Vector<String> &list, const String &condFile, bool recurse, const String &findText, bool files, bool folders) {
	FindFile ff;
	
	String dir = pick(list[list.size()-1]);
	list.Remove(list.size()-1);  
	
	if (ff.Search(AppendFileNameX(dir, condFile))) {
		do {
			if (files && ff.IsFile()) {
				String p = AppendFileNameX(dir, ff.GetName());
				//if (ff.IsSymLink()) { 
				//	p = p;
				//}	
				/*
					fileList.Add(FileData(true, ff.GetName(), GetRelativePath(dir), 0, ff.GetLastWriteTime(), 0));
					folderCount++;
					if (recurse)
						Search_Each(p, condFile, recurse, text);
				} else */ 
				if (findText.IsEmpty()) {
					uint64 len = (uint64)ff.GetLength();
					fileList << FileData(false, ff.GetName(), GetRelativePath(dir), len, ff.GetLastWriteTime(), 
											(useId && len > 0) ? GetFileId(p) : 0);
					fileCount++;
					fileSize += len;
				} else {
					FILE *fp = fopen(p, "r");
					if (fp != NULL) {
						int i = 0, c;
						while ((c = fgetc(fp)) != EOF) {
							if (c == findText[i]) {
								++i;
								if (i == findText.GetCount()) {
									uint64 len = (uint64)ff.GetLength();
									fileList << FileData(false, ff.GetName(), GetRelativePath(dir), len, ff.GetLastWriteTime(), useId ? GetFileId(p) : 0);
									fileCount++;
									fileSize += len;
									break;
								}
							} else {
								if (i != 0) 
									fseek(fp, -(i-1), SEEK_CUR);
								i = 0;
							}
						}
						fclose(fp);
					} else
						errorList.Add(AppendFileNameX(dir, ff.GetName()) + String(": ") + t_("Impossible to open file"));
				}
			} 
		} while (ff.Next()); 
	}
	if (ff.Search(AppendFileNameX(dir, "*"))) {
		do {
			String name = ff.GetName();
			if(folders && ff.IsDirectory() && name != "." && name != "..") {
				String p = AppendFileNameX(dir, name);
				fileList << FileData(true, name, GetRelativePath(dir), 0, ff.GetLastWriteTime(), 0);
				folderCount++;
				if (recurse)
					list << p;
			}
		} while (ff.Next()); 
	}
}

uint64 FileDataArray::GetFileId(String fileName) {
	uint64 id = 0;
#ifdef PLATFORM_POSIX
	FILE *fp = fopen(fileName, "rb");
#else
	FILE *fp = _wfopen((const wchar_t *)fileName.ToWString().Begin(), L"rb");
#endif
	if (fp != NULL) {
		int c;
		uint64 i = 0;
		while ((c = fgetc(fp)) != EOF) {
			id += (uint64)c*i;
			i++;
		}
		fclose(fp);
	}
	return id;
}

static bool SortFileData(const FileData& a, const FileData& b, char fileDataSortBy, char fileDataSortAscending) {
	if (fileDataSortBy == 'n') {
		int aint = atoi(a.fileName);
		int bint = atoi(b.fileName);
		if (!IsNull(aint) && !IsNull(bint)) {
			if (fileDataSortAscending)
				return aint < bint;
			else
				return aint > bint;
		}
		if (!IsNull(aint))
			return fileDataSortAscending;
		if (!IsNull(bint))
			return !fileDataSortAscending;

		if (fileDataSortAscending)
#ifdef PLATFORM_POSIX				
			return a.fileName < b.fileName; 
		else
			return a.fileName > b.fileName; 
#else
			return ToLower(a.fileName) < ToLower(b.fileName); 
		else
			return ToLower(a.fileName) > ToLower(b.fileName); 
#endif
	} else if (fileDataSortBy == 'd') {
		if (fileDataSortAscending)
			return a.t < b.t;
		else 
			return a.t > b.t;
	} else {
		if (fileDataSortAscending)
			return a.length < b.length;
		else 
			return a.length > b.length;
	}
}

static bool SortFileDataByNameAscending(const FileData& a, const FileData& b) 	{return SortFileData(a, b, 'n', true);}
static bool SortFileDataByNameDescending(const FileData& a, const FileData& b) 	{return SortFileData(a, b, 'n', false);}

void FileDataArray::SortByName(bool ascending) {
	if (ascending)
		Sort(fileList, SortFileDataByNameAscending);
	else
		Sort(fileList, SortFileDataByNameDescending);
}

static bool SortFileDataByDateAscending(const FileData& a, const FileData& b) 	{return SortFileData(a, b, 'd', true);}
static bool SortFileDataByDateDescending(const FileData& a, const FileData& b) 	{return SortFileData(a, b, 'd', false);}

void FileDataArray::SortByDate(bool ascending) {
	if (ascending)
		Sort(fileList, SortFileDataByDateAscending);
	else
		Sort(fileList, SortFileDataByDateDescending);
}

static bool SortFileDataBySizeAscending(const FileData& a, const FileData& b) 	{return SortFileData(a, b, 's', true);}
static bool SortFileDataBySizeDescending(const FileData& a, const FileData& b) 	{return SortFileData(a, b, 's', false);}

void FileDataArray::SortBySize(bool ascending) {
	if (ascending)
		Sort(fileList, SortFileDataBySizeAscending);
	else
		Sort(fileList, SortFileDataBySizeDescending);
}


bool CheckFileData(FileData &data, String &, String &, const String &lowrelFileName, const String &lowfileName, bool isFolder) {
	if (data.isFolder == isFolder) {
		if (ToLower(data.fileName) == lowfileName) {
			if (ToLower(data.relFilename) == lowrelFileName) 
	    		return true;
		}
	}	
	return false;
}

int FileDataArray::Find(String &relFileName, String &fileName, bool isFolder) {
	String lowrelFileName = ToLower(relFileName);
	String lowfileName = ToLower(fileName);
	for (int i = 0; i < fileList.GetCount(); ++i) {
		if (CheckFileData(fileList[i], relFileName, fileName, lowrelFileName, lowfileName, isFolder))
		    return i;
	}
	return -1;
}

/*
int FileDataArray::Find(FileDataArray &data, int id) {
	return Find(data[id].relFilename, data[id].fileName, data[id].isFolder);
}
*/

int FileDataArray::Find(FileDataArray &data, int id) {
	String lowrelFileName = ToLower(data[id].relFilename);
	String lowfileName = ToLower(data[id].fileName);
	bool isFolder = data[id].isFolder;
	
	int num = fileList.GetCount();
	if (num == 0)
		return -1;
	if (num == 1) {
		if (CheckFileData(fileList[0], data[id].relFilename, data[id].fileName, lowrelFileName, lowfileName, isFolder))
			return 0;
		else
			return -1;
	}
	int down, up;
	down = id < num-1 ? id : num-2;
	up = down + 1; 
	while (down >= 0 || up < num) {
		if (down >= 0) {
			if (CheckFileData(fileList[down], data[id].relFilename, data[id].fileName, lowrelFileName, lowfileName, isFolder))
		    	return down;
			down--;
		}
		if (up < num) {
			if (CheckFileData(fileList[up], data[id].relFilename, data[id].fileName, lowrelFileName, lowfileName, isFolder))
		    	return up;
			up++;
		}
	}
	return -1;
}

String FileDataArray::ToString() {
	if (fileList.IsEmpty())
		return String("");
	
	String ret;
	String sp(sep, 1);
	
	ret << "Folder" << sp << "File" << sp << "Ext" << sp << "IsFolder" << sp << "Size" << sp 
		<< "Year" << sp << "Month" << sp << "Day" << sp << "Hour" << sp << "Min" << sp << "Sec" << sp 
		<< "Id" << sp << "#char" << sp << "#Total char (max. 400)";
	for (int i = 0; i < fileList.GetCount(); ++i) {
		ret << "\n";
		ret << "\"" << fileList[i].relFilename << "\"" << sp;
		if (fileList[i].isFolder) {
			ret << "\"" << fileList[i].fileName << "\"" << sp;
			ret << sp;
		} else {
			String ext = GetFileExt(fileList[i].fileName);
			if (ext.GetCount() > 6) {		// Too long to be an extension
				ret << "\"" << fileList[i].fileName << "\"" << sp;
				ret << sp;	
			} else {
				ret << "\"" << GetFileTitle(fileList[i].fileName) << "\"" << sp;
				ret << ext << sp;
			}
		}
		ret << fileList[i].isFolder << sp;
		ret << fileList[i].length << sp;
		ret << FormatInt(fileList[i].t.year) << sp;
		ret << FormatInt(fileList[i].t.month) << sp;
		ret << FormatInt(fileList[i].t.day) << sp;
		ret << FormatInt(fileList[i].t.hour) << sp;
		ret << FormatInt(fileList[i].t.minute) << sp;
		ret << FormatInt(fileList[i].t.second) << sp;
		ret << fileList[i].id << sp;
		int len = fileList[i].relFilename.GetCount() + fileList[i].fileName.GetCount() + 1;
		ret << len << sp;
		ret << (len + basePath.GetCount() + 1) << sp;
	}
	return ret;	
}

String FileDataArray::GetErrorText() {
	String ret;
	
	ret << "Error list";
	for (int i = 0; i < fileList.GetCount(); ++i) {
		ret << "\n";
		ret << errorList[i];
	}
	return ret;	
}

bool FileDataArray::SaveFile(const char *fileName) {
	if (fileList.IsEmpty())
		throw Exc("No data found");
		
	return Upp::SaveFileBOMUtf8(fileName, ToString());
}

bool FileDataArray::SaveErrorFile(const char *fileName) {
	return Upp::SaveFileBOMUtf8(fileName, GetErrorText());
}

bool FileDataArray::AppendFile(const char *fileName) {
	return Upp::AppendFile(fileName, ToString());
}

bool FileDataArray::LoadFile(const char *fileName) {
	Clear();
	StringParse in = Upp::LoadFile(fileName);
	
	if (in == "")
		return false;
	
	in.GetLine();
	int numData = (int)in.Count("\n")-1;
	fileList.SetCount(numData);	
	for (int row = 0; row < numData; ++row) {		
		fileList[row].relFilename = in.GetText(sep);	
		fileList[row].fileName = in.GetText(sep);
		fileList[row].fileName += in.GetText(sep);	
		fileList[row].isFolder = in.GetText(sep) == "true" ? true : false;	
		if (fileList[row].isFolder)
			folderCount++;
		else
			fileCount++; 
		fileList[row].length = in.GetUInt64(sep);	
		fileList[row].t.year = in.GetInt(sep);
		fileList[row].t.month = in.GetInt(sep);
		fileList[row].t.day = in.GetInt(sep);
		fileList[row].t.hour = in.GetInt(sep);
		fileList[row].t.minute = in.GetInt(sep);
		fileList[row].t.second = in.GetInt(sep);
		fileList[row].id = in.GetUInt64(sep);	
		in.GetText(sep);		// Unused data
		in.GetText(sep);
	}
	return true;
}

String FileDataArray::GetRelativePath(const String &fullPath) {
	if (basePath != fullPath.Left(basePath.GetCount()))
		return "";
	return fullPath.Mid(basePath.GetCount());
}

uint64 GetDirectoryLength(const char *directoryName) {
	FileDataArray files;
	files.Search(directoryName, "*.*", true);
	return files.GetSize();
}

String ForceExtSafer(const char* fn, const char* ext) {
#ifdef PLATFORM_WIN32
	return ForceExt(fn, ext);
#else
	String ret = ForceExt(fn, ToLower(ext));
	if (FileExists(ret))
		return ret;
	ret = ForceExt(fn, ToUpper(ext));
	if (FileExists(ret))
		return ret;
	return ForceExt(fn, ext);
#endif	
}

uint64 GetLength(const char *fileName) {
	if (FileExists(fileName))
		return (uint64)GetFileLength(fileName);
	else	
		return GetDirectoryLength(fileName);
}

FileDiffArray::FileDiffArray() {
	Clear();
}

void FileDiffArray::Clear() {
	diffList.Clear();
}

// True if equal
bool FileDiffArray::Compare(FileDataArray &master, FileDataArray &secondary, const String folderFrom,
				 Vector<String> &excepFolders, Vector<String> &excepFiles, int sensSecs, 
				 Function<void(int)> progress) {
	int prog = 0, oldprog = -1;
	if (master.size() == 0) {
		if (secondary.size() == 0)
			return true;
		else
			return false;
	} else if (secondary.size() == 0)
		return false;
	
	bool equal = true;
	diffList.Clear();
	Vector<bool> secReviewed;
	secReviewed.SetCount((int)secondary.size(), false);
	
	for (int i = 0; i < (int)master.size(); ++i) {
		prog = (50*i)/((int)master.size());
		if (prog != oldprog) {
			progress(prog);
			oldprog = prog;
		}
		bool cont = true;
		if (master[i].isFolder) {
			String fullfolder = AppendFileNameX(folderFrom, master[i].relFilename, master[i].fileName);
			for (int iex = 0; iex < excepFolders.size(); ++iex)
				if (PatternMatch(excepFolders[iex] + "*", fullfolder)) {// Subfolders included
					cont = false;
					break;
				}
		} else {
			String fullfolder = AppendFileNameX(folderFrom, master[i].relFilename);
			for (int iex = 0; iex < excepFolders.size(); ++iex)
				if (PatternMatch(excepFolders[iex] + "*", fullfolder)) {
					cont = false;
					break;
				}
			for (int iex = 0; iex < excepFiles.size(); ++iex)
				if (PatternMatch(excepFiles[iex], master[i].fileName)) {
					cont = false;
					break;
				}
		}		
		if (cont) {		
			int idSec = secondary.Find(master, i);
			if (idSec >= 0) {
				bool useId = master.UseId() && secondary.UseId();
				secReviewed[idSec] = true;
		
				if (master[i].isFolder) 
					;
				else if (useId && (master[i].id == secondary[idSec].id))
					;
				else if (!useId && (master[i].length == secondary[idSec].length) && 
						 			(abs(master[i].t - secondary[idSec].t) <= sensSecs))
					;
				else {
					equal = false;
					FileDiffData &f = diffList.Add();
					f.isFolder = master[i].isFolder;
					f.relPath = master[i].relFilename;
					f.fileName = master[i].fileName;
					f.idMaster = master[i].id;
					f.idSecondary = secondary[idSec].id;
					f.tMaster = master[i].t;
					f.tSecondary = secondary[idSec].t;
					f.lengthMaster = master[i].length;
					f.lengthSecondary = secondary[idSec].length;
					if (master[i].t > secondary[idSec].t)
						f.action = 'u';
					else
						f.action = 'p';
				}
			} else {					// Not found, is new
				equal = false;
				FileDiffData &f = diffList.Add();
				f.isFolder = master[i].isFolder;
				f.relPath = master[i].relFilename;
				f.fileName = master[i].fileName;
				f.idMaster = master[i].id;
				f.idSecondary = 0;
				f.tMaster = master[i].t;
				f.tSecondary = Null;
				f.lengthMaster = master[i].length;
				f.lengthSecondary = 0;
				f.action = 'n';
			}	
		}
	}
	for (int i = 0; i < secReviewed.size(); ++i) {
		prog = 50 + (50*i)/secReviewed.size();
		if (prog != oldprog) {
			progress(prog);
			oldprog = prog;
		}
		if (!secReviewed[i]) {
			bool cont = true;
			if (secondary[i].isFolder) {
				String fullfolder = AppendFileNameX(folderFrom, secondary[i].relFilename, secondary[i].fileName);
				for (int iex = 0; iex < excepFolders.size(); ++iex)
					if (PatternMatch(excepFolders[iex] + "*", fullfolder)) {
						cont = false;
						break;
					}
			} else {
				String fullfolder = AppendFileNameX(folderFrom, secondary[i].relFilename);
				for (int iex = 0; iex < excepFolders.size(); ++iex)
					if (PatternMatch(excepFolders[iex] + "*", fullfolder)) {
						cont = false;
						break;
					}
				for (int iex = 0; iex < excepFiles.size(); ++iex)
					if (PatternMatch(excepFiles[iex], secondary[i].fileName)) {
						cont = false;
						break;
					}
			}
			if (cont) {
				equal = false;
				FileDiffData &f = diffList.Add();
				f.isFolder = secondary[i].isFolder;
				f.relPath = secondary[i].relFilename;
				f.fileName = secondary[i].fileName;
				f.idMaster = 0;
				f.idSecondary = secondary[i].id;
				f.tMaster = Null;
				f.tSecondary = secondary[i].t;
				f.lengthMaster = 0;
				f.lengthSecondary = secondary[i].length;
				f.action = 'd';
			}
		}
	}
	return equal;
}

bool FileDiffArray::Apply(String toFolder, String fromFolder, EXT_FILE_FLAGS flags)
{
	for (int i = 0; i < diffList.GetCount(); ++i) {
		bool ok = true;
		String dest = AppendFileNameX(toFolder, diffList[i].relPath, diffList[i].fileName);		
		if (diffList[i].action == 'u' || diffList[i].action == 'd') {
			if (diffList[i].isFolder) {
				if (DirectoryExists(dest)) {
					if (!SetReadOnly(dest, false))
						ok = false;
				}
			} else {
				if (FileExists(dest)) {
					if (!SetReadOnly(dest, false))
						ok = false;
				}
			}
		}
		if (!ok) {
			String strError = t_("Not possible to modify ") + dest;	
			SetLastError(strError);
			//return false;
		}

		switch (diffList[i].action) {
		case 'n': case 'u': 	
			if (diffList[i].isFolder) {
				if (!DirectoryExists(dest)) {
					ok = DirectoryCreate(dest);	////////////////////////////////////////////////////////////////////////////////////////
				}
			} else {
				if (FileExists(dest)) {
					if (!SetReadOnly(dest, false))
						ok = false;
				}
				if (ok) {
					ok = FileCopy(AppendFileNameX(fromFolder, FormatInt(i)), dest);
					diffList[i].tSecondary = diffList[i].tMaster;
				}
			}
			
			if (!ok) {
				String strError = t_("Not possible to create ") + dest;
#if defined(PLATFORM_WIN32) || defined (PLATFORM_WIN64)
			  	strError += ". " + WinLastError();
#endif
				SetLastError(strError);
				//return false;
			}
			break;
		case 'd': 
			if (diffList[i].isFolder) {
				if (DirectoryExists(dest))
					ok = DeleteFolderDeep(dest);	// Necessary to add the "X"
				Sleep(100);
			} else {
				if (FileExists(dest))
					ok = FileDeleteX(dest, flags);
			}
			if (!ok) {
				String strError = t_("Not possible to delete") + String(" ") + dest;
#if defined(PLATFORM_WIN32) || defined (PLATFORM_WIN64)
			  	strError += ". " + WinLastError();
#endif
				SetLastError(strError);				
				//return false;
			}
			break;		
		case 'p': 
			SetLastError(t_("There was a problem in the copy"));
			//return false;
			break;
		}
	}
	return true;
}

bool FileDiffArray::SaveFile(const char *fileName) {
	return Upp::SaveFileBOMUtf8(fileName, ToString());
}

String FileDiffArray::ToString() {
	String ret;
	
	ret << "Action" << sep << "IsFolder" << sep << "Folder" << sep << "File" << sep << "IdMaster" << sep << "IdSecondary" << sep 
		<< "Master Year" << sep << "Master Month" << sep << "Master Day" << sep << "Master Hour" << sep << "Master Min" << sep << "Master sec" << sep 
		<< "Secondary Year" << sep << "Secondary Month" << sep << "Secondary Day" << sep << "Secondary Hour" << sep << "Secondary Min" << sep << "Secondary sec" << sep 
		<< "Size master" << sep << "Size secondary";
	for (int i = 0; i < diffList.GetCount(); ++i) {
		ret << "\n";
		ret << diffList[i].action << sep;
		ret << diffList[i].isFolder << sep;
		ret << "\"" << diffList[i].relPath << "\"" << sep;
		ret << "\"" << diffList[i].fileName << "\"" << sep;
		ret << diffList[i].idMaster << sep;
		ret << diffList[i].idSecondary << sep;
		ret << FormatInt(diffList[i].tMaster.year) << sep;
		ret << FormatInt(diffList[i].tMaster.month) << sep;
		ret << FormatInt(diffList[i].tMaster.day) << sep;
		ret << FormatInt(diffList[i].tMaster.hour) << sep;
		ret << FormatInt(diffList[i].tMaster.minute) << sep;
		ret << FormatInt(diffList[i].tMaster.second) << sep;
		ret << FormatInt(diffList[i].tSecondary.year) << sep;
		ret << FormatInt(diffList[i].tSecondary.month) << sep;
		ret << FormatInt(diffList[i].tSecondary.day) << sep;
		ret << FormatInt(diffList[i].tSecondary.hour) << sep;
		ret << FormatInt(diffList[i].tSecondary.minute) << sep;
		ret << FormatInt(diffList[i].tSecondary.second) << sep;
		ret << diffList[i].lengthMaster << sep;
		ret << diffList[i].lengthSecondary << sep;
	}
	return  ret;
}

bool FileDiffArray::LoadFile(const char *fileName)
{
	Clear();
	StringParse in = Upp::LoadFile(fileName);
	
	if (in == "")
		return false;

	in.GetLine();
	int numData = (int)in.Count("\n") - 1;
	diffList.SetCount(numData);	
	for (int row = 0; row < numData; ++row) {		
		diffList[row].action = TrimLeft(in.GetText(sep))[0];	
		diffList[row].isFolder = ToLower(in.GetText(sep)) == "true" ? true : false;	
		diffList[row].relPath = in.GetText(sep);	
		diffList[row].fileName = in.GetText(sep);	
		diffList[row].idMaster = in.GetUInt64(sep);
		diffList[row].idSecondary = in.GetUInt64(sep);
		diffList[row].tMaster.year = in.GetInt(sep);
		diffList[row].tMaster.month = in.GetInt(sep);
		diffList[row].tMaster.day = in.GetInt(sep);
		diffList[row].tMaster.hour = in.GetInt(sep);
		diffList[row].tMaster.minute = in.GetInt(sep);
		diffList[row].tMaster.second = in.GetInt(sep);
		diffList[row].tSecondary.year = in.GetInt(sep);
		diffList[row].tSecondary.month = in.GetInt(sep);
		diffList[row].tSecondary.day = in.GetInt(sep);
		diffList[row].tSecondary.hour = in.GetInt(sep);
		diffList[row].tSecondary.minute = in.GetInt(sep);
		diffList[row].tSecondary.second = in.GetInt(sep);
		diffList[row].lengthMaster = in.GetUInt64(sep);
		diffList[row].lengthSecondary = in.GetUInt64(sep);
	}
	return true;
}
	
}
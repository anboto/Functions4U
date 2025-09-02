// SPDX-License-Identifier: Apache-2.0
// Copyright 2021 - 2022, the Anboto author and contributors
#ifndef _Functions4U_Functions4U_h
#define _Functions4U_Functions4U_h

#include <float.h>
#include <Draw/Draw.h>
#ifdef flagGUI
#include "GatherTpp.h"
#endif

#include "Defs.h"
#include "SvgColors.h"
#include "StaticPlugin.h"
#include "LocalProcess2.h"
#include <random>


namespace Upp {

enum EXT_FILE_FLAGS {NO_FLAG = 0, 
					 USE_TRASH_BIN = 1,
					 BROWSE_LINKS = 2,
					 DELETE_READ_ONLY = 4
};

String GetDesktopManagerNew();

bool LaunchFile(const char *file, const char *params = nullptr, const char *directory = ".");
int64 LaunchCommand(const char* command, const char* directory = NULL);
	
bool FileCat(const char *file, const char *appendFile);

int FileCompare(const char *path1, const char *path2);

int64 FindStringInFile(const char *file, const String text, int64 pos0 = 0);

bool FileStrAppend(const char *file, const char *str);
bool AppendFile(const char *filename, const char *str);

template<typename T>
String AppendFileNameX(const T &t) {
    return t;
}

template<typename T, typename... Args>
String AppendFileNameX(const T &t, Args... args) {
    return AppendFileName(t, AppendFileNameX(args...));
}

#define AFX AppendFileNameX

inline String Trim(const String& s)   {return TrimBoth(s);}

String FitFileName(String fileName, int len);

Vector<String> Tokenize(const String &str, const String &token, int pos = 0);
void Tokenize(const String &str, const String &token, Vector<String> &ret, int pos = 0);
String Tokenize2(const String &str, const String &token, int &pos);
String Tokenize2(const String &str, const String &token, int &pos);
String Tokenize2(const String &str, const String &token);
String GetLine(const String &str, int &pos);
String GetField(const String &str, int &pos, char separator);
	
/////////
bool DirectoryExistsX(const char *path, EXT_FILE_FLAGS flags = NO_FLAG); 
void DirectoryCopyX(const char *dir, const char *newPlace, bool replaceOnlyNew, String filesToExclude, String &errorList);
bool DirectoryMove(const char *dir, const char *newPlace);
bool DeleteDeepWildcardsX(const char *path, bool filefolder, EXT_FILE_FLAGS flags = NO_FLAG, bool deep = true);
bool DeleteDeepWildcardsX(const char *pathwc, const char *namewc, bool filefolder, EXT_FILE_FLAGS flags = NO_FLAG, bool deep = true);
bool DeleteFolderDeepWildcardsX(const char *path, EXT_FILE_FLAGS flags = NO_FLAG);
bool DeleteFolderDeepWildcardsX(const char *path, const char *name, EXT_FILE_FLAGS flags = NO_FLAG);
bool DeleteFileDeepWildcardsX(const char *path, EXT_FILE_FLAGS flags = NO_FLAG);
bool DeleteFileWildcardsX(const char *path, EXT_FILE_FLAGS flags = NO_FLAG);
bool DeleteFolderDeepX(const char *path, EXT_FILE_FLAGS flags = NO_FLAG);
bool RenameDeepWildcardsX(const char *path, const char *namewc, const char *newname, bool forfile, bool forfolder, EXT_FILE_FLAGS flags = NO_FLAG);
bool FolderIsEmpty(const char *path);

bool DirectoryCreateX(const char *path);
	
bool FileMoveX(const char *oldpath, const char *newpath, EXT_FILE_FLAGS flags = NO_FLAG);
bool FileDeleteX(const char *path, EXT_FILE_FLAGS flags = NO_FLAG);

bool IsRootFolder(const char *folderName);
String GetUpperFolder(const String &folderName);
String GetNextFolder(const String &folder, const String &lastFolder);
String FileRealName(const char *fileName);
bool IsFile(const char *fileName);
bool IsFolder(const char *fileName);
String GetRelativePath(String from, String path, bool normalize = true);
String GetAbsolutePath(String from, String relative);
	
bool IsSymLink(const char *path);

bool SetReadOnly(const char *path, bool readOnly);
bool SetReadOnly(const char *path, bool usr, bool grp, bool oth);
bool IsReadOnly(const char *path, bool &usr, bool &grp, bool &oth);

String LoadFile_Safe(const String fileName);
String LoadFile(const char *fileName, off_t from, size_t len = 0);

uint64 GetLength(const char *fileDirName); 
uint64 GetDirectoryLength(const char *directoryName);

String GetSourceFolder();
bool IsTheIDE();

String ForceExtSafer(const char* fn, const char* ext);
	
///////////////////////////////
Vector<String> SearchFile(String dir, const Vector<String> &condFiles, const Vector<String> &condFolders, 
								 const Vector<String> &extFiles,  const Vector<String> &extFolders, 
								 const String text, Vector<String> &errorList);
Vector<String> SearchFile(String dir, String condFile, String text, Vector<String> &errorList);//, int flags = 0);
Vector<String> SearchFile(String dir, String condFile = "*.*", String text = "");//, int flags = 0);
///////////////////////////////

bool FileToTrashBin(const char *path);
int64 TrashBinGetCount();
bool TrashBinClear();

String GetPersonalFolder();
String GetRootFolder();
String GetTempFolder();
String GetOsFolder();
String GetSystemFolder();
#ifdef PLATFORM_WIN32
String GetCommonAppDataFolder();
#endif
//bool SetEnv(const char *id, const char *val);

struct FileData : Moveable<FileData> {
	bool isFolder;
	String fileName;
	String relFilename;
	uint64 length;
	struct Upp::Time t;
	uint64 id;
	
	String ToString() const { return Format("%s %0n", fileName, (int64)length); }

	FileData(bool _isFolder, String _fileName, String _relFilename, uint64 _length, 
		struct Upp::Time _t, uint64 _id) : isFolder(_isFolder), fileName(_fileName), 
		relFilename(_relFilename), length(_length), t(_t), id(_id) {}
	FileData() {}
};

class ErrorHandling {
public:
	void SetLastError(String _lastError)	{lastError = _lastError;};
	String GetLastError()					{return lastError;};
	
private:
	String lastError;
};

class FileDiffArray;

class FileDataArray : public ErrorHandling {
public:
	FileDataArray(bool useId = false, int fileFlags = 0);
	bool Init(FileDataArray &orig, FileDiffArray &diff);
	void Clear();
	bool Search(const String &dir, const String &condFile, bool recurse = false, const String &findText = "", Function<bool(uint64, uint64)> Print = Null, bool files = true, bool folders = true);
	FileData& operator[](long i)	{return fileList[i];}
	uint64 GetFileCount()			{return fileCount;};
	uint64 GetFolderCount()			{return folderCount;};
	int size() 						{return fileList.size();};
	uint64 GetSize()				{return fileSize;};
	inline bool UseId() 			{return useId;};
	void SortByName(bool ascending = true);
	void SortByDate(bool ascending = true);
	void SortBySize(bool ascending = true);
	Vector<String> &GetLastError()	{return errorList;};
	int Find(String &relFileName, String &fileName, bool isFolder);
	int Find(FileDataArray &data, int id);
	String FullFileName(int i)		{return AppendFileNameX(basePath, fileList[i].fileName);};
	bool SaveFile(const char *fileName);
	bool AppendFile(const char *fileName);
	bool LoadFile(const char *fileName);
	bool SaveErrorFile(const char *fileName);
	void SetSeparator(char c) {sep = String(c, 1);}

private:
	void Search_Each(Vector<String> &list, const String &condFile, bool recurse, const String &findText, bool files, bool folders);
	uint64 GetFileId(String fileName);
	String GetRelativePath(const String &fullPath);
	String ToString();
	String GetErrorText();
	
	Array<FileData> fileList;
	Vector<String> errorList;
	String basePath;
	uint64 fileCount, folderCount;
	uint64 fileSize;
	bool useId;
	int fileFlags;
	String sep = ";";  
};

struct FileDiffData {
	char action;	// 'n': New, 'u': Update, 'd': Delete, 'p': Problem
	bool isFolder;
	String relPath;
	String fileName;
	uint64 idMaster, idSecondary;
	struct Upp::Time tMaster, tSecondary;
	uint64 lengthMaster, lengthSecondary;
};

class FileDiffArray : public ErrorHandling {
public:
	FileDiffArray();
	void Clear();
	FileDiffData& operator[](long i)	{return diffList[i];}
	bool Compare(FileDataArray &master, FileDataArray &secondary, const String folderFrom, 
		Vector<String> &excepFolders, Vector<String> &excepFiles, int sensSecs = 0, 
		Function<void(int)> progress = Null);
	bool Compare(FileDataArray &master, FileDataArray &secondary, const String folderFrom, int sensSecs = 0, 
		Function<void(int)> progress = Null) {
		Vector<String> excepFolders, excepFiles;
		return Compare(master, secondary, folderFrom, excepFolders, excepFiles, sensSecs, progress);
	}
	bool Apply(String toFolder, String fromFolder, EXT_FILE_FLAGS flags = NO_FLAG);
	long GetCount()				{return diffList.size();};
	long size()					{return diffList.size();};
	bool SaveFile(const char *fileName);
	bool LoadFile(const char *fileName);
	String ToString();
	void SetSeparator(char c) {sep = String(c, 1);}
	
private:
	Array<FileDiffData> diffList;
	String sep = ";";
};

String Replace(String str, String find, String replace); 
String Replace(String str, char find, char replace);

int ReverseFind(const String& s, const String& toFind, int from = 0);

String FormatLong(long a); 

Upp::Time StringToTime(const char *s);
Date StringToDate(const char *s);
Upp::Time StrToTime(const char *s);
Date StrToDate(const char *s);

double StringToSeconds(const char *s);		
void StringToHMS(const char *s, int &hour, int &min, double &seconds); 

String BytesToString(uint64 bytes, bool units = true);

String SecondsToString(double seconds, int dec = 2, bool units = false, bool space = false,
						bool tp = false, bool longUnits = false, bool forceSec = false);
String HMSToString(int hour, int min, double seconds, int dec = 2, bool units = false, bool space = false,
						bool tp = false, bool longUnits = false, bool forceSec = false);

String SeasonName(int iseason);
int GetSeason(Date &date);

int NumAdvicedDigits(double range);
String FormatDoubleAutosize(double d);	
String FormatDoubleAutosize(double d, double range);
String FormatDoubleSize(double d, int fieldWidth, bool fillSpaces = false, const String &strNull = "nan");
String FormatDoubleDecimals(double d, int maxDecimals);
#define	FDAS	FormatDoubleAutosize
#define	FDS		FormatDoubleSize

int GetExponent10(double d);
double NumberWithLeastSignificantDigits(double minVal, double maxVal);
double GetRangeMajorUnits(double minV, double maxV);
	
String RemoveAccents(String str);
String RemoveAccent(wchar c);
WString RemoveAccentW(wchar c);
String RemovePunctuation(String str);
bool IsPunctuation(wchar c);

String GreekToText(wchar c);
bool IsGreek(wchar c);
String RemoveGreek(String str);

String CharToSubSupScript(char c, bool subscript);
String NumToSubSupScript(int d, bool subscript);

const char *Ordinal(int num);

inline float ToRad(float angle)	{
	return angle*static_cast<float>(M_PI/180.);
}
	
template<typename T>	
inline double ToRad(T angle)	{
	return angle*M_PI/180.;
}

template <class Range>
Range ToRadArray(const Range& r) {
	Range ret(r.size());
	for (int i = 0; i < r.size(); i++)
		ret[i] = ToRad(r[i]); 
	return ret;
}

template<typename T>
inline float ToDeg(float angle)	{
	return angle*static_cast<float>(180./M_PI);
}

template<typename T>
inline T ToDeg(T angle)	{
	return angle*180./M_PI;
}

template <class Range>
Range ToDegArray(const Range& r) {
	Range ret(r.size());
	for (int i = 0; i < r.size(); i++)
		ret[i] = ToDeg(r[i]); 
	return ret;
}

template<typename T>
inline float atan2_360(float y, float x) {
	float ret = ToDeg(atan2(y, x));
	return ret > 90 ? 450 - ret : 90 - ret; 
}

template<typename T>
inline double atan2_360(T y, T x) {
	double ret = ToDeg(atan2(y, x));
	return ret > 90 ? 450 - ret : 90 - ret; 
}

inline bool Odd(int val)	  		{return val%2;}
inline bool Even(int val) 	  		{return !Odd(val);}
inline int RoundEven(int val) 		{return Even(val) ? val : val+1;}
template<class T>
inline int Sign(T a) 				{return (a >= 0) - (a < 0);}
template<class T>
inline T Neg(T a) 					{return a > 0 ? -a : a;}
template<class T>
inline T Average(T a, T b) 			{return T(a+b)/T(2);}
template<class T>
inline T Avg(T a, T b) 				{return Average(a, b);}
template<class T>
inline T Average(T a, T b, T c)		{return T(a+b+c)/T(3);}
template<class T>
inline T Avg(T a, T b, T c) 		{return Average(a, b, c);}
template<class T>
inline T Average(T a, T b, T c, T d){return T(a+b+c+d)/T(4);}
template<class T>
inline T Avg(T a, T b, T c, T d)	{return Average(a, b, c, d);}
template<class T>
inline T pow2(T a) {return a*a;}
template<class T>
inline T pow3(T a) {return a*a*a;}
template<class T>
inline T pow4(T a) {return pow2(pow2(a));}
template<typename T>
inline T LogBase(T a, T base) {
	ASSERT(base >= 2);
	if (base == 10)
		return log10(a);
	else
		return log(a)/log(base);
}
template<typename T>
T PowInt(T x, int n) {
	ASSERT_(std::is_floating_point<T>::value, "Type has to be floating point");
	if (n == 0) 
		return 1;
	if (n < 0) {
		x = 1/x;
		n = -n;
	}
	T y = 1;
	while (n > 1) {
		if (n&1) {
			y *= x;
			n--;
		}
		x *= x;
		n /= 2;
	}
	return x*y;
}
template<typename T>
T Pow10Int(int n) {
	ASSERT_(std::is_floating_point<T>::value, "Type has to be floating point");
	static T pow10[10] = {1, 10, 1e2, 1e3, 1e4, 1e5, 1e6, 1e7, 1e8, 1e9};
	if (n >= 0 && n < 10) 
		return pow10[n];
	return PowInt((T)10, n);
}
template<typename T>
inline T fround(T x, int numdec) {
	T pow10 = Pow10Int<T>(numdec);
  	return round(x*pow10)/pow10;
}

template<class T>
inline T Mirror(T x, T val) 		{return 2*val - x;}		// -(x - val) + val

template<class T>
T RoundClosest(T val, T grid, T eps) {
	ASSERT(eps > 1E-15);
	T rnd = round(val/grid);
	if (abs(rnd - val/grid) > eps)
		return val;
	else {
		if (rnd == 0)
			return 0;
		return rnd*grid;
	}
}

template<class T>
T RoundClosest(T val, T step) {
    long long factor = static_cast<long long>(round(1./step));
    T ret =  round(val * factor) / factor;
    if (ret == -0.)
        ret = 0;
    return ret;
}

template<class T>
T AvgSafe(const T &a, const T &b) {
	if (IsNum(a)) {
		if (IsNum(b)) 
			return Avg(a, b);
		else
			return a;
	} else 
		return b;
}

template <class Range1, class Range2>
Range1 ApplyIndex(const Range1 &x, const Range2 &indices) {
	ASSERT(x.size() == indices.size());
	Range1 ret(x.size());
	for (int i = 0; i < x.size(); ++i)
		ret[i] = x[indices[i]];
	return ret;
}

template<class T>
inline T Nvl2(T a, T b) {return IsNum(a) ? a : b;}

template<class T1, class T2>
inline T2 Nvl2(T1 cond, T2 a, T2 b) {return IsNum(cond) ? a : b;}


template<typename T>
T fact(T val) {
	if (val <= 0)
		throw std::invalid_argument("fact() accepts only nonzero positive numbers");
	T ret = 1;
	while (val > 1)
		ret *= val--;
	return ret; 
}

template <class T> 
inline bool Between(const T& val, const T& min, const T& max) { 
	ASSERT(max >= min);
	return val >= min && val <= max;
}
template <class T> 
inline bool Between(const T& val, const T& range) { 
	ASSERT(range > 0);
	return val >= -range && val <= range;
}
template <class T> 
inline T BetweenVal(const T& val, const T& _min, const T& _max) { 
	ASSERT(_max >= _min);
	return max(_min, min(_max, val));
}
template <class T> 
inline T FixFloat(T val) {
	if(std::isnan(val) || std::isinf(val) || val == HUGE_VAL || val == -HUGE_VAL)
		return Null;
	return val;
}

template <class T> 
T AngleAdd360(T ang, T val) {
	ang += val;
	while (ang >= 360)
		ang -= 360;
	while (ang < 0)
		ang += 360;
	return ang;
}

template <class T> 
inline const T Norm(const T& dx, const T& dy)  { 
	return static_cast<T>(sqrt(dx*dx + dy*dy)); }

template <class T> 
inline const T Norm(const T& dx, const T& dy, const T& dz)  { 
	return static_cast<T>(sqrt(dx*dx + dy*dy + dz*dz)); }
		
template <class T> 
inline const T Distance(const T& x1, const T& y1, const T& x2, const T& y2)  { 
	return Norm(x1-x2, y1-y2); }

template <class T> 
inline const T Distance(const Point_<T>& p1, const Point_<T>& p2)  { 
	return Distance<T>(p1.x, p1.y, p2.x, p2.y); }

template <class T> 
inline const T Distance(const T& x1, const T& y1, const T& z1, const T& x2, const T& y2, const T& z2)  { 
	return static_cast<T>(sqrt(pow2(x1-x2) + pow2(y1-y2) + pow2(z1-z2))); }
	
template <class T> 
inline const T Angle(const T& x1, const T& y1, const T& x2, const T& y2)  { 
	return static_cast<T>(atan2(y2-y1, x2-x1));
}

template <class T> 
inline const T Angle(const Point_<T>& p1, const Point_<T>& p2)  { 
	return Angle<T>(p1.x, p1.y, p2.x, p2.y);
}

template <class T> 
inline const Point_<T> Middle(const Point_<T>& p1, const Point_<T>& p2)  { 
	return Point_<T>(Avg(p1.x, p2.x), Avg(p1.y, p2.y));
}

Vector<Vector <Value> > ReadCSV(const String &strFile, char separator = ',', bool bycols = true, bool removeRepeated = true, char decimalSign = '.', bool onlyStrings = false, int fromRow = 0);
Vector<Vector <Value> > ReadCSVFile(const String &fileName, char separator = ',', bool bycols = true, bool removeRepeated = true, char decimalSign = '.', bool onlyStrings = false, int fromRow = 0);
bool ReadCSVFileByLine(const String &fileName, Gate<int, Vector<Value>&, String &> WhenRow, char separator = ',', char decimalSign = '.', bool onlyStrings = false, int fromRow = 0);
String WriteCSV(Vector<Vector <Value> > &data, char separator = ',', bool bycols = true, char decimalSign = '.');
bool WriteCSVFile(const String &fileName, Vector<Vector <Value> > &data, char separator = ',', bool bycols = true, char decimalSign = '.');

bool GuessCSV(const String &fileName, bool onlyNumbers, String &header, Vector<String> &parameters, char &separator, bool &repetition, char &decimalSign, int64 &beginData, int &beginDataRow);
bool GuessCSVStream(Stream &in, bool onlyNumbers, String &header, Vector<String> &parameters, char &separator, bool &repetition, char &decimalSign, int64 &beginData, int &beginDataRow);
struct CSVParameters {
	String header;
	Vector<String> parameters;
	char separator, decimalSign;
	bool repetition;
	int64 beginData;
	int beginDataRow;
};
bool GuessCSV(const String &fileName, bool onlyNumbers, CSVParameters &param);
bool GuessCSVStream(Stream &in, bool onlyNumbers, CSVParameters &param);

bool IsRealNumber(const char *str, char dec_sep);
String CleanThousands(const char *str, char dec_sep);
	
// A String based class to parse into
class StringParse : public String {
public:
	explicit StringParse() : String("") {GoInit();};
	StringParse(String _s): String(_s)  {GoInit();};
	
	void GoInit()	{pos = 0; lastSeparator='\0';};
	bool GoBefore(const String text) {
		if (pos >= GetLength()) {
			pos = GetLength()-1;
			return false;
		}
		int newpos = String::Find(text, pos);
		if (newpos < 0)
			return false;	// If it does not find it, it does not move
		pos = newpos;
		return true;
	};	
	bool GoAfter(const String text) {
		if(!GoBefore(text))
			return false;
		pos += int(strlen(text));
		return true;
	};
	bool GoAfter(const String text, const String text2) {
		if(!GoAfter(text))
			return false;
		if(!GoAfter(text2))
			return false;
		return true;
	};
	bool GoAfter(const String text, const String text2, const String text3) {
		if(!GoAfter(text))
			return false;
		if(!GoAfter(text2))
			return false;
		if(!GoAfter(text3))
			return false;
		return true;
	};
	bool GoAfter_Init(const String text) {GoInit();	return GoAfter(text);};
	bool GoAfter_Init(const String text, const String text2) {GoInit();	return GoAfter(text, text2);};
	bool GoAfter_Init(const String text, const String text2, const String text3) {GoInit();	return GoAfter(text, text2, text3);};		
	
	void GoBeginLine() {
		for (; pos >= 0; --pos) {
			if ((ToString()[pos-1] == '\r') || (ToString()[pos-1] == '\n'))
				return;
		} 
	}
	bool IsBeginLine() {
		if (pos == 0)
			return true;
		if ((ToString()[pos-1] == '\r') || (ToString()[pos-1] == '\n'))
			return true;
		return false;
	}
	bool IsSpaceRN(int c) {
		if (IsSpace(c))
			return true;
		if ((c == '\r') || (c == '\n'))
		     return true;
		return false;
	}
	// Gets text between "" or just a word until an space
	// It considers special characters with \ if between ""
	// If not between "" it gets the word when it finds one of the separator characters
	String GetText(String separators = "") {
		String ret = "";
		if (pos > GetCount() || pos == -1)
			return ret;
		int newpos = pos;
		
		while ((IsSpaceRN(ToString()[newpos]) && (ToString()[newpos] != '\"') && 
			   (ToString()[newpos] != '\0')))
			newpos++;
		if (ToString()[newpos] == '\0') {
			pos = newpos;
			return "";
		}
	
		if (ToString()[newpos] == '\"') {	// Between ""
			newpos++;
			while (ToString()[newpos] != '\"' && ToString()[newpos] != '\0') {
				if (ToString()[newpos] == '\\') {
					newpos++;
					if (ToString()[newpos] == '\0')
						return "";
				} 
				ret.Cat(ToString()[newpos]);
				newpos++;
			}
			lastSeparator = '"';
		} else if (separators == "") {		// Simple word
			while (!IsSpaceRN(ToString()[newpos]) && ToString()[newpos] != '\0') {
				if (ToString()[newpos] == '\"') {
					newpos--;	// This " belongs to the next
					break;
				}
				ret.Cat(ToString()[newpos]);
				newpos++;
			}
			lastSeparator = ToString()[newpos];
		} else {							// Simple word, special separator
			while (ToString()[newpos] != '\0') {// Only consider included spaces (!IsSpaceRN(ToString()[newpos]) && ToString()[newpos] != '\0') {
				if (ToString()[newpos] == '\"') {
					newpos--;	// This " belongs to the next
					break;
				}				
				if (separators.Find(ToString()[newpos]) >= 0) {
					lastSeparator = ToString()[newpos];
					break;
				}
				ret.Cat(ToString()[newpos]);
				newpos++;
			} 
			lastSeparator = ToString()[newpos];
		}
		pos = ++newpos;		// After the separator: ", space or separator
		return ret;
	}
	String GetLine() {
		String ret;
		if (pos > GetCount() || pos == -1)
			return String();
		while (ToString()[pos] != '\0') {
			if (ToString()[pos] == '\n') {
				pos++;
				return ret;
			}
			if (ToString()[pos] == '\r' && ToString()[pos+1] == '\n') {
				pos += 2;
				return ret;
			}
			ret.Cat(ToString()[pos]);
			pos++;
		}
		return ret;
	}
	double GetDouble(String separators = "")  	{return FixFloat(atof(GetText(separators)));};
	int GetInt(String separators = "")			{return static_cast<int>(FixFloat(atof(GetText(separators))));};
	long GetLong(String separators = "")		{return static_cast<long>(FixFloat(atof(GetText(separators))));};
	uint64 GetUInt64(String separators = "")	{return static_cast<uint64>(FixFloat(atof(GetText(separators))));};
	
	String Right() 			{return String::Mid(pos+1);}
	int GetLastSeparator() 	{return lastSeparator;}
	void MoveRel(int val) {
		pos += val;
		if (pos < 0)
			pos = 0;
		else if (pos >= GetCount())
			pos = GetCount() - 1;
	}
	int GetPos() {return pos;};
	bool SetPos(int i) 
	{
		if (i < 0 || i >= GetCount())
			return false;
		else {
			pos = i;
			return true;
		}
	}
	bool Eof()
	{
		return pos >= GetCount();
	}
	unsigned Count(String _s)
	{
		int from = 0;
		unsigned count = 0;
		
		while ((from = ToString().Find(_s, from)) >= 0) {
			count++;
			from++;
		}
		return count;
	}
private:
	int pos;
	int lastSeparator;
};

#if defined(PLATFORM_WIN32) 
Value GetVARIANT(VARIANT &result);
String WideToString(LPCWSTR wcs, int len = -1);
bool StringToWide(String str, LPCWSTR &wcs);
bool BSTRSet(const String str, BSTR &bstr);
String BSTRGet(BSTR &bstr);
#endif
 

String GetExtExecutable(const String ext);

struct SoftwareDetails : DeepCopyOption<SoftwareDetails> {
	String name;
	String publisher;
	String version;
	String path;
	String description;
	String architecture;
	
	Vector<int> GetVersion() {
		Vector<String> ver = Split(version, '.');
		Vector<int> ret(ver.size());
		for (int i = 0; i < ver.size(); ++i)
			ret[i] = ScanInt(ver[i]);
		return ret;
	}
	static int IsHigherVersion(Vector<int> &a, Vector<int> &b) {
		return CompareRanges(a, b);
	}
};

Array<SoftwareDetails> GetInstalledSoftware();
Array<SoftwareDetails> GetSoftwareDetails(String software);
#ifdef PLATFORM_POSIX
void GetSoftwarePath(SoftwareDetails &r);
#endif
	
Vector<String> GetDriveList();

#define DLLFunction(dll, ret, function, args)    auto function = (ret(*)args)dll.GetFunction_throw(#function)
#define DLLGetFunction(dll, ret, function, args) 				 (ret(*)args)dll.GetFunction_throw(#function)

class Dl {
public:
	Dl() {}
	Dl(const String &fileDll) 	{Load(fileDll);}
	virtual ~Dl();
	bool IsLoaded()				{return hinstLib != 0;}
	bool Load(const String &fileDll);
	void Load_throw(const String &fileDll);
	void *GetFunction(const String &functionName) const;
	void *GetFunction_throw(const String &functionName) const;
#if defined(PLATFORM_WIN32)
	HINSTANCE GetHandle() {return hinstLib;}
#else
	void *GetHandle()	  {return hinstLib;}
#endif
	operator bool() const {return hinstLib;}
	
private:
#if defined(PLATFORM_WIN32) 
	HINSTANCE hinstLib = 0;	
#else
	void *hinstLib = 0;
#endif
};

typedef Dl Dll;



String BsGetLastError();
bool BSPatch(String oldfile, String newfile, String patchfile);
bool BSDiff(String oldfile, String newfile, String patchfile);


template <class T>
Rect_<T> FitInFrame(const Size_<T> &frame, const Size_<T> &object)
{
	double frameAspect  = frame.cx/static_cast<double>(frame.cy); 
	double objectAspect = object.cx/static_cast<double>(object.cy);	
	
	if (frameAspect > objectAspect) {
		double x = (frame.cx - objectAspect*frame.cy)/2.;
		return Rect_<T>(static_cast<T>(x), 0, static_cast<T>(x + objectAspect*frame.cy), frame.cy);
	} else {
		double y = (frame.cy - frame.cx/objectAspect)/2.;
		return Rect_<T>(0, static_cast<T>(y), frame.cx, static_cast<T>(y + frame.cx/objectAspect));
	}
}

inline const RGBA *GetPixel(const Image &img, int x, int y) {
	return &img[y][x];
}

inline RGBA *GetPixel(ImageBuffer &img, int x, int y) {
	return &img[y][x];
}

inline bool IsValid(const Image &img, int x, int y) {
	return x >= 0 && y >= 0 && x < img.GetWidth() && y < img.GetHeight();
}

inline bool IsValid(ImageBuffer &img, int x, int y) {
	return x >= 0 && y >= 0 && x < img.GetWidth() && y < img.GetHeight();
}

template <class T> 
inline bool IsValid(const Image &img, T &t) {
	return t.x >= 0 && t.y >= 0 && t.x < img.GetWidth() && t.y < img.GetHeight();
}

template <class T> 
inline bool IsValid(ImageBuffer &img, T &t) {
	return t.x >= 0 && t.y >= 0 && t.x < img.GetWidth() && t.y < img.GetHeight();
}

Color RandomColor();

Image GetRect(const Image& orig, const Rect &r);

double tmGetTimeX();

int SysX(const char *cmd, String& out, String& err, double timeOut = Null, 
			Gate3<double, String&, String&> progress = false, bool convertcharset = true);
			
	
class _NRFuse {
public:
	explicit _NRFuse(bool *_inside) {inside = _inside; failed = true;}
	virtual ~_NRFuse() 			   	{if (!failed) *inside = false;}
	bool failed;
private:
	bool *inside;
};

#define NON_REENTRANT_V	 	static bool _insideNR; _NRFuse _fuseNR(&_insideNR); \
							if(!_insideNR) {									\
								_insideNR = true; 								\
								_fuseNR.failed = false;							\
							} else 												\
								return
#define NON_REENTRANT(v) 	static bool _insideNR; _NRFuse _fuseNR(&_insideNR); \
							if(!_insideNR) {									\
								_insideNR = true; 								\
								_fuseNR.failed = false;							\
							} else 												\
								return v

template <class T>
struct TempAssign {
	TempAssign(T &_variable, T newvalue) : oldvalue(_variable), variable(&_variable) {
		*variable = newvalue;
	}
	virtual ~TempAssign() {
		*variable = oldvalue;
	}
	
	T oldvalue, *variable;
};

// Replaced with std::atomic only for trivial classes, not for String for example
template <class T>
class ThreadSafe : public T {
public:
     ThreadSafe& operator=(const ThreadSafe& s) {
        if (this == &s) 
            return *this;
        Mutex::Lock lock(mutex);
        Mutex::Lock lock_s(s.mutex);
        T::operator=(s);
        return *this;
    }
    ThreadSafe& operator=(const T& s) {
        Mutex::Lock lock(mutex);
        T::operator=(s);
        return *this;
    }

private:
    mutable Mutex mutex;
};

template <class Range>
static void ShuffleAscending(Range &data, std::default_random_engine &generator) {
	for (int i = 0; i < data.size() - 2; i++) {
	  	std::uniform_int_distribution<int> distribution(i, data.size() - 1);
        Swap(data[i], data[distribution(generator)]);
    }
}

template <class Range>
static void ShuffleDescending(Range &data, std::default_random_engine &generator) {
	for (int i = data.size() - 1; i > 0; i--) {
	  	std::uniform_int_distribution<int> distribution(0, i);
        Swap(data[i], data[distribution(generator)]);
    }
}

template <class Range>
void Shuffle(Range &data, int randomSeed = Null) {
	if (IsNull(randomSeed))	{
		std::random_device rd;
		randomSeed = (int)rd();
	}
	std::default_random_engine re((std::default_random_engine::result_type)randomSeed);
	
	std::mt19937 generator((std::default_random_engine::result_type)randomSeed);
  
	ShuffleAscending(data, re);
	ShuffleDescending(data, re);	
}

template <class Range>
bool IsSorted(const Range &data) {
	int64 num = data.size();
	if (num == 0)
		return false;
	if (num == 1)
		return true;
	for (int i = 1; i < num; ++i) {
		if (data[i] < data[i-1])
			return false;
	}
	return true;
}

template <class Range, class Less>	// Valid for containers without GetCount(). To use with SetSortOrder
Vector<int> GetSortOrderX(const Range& r, const Less& less) {
	auto begin = r.begin();
	Vector<int> index;
	index.SetCount(int(r.size()));
	for(int i = index.size(); --i >= 0; index[i] = i)
		;
	typedef SortOrderIterator__<decltype(begin), ValueTypeOf<Range>> It;
	Sort__(It(index.begin(), begin), It(index.end(), begin), less);
	return index;
}

template <class Range>
Vector<int> GetSortOrderX(const Range& r) {
	return GetSortOrderX(r, std::less<ValueTypeOf<Range>>());
}

template <class Range, class Less>	// Valid for containers without GetCount()
Vector<int> GetCoSortOrderX(const Range& r, const Less& less) {
	auto begin = r.begin();
	Vector<int> index;
	index.SetCount(int(r.size()));
	for(int i = index.size(); --i >= 0; index[i] = i)
		;
	typedef SortOrderIterator__<decltype(begin), ValueTypeOf<Range>> It;
	CoSort__(It(index.begin(), begin), It(index.end(), begin), less);
	return index;
}

template <class T>
bool EqualRatio(const std::complex<T>& a, const std::complex<T>& b, const T& ratio, const T& zero = 0) {
	T absa = abs(a);
	T absb = abs(b);	
	if (absa <= zero) {
		if (absb <= zero)
			return true;
		else {
			if(abs((zero - absb)/absb) <= ratio) 
				return true;
			else
				return false;
		}
	} else if (absb <= zero) {
		if(abs((absa - zero)/absa) <= ratio) 
			return true;
		else
			return false;
	}
	if(abs((a - b)/b) <= ratio) 
		return true;
	return false;
}

template <class T>
bool EqualRatio(const T& a, const T& b, const T& ratio, const T& zero = 0) {
	if (abs(a) <= zero) {
		if (abs(b) <= zero)
			return true;
		else {
			if(abs((zero - b)/b) <= ratio) 
				return true;
			else
				return false;
		}
	} else if (abs(b) <= zero) {
		if(abs((a - zero)/a) <= ratio) 
			return true;
		else
			return false;
	}
	if(abs((a - b)/b) <= ratio) 
		return true;
	return false;
}

template <typename T1, typename T2>
bool EqualDecimals(const T1& a, const T2& b, int numDecimals) {
	return std::abs(a - b)*Pow10Int<double>(numDecimals) < 1;
}

template <class Range>
int Find(const Range& r, const typename Range::value_type& value, int from = 0) {
	for (int i = from; i < r.size(); i++)
		if(r[i] == value) 
			return i;
	return -1;
}

template <class Range>
int FindFunction(const Range& r, Function<bool(const typename Range::value_type &)> IsEqual, int from = 0) {
	for (int i = from; i < r.size(); i++)
		if(IsEqual(r[i])) 
			return i;
	return -1;
}

template <class Range>
int FindAdd(Range& r, const typename Range::value_type& value, int from = 0) {
	int id = Find(r, value, from);
	if (id >= 0)
		return id; 
	r.Add(value);
	return int(r.size()-1);
}

template<class Range, typename T>
int FindClosest(const Range& r, const std::complex<T>& value, int from = 0) {
	int minId = -1;
	T minDiff = std::numeric_limits<T>::max();
	for (int i = from; i < r.size(); i++) {
		T diff = abs(value - r[i]);
		if (diff < minDiff) {
			minDiff = diff;	
			minId = i;		
		}
	}
	return minId;
}

template<class Range>
int FindClosest(const Range& r, const typename Range::value_type& value, int from = 0) {
	int minId = -1;
	typename Range::value_type minDiff = std::numeric_limits<typename Range::value_type>::max();
	for (int i = from; i < r.size(); i++) {
		typename Range::value_type diff = abs(value - r[i]);
		if (diff < minDiff) {
			minDiff = diff;	
			minId = i;		
		}
	}
	return minId;
}

template <class Range>
int FindRatio(const Range& r, const typename Range::value_type& value, const typename Range::value_type& ratio, int from = 0) {
	int id = FindClosest(r, value, from);
	if (id >= 0) {
		if (EqualRatio(r[id], value, ratio))
			return id;
	}
	return -1;
}

template <class Range, typename T>
int FindRatio(const Range& r, const std::complex<T>& value, const T& ratio, int from = 0) {
	int id = FindClosest(r, value, from);
	if (id >= 0) {
		if (EqualRatio(r[id], value, ratio))
			return id;
	}
	return -1;
}

template <class Range>
int FindAddRatio(Range& r, const typename Range::value_type& value, const typename Range::value_type& ratio, int from = 0) {
	int id = FindRatio(r, value, ratio, from);
	if (id >= 0)
		return id; 
	r.Add(value);
	return int(r.size()-1);
}

template<class Range, typename T>
int FindDelta(const Range& r, const std::complex<T>& value, const T& delta, int from = 0) {
	int id = FindClosest(r, value, from);
	if (id >= 0) {
		if (abs(r[id] - value) <= delta) 
			return id;
	}
	return -1;
}

template <class Range>
int FindDelta(const Range& r, const typename Range::value_type& value, const typename Range::value_type& delta, int from = 0) {
	int id = FindClosest(r, value, from);
	if (id >= 0) {
		if (abs(r[id] - value) <= delta) 
			return id;
	}
	return -1;
}

template <class Range, typename T>
int FindAddDelta(Range& r, const std::complex<T>& value, const T& delta, int from = 0) {
	int id = FindDelta(r, value, delta, from);
	if (id >= 0)
		return id; 
	r.Add(value);
	return int(r.size()-1);
}

template <class Range>
int FindAddDelta(Range& r, const typename Range::value_type& value, const typename Range::value_type& delta, int from = 0) {
	int id = FindDelta(r, value, delta, from);
	if (id >= 0)
		return id; 
	r.Add(value);
	return int(r.size()-1);
}

template <class Range>
int FindRoundDecimals(const Range& r, const typename Range::value_type& value, int numDecimals, int from = 0) {
	int id = FindClosest(r, value, from);
	if (id >= 0) {
		if (EqualDecimals(value, r[id], numDecimals))
			return id;
	}
	return -1;
}
    
template <class Range1, class Range2>
bool Compare(const Range1& a, const Range2& b) {
	if (a.size() != b.size())
		return false;
	for (int i = 0; i < a.size(); i++) {
		if(a[i] != b[i]) 
			return false;
	}
	return true;
}


template <typename T>
const T& At(const Vector<T> &v, int i) {return v[i];}

template <typename T>
T& At(Vector<T> &v, int i) {return v[i];}

template <class Range>
const typename Range::value_type& At(const Range &v, int i) {return v(i);}

template <class Range>
typename Range::value_type& At(Range &v, int i) {return v(i);}


template <class Range1, class Range2>
bool CompareRatio(const Range1& a, const Range2& b, const typename Range1::value_type& ratio, const typename Range1::value_type& zero = 0) {
	if (a.size() != b.size())
		return false;
	for(int i = 0; i < a.size(); i++) 
		if (!EqualRatio(At(a, i), At(b, i), ratio, zero)) 
			return false;
	return true;
}

template <class Range1, class Range2>
bool CompareDecimals(const Range1& a, const Range2& b, int numDecimals) {
	if (a.size() != b.size())
		return false;
	for (int i = 0; i < a.size(); i++) 
		if (!EqualDecimals(a[i], b[i], numDecimals)) 
			return false;
	return true;
}

template <class Range>
String ToString(const Range& a) {
	String ret;
	for (int i = 0; i < a.size(); i++) {
		if (i > 0)
			ret << ";";
		ret << a[i]; 
	}
	return ret;
}

template <class Range1, class Range2>
void SetSortOrder(Range1& a, const Range2& order) {
	ASSERT(a.size() == order.size());
	Range1 temp = clone(a);
	for (int i = 0; i < order.size(); ++i)
		a[i] = pick(temp[order[i]]);	
}

// Trims a range of Strings. It only works if Range has size() function
template <class Range>
typename std::enable_if<std::is_member_function_pointer<decltype(&Range::size)>::value, Range>::type
Trim(const Range& a) {
	Range ret(a.size());
	for (int i = 0; i < a.size(); i++) 
		ret[i] = Trim(a[i]);
	return ret;
}


class RealTimeStop {  
typedef RealTimeStop CLASSNAME;
public:
	RealTimeStop() {
#ifdef CTRLLIB_H	
		callbackOn = false;
		lastTick = -1;
#endif 
		Start();
	}
	void Reset() {
		timeElapsed = lastTimeElapsed = 0;
#ifdef CTRLLIB_H
		if (!callbackOn) {
			timeCallback.Set(-5*1000, THISBACK(Tick));
			callbackOn = true;
		}
#endif
		isPaused = true;
		Continue();
	}
	void Start() {Reset();}
	void Pause(bool pause) {
		if (pause)
			Pause();
		else
			Continue();
	}
	void Pause() {
		if (!isPaused) { 		
			timeElapsed += (tmGetTimeX() - time0);
			isPaused = true;
		}
	}
	void Continue() {
		if (isPaused) {
			time0 = tmGetTimeX();
			isPaused = false;
		}
	}
	double Seconds() {
		if (isPaused)
			return timeElapsed;
		else
			return timeElapsed + (tmGetTimeX() - time0);
	}
	double Elapsed() {
		double t = Seconds();
		double elapsed = t - lastTimeElapsed;
		lastTimeElapsed = t;
		return elapsed;
	}
	void SetBack(double secs) {
		timeElapsed -= secs;
	}
	bool IsPaused()		{return isPaused;}
		
private:
	double timeElapsed;				// Time elapsed
	double time0;					// Time of last Continue()
	double lastTimeElapsed;
	bool isPaused;
#ifdef CTRLLIB_H
	bool callbackOn;
	double lastTick;
	TimeCallback timeCallback;
	
	void Tick() {
		double tActual = tmGetTimeX();
		if (!isPaused && lastTick > -1) {
			double deltaLastTick = tActual - lastTick;
			if (deltaLastTick > 5*10) 	// Some external issue has stopped normal running
				SetBack(deltaLastTick);	// Timeout timer is fixed accordingly
		}
		lastTick = tActual;
	}
#endif
};

class LocalProcessX
#ifdef CTRLLIB_H	
 : public Ctrl 
 #endif
 {
typedef LocalProcessX CLASSNAME;
public:
	virtual ~LocalProcessX() 		{Stop();}
	enum ProcessStatus {RUNNING = 1, STOP = 0, STOP_TIMEOUT = -1, STOP_USER = -2, STOP_NORESPONSE = -3};
	bool Start(const char *cmd, const char *envptr = nullptr, const char *dir = nullptr, double _refreshTime = -1, 
		double _maxTimeWithoutOutput = -1, double _maxRunTime = -1, bool convertcharset = true) {
		status = STOP;
		p.ConvertCharset(convertcharset);
		timeElapsed.Start();
		timeWithoutOutput.Start();
		if(!p.Start(cmd, envptr, dir))
			return false;
		status = RUNNING;
		maxTimeWithoutOutput = _maxTimeWithoutOutput;
		maxRunTime = _maxRunTime;
		refreshTime = _refreshTime;
	
#ifdef CTRLLIB_H
		if (refreshTime > 0) {
			if (!callbackOn) {
				timeCallback.Set(-int(refreshTime*1000), THISBACK(Perform));
				callbackOn = true;
			}
		}
#endif
		return true;
	}
	void Perform() {
		if (status <= 0)
			return;
		String out;
		p.Read(out);
		if(p.IsRunning()) {
#ifdef PLATFORM_WIN32			
			if (!p.IsPaused()) {
#endif
				if (maxTimeWithoutOutput > 0 && timeWithoutOutput.Seconds() > maxTimeWithoutOutput) 
					status = STOP_NORESPONSE;
				else if (maxRunTime > 0 	 && timeElapsed.Seconds() > maxRunTime) 
					status = STOP_TIMEOUT;
#ifdef PLATFORM_WIN32				
			}
#endif
		} else 
			status = STOP;
		
		bool resetTimeout = false;
		if (!out.IsEmpty())
			resetTimeout = true;
		
		if (!WhenTimer(timeElapsed.Seconds(), out, status <= 0, resetTimeout))
			status = STOP_USER;
		
		if (resetTimeout)
			timeWithoutOutput.Reset();
		
		if (status < 0)
			p.Kill();

#ifdef CTRLLIB_H		
		if (callbackOn) {
			timeCallback.Kill();
			callbackOn = false;
		}
#endif
	}
	void Stop(ProcessStatus _status = STOP_USER) {
		if (!IsRunning())
			return;
		status = _status;
		p.Kill();		
#ifdef CTRLLIB_H		
		if (callbackOn) {
			timeCallback.Kill();
			callbackOn = false;
		}
#endif		
	}
#ifdef PLATFORM_WIN32
	void Pause() {
		p.Pause();
		if (p.IsRunning()) {
			timeElapsed.Pause(p.IsPaused());
			timeWithoutOutput.Pause(p.IsPaused());
		}
	}
	bool IsPaused()			{return p.IsPaused();}
	double Seconds()		{return timeElapsed.Seconds();}
	void SetMaxRunTime(double t)	{maxRunTime = t;}
	double GetMaxRunTime()	{return maxRunTime;}
#endif
	void Write(String str) 	{p.Write(str);}
	int GetStatus()  		{return status;}
	int GetExitCode()		{return p.GetExitCode();}
	bool IsRunning() 		{return status > 0;}
	Function<bool(double, const String&, bool, bool&)> WhenTimer;
	#ifdef PLATFORM_WIN32
	DWORD GetPid()			{return p.GetPid();}
	uint64 GetMemory() 		{return p.GetMemory();}
	#endif
	
	virtual void  SetData(const Value& v)	{value = v;}
	virtual Value GetData() const       	{return value;}

private:
	Value value;
	LocalProcess2 p;
	RealTimeStop timeElapsed, timeWithoutOutput;
	ProcessStatus status = STOP;
	double maxTimeWithoutOutput = 0, maxRunTime = 0;
	double refreshTime = 0;
#ifdef CTRLLIB_H	
	bool callbackOn = false;
	TimeCallback timeCallback;
#endif
};

int LevenshteinDistance(const char *s, const char *t);
int DamerauLevenshteinDistance(const char *s, const char *t, size_t alphabetLength = 256);
int SentenceSimilitude(const char *s, const char *t);

//#define S(y)	Upp::String(y)
String S(const char *s);
String S(const Value &v);
	
template<class T>
void Jsonize(JsonIO& io, std::complex<T>& var) {
	T re, im;
	if (io.IsStoring()) {
		re = var.real();
		im = var.imag();
	}
	io("re", re)("im", im);
	if (io.IsLoading()) {
		var.real(re);
		var.imag(im);
	}
}

template<typename T>
String FormatComplex(std::complex<T> &val) {
	double real = val.real();
	double imag = val.imag();
	String ret;
	if (real == 0 || real == -0)
		ret << "0";
	else
		ret << FDS(real, 8);
	if (imag != 0 && imag != -0) {
		if (imag > 0)
			ret << " + i";
		else
			ret << " - i";
		ret << FDS(abs(imag), 8);
	}
	return ret;
}

size_t GetNumLines(Stream &stream);

class FileInLine : public FileIn {
public:
	explicit FileInLine(String _fileName) : FileIn(_fileName), line(0), fileName(_fileName) {};
	
	bool Open(const char *fn) {
		line = 0;
		return FileIn::Open(fn);
	}
	String GetLine() {
		line++;	
		return FileIn::GetLine();
	}
	String GetLine(int num) {
		if (num == 0)
			return String();
		for (int i = 0; i < num-1; ++i)
			GetLine();
		return GetLine();
	}
	int GetLineNumber()	const 	{return line;}
	String Str() const {
		if (line > 0)
			return Format(t_("file: '%s', line: %d: "), fileName, line);
		else
			return Format(t_("file: '%s': "), fileName);
	}
		
	struct Pos {
		Pos() : byt(0), line(0) {}
		int64 byt;
		int line;
	};
	
	Pos GetPos() {
		Pos ret;
		ret.byt = FileIn::GetPos();
		ret.line = line;
		return ret;
	}
	
	void SeekPos(Pos &ps) {
		FileIn::Seek(ps.byt);
		line = ps.line;
	}
	
private:
	int line;
	String fileName;
};

class FileInBinary : public FileIn {
public:
	FileInBinary()                  		        	{}
	explicit FileInBinary(const char *fn) : FileIn(fn)	{}
	
	using FileIn::Read;
	void Read(void *data, size_t sz) {
		int64 len = Get64(data, (int64)sz);
		if (len != int64(sz))
			throw Exc(Format(t_("Data not loaded in FileInBinary::Read(%ld)"), int64(sz)));
	}
	template <class T>
	T Read() {
		T data;
		Read(&data, sizeof(T));
		return data;
	}
};

class FileOutBinary : public FileOut {
public:
	explicit FileOutBinary(const char *fn) : FileOut(fn)	{}
	FileOutBinary()                          				{}

	using FileOut::Write;
	void Write(const void *data, size_t sz) {
		Put64(data, (int64)sz);
	}
	template <class T>
	void Write(T data) {
		Put64(&data, sizeof(T));
	}
};

class YmlParser {
public:
	explicit YmlParser(FileInLine &_in) {in = &_in;}	
	
	bool GetLine();
	bool FirstIs(const String &val);
	bool FirstMatch(const String &pattern);	
	int Index() const 					{return index[idvar];}
	const Vector<int> &GetIndex() const	{return index;}
	
	const String &GetVal() const		{return val;}
	
	Vector<double> GetVectorDouble() const;	
	Vector<Vector<double>> GetMatrixDouble(bool isrect = true) const;
	
	const Vector<String> &GetVector() const			{return matrix[0];}	
	const Vector<Vector<String>> &GetMatrix() const	{return matrix;}
	
	String StrVal() const;
	String StrVar() const;
	
	const String &GetText()	{return lastLine;}
	
	bool IsEof() const		{return in->IsEof();}
		
private:
	FileInLine *in = nullptr;
	Vector<int> indentation;
	int idvar;
	
	Vector<String> var;
	Vector<int> index;
	Vector<Vector<String>> matrix;
	String val;
	String lastLine;
};

class LineParser {
public:
	const int FIRST = 0;
	const int LAST = Null;
	
	LineParser() {}
	explicit LineParser(FileInLine &_in) {in = &_in;}
	
	LineParser& Load(String _line) {
		line = _line;
		fields = Split(line, IsSeparator, true);
		return *this;
	}
	LineParser& Load(String _line, const Vector<int> &separators) {
		line = _line;
		fields.Clear();
		int from = 0, to;
		for (int i = 0; i < separators.size(); ++i) {
			to = separators[i];
			if (to > line.GetCount()-1)
				break; 
			fields << line.Mid(from, to-from);
			from = to;
		}
		fields << line.Mid(from);
		return *this;
	}
	LineParser& LoadFields(String _line, const Vector<int> &pos) {
		line = _line;
		fields.Clear();
		for (int i = 0; i < pos.size()-1 && pos[i] < line.GetCount(); ++i)
			fields << Trim(line.Mid(pos[i], pos[i+1]-pos[i]));
		if (pos[pos.size()-1] < line.GetCount())
			fields << Trim(line.Mid(pos[pos.size()-1]));
		return *this;
	}
	String& GetLine(int num = 1) {
		ASSERT(in);
		Load(in->GetLine(num));
		return line;
	}
	String& GetLine_discard_empty() {
		ASSERT(in);
		while (!in->IsEof()) {
			Load(in->GetLine());
			if (size() > 0)
				return line;
		}
		return line;
	}
	String& GetLineFields(const Vector<int> &pos) {
		ASSERT(in);
		LoadFields(in->GetLine(), pos);
		return line;
	}
	bool IsEof() {
		ASSERT(in);
		return in->IsEof();
	}
	const String &GetText() const {
		return line;
	}
	const String &GetText(int i) const {
		if (fields.IsEmpty())
			throw Exc(in->Str() + t_("The row is empty"));
		if (IsNull(i))
			i = fields.GetCount()-1;
		CheckId(i);
		return fields[i];
	}
	int GetInt_nothrow(int i) const {
		if (fields.IsEmpty())
			return Null;//throw Exc(in->Str() + t_("No data available"));
		if (IsNull(i))
			i = fields.GetCount()-1;
		CheckId(i);
		return ScanInt(fields[i]);
	}
	int GetInt(int i) const {
		int res = GetInt_nothrow(i);
		if (IsNull(res)) {
			if (i < fields.size())
				throw Exc(in->Str() + Format(t_("Bad %s '%s' in field #%d, line:\n'%s'"), "integer", fields[i], i+1, line));
			else
				throw Exc(in->Str() + Format(t_("Field #%d not found in line:\n'%s'"), i+1, line));
		}
		return res; 
	}
	bool IsInt(int i) const {
		return !IsNull(GetInt_nothrow(i));
	}
	double GetDouble_nothrow(int i) const {
		if (fields.IsEmpty())
			return Null;//throw Exc(in->Str() + t_("No data available"));
		if (IsNull(i))
			i = fields.GetCount()-1;
		if (!CheckId_nothrow(i))
			return Null;
		String data = fields[i];
		data.Replace("D", "");
		return ScanDouble(data);
	}
	double GetDouble(int i) const {
		double res = GetDouble_nothrow(i);
		if (IsNull(res)) {
			if (i < fields.size())
				throw Exc(in->Str() + Format(t_("Bad %s '%s' in field #%d, line:\n'%s'"), "double", fields[i], i+1, line));
			else
				throw Exc(in->Str() + Format(t_("Field #%d not found in line:\n'%s'"), i+1, line));
		}
		return res; 
	}
	bool IsDouble(int i) const {
		return !IsNull(GetDouble_nothrow(i));
	}
	bool IsInLine(String str) {
		return line.Find(str) >= 0;
	}
	
	int size() const 		{return fields.GetCount();}
	int GetCount() const 	{return size();}
	bool IsEmpty() const 	{return size() == 0;}
	
	int (*IsSeparator)(int) = defaultIsSeparator;
		
protected:
	String line;
	Vector<String> fields;
	FileInLine *in = nullptr;
	
	bool CheckId_nothrow(int i) const {
		return i >= 0 && i < fields.GetCount();
	}
	void CheckId(int i) const {
		if (!CheckId_nothrow(i))
			throw Exc(in->Str() + Format(t_("Field #%d not found in line:\n'%s'"), i+1, line));
	}
	static int defaultIsSeparator(int c) {
		if (c == '\t' || c == ' ' || c == ';' || c == ',')
			return true;
		return false;
	}
};

enum CONSOLE_COLOR {
#ifdef PLATFORM_WIN32
    BLACK       = 0,
    BLUE        = FOREGROUND_BLUE,
    GREEN       = FOREGROUND_GREEN,
    CYAN        = FOREGROUND_GREEN | FOREGROUND_BLUE,
    RED         = FOREGROUND_RED,
    MAGENTA     = FOREGROUND_RED | FOREGROUND_BLUE,
    YELLOW      = FOREGROUND_RED | FOREGROUND_GREEN,
    GRAY        = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE,
    LTBLUE      = FOREGROUND_INTENSITY | FOREGROUND_BLUE,
    LTGREEN     = FOREGROUND_INTENSITY | FOREGROUND_GREEN,
    LTCYAN      = FOREGROUND_INTENSITY | FOREGROUND_GREEN | FOREGROUND_BLUE,
    LTRED       = FOREGROUND_INTENSITY | FOREGROUND_RED,
    LTMAGENTA	= FOREGROUND_INTENSITY | FOREGROUND_RED | FOREGROUND_BLUE,
    LTYELLOW    = FOREGROUND_INTENSITY | FOREGROUND_RED | FOREGROUND_GREEN,
    WHITE      	= FOREGROUND_INTENSITY | FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE,
    RESET, 
    PREVIOUS
#else
	BLACK     	= 30,
	RED     	= 31,
	GREEN   	= 32,
	YELLOW  	= 33,
	BLUE    	= 34,
	MAGENTA 	= 35,
	CYAN    	= 36,
	GRAY    	= 37,
	LTRED     	= 131,
	LTGREEN   	= 132,
	LTYELLOW  	= 133,
	LTBLUE    	= 134,
	LTMAGENTA 	= 135,
	LTCYAN    	= 136,
	WHITE    	= 137,
	RESET   	= 0,
	PREVIOUS	= 1000
#endif
};


bool SetConsoleColor(CONSOLE_COLOR color);
void ConsoleOutputDisable(bool disable);

String GetPythonDeclaration(const String &name, const String &include);
String CleanCFromDeclaration(const String &include, bool removeSemicolon = true);

class CoutStreamX : public Stream {
public:
	static void NoPrint(bool set = true) {noprint = set;}	
	
private:
	String buffer;
	static bool noprint;

	virtual void Flush() {
#ifdef PLATFORM_POSIX
		fflush(stdout);
#else
		ONCELOCK {
			SetConsoleOutputCP(65001); // set console to UTF8 mode
		}
		static HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
		dword dummy;
		WriteFile(h, ~buffer, (DWORD)buffer.GetCount(), &dummy, NULL);
		buffer.Clear();
#endif	
	}

	void Put0(int w) {
#ifdef PLATFORM_WIN32
		buffer.Cat(w);
		if(CheckUtf8(buffer) || buffer.GetCount() > 8)
			Flush();
#else
		putchar(w);
#endif
	}
	virtual void    _Put(int w) {
		if (noprint)
			return;
		if(w == '\n') {
#ifdef PLATFORM_WIN32
			Put0('\r');
#endif
			Put0('\n');
			Put0('>');
		}
		else
		if(w != '\r')
			Put0(w);
	}
	virtual   bool  IsOpen() const { return true; }
};

Stream& CoutX();

template <class T>
String LoadFromJsonError(T& var, const char *json) {
	if (json[0] == '\0')
		return t_("No data found");
	try {
		Value jv = ParseJSON(json);
		if (jv.IsError())
			return GetErrorText(jv);
		LoadFromJsonValue(var, jv);
	} catch (const ValueTypeError &err) {
		return err;
	} catch (const JsonizeError &err) {
		return err;
	} catch (...) {
		return t_("Unknown error");
	}
	return String();
}

class Grid  {
public:
	void ColWidths(const Vector<int> &colWidths);
	Grid &AddCol(int colWidth = 10);
	int GetWidth(int c)	const {
		if (c < widths.size())
			return widths[c];
		return widths[widths.size()-1];
	}
	
	Grid& SetCol(int col)		{actualCol = col;	return *this;}
	Grid& SetRow(int row)		{actualRow = row;	return *this;}
	Grid& NextCol()				{++actualCol;		return *this;}
	Grid& NextRow()				{++actualRow;		return *this;}
	Grid& NextRowLF()			{++actualRow; actualCol = 0;	return *this;}
	
	Grid& Set(int row, int col, const Value &data);
	Grid& Set(const Value &data);
	const Value &Get(int row, int col) const;
	Grid& SetRow(const Vector<Value> &data);
		
	template<typename T>
	static String Nvl(T cond, String val) {return IsFin(cond) && !IsNull(cond) ? val : String();}

	String AsString(bool format, bool removeEmpty, const String &separator = " ");
	String AsLatex(bool removeEmpty, bool setGrid, bool full, String caption, String label);
	
	Grid& SetNumHeaderRows(int n)	{numHeaderRows = n;	return *this;}
	int  GetNumHeaderRows()	const	{return numHeaderRows;}
	Grid& SetNumHeaderCols(int n)	{numHeaderCols = n;	return *this;}
	int  GetNumHeaderCols()	const	{return numHeaderCols;}
	
	int rows(int col = 0) const;
	int cols() const;

	Grid& SetTextColor(Color c);
	Grid& SetBackgroundColor(Color c);
	
	void SetVirtualCount(int n)	{
		vheader.Clear();
		vconvert.Clear();
		SetNumHeaderRows(1);
		vnum = n;
		isVirtual = true;
	}
	void AddVirtualCol(String name, Convert &c, int colWidth = 10) {
		vheader << name;
		vconvert << &c;
		AddCol(colWidth);
		numHeaderCols++;
	}
	const String& GetVirtualHeader(int id)	 const 	{return vheader[id];}
	String& SetVirtualHeader(int id, String txt)	{return vheader[id] = txt;}
	
	const Convert& GetVirtualConvert(int id) const {return *vconvert[id];}
	
	void Clear() {
		columns.Clear();
		widths.Clear();	
		numHeaderRows = numHeaderCols = actualCol = actualRow = 0;
	
		isVirtual = false;
		vheader.Clear();
		vconvert.Clear();
		vnum = 0;
		
		formatCell.Clear();
		formatRange.Clear();
		alignCell.Clear();
		alignRange.Clear();
	}
	
	bool IsEmpty() {return columns.IsEmpty();}
	
	struct Fmt : Moveable<Fmt> {
		Fmt() {}
		Fmt(const Font &fnt, const Color &text = Null, const Color &back = Null) : fnt(fnt), text(text), back(back) {}
		Font fnt;
		Color text = Null, back = Null;
	};
	
	Grid& Set(int row, int col, const Fmt &fmt);
	Grid& Set(const Fmt &fmt);
	const Fmt &GetFormat(int row, int col) const;
	
	Grid& Set(int row, int col, Alignment align);
	Grid& Set(Alignment align);
	Alignment GetAlignment(int row, int col) const;
	
	void SetDefaultWidth(int w)	{defaultWidth = w;}
	
private:
	Array<Array<Value>> columns;
	Vector<int> widths;		
	
	int numHeaderRows = 0, numHeaderCols = 0;
	int actualCol = 0, actualRow = 0;
	
	int defaultWidth = 10;
	
	bool isVirtual = false;
	int vnum = 0;
	Vector<String> vheader;
	Vector<Convert*> vconvert;
	
	VectorMap<Tuple<int, int>, Fmt> formatCell;
	VectorMap<Tuple<int, int, int, int>, Fmt> formatRange;
	VectorMap<Tuple<int, int>, int> alignCell;
	VectorMap<Tuple<int, int, int, int>, int> alignRange;
};

String DocxToText(String filename, bool noFormat);
String PptxToText(String filename);
String XlsxToText(String filename);
Index<String> TextToWords(const String &str, bool repeat);
	
}
  
#endif

// SPDX-License-Identifier: Apache-2.0
// Copyright 2021 - 2022, the Anboto author and contributors
#include <Core/Core.h>

using namespace Upp;

#include "Spreadsheet.h"


void SpreadsheetDemo(Spreadsheet &spreadsheet) {
	spreadsheet.Open("myfile.xls");
	spreadsheet.SetData(4, 6, "Hello world");
}

void PluginDemo() {
	Spreadsheet spreadsheet;
	
	if (!PluginInit(spreadsheet, "Open")) {
		UppLog() << "Impossible to init OpenOffice/LibreOffice";
		return;
	}
	SpreadsheetDemo(spreadsheet);
	if (!PluginInit(spreadsheet, "Excel")) {
		UppLog() << "Impossible to init Excel";
		return;
	}
	SpreadsheetDemo(spreadsheet);
}

void FilesDemo() {
	{
		String from = "/books/technology/computers"; 
		String path = "/books/biology/mammals";
		String ret = GetRelativePath(from, path, false);
		if (ret.IsVoid())
			ret = "Null";
		UppLog() << "\nGetRelativePath(\"" << from << "\", \"" << path << "\")\n= \"" << ret << "\"";
		VERIFY(ret == "../../biology/mammals");
	}
	{
		String from = "/books/technology/computers";
		String path = "/books/technology/computers";
		String ret = GetRelativePath(from, path, false);
		if (ret.IsVoid())
			ret = "Null";
		UppLog() << "\nGetRelativePath(\"" << from << "\", \"" << path << "\")\n= \"" << ret << "\"";
		VERIFY(ret == "");
	}
	{
		String from = "/books/technology/computers";
		String path = "/books2/biology/mammals";
		String ret = GetRelativePath(from, path, false);
		if (ret.IsVoid())
			ret = "Null";
		UppLog() << "\nGetRelativePath(\"" << from << "\", \"" << path << "\")\n= \"" << ret << "\"";
		VERIFY(ret == "../../../books2/biology/mammals");
	}
	{
		String from = "c:/books/technology/computers";
		String path = "y:/books2/biology/mammals";
		String ret = GetRelativePath(from, path, false);
		if (ret.IsVoid())
			ret = "Null";
		UppLog() << "\nGetRelativePath(\"" << from << "\", \"" << path << "\")\n= \"" << ret << "\"";
		VERIFY(ret == "Null");
	}
	UppLog() << "\n";
	
	String filename1 = AppendFileNameX(GetExeFolder(), "Demo", "file1.txt");
	RealizePath(filename1);
	String str1 = "This is the First string";
	SaveFile(filename1, str1);
	String filename2 = AppendFileNameX(GetExeFolder(), "Demo", "file2.txt");
	String str2 = "This is the Second string";
	SaveFile(filename2, str2);
	
	FileCat(filename1, filename2);
	UppLog() << Format("\nFileCat(%s, %s)", filename1, filename2);

//	LaunchFile(filename1);	
//	UppLog() << Format("LaunchFile(%s)", filename1);		

	int intres = FileCompare(filename1, filename2);
	UppLog() << Format("\nFileCompare(%s, %s) = %d (has to be -1)", filename1, filename2, intres);

	int64 pos = FindStringInFile(filename1, "Second");
	UppLog() << Format("\nFindStringInFile(%s, %s) = %d (has to be 35)", filename1, filename2, pos);

	bool boolres = FileStrAppend(filename1, ". Text appended at the end.");
	UppLog() << Format("\nFileStrAppend(%s, \". Text appended at the end.\") = %s (has to be true)", 
				filename1, boolres ? "true" : "false");
	
	//boolres = UpperFolder(GetFileDirectory(filename1));
	//UppLog() << Format("\nUpperFolder(%s) = %s (has to be true)", filename1, boolres ? "true" : "false");
	
	String upperFolder = GetUpperFolder(GetFileDirectory(filename1));
	UppLog() << Format("\nGetUpperFolder(%s) = %s (has to be %s)", filename1, upperFolder, GetExeFolder());
	
	String stringres = GetNextFolder(GetUpperFolder(GetExeFolder()), GetFileDirectory(filename1));
	UppLog() << Format("\nGetNextFolder(%s, %s) = %s (has to be %s)", GetUpperFolder(GetExeFolder()), 
										GetFileDirectory(filename1), stringres, GetExeFolder());
	
	String lfilename1 = ToLower(filename1);
	filename1 = FileRealName(lfilename1);
	UppLog() << Format("\nFileRealName(%s) = %s", lfilename1, filename1);
}

void NonReentrantDemo() {
	UppLog() << "\nTrying to enter Non reentrant. ";
	NON_REENTRANT_V;
	
	UppLog() << "Entered in Non reentrant. It has to be once.";
	NonReentrantDemo();	
	NonReentrantDemo();
}

void DistanceDemo() {
	UppLog() << Format("\nDistance between 'hello' and 'hello'  is %d", DamerauLevenshteinDistance("hello", "hello"));
	UppLog() << Format("\nDistance between 'hello' and 'helo'   is %d", DamerauLevenshteinDistance("hello", "helo"));
	UppLog() << Format("\nDistance between 'hello' and 'heloo'  is %d", DamerauLevenshteinDistance("hello", "helloo"));
	UppLog() << Format("\nDistance between 'hello' and 'yellow' is %d", DamerauLevenshteinDistance("hello", "yellow"));
}

void InstalledDemo() {
	Array<SoftwareDetails> softs = GetInstalledSoftware();
	
	for (auto &soft : softs) {
		UppLog() << soft.name << "\n";
		UppLog() << Format("\tPublisher:   %s\n", soft.publisher);
		UppLog() << Format("\tVersion:     %s\n", soft.version);
		UppLog() << Format("\tArchitect:   %s\n", soft.architecture);
		UppLog() << Format("\tPath:        %s\n", soft.path);
		UppLog() << Format("\tDescription: %s\n", soft.description);
	}
	UppLog() << "\n";
}	
	
void MiscellaneousDemos() {
	UppLog() << "\nFloat formatting\n";
	
	const double num = 2345678.9017654;
	UppLog() << "\nFormatting 2345678.9017654";
	String str;
	UppLog() << "\nFormatF(, 2) = '" << (str = FormatF(num, 2)) << "'";
	VERIFY(str == "2345678.90");
	UppLog() << "\nFormatF(, 3) = '" << (str = FormatF(num, 3)) << "'";
	VERIFY(str == "2345678.902");
	UppLog() << "\nFormatF(, 4) = '" << (str = FormatF(num, 4)) << "'";
	VERIFY(str == "2345678.9018");
	UppLog() << "\nFormatF(, 5) = '" << (str = FormatF(num, 5)) << "'";
	VERIFY(str == "2345678.90177");
	
	double res;
	UppLog() << "\nfround(, 2) = '" << (res = fround(num, 2)) << "'";
	VERIFY(res == 2345678.90);
	UppLog() << "\nfround(, 3) = '" << (res = fround(num, 3)) << "'";
	VERIFY(res == 2345678.902);
	UppLog() << "\nfround(, 4) = '" << (res = fround(num, 4)) << "'";
	VERIFY(res == 2345678.9018);
	UppLog() << "\nfround(, 5) = '" << (res = fround(num, 5)) << "'";
	VERIFY(res == 2345678.90177);
	
	UppLog() << "\nFormatDoubleSize(, 3) = '" << (str = FDS(num, 3, true)) << "'";
	VERIFY(str == "2.3E6");
	UppLog() << "\nFormatDoubleSize(, 4) = '" << (str = FDS(num, 4, true)) << "'";
	VERIFY(str == "2.3E6");
	UppLog() << "\nFormatDoubleSize(, 5) = '" << (str = FDS(num, 5, true)) << "'";
	VERIFY(str == "2.3E6");
	UppLog() << "\nFormatDoubleSize(, 6) = '" << (str = FDS(num, 6, true)) << "'";
	VERIFY(str == "2.35E6");
	UppLog() << "\nFormatDoubleSize(, 7) = '" << (str = FDS(num, 7, true)) << "'";
	VERIFY(str == "2345679");
	UppLog() << "\nFormatDoubleSize(, 8) = '" << (str = FDS(num, 8, true)) << "'";
	VERIFY(str == " 2345679");
	UppLog() << "\nFormatDoubleSize(, 9) = '" << (str = FDS(num, 9, true)) << "'";
	VERIFY(str == "2345678.9");
	UppLog() << "\nFormatDoubleSize(,10) = '" << (str = FDS(num,10, true)) << "'";
	VERIFY(str == " 2345678.9");
	UppLog() << "\nFormatDoubleSize(,11) = '" << (str = FDS(num,11, true)) << "'";
	VERIFY(str == "2345678.902");
	UppLog() << "\nFormatDoubleSize(,12) = '" << (str = FDS(num,12, true)) << "'";
	VERIFY(str == "2345678.9018");
	UppLog() << "\nFormatDoubleSize(,13) = '" << (str = FDS(num,13, true)) << "'";
	VERIFY(str == "2345678.90177");
	

	UppLog() << "\nFloat formatting\n";
	
	UppLog() << Format("\nBase 10 exponent for %e is %d", 123456.789, GetExponent10(123456.789));
	UppLog() << Format("\nBase 10 exponent for %e is %d", 1.234E57, GetExponent10(1.234E57));
	UppLog() << "\n";
	UppLog() << Format("\nNumber with the least significant digits between %f and %f is %f", -1.12345, 2.468, NumberWithLeastSignificantDigits(-1.12345, 2.468));
	UppLog() << Format("\nNumber with the least significant digits between %f and %f is %f", 1.12345, 2.468, NumberWithLeastSignificantDigits(1.12345, 2.468));
	UppLog() << Format("\nNumber with the least significant digits between %e and %e is %e", 1.12345E33, 2.468E33, NumberWithLeastSignificantDigits(1.12345E33, 2.468E33));
	UppLog() << Format("\nNumber with the least significant digits between %e and %e is %e", 1.12345E32, 2.468E33, NumberWithLeastSignificantDigits(1.12345E32, 2.468E33));
	

	UppLog() << "\nIsNull() testing\n";
	{
		float f = Null;
		double d = f;
		float f2 = d;
		VERIFY(IsNull(f));
		VERIFY(IsNull(d));
		VERIFY(IsNull(f2));
	}
	
	UppLog() << "\nNaN testing\n";

	int n = Null;
	VERIFY(!IsNum(n));
	n = 23;
	VERIFY(IsNum(n));
	
	float f = std::numeric_limits<float>::quiet_NaN();
	VERIFY(!IsNum(f));
	f = 23;
	VERIFY(IsNum(f));	
	
	double d = NaNDouble;
	VERIFY(!IsNum(d));
	d = Null;
	VERIFY(!IsNum(d));
	d = 23;
	VERIFY(IsNum(d));	

	std::complex<double> c = Null;
	VERIFY(!IsNum(c));
	c = {23, NaNDouble};
	VERIFY(!IsNum(c));
	c = NaNComplex;
	VERIFY(!IsNum(c));
	c = {23, 12};
	VERIFY(IsNum(c));
		
		
	UppLog() << "\nConsole colours\n";
	
	SetConsoleColor(CONSOLE_COLOR::LTRED);
	UppLog() << "This message is in red\n";
	SetConsoleColor(CONSOLE_COLOR::LTYELLOW);
	UppLog() << "This message is in yellow\n";
	SetConsoleColor(CONSOLE_COLOR::RESET);
	UppLog() << "This message is in normal color\n";
	
	UppLog() << "Next text messages (printf, Cout(), UppLog()) will be disabled\n";
	ConsoleOutputDisable(true);
	Cout() << "Text with Cout()\n";
	UppLog() << "Text with UppLog()\n";
	printf("Text with printf()\n");
	ConsoleOutputDisable(false);
	UppLog() << "Text messages are now enabled\n";
}

CONSOLE_APP_MAIN
{	
	StdLogSetup(LOG_COUT|LOG_FILE);

	UppLog() << "Installed software demo";
	InstalledDemo();
	
	UppLog() << "Miscellaneous demos";
	MiscellaneousDemos();
	
	UppLog() << "\n\nFiles demo";
	FilesDemo();
	
	UppLog() << "\n\nPlugin demo";
	PluginDemo();	

	UppLog() << "\n\nNonReentrant demo";
	NonReentrantDemo();	
	
	UppLog() << "\n\nDistance demo";
	DistanceDemo();	
	
	#ifdef flagDEBUG
	UppLog() << "\n";
	Cout() << "\nPress enter key to end";
	ReadStdIn();
	#endif   
}


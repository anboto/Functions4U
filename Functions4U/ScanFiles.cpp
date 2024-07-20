#include <Core/Core.h>
#include "Functions4U.h"
#include <plugin/sqlite3/Sqlite3.h>
#include <plugin/zip/zip.h>


// pdftotext -nopgbrk fichero.pdf fichero.txt

namespace Upp {
	
String DocxToText(String filename, bool noFormat) {
    FileUnZip unzip(filename);
	if (unzip.IsError()) 
        return String();
    
    String strxml = unzip.ReadFile("word/document.xml");
	if (strxml.IsEmpty())
		return String();
	
	XmlNode doc = ParseXML(strxml);
	const XmlNode &body = doc["w:document"]["w:body"];

	String text;
    for (const XmlNode &node : body) {
        if (node.GetTag() == "w:p") { // paragraph
            for (const XmlNode &textNode : node) {
                if (textNode.GetTag() == "w:r") {
                    const XmlNode &tt = textNode["w:t"];
                    for (int i = 0; i < tt.GetCount(); ++i)
                    	text << tt.Node(i).GetText();
                } else if (!noFormat && textNode.GetTag() == "w:pPr") {
            		const XmlNode &tt = textNode["w:pStyle"];
            		for (int i = 0; i < tt.GetAttrCount(); ++i)
            			if (tt.Attr(i) == "ListParagraph")
            				text << "- ";
                }
            }
            if (!noFormat)
            	text << '\n';
            else 
                text << ' ';
        }
    }
    return text;
}

String PptxToText(String filename) {
    FileUnZip unzip(filename);
	if (unzip.IsError()) 
        return String();
    
 	String text;
    for (int i = 0; i < unzip.GetCount(); ++i) {
        const String &path = unzip.GetPath(i);
        if (path.StartsWith("ppt/slides/slide") && path.EndsWith(".xml")) {
            unzip.Seek(i);
            String strxml = unzip.ReadFile();
            if (!strxml.IsEmpty()) {
				XmlNode doc = ParseXML(strxml);
				const XmlNode &body = doc["p:sld"]["p:cSld"]["p:spTree"];
				
				for (const XmlNode &node : body) {
 					if (node.GetTag() == "p:sp") {
 						for (const XmlNode &sp : node) {		
 							if (sp.GetTag() == "p:txBody") {	
 								for (const XmlNode &txBody : sp) {		
 									if (txBody.GetTag() == "a:p") {
 										for (const XmlNode &ap : txBody) {			
		 									if (ap.GetTag() == "a:r") {
		 										const XmlNode &tt = ap["a:t"];
		                    					for (int i = 0; i < tt.GetCount(); ++i)
		                    						text << tt.Node(i).GetText();
		 									}
 										}
 									}
 									text << "\n";
 								}
 							}
 						}
 					}
				}
            }
        }
    }
    return text;
}

String XlsxToText(String filename) {
    FileUnZip unzip(filename);
	if (unzip.IsError()) 
        return String();
 
    String strxml = unzip.ReadFile("xl/sharedStrings.xml");
	if (strxml.IsEmpty())
		return String();
	
	XmlNode doc = ParseXML(strxml);
	const XmlNode &body = doc["sst"];

	String text;
    for (const XmlNode &node : body) {
		if (node.GetTag() == "si") {
            const XmlNode &tt = node["t"];
            for (int i = 0; i < tt.GetCount(); ++i)
            	text << tt.Node(i).GetText() << "\n";
        }
    }
    return text;
}

Index<String> TextToWords(const String &str, bool repeat) {
	WString wstr(str);
	
	Index<String> ret;
	
	WString wtxt;
	for (int i = 0; i < wstr.GetCount(); ++i) {
		if (IsLetter(wstr[i]))
			wtxt.Cat(wstr[i]);
		else if (wtxt.GetCount() > 0) {
			if (repeat)
				ret << wtxt.ToString();
			else
				ret.FindAdd(wtxt.ToString());
			wtxt.Clear();
		}
	}
	if (wtxt.GetCount() > 0) {
		if (repeat)
				ret << wtxt.ToString();
		else
			ret.FindAdd(wtxt.ToString());
	}	
	return ret;
}

/*

void ScanForDocxFiles(Vector<String>& fileList, const String& directory);
void ScanForPptxFiles(Vector<String>& fileList, const String& directory);
void ScanForXlsxFiles(Vector<String>& fileList, const String& directory);

String ExtractTextFromDocx(const String& filePath);
String ExtractTextFromPptx(const String& filePath);
String ExtractTextFromXlsx(const String& filePath);

std::unordered_set<String> Tokenize(const String& text);
void SaveWordsToDatabase(const String& filePath, const std::unordered_set<String>& words);
void CreateFTSTable();
void SearchKeyword(const String& keyword);

std::unordered_set<String> Tokenize(const String& text) {
    std::unordered_set<String> words;
    StringStream ss(text);
    String word;
    while (!ss.IsEof()) {
        ss >> word;
        word = ToLower(word);
        word = Filter(word, [](int c) { return IsAlNum(c); });
        words.insert(word);
    }
    return words;
}

void SaveWordsToDatabase(const String& filePath, const std::unordered_set<String>& words) {
    Sqlite3Session sqlite3;
    if (!sqlite3.Open("file_words.db"))
        throw Exc("Can't create or open database file");

    Sql sql(sqlite3);
    sql.Execute("CREATE TABLE IF NOT EXISTS FileWords (path TEXT PRIMARY KEY, words TEXT)");

    String wordsStr;
    for (const auto& word : words)
        wordsStr << word << ' ';

    sql.Execute("INSERT OR REPLACE INTO FileWords (path, words) VALUES (?, ?)",
                filePath, wordsStr);
}


void SearchKeyword(const String& keyword) {
    Sqlite3Session sqlite3;
    if (!sqlite3.Open("file_words.db"))
        throw Exc("Can't create or open database file");

    Sql sql(sqlite3);
    sql.Execute("SELECT path FROM FileWords WHERE words MATCH ?", keyword);
    while (sql.Fetch())
        Cout() << sql[0] << '\n';
}

CONSOLE_APP_MAIN {
    const String directory = "/path/to/scan";

    // Create FTS table
    CreateFTSTable();

    // Scan for DOCX files
    Vector<String> docxFiles;
    ScanForDocxFiles(docxFiles, directory);
    for (const auto& filePath : docxFiles) {
        String text = ExtractTextFromDocx(filePath);
        std::unordered_set<String> words = Tokenize(text);
        SaveWordsToDatabase(filePath, words);
    }

    // Scan for PPTX files
    Vector<String> pptxFiles;
    ScanForPptxFiles(pptxFiles, directory);
    for (const auto& filePath : pptxFiles) {
        String text = ExtractTextFromPptx(filePath);
        std::unordered_set<String> words = Tokenize(text);
        SaveWordsToDatabase(filePath, words);
    }

    // Scan for XLSX files
    Vector<String> xlsxFiles;
    ScanForXlsxFiles(xlsxFiles, directory);
    for (const auto& filePath : xlsxFiles) {
        String text = ExtractTextFromXlsx(filePath);
        std::unordered_set<String> words = Tokenize(text);
        SaveWordsToDatabase(filePath, words);
    }

    // Search for a keyword
    SearchKeyword("example");
}*/

}
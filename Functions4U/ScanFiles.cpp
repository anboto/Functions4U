#include <Core/Core.h>
#include "Functions4U.h"
#include <plugin/sqlite3/Sqlite3.h>
#include <plugin/zip/zip.h>


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

String XlsxToText_shared(FileUnZip &unzip) {
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

String XlsxToText_sheets(FileUnZip &unzip) {
	String text;
	for (int i = 1; true; ++i) {
	    String strxml = unzip.ReadFile(Format("xl/worksheets/sheet%d.xml", i).begin());
	    if (strxml.IsVoid())
			return text;
		if (strxml.IsEmpty())
			continue;
	
		XmlNode doc = ParseXML(strxml);
		const XmlNode &body = doc["sheetData"];

	    for (const XmlNode &node : body) {
			if (node.GetTag() == "row") {
	            const XmlNode &tt = node["t"];
	            for (int i = 0; i < tt.GetCount(); ++i)
	            	text << tt.Node(i).GetText() << "\n";
	        }
	    }
	    text << "\n";
	}
}

String XlsxToText(String filename) {
	FileUnZip unzip(filename);
	if (unzip.IsError()) 
        return String();
	
	return XlsxToText_shared(unzip) + S("\n") + XlsxToText_sheets(unzip);
}

Index<String> TextToWords(const String &str, bool repeat) {
	WString wstr(str);
	
	Index<String> ret;
	
	WString wtxt;
	for (int i = 0; i < wstr.GetCount(); ++i) {
		if (IsLetter(wstr[i]) || IsDigit(wstr[i]))
			wtxt.Cat(RemoveAccent(wstr[i]).ToWString());
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

}
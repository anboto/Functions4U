// SPDX-License-Identifier: Apache-2.0
// Copyright 2021 - 2023, the Anboto author and contributors
#include <Core/Core.h>
#include "Functions4U.h"

#include <Functions4U/EnableWarnings.h>

namespace Upp {
	
bool YmlParser::GetLine() {
	auto DiscardSpaces = [](String &line, int &pos) {
		for (; pos < line.GetCount(); ++pos) {
			if (line[pos] != ' ' && line[pos] != '\t')
				break;
		}
	};
	
	auto GetVector = [](FileInLine &inn, const String &llastLine, String str = "")->Vector<String> {
		Vector<String> list;

		while (true) {
			if (list.IsEmpty()) {
				int id = str.Find("[");
				if (id >= 0) {
					str = str.Mid(id+1);
					list = Split(str, ",");
					str.Clear();
				} else if (!IsNull(ScanDouble(str))) {
					list.Append({str});
					return list;	
				}
			} else {
				list.Append(Split(str, ","));
				str.Clear();
			}
			String &last = list[list.size()-1];
			if (!list.IsEmpty() && last.EndsWith("]")) {
				last = last.Left(last.GetCount()-1);
				break;
			}
			if (inn.IsEof())
				throw Exc("End Of File found when reading a matrix");
			String line = inn.GetLine();
			line << llastLine << "\n";
			str << Trim(line);
		}
		
		for (int i = list.size()-1; i >= 0; --i) {
			if (list[i].IsEmpty())
				list.Remove(i);
		}
		return list;
	};
	
	lastLine.Clear();
	
	while (!in->IsEof()) {	
		idvar = -1;
		val.Clear();
		matrix.Clear();
		
		String line;
		while (!in->IsEof()) {		
			line = in->GetLine();
			//lastLine << line << "\n";
			int idc = line.Find("#");
			if (idc >= 0)
				line = line.Left(idc);		// Remove comments
			String trim = TrimLeft(line);
			if (!trim.IsEmpty() && !(trim.Find("%YAML") >= 0) && 
				!trim.StartsWith("---") && !trim.StartsWith("..."))
				break;
		}
		if (in->IsEof()) 
			return false;
		
		int level = 0;
		int pos = 0;
		bool hyphen = false;
		for (; pos < line.GetCount(); ++pos) {
			if (line[pos] == ' ')
				level++;
			else if (line[pos] == '\t')
				level += 2;
			else if (line[pos] == '-') {
				if (hyphen)
					throw Exc(t_("Two hyphen in series are not allowed"));
				level++;	
				hyphen = true;	
			} else
				break;
		}
				
		String varname;
		for (; pos < line.GetCount(); ++pos) {
			if (line[pos] == ':') {
				varname = Trim(varname);	// Discards the white spaces until the :
				pos++;
				break;
			}
			varname.Cat(line[pos]);
		}
		
		int id;
		for (id = 0; id < indentation.size(); ++id) {
			if (indentation[id] == level) {
				indentation.SetCountR(id+1);
				var.SetCountR(id+1);			var[id] = varname;
				index.SetCountR(id+1);
				if (hyphen)
					index[id]++;
				break;
			} else if (indentation[id] > level)
				throw Exc(t_("Indentation doesn't match with previous"));
		}
		if (id == indentation.size()) {
			indentation << level;
			var << varname;
			if (!hyphen)
				index << -1;
			else
				index << 0;
		}
		
		DiscardSpaces(line, pos);			
		
		if (line[pos] == '[') {
			Vector<String> v = GetVector(*in, lastLine, line.Mid(pos));
			if (!v.IsEmpty())
				matrix << pick(v);
		} else if (pos >= line.GetCount()) {		// The val is not in the same line
			while (!in->IsEof()) {
				FileInLine::Pos fpos = in->GetPos();
				line = in->GetLine();
				pos = 0;
				DiscardSpaces(line, pos);
				if (line[pos] != '-') {
					in->SeekPos(fpos);
					break;
				}
				pos++;
				DiscardSpaces(line, pos);
				if ((line[pos] < '0' || line[pos] > '9') && line[pos] != '[') {
					in->SeekPos(fpos);	// It is a var name
					break;
				}
				Vector<String> v = GetVector(*in, lastLine, line.Mid(pos));
				if (!v.IsEmpty())
					matrix << pick(v);
			}
		} else 
			val = Trim(line.Mid(pos)); 	
			
		if (!matrix.IsEmpty() || !val.IsEmpty())	
			break;
	}

	return true;
}			

bool YmlParser::FirstIs(const String &vval) {
	if (idvar >= var.size()-1)
		return false;

	if (var[idvar+1] == vval) {
		idvar++;
		return true;
	}
	return false;
}

bool YmlParser::FirstMatch(const String &pattern) {
	if (idvar >= var.size()-1)
		return false;

	if (PatternMatch(pattern, var[idvar+1])) {
		idvar++;
		return true;
	}
	return false;
}

String YmlParser::StrVar() const {
	String str;
	for (int i = 0; i < var.size(); ++i) {
		if (i > 0)
			str << ".";	
		str << var[i];
		if (index[i] >= 0)
			str << "[" << index[i] << "]";
	}
	return str;
}
		
String YmlParser::StrVal() const {
	if (matrix.IsEmpty())	
		return val;
	else {
		String ret;
		if (matrix.size() == 1)
			ret << matrix[0];
		else
			ret << matrix;
		return ret;
	}
}
	
Vector<double> YmlParser::GetVectorDouble() const {
	Vector<double> ret;
	
	if (matrix.IsEmpty())
		return ret;	
	
	const Vector<String> &vector = matrix[0];
	ret.SetCount(vector.size());
	for (int i = 0; i < ret.size(); ++i)
		ret[i] = ScanDouble(vector[i]);
			
	return ret;
}

Vector<Vector<double>> YmlParser::GetMatrixDouble(bool isrect) const {
	Vector<Vector<double>> ret;
	
	if (matrix.IsEmpty())
		return ret;	
	
	ret.SetCount(matrix.size());
	for (int i = 0; i < ret.size(); ++i) {
		const Vector<String> &vector = matrix[i];
		Vector<double> &vectord = ret[i];
		if (isrect && i > 0 && vector.size() != matrix[0].size())
			throw Exc(t_("The matrix should have the same number of columns in each row"));
		vectord.SetCount(vector.size());
		for (int j = 0; j < vector.size(); ++j)
			vectord[j] = ScanDouble(vector[j]);
	}
	return ret;
}
		
}
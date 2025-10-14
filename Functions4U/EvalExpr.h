// SPDX-License-Identifier: Apache-2.0
// Copyright 2021 - 2025, the Anboto author and contributors
#ifndef _Functions4U_EvalExpr_h
#define _Functions4U_EvalExpr_h


namespace Upp {

class PostFixOperation {
public:
	struct Item {
		Item(char type, int id, double val = Null) : type(type), id(id), val(val){}
		char type;
		int id;
		double val;
	};
	Vector<Item> operations;
	
	const Item &Get(int id) const {
		if (id >= operations.size())
			throw Exc("Problem parsing equation");
		return operations[id];
	}
	
	void Append(const PostFixOperation &op) {operations.Append(op.operations);}
	void Append(PostFixOperation &&op) 		{operations.Append(pick(op.operations));}
	void Append(Item &&item) 				{operations.Add(pick(item));}
	void Insert0(Item &&item) 				{operations.Insert(0, pick(item));}
	
	bool IsEmpty() const				{return operations.IsEmpty();}
	
	String ToString() {
		String ret;
	    for (const PostFixOperation::Item &v : operations) {
	        switch (v.type) {
			case 'n':	ret << v.val;				break;
			case 'v':	ret << "variab:" << v.id;  	break;
			case 'a':	ret << "assign:" << v.id;  	break;
			case 'f':	ret << "fun(1):" << v.id;	break;
			case 'g':	ret << "fun(2):" << v.id;  	break;
			default:	NEVER();
	        }
        	ret << ' ';
        }
        return ret;
	}
};

class EvalExprX {
public:
	EvalExprX() {
		AddFunction("-", minus_fun).AddFunction("abs", std::abs).AddFunction("sqrt", std::sqrt).
		AddFunction("sum", sum_fun).AddFunction("diff", diff_fun).AddFunction("mult", mult_fun).
		AddFunction("div", div_fun).AddFunction("pow", std::pow);
		AddConstant("pi", M_PI);
	}
	
	PostFixOperation Get(const char *s) {
		CParser p(s);
		if(p.IsId()) {
			CParser::Pos pos = p.GetPos();
			String var = p.ReadId();
			if (functions1.Find(var) < 0 && functions2.Find(var) < 0) {
				if(p.Char('=')) {
					if (!WhenDeclareVariable)
						throw Exc("No way to add a variable");
					PostFixOperation x = Exp(p);
					x.Insert0(PostFixOperation::Item('a', WhenDeclareVariable(var)));
					return x;
	            }
			} 
			p.SetPos(pos);
		}
	    return Exp(p);
	}
	
	double Eval(const PostFixOperation &list) {
		int id = 0;
		return Eval0(list, id);
	}

	double Eval(const char *s) {
		return Eval(Get(s));
	}
	
	EvalExprX &AddFunction(const char *name, double(*fun)(double))	{
		functions1 << name;
		functionsCalls1 << fun;
		return *this;
	}

	EvalExprX &AddFunction(const char *name, double(*fun)(double, double))	{
		functions2 << name;
		functionsCalls2 << fun;
		return *this;
	}

	EvalExprX &AddConstant(const char *name, double val)	{
		constants << name;
		constantValues << val;
		return *this;
	}
	
	Function<int(const char *name)> WhenGetVariableId;
	Function<double(int id)> WhenGetVariableValue;
	
	Function<int(const char *name)> WhenDeclareVariable;
	Function<void(int id, double val)> WhenSetVariableValue;
			
private:
	Index<String> functions1;
	Vector<double(*)(double)> functionsCalls1;
	Index<String> functions2;
	Vector<double(*)(double, double)> functionsCalls2;
	
	Index<String> constants;
	Vector<double> constantValues;

	PostFixOperation Term(CParser& p) {
		PostFixOperation ret;
	    if(p.IsId()) {
	        String sid = p.ReadId();
	        int id;
	        if ((id = constants.Find(sid)) >= 0)
	        	ret.Append(PostFixOperation::Item('n', Null, constantValues[id]));    
	        else if (WhenGetVariableId && (id = WhenGetVariableId(sid)) >= 0)
	            ret.Append(PostFixOperation::Item('v', id));
	        else if ((id = functions1.Find(sid)) >= 0) {
	            ret.Append(PostFixOperation::Item('f', id));
		        p.PassChar('(');
		        ret.Append(Exp(p));
		        p.PassChar(')');
	        } else if ((id = functions2.Find(sid)) >= 0) {
	            ret.Append(PostFixOperation::Item('g', id));
		        p.PassChar('(');
		        ret.Append(Exp(p));
		        p.PassChar(',');
		        ret.Append(Exp(p));
		        p.PassChar(')');
	        } else
	            throw Exc(Format("Wrong data '%s'", sid));
	    } else if(p.Char('-')) {
	        ret.Append(PostFixOperation::Item('f', functions1.Find("-")));
	        ret.Append(Term(p));
	    } else if(p.Char('(')) {
	        ret = Exp(p);
	        p.PassChar(')');
	    } else
	    	ret.Append(PostFixOperation::Item('n', Null, p.ReadDouble()));
	    return ret;
	}	
	
	PostFixOperation Pow(CParser& p) {
	    PostFixOperation ret;
	    PostFixOperation x = Term(p);
	    for(;;)
	        if(p.Char('^')) {
	            ret.Insert0(PostFixOperation::Item('g', functions2.Find("pow")));
	            ret.Append(pick(x));
	            ret.Append(Term(p));
	        } else {
	            if (ret.IsEmpty())
	            	ret = pick(x);
	            return ret;
	        }
	}
	 
	PostFixOperation Mul(CParser& p) {
		PostFixOperation ret;
	    PostFixOperation x = Pow(p);
	    for(;;)
	        if(p.Char('*')) {
	            ret.Insert0(PostFixOperation::Item('g', functions2.Find("mult")));
	            ret.Append(pick(x));
	            ret.Append(Pow(p));
	        } else if(p.Char('/')) {
	            ret.Insert0(PostFixOperation::Item('g', functions2.Find("div")));
	            ret.Append(pick(x));
	            ret.Append(Pow(p));
	        } else {
	            if (ret.IsEmpty())
	            	ret = pick(x);
	            return ret;
	        }
	}
	 
	PostFixOperation Exp(CParser& p) {
		PostFixOperation ret;
	    PostFixOperation x = Mul(p);
	    for(;;)
	        if(p.Char('+')) {
	            ret.Insert0(PostFixOperation::Item('g', functions2.Find("sum")));
	            ret.Append(pick(x));
	            ret.Append(Mul(p));
	        } else if(p.Char('-')) {
	            ret.Insert0(PostFixOperation::Item('g', functions2.Find("diff")));
	            ret.Append(pick(x));
	            ret.Append(Mul(p));
	        } else {
	            if (ret.IsEmpty())
	            	ret = pick(x);
	            return ret;
	        }
	}
	
	static double minus_fun(double a) 			{return -a;}
	static double sum_fun  (double a, double b) {return a+b;}
	static double diff_fun (double a, double b) {return a-b;}
	static double mult_fun (double a, double b) {return a*b;}
	static double div_fun  (double a, double b) {return a/b;} 

	double Eval0(const PostFixOperation &list, int &id) {
		double val;
		const PostFixOperation::Item &first = list.Get(id++);
		switch (first.type) {
		case 'n':	return first.val;
		case 'v':	if (!WhenGetVariableValue)
						throw Exc("No way to get the value of the variables");
					return WhenGetVariableValue(first.id);
		case 'a':	if (!WhenSetVariableValue)
						throw Exc("No way to set the value of the variables");
					val = Eval0(list, id);
					WhenSetVariableValue(first.id, val);			
					return val;
		case 'f':	return functionsCalls1[first.id](Eval0(list, id));
		case 'g':	return functionsCalls2[first.id](Eval0(list, id), Eval0(list, id));
		}
		throw Exc("Problem parsing equation");	return Null;
	}
};

}

#endif
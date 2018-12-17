#include <iostream>
#include <sstream>
#include <memory>
#include <string>
#include <vector>
#include <set>
#include <map>

using namespace std;

struct Token {
	int value;
	string strvalue;
	enum Type {
		Number,
		Plus,
		Minus,
		Mult,
		Div,
		Mod,
		Less,
		More,
		Equal,
		Nequal,
		Neg,
		And,
		Or,
		LPar,
		RPar,
		LBrace,
		RBrace,
		If,
		Identifier,
		Semicolon,
		Comma
	} type;
	Token(Type t) : value(0),  type(t) {}
	Token(int value) : value(value), type(Type::Number) {}
	Token(string value) : value(0), strvalue(value), type(Type::Identifier) {}
};

//Translation table Token::Type -> string symbol
string opsym[] = {"#", "+", "-", "*", "/", "%", "<", ">", "==", "!=", "!", "&&", "||", "(", ")", "{", "}", "if", "__name__", ";", ","};

bool isStrLow(const string& str) {
	for (auto&& c : str) {
		if (c > 'z' || c < 'a')
			return false;
	}
	return true;
}

vector<Token> tokenize (istream& in) {
	vector<Token> tokens;
	string delim = "\r\n\t +-*/%<>=!~&|(){};,";
	char c;
	string buf;
	
	//Tokenization buffers characters until delim is present
	//then parses both buffer and delimeter into tokens 

	while (in.get(c)) {
		bool isdelim = false;
		for (char d : delim) {
			if (c == d) {
				isdelim = true;

				//Buffer can be either lowalphabet identifier, number or 'if', error otherwise
				if (!buf.empty()) {
					if(buf == "if")
						tokens.emplace_back(Token::If);
					else if (isStrLow(buf))
						tokens.emplace_back(buf);
					else {
						stringstream ss(buf);
						int i;
						ss >> i;
						if (ss.fail()) {
							string k;
							ss >> k;
							throw logic_error("Unknown token type");
						}
						tokens.emplace_back(i);
					}
					buf.clear();
				}

#define tok(expr, res) if (c == expr) tokens.emplace_back(res);
				tok('+', Token::Plus)
				else tok('-', Token::Minus)
				else tok('*', Token::Mult)
				else tok('/', Token::Div)
				else tok('%', Token::Mod)
				else tok('<', Token::Less)
				else tok('>', Token::More)
				else tok('~', Token::Neg)
				else tok('(', Token::LPar)
				else tok(')', Token::RPar)
				else tok('{', Token::LBrace)
				else tok('}', Token::RBrace)
				else tok(',', Token::Comma)
				else tok(';', Token::Semicolon)
#undef tok
				else if (c == '=') {
					in.get(c);
					if (c != '=')
						throw logic_error("invalid occurence =, must be ==");
					tokens.emplace_back(Token::Equal);
				}
				else if (c == '!') {
					in.get(c);
					if (c != '=')
						throw logic_error("invalid occurence !, must be !=");
					tokens.emplace_back(Token::Nequal);
				}
				else if (c == '|') {
					in.get(c);
					if (c != '|')
						throw logic_error("invalid occurence |, must be ||");
					tokens.emplace_back(Token::Or);
				}
				else if (c == '&') {
					in.get(c);
					if (c != '&')
						throw logic_error("invalid occurence &, must be &&");
					tokens.emplace_back(Token::And);
				}
				break;
			}
		}
		if (!isdelim)
			buf += c;
	}
	return tokens;
}				

//Scope is a set of usable variables in current context
//FScope is a map of known functions' names to number of their arguments
using Scope = set<string>;
using FScope = map<string,size_t>;

class Expression {
public:
	virtual string translate(Scope& var, FScope& func) = 0;
	virtual ~Expression() {}
};

class Identifier : public Expression {
public:
	string name;
	Identifier(string name) : name(name) {}
	virtual string translate(Scope& var, FScope& func) {
		if (var.count(name) == 0)
			throw logic_error("variable " + name + " cannot be used in this scope");
		return "_ssharp_" + name;
	}
};


class Source : public Expression {
	unique_ptr<Expression> l, r;
public:
	Source(unique_ptr<Expression>&& l, unique_ptr<Expression>&& r) : l(move(l)), r(move(r)) {}
	virtual string translate(Scope& var, FScope& func) {
		//Need l to be translated first (for FScope func adjustment)
		string left = l->translate(var, func);
		return left + "\n" + r->translate(var, func);
	}
};

class Params : public Expression {
public:
	vector<string> parameters;
	Params() {}
	virtual string translate(Scope& var, FScope& func) {

			string res;
			for (auto& p : parameters) {
				if (func.count(p) > 0)
					throw logic_error("invalid variable name, collision with function " + p);
				res += "u _ssharp_" + p + ",";
			}

			//Removes last comma
			if (!res.empty())
				res.pop_back();

			return res;
	}
	virtual Scope make_scope() {
		Scope scope;
		for (auto p : parameters) 
			if (!scope.insert(p).second)
				throw logic_error("Function has parameters with same name");
		return scope;
	}
	size_t count() {
		return parameters.size();
	}
};

class FuncDef : public Expression {
	string name;
	unique_ptr<Expression> params, prog;

public:
	FuncDef(string name, unique_ptr<Expression>&& params, unique_ptr<Expression>&& prog) : name(name), params(move(params)), prog(move(prog)) {}
	virtual string translate(Scope& var, FScope& func) {
		Params* ptr = dynamic_cast<Params*>(params.get());
		if (name == "if") 
			throw logic_error("Invalid function name (if)");

		if (!func.insert({name,ptr->count()}).second)
			throw logic_error("Function with name " + name + " already exists");

		if (name == "main" && ptr->count() > 0) 
			throw logic_error("Invalid main function - has parameters");

		Scope varscope = ptr->make_scope();
		return (name == "main" ? "int " : "u _ssharp_") + name + "(" + ptr->translate(var, func) + ") {\n\t return " + prog->translate(varscope,func) + ";\n}" ;
	}

	virtual Scope make_scope() {
		return {name};
	}
};

class Arguments : public Expression {
public:
	vector<unique_ptr<Expression>> args;
	Arguments() {}
	virtual string translate(Scope& var, FScope& func) {
			string res;
			for (auto&& p : args) 
				res += p->translate(var,func) + ",";

			//Removes last comma
			if (!res.empty())
				res.pop_back();

			return res;
	}
	size_t count() {
		return args.size();
	}
};

class FuncCall : public Expression {
	string name;
	unique_ptr<Expression> args;
public:
	FuncCall(string name, unique_ptr<Expression>&& args) : name(name), args(move(args)) {}
	virtual string translate(Scope& var, FScope& func) {
		if (func.count(name) == 0)
			throw logic_error("Function " + name + " doesn't exist");

		if (func[name] != (dynamic_cast<Arguments*>(args.get()))->count()) 	
			throw logic_error("Function " + name + " has a different number of arguments");
	
		return "_ssharp_" + name + "(" + args->translate(var, func) +")";
	}
};

class BrProg : public Expression {
	unique_ptr<Expression> prog;
public:
	BrProg(unique_ptr<Expression>&& prog) : prog(move(prog)) {}
	virtual string translate(Scope& var, FScope& func) {
		return "(" + prog->translate(var, func) + ")";
	}
};

class Prog : public Expression {
	unique_ptr<Expression> l, r;
public:
	Prog(unique_ptr<Expression>&& l, unique_ptr<Expression>&& r) : l(move(l)), r(move(r)) {}
	virtual string translate(Scope& var, FScope& func) {
		return l->translate(var, func) + "," + r->translate(var, func);
	}
};

class UnOpExpr : public Expression {
	unique_ptr<Expression> l;
	string op;
public:
	UnOpExpr(unique_ptr<Expression>&& l, string op) : l(move(l)),  op(move(op)) {}
	virtual string translate(Scope& var, FScope& func) {
		return op + "(" + l->translate(var, func) + ")";
	}
};

class BinOpExpr : public Expression {
	unique_ptr<Expression> l, r;
	string op;
public:
	BinOpExpr(unique_ptr<Expression>&& l, unique_ptr<Expression>&& r, string op) : l(move(l)), r(move(r)), op(move(op)) {}
	virtual string translate(Scope& var, FScope& func) {
		return "(" + l->translate(var, func) + op + r->translate(var, func) + ")";
	}
};

class Number : public Expression {
	int value;
public:
	Number(int value) : value(value) {}
	virtual string translate(Scope& var, FScope& func) {
		return to_string(value);
	}
};

class IfStatement : public Expression {
	unique_ptr<Expression> condition, ok, notok;
public:
	IfStatement(unique_ptr<Expression>&& condition, unique_ptr<Expression>&& ok, unique_ptr<Expression>&& notok) : condition(move(condition)), ok(move(ok)), notok(move(notok)) {}
	virtual string translate(Scope& var, FScope& func) {
		return "((" + condition->translate(var,func) + ") ? \n\t" + ok->translate(var,func) + "\n\t : " + notok->translate(var,func) + ")";
	}
};

//Parsing grammar:
//[] means optional, {} means 0 times or more
//Operator precedence same as in C

//SOURCE -> FDEF [SOURCE]
//FDEF -> IDENTIFIER PARAMS BRPROG
//BRPROG -> LBRACE PROG RBRACE
//PROG -> DISJ [; PROG], BRPROG
//DISJ -> CONJ { || CONJ }
//CONJ -> EQ { && EQ }
//EQ -> ORDER { ==,!= ORDER }
//ORDER -> ADD { >,< ADD }
//ADD -> MULT { +,- MULT }
//MULT -> BASIC { *,/,% BASIC }
//BASIC -> ~BASIC, -BASIC, IF, FCALL, NUM, IDENTIFIER, (DISJ), BRPROG

unique_ptr<Expression> parse_add(const vector<Token>& tokens, size_t& p);
unique_ptr<Expression> parse_args(const vector<Token>& tokens, size_t& p);
unique_ptr<Expression> parse_basic(const vector<Token>& tokens, size_t& p);
unique_ptr<Expression> parse_brprog(const vector<Token>& tokens, size_t& p);
unique_ptr<Expression> parse_conj(const vector<Token>& tokens, size_t& p);
unique_ptr<Expression> parse_disj(const vector<Token>& tokens, size_t& p);
unique_ptr<Expression> parse_eq(const vector<Token>& tokens, size_t& p);
unique_ptr<Expression> parse_function_call(const vector<Token>& tokens, size_t& p);
unique_ptr<Expression> parse_function_definition(const vector<Token>& tokens, size_t& p);
unique_ptr<Expression> parse_if(const vector<Token>& tokens, size_t& p);
unique_ptr<Expression> parse_mult(const vector<Token>& tokens, size_t& p);
unique_ptr<Expression> parse_num(const vector<Token>& tokens, size_t& p);
unique_ptr<Expression> parse_order(const vector<Token>& tokens, size_t& p);
unique_ptr<Expression> parse_params(const vector<Token>& tokens, size_t& p);
unique_ptr<Expression> parse_prog(const vector<Token>& tokens, size_t& p);
unique_ptr<Expression> parse_source(const vector<Token>& tokens, size_t& p);
unique_ptr<Expression> parse_variable(const vector<Token>& tokens, size_t& p);


unique_ptr<Expression> parse_args(const vector<Token>& tokens, size_t& p) {
	unique_ptr<Arguments> args = make_unique<Arguments>();
	unique_ptr<Expression> l;
	bool lastcomma = false;

	while (l = parse_disj(tokens, p)) {
		lastcomma = false;
		args->args.push_back(move(l));
		if (tokens[p].type == Token::Comma) {
			p++;
			lastcomma = true;
		}
	}

	if (lastcomma) 
		throw logic_error("Found unexpected comma after the last arguments");

	return args;
}

unique_ptr<Expression> parse_function_call(const vector<Token>& tokens, size_t& p) {
	size_t backup = p;

	if (tokens[p].type != Token::Identifier)
		return unique_ptr<Expression>();

	string name = tokens[p++].strvalue;

	if (tokens[p].type != Token::LPar) {
		p = backup;
		return unique_ptr<Expression>();
	}
	p++;

	unique_ptr<Expression> args = parse_args(tokens, p);
	if (tokens[p].type != Token::RPar) {
		throw logic_error("Missing ) after function call");
	}
	p++;
	return make_unique<FuncCall>(name, move(args));
}

unique_ptr<Expression> parse_if(const vector<Token>& tokens, size_t& p) {
	if (tokens[p].type == Token::If) {
		p++;
		if (tokens[p++].type != Token::LPar) {
			throw logic_error("Missing ( after if statement");
		}

		unique_ptr<Expression> condition = parse_disj(tokens, p);
		if (!condition) 
			condition = parse_brprog(tokens, p);
		
		if (!condition)
			throw logic_error("Missing if condition");
		
		if (tokens[p++].type != Token::RPar) {
			throw logic_error("Missing ) after if condition");
		}

		unique_ptr<Expression> ok = parse_brprog(tokens,p);
		if (!ok)
			throw logic_error("Missing main if branch");
		
		unique_ptr<Expression> notok = parse_brprog(tokens,p);
		if (!notok)
			throw logic_error("Missing else if branch");

		return make_unique<IfStatement>(move(condition), move(ok), move(notok));
	}
	return unique_ptr<Expression>();
}	

unique_ptr<Expression> parse_num(const vector<Token>& tokens, size_t& p) {
	if (tokens[p].type == Token::Number) {
		return make_unique<Number>(tokens[p++].value);
	}
	return unique_ptr<Expression>();
}

unique_ptr<Expression> parse_variable(const vector<Token>& tokens, size_t& p) {
	if (tokens[p].type == Token::Identifier) {
		return make_unique<Identifier>(tokens[p++].strvalue);
	}
	return unique_ptr<Expression>();
}

unique_ptr<Expression> parse_basic(const vector<Token>& tokens, size_t& p) {
	if (tokens[p].type == Token::Neg) {
		string sym = opsym[tokens[p].type];
		p++;
		unique_ptr<Expression> l = parse_basic(tokens, p);

		if (!l) return l;
		return make_unique<UnOpExpr>(move(l), sym);
	}
	if (tokens[p].type == Token::Minus) {
		string sym = opsym[tokens[p].type];
		p++;
		unique_ptr<Expression> l = parse_basic(tokens, p);

		if (!l) return l;
		return make_unique<UnOpExpr>(move(l), sym);
	}


	unique_ptr<Expression> l = parse_if(tokens, p);
	if (l) return l;

	l = parse_function_call(tokens, p);
	if (l) return l;

	l = parse_num(tokens, p);
	if (l) return l;
	
	l = parse_variable(tokens, p);
	if (l) return l;

	//čekovat p < size?
	if (tokens[p].type == Token::LPar) {
		p++;
		l = parse_disj(tokens, p);
		if (!l)
			throw logic_error("Invalid expression after (");
		if (tokens[p++].type != Token::RPar)	
			throw logic_error("Missing )");
		return l;
	}
	
	l = parse_brprog(tokens, p);
	if (l) return l;

	return unique_ptr<Expression>();
}

#define BINOP_PARSE(child, condition) \
	unique_ptr<Expression> l = parse_##child(tokens, p); \
	if (!l) return l;\
	while (condition) {\
		string sym = opsym[tokens[p].type];\
		p++;\
		unique_ptr<Expression> r = parse_##child(tokens, p);\
		if (!r) throw logic_error("Missing second operand");\
		unique_ptr<Expression> tmp = make_unique<BinOpExpr>(move(l), move(r), sym);\
		l = move(tmp);\
	}\
	return l;

unique_ptr<Expression> parse_mult(const vector<Token>& tokens, size_t& p) {
	BINOP_PARSE(basic, tokens[p].type == Token::Mult || tokens[p].type == Token::Div || tokens[p].type == Token::Mod)
}

unique_ptr<Expression> parse_add(const vector<Token>& tokens, size_t& p) {
	BINOP_PARSE(mult, tokens[p].type == Token::Plus || tokens[p].type == Token::Minus)
}

unique_ptr<Expression> parse_order(const vector<Token>& tokens, size_t& p) {
	BINOP_PARSE(add, tokens[p].type == Token::Less || tokens[p].type == Token::More)
}

unique_ptr<Expression> parse_eq(const vector<Token>& tokens, size_t& p) {
	BINOP_PARSE(order, tokens[p].type == Token::Equal || tokens[p].type == Token::Nequal)
}

unique_ptr<Expression> parse_conj(const vector<Token>& tokens, size_t& p) {
	BINOP_PARSE(eq, tokens[p].type == Token::And)
}

unique_ptr<Expression> parse_disj(const vector<Token>& tokens, size_t& p) {
	BINOP_PARSE(conj, tokens[p].type == Token::Or)
}

#undef BINOP_PARSE

unique_ptr<Expression> parse_brprog(const vector<Token>& tokens, size_t& p) {
	size_t backup = p;
	if (tokens[p++].type == Token::LBrace) {	
		unique_ptr<Expression> prog = parse_prog(tokens, p);
		if (!prog)
			throw logic_error("Invalid construction in {...}");

		if (tokens[p++].type == Token::RBrace) {
			return make_unique<BrProg>(move(prog));
		}
		else {
			throw logic_error("Missing }");
		}
	}
	p = backup;
	return unique_ptr<Expression>();
}

unique_ptr<Expression> parse_prog(const vector<Token>& tokens, size_t& p) {
	size_t backup = p;
	unique_ptr<Expression> l = parse_disj(tokens, p);
	if (l) {
		if (tokens[p].type != Token::Semicolon )
			return l;
		p++;
		unique_ptr<Expression> r = parse_prog(tokens, p);
		if (r)
			return make_unique<Prog>(move(l), move(r));
		
		return l;
	}
	p = backup;
	return parse_brprog(tokens, p);
}

unique_ptr<Expression> parse_params(const vector<Token>& tokens, size_t& p) {
	unique_ptr<Params> ps = make_unique<Params>();
	while (tokens[p].type == Token::Identifier){
		(ps->parameters).push_back(tokens[p].strvalue);
		p++;
	}	
	return ps;
}


unique_ptr<Expression> parse_function_definition(const vector<Token>& tokens, size_t& p) {
	if (tokens[p].type != Token::Identifier) {
		return unique_ptr<Expression>();
	}
	string name = tokens[p++].strvalue;
	unique_ptr<Expression> params = parse_params(tokens, p);
	unique_ptr<Expression> prog = parse_brprog(tokens, p);
	if (!prog) {
		throw logic_error("Missing function body");
		return unique_ptr<Expression>();
	}
	return make_unique<FuncDef>(move(name), move(params), move(prog));
}

unique_ptr<Expression> parse_source(const vector<Token>& tokens, size_t& p) {
	if (p >= tokens.size()) 
		throw logic_error("Parse error");
	
	unique_ptr<Expression> l = parse_function_definition(tokens, p);
	if (!l) 
		throw logic_error("Expected function definition");
	if (p >= tokens.size())
		return l;
	else { 
		unique_ptr<Expression> r = parse_source(tokens, p);
		if (!r) return l;
		
		return make_unique<Source>(move(l), move(r));
	}
}

int main() {
	auto t = tokenize(cin);

	//prints tokenization result
	//for (auto&& tok : t)
	//	cerr << tok.type << " " << tok.value << " " << tok.strvalue << endl;

	size_t p = 0;
	Scope var;
	FScope func;
	func["write"] = 1;
	func["read"]  = 0;

	unique_ptr<Expression> ptr = parse_source(t, p);
	if (!ptr || p < t.size())
		throw logic_error("Parse error");
	
	stringstream output;
	output << "#include <stdio.h>\n#include <stdint.h>\n\ntypedef uint64_t u;\n\n";
	output << "u _ssharp_write(u _input) {\n\t printf(\"%lu\\n\", _input);\n}\n";
	output << "u _ssharp_read() {\n\tu _tmp;\n\tscanf(\"%lu\", &_tmp);\n\treturn _tmp;\n}\n";
	output << ptr->translate(var, func) << endl;
	
	if (func.count("main") == 0)
		throw logic_error("main function doesn't exist");
	
	cout << output.str();

	return 0;
}

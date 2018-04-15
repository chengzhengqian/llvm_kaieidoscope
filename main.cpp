#include "parse.h"
#include "token.h"
#include <ctype.h>
#include <iostream>
#include <memory>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <vector>
#include <map>
/* use static variable to hold a parse state*/
static std::string identifierString;
static std::string commentString;
static double numberValue;

static std::string currentInput;
static int currentPos;
static int tokStartPos;
int getcharInString() {
  char a;
  if (currentPos >= currentInput.size()) {
    currentPos=currentInput.size()+1;
    return EOF;
  } else {
    a = currentInput[currentPos];
    currentPos++;
  }
  return (int)a;
}
void markAsStart(){
  tokStartPos=currentPos;
}

int getToken() {
  static int lastChar = ' ';
  while (isspace(lastChar)) {
    lastChar = getcharInString();
  }
  markAsStart();
  if (isalpha(lastChar)) {
    identifierString = lastChar;
    while (isalnum(lastChar = getcharInString())) {
      identifierString += lastChar;
    }
    if (identifierString == "def")
      return tok_def;
    if (identifierString == "extern")
      return tok_extern;
    return tok_identifier;
  } else if (isdigit(lastChar) || lastChar == '.') {
    std::string numberString;
    do {
      numberString += lastChar;
      lastChar = getcharInString();
    } while (isdigit(lastChar) || lastChar == '.');
    numberValue = strtod(numberString.c_str(), 0);
    return tok_number;
  } else if (lastChar == '#') {
    do {
      lastChar = getcharInString();
      if(lastChar!=EOF)
	commentString += lastChar;
    } while (lastChar != EOF && lastChar != '\n' && lastChar != '\r');
    return tok_comment;
  }
  else if(lastChar==EOF){
    lastChar=' ';
    return tok_eof;
  }
  else{
    int thisChar=lastChar;
    lastChar=getcharInString();
    return thisChar;
  }
}

class Expr {
public:
  virtual ~Expr() {}
  virtual std::string toString(){return std::string("Expr");}
};

class Number : public Expr {
  double value;

public:
  Number(double v) : value(v) {}
  virtual std::string toString(){return std::string("Number:")+std::to_string(value);}
};

class Variable : public Expr {
  std::string name;

public:
  Variable(const std::string &name) : name(name) {}
  virtual std::string toString(){return std::string("Variable:")+name;}
};

class BinaryExpr : public Expr {
  std::string op;
  std::unique_ptr<Expr> left, right;

public:
  BinaryExpr(const std::string &op, std::unique_ptr<Expr> left,
             std::unique_ptr<Expr> right)
      : op(op), left(std::move(left)), right(std::move(right)) {}
  
  virtual std::string toString(){
      std::string result=std::string("[");
      result.append(op+": ");
      result.append(left.get()->toString()+", ");
      result.append(right.get()->toString()+"]");
      return result;
  }
};

class CallExpr : public Expr {
  std::string funcName;
  std::vector<std::unique_ptr<Expr>> args;

public:
  CallExpr(const std::string &funcName, std::vector<std::unique_ptr<Expr>> args)
      : funcName(funcName), args(std::move(args)) {}

  virtual std::string toString(){
      std::string result=std::string("[Call:");
      result.append(funcName);
      for(auto const& arg: args){
	result.append("[");
	result.append(arg.get()->toString());
	result.append("]");
      }
      result.append("]");
      return result;
    }
};

class Prototype : public Expr {
  std::string funcName;
  std::vector<std::string> argsName;

public:
  Prototype(const std::string &name, std::vector<std::string> args)
      : funcName(name), argsName(std::move(args)) {}

  virtual std::string toString(){
    std::string result=std::string("[Prototype:");
    result.append(funcName);
    for(auto const& arg: argsName){
      result.append("[");
      result.append(arg);
      result.append("]");
    }
    result.append("]");
    return result;
  }
};

class Function {
  std::unique_ptr<Prototype> proto;
  std::unique_ptr<Expr> body;

public:
  Function(std::unique_ptr<Prototype> proto, std::unique_ptr<Expr> body)
      : proto(std::move(proto)), body(std::move(body)) {}
};

void buildASTExample() {
  auto left = std::make_unique<Variable>("x");
  auto right = std::make_unique<Variable>("y");
  auto sum =
      std::make_unique<BinaryExpr>("+", std::move(left), std::move(right));
}

std::unique_ptr<Expr> error(const char *info) {
  fprintf(stderr, "error %s\n", info);
  return nullptr;
}

std::unique_ptr<Expr> errorPrint(const char *info) {
  error(info);
  return nullptr;
}

std::istream& readLine() {
  currentPos=0;
  return getline(std::cin, currentInput);
  
}
int showNextToken(){
  int a = getToken();
  std::cout<<"["<<tokStartPos-1<<", "<<currentPos-1<<"]";
  if(a==tok_number)
    std::cout<<" number "<<numberValue<<std::endl;
  else  if(a==tok_identifier)
    std::cout<<" variable "<<identifierString<<std::endl;
  else  if(a==tok_comment)
    std::cout<<" comment "<<commentString<<std::endl;
  else  if(a==tok_eof)
    std::cout<<" eof "<<std::endl;
  else{
    std::cout<<" char"<<(char)a<<std::endl;
  }
  return a;
}

int currentToken;
int getNextToken(){
  return currentToken=getToken();
}

std::unique_ptr<Expr> parseExpr();

std::unique_ptr <Expr> parseNumber(){
  auto result=std::make_unique<Number>(numberValue);
  getNextToken();
  return std::move(result);
}

std::unique_ptr<Expr> parseParen(){
  getNextToken();
  auto v=parseExpr();
  if(!v)
    return nullptr;
  if(currentToken!=')')
    return error("expected ')'");
  getNextToken();
  return v;
}

std::unique_ptr<Expr> parseIdentifier(){
  std::string idName=identifierString;
  getNextToken();
  if(currentToken!='(')
    return std::make_unique<Variable>(idName);

  getNextToken();
  std::vector<std::unique_ptr<Expr>> args;
  if(currentToken!=')'){
    while(true){
      if(auto arg=parseExpr())
	args.push_back(std::move(arg));
      else
	return nullptr;
      if(currentToken==')')
	break;
      if(currentToken!=',')
	return error("expected ')' or ',' in args");
      getNextToken();
    }
  }
  getNextToken();
  return std::make_unique<CallExpr>(idName,std::move(args));
}

std::unique_ptr<Expr> parsePrimary(){
  switch(currentToken){
  case tok_identifier:
    return parseIdentifier();
  case tok_number:
    return parseNumber();
  case '(':
    return parseParen();
  default:
    return error("unkown token when expecting an expression");
  }  
}

std::map<char, int> binaryOpPrecedence;

int getTokenPrecedence(){
  if(!isascii(currentToken))
    return -1;
  int tokenPrecedence=binaryOpPrecedence[currentToken];
  if(tokenPrecedence<=0) return -1;
  return tokenPrecedence;
}

void defineBinaryOpPrecedence(){
  binaryOpPrecedence['<']=10;
  binaryOpPrecedence['+']=20;
  binaryOpPrecedence['-']=20;
  binaryOpPrecedence['*']=30;
}

std::unique_ptr<Expr> parseBinaryExpr(int minPrec, std::unique_ptr<Expr> left);

std::unique_ptr<Expr> parseExpr(){
  auto left=parsePrimary();
  if(!left)
    return nullptr;
  return parseBinaryExpr(0,std::move(left));
}

// notice that the first argument indicates the minimal precedence that binary operator need to associate with to get a new expression
std::unique_ptr<Expr> parseBinaryExpr(int minPrec, std::unique_ptr<Expr> left){
  while(1){
    int tokenPrec=getTokenPrecedence();
    if(tokenPrec<minPrec)
      return left;
    int binaryOp=currentToken;
    getNextToken(); // once we want to move to next token, we call it.
    auto right=parsePrimary();
    if(!right) return nullptr;
    int nextPrec= getTokenPrecedence();
    if(tokenPrec<nextPrec){
      right=parseBinaryExpr(nextPrec,std::move(right));
      if(!right)
	return nullptr;
    }
    left=std::make_unique<BinaryExpr>(std::string(1,(char)binaryOp), std::move(left),std::move(right));
  }
  return left;
}




void printTokens(){
  int a;
  do {
    a=showNextToken();
  } while (a != tok_eof);
}

void print(Expr* v){
  if(v)
    std::cout<<v->toString()<<std::endl;
  else{
    std::cout<<"error!"<<std::endl;
  }
}

void printParseExpr(){
  std::cout<<"read: "<<currentInput<<std::endl;
  getNextToken();
  auto v=parseExpr();
  print(v.get());
}

int main() {
  defineBinaryOpPrecedence();
  while (readLine()) {
    printParseExpr();
    //printTokens();
  }
  return 0;
}
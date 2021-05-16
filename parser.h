//----------------------------------------------------------------------
// NAME:Wesley Muehlhausen
// FILE:parser.h
// DATE:Jan 12, 2020
// DESC:Implementation of a parser for MYPL, also used to develope AST
//----------------------------------------------------------------------

#ifndef PARSER_H
#define PARSER_H

#include "token.h"
#include "mypl_exception.h"
#include "ast.h"


class Parser
{
public:

  // create a new recursive descent parser
  Parser(const Lexer& program_lexer);

  // run the parser
  void parse(Program& node);
  
private:
  Lexer lexer;
  Token curr_token;
  
  // helper functions
  void advance();
  void eat(TokenType t, std::string err_msg);
  void error(std::string err_msg);
  bool is_operator(TokenType t);
  bool is_val(TokenType t);
  //bool is_type(TokenType t);
  
  // recursive descent functions
  void tdecl(TypeDecl& node);
  void fdecl(FunDecl& node);
  void vdecls(TypeDecl& node);
  void params(FunDecl& node);
  void dtype();
  void stmts(std::list<Stmt*>& stmt_list);
  void stmt(std::list<Stmt*>& stmt_list);
  void vdecl_stmt(VarDeclStmt& node);
  void assign_stmt(AssignStmt& stmt);
  void lvalue(std::list<Token>& tokens);
  void cond_stmt(std::list<Stmt*>& stmt_list);
  void condt(IfStmt& node);
  void while_stmt(std::list<Stmt*>& stmt_list);
  void for_stmt(std::list<Stmt*>& stmt_list);
  void call_expr(CallExpr& node);
  void args(CallExpr& node);
  void exit_stmt(std::list<Stmt*>& stmt_list);
  void expr(Expr& node);
  void operator_();
  void rvalue(SimpleTerm& node);
  void pval();
  void idrval(IDRValue& node);
};	


// constructor
Parser::Parser(const Lexer& program_lexer) : lexer(program_lexer)
{
}

// Helper functions

//Advances to next token
void Parser::advance()
{
  curr_token = lexer.next_token();
}

//Eats current token and advances or throws an error
void Parser::eat(TokenType t, std::string err_msg)
{
  if (curr_token.type() == t)
    advance();
  else
    error(err_msg);
}

//Error Message
void Parser::error(std::string err_msg)
{
  std::string s = err_msg + "found '" + curr_token.lexeme() + "'";
  int line = curr_token.line();
  int col = curr_token.column();
  throw MyPLException(SYNTAX, s, line, col);
}

//Check to see if token is an operator
bool Parser::is_operator(TokenType t)
{
  return t == PLUS or t == MINUS or t == DIVIDE or t == MULTIPLY or
    t == MODULO or t == AND or t == OR or t == EQUAL or t == LESS or
    t == GREATER or t == LESS_EQUAL or t == GREATER_EQUAL or t == NOT_EQUAL;
}

//checks if token is a data type
bool Parser::is_val(TokenType t)
{
  return t == INT_VAL or t == DOUBLE_VAL or t == BOOL_VAL or t == CHAR_VAL or t == STRING_VAL;
}



// Recursive-decent functions

//Program - checks for function and type declarations
void Parser::parse(Program& node)
{
  //std::cout << "[Parse]->";
  advance();
  while (curr_token.type() != EOS) {
	//Type Declaration  
    if (curr_token.type() == TYPE)
	{
	  TypeDecl* t_decl = new TypeDecl();
      tdecl(*t_decl);
	  node.decls.push_back(t_decl);
	}
	//Function Declaration 
    else
	{
	  FunDecl* f_decl = new FunDecl();
      fdecl(*f_decl);
	  node.decls.push_back(f_decl);
	}
  }
  eat(EOS, " (1) expecting end-of-file ");
  //std::cout << "PROGRAM ENDED SUCCESSFULLY" << std::endl;
}

//Type Declaration
void Parser::tdecl(TypeDecl& node)
{
	//std::cout << "[TDecl]->";
	//eat type
  	eat(TYPE, " (2) Expected token: TYPE ");
	//set id to curr token and eat id
	node.id = curr_token;
	eat(ID, " (3) Expected token: ID ");
  	vdecls(node);//call variable declaration
  	eat(END, " (4) Expected token: END ");	  	
}

//Variable declaration
void Parser::vdecls(TypeDecl& node)
{
	//std::cout << "[VDecls]->";
	if(curr_token.type() == VAR)
	{
		//create a v_decl, pass it through the v_decl_stmt(),
		//then add to node and continue to vdecls()
		VarDeclStmt* v_decls = new VarDeclStmt();
		vdecl_stmt(*v_decls);
		node.vdecls.push_back(v_decls);
		vdecls(node);
	}
}

//Variable declaration statement
void Parser::vdecl_stmt(VarDeclStmt& node)
{
	//std::cout << "[VDeclStmt]->";
	eat(VAR, " (24) Expected token: VAR");
	node.id = curr_token;//set ID
	eat(ID, " (25) Expected token: ID");
	if(curr_token.type() == COLON)//Explicit Declaration
	{
		eat(COLON, " (26) Expected token: COLON");
		node.type = new Token();
		*node.type = curr_token;
		dtype();
	}
	eat(ASSIGN, " (27) Expected token: ASSIGN");
	Expr* exprn = new Expr();
	expr(*exprn);//pass in expression
	node.expr = exprn;
}

//Function declaration
void Parser::fdecl(FunDecl& node)
{
	//std::cout << "[FDecl]->";
	FunDecl* tester = new FunDecl();
  	eat(FUN, " (5) Expected token: FUN");
	node.return_type = curr_token;
  	if(curr_token.type() == NIL)//if NIL, eat it, else check for data types
	{	
		eat(NIL, " (6) Expected token: NIL");
	} 
  	else//else check for other data type
  	{
		dtype();
	}
	node.id = curr_token;//set id
  	eat(ID, " (7) Expected token: ID");  
  	eat(LPAREN, " (8) Expected token: LPAREN");	
  	params(node);//set params
  	eat(RPAREN, " (9) Expected token: RPAREN");
	stmts(node.stmts);//go to statements
  	eat(END, " (10) Expected token: END");
}

//Parameters
void Parser::params(FunDecl& node)
{
	//std::cout << "[Params]->";
	if(curr_token.type() == ID)
	{
		//Create a FunParam, obtain variables and add to node
		FunDecl::FunParam* pms = new FunDecl::FunParam();
		pms->id = curr_token;
		eat(ID, " (11) Expected token: ID");
		eat(COLON, " (12) Expected token: COLON");
		pms->type = curr_token;
		dtype();
		node.params.push_back(*pms);
		while(curr_token.type() == COMMA)//List of Parameters
		{		
			FunDecl::FunParam* pm = new FunDecl::FunParam();
			eat(COMMA, " (13) Expected token: COMMA");
			pm->id = curr_token;
			eat(ID, " (14) Expected token: ID");
			eat(COLON, " (15) Expected token: COLON");
			pm->type = curr_token;
			dtype();
			node.params.push_back(*pm);
		}
	}
}

//Data type - Check for valid data types
void Parser::dtype()
{
	//std::cout << "[DataType]->";
	if(curr_token.type() == INT_TYPE)
		eat(INT_TYPE, " (16) Expected token: INT_TYPE");
	else if(curr_token.type() == DOUBLE_TYPE)
		eat(DOUBLE_TYPE, " (17) Expected token: DOUBLE_TYPE");
	else if(curr_token.type() == BOOL_TYPE)
		eat(BOOL_TYPE, " (18) Expected token: BOOL_TYPE");
	else if(curr_token.type() == CHAR_TYPE)
		eat(CHAR_TYPE, " (19) Expected token: CHAR_TYPE");
	else if(curr_token.type() == STRING_TYPE)
		eat(STRING_TYPE, " (20) Expected token: STRING_TYPE");
	else if(curr_token.type() == ID)
		eat(ID, " (21) Expected token: ID");
	else
		error(" (22) Invalid use of <dtype>");
}

//Statement->S<-
void Parser::stmts(std::list<Stmt*>& stmt_list)
{
	//std::cout << "[Stmts]->";
	while((curr_token.type() == VAR || curr_token.type() == ID) || ((curr_token.type() == IF || curr_token.type() == WHILE) || (curr_token.type() == RETURN || curr_token.type() == FOR)))
	{
		//Pass list to stmt()
		stmt(stmt_list);
	}
}

//Statement
void Parser::stmt(std::list<Stmt*>& stmt_list)
{
	//std::cout << "[Stmt]->";
	if(curr_token.type() == VAR)//Variable Declaration Statement 
	{
		VarDeclStmt* stmt = new VarDeclStmt();
		vdecl_stmt(*stmt);
		stmt_list.push_back(stmt);
	}
	else if(curr_token.type() == IF)//IF Statement
	{
		cond_stmt(stmt_list);
	}
	else if(curr_token.type() == WHILE)//While Loop
	{
		while_stmt(stmt_list);
	}
	else if(curr_token.type() == FOR)//For Loop
	{
		for_stmt(stmt_list);
	}
	else if(curr_token.type() == RETURN)//Return Statement
	{
		exit_stmt(stmt_list);
	}
	else//Other situations: 
	{
		//Initialize Variables
		AssignStmt* astmt = new AssignStmt();
		astmt->lvalue_list.push_back(curr_token);;
		CallExpr* cxpr = new CallExpr();
		cxpr->function_id = curr_token;
		eat(ID, " (23) Expected token: ID");
		//situations: 
		if(curr_token.type() == LPAREN)//Call Expression
		{
			call_expr(*cxpr);
			stmt_list.push_back(cxpr);
		}
		else//Assignment Statement
		{
			assign_stmt(*astmt);
			stmt_list.push_back(astmt);
		}
	}
}

//Assignment statement
void Parser::assign_stmt(AssignStmt& stmt)
{
	//std::cout << "[AssignStmt]->";
	lvalue(stmt.lvalue_list);
	eat(ASSIGN, " (28) Expected token: ASSIGN");
	Expr* ex = new Expr();
	expr(*ex);
	stmt.expr = ex;
}

//Left value
void Parser::lvalue(std::list<Token>& tokens)
{
	//std::cout << "[LValue]->";
	while(curr_token.type() == DOT)
	{
		eat(DOT, " (29) Expected token: DOT");
		tokens.push_back(curr_token);
		eat(ID, " (30) Expected token: ID");
	}
}

//Condition statement
void Parser::cond_stmt(std::list<Stmt*>& stmt_list)
{
	//std::cout << "[CondStmt]->";
	//Create Objects
	IfStmt* stmt = new IfStmt();
	BasicIf* bif = new BasicIf();
	Expr* ex = new Expr();
	std::list<Stmt*> stms;
	eat(IF, " (31) Expected token: IF");
	//If statement condtitions
	expr(*ex);
	bif->expr = ex;
	eat(THEN, " (32) Expected token: THEN");
	//If Statement Contents
	stmts(stms);
	bif->stmts = stms;
	stmt->if_part = bif;
	std::list<BasicIf*> eifs;
	stmt->else_ifs = eifs;
	//Else part
	condt(*stmt);
	stmt_list.push_back(stmt);
	eat(END, " (33) Expected token: END");
}

//Condition
void Parser::condt(IfStmt& node)
{
	//std::cout << "[Condt]->";
	if(curr_token.type() == ELSEIF)//Elseif Part
	{
		//Create Objects
		BasicIf* bscif = new BasicIf();
		Expr* exp = new Expr();
		std::list<Stmt*> stm;
		//Else If part
		eat(ELSEIF, " (34) Expected token: ELSEIF");
		//conditions
		expr(*exp);
		bscif->expr = exp;
		eat(THEN, " (35) Expected token: THEN");
		//statements	
		stmts(stm);
		bscif->stmts = stm;
		node.else_ifs.push_back(bscif);
		condt(node);
	}
	else if(curr_token.type() == ELSE)//Else Part
	{
		eat(ELSE, " (36) Expected token: ELSE");
		std::list<Stmt*> els;
		//contents
		stmts(els);
		node.body_stmts = els;
	}
}

//While statement
void Parser::while_stmt(std::list<Stmt*>& stmt_list)
{
	//std::cout << "[WhileStmt]->";
	WhileStmt* stmt = new WhileStmt();//create statement list
	std::list<Stmt*> stm;
	eat(WHILE, " (37) Expected token: WHILE");
	//Conditions
	Expr* ex = new Expr();
	expr(*ex);
	stmt->expr = ex;
	eat(DO, " (38) Expected token: DO");
	//Statements in the body
	stmts(stm);
	stmt->stmts = stm;
	stmt_list.push_back(stmt);
	eat(END, " (39) Expected token: END");
}

//For statement
void Parser::for_stmt(std::list<Stmt*>& stmt_list)
{
	//std::cout << "[ForStmt]->";
	ForStmt* stmt = new ForStmt();
	std::list<Stmt*> stm;
	eat(FOR, " (40) Expected token: FOR");
	stmt->var_id = curr_token;
	eat(ID, " (41) Expected token: ID");
	eat(ASSIGN, " (42) Expected token: ASSIGN");
	//Conditions
	Expr* ex1 = new Expr();
	expr(*ex1);
	stmt->start = ex1;
	eat(TO, " (43) Expected token: TO");
	Expr* ex2 = new Expr();
	expr(*ex2);
	stmt->end = ex2;
	eat(DO, " (44) Expected token: DO");
	//Body Statements
	stmts(stm);
	stmt->stmts = stm;
	stmt_list.push_back(stmt);
	eat(END, " (45) Expected token: END");
}

//Call expression
void Parser::call_expr(CallExpr& node)
{
	//std::cout << "[CallExpr]->";
	eat(LPAREN, " (46) Expected token: LPAREN");
	//Arguments
	args(node);
	eat(RPAREN, " (47) Expected token: RPAREN");
}

//arguments
void Parser::args(CallExpr& node)
{
	//std::cout << "[ARGS]->";
	if(((is_val(curr_token.type()) || curr_token.type() == NIL) || (curr_token.type() == NEW || curr_token.type() == ID)) || ((curr_token.type() == NEG || curr_token.type() == NOT) || (curr_token.type() == LPAREN)))
	{
		Expr* ex = new Expr();
		expr(*ex);
		node.arg_list.push_back(ex);
		while(curr_token.type() == COMMA)//Function call Arguments
		{
			eat(COMMA, " (48) Expected token: COMMA");
			Expr* ex2 = new Expr();
			expr(*ex2);	
			node.arg_list.push_back(ex2);
		}
	}
}

//Exit statement
void Parser::exit_stmt(std::list<Stmt*>& stmt_list)
{
	//std::cout << "[ExitStmt]->";
	eat(RETURN, " (49) Expected token: RETURN");
	ReturnStmt* rs = new ReturnStmt();
	Expr* exp = new Expr();
	//REturn Expression
	expr(*exp);
	rs->expr = exp;
	stmt_list.push_back(rs);	
}

//Expression
void Parser::expr(Expr& node)
{
	//std::cout << "[Expr]->";
	if(curr_token.type() == NOT)//first term complex term (notted)
	{
		eat(NOT, " (50) Expected token: NOT");
		node.negated = true;
		ComplexTerm* cmpt = new ComplexTerm();
		Expr* exprn = new Expr();
		expr(*exprn);
		cmpt->expr = exprn;
		node.first = cmpt;
		
	}
	else if(curr_token.type() == LPAREN)//first term complex term
	{
		eat(LPAREN, " (51) Expected token: LPAREN");
		ComplexTerm* cmpt = new ComplexTerm();
		Expr* exprn = new Expr();
		expr(*exprn);
		cmpt->expr = exprn;
		node.first = cmpt;	
		eat(RPAREN, " (52) Expected token: RPAREN");
	}
	else//first term simple term
	{
		SimpleTerm* sptm = new SimpleTerm();
		rvalue(*sptm);
		node.first = sptm;
	}
	if(is_operator(curr_token.type()) == true)//possible second term
	{
		node.op = new Token();
		*node.op = curr_token;
		operator_();
		Expr* exprn = new Expr();
		expr(*exprn);
		node.rest = exprn;
	}
}

//Right Value
void Parser::rvalue(SimpleTerm& node)
{
	//std::cout << "[RValue]->";
	if(is_val(curr_token.type()) == true)//Simple Right Value 
	{
		SimpleRValue* val = new SimpleRValue();
		val->value = curr_token;
		pval();
		node.rvalue = val;
	}	
	else if(curr_token.type() == NIL)//Nilled R Value
	{
		SimpleRValue* val = new SimpleRValue();
		val->value = curr_token;
		node.rvalue = val;
		eat(NIL, " (54) Expected token: NIL");
	}
	else if(curr_token.type() == NEW)
	{
		eat(NEW, " (55) Expected token: NEW");//New R Value
		NewRValue* val = new NewRValue();
		val->type_id = curr_token;
		node.rvalue = val;
		eat(ID, " (56) Expected token: ID");
	}
	else if(curr_token.type() == NEG)//Negated R Value
	{
		eat(NEG, " (57) Expected token: NEG");
		NegatedRValue* val = new NegatedRValue();
		Expr* exprn = new Expr();
		expr(*exprn);
		val->expr = exprn;
		node.rvalue = val;
	}
	else if(curr_token.type() == ID)//Other Scenarios
	{
		Token tmp_id = curr_token;
		eat(ID, " (58) Expected token: ID ");
		if(curr_token.type() == LPAREN)//Call Expression
		{
			CallExpr* cexpr = new CallExpr();
			cexpr->function_id = tmp_id;
			call_expr(*cexpr);
			node.rvalue = cexpr;
		}
		else//ID Right Value
		{
			IDRValue* idv = new IDRValue();
			idv->path.push_back(tmp_id);
			idrval(*idv);
			node.rvalue = idv;
		}	
	}
}

//Operator - Checks if Token is a valid operator 
void Parser::operator_()
{
	//std::cout << "[Operator]->";
	if(curr_token.type() == PLUS)
		eat(PLUS, " (53a) Expected token: PLUS");
	else if(curr_token.type() == MINUS)
		eat(MINUS, " (53b) Expected token: MINUS");
	else if(curr_token.type() == DIVIDE)
		eat(  DIVIDE , " (53c) Expected token: DIVIDE");
	else if(curr_token.type() == MULTIPLY)
		eat(  MULTIPLY , " (53d) Expected token: MULTIPLY");
	else if(curr_token.type() == MODULO)
		eat( MODULO  , " (53e) Expected token: MODULO");
	else if(curr_token.type() == AND)
		eat( AND  , " (53f) Expected token: AND");
	else if(curr_token.type() == OR)
		eat(  OR , " (53g) Expected token: OR");
	else if(curr_token.type() == EQUAL)
		eat( EQUAL  , " (53h) Expected token:  EQUAL");
	else if(curr_token.type() == LESS)
		eat( LESS  , " (53i) Expected token: LESS");
	else if(curr_token.type() ==  GREATER)
		eat(  GREATER , " (53j) Expected token: GREATER");
	else if(curr_token.type() == LESS_EQUAL)
		eat( LESS_EQUAL  , " (53k) Expected token: LESS_EQUAL");
	else if(curr_token.type() ==  GREATER_EQUAL)
		eat( GREATER_EQUAL  , " (53l) Expected token: GREATER_EQUAL");
	else if(curr_token.type() == NOT_EQUAL)
		eat(  NOT_EQUAL , " (53m) Expected token: NOT_EQUAL");
	else
		error(" (53n) Expected Operator Token");
}

//Data Value 
void Parser::pval()
{
	//std::cout << "[PValue]->";
	if(curr_token.type() == INT_VAL)
		eat(INT_VAL, " (59) Expected token: INT_VAL");
	else if(curr_token.type() == DOUBLE_VAL)
		eat(DOUBLE_VAL, " (60) Expected token: DOUBLE_VAL");
	else if(curr_token.type() == BOOL_VAL)
		eat(BOOL_VAL, " (61) Expected token: BOOL_VAL");
	else if(curr_token.type() == CHAR_VAL)
		eat(CHAR_VAL, " (62) Expected token: CHAR_VAL");
	else if(curr_token.type() == STRING_VAL)
		eat(STRING_VAL, " (63) Expected token: STRING_VAL");
	else
		error(" (64) Expected Value Token");
}

//Right ID Value
void Parser::idrval(IDRValue& node)
{
	//std::cout << "[IDRValue]->";
	while(curr_token.type() == DOT)
	{
		eat(DOT, " (65) Expected token: DOT");
		node.path.push_back(curr_token);
		eat(ID, " (66) Expected token: ID");
	}
}

#endif

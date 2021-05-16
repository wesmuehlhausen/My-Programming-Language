//----------------------------------------------------------------------
// NAME: Wesley Muehlhausen
// FILE: type_checker.h
// DATE: 3/22/2021
// DESC: Implements a type checker within MYPL which decends through the AST
//       and checks all function, type, and variable type compatability 
//----------------------------------------------------------------------


#ifndef TYPE_CHECKER_H
#define TYPE_CHECKER_H

#include <iostream>
#include <list>
#include "ast.h"
#include "symbol_table.h"


class TypeChecker : public Visitor
{
public:

  // top-level
  void visit(Program& node);
  void visit(FunDecl& node);
  void visit(TypeDecl& node);
  // statements
  void visit(VarDeclStmt& node);
  void visit(AssignStmt& node);
  void visit(ReturnStmt& node);
  void visit(IfStmt& node);
  void visit(WhileStmt& node);
  void visit(ForStmt& node);
  // expressions
  void visit(Expr& node);
  void visit(SimpleTerm& node);
  void visit(ComplexTerm& node);
  // rvalues
  void visit(SimpleRValue& node);
  void visit(NewRValue& node);
  void visit(CallExpr& node);
  void visit(IDRValue& node);
  void visit(NegatedRValue& node);

private:

  // the symbol table 
  SymbolTable sym_table;

  // the previously inferred type
  std::string curr_type;

  // helper to add built in functions
  void initialize_built_in_types();

  // error message
  void error(const std::string& msg, const Token& token);
  void error(const std::string& msg); 

};


void TypeChecker::error(const std::string& msg, const Token& token)
{
  throw MyPLException(SEMANTIC, msg, token.line(), token.column());
}


void TypeChecker::error(const std::string& msg)
{
  throw MyPLException(SEMANTIC, msg);
}


void TypeChecker::initialize_built_in_types()
{
  // print function
  sym_table.add_name("print");
  sym_table.set_vec_info("print", StringVec {"string", "nil"});
  // stoi function
  sym_table.add_name("stoi");
  sym_table.set_vec_info("stoi", StringVec {"string", "int"});  
  // stod function
  sym_table.add_name("stod");
  sym_table.set_vec_info("stod", StringVec {"string", "double"});
  // itos
  sym_table.add_name("itos");
  sym_table.set_vec_info("itos", StringVec {"int", "string"});
  // dtos
  sym_table.add_name("dtos");
  sym_table.set_vec_info("dtos", StringVec {"double", "string"});
  //  get
  sym_table.add_name("get");
  sym_table.set_vec_info("get", StringVec {"int", "string", "char"});
  // length
  sym_table.add_name("length");
  sym_table.set_vec_info("length", StringVec {"string", "int"});
  // read
  sym_table.add_name("read");
  sym_table.set_vec_info("read", StringVec {"string"});
}

void TypeChecker::visit(Program& node)
{
  // push the global environment
  sym_table.push_environment();
  
  // add built-in functions
  initialize_built_in_types();
 
  for (Decl* d : node.decls)
    d->accept(*this);
    
  //check for main function
  if(sym_table.name_exists("main") and sym_table.has_vec_info("main")) 
  {
    StringVec the_type;
    sym_table.get_vec_info("main", the_type);
    std::string return_type = the_type[the_type.size()-1];
    //check for return type and no params
    if(return_type != "int" || the_type.size() != 1)
      error("invalid 'main' function: a valid main function has a return type 'int' and no parameters");
  }
  
  else//needs to have an int main() function 
    error("undefined 'main' function");
    
   // pop the global environment
  sym_table.pop_environment();
}

void TypeChecker::visit(FunDecl& node)
{
  //FUNCTION PARAMETERS AND RETURN TYPE
  StringVec the_type;
  
  //to get parameters
  for(auto vars = node.params.begin(); vars != node.params.end(); ++vars)
  {
  	sym_table.add_name(vars->id.lexeme());
  	sym_table.set_str_info(vars->id.lexeme(), vars->type.lexeme());//add type to var name
    the_type.push_back(vars->type.lexeme());
  }
  
  the_type.push_back(node.return_type.lexeme());//add return type
  sym_table.add_name(node.id.lexeme());//add function name
  sym_table.set_vec_info(node.id.lexeme(), the_type);//add type and params
  
  sym_table.push_environment();//push environment
  
  //FUNCTION BODY
  sym_table.add_name("return");//add new return value for later checking
  sym_table.set_str_info("return", node.return_type.lexeme());
  
  //Continue to statements
  for(Stmt* s : node.stmts)
    s->accept(*this);
  
  sym_table.pop_environment();//pop it back
}
  
//Type Declaration ex. Type Node {t1, t2, etc.}
void TypeChecker::visit(TypeDecl& node)
{ 
  sym_table.add_name(node.id.lexeme()); //set the name of the UDT 
  sym_table.push_environment();//push environment
  StringMap the_type;//init type decl map name
  sym_table.set_map_info(node.id.lexeme(), the_type);//create the empty map right now in case of same type within itself 
  
  for(VarDeclStmt* s : node.vdecls)//traverse ast
  {
    //take care of statements of type declaration
    s->accept(*this);
    the_type[s->id.lexeme()] = curr_type;
  }
  
  sym_table.pop_environment();//pop
  sym_table.set_map_info(node.id.lexeme(), the_type);//add udt to symbol table
}
  
//Variable Declaration Statement ex. var x: int = 14
void TypeChecker::visit(VarDeclStmt& node)
{
	//check for if implicit declaration of UDT, that it has been defined
  if(node.type != nullptr && node.type->is_id() == true)  
    if(sym_table.name_exists(node.type->lexeme()) == false)
      error(node.type->location() + " UDT " + node.type->lexeme() + " does not exist");
  
  node.expr->accept(*this);//traverse to expression of vdcl

  //if explicitly defined and value is nil, still set type
  if(node.type != nullptr && curr_type == "nil")
    curr_type = node.type->lexeme();

  //check if type matches expression for explicit functions
  if(node.type != nullptr)
    if(node.type->lexeme() != curr_type && curr_type != "nil")
      error(node.type->location() + "VarDeclStmt Error: expression (rhs) does not match explicitly defined type");

  //if implicitly defined, cannot have value be nil
  if(node.type == nullptr && curr_type == "nil")
      error("VarDeclStmt Error: Cannot implicitly define a variable to be a nil value");

	//initialize variable info for symbol table
  std::string var_name = node.id.lexeme();
  std::string expr_type = curr_type;

	//check if the variable is already defined in the current environment
  if(sym_table.name_exists_in_curr_env(var_name))
    error(node.id.location() + "1 Redefinition of variable: " + var_name);

  //Add variable to symbol table
  sym_table.add_name(var_name);
  sym_table.set_str_info(var_name, expr_type);//add type to var name
}
  
//Assignment Statement ex. x = (1 + 2)
void TypeChecker::visit(AssignStmt& node)
{
	//init variables
  std::string prev_path_type;//keeps track of type of previous value in a path
  int path_num = 1;//for counting position in path
  
  //Go through lhs variable. Could be a path too
  for(Token t : node.lvalue_list)//iterate through lhs. Potentially a path
  {	
  		if(path_num == 1)//if on the first path item
  		{
  		  if(sym_table.name_exists(t.lexeme()))//if the value is defined
  				sym_table.get_str_info(t.lexeme(), curr_type);//get type of curr id
  			else
  				error(t.location() + "Var/Type " + t.lexeme() + " not found");
  		}
  		else//if there is a path
  		{
  		  if(sym_table.has_map_info(prev_path_type) == false)//if it is the second value
  				error("ID Value does not exist");
  				
				StringMap map;//init map
				sym_table.get_map_info(prev_path_type, map);//get map info of previous type
				
				//name of current path id should be the name of the previous path id's type
				if(map.count(t.lexeme()) > 0)
					curr_type = map[t.lexeme()];
				else//type not found
					error(t.location() + "1 No type");
  		}
  		
  	prev_path_type = curr_type;//update previous type
		++path_num;//update path location 
  }

  //check to see if lhs matches rest of expression
  std::string lhs_type = curr_type;
	Expr* e = node.expr;
  e->accept(*this);
  if(lhs_type != curr_type && curr_type != "nil")//if it doesnt match with rhs, error.
    error(node.lvalue_list.front().location() + "LHS type " + lhs_type + " does not match rhs type" + curr_type);
}

//Return Statement
void TypeChecker::visit(ReturnStmt& node)
{
  Expr* e = node.expr;//traverse ast
  e->accept(*this);
  
  std::string return_type;//init return type

  //Get the return type of the function
  if(sym_table.has_str_info("return"))
    sym_table.get_str_info("return", return_type);

  //if return statement in nil function, error
  //if(return_type == "nil")
  //  error("No return statements needed in nil type function");

  //check for the type
  if(curr_type != return_type && curr_type != "nil")
    error("Function type [" + return_type + "] does not match returned type [" + curr_type + "]");
}

//IF/ IF ELSE Statements
void TypeChecker::visit(IfStmt& node)
{
  //IF PART
  //conditions
  Expr* e = node.if_part->expr;
  e->accept(*this);
  
  if(curr_type != "bool")//check to see if conditions are boolean
  	error("If statement conditions need to be boolean type");//25
  
	//body statements 
  sym_table.push_environment();
  for(Stmt* s : node.if_part->stmts)
  	s->accept(*this);
  sym_table.pop_environment();
  
  //ELSEIF PART
  for(BasicIf* b : node.else_ifs)
  {
  	//head conditions
  	Expr* e = b->expr;
  	e->accept(*this);
  	
  	if(curr_type != "bool")//check to see if conditions are boolean
  		error("If statement conditions need to be boolean type");//26
  		
  	//body statements
    sym_table.push_environment();
  	for(Stmt* s : b->stmts)
			s->accept(*this);
    sym_table.pop_environment();
  }
  //ELSE PART
  if(node.body_stmts.empty() == false)
  {
		//body statements
    sym_table.push_environment();
  	for(Stmt* st : node.body_stmts)
			st->accept(*this);
    sym_table.pop_environment();
  }	
}

//While Statement
void TypeChecker::visit(WhileStmt& node)
{
	//conditions
  Expr* e = node.expr;//go through ast
  e->accept(*this);
  
  if(curr_type != "bool")//check to see if conditions are boolean
  	error("While statement conditions need to be boolean type");//24
  
  //body statements
  sym_table.push_environment();
  for(Stmt* s : node.stmts)
		s->accept(*this);
  sym_table.pop_environment();
}

//For Statement
void TypeChecker::visit(ForStmt& node)
{
  sym_table.push_environment();//push
  
  //for loop conditions
  Expr* e = node.start;
  e->accept(*this);

	//add for loop parameter variable to symbol table
	std::string var_name = node.var_id.lexeme();
	std::string expr_type = curr_type;
	sym_table.add_name(var_name);
	sym_table.set_str_info(var_name, expr_type);//add type to var name

  if(curr_type != "int")//check to see if conditions are boolean
  	error(node.var_id.location() + "For statement conditions need to be int type");//27
  	
  //body statements	
  Expr* n = node.end;
  n->accept(*this);
  
  if(curr_type != "int")//check to see if conditions are boolean
  	error(node.var_id.location() + "For statement conditions need to be int type");//27
  
	//body statements
	sym_table.push_environment();
  for(Stmt* st : node.stmts)
    st->accept(*this);
  sym_table.pop_environment();//pop body
  sym_table.pop_environment();//pop loop parameter
}


// expressions
void TypeChecker::visit(Expr& node)
{
  //Left hand side of expression
  ExprTerm* e = node.first;
  e->accept(*this);

  std::string lhs_type = curr_type; //keep copy of the current type

  if(node.rest)//Right part of the expression
  {
  	Expr* ex = node.rest;
  	ex->accept(*this);
  }

	//Very large check of all type rules
  if(node.op != nullptr)
  {
    //Math operators
    if((node.op->lexeme() == "+" || node.op->lexeme() == "-") || (node.op->lexeme() == "*" || node.op->lexeme() == "/"))//for math operators
    {
      if(node.op->lexeme() == "+" && (lhs_type == "char" || lhs_type == "string"))//char/string concatination
      {
      	if(curr_type == "char" || curr_type == "string")
      		curr_type = "string";
      	else
      		error("Concatination has to be between strings and chars");
      }
      else if((lhs_type == "int" && curr_type == "int"))//if int on lhs, then int on rhs is needed
        curr_type = "int";
      else if((lhs_type == "double" && curr_type == "double"))//if double on lhs, then double on rhs is needed 
        curr_type = "double";
      else
        	error("Expressions with +,-,*,/ need to be (int [op] int) or (double [op] double)");
    }
    
    //Comparison operators
    else if((node.op->lexeme() == "<" || node.op->lexeme() == ">") 
    || (node.op->lexeme() == ">=" || node.op->lexeme() == "<="))
    {
      if((lhs_type == "int" && curr_type == "int") || (lhs_type == "double" && curr_type == "double") //check for matching types in comparison
      || (lhs_type == "char" && curr_type == "char") || (lhs_type == "string" && curr_type == "string"))
        curr_type = "bool";
      else
        error("Cannot use >,>=,<,<= operators in expressions without matching double/int/char/string vals" + node.op->location());
    }
    
    //MOD operator
    else if(node.op->lexeme() == "%")//mod operations 
    {
      if((lhs_type == "int" && curr_type == "int"))//if int on lhs, then int on rhs is needed
        curr_type = "int";
      else 
        error("Use of MOD % needs to be int on lhs and rhs");
    }
    
    //Boolean && || operations
    else if(node.op->lexeme() == "or" || node.op->lexeme() == "and")
    {
      if(lhs_type != "bool" || curr_type != "bool")
      	error("'or' and 'and' operators can only be used with boolean expressions");//21,22
    }
    
    //Equivalence operations
    else if(node.op->lexeme() == "==" || node.op->lexeme() == "!=")//17, 18, 19
    {
      if(lhs_type != curr_type)//check if the type types match or if one of them is nil
      {
      	if(lhs_type != "nil" && curr_type != "nil")
      		error("'' and '' comparisons need to be between two matching types or nils");
      }
      curr_type = "bool"; 	
    }

  }

	//Extra check for negated boolean value. If it is type boolean
	if(node.negated == true && curr_type != "bool")
  {
    	error("Cannot negate (NOT) non-bool expressions");//23
  }    
}

//Simple Term: simple value, call expr, new value, idr val, negated value
void TypeChecker::visit(SimpleTerm& node)
{
  RValue* r = node.rvalue;//continue with simple term
  r->accept(*this);
}

//Complex Term: another expression
void TypeChecker::visit(ComplexTerm& node)
{
  Expr* e = node.expr;//continue with complex term
  e->accept(*this);
}

// Simple Value ex. "5"
void TypeChecker::visit(SimpleRValue& node)
{
  curr_type = node.value.get_type();
}

//New RHS Value ex. var x = "new Node"
void TypeChecker::visit(NewRValue& node)
{
  curr_type = node.type_id.lexeme();//set current type

  //check to see if user defined type exists
  if(sym_table.has_map_info(curr_type) == false)
    error(node.type_id.location() + "User defined type doesn't exist");
}

//Function Call Expression. Can be LHS or RHS
void TypeChecker::visit(CallExpr& node)
{
  //Get the info from the ast
  std::string fun_name = node.function_id.lexeme();
  StringVec fun_type;
  sym_table.get_vec_info(fun_name, fun_type);
  
  //check for function name existance
  if(sym_table.name_exists(fun_name) == false)//check for function
  	error("Function " + fun_name + " does not exist");
  
  //check for correct number of parameters
  if(fun_type.size()-1 != node.arg_list.size())//check for param sizes
  	error("Incorrect number of function params");
  
  //Check for matching parameter types
  int i = 0;//compare params
  for(Expr* x : node.arg_list)
  {
  	x->accept(*this);
  	if(fun_type[i] != curr_type && curr_type != "nil")
  		error(node.function_id.location() + "Mismatched function call");//15
  	++i;
  }
  
  curr_type = fun_type[fun_type.size()-1];//set curr type to return type
}

//RHS ID value 
void TypeChecker::visit(IDRValue& node)
{
	//set up values for a path
  std::string prev_path_type;
  int path_num = 1;
  
  //Go through path
  for(Token t : node.path)//iterate through rhs. Potentially a path
  {	
  		if(path_num == 1)//if on the first path item
  		{
  		  if(sym_table.name_exists(t.lexeme()))//if the value is defined
  				sym_table.get_str_info(t.lexeme(), curr_type);
  			else
  				error(t.location() + "Var/Type " + t.lexeme() + " not found");
  		}
  		else//if there is a path
  		{
  		  if(sym_table.has_map_info(prev_path_type) == false)//if it is the second value
  				error("ID Value does not exist");
  			
  			//create new map value to get type of previous path id to check if matching	
				StringMap map;
				sym_table.get_map_info(prev_path_type, map);
				
				//Check to see if current path value exists
				if(map.count(t.lexeme()) > 0)//if exists
					curr_type = map[t.lexeme()];
				else
					error(t.location() + "1 No type");
  		}
  		
  	//update type and path position
  	prev_path_type = curr_type;
		++path_num;
  }
}

//Negated RHS Value (negative)
void TypeChecker::visit(NegatedRValue& node)
{
	//get expression
  Expr* e = node.expr;
  e->accept(*this);

  //check to see if the type is an int or double
  if(curr_type != "int" && curr_type != "double")
    error("Cannot negate non int/double values");
}

#endif

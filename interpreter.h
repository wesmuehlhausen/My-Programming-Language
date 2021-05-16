//----------------------------------------------------------------------
// NAME: Wesley Muehlhausen  
// FILE: interpreter.h
// DATE: 04/06/2021
// DESC: Ties the whole project together. This implements
//       data objects and a heap to keep track of user
//       defined types. Also had built in deugger
//----------------------------------------------------------------------


#ifndef INTERPRETER_H
#define INTERPRETER_H

#include <iostream>
#include <algorithm>
#include <unordered_map>
#include <regex>
#include "ast.h"
#include "symbol_table.h"
#include "data_object.h"
#include "heap.h"


class Interpreter : public Visitor
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

// return code from calling main
int return_code() const;


private:

// return exception
class MyPLReturnException : public std::exception {};

// the symbol table 
SymbolTable sym_table;

// holds the previously computed value
DataObject curr_val;

// the heap
Heap heap;

// debugging activation
bool debug;//global control over debugger
bool step_rng;//keep on printing as long as this is true. 
size_t curr_step = 1;
bool step_to_end = false;
std::vector<int> breaks;

// the next oid
size_t next_oid = 0;

// the functions (all within the global environment)
std::unordered_map<std::string,FunDecl*> functions;

// the user-defined types (all within the global environment)
std::unordered_map<std::string,TypeDecl*> types;

// the global environment id
int global_env_id = 0;

// the program return code
int ret_code = 0;

// error message
void error(const std::string& msg, const Token& token);
void error(const std::string& msg); 

// debugger helpers
void init_debugger();
bool step_debugger();
void help_debugger();
};



int Interpreter::return_code() const
{
	return ret_code;
}

void Interpreter::error(const std::string& msg, const Token& token)
{
	throw MyPLException(RUNTIME, msg, token.line(), token.column());
}


void Interpreter::error(const std::string& msg)
{
	throw MyPLException(RUNTIME, msg);
}


// top-level
void Interpreter::visit(Program& node)
{

	//Push Global Environment
	sym_table.push_environment();

	//Store the global environment id
	global_env_id = sym_table.get_environment_id();

	//start debugger
	init_debugger();	

	//Add functions and UDTs
	for(Decl* d : node.decls)
		d->accept(*this);

	//execute the main function 
	CallExpr expr;
	expr.function_id = functions["main"]->id;
	expr.accept(*this);

	//pop the global environment
	sym_table.pop_environment();
	std::cout << "" << std::endl;//for aesthetic purposes
}

//Function declaration
void Interpreter::visit(FunDecl& node)
{
	FunDecl* f = new FunDecl(); //just create the new function decl to get function id
	*f = node;
	functions.insert({node.id.lexeme(), f});
}

//UDT Declaration
void Interpreter::visit(TypeDecl& node)
{
	//create a new Type decl for the name
	TypeDecl* t = new TypeDecl();
	*t = node;
	types.insert({node.id.lexeme(), t});
}

// statements
void Interpreter::visit(VarDeclStmt& node)
{
	node.expr->accept(*this);//traverse to expression of vdcl
	std::string var_name = node.id.lexeme();
	sym_table.add_name(var_name);//add name
	sym_table.set_val_info(var_name, curr_val);//add type to var name
	
	//NOTE step check debugging
	if(step_debugger())
	{
		std::cout << "  |#" << curr_step << "| [Variable->" + var_name + 
		"][Type->" + curr_val.to_string_type()+ 
		"][Value->" + curr_val.to_string() + "]" << std::endl;
		++curr_step;
	}
}

//Assignment of variable
void Interpreter::visit(AssignStmt& node)
{
	std::string root_id;
	//Go through path
	int path_num = 1;
	DataObject tmp_dat;
	HeapObject tmp_obj;
	size_t tmp_oid;
	size_t root_oid;
	
	//NOTE get the path as a string for later
	std::string path_id = "";
	int count = 0;
	for(Token t : node.lvalue_list)
	{
		path_id += t.lexeme();
		if(count < node.lvalue_list.size()-1)
			path_id += ".";
		++count;
	}
	
	
	for(Token t : node.lvalue_list)//iterate through lhs. Potentially a path
	{	
		if(node.lvalue_list.size() == 1)//for one variable paths
		{
			root_id = t.lexeme();
			//rhs
			Expr* e = node.expr;
			e->accept(*this);
			//set the current value to 
			sym_table.set_val_info(root_id, curr_val);
				
			//NOTE step check debugging
			if(step_debugger())
			{
				std::cout << "  |#" << curr_step << "| [Variable->" + root_id + 
				"][Type->" + curr_val.to_string_type()+ 
				"][Value->" + curr_val.to_string() + "]" << std::endl;
				++curr_step;
			}
			
		}
		else//for multivariable paths
		{
			
			if(path_num == 1)//For first value
			{
				sym_table.get_val_info(t.lexeme(), tmp_dat);//get the value with the current id name and put it in tmp data object (this should hold an oid)
				tmp_dat.value(tmp_oid);//put the oid from tmp_dat into tmp_oid
				heap.get_obj(tmp_oid, tmp_obj);//get the heap object that has the current id of tmp_oid
				root_oid = tmp_oid;
				
				//NOTE setup lhs path print
				step_rng = step_debugger();
				if(step_rng)
				{
					std::cout << "  |#" << curr_step << "| [LHS Path Var->" + path_id << "]" << std::endl;
					std::cout << "|   [ID->" + t.lexeme() + 
					             "][Type->" + tmp_dat.to_string_type() + 
					             "][Value->" + tmp_dat.to_string() + "]" << std::endl; 
					++curr_step;							 
				}
			}	
			else//For general cases
			{
				tmp_obj.get_val(t.lexeme(), tmp_dat);//set value to attribute or last oid
				if(path_num != node.lvalue_list.size())//if we aren't on the final element
				{
					//set the current oid value to one stored in curr_val
					tmp_dat.value(tmp_oid);
					//tmp_dat.value(tmp_oid);//get the oid
					heap.get_obj(tmp_oid, tmp_obj);//get heap object i.e. udt
					
					//NOTE for the normal 
					if(step_rng)
					{
					std::cout << "|   [ID->" + t.lexeme() + 
					             "][Type->" + tmp_dat.to_string_type() + 
					             "][Value->" + tmp_dat.to_string() + "]" << std::endl; 
												 
					}
				}
				else//if we are on the final element
				{
					Expr* e = node.expr;
					e->accept(*this);
					//set the current value to 
					tmp_dat = curr_val;
					tmp_obj.set_att(t.lexeme(), tmp_dat);
					heap.set_obj(root_oid, tmp_obj);		
					
					//NOTE for the normal 
					if(step_rng)
					{
					std::cout << "|   [ID->" + t.lexeme() + 
					             "][Type->" + curr_val.to_string_type() + 
					             "][Value->" + curr_val.to_string() + "]" << std::endl; 
					std::cout << "| [LHS Path Val->" + curr_val.to_string() << "]" << std::endl;						 
					}
				}
			}			
		}
		++path_num;
	}
	//NOTE set step check range to false
	step_rng = false;
}

//Return Statement
void Interpreter::visit(ReturnStmt& node)
{
	//evaluate expression
	node.expr->accept(*this);
	
	//NOTE step check debugging
	if(step_debugger())
	{
		std::cout << "  |#" << curr_step << "| [Return Value->" + curr_val.to_string() + 
		"][Type->" + curr_val.to_string_type()+ "]" << std::endl;
		++curr_step;
	}
	
	//throw the return exception
	throw new MyPLReturnException;
}

//If Statements
void Interpreter::visit(IfStmt& node)
{
	//IF PART
	
	//NOTE check for debugger 
	step_rng = step_debugger();

	
	//conditions
	Expr* e = node.if_part->expr;
	e->accept(*this);
	bool if_param_val;
	curr_val.value(if_param_val);//get a copy of the current value
	
	//NOTE print the if expr value
	if(step_rng)
	{
		if(if_param_val)
			std::cout << "  |#" << curr_step << "| [IF EXPR Value->true]" << std::endl; 
		else
			std::cout << "  |#" << curr_step << "| [IF EXPR Value->false]" << std::endl; 			
		++curr_step;						
	}
	

	if(if_param_val == true)//if the expression is true, loop
	{
		//body statements 
		sym_table.push_environment();
		for(Stmt* s : node.if_part->stmts)
		s->accept(*this);
		sym_table.pop_environment();		
	}
	else//if the if statement didn't catch 
	{
		bool elseif_val = false;
		//ELSEIF PART
		for(BasicIf* b : node.else_ifs)
		{
			if(elseif_val != true)//if we haven't gone into else if
			{
				//head conditions
				Expr* e = b->expr;
				e->accept(*this);
				curr_val.value(elseif_val);//get a copy of the current value
				
					//NOTE print the if parameters
					if(step_rng)
					{
						if(elseif_val)
							std::cout << "  |#" << curr_step << "| [ELSE-IF EXPR Value->true]" << std::endl; 
						else
							std::cout << "  |#" << curr_step << "| [ELSE-IF EXPR Value->false]" << std::endl; 
						++curr_step;									
					}
					

				if(elseif_val == true)//if the exp is true, enter if	
				{
					//body statements
					sym_table.push_environment();
					for(Stmt* s : b->stmts)
						s->accept(*this);
					sym_table.pop_environment();					
				}
			}
		}
		if(elseif_val == false)//if did not enter any if statements
		{
			//ELSE PART
			if(node.body_stmts.empty() == false)
			{
					//NOTE print the else step
					if(step_rng)
					{
						std::cout << "  |#" << curr_step << "| [ELSE->]" << std::endl; 
						++curr_step;									
					}
			
				//body statements
				sym_table.push_environment();
				for(Stmt* st : node.body_stmts)
					st->accept(*this);
				sym_table.pop_environment();
			}
		}
	}
}

//While Statement
void Interpreter::visit(WhileStmt& node)
{

	//conditions
	bool condt = true;
	
	//NOTE print the if expr value
	if(step_debugger())
	{
			std::cout << "  |#" << curr_step << "| [While->]" << std::endl; 	
			++curr_step;								
	}
	
	while(condt == true)
	{
		Expr* e = node.expr;//go through ast
		e->accept(*this);
		curr_val.value(condt);
		if(condt == true)
		{
			//body statements
			sym_table.push_environment();
			for(Stmt* s : node.stmts)
				s->accept(*this);
			sym_table.pop_environment();
		}
		else //exit case
			condt = false;
	}
}
void Interpreter::visit(ForStmt& node)
{

	sym_table.push_environment();//push

	//for loop conditions
	Expr* e = node.start;
	e->accept(*this);

	//add for loop parameter variable to symbol table
	std::string index_name = node.var_id.lexeme();
	DataObject index_val = curr_val;
	sym_table.add_name(index_name);
	sym_table.set_val_info(index_name, index_val);//add type to var name
	int start_i;
	index_val.value(start_i);//set index val

	//end loop condition
	Expr* n = node.end;
	n->accept(*this);
	int end_i;
	curr_val.value(end_i);//set end loop condition

  //NOTE print the if expr value
	if(step_debugger())
	{
			std::cout << "  |#" << curr_step << "| [For Start->" << start_i << "[End->" << end_i << "]" << std::endl; 
			++curr_step;									
	}

	//body statements
	for(int i = start_i; i <= end_i; ++i)
	{
		index_val.set(i);
		sym_table.set_val_info(index_name, index_val);//add type to var name
		sym_table.push_environment();			
		for(Stmt* st : node.stmts)
			st->accept(*this);
		sym_table.pop_environment();//pop body			
	}
	sym_table.pop_environment();//pop loop parameter
}

//expressions
void Interpreter::visit(Expr& node)
{
	//for negated values
	if(node.negated == true)
	{
		node.first->accept(*this);
		bool val;
		curr_val.value(val);//copy curr val into val
		curr_val.set(!val);//set curr val as not val
	}
	else//for all other cases
	{
		node.first->accept(*this);//get first value
		if(node.op != nullptr)
		{
			//get a copy of the lhs value and get the rhs value
			DataObject lhs_val = curr_val;
			node.rest->accept(*this);
			DataObject rhs_val = curr_val;
			TokenType op = node.op->type();

			//Cases:
			if(op == PLUS)// x + x
			{
				//Integer addition
				if(lhs_val.is_integer() && rhs_val.is_integer())// 1 + 2
				{
					int l, r;//init variables
					lhs_val.value(l);//get l's value
					rhs_val.value(r);//get r's value
					curr_val.set(l+r);
				}
				//double addition
				else if(lhs_val.is_double() && rhs_val.is_double())// 1.23 + 23.2
				{
					double l, r;//init variables
					lhs_val.value(l);//get l's value
					rhs_val.value(r);//get r's value
					curr_val.set(l+r);
				}
				//Char/string concatination
				else//x + y where either is 
				{

					if(lhs_val.is_char() && rhs_val.is_char()) // C + C
					{
						char l, r;
						lhs_val.value(l);//get l's value
						rhs_val.value(r);//get r's value
						std::string out = std::string() + l + r;
						curr_val.set(out);
					}
					else if(lhs_val.is_char() && rhs_val.is_string()) // C + S
					{
						char l;
						std::string r;
						lhs_val.value(l);//get l's value
						rhs_val.value(r);//get r's value
						std::string out(1, l);
						out += r;
						curr_val.set(out);
					}
					else if(lhs_val.is_string() && rhs_val.is_char()) // S + C
					{
						char r;
						std::string l;
						lhs_val.value(l);//get l's value
						rhs_val.value(r);//get r's value
						std::string out(1, r);
						l += out;
						curr_val.set(l);
					}
					else if(lhs_val.is_string() && rhs_val.is_string())// S + S
					{
						std::string l, r;
						lhs_val.value(l);
						rhs_val.value(r);
						l += r;
						curr_val.set(l);
					}
					else
					{
						error("unable to add expressions provided");
					}
				}
			}
			//More math operators
			else if(op == MINUS || (op == MULTIPLY || op == DIVIDE))
			{
				if(lhs_val.is_integer() && rhs_val.is_integer())
				{
					int l, r;//init variables
					lhs_val.value(l);//get l's value
					rhs_val.value(r);//get r's value
					if(op == MINUS)
						curr_val.set(l-r);
					else if(op == MULTIPLY)
						curr_val.set(l*r);
					else
						curr_val.set(l/r);
				}
				else if(lhs_val.is_double() && rhs_val.is_double())
				{
				double l, r;//init variables
				lhs_val.value(l);//get l's value
				rhs_val.value(r);//get r's value
				if(op == MINUS)
					curr_val.set(l-r);
				else if(op == MULTIPLY)
					curr_val.set(l*r);
				else
					curr_val.set(l/r);
				}
				else
					error("Simple Arithmetic Error");
			}
			//Comparison Operators
			else if(op == GREATER || op == GREATER_EQUAL || op == LESS || op == LESS_EQUAL)
			{
				if(lhs_val.is_integer() && rhs_val.is_integer())
				{
					int l, r;//init variables
					lhs_val.value(l);//get l's value
					rhs_val.value(r);//get r's value
					if(op == GREATER)
					{
						curr_val.set(l>r);
					}
					else if(op == GREATER_EQUAL)
						curr_val.set(l>=r);
					else if(op == LESS)
						curr_val.set(l<r);
					else
						curr_val.set(l<=r);
				}
				else if(lhs_val.is_double() && rhs_val.is_double())//X1 >=,<=,<,> X2 Double
				{
					double l, r;//init variables
					lhs_val.value(l);//get l's value
					rhs_val.value(r);//get r's value
					if(op == GREATER)
						curr_val.set(l>r);
					else if(op == GREATER_EQUAL)
						curr_val.set(l>=r);
					else if(op == LESS)
						curr_val.set(l<r);
					else
						curr_val.set(l<=r);
				}
				else if(lhs_val.is_char() && rhs_val.is_char())//X1 >=,<=,<,> X2 Char
				{
					char l, r;//init variables
					lhs_val.value(l);//get l's value
					rhs_val.value(r);//get r's value
					if(op == GREATER)
						curr_val.set(l>r);
					else if(op == GREATER_EQUAL)
						curr_val.set(l>=r);
					else if(op == LESS)
						curr_val.set(l<r);
					else
						curr_val.set(l<=r);
				}
				else if(lhs_val.is_string() && rhs_val.is_string())//String1 >=,<=,<,> String2
				{
					std::string l, r;//init variables
					lhs_val.value(l);//get l's value
					rhs_val.value(r);//get r's value	
					if(op == GREATER)
						curr_val.set(l>r);
					else if(op == GREATER_EQUAL)
						curr_val.set(l>=r);
					else if(op == LESS)
						curr_val.set(l<r);
					else
						curr_val.set(l<=r);				  
				}
				else if(lhs_val.is_bool() && rhs_val.is_bool()) //Bool1 >=,<=,<,> Bool2
				{
					bool l, r;//init variables
					lhs_val.value(l);//get l's value
					rhs_val.value(r);//get r's value
					if(op == GREATER)
						curr_val.set(l>r);
					else if(op == GREATER_EQUAL)
						curr_val.set(l>=r);
					else if(op == LESS)
						curr_val.set(l<r);
					else
						curr_val.set(l<=r);
				}
				else
					error("Unable to compute comparison operation");
			}
			//MOD operator
			else if(op == MODULO)
			{
				if(lhs_val.is_integer() && rhs_val.is_integer())//if both are integers then good
				{
					int l, r;//init variables
					lhs_val.value(l);//get l's value
					rhs_val.value(r);//get r's value
					curr_val.set(l%r);
				}
				else
					error("mod operator error");
			}
				//Equivalence operators
			else if(op == EQUAL || op == NOT_EQUAL)
			{
				if(lhs_val.is_integer() && rhs_val.is_integer())
				{
					int l, r;//init variables
					lhs_val.value(l);//get l's value
					rhs_val.value(r);//get r's value
					if(op == EQUAL)
						curr_val.set(l==r);
					else
						curr_val.set(l!=r);
				}
				else if(lhs_val.is_double() && rhs_val.is_double())
				{
					double l, r;//init variables
					lhs_val.value(l);//get l's value
					rhs_val.value(r);//get r's value
					if(op == EQUAL)
						curr_val.set(l==r);
					else
						curr_val.set(l!=r);
				}
				else if(lhs_val.is_bool() && rhs_val.is_bool())
				{
					bool l, r;//init variables
					lhs_val.value(l);//get l's value
					rhs_val.value(r);//get r's value
					if(op == EQUAL)
						curr_val.set(l==r);
					else
						curr_val.set(l!=r);
				}
				else if(lhs_val.is_char() && rhs_val.is_char())
				{
					char l, r;//init variables
					lhs_val.value(l);//get l's value
					rhs_val.value(r);//get r's value
					if(op == EQUAL)
						curr_val.set(l==r);
					else
						curr_val.set(l!=r);
				}
				else if(lhs_val.is_string() == rhs_val.is_string())
				{
					std::string l, r;//init variables
					lhs_val.value(l);//get l's value
					rhs_val.value(r);//get r's value
					if(op == EQUAL)
						curr_val.set(l==r);
					else
						curr_val.set(l!=r);
				}
				else//else, throw error
				{
					error(node.op->location() + "Expression Equivalence operator error of L: " + lhs_val.to_string() + " R; "+ rhs_val.to_string());
				}
			}
			else if(op == AND || op == OR)
			{
				if(lhs_val.is_bool() == rhs_val.is_bool())
				{
					bool l, r;//init variables
					lhs_val.value(l);//get l's value
					rhs_val.value(r);//get r's value
					if(op == AND)
						curr_val.set(l && r);
					else
						curr_val.set(l || r);
				}
				else
					error("AND/OR comparosion operator error");
			}
			else
				error("Operator Error in expression");
		}
	}
}

//Simple Term
void Interpreter::visit(SimpleTerm& node)
{
	RValue* r = node.rvalue;//continue with simple term
	r->accept(*this);
}

//Complex Term
void Interpreter::visit(ComplexTerm& node)
{
	Expr* e = node.expr;//continue with complex term
	e->accept(*this);
}
//Simple RHS values
void Interpreter::visit(SimpleRValue& node)
{
	//set char value
	if(node.value.type() == CHAR_VAL)
		curr_val.set(node.value.lexeme().at(0));
	//set string value
	else if(node.value.type() == STRING_VAL)
	{
		if(node.value.lexeme() == "\n")//check for newline
		{
			curr_val.set("");
			std::cout << "" << std::endl;
		}
		else
		{
			curr_val.set(node.value.lexeme());
		}
	}
	//int value
	else if(node.value.type() == INT_VAL)
	{
		try 
		{
			curr_val.set(std::stoi(node.value.lexeme()));
		}
		catch(const std::invalid_argument& e) 
		{
			error ("internal error", node.value);
		}
		catch(const std::out_of_range& e) 
		{
			error ("int out of range", node.value);
		}
	}
	//Double value
	else if(node.value.type() == DOUBLE_VAL)
	{
		try 
		{
			curr_val.set(std::stod(node.value.lexeme()));
		}
		catch(const std::invalid_argument& e) 
		{
			error ("internal error", node.value);
		}
		catch(const std::out_of_range& e) 
		{
			error ("double out of range", node.value);
		}
	}
	else if(node.value.type() == BOOL_VAL)
	{
		if(node.value.lexeme() == "true")
			curr_val.set(true);
		else
			curr_val.set(false);
	}  	
	else if(node.value.type() == NIL)
		curr_val.set_nil();
	else
		error("Simple R Value invalid value");
}

//New R Value  ... = new Node
void Interpreter::visit(NewRValue& node)
{
	//set title and get decl
	size_t tmp_oid = next_oid;
	++next_oid;//get next oid
	std::string type_name = node.type_id.lexeme();//type name
	TypeDecl* type_node = types[type_name];//get typedecl for type
	HeapObject type;//create new heap object

	sym_table.push_environment();//push environment

	for(VarDeclStmt* s : type_node->vdecls)//traverse ast
	{
		//take care of statements of type declaration
		s->accept(*this);
		type.set_att(s->id.lexeme(), curr_val);
	}

	sym_table.pop_environment();//pop
	heap.set_obj(tmp_oid, type);
	curr_val.set(tmp_oid);//set the value of the current val to the current oid
}

void Interpreter::visit(CallExpr& node)
{

	std::string fun_name = node.function_id.lexeme();

	//Built in functions
	if(fun_name == "print")//print(string)
	{
		node.arg_list.front()->accept(*this);
		std::string s = curr_val.to_string();
		s = std::regex_replace(s, std::regex("\\\\n"), "\n");
		s = std::regex_replace(s, std::regex("\\\\t"), "\t");
		std::cout << s;
		
		//NOTE debugger for udf
		if(step_debugger())
		{
			std::cout << "  |#" << curr_step << "| [UDF Print->" << curr_val.to_string() << 
			"]" << std::endl;
		  ++curr_step;
		}
	}
	else if(fun_name == "itos")//int to string 
	{
		node.arg_list.front()->accept(*this);
		int val;
		curr_val.value(val);//get a copy of the in value
		std::string str = std::to_string(val);
		curr_val.set(str);
		
		//NOTE debugger for udf
		if(step_debugger())
		{
			std::cout << "  |#" << curr_step << "| [UDF ITOS->" << curr_val.to_string() << 
			"]" << std::endl;
		  ++curr_step;
		}
	}
	else if(fun_name == "length")//length of string
	{
		node.arg_list.front()->accept(*this);
		std::string in;
		curr_val.value(in);//get a copy of the input value
		int out = in.length();
		curr_val.set(out);
		
		//NOTE debugger for udf
		if(step_debugger())
		{
			std::cout << "  |#" << curr_step << "| [UDF Length->" << curr_val.to_string() << 
			"]" << std::endl;
		  ++curr_step;
		}
	}
	else if(fun_name == "stoi")//string to int
	{
		node.arg_list.front()->accept(*this);
		std::string in;
		curr_val.value(in);//get a copy of the in value
		int out = std::stoi(in);
		curr_val.set(out);		
		
		//NOTE debugger for udf
		if(step_debugger())
		{
			std::cout << "  |#" << curr_step << "| [UDF STOI->" << curr_val.to_string() << 
			"]" << std::endl;
		  ++curr_step;
		}
	}
	else if(fun_name == "dtos")//double to string
	{
		node.arg_list.front()->accept(*this);
		double val;
		curr_val.value(val);//get a copy of the in value
		std::string str = std::to_string(val);
		curr_val.set(str);
		
		//NOTE debugger for udf
		if(step_debugger())
		{
			std::cout << "  |#" << curr_step << "| [UDF DTOS->" << curr_val.to_string() << 
			"]" << std::endl;
		  ++curr_step;
		}
	}
	else if(fun_name == "get")//get character from string at index int
	{
		int count = 1;
		int index;
		std::string input;
		char out;
		for(Expr* e : node.arg_list)
		{
			if(count == 1)
			{
				e->accept(*this);
				curr_val.value(index);
			}
			else if(count == 2)
			{
				e->accept(*this);
				curr_val.value(input);
			}
			++count;
		}
		if(input.length() > 0)
		{
			if(index >= 0 && index < input.length())
			{
				out = input.at(index);
			}
			else	
				error("invalid index provided for get() function");
		}
		else
			error("get() function requires string size greater than 0");
		curr_val.set(out);
		
		//NOTE debugger for udf
		if(step_debugger())
		{
			std::cout << "  |#" << curr_step << 
			             "| [UDF GET->" << index <<" from "<< input <<
			             " is "<< out << "]" << std::endl;
		  ++curr_step;
		}
		
	}
	else if(fun_name == "read")//reads a value
	{
		std::string in;
		std::cin >> in;
		curr_val.set(in);
		
		
		//NOTE debugger for udf
		if(step_debugger())
		{
			std::cout << "  |#" << curr_step << "| [UDF Read->" << curr_val.to_string() << 
			"]" << std::endl;
		  ++curr_step;
		}
	}
	else if(fun_name == "stod")
	{
		node.arg_list.front()->accept(*this);
		std::string in;
		curr_val.value(in);//get a copy of the in value
		double out = std::stod(in);
		curr_val.set(out);
		
		
		//NOTE debugger for udf
		if(step_debugger())
		{
			std::cout << "  |#" << curr_step << "| [UDF STOD->" << curr_val.to_string() << 
			"]" << std::endl;
		  ++curr_step;
		}
	}
	//for normal functions
	else
	{	
		//Get the Function Delcaration
		FunDecl* fun_node = functions[fun_name];
		
		//NOTE debugger step check
		step_rng = false;
		step_rng = step_debugger();
		if(step_rng)
			std::cout << "  |#" << curr_step << "| [Function->" << fun_name << 
			"][Type->" << fun_node->return_type.lexeme() << 
			"][Parameters->";
			
		
		//create a list of the arguments
		std::list<DataObject> args;
		for(Expr* e: node.arg_list)
		{
			e->accept(*this);
			args.push_back(curr_val);
		}

		//save and set up environment
		int previous_environment = sym_table.get_environment_id();
		sym_table.set_environment_id(global_env_id);
		sym_table.push_environment();

		//get all of the parameter names for function
		auto x = args.begin();
		for(FunDecl::FunParam param: fun_node->params)
		{
			sym_table.add_name(param.id.lexeme());
			sym_table.set_val_info(param.id.lexeme(), *x);
			DataObject tmp = *x;
			//NOTE print the parameters
			if(step_rng)
				std::cout << "(" << param.id.lexeme() << "->" << tmp.to_string() << ")";
			x++;
		}

		//NOTE recover step check
		if(step_rng)
			std::cout << "]" << std::endl;
		step_rng = false;
		++curr_step;

		//evaluate the statements
		try
		{
			for(Stmt* s: functions[fun_name]->stmts)
			s->accept(*this);
		} catch (MyPLReturnException* e){}

		//pop back out and return to previous environment
		sym_table.pop_environment();
		sym_table.set_environment_id(previous_environment);
	}
}

//IDR Value
void Interpreter::visit(IDRValue& node)
{
	//Go through path
	int path_num = 1;
	DataObject tmp_dat;
	HeapObject tmp_obj;
	size_t tmp_oid;

	for(Token t : node.path)//iterate through lhs. Potentially a path
	{	
		if(node.path.size() == 1)
		{
			//set the current value to 
			sym_table.get_val_info(t.lexeme(), curr_val);
		}
		else
		{
			if(path_num == 1)//a.b.c
			{
				sym_table.get_val_info(t.lexeme(), tmp_dat);//get data object that holds the oid
				tmp_dat.value(tmp_oid);//get the oid
				heap.get_obj(tmp_oid, tmp_obj);//get heap object i.e. udt
			}
			else//set value to the value in last given id
			{
				tmp_obj.get_val(t.lexeme(), curr_val);//set value to attribute or last oid
				if(path_num != node.path.size())//if we aren't on the final element
				{
					//set the current oid value to one stored in curr_val
					curr_val.value(tmp_oid);
					//tmp_dat.value(tmp_oid);//get the oid
					heap.get_obj(tmp_oid, tmp_obj);//get heap object i.e. udt
				}
			}
		}
		++path_num;
	}
}


void Interpreter::visit(NegatedRValue& node)
{
	//get expression
	Expr* e = node.expr;
	e->accept(*this);
	if(curr_val.is_integer())//if it is an int
	{
		int tmp;
		curr_val.value(tmp);
		curr_val.set(tmp * (-1));
	}
	else if(curr_val.is_double())//if it is a double
	{
		double tmp;
		curr_val.value(tmp);
		curr_val.set(tmp * (-1.0));
	}
	else//else it an error
		error("Cannot negate non double/int expressions");
}


//DEBUGGER HELPER FUNCTIONS

//Ask user if they want to use debugger
void Interpreter::init_debugger()
{
	//initialize variables and ask user what they want to do
	char input;
	std::cout << "____________________________" << std::endl;
	std::cout << "| ENTER DEBUGGER [Y/N]:  ";
	std::cin >> input;
	
	//if y then they want to use the debugger
	if(input == 'Y' || input == 'y' || input == '1')
		debug = true;
	
	//if h then they need help
	else if(input == 'h' || input == 'H')
	{
		help_debugger();
		init_debugger();
	}
	
	//if b then they want to set breakpoints
	else if(input == 'b' || input == 'B')
	{
		std::cout << "| Enter Numerical Checkpoints (Enter 0 or negative to exit)" << std::endl;
		bool loop;
		
		//loop until they are dont inputting breakpoints
		do
		{
		  loop = true;
			int x;
			std::cout << "Breakpoint: ";
			std::cin >> x;
			if(x > 0)
			{
				breaks.push_back(x);
			}
			else
				loop = false;
			 
		}while(loop);
		debug = true;
	}
	else//if no, then debug is false and it wont debug
		debug = false;
}

//Debugger Stepper
bool Interpreter::step_debugger()
{
	if(debug == true)//if true, then we get to debug
	{
		if(step_to_end == true)//if step to end variable is true… i.e. someone hit q
		{
			if((std::find(breaks.begin(), breaks.end(), curr_step) != breaks.end()) == false)//first check to see that a breakpoint is not set here
				return true;//this will basically auto step until the end
		}
			
		//For each normal step case… as if we want to step or exit the code	
		char input;
		std::cout << std::endl;
		std::cout << "| STEP [S/X]: ";
		std::cin >> input;
		
		//if S, then Step
		if(input == 'S' || input == 's')
			return true;
		else if(input == 'Q' || input == 'q')//speed skip to end
		{
			step_to_end = true;
			return true;
		}
		else//otherwise we stop there and exit
		{
			debug = false;
			return false;
			std::cout << "| EXITING DEBUGGER          " << std::endl;
			std::cout << "____________________________" << std::endl;
		}
	}
	else
	{
		return false;
	}
}

//Debugger help section and instructions
void Interpreter::help_debugger()
{
	std::cout << " ________________________________________ " << std::endl;
	std::cout << "|                                        |" << std::endl;
	std::cout << "|            DEBUGGER CONTROLS           |" << std::endl;
	std::cout << "|                                        |" << std::endl;
	std::cout << "|->Press h in the initial prompt to get  |" << std::endl;
	std::cout << "|  here.                                 |" << std::endl;
	std::cout << "|                                        |" << std::endl;
  std::cout << "|->Press Y/y in the initial prompt to    |" << std::endl;
	std::cout << "|  enter debugging. Anything else will   |" << std::endl;
	std::cout << "|  skip debugging.                       |" << std::endl;
	std::cout << "|                                        |" << std::endl;
	std::cout << "|->When in debugging, press S/s to 'step'|" << std::endl;
	std::cout << "|  to the next line.                     |" << std::endl;
	std::cout << "|                                        |" << std::endl;
	std::cout << "|->When in debugging, press anything else|" << std::endl;
	std::cout << "|  to exit debugging.                    |" << std::endl;
	std::cout << "|                                        |" << std::endl;
  std::cout << "|->When stepping, you can step to the end|" << std::endl;
  std::cout << "|  by pressing Q/q.                      |" << std::endl;
  std::cout << "|                                        |" << std::endl;
  std::cout << "|->The left hand column shows the current|" << std::endl;
  std::cout << "|  step number you are on.               |" << std::endl;
  std::cout << "|                                        |" << std::endl;
  std::cout << "|->To set breakpoints or stopping points |" << std::endl;
  std::cout << "|  press B/b upon startup, then select   |" << std::endl;
  std::cout << "|  the numerical values of the step lines|" << std::endl;
  std::cout << "|  you want to stop at.                  |" << std::endl;
  std::cout << "|                                        |" << std::endl;
	std::cout << "|________________________________________|" << std::endl;
}
#endif

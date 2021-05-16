 //----------------------------------------------------------------------
// NAME:Wesley Muehlhausen
// FILE:printer.h
// DATE:Mar 1, 2021
// DESC:This file uses visitor functions to "pretty print" the
//      Abstract Syntax Tree created in Parser.h
//----------------------------------------------------------------------

#ifndef PRINTER_H
#define PRINTER_H

#include <iostream>
#include "ast.h"
#include <list>

class Printer : public Visitor
{
public:
  // constructor
  Printer(std::ostream& output_stream) : out(output_stream) {}

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
  std::ostream& out;
  int indent = 0;

  void inc_indent() {indent += 3;}
  void dec_indent() {indent -= 3;}
  std::string get_indent() {return std::string(indent, ' ');}

};

  // top-level
  void Printer::visit(Program& node)
  {
	//Go to function and type Declarations 
  	for(Decl* d : node.decls)
  		d->accept(*this);
  }
  
  //Function Declaration
  void Printer::visit(FunDecl& node)
  {
  	//function return type and parameters
  	std::cout << "fun ";
  	std::cout << node.return_type.lexeme() + " ";
  	std::cout << node.id.lexeme();
  	std::cout << "(";
  	int len = 0;
  	//parameters
  	for(auto vars = node.params.begin(); vars != node.params.end(); ++vars)
  	{
  		++len;
  		std::cout << vars->id.lexeme();
  		std::cout << ": ";
  		std::cout << vars->type.lexeme();
  		if(len != node.params.size())
  			std::cout << ", ";
  	}
	std::cout << ")" << std::endl;
	//statements
	inc_indent();
  	for(Stmt* s : node.stmts)
	{
		std::cout << get_indent();
  		s->accept(*this);
		std::cout << "" << std::endl;
	}
	dec_indent();
	std::cout << "end" << std::endl;
  	std::cout << "" << std::endl;
  }
  
  //Type Declaration
  void Printer::visit(TypeDecl& node)
  {
	//Header  
  	std::cout << "type " << node.id.lexeme() << std::endl;
	//Body Statements  
  	inc_indent();
  	for(VarDeclStmt* s : node.vdecls)
	{
		std::cout << get_indent();
		s->accept(*this);
		std::cout << "" << std::endl;
	}
  		
  	dec_indent();
  	std::cout << "end" << std::endl;
  	std::cout << "" << std::endl;
  }

  //Variable Declaration Statement
  void Printer::visit(VarDeclStmt& node)
  {
    //Left side
  	std::cout << "var ";
  	std::cout << node.id.lexeme();
  	if(node.type != nullptr)//Implicit Declaration statement
  	{
  		std::cout << ": ";
  		std::cout << node.type->lexeme();
  	}
  	//assignment to the expression
	std::cout << " = ";
	Expr* e = node.expr;
	e->accept(*this);
  	
  }

  //Assignment Statement
  void Printer::visit(AssignStmt& node)
  {
  	int len = 0;
    //left side
  	for(Token t : node.lvalue_list)
  	{	
  		++len;
  		std::cout << t.lexeme();
  		if(len != node.lvalue_list.size())
  			std::cout << ".";
  	}
  	//right side
  	std::cout << " = ";
  	Expr* e = node.expr;
  	e->accept(*this);
  }
  
  //Return Statement
  void Printer::visit(ReturnStmt& node)
  {
  	std::cout << "return ";
  	Expr* e = node.expr;
  	e->accept(*this);
  }
  
  //If-Elseif-Else Statemnts
  void Printer::visit(IfStmt& node)
  {
  	//IF PART
	//conditions
  	std::cout << "if ";	
  	Expr* e = node.if_part->expr;
  	e->accept(*this);
  	std::cout << " then" << std::endl;
  	inc_indent();
	//body statements 
  	for(Stmt* s : node.if_part->stmts)
	{  
		std::cout << get_indent();
  		s->accept(*this);
		std::cout << "" << std::endl;
	}
  	dec_indent();
  	//ELSEIF PART
  	for(BasicIf* b : node.else_ifs)
  	{
  		//head conditions
  		std::cout << get_indent() + "elseif ";
  		Expr* e = b->expr;
  		e->accept(*this);
  		std::cout << " then" << std::endl;
  		//body statements
  		inc_indent();
  		for(Stmt* s : b->stmts)
		{
			std::cout << get_indent();
			s->accept(*this);
			std::cout << "" << std::endl;
		}  
  		dec_indent();
  	}
  	//ELSE PART
  	if(node.body_stmts.empty() == false)
  	{
  		std::cout << get_indent() + "else" << std::endl;
  		inc_indent();
		//body statements
  	  	for(Stmt* st : node.body_stmts)
		{
			std::cout << get_indent();
			st->accept(*this);
			std::cout << "" << std::endl;
		}
  		dec_indent();
  	}	
  	std::cout << get_indent() + "end";

  }

  //While Statment
  void Printer::visit(WhileStmt& node)
  {
	//header with conditions
  	std::cout << "while ";
  	Expr* e = node.expr;
  	e->accept(*this);
  	std::cout << " do " << std::endl;
  	inc_indent();
	//body statements
  	for(Stmt* s : node.stmts)
	{
		std::cout << get_indent();
		s->accept(*this);
		std::cout << "" << std::endl;
	}	
  	dec_indent();
  	std::cout << get_indent() + "end ";
  }
  
  //For Statement
  void Printer::visit(ForStmt& node)
  {
	//for loop conditions
  	std::cout << "for " << node.var_id.lexeme() << " = ";
  	Expr* e = node.start;
  	e->accept(*this);
    std::cout << " to ";
  	Expr* n = node.end;
  	n->accept(*this);
  	std::cout << " do" << std::endl;
  	inc_indent();
	//body statements
  	for(Stmt* st : node.stmts)
	{
		std::cout << get_indent();
		st->accept(*this);
		std::cout << "" << std::endl;
	}
  	dec_indent();
  	std::cout << get_indent() + "end";
  }

  //Expression
  void Printer::visit(Expr& node)
  {
  	//Negated Check
  	if(node.negated == true)
  	 	std::cout << "not ";
	//For multi-term expressions
	if(node.op)
		std::cout << "(";
  	 ExprTerm* e = node.first;
  	 e->accept(*this);
  	 if(node.op)//If operator is used
	 {
		std::cout << " " << node.op->lexeme(); 
	 }
  	 if(node.rest)//Right part of the expression
  	 {
  	 	std::cout << " ";
  	 	Expr* ex = node.rest;
  	 	ex->accept(*this);
  	 }
	//For multi-term expressions
	if(node.op)
		std::cout << ")";
  } 

  //Simple Term
  void Printer::visit(SimpleTerm& node)
  {
  	RValue* r = node.rvalue;
  	r->accept(*this);
  }

  //Complex Term
  void Printer::visit(ComplexTerm& node)
  {
  	Expr* e = node.expr;
  	e->accept(*this);
  }
  
  //Simple Right Value
  void Printer::visit(SimpleRValue& node)
  {
  	std::cout << node.value.lexeme();
  }

  //New Right Value
  void Printer::visit(NewRValue& node)
  {
  	std::cout << "new " << node.type_id.lexeme();
  }

  //Call Expression
  void Printer::visit(CallExpr& node)
  {
  	std::cout  << node.function_id.lexeme() << "(";
  	int len = 0;
	//parameters
  	for(Expr* e : node.arg_list)
  	{
  		++len;
  		e->accept(*this);
  		if(len != node.arg_list.size())
  			std::cout << ", ";
  	}
  	std::cout << ")"; 
  	
  }

  //ID Right Value
  void Printer::visit(IDRValue& node)
  {
  	int len = 0;
  	//parameters
  	for(auto vals = node.path.begin(); vals != node.path.end(); ++vals)
  	{
  		++len;
  		std::cout << vals->lexeme();
  		if(len != node.path.size())
  			std::cout << ".";
  	}
  }

  //Negated Right Value
  void Printer::visit(NegatedRValue& node)
  {
  	std::cout << "neg ";
  	Expr* e = node.expr;
  	e->accept(*this);
  }


#endif

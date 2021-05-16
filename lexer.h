//----------------------------------------------------------------------
// NAME: Wesley Muehlhausen
// FILE: lexer.h
// DATE: Feb 1, 2020
// DESC: Implementation of a lexer including the next_token function
//----------------------------------------------------------------------

#ifndef LEXER_H
#define LEXER_H

#include <istream>
#include <string>
#include "token.h"
#include "mypl_exception.h"


class Lexer
{
public:

  // construct a new lexer from the input stream
  Lexer(std::istream& input_stream);

  // return the next available token in the input stream (including
  // EOS if at the end of the stream)
  Token next_token();
  
private:

  // input stream, current line, and current column
  std::istream& input_stream;
  int line;
  int column;

  // return a single character from the input stream and advance
  char read();

  // return a single character from the input stream without advancing
  char peek();

  // create and throw a mypl_exception (exits the lexer)
  void error(const std::string& msg, int line, int column) const;
};


Lexer::Lexer(std::istream& input_stream)
  : input_stream(input_stream), line(1), column(1)
{
}


char Lexer::read()
{
  return input_stream.get();
}


char Lexer::peek()
{
  return input_stream.peek();
}


void Lexer::error(const std::string& msg, int line, int column) const
{
  throw MyPLException(LEXER, msg, line, column);
}


Token Lexer::next_token()
{
  
 //Build Token 
  int temp_col;
  std::string lexeme = "";//current lexeme
  char ch;
  ch = read();//read first chararcter
  lexeme += ch;
  
  ////////////////////////////////////////////////
  //1) Read through the whitespace and/or comments  
  bool found_char = false;
  while(found_char == false)
  {
	//if newline, go to next line and continue
  	if(ch == '\n')
  	{
  			ch = read();
  			column = 1;
  			temp_col = column;
  			++line;
  	}
  	//while next character is whitespace, continue;
  	else if(std::isspace(ch))
  	{
  			ch = read();
  			++column;
  			temp_col = column;
  	}
  	//check for comments
  	else
  	{		
  			if(ch == '#')//if a comment line
  			{
  				ch = read();
  				column++;
  				bool contin = true;
  				while(contin == true)
  				{

  					if(ch == '\n')//no comment on next line
  					{
  						column = 1;
  						++line; 
  						ch = read();
  						contin = false;
  					}
					else//for newline
  					{
  						ch = read();
  						column++;
  					}
  				} 				
  			}
			else
			{
				found_char = true;
			}
  	}
  }


  //////////////////////////////////////////////////
  //2) check for the end-of-file (return  EOS token)
  if(ch == EOF)
  	return Token(EOS , "" , line , column);
  
  //////////////////////////////////////////////
  //3)check for simple, single character symbols  	
  if(ch == '(')
  	return Token(LPAREN , "(" , line , column++);
	
  else if(ch == ')')
  	return Token(RPAREN , ")" , line , column++);
  	
  else if(ch == ':')
  	return Token(COLON , ":" , line , column++);
  	
  else if(ch == '.')
  	return Token(DOT , "." , line , column++);
  	
  else if(ch == ',')
  	return Token(COMMA , "," , line , column++);
  	
  else if(ch == '-')
  	return Token(MINUS , "-" , line , column++);
  	
  else if(ch == '*')
  	return Token(MULTIPLY , "*" , line , column++);
  	
  else if(ch == '/')
  	return Token(DIVIDE , "/" , line , column++);
  	
  else if(ch == '+')
  	return Token(PLUS , "+" , line , column++);
  	
  else if(ch == '%')
  	return Token(MODULO , "%" , line , column++);

  /////////////////////////////////////
  //4) check for more invlolved symbols i.e. == = <= < != etc. 
  if(ch == '!')//check for proper use of !
  {
  	if(peek() == '=')//check for !=
  	{
  		read();
  		temp_col = column;
  		column += 2;
  		return Token(NOT_EQUAL , "!=" , line , temp_col);
  	}
  	else
  		error("Incorrect use of !", line, column);
  }
  
  else if(ch == '=')//check for == and =
  {
  	if(peek() == '=')
  	{
  		read();
  		temp_col = column;
  		column += 2;
  		return Token(EQUAL , "==" , line , temp_col);
  	}
  	else
  		return Token(ASSIGN , "=" , line , column++);
  }
	
  else if(ch == '<' || ch == '>')//check for < > <= >=
  {
  		if(peek() == '=')
  		{
  			read();
  			temp_col = column;
  			column += 2;
  			if(ch == '<')
				return Token(LESS_EQUAL , "<=" , line , temp_col);
			else
				return Token(GREATER_EQUAL , ">=" , line , temp_col);
  		}
  		else
  		{
  			if(ch == '<')
  				return Token(LESS , "<" , line , column++);
  			else
  				return Token(GREATER , ">" , line , column++);
  				
  		}
  }
  
  ///////////////////////////////
  //5) check for character values
  
  //set up and check for first quote
  lexeme = "";
  lexeme += ch;
  int tmpcol = column;
  std::string next = "";
  next += peek();
  
  //if found first quote, continue...
  if(next == "'")
  	error("Incomplete Char Value", line, column);
  if(lexeme == "'")//if first quote found, continue
  {
  	//check to see if next character is in the list of possible characters
  	char tmp = peek();//see if next character is valid
  	bool found = false;
  	if(std::isalpha(tmp) == 1024 || std::isdigit(tmp) == true)
  		found = true;
  	if(found == true)//if it is found, then check for the next character to see if it fits
  	{
  		column++;
  		ch = read();//advance
  		std::string char_val = "";
  		char_val += ch;
  		lexeme += ch;
  		std::string temp = "";
  		tmp = peek();
  		temp += tmp;
  		if(temp == "'")//check for final quote
  		{
  			column+=2;
  			ch = read();
  			lexeme += ch;
  			return Token(CHAR_VAL , char_val , line , tmpcol);
  		}
  		else//else trigger errors
  		{
  			error("1 Incomplete Char Value", line, column);
  		}
  	}
  	else
  	{
  		error("2 Incomplete Char Value", line, column);
  	}
  } 
  
  ////////////////////////////
  //6) check for string values
  
  //check for first quote
  if(ch == '"')
  {
  	std::string out_string = "";
  	ch = read();
  	int tmp = column;
  	column++;
  	if(ch == '"')//check for empty quote case
  	{
  		column++;
  		return Token(STRING_VAL , "" , line , tmp);
  	}
  	out_string += ch;
  	while(ch != '"')//while not at the end of the string or at the end of the file
  	{
  			if(ch == '\n')
  				error("Incomplete String", line, column);
  	
  			//std::cout << ch << std::endl;
  			ch = read();
  			++column;
  			if(ch != '"')
  				out_string += ch;
  				
  	}
  	++column;
  	return Token(STRING_VAL , out_string , line , tmp);
  }
  
  //////////////////////////////////////////////
  //7)check for numeric values(ints and doubles)
  bool found = false;
  bool is_double = false;
  if(std::isdigit(ch) == true)
  	found = true;
  if(found == true)//if this is a number...
  {
  	while(found == true)//keep on checking if char is a number or decimal
  	{
  		found = false;
  		if(std::isdigit(peek()) == true || peek() == '.')
  		{
  			found = true;
  			if(peek() == '.' && is_double == true)//if the decimal is already used
  					error("Incorrect Double Value", line, column);	
  			else if(peek() == '.' && is_double == false)//if first time declaring double val
  					is_double = true;
  				ch = read();
  				lexeme += ch;
  				column++;
  				found = true;
  		}
  	}
  	if(is_double == true)//double
  		return Token(DOUBLE_VAL , lexeme , line , column);
  	else//int
  		return Token(INT_VAL , lexeme , line , column);
  }
  
  //8)check for reserved words/ids (letter followed by letter, number or '\_')
  bool cont = true;
  char tmpc;
  tmpcol = column;
  column++;
  //continually build lexer and check if any of them are matches, if not then ID
  while(cont == true)
  {
  	
  	tmpc = peek();
  	if((std::isdigit(tmpc) == false && std::isalpha(tmpc) == false) && tmpc != '_')
  	{
		if(lexeme == "bool")
		{
			return Token(BOOL_TYPE, lexeme, line, tmpcol);
		}
		else if(lexeme == "char")
		{
			return Token(CHAR_TYPE, lexeme, line, tmpcol);	
		}
		else if(lexeme == "do" && peek() != 'u')
		{
			return Token(DO, lexeme, line, tmpcol);	
		}
		else if(lexeme == "true")
		{
			return Token(BOOL_VAL, lexeme, line, tmpcol);
		}
		
		else if(lexeme == "neg")
		{
			return Token(NEG, lexeme, line, tmpcol);
		}
		else if(lexeme == "and")
		{
			return Token(AND, lexeme, line, tmpcol);
		}
		else if(lexeme == "or")
		{
			return Token(OR, lexeme, line, tmpcol);
		}
		else if(lexeme == "not")
		{
			return Token(NOT, lexeme, line, tmpcol);
		}
		
		else if(lexeme == "nil")
		{
			return Token( NIL , lexeme, line, tmpcol);
		}
		else if(lexeme == "false")
		{
			return Token( BOOL_VAL , lexeme, line, tmpcol);
		}
		else if(lexeme == "double")
		{
			return Token( DOUBLE_TYPE , lexeme, line, tmpcol);
		}
		else if(lexeme == "else" && peek() != 'i')
		{
			return Token( ELSE , lexeme, line, tmpcol);
		}
		else if(lexeme == "elseif")
		{
			return Token( ELSEIF , lexeme, line, tmpcol);
		}
		else if(lexeme == "end")
		{
			return Token( END , lexeme, line, tmpcol);
		}
		else if(lexeme == "for")
		{
			return Token( FOR , lexeme, line, tmpcol);
		}
		else if(lexeme == "fun")
		{
			return Token( FUN , lexeme, line, tmpcol);
		}
		else if(lexeme == "if")
		{
			return Token( IF , lexeme, line, tmpcol);
		}
		else if(lexeme == "int")
		{
			return Token( INT_TYPE , lexeme, line, tmpcol);
		}
		else if(lexeme == "new")
		{
			return Token( NEW , lexeme, line, tmpcol);
		}
		else if(lexeme == "return")
		{
			return Token( RETURN , lexeme, line, tmpcol);
		}
		else if(lexeme == "string")
		{
			return Token( STRING_TYPE , lexeme, line, tmpcol);
		}
		else if(lexeme == "then")
		{
			return Token( THEN , lexeme, line, tmpcol);
		}
		else if(lexeme == "to")
		{
			return Token( TO , lexeme, line, tmpcol);
		}
		else if(lexeme == "type")
		{
			return Token( TYPE , lexeme, line, tmpcol);
		}
		else if(lexeme == "var")
		{
			return Token( VAR , lexeme, line, tmpcol);
		}
		else if(lexeme == "while")
		{
			return Token( WHILE , lexeme, line, tmpcol);
		}
	}
	if(peek() == '\n')//if the next character is a space
	{
		return Token(ID , lexeme , line , tmpcol);
	}
	else if(std::isspace(tmpc))
	{
		return Token(ID , lexeme , line , tmpcol);
	}
	else if((std::isalpha(tmpc) == false && std::isdigit(tmpc) == false) && tmpc != '_')
	{
		return Token(ID , lexeme , line , tmpcol);
	}
	else//else, keep on going
	{
		ch = read();
		column++;
		lexeme += ch;
	}
  }
  
  //otherwise, end
  return Token(EOS , "" , line , column);
}


#endif

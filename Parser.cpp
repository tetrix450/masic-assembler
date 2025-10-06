#include "Parser.hpp"
#include <vector>
#include <iostream>

std::string token_name[] = {"IDENTIFIER", "NUMBER", "STRING", "DIRECTIVE", "LABEL", "LBRACKET", "RBRACKET",
"ENDLINE", "PLUS", "MINUS", "MULTIPLY", "DIVIDE", "LPAREN", "RPAREN", "HASH", "END",
"COMMA", "PERCENT", "INVALID"
};

Parser::Parser(std::vector<token_t> tokens):tokens(tokens){
    if(tokens.size() > 0){
        token = tokens[0];
    }
}

irnode_t* Parser::create_node(token_t t, addr_t addressing, size_t size){
    irnode_t* node = new irnode_t{t, addressing, {}, size};
    nodes.push_back(node);
    return node;
}

void Parser::show_token(){
    std::cout << "(" << token.line << ", " << token.column << ") " << token_name[token.type] << ":" << token.value << " ";

    if(token.type == TK_ENDLINE || token.type == TK_END){
        std::cout << std::endl;
    }
}

bool Parser::advance(){
    if(pos < tokens.size() - 1){
        pos++;
        token = tokens[pos];
        
        show_token();

        return true;
    }else{
        return false;
    }
}

void Parser::expect(std::vector<ttype_t> types){
    if(types.size() == 1){ // Single type expected
        if(token.type == types[0]){
            return;
        }
        error_unexpected(token_name[types[0]]);
    }else{ // Multiple types expected
        std::string exp = "";
        for(size_t i = 0; i < types.size(); i++){
            // Return if token is expected
            if(token.type == types[i]){
                return;
            }
            
            // Construct string of expected identifiers
            if(i < types.size() - 2){
                exp += token_name[types[i]] + ", ";
            }else{
                exp += " or " + token_name[types[i]];
            }
        }
        
        // Throw an error showing what the parser expected
        error_unexpected(exp);
    }
}

// ###############################################
// ############### ERROR HANDLING ################
// ###############################################

void Parser::error_show_line(token_t t){
    std::cerr << "(" << t.line << ", " << t.column << ") [Parsing error]";
}

void Parser::error_unexpected(std::string expected){
    error_show_line(token);
    std::cerr << "Unexpected " << token_name[token.type] << " \"" << token.value << "\": expected " << expected << std::endl;
    exit(1);
}

void Parser::error_invalid(){
    std::cerr << "(" << token.line << ", " << token.column << ") [Parsing error] Invalid token \"" << token.value << "\"" << std::endl;
    exit(1);
}

void Parser::error_recursive_macro_expansion(token_t t){
    std::cerr << "(" << t.line << ", " << t.column << ") [Parsing error] Infinitely recursive macro expansion \"" << t.value << std::endl;
    exit(1);
}

void Parser::error_redefined_label(token_t t){
    std::cerr << "(" << t.line << ", " << t.column << ") [Parsing error] Label " << t.value << " is already defined" << std::endl;
    exit(1);
}

void Parser::error_redefined_symbol(token_t t){
    std::cerr << "(" << t.line << ", " << t.column << ") [Parsing error] Symbol " << t.value << " already defined" << std::endl;
    exit(1);
}

void Parser::error_redefined_macro(token_t t){
    std::cerr << "(" << t.line << ", " << t.column << ") [Parsing error] Macro " << t.value << std::endl;
    exit(1);
}

void Parser::error_reserved(){
    std::cerr << "(" << token.line << ", " << token.column << ") [Parsing error]: Tried to define " << token.value << ", which is a reserved word" << std::endl;
    exit(1);
}

void Parser::error_unrecognized_identifier(token_t t){
    std::cerr << "(" << t.line << ", " << t.column << ") [Parsing error]: Unrecognized identifier \"" << t.value << "\"" << std::endl;
    exit(1);
}

void Parser::error_unrecognized_directive(token_t t){
    std::cerr << "(" << t.line << ", " << t.column << ") [Parsing error]: Unrecognized directive \"" << t.value << "\"" << std::endl;
    exit(1);
}

// ###############################################
// ########## EXPRESSION EVALUATION ##############
// ###############################################

// A factor is a number, label, symbol or a parenthesis
int Parser::parse_factor(bool permit_labels){
    if(token.type == TK_NUMBER){
        int value = strto16(token.value);
        advance();
        return value;
    }else if(token.type == TK_IDENTIFIER){
        int value;
        if(symbol_table.find(token.value) != symbol_table.end()){
            value = symbol_table[token.value];
        }else if(label_table.find(token.value) != label_table.end()){
            if(permit_labels){
                value = label_table[token.value];
            }else{
                std::cerr << "(" << token.line << ", " << token.column << ") [Parsing error] labels are not permited in the expression on line " << token.line << std::endl;
                exit(1);
            }
        }else{
            std::cerr << "(" << token.line << ", " << token.column << ") [Parsing error]: Undefined symbol or label " << token.value << std::endl;
            exit(1);
        }
        advance();
        return value;
    }else if(token.type == TK_LPARENTHESIS){
        advance();
        int value = parse_expression(permit_labels);
        if(token.type != TK_RPARENTHESIS){
            error_unexpected(")");
        }
        advance();
        return value;
    }else if(token.type == TK_PLUS){
        advance();
        return parse_factor(permit_labels);
    }else if(token.type == TK_MINUS){
        advance();
        return -parse_factor(permit_labels);
    }else{
        error_unexpected("a NUMERICAL EXPRESSION");
        return 0;
    }
}

// A term is a product or quotient of factors
int Parser::parse_term(bool permit_labels){
    int value = parse_factor(permit_labels);

    while(token.type == TK_MULTIPLY || token.type == TK_DIVIDE){
        int op = token.type;
        advance();
        int rhs = parse_factor(permit_labels);

        if(op == TK_MULTIPLY) value *= rhs;
        else if(op == TK_DIVIDE) value /= rhs;
    }
    return value;
}

// An expression is a sum of terms
int Parser::parse_expression(bool permit_labels){
    int value = parse_term(permit_labels);

    while(token.type == TK_PLUS || token.type == TK_MINUS){
        int op = token.type;
        advance();
        int rhs = parse_term(permit_labels);

        if(op == TK_PLUS) value += rhs;
        else if(op == TK_MINUS) value -= rhs;
    }
    return value;
}

// Advance cursor until end of expression
void Parser::skip_expression(){
    bool keep_going = true;
    while(keep_going){
        switch(token.type){
            case TK_NUMBER:
            case TK_PLUS:
            case TK_MINUS:
            case TK_MULTIPLY:
            case TK_DIVIDE:
            case TK_LPARENTHESIS:
            case TK_RPARENTHESIS:
            case TK_IDENTIFIER:
                advance();
            break;
            default:
                keep_going = false;
            break;
        }
    }
}

// ###############################################
// ###############################################
// ###############################################

Parser::Parser(std::vector<token_t> tokens);

~Parser::Parser(){ // Free dynamic memory
    for(int i = nodes.size() - 1; i >= 0; i--){
        delete nodes[i];
    }
}

std::vector<irnode_t*> Parser::parse(){ // Returns machine code
    show_token();
    // Go through all the tokens
    while(token.type != TK_END){
        switch(token.type){
            case TK_DIRECTIVE:
                if(token.value == "equ"){ //.equ identifier expression
                    
                    advance();
                    
                    expect({TK_IDENTIFIER});
                    std::string symbol_identifier = token.value;
                    
                    advance();
                    
                    // Check if the symbol is not defined yet
                    if(symbol_table.find(symbol_identifier) == symbol_table.end()){

                        switch(token.type){ // Check if it is the start of an expression
                            case TK_NUMBER:
                            case TK_PLUS:
                            case TK_MINUS:
                            case TK_LPARENTHESIS:
                            case TK_IDENTIFIER:
                                    symbol_table[symbol_identifier] = pos;
                                    skip_expression();
                            break;
                            default:
                                error_unexpected("NUMERICAL EXPRESSION");
                        }

                    }else{
                        error_redefined_symbol(token);
                    }
                }else if(token.value == "org"){ // .org constant_expression
                    
                    advance();
                    
                    // Evaluate expression and change location counter
                    switch(token.type){
                        case TK_NUMBER:
                        case TK_PLUS:
                        case TK_MINUS:
                        case TK_LPARENTHESIS:
                            location_counter = parse_expression(false);
                        break;
                        default:
                            error_unexpected("CONSTANT NUMERICAL EXPRESSION");
                    }
                }else if(token.value == "d8" || token.value == "d16"){ // .d8/d16 expression, expression, ...
                    irnode_t* node = create_node(token.value, 0);

                    advance();
                    do{
                        // Expect start of a numerical expression
                        expect({TK_IDENTIFIER, TK_NUMBER, TK_PLUS, TK_MINUS, TK_LPARENTHESIS});

                        // Evaluate the expression later, when labels are resolved
                        node->expressions.push_back(pos);
                        skip_expression();

                        // Make room for the expression result in the node
                        if(node->identifier == "d8"){
                            node->size++;
                        }else{
                            node->size += 2;
                        }
                        
                        // Exit the loop when no more expressions are found
                        if(token.type != TK_COMMA){
                            break;
                        }
                    }while(advance());

                    location_counter += node->size;

                }else if(token.value == "r8" || token.value == "r16"){
                    irnode_t* node = create_node(token.value, 0);
                    advance();

                    // Evaluate expression and reserve bytes
                    switch(token.type){
                        case TK_NUMBER:
                        case TK_PLUS:
                        case TK_MINUS:
                        case TK_LPARENTHESIS:
                            if(token.value == "r8"){
                                node->size = parse_expression(false);
                            }else{
                                node->size = 2*parse_expression(false);
                            }
                        break;
                        default:
                            error_unexpected("CONSTANT NUMERICAL EXPRESSION");
                    }

                    location_counter += node->size;

                }else if(token.value == "ascii"){
                    advance();
                    expect({TK_STRING});
                    irnode_t* node = create_node("ascii", token.value.size());
                    node->expressions.push_back(pos);
                    location_counter += node->size;
                }else if(token.value == "macro"){
                    error_invalid();
                }else if(token.value == "endm"){
                    error_unexpected("PREVIOUS MACRO DEFINITION");
                }else{
                    error_unrecognized_directive(token);
                }
            break;
            case TK_IDENTIFIER:
                if(is_instruction(token.value)){
                    irnode_t* node = create_node(token.value, 0);
                    
                    advance();
                    switch(token.type){
                        case TK_IDENTIFIER:
                        case TK_NUMBER:
                        case TK_PLUS:
                        case TK_MINUS:
                        case TK_LPARENTHESIS:
                            node->addressing = ADDR_DIRECT;
                            node->size = 3;
                            expect({TK_IDENTIFIER, TK_NUMBER, TK_PLUS, TK_MINUS, TK_LPARENTHESIS});
                            node->expressions.push_back(pos);
                            skip_expression();
                        break;
                        case TK_HASH:
                            node->addressing = ADDR_IMMEDIATE;
                            node->size = 2;
                            advance();
                            expect({TK_IDENTIFIER, TK_NUMBER, TK_PLUS, TK_MINUS, TK_LPARENTHESIS});
                            node->expressions.push_back(pos);
                            skip_expression();
                        break;
                        case TK_LBRACKET:
                            node->addressing = ADDR_INDIRECT;
                            node->size = 3;
                            advance();
                            expect({TK_IDENTIFIER, TK_NUMBER, TK_PLUS, TK_MINUS, TK_LPARENTHESIS});
                            node->expressions.push_back(pos);
                            skip_expression();
                            expect({TK_RBRACKET});
                        break;
                        case TK_ENDLINE:
                        case TK_END:
                            node->addressing = ADDR_NOOP;
                            node->size = 1;
                        break;
                        default:
                            error_unexpected(token.value);
                        break;
                    }
                }else if(is_macro(token.value)){
                    error_invalid();
                }else{
                    error_invalid();
                }
            break;
            case TK_ENDLINE:
            case TK_END:
                // Do nothing
            break;
            case TK_LABEL:{
                if(label_table.find(token.value) == label_table.end()){
                    label_table[token.value] = location_counter;
                }else{
                    error_redefined_label(token);
                }
            }
            break;
            default:
                error_unexpected("a LABEL, DIRECTIVE or IDENTIFIER");
            break;
        }
        advance();
    }

    // Calculate all expressions

    return nodes;
};
#ifndef DSTRUCT_HPP
#define DSTRUCT_HPP
#include <string>
#include <vector>

// ########################################################
// ####################### Helpers ########################
// ########################################################

template<typename T>
bool contains(std::vector<T> v, T element){
    for(size_t i = 0; i < v.size(); i++){
        if(v[i] == element){
            return true;
        }
    }
    return false;
}

// ########################################################
// ################ Datatypes & structs ###################
// ########################################################

enum ttype_t{ // Token type
    TK_IDENTIFIER, TK_NUMBER, TK_STRING, TK_DIRECTIVE, TK_LABEL, TK_LBRACKET,
    TK_RBRACKET, TK_ENDLINE, TK_PLUS, TK_MINUS, TK_MULTIPLY, TK_DIVIDE,
    TK_LPARENTHESIS, TK_RPARENTHESIS, TK_HASH, TK_END, TK_COMMA, TK_PERCENT, TK_INVALID
};

enum addr_t{ // Addressing type
    ADDR_DIRECT, ADDR_IMMEDIATE, ADDR_INDIRECT, ADDR_NOOP, ADDR_INVALID
};

struct token_t{ // Token
    ttype_t type;
    std::string value;
    size_t line, column;
};

struct macro_t{ // Macro
    std::vector<std::string> parameters;
    std::vector<token_t> body;
    std::vector<std::string> labels; // Local labels inside the macro
};

struct irnode_t{ // Intermediate representation node
    token_t token; // Main token
    addr_t addressing; // Addressing mode of the instruction (if it is an instruction)
    std::vector<size_t> expressions; // Expression positions to be evaluated
    size_t size;
};

struct inst_t{
    std::string mnemonic;
    addr_t addressing;
    uint8_t opcode;
};

#endif
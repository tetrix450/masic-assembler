#ifndef DSTRUCT_HPP
#define DSTRUCT_HPP
#include <string>
#include <vector>
#include <inttypes.h>
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

inline bool ends_with(std::string const & value, std::string const & ending){
    if (ending.size() > value.size()) return false;
    return std::equal(ending.rbegin(), ending.rend(), value.rbegin());
}

// ########################################################
// ################ Datatypes & structs ###################
// ########################################################

enum ttype_t{ // Tipo de token
    TK_IDENTIFIER, TK_NUMBER, TK_STRING, TK_DIRECTIVE, TK_LABEL, TK_LBRACKET,
    TK_RBRACKET, TK_ENDLINE, TK_PLUS, TK_MINUS, TK_MULTIPLY, TK_DIVIDE,
    TK_LPARENTHESIS, TK_RPARENTHESIS, TK_HASH, TK_END, TK_COMMA, TK_PERCENT, TK_INVALID, TK_SHL, TK_SHR,
    TK_BAR, TK_AMPERSAND, TK_CIRCUMFLEX
};

enum addr_t{ // Tipo de datos del modo de direccionamiento
    ADDR_DIRECT, ADDR_IMMEDIATE, ADDR_INDIRECT, ADDR_NOOP, ADDR_INVALID, ADDR_IMMEDIATE_16
};

struct token_t{ // Token
    ttype_t type;
    std::string value;
    size_t line, column;
};

struct macro_t{ // Macro
    std::vector<std::string> parameters;// Identificadores de los parámetros
    std::vector<token_t> body;          // Cuerpo de la macro
    std::vector<std::string> labels;    // Etiquetas locales dentro de la macro
};

struct irnode_t{ // Intermediate representation node
    token_t token;                      // Token principal
    addr_t addressing;                  // Modo de direccionamiento de la instrucción (si es una instrucción)
    std::vector<size_t> expressions;    // Posiciones de las expresiones que serán evaluadas luego
    size_t size;
};

struct inst_t{
    std::string mnemonic;
    addr_t addressing;
    uint8_t opcode;
};

#endif
#ifndef LEXER_HPP
#define LEXER_HPP
#include <vector>
#include <string>
#include "DataStructures.hpp"

// ########################################################
// ######################## LEXER #########################
// ########################################################

class Lexer{
    private:
        std::string src = ""; // Source code
        size_t pos = 0; // Current position in src currently being indexed
        size_t line = 1; // Current line number in source code
        size_t col = 1; // Current column number in source code
        std::vector<token_t> tokens; // All the tokens in the program

        int advance(); // Advance cursor one character
        token_t read_identifier_or_label();
        token_t read_directive();
        token_t read_string();
        token_t read_number();

    public:
        Lexer(std::string source_filename);
        std::vector<token_t> tokenize();
        void print();
};

#endif
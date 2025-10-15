#ifndef PARSER_HPP
#define PARSER_HPP
#include "DataStructures.hpp"
#include <map>

class Parser{
private:
    std::vector<token_t> tokens; // Tokens to parse
    token_t token; // Actual token being parsed
    size_t pos = 0; // Position in the token list currently being parsed

    std::vector<irnode_t*> nodes; // Parsed nodes

    uint16_t location_counter = 0; // Necessary for label calculation
    uint32_t expanded_macros = 0; // Number of expanded macros (necessary to make labels unique after expansion)
    
    std::map<std::string, size_t> equ_table; // equ -> start of expression in token list
    std::map<std::string, uint16_t> label_table; // Label -> location
    std::map<std::string, macro_t> macros;

    irnode_t* createNode(token_t t, size_t size);
    bool advance();
    void expect(std::vector<ttype_t> types);
    void error(token_t t, std::string message);
    bool isMnemonic(std::string value);
    uint8_t getOpcode(token_t t, addr_t addressing);
    uint16_t strto16(const std::string& input);

    // Expression evaluation
    int parseFactor(bool permit_labels);
    int parseTerm(bool permit_labels);
    int parseExpression(bool permit_labels);
    void skipExpression();
    void showMacros();

    void show_node(irnode_t* node);
    
public:
    // Constructor and destructor
    Parser(std::vector<token_t> tokens);
    ~Parser();

    std::vector<uint8_t> parse();
};

#endif
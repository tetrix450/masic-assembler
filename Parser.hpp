#ifndef PARSER_HPP
#define PARSER_HPP
#include <vector>
#include "DataStructures.hpp"
#include <map>

// ########################################################
// ####################### PARSER #########################
// ########################################################

class Parser{
private:
    std::vector<token_t> tokens; // Tokens to parse
    std::vector<irnode_t*> nodes; // All nodes after parsing
    std::map<std::string, macro_t> macros;
    token_t token; // Actual token being parsed
    size_t pos = 0; // Position currently parsing
    uint16_t location_counter = 0; // Necessary for label calculation
    uint32_t instantiated_macros = 0; // Number of expanded macros (necessary to make labels unique after expansion)

    Parser(std::vector<token_t> tokens);
    irnode_t* create_node(token_t t, addr_t addressing, size_t size);
    void show_token();
    bool advance();
    void expect(std::vector<ttype_t> types);
    void error_show_line(token_t t);
    void error_unexpected(std::string expected);
    void error_invalid();
    void error_recursive_macro_expansion(token_t t);
    

public:

};


#endif
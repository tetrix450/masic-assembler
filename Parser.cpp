#include "Parser.hpp"
#include "Lexer.hpp"
#include "DataStructures.hpp"
#include <string>
#include <vector>
#include <iostream>
#include <stdexcept>
#include <cstdint>
#include <limits>
#include <algorithm>

std::string ttypeToString[] = {"IDENTIFIER", "NUMBER", "STRING", "DIRECTIVE", "LABEL", "LBRACKET", "RBRACKET",
"ENDLINE", "PLUS", "MINUS", "MULTIPLY", "DIVIDE", "LPAREN", "RPAREN", "HASH", "END",
"COMMA", "PERCENT", "INVALID"
};

std::vector<inst_t> instructions = {
    {"JMP", ADDR_DIRECT, 0x00},
    {"CLC", ADDR_NOOP, 0x01},
    {"STC", ADDR_NOOP, 0x02},
    {"CLH", ADDR_NOOP, 0x03},
    {"STH", ADDR_NOOP, 0x04},
    {"SUBC", ADDR_DIRECT, 0x05},
    {"SUBC", ADDR_IMMEDIATE, 0x06},
    {"STSP", ADDR_DIRECT, 0x07},
    {"LOAD", ADDR_DIRECT, 0x08},
    {"LOAD", ADDR_IMMEDIATE, 0x09},
    {"LOAD", ADDR_INDIRECT, 0x0A},
    {"STORE", ADDR_DIRECT, 0x0B},
    {"STORE", ADDR_INDIRECT, 0x0C},
    {"ADD", ADDR_DIRECT, 0x0D},
    {"ADD", ADDR_IMMEDIATE, 0x0E},
    {"ADC", ADDR_DIRECT, 0x0F},
    {"ADC", ADDR_IMMEDIATE, 0x10},
    {"SUB", ADDR_DIRECT, 0x11},
    {"SUB", ADDR_IMMEDIATE, 0x12},
    {"AND", ADDR_DIRECT, 0x13},
    {"AND", ADDR_IMMEDIATE, 0x14},
    {"OR", ADDR_DIRECT, 0x15},
    {"OR", ADDR_IMMEDIATE, 0x16},
    {"NOT", ADDR_NOOP, 0x17},
    {"STSP", ADDR_IMMEDIATE_16, 0x18},
    {"CMP", ADDR_DIRECT, 0x19},
    {"CMP", ADDR_IMMEDIATE, 0x1A},
    {"NOP", ADDR_NOOP, 0x1B},
    {"JV", ADDR_DIRECT, 0x1C},
    {"JNV", ADDR_DIRECT, 0x1D},
    {"JZ", ADDR_DIRECT, 0x1E},
    {"JNZ", ADDR_DIRECT, 0x1F},
    {"JC", ADDR_DIRECT, 0x20},
    {"JNC", ADDR_DIRECT, 0x21},
    {"JS", ADDR_DIRECT, 0x22},
    {"JNS", ADDR_DIRECT, 0x23},
    //{"???", ADDR_DIRECT, 0x24},
    //{"???", ADDR_DIRECT, 0x25},
    //{"???", ADDR_DIRECT, 0x26},
    //{"???", ADDR_DIRECT, 0x27},
    //{"???", ADDR_DIRECT, 0x28},
    //{"???", ADDR_DIRECT, 0x29},
    {"JMP", ADDR_INDIRECT, 0x2A},
    {"SHL", ADDR_NOOP, 0x2B},
    {"SHRA", ADDR_NOOP, 0x2C},
    {"SHR", ADDR_NOOP, 0x2D},
    {"ROL", ADDR_NOOP, 0x2E},
    {"ROR", ADDR_NOOP, 0x2F},
    {"IN", ADDR_INDIRECT, 0x30},
    {"PUSH", ADDR_NOOP, 0x31},
    {"POP", ADDR_NOOP, 0x32},
    {"CALL", ADDR_DIRECT, 0x33},
    {"RET", ADDR_NOOP, 0x34},
    {"INT", ADDR_IMMEDIATE, 0x35},
    {"RETI", ADDR_NOOP, 0x36},
    {"SCL", ADDR_NOOP, 0x37},
    {"SCR", ADDR_NOOP, 0x38},
    {"LDSPL", ADDR_NOOP, 0x39},
    {"RCL", ADDR_NOOP, 0x3A},
    {"RCR", ADDR_NOOP, 0x3B},
    {"OUT", ADDR_INDIRECT, 0x3C},
    {"IN", ADDR_DIRECT, 0x3D},
    {"OUT", ADDR_DIRECT, 0x3E},
    {"LDSPH", ADDR_NOOP, 0x3F},
};

// ########################################################
// ######################## Methods #######################
// ########################################################

// Show an error message at the line given by the token and exit the program
void Parser::error(token_t t, std::string message){
    std::cerr << "(" << t.line << ", " << t.column << ") [Parsing error]: " << message << std::endl;
    exit(1);
}

Parser::Parser(std::vector<token_t> tokens):tokens(tokens){
    if(tokens.size() > 0){
        token = tokens[0];
    }else{
        error({TK_INVALID, "INVALID", 1, 1}, "No tokens to parse");
    }
}

irnode_t* Parser::createNode(token_t t, size_t size){
    irnode_t* node = new irnode_t{t, ADDR_NOOP, {}, size};
    nodes.push_back(node);
    return node;
}

// Convert a string to an uint16_t securely. Accepts binary, hexadecimal and octal
uint16_t Parser::strto16(const std::string &input){
    std::string s = input;
    if (s.empty()) throw std::invalid_argument("Empty string: no value to convert to int");

    int base = 10;
    size_t start = 0;

    // Detect sign
    bool negative = false;
    if(s[0] == '-') {
        negative = true;
        start = 1;
    }else if (s[0] == '+'){
        start = 1;
    }

    // Detect base
    if (s.size() > start + 2 && s[start] == '0'){
        if(s[start + 1] == 'x' || s[start + 1] == 'X'){
            base = 16;
            start += 2;
        }else if(s[start + 1] == 'b' || s[start + 1] == 'B'){
            base = 2;
            start += 2;
        }else{
            base = 8;
            start += 1;
        }
    }

    if (start >= s.size()) throw std::invalid_argument("Invalid number");

    // Check valid characters knowing the base
    for (size_t i = start; i < s.size(); ++i){
        char c = s[i];
        bool valid = false;
        if (base == 2) valid = (c == '0' || c == '1');
        else if (base == 8) valid = (c >= '0' && c <= '7');
        else if (base == 10) valid = std::isdigit(c);
        else if (base == 16) valid = std::isxdigit(c);
        if (!valid) throw std::invalid_argument("Invalid character");
    }

    // Secure conversion
    uint32_t value = 0;
    for (size_t i = start; i < s.size(); ++i){
        char c = s[i];
        uint32_t digit = 0;
        if (std::isdigit(c)) digit = c - '0';
        else if (std::isalpha(c)) digit = std::tolower(c) - 'a' + 10;

        if (digit >= (uint32_t)base) throw std::invalid_argument("Out of range digit");

        if (value > (std::numeric_limits<uint16_t>::max() - digit) / base)
            throw std::overflow_error("Overflow: number does not fit in 16bits");

        value = value * base + digit;
    }

    if (negative){
        if (value > 32768) throw std::overflow_error("16-bit negative number overflow");
        return static_cast<uint16_t>(-static_cast<int32_t>(value));
    } else {
        return static_cast<uint16_t>(value);
    }
}

// Get instruction opcode given the mnemonic and addressing
uint8_t Parser::getOpcode(token_t t, addr_t addressing){
    // Get mnemonic and make it uppercase
    std::string mnemonic = t.value;
    std::transform(mnemonic.begin(), mnemonic.end(), mnemonic.begin(), [](unsigned char c){ return std::toupper(c); });
    
    for(size_t i = 0; i < instructions.size(); i++){
        if(instructions[i].mnemonic == mnemonic && instructions[i].addressing == addressing){
            return instructions[i].opcode;
        }
    }
    // Not found
    error(t, " invalid addressing for instruction " + mnemonic);
    return 0xFF;
}

// Advance position in the token list
bool Parser::advance(){
    if(pos < tokens.size() - 1){
        pos++;
        token = tokens[pos];
        
        /* --- Show token ---
        std::cout << "(" << token.line << ", " << token.column << ") " << ttypeToString[token.type] << ":" << token.value << " ";

        if(token.type == TK_ENDLINE || token.type == TK_END){
            std::cout << std::endl;
        }
        // ------------------*/

        return true;
    }else{
        return false;
    }
}

// Expect actual token to be one of the given
void Parser::expect(std::vector<ttype_t> types){
    if(types.size() == 1){ // Single type expected
        if(token.type == types[0]){
            return;
        }
        error(token, "Unexpected " + ttypeToString[token.type] + "(" + token.value + ") expected " + ttypeToString[types[0]]);
    }else{ // Multiple types expected
        std::string exp = "";
        for(size_t i = 0; i < types.size(); i++){
            // Return if token is expected
            if(token.type == types[i]){
                return;
            }
            
            // Construct string of expected identifiers
            if(i < types.size() - 2){
                exp += ttypeToString[types[i]] + ", ";
            }else if(i == types.size() - 2){
                exp += ttypeToString[types[i]];
            }else{
                exp += " or " + ttypeToString[types[i]];
            }
        }
        
        // Throw an error showing what the parser expected
        error(token, "Unexpected " + ttypeToString[token.type] + "(" + token.value + ") expected " + exp);
    }
}

// ###############################################
// ########## EXPRESSION EVALUATION ##############
// ###############################################

// A factor is a number, label, symbol or a parenthesis
int Parser::parseFactor(bool permit_labels){
    if(token.type == TK_NUMBER){
        int value = strto16(token.value);
        advance();
        return value;
    }else if(token.type == TK_IDENTIFIER){
        int value;
        if(equ_table.find(token.value) != equ_table.end()){
            value = equ_table[token.value];
        }else if(label_table.find(token.value) != label_table.end()){
            if(permit_labels){
                value = label_table[token.value];
            }else{
                std::cerr << "(" << token.line << ", " << token.column << ") [Parsing error] labels are not permited on this expression" << std::endl;
                exit(1);
            }
        }else{
            std::cerr << "(" << token.line << ", " << token.column << ") [Parsing error]: Undefined symbol " << token.value << std::endl;
            exit(1);
        }
        advance();
        return value;
    }else if(token.type == TK_LPARENTHESIS){
        advance();
        int value = parseExpression(permit_labels);
        if(token.type != TK_RPARENTHESIS){
            error(token, "Unexpected " + ttypeToString[token.type] + " (" + token.value + "), expected )");
        }
        advance();
        return value;
    }else if(token.type == TK_PLUS){
        advance();
        return parseFactor(permit_labels);
    }else if(token.type == TK_MINUS){
        advance();
        return -parseFactor(permit_labels);
    }else{
        error(token, "Unexpected " + ttypeToString[token.type] + " (" + token.value + "), expected a numerical expression");
        return 0;
    }
}

// A term is a product or quotient of factors
int Parser::parseTerm(bool permit_labels){
    int value = parseFactor(permit_labels);

    while(token.type == TK_MULTIPLY || token.type == TK_DIVIDE ||
    token.type == TK_SHR || token.type == TK_SHL ||
    token.type == TK_BAR || token.type == TK_AMPERSAND ||
    token.type == TK_CIRCUMFLEX){

        int op = token.type;
        advance();
        int rhs = parseFactor(permit_labels);

        if      (op == TK_MULTIPLY) value *= rhs;
        else if (op == TK_DIVIDE) value /= rhs;
        else if (op == TK_SHL) value <<= rhs;
        else if (op == TK_SHR) value >>= rhs;
        else if (op == TK_AMPERSAND) value &= rhs;
        else if (op == TK_BAR) value |= rhs;
        else if (op == TK_CIRCUMFLEX) value ^= rhs;
    }

    return value;
}

// An expression is a sum of terms
int Parser::parseExpression(bool permit_labels){
    int value = parseTerm(permit_labels);

    while(token.type == TK_PLUS || token.type == TK_MINUS){
        int op = token.type;
        advance();
        int rhs = parseTerm(permit_labels);

        if(op == TK_PLUS) value += rhs;
        else if(op == TK_MINUS) value -= rhs;
    }
    return value;
}

// Advance cursor until end of expression
void Parser::skipExpression(){
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
            case TK_BAR:
            case TK_AMPERSAND:
            case TK_CIRCUMFLEX:
            case TK_SHL:
            case TK_SHR:
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

bool Parser::isMnemonic(std::string value){
    // Put every character in uppercase
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c){ return std::toupper(c); });
    
    // Check if an instruction with this mnemonic exists
    for(size_t i = 0; i < instructions.size(); i++){
        if(instructions[i].mnemonic == value){
            return true;
        }
    }
    return false;
}

// Free dynamic memory
Parser::~Parser(){
    for(int i = nodes.size() - 1; i >= 0; i--){
        delete nodes[i];
    }
}

void Parser::show_node(irnode_t* node){
    std::cout << "##########################" << std::endl;
    std::cout << "+token:" << std::endl;
    std::cout << "      -value: " << node->token.value << std::endl;
    //std::cout << "      -type: " << ttypeToString[node->token.type] << std::endl;
    //std::cout << "      -line: " << node->token.line << std::endl;
    //std::cout << "      -column: " << node->token.column << std::endl;
    if(node->expressions.size() > 0){
        std::cout << "+expressions:" << std::endl;
            for(size_t i = 0; i < node->expressions.size(); i++){
            std::cout << "      " << node->expressions[i] << "(" << tokens[node->expressions[i]].value << ")"<< std::endl;
        }
    }else{
        std::cout << "+(no expressions)" << std::endl;
    }
    std::cout << "+size: " << node->size << std::endl;
}

void Parser::showMacros(){
    // Show macros (debug)
    for (const auto& pair:macros){
        std::cout << "#######################" << std::endl;
        macro_t macro = pair.second;

        std::cout << pair.first;
        for(size_t i = 0; i < macro.parameters.size(); i++){
            std::cout << " " + macro.parameters[i];
        }
        std::cout << std::endl;

        for(size_t i = 0; i < macro.body.size(); i++){
            if(macro.body[i].type == TK_ENDLINE){
                std::cout << std::endl;
            }else{
                std::cout << macro.body[i].value + " ";
            }
        }

        std::cout << "[Labels: ";
        for(size_t i = 0; i < macro.labels.size(); i++){
            std::cout << macro.labels[i] + " ";
        }
        std::cout << "]\n " << "#######################" << std::endl;
    }
}

std::vector<uint8_t> Parser::parse(){ // Returns machine code
    // Go through all the tokens
    while(token.type != TK_END){
        switch(token.type){
            case TK_DIRECTIVE:
                if(token.value == "equ"){ //.equ identifier expression
                    advance();
                    expect({TK_IDENTIFIER});
                    std::string symbol_identifier = token.value;

                    if(equ_table.find(symbol_identifier) == equ_table.end()){ // Check if the symbol is not defined yet
                        advance();
                        expect({TK_NUMBER, TK_PLUS, TK_MINUS, TK_LPARENTHESIS, TK_IDENTIFIER}); // Check if it is the start of an expression
                        equ_table[symbol_identifier] = pos;
                        skipExpression();
                    }else{
                        error(token, "Redefined " + token.value + " symbol");
                    }
                }else if(token.value == "org"){ // .org constant_expression
                    advance();
                    expect({TK_NUMBER, TK_PLUS, TK_MINUS, TK_LPARENTHESIS}); // Evaluate expression and change location counter
                    size_t old_location_counter = location_counter;
                    location_counter = parseExpression(false);

                    // Fill blank space with zeroes
                    if(location_counter > old_location_counter){
                        createNode({TK_DIRECTIVE, "filling", token.line, token.column - 1}, location_counter - old_location_counter);
                    }else if(location_counter != old_location_counter){
                        error(token, " .org directive can move the code forwards, not backwards");
                    }
                }else if(token.value == "d8" || token.value == "d16"){ // .d8/d16 <expression(s)>...
                    irnode_t* node = createNode(token, 0);

                    advance();
                    do{
                        // Expect start of a constant numerical expression
                        expect({TK_IDENTIFIER, TK_NUMBER, TK_PLUS, TK_MINUS, TK_LPARENTHESIS});

                        // Evaluate the expression later, when labels are resolved
                        node->expressions.push_back(pos);
                        skipExpression();

                        // Make room for the expression result in the node
                        if(node->token.value == "d8"){
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
                    irnode_t* node = createNode(token,  0);
                    std::string type = token.value;
                    advance();

                    // Expect start of a constant numerical expression
                    expect({TK_IDENTIFIER, TK_NUMBER, TK_PLUS, TK_MINUS, TK_LPARENTHESIS});
                    if(type == "r8"){ // Evaluate expression and reserve bytes
                        node->size = parseExpression(false);
                    }else{
                        node->size = 2*parseExpression(false);
                    }

                    location_counter += node->size;

                }else if(token.value == "ascii"){
                    advance();
                    expect({TK_STRING});
                    irnode_t* node = createNode(token, token.value.size() + 1);
                    location_counter += node->size;
                }else if(token.value == "macro"){
                    macro_t macro;
                    
                    // Pick up macro identifier
                    advance();
                    expect({TK_IDENTIFIER});
                    std::string macro_identifier = token.value;

                    // Pick up arguments
                    advance();
                    while(token.type != TK_ENDLINE){
                        expect({TK_IDENTIFIER});
                        macro.parameters.push_back(token.value);

                        advance();
                        expect({TK_COMMA, TK_ENDLINE});
                        if(token.type == TK_ENDLINE){
                            break;
                        }
                        advance();
                    }

                    // Pick up body and labels
                    while(!(token.type == TK_DIRECTIVE && token.value == "endm")){
                        if(token.type == TK_LABEL){
                            
                            // Check if global label exists
                            if(label_table.find(token.value) == label_table.end()){
                                
                                // Check if local labels and paramater names collide
                                for(size_t i = 0; i < macro.parameters.size(); i++){
                                    if(token.value == macro.parameters[i]){
                                        error(token, "Label cannot be named the same as a parameter in macro " + macro_identifier);
                                    }
                                }

                                // Check if local label exists
                                for(size_t i = 0; i < macro.labels.size(); i++){
                                    if(token.value == macro.labels[i]){
                                        error(token, "Redefined local label " + token.value + " at macro " + macro_identifier);
                                    }
                                }

                                macro.labels.push_back(token.value);
                            }else{
                                error(token, "Redefined global label " + token.value + " at macro " + macro_identifier);
                            }
                        }else if(token.type == TK_PERCENT){
                            macro.body.push_back(token);
                            advance();
                            expect({TK_IDENTIFIER});

                            // Check if it is a valid parameter
                            bool found = false;
                            for(size_t i = 0; i < macro.parameters.size() && !found; i++){
                                if(token.value == macro.parameters[i]){
                                    found = true;
                                }else if(token.value == macro.parameters[i] + ".low"){
                                    found = true;
                                }else if(token.value == macro.parameters[i] + ".high"){
                                    found = true;
                                }
                            }

                            if(!found){
                                error(token, "Undefined parameter " + token.value + " in macro " + macro_identifier);
                            }
                        }

                        macro.body.push_back(token);
                        advance();
                    }

                    macros[macro_identifier] = macro;
                }else if(token.value == "include"){
                    // Read the filename
                    advance();
                    expect({TK_STRING});
                    std::string filename = token.value;
                    advance();

                    // Read file and tokenize
                    Lexer lex(filename);
                    std::vector<token_t> readtoks = lex.tokenize();

                    // Insert all tokens
                    for (int i = readtoks.size() - 1; i >= 0; --i) {
                        tokens.insert(tokens.begin() + pos + 1, readtoks[i]);
                    }
                }else{
                    error(token, "Invalid directive " + token.value);
                }
            break;
            case TK_IDENTIFIER:{
                // Store mnemonic and make it uppercase
                std::string mnemonic = token.value;
                std::transform(mnemonic.begin(), mnemonic.end(), mnemonic.begin(), [](unsigned char c){ return std::toupper(c); });

                if(isMnemonic(mnemonic)){
                    irnode_t* node = createNode(token, 0);
                    
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
                            skipExpression();
                        break;
                        case TK_HASH:
                            if(mnemonic == "STSP"){
                                node->addressing = ADDR_IMMEDIATE_16;
                                node->size = 3;
                            }else{
                                node->addressing = ADDR_IMMEDIATE;
                                node->size = 2;
                            }
                            advance();
                            expect({TK_IDENTIFIER, TK_NUMBER, TK_PLUS, TK_MINUS, TK_LPARENTHESIS});
                            node->expressions.push_back(pos);
                            skipExpression();
                        break;
                        case TK_LBRACKET:
                            node->addressing = ADDR_INDIRECT;
                            node->size = 3;
                            advance();
                            expect({TK_IDENTIFIER, TK_NUMBER, TK_PLUS, TK_MINUS, TK_LPARENTHESIS});
                            node->expressions.push_back(pos);
                            skipExpression();
                            expect({TK_RBRACKET});
                        break;
                        case TK_ENDLINE:
                        case TK_END:
                            node->addressing = ADDR_NOOP;
                            node->size = 1;
                        break;
                        default:
                            error(token, "Unexpected " + ttypeToString[token.type] + "(" + token.value + "), " + "expected operand for " + node->token.value);
                        break;
                    }

                    location_counter += node->size;
                }else if(macros.find(token.value) != macros.end()){ // Macro expansion
                    macro_t macro = macros[token.value];

                    // -------------- Pick up arguments -----------------
                    std::map<std::string, token_t> arguments;
                    for(size_t i = 0; i < macro.parameters.size(); i++){
                        advance();
                        expect({TK_IDENTIFIER, TK_NUMBER});

                        uint16_t number = parseExpression(true);

                        token_t t = token;
                        t.type = TK_NUMBER;
                        t.value = std::to_string(number);
                        
                        arguments[macro.parameters[i]] = t;

                        if(i < macro.parameters.size() - 1){
                            expect({TK_COMMA});
                        }
                    }

                    if(macro.parameters.empty()){
                        advance();
                    }

                    expect({TK_ENDLINE, TK_END});
                    
                    // ------------- Insert body tokens -----------------
                    std::vector<token_t> expanded_tokens;
                    for(size_t i = 0; i < macro.body.size(); i++){
                        token_t t = macro.body[i];

                        if(t.type == TK_LABEL){ // Rename label
                            t.value = "__" + t.value + "__" + std::to_string(expanded_macros);

                        }else if(t.type == TK_IDENTIFIER){

                            // Rename if it is a label
                            for(size_t j = 0; j < macro.labels.size(); j++){
                                if(macro.labels[j] == t.value){
                                    t.value = "__" + t.value + "__" + std::to_string(expanded_macros);
                                    break;
                                }
                            }
                        }else if(t.type == TK_PERCENT){ // Parameter
                            t = macro.body[++i]; // Take next token

                            if(t.type == TK_IDENTIFIER || t.type == TK_NUMBER || t.type == TK_STRING){
                                
                                // Check if parameter is valid and replace
                                bool found = false;
                                for(size_t j = 0; j < macro.parameters.size() && !found; j++){
                                    if(t.value == macro.parameters[j]){
                                        t = arguments[macro.parameters[j]];
                                        found = true;
                                    }else if(t.value == macro.parameters[j] + ".low"){
                                        t = arguments[macro.parameters[j]];
                                        t.value += ".low";
                                        found = true;
                                    }else if(t.value == macro.parameters[j] + ".high"){
                                        t = arguments[macro.parameters[j]];
                                        t.value += ".high";
                                        found = true;
                                    }
                                }

                                if(!found){
                                    error(token, "Unknown parameter \"%" + t.value + "\"");
                                }
                            }else{
                                error(token, "Wrong parameter in macro after %: expected IDENTIFIER, NUMBER or STRING");
                            }
                        }

                        expanded_tokens.insert(expanded_tokens.begin(), t);
                    }

                    // Insert macro tokens into program
                    for(size_t i = 0; i < expanded_tokens.size(); i++){
                        tokens.insert(tokens.begin() + pos, expanded_tokens[i]);
                    }

                    // --------------------------------------------------
                    expanded_macros++;
                }else{
                    error(token, "Unrecognized identifier " + token.value);
                }
            break;
            }
            case TK_ENDLINE:
            case TK_END:
                // Do nothing
            break;
            case TK_LABEL:{
                if(label_table.find(token.value) == label_table.end()){
                    label_table[token.value] = location_counter;
                    equ_table[token.value + ".low"] = location_counter&0xFF;
                    equ_table[token.value + ".high"] = (location_counter>>8)&0xFF;
                }else{
                    error(token, "Redefined label " + token.value);
                }
            }
            break;
            default:
                error(token, "Unexpected " + ttypeToString[token.type] + "(" + token.value + "), expected a LABEL, DIRECTIVE or IDENTIFIER");
            break;
        }
        advance();
    }

    // Assemble all nodes
    std::vector<uint8_t> bytes;
    for(size_t i = 0; i < nodes.size(); i++){
        irnode_t* node = nodes[i];
        //show_node(node);
        std::string id = node->token.value;
        if(id == "r8" || id == "r16" || id == "filling"){
            // Reserve space
            for(size_t j = 0; j < node->size; j++){
                bytes.push_back(0);
            }
        }else if(id == "d8" || id == "d16"){
            // Evaluate all expressions
            for(size_t j = 0; j < node->expressions.size(); j++){
                pos = node->expressions[j];
                token = tokens[pos];
                if(id == "d8"){
                    bytes.push_back(parseExpression(true));
                }else{
                    uint16_t number = parseExpression(true);
                    bytes.push_back(number&0xFF);
                    bytes.push_back((number >> 8)&0xFF);
                }
            }
        }else if(node->token.type == TK_STRING){ // .ascii
            // Place the string as is
            for(size_t j = 0; j < node->token.value.size(); j++){
                bytes.push_back(node->token.value[j]);
            }
            bytes.push_back(0); // Null terminating character
        }else if(isMnemonic(id)){
            bytes.push_back(getOpcode(node->token, node->addressing));
            // Evaluate expression if has operand
            if(node->addressing != ADDR_NOOP){
                pos = node->expressions[0];
                token = tokens[pos];
                if(node->addressing == ADDR_IMMEDIATE){
                    bytes.push_back(parseExpression(true));
                }else{ // IMMEDIATE_16 && DIRECT
                    uint16_t number = parseExpression(true);
                    bytes.push_back(number&0xFF);
                    bytes.push_back((number >> 8)&0xFF);
                }
            }
        }else{
            error(token, "Invalid node");
        }
    }

    return bytes;
};
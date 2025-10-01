#include <iostream>
#include <fstream>
#include <map>
#include <vector>
#include <cctype>
#include <stdexcept>
#include <cstdint>
#include <limits>
#include <set>

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

struct csymbol_t{ // Constant symbol
    std::vector<token_t> expression;
    uint16_t value;
};

struct macro_t{ // Macro
    std::vector<std::string> arguments;
    std::vector<token_t> body;
    std::map<std::string, uint16_t> llabels; // Local labels inside the macro
};

struct irnode_t{ // Intermediate representation node
    std::string identifier; // Instruction mnemonic, directive identifier...
    addr_t addressing; // Addressing mode of the instruction (if it is an instruction)
    uint16_t location; // Calculated address
    size_t size; // Size in bytes of this node
};

struct expr_t{
    size_t start; // Expression start in token list
    irnode_t* node; // Node to store the expression result in
    std::string csymbol; // Constant symbol to store the expression result in
};

// ########################################################
// ################# Debug helpers ########################
// ########################################################

std::string token_name[] = {"IDENTIFIER", "NUMBER", "STRING", "DIRECTIVE", "LABEL", "LBRACKET", "RBRACKET",
"ENDLINE", "PLUS", "MINUS", "MULTIPLY", "DIVIDE", "LPAREN", "RPAREN", "HASH", "END", "COMMA", "PERCENT", "INVALID"
};
std::set<std::string> directives = {"d8", "d16", "r8", "r16", "org", "macro","endm", "equ", "ascii"};

// ########################################################
// ##################### Tables ###########################
// ########################################################

std::vector<expr_t> expressions;
std::map<std::string, csymbol_t> symbol_table;
std::map<std::string, uint16_t> labels;
std::map<std::string, macro_t> macros;

// ########################################################
// ##################### Instructions #####################
// ########################################################


struct inst_t{
    std::string mnemonic;
    addr_t addressing;
    uint8_t opcode;
};

std::vector<inst_t> instructions = {
    {"JMP", ADDR_DIRECT, 0x00},
    {"CLC", ADDR_NOOP, 0x01},
    {"STC", ADDR_NOOP, 0x02},
    {"CLI", ADDR_NOOP, 0x03},
    {"STI", ADDR_NOOP, 0x04},
    {"HLT", ADDR_NOOP, 0x05},
    {"INC", ADDR_NOOP, 0x06},
    {"DEC", ADDR_NOOP, 0x07},
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
    {"NEG", ADDR_NOOP, 0x18},
    {"CMP", ADDR_DIRECT, 0x19},
    {"CMP", ADDR_IMMEDIATE, 0x1A},
    {"NOP", ADDR_NOOP, 0x1B},
    {"JV", ADDR_DIRECT, 0x1C}, // V
    {"JNV", ADDR_DIRECT, 0x1D}, // !V
    {"JZ", ADDR_DIRECT, 0x1E}, // Z
    {"JNZ", ADDR_DIRECT, 0x1F}, // !Z
    {"JSNV", ADDR_DIRECT, 0x20}, // S!=V
    {"JSEV", ADDR_DIRECT, 0x21}, // S=V
    {"JZOSNV", ADDR_DIRECT, 0x22}, // Z | S!=V
    {"JNZASEV", ADDR_DIRECT, 0x23}, // !Z & S=V
    {"JC", ADDR_DIRECT, 0x24}, // C
    {"JNC", ADDR_DIRECT, 0x25}, // !C
    {"JZOC", ADDR_DIRECT, 0x26}, // Z | C
    {"JNZANC", ADDR_DIRECT, 0x27}, // !Z & !C
    {"JS", ADDR_DIRECT, 0x28}, // S
    {"JNS", ADDR_DIRECT, 0x29}, // !S
    {"JMP", ADDR_INDIRECT, 0x2A},
    {"SHL", ADDR_NOOP, 0x2B},
    {"SHRA", ADDR_DIRECT, 0x2C},
    {"SHR", ADDR_DIRECT, 0x2D},
    {"ROL", ADDR_NOOP, 0x2E},
    {"ROR", ADDR_NOOP, 0x2F},
    {"IN", ADDR_INDIRECT, 0x30},
    {"PUSH", ADDR_NOOP, 0x31},
    {"POP", ADDR_NOOP, 0x32},
    {"CALL", ADDR_NOOP, 0x33},
    {"RET", ADDR_NOOP, 0x34},
    {"INT", ADDR_DIRECT, 0x35},
    {"IRET", ADDR_NOOP, 0x36},
    {"STSP", ADDR_DIRECT, 0x37},
    {"OUT", ADDR_INDIRECT, 0x38},
    {"LDSPL", ADDR_NOOP, 0x39},
    {"RCL", ADDR_NOOP, 0x3A},
    {"RCR", ADDR_NOOP, 0x3B},
    {"CMP", ADDR_INDIRECT, 0x3C},
    {"IN", ADDR_DIRECT, 0x3D},
    {"OUT", ADDR_DIRECT, 0x3E},
    {"LDSPH", ADDR_NOOP, 0x3F},
};
// ########################################################
// ##################### Checkers #########################
// ########################################################
bool is_instruction(std::string value){
    for(size_t i = 0; i < instructions.size(); i++){
        if(instructions[i].mnemonic == value){
            return true;
        }
    }
    return false;
}

bool is_macro(std::string value){
    if(macros.find(value) != macros.end()){
        return true;
    }
    return false;
}

uint8_t get_opcode(std::string mnemonic, addr_t addressing){
    for(size_t i = 0; i < instructions.size(); i++){
        if(instructions[i].mnemonic == mnemonic && instructions[i].addressing == addressing){
            return instructions[i].opcode;
        }
    }
    // Not found
    return 0xFF;
}
// ########################################################
// ################ Classes and functions #################
// ########################################################

// Convert a string to an uint8_t securely. Accepts binary, hexadecimal and octal
uint8_t strto8(const std::string &input) {
    std::string s = input;
    if (s.empty()) throw std::invalid_argument("Empty string: no value to convert to uint8_t");

    int base = 10;
    size_t start = 0;

    // Detect sign
    bool negative = false;
    if(s[0] == '-') {
        negative = true;
        start = 1;
    } else if (s[0] == '+') {
        start = 1;
    }

    // Detect base
    if (s.size() > start + 2 && s[start] == '0') {
        if(s[start + 1] == 'x' || s[start + 1] == 'X') {
            base = 16;
            start += 2;
        } else if(s[start + 1] == 'b' || s[start + 1] == 'B') {
            base = 2;
            start += 2;
        } else {
            base = 8;
            start += 1;
        }
    }

    if (start >= s.size()) throw std::invalid_argument("Invalid number");

    // Check valid characters
    for (size_t i = start; i < s.size(); ++i){
        char c = s[i];
        bool valid = false;
        if (base == 2) valid = (c == '0' || c == '1');
        else if (base == 8) valid = (c >= '0' && c <= '7');
        else if (base == 10) valid = std::isdigit(c);
        else if (base == 16) valid = std::isxdigit(c);
        if (!valid) throw std::invalid_argument("Invalid character");
    }

    // Conversion
    uint32_t value = 0;
    for (size_t i = start; i < s.size(); ++i){
        char c = s[i];
        uint32_t digit = 0;
        if (std::isdigit(c)) digit = c - '0';
        else if (std::isalpha(c)) digit = std::tolower(c) - 'a' + 10;

        if (digit >= (uint32_t)base) throw std::invalid_argument("Digit out of range");

        if (value > (std::numeric_limits<uint8_t>::max() - digit) / base)
            throw std::overflow_error("Overflow: number does not fit in 8 bits");

        value = value * base + digit;
    }

    if (negative) {
        return static_cast<uint8_t>(-static_cast<int32_t>(value));
    } else {
        return static_cast<uint8_t>(value);
    }
}

// Convert a string to an uint16_t securely. Accepts binary, hexadecimal and octal
uint16_t strto16(const std::string &input){
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

// This is where all the source code is stored as tokens
std::vector<struct token_t> program;

// ########################################################
// ######################## LEXER #########################
// ########################################################

class Lexer{
    private:
        std::string src = ""; // Source code

        size_t pos = 0; // Current position in src currently being indexed
        size_t line = 1; // Current line number in source code
        size_t col = 1; // Current column number in source code

        std::vector<token_t> tokens;

        // Advance cursor
        int advance(){
            if(src[pos] == '\n'){
                col = 1;
                line++;
            }else{
                col++;
            }
            return pos++;
        }

        token_t read_identifier_or_label(){
            size_t start = pos;
            size_t start_col = col;

            // Move cursor until end of alfanumeric word
            while(pos < src.size() && (isalnum(src[pos]) || src[pos] == '_')){
                advance();
            }

            // Get the identifier/label string
            std::string value = src.substr(start, pos - start);

            // It is a label
            if(pos < src.size() && src[pos] == ':'){
                advance();
                return {TK_LABEL, value, start + 1, start_col};
            }

            // It is an identifier
            return {TK_IDENTIFIER, value, start + 1, start_col};
        }

        token_t read_directive(){
            size_t start = pos;
            size_t start_col = col;
            
            // Move cursor until end of alfanumeric word
            while(pos < src.size() && (isalnum(src[pos]) || src[pos] == '_')){
                advance();
            }

            // Get the directive string
            std::string value = src.substr(start, pos - start);

            // Return the token
            return {TK_DIRECTIVE, value, start + 1, start_col};
        }

        token_t read_string(){
            size_t start = pos;
            size_t start_col = col;
            
            // Move cursor until \"
            while(pos < src.size() && src[pos] != '\"'){
                advance();
            }

            if(pos == src.size() - 1){
                std::cerr << "[Lexical error] Found string with no closing quotes (\")" << std::endl;
                exit(1);
            }else{
                advance();
            }

            // Get the directive string
            std::string value = src.substr(start, pos - start - 1);

            return {TK_STRING, value, start + 1, start_col};
        }

        token_t read_number(){
            size_t start = pos;
            size_t start_col = col;

            // Leer el signo si lo tiene
            bool negative = false;
            if(pos < src.size() && src[pos] == '-'){
                negative = true;
                advance();
            } else if(pos < src.size() && src[pos] == '+'){
                advance();
            }

            // Detectar la base
            int base = 10;
            if(pos + 1 < src.size() && src[pos] == '0'){
                char next = src[pos + 1];
                if(next == 'x' || next == 'X'){         // Hexadecimal
                    base = 16;
                    advance();
                    advance();
                }else if(next == 'b' || next == 'B'){   // Binario
                    base = 2;
                    advance();
                    advance();
                }else{                                  // Octal
                    base = 8;
                    advance();
                }
            }

            // Leer dígitos válidos según la base
            while(pos < src.size()){
                char c = src[pos];
                bool valid = false;
                if(base == 2){
                    valid = (c == '0' || c == '1');
                }else if(base == 8){
                    valid = (c >= '0' && c <= '7');
                }else if(base == 10){
                    valid = isdigit(c);
                }else if(base == 16){
                    valid = isxdigit(c);
                }

                if(!valid){
                    break;
                }
                advance();
            }

            if(start == pos){
                std::cerr << "[Lexical error] Invalid number at line " << line << ", position " << col << std::endl;
                exit(1);
            }

            std::string value = src.substr(start, pos - start);
            if(negative) value = "-" + value;

            return {TK_NUMBER, value, line, start_col};
        }

    public:
        // The constructor reads the file into src and removes all comments
        Lexer(std::string source_filename){
            // Load source file
            std::ifstream source_file(source_filename);
            if(!source_file.is_open()){
                std::cerr << "Cannot open source file: " << source_filename << std::endl;
            }

            // Load all the source code into src and remove comments
            char c;
            while(source_file.get(c)){
                if(c != ';'){
                    src += c;
                // If a comment is found, remove it
                }else{
                    while(c != '\n' && source_file.get(c)){
                        // Skip comment characters
                    }
                    src += c;
                }
            }

            // Close source file
            source_file.close();
        }

        std::vector<token_t> tokenize(){

            // Read character by character and tokenize all the program
            while(pos < src.size()){
                char c = src[pos];

                if(c == ' ' || c == '\t'){
                    // Skip spaces or tabs
                    advance();
                }else if (isdigit(c)) {
                    // If it starts with a number or a sign followed by a number, then it's a number
                    tokens.push_back(read_number());
                }else if(isalnum(c) || c == '_'){
                    // Identifier or label
                    tokens.push_back(read_identifier_or_label());
                }else if(c == '.'){
                    // Directive
                    advance();
                    tokens.push_back(read_directive());
                }else if(c == '\n'){
                    // Endline
                    tokens.push_back({TK_ENDLINE, "ENDLINE", line, col});
                    advance();
                }else if(c == '\"'){
                    // String
                    advance();
                    tokens.push_back(read_string());
                }else if(c == '#'){
                    // ADDR_IMMEDIATE value
                    tokens.push_back({TK_HASH, "#", line, col});
                    advance();
                }else if(c == '['){
                    // Left bracket
                    tokens.push_back({TK_LBRACKET, "[", line, col});
                    advance();
                }else if(c == ']'){
                    // Right bracket
                    tokens.push_back({TK_RBRACKET, "]", line, col});
                    advance();
                }else if(c == '('){
                    // Left parentheses
                    tokens.push_back({TK_LPARENTHESIS, "(", line, col});
                    advance();
                }else if(c == ')'){
                    // Right parentheses
                    tokens.push_back({TK_RPARENTHESIS, ")", line, col});
                    advance();
                }else if(c == '+'){
                    tokens.push_back({TK_PLUS, "+", line, col});
                    advance();
                }else if(c == '-'){
                    tokens.push_back({TK_MINUS, "-", line, col});
                    advance();
                }else if(c == '*'){
                    tokens.push_back({TK_MULTIPLY, "*", line, col});
                    advance();
                }else if(c == '/'){
                    tokens.push_back({TK_DIVIDE, "/", line, col});
                    advance();
                }else if(c == ','){
                    tokens.push_back({TK_COMMA, ",", line, col});
                    advance();
                }else if(c == '%'){
                    tokens.push_back({TK_PERCENT, "%", line, col});
                }else{
                    std::cerr << "[Lexical error] Unexpected character: " << c << " on line " << line << ", position " << col << std::endl;
                    exit(1);
                    advance();
                }
            }

            tokens.push_back({TK_END, "END\n", line, col});
            return tokens;
        }

        void print(){
            for(size_t i = 0; i < tokens.size(); i++){
                token_t token = tokens[i];
                std::cout << "[" << i << "]->";

                switch(token.type){
                    case TK_PLUS:
                        std::cout << "TK_PLUS:";
                    break;
                    case TK_MINUS:
                        std::cout << "TK_MINUS:";
                    break;
                    case TK_MULTIPLY:
                        std::cout << "TK_MULTIPLY:";
                    break;
                    case TK_DIVIDE:
                        std::cout << "TK_DIVIDE:";
                    break;
                    case TK_DIRECTIVE:
                        std::cout << "TK_DIRECTIVE:";
                    break;
                    case TK_ENDLINE:
                        std::cout << "TK_ENDLINE" << std::endl;
                    break;
                    case TK_HASH:
                        std::cout << "TK_HASH:";
                    break;
                    case TK_LABEL:
                        std::cout << "TK_LABEL:";
                    break;
                    case TK_LBRACKET:
                        std::cout << "TK_LBRACKET:";
                    break;
                    case TK_RBRACKET:
                        std::cout << "TK_RBRACKET:";
                    break;
                    case TK_LPARENTHESIS:
                        std::cout << "TK_LPARENTHESIS:";
                    break;
                    case TK_RPARENTHESIS:
                        std::cout << "TK_RPARENTHESIS:";
                    break;
                    case TK_NUMBER:
                        std::cout << "TK_NUMBER:";
                    break;
                    case TK_STRING:
                        std::cout << "TK_STRING:";
                    break;
                    case TK_IDENTIFIER:
                        std::cout << "TK_IDENTIFIER:";
                    break;
                    case TK_INVALID:
                        std::cout << "TK_INVALID:";
                    break;
                    case TK_COMMA:
                        std::cout << "TK_COMMA:";
                    break;
                    default:
                        std::cout << "TK_END" << std::endl;
                    break;
                }

                if(token.type != TK_ENDLINE && token.type != TK_END)
                std::cout << token.value << " ";
            }
        }
};

// ########################################################
// ####################### PARSER #########################
// ########################################################

class Parser{
private:
    std::vector<token_t> tokens;
    std::vector<irnode_t*> nodes;
    token_t token;
    size_t pos;
    uint16_t location_counter;

    // ###############################################

    irnode_t* create_node(std::string identifier, addr_t addressing, uint16_t location, size_t size){
        irnode_t* node = new irnode_t{identifier, addressing, location, size};
        nodes.push_back(node);
        return node;
    }

    bool advance(){
        if(pos < tokens.size()){
            pos++;
            token = tokens[pos];

            std::cout << " " << token_name[token.type] << ":" << token.value;
            if(token.type == TK_ENDLINE || token.type == TK_END){
                std::cout << std::endl;
            }
            return true;
        }else{
            return false;
        }
    }

    // ###############################################
    // ############### ERROR HANDLING ################
    // ###############################################

    void error_unexpected(std::string expected){
        std::cerr << "[Parsing error] Unexpected token \"" << token.value << "\": expected " << expected << " at line " << token.line << ", column " << token.column << std::endl;
        exit(1);
    }

    void error_invalid(){
        std::cerr << "[Parsing error] Invalid token \"" << token.value << "\" at line " << token.line << ", column " << token.column << std::endl;
        exit(1);
    }

    void error_redefined_label(std::string label_value){
        std::cerr << "Label " << label_value << " already defined" << std::endl;
        exit(1);
    }

    void error_reserved(){
        std::cerr << "[Parsing error]: User-defined symbol " << token.value << " is a reserved word" << std::endl;
        exit(1);
    }

    void error_addressing(std::string mnemonic){
        std::cerr << "[Parsing error]: Incorrect addressing for instruction " << mnemonic << " at line " << token.line << ", column " << token.column << std::endl;
        exit(1);
    }

    void error_unrecognized_directive(){
        std::cerr << "[Parsing error]: Unrecognized directive " << token.value << " at line " << token.line << ", column " << token.column << std::endl;
        exit(1);
    }

    void error_undefined_label(token_t t){
        std::cerr << "[Parsing error]: Undefined label \"" << t.value << "\" at line " << t.line << ", column " << t.column << std::endl;
        exit(1);
    }

    // ###############################################
    // ########## EXPRESSION EVALUATION ##############
    // ###############################################

    // A factor is a number or a parenthesis
    int parse_factor(){
        if(token.type == TK_NUMBER){
            int value = strto16(token.value);
            advance();
            return value;
        }else if(token.type == TK_LPARENTHESIS){
            advance();
            int value = parse_expression();
            if(token.type != TK_RPARENTHESIS){
                error_unexpected(")");
            }
            advance();
            return value;
        }else if(token.type == TK_PLUS){
            advance();
            return parse_factor();
        }else if(token.type == TK_MINUS){
            advance();
            return -parse_factor();
        }else{
            error_unexpected("a NUMERICAL EXPRESSION");
            return 0;
        }
    }

    // A term is a product or quotient of factors
    int parse_term(){
        int value = parse_factor();

        while(token.type == TK_MULTIPLY || token.type == TK_DIVIDE){
            int op = token.type;
            advance();
            int rhs = parse_factor();

            if(op == TK_MULTIPLY) value *= rhs;
            else if(op == TK_DIVIDE) value /= rhs;
        }
        return value;
    }

    // An expression is a sum of terms
    int parse_expression(){
        int value = parse_term();

        while(token.type == TK_PLUS || token.type == TK_MINUS){
            int op = token.type;
            advance();
            int rhs = parse_term();

            if(op == TK_PLUS) value += rhs;
            else if(op == TK_MINUS) value -= rhs;
        }
        return value;
    }

    void store_expression(irnode_t* node){
        switch(token.type){
            case TK_NUMBER:
            case TK_PLUS:
            case TK_MINUS:
            case TK_LPARENTHESIS:
                // Evaluate the expression later, when labels are resolved
                expressions.push_back({pos,node, ""});
            break;
            default:
                error_unexpected("NUMERICAL EXPRESSION");
        }
    }

    // ###############################################
    // ###############################################
    // ###############################################

public:
    Parser(std::vector<token_t> tokens):tokens(tokens), pos(0), location_counter(0){
        if(tokens.size() > 0){
            token = tokens[0];
        }
    }

    ~Parser(){ // Free dynamic memory
        for(int i = nodes.size() - 1; i > 0; i--){
            delete nodes[i];
        }
    }

    std::vector<irnode_t*> parse(){
        // First pass
        while(token.type != TK_END){
            switch(token.type){
                case TK_DIRECTIVE:
                    if(token.value == "equ"){
                        advance();

                        if(token.type == TK_IDENTIFIER){
                            std::string symbol = token.value;
                            advance();
                            switch(token.type){
                                case TK_NUMBER:
                                case TK_PLUS:
                                case TK_MINUS:
                                case TK_LPARENTHESIS:
                                    // Evaluate the expression later, when labels are resolved
                                    expressions.push_back({pos,NULL, symbol});
                                break;
                                default:
                                    error_unexpected("NUMERICAL EXPRESSION");
                            }
                        }else{
                            error_unexpected("IDENTIFIER");
                        }
                    }else if(token.value == "org"){
                        advance();
                        // Evaluate expression and change location counter
                        switch(token.type){
                            case TK_NUMBER:
                            case TK_PLUS:
                            case TK_MINUS:
                            case TK_LPARENTHESIS:
                                location_counter = parse_expression();
                            break;
                            default:
                                error_unexpected("NUMERICAL EXPRESSION");
                        }
                    }else if(token.value == "d8"){
                        irnode_t* node = create_node("d8", ADDR_NOOP, location_counter, 0);

                        while(token.type != TK_ENDLINE || token.type != TK_END){
                            advance();
                            store_expression(node);
                            node->size++;
                            advance();
                            if(token.type != TK_COMMA){
                                break;
                            }
                        }
                    }else if(token.value == "d16"){
                        irnode_t* node = create_node("d16", ADDR_NOOP, location_counter, 0);

                        while(token.type != TK_ENDLINE || token.type != TK_END){
                            advance();
                            store_expression(node);
                            node->size += 2;
                            advance();
                            if(token.type != TK_COMMA){
                                break;
                            }
                        }
                    }else if(token.value == "r8"){
                        
                    }else if(token.value == "r16"){
                        
                    }else if(token.value == "ascii"){
                        
                    }else if(token.value == "macro"){
                        
                    }else if(token.value == "endm"){
                        
                    }else{
                        error_unrecognized_directive();
                    }
                break;
                case TK_IDENTIFIER:
                    if(is_instruction(token.value)){
                        
                    }else if(is_macro(token.value)){
                        
                    }else{
                        error_invalid();
                    }
                break;
                case TK_ENDLINE:
                case TK_END:
                    // Do nothing
                break;
                case TK_LABEL:{
                    if(labels.find(token.value) == labels.end()){
                        labels[token.value] = location_counter;
                    }else{
                        error_redefined_label(token.value);
                    }
                }
                break;
                default:
                    error_unexpected("a LABEL, DIRECTIVE or IDENTIFIER");
                break;
            }
            advance();
        }

        return nodes;
    };
};

int main(int argc, char* argv[]){
    // Check if the minimum number of arguments is correct
    if(argc < 2){
        std::cout << "Use: " << argv[0] << " <source file> <out file>" << std::endl;
        return 1;
    }

    // Get the filenames
    std::string source_filename(argv[1]);
    std::string output_filename = "out.mmc";
    if(argc > 2){
        output_filename = argv[2];
    }

    // Open output file
    std::ofstream output_file(output_filename, std::ios::binary);
    if(!output_file.is_open()){
        std::cerr << "[File error] Cannot open output file: " << output_filename << std::endl;
        return 1;
    }
    
    // Tokenize
    Lexer lexer(source_filename);
    std::vector<token_t> tokens = lexer.tokenize();
    
    // Parse
    Parser parser(tokens);
    std::vector<irnode_t*> nodes = parser.parse();

    // Assemble

    // Close files
    output_file.close();
}
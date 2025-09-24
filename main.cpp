#include <iostream>
#include <fstream>
#include <map>
#include <vector>
#include <cctype>
#include <stdexcept>
#include <cstdint>
#include <limits>
#include <set>

// Token type
enum ttype_t{TK_IDENTIFIER, TK_NUMBER, TK_STRING, TK_DIRECTIVE, TK_LABEL, TK_LBRACKET, TK_RBRACKET,
TK_ENDLINE, TK_PLUS, TK_MINUS, TK_MULTIPLY, TK_DIVIDE, TK_LPARENTHESIS, TK_RPARENTHESIS, TK_HASH, TK_END, TK_INVALID};

// Token struct
struct token_t{
    ttype_t type;
    std::string value;
    size_t line, column;
};

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

// Addressing datatype
enum addr_t {ADDR_DIRECT, ADDR_IMMEDIATE, ADDR_INDIRECT, ADDR_NOOP, ADDR_DIRECT_SIGNED, ADDR_DIRECT_UNSIGNED};

// Label dictionary
std::map<std::string, uint16_t> labels;

// Macro dictionary
struct macro_t{
    std::vector<std::string> parameters;
    std::vector<token_t> body;
};
std::map<std::string, std::vector<token_t>> macros;

// Symbol table (labels, equs...)
std::map<std::string, uint16_t> symbol_table;

// Opcode list
std::map<std::pair<std::string, addr_t>, uint8_t> opcodes = {
    {{"JMP", ADDR_DIRECT}, 0x00},
    {{"CLC", ADDR_NOOP}, 0x01},
    {{"STC", ADDR_NOOP}, 0x02},
    {{"CLI", ADDR_NOOP}, 0x03},
    {{"STI", ADDR_NOOP}, 0x04},
    {{"HLT", ADDR_NOOP}, 0x05},
    {{"INC", ADDR_NOOP}, 0x06},
    {{"DEC", ADDR_NOOP}, 0x07},
    {{"LOAD", ADDR_DIRECT}, 0x08},
    {{"LOAD", ADDR_IMMEDIATE}, 0x09},
    {{"LOAD", ADDR_INDIRECT}, 0x0A},
    {{"STORE", ADDR_DIRECT}, 0x0B},
    {{"STORE", ADDR_INDIRECT}, 0x0C},
    {{"ADD", ADDR_DIRECT}, 0x0D},
    {{"ADD", ADDR_IMMEDIATE}, 0x0E},
    {{"ADC", ADDR_DIRECT}, 0x0F},
    {{"ADC", ADDR_IMMEDIATE}, 0x10},
    {{"SUB", ADDR_DIRECT}, 0x11},
    {{"SUB", ADDR_IMMEDIATE}, 0x12},
    {{"AND", ADDR_DIRECT}, 0x13},
    {{"AND", ADDR_IMMEDIATE}, 0x14},
    {{"OR", ADDR_DIRECT}, 0x15},
    {{"OR", ADDR_IMMEDIATE}, 0x16},
    {{"NOT", ADDR_NOOP}, 0x17},
    {{"NEG", ADDR_NOOP}, 0x18},
    {{"CMP", ADDR_DIRECT}, 0x19},
    {{"CMP", ADDR_IMMEDIATE}, 0x1A},
    {{"NOP", ADDR_NOOP}, 0x1B},
    {{"JO", ADDR_DIRECT}, 0x1C},
    {{"JNO", ADDR_DIRECT}, 0x1D},
    {{"JZ", ADDR_DIRECT}, 0x1E},
    {{"JE", ADDR_DIRECT}, 0x1E},
    {{"JNZ", ADDR_DIRECT}, 0x1F},
    {{"JNE", ADDR_DIRECT}, 0x1F},
    {{"JNAE", ADDR_DIRECT_SIGNED}, 0x20},
    {{"JB", ADDR_DIRECT_SIGNED}, 0x20},
    {{"JAE", ADDR_DIRECT_SIGNED}, 0x21},
    {{"JNB", ADDR_DIRECT_SIGNED}, 0x21},
    {{"JNA", ADDR_DIRECT_SIGNED}, 0x22},
    {{"JBE", ADDR_DIRECT_SIGNED}, 0x22},
    {{"JA", ADDR_DIRECT_SIGNED}, 0x23},
    {{"JNBE", ADDR_DIRECT_SIGNED}, 0x23},
    {{"JNAE", ADDR_DIRECT_UNSIGNED}, 0x24},
    {{"JB", ADDR_DIRECT_UNSIGNED}, 0x24},
    {{"JAE", ADDR_DIRECT_UNSIGNED}, 0x25},
    {{"JNB", ADDR_DIRECT_UNSIGNED}, 0x25},
    {{"JBE", ADDR_DIRECT_UNSIGNED}, 0x26},
    {{"JNA", ADDR_DIRECT_UNSIGNED}, 0x26},
    {{"JA", ADDR_DIRECT_UNSIGNED}, 0x27},
    {{"JNBE", ADDR_DIRECT_UNSIGNED}, 0x27},
    {{"JS", ADDR_DIRECT}, 0x28},
    {{"JNS", ADDR_DIRECT}, 0x29},
    {{"JMP", ADDR_INDIRECT}, 0x2A},
    {{"SHL", ADDR_NOOP}, 0x2B},
    {{"SHR", ADDR_DIRECT_SIGNED}, 0x2C},
    {{"SHR", ADDR_DIRECT_UNSIGNED}, 0x2D},
    {{"ROL", ADDR_NOOP}, 0x2E},
    {{"ROR", ADDR_NOOP}, 0x2F},
    {{"IN", ADDR_INDIRECT}, 0x30},
    {{"PUSH", ADDR_NOOP}, 0x31},
    {{"POP", ADDR_NOOP}, 0x32},
    {{"CALL", ADDR_NOOP}, 0x33},
    {{"RET", ADDR_NOOP}, 0x34},
    {{"INT", ADDR_DIRECT}, 0x35},
    {{"IRET", ADDR_NOOP}, 0x36},
    {{"RETI", ADDR_NOOP}, 0x36},
    {{"STSP", ADDR_DIRECT}, 0x37},
    {{"OUT", ADDR_INDIRECT}, 0x38},
    {{"LDSPL", ADDR_NOOP}, 0x39},
    {{"RCL", ADDR_NOOP}, 0x3A},
    {{"RCR", ADDR_NOOP}, 0x3B},
    {{"CMP", ADDR_INDIRECT}, 0x3C},
    {{"IN", ADDR_DIRECT}, 0x3D},
    {{"OUT", ADDR_DIRECT}, 0x3E},
    {{"LDSPH", ADDR_NOOP}, 0x3F},
};

// Instructions
std::vector<std::pair<std::string, addr_t>> opcodes_inverse = {
    {"JMP", ADDR_DIRECT},
    {"CLC", ADDR_NOOP},
    {"STC", ADDR_NOOP},
    {"CLI", ADDR_NOOP},
    {"STI", ADDR_NOOP},
    {"HLT", ADDR_NOOP},
    {"INC", ADDR_NOOP},
    {"DEC", ADDR_NOOP},
    {"LOAD", ADDR_DIRECT},
    {"LOAD", ADDR_IMMEDIATE},
    {"LOAD", ADDR_INDIRECT},
    {"STORE", ADDR_DIRECT},
    {"STORE", ADDR_INDIRECT},
    {"ADD", ADDR_DIRECT},
    {"ADD", ADDR_IMMEDIATE},
    {"ADC", ADDR_DIRECT},
    {"ADC", ADDR_IMMEDIATE},
    {"SUB", ADDR_DIRECT},
    {"SUB", ADDR_IMMEDIATE},
    {"AND", ADDR_DIRECT},
    {"AND", ADDR_IMMEDIATE},
    {"OR", ADDR_DIRECT},
    {"OR", ADDR_IMMEDIATE},
    {"NOT", ADDR_NOOP},
    {"NEG", ADDR_NOOP},
    {"CMP", ADDR_DIRECT},
    {"CMP", ADDR_IMMEDIATE},
    {"NOP", ADDR_NOOP},
    {"JO", ADDR_DIRECT},
    {"JNO", ADDR_DIRECT},
    {"JZ", ADDR_DIRECT},
    {"JNZ", ADDR_DIRECT},
    {"JB", ADDR_DIRECT_SIGNED},
    {"JAE", ADDR_DIRECT_SIGNED},
    {"JNA", ADDR_DIRECT_SIGNED},
    {"JA", ADDR_DIRECT_SIGNED},
    {"JB", ADDR_DIRECT_UNSIGNED},
    {"JAE", ADDR_DIRECT_UNSIGNED},
    {"JBE", ADDR_DIRECT_UNSIGNED},
    {"JA", ADDR_DIRECT_UNSIGNED},
    {"JS", ADDR_DIRECT},
    {"JNS", ADDR_DIRECT},
    {"JMP", ADDR_INDIRECT},
    {"SHL", ADDR_NOOP},
    {"SHR", ADDR_DIRECT_SIGNED},
    {"SHR", ADDR_DIRECT_UNSIGNED},
    {"ROL", ADDR_NOOP},
    {"ROR", ADDR_NOOP},
    {"IN", ADDR_INDIRECT},
    {"PUSH", ADDR_NOOP},
    {"POP", ADDR_NOOP},
    {"CALL", ADDR_NOOP},
    {"RET", ADDR_NOOP},
    {"INT", ADDR_DIRECT},
    {"IRET", ADDR_NOOP},
    {"STSP", ADDR_DIRECT},
    {"OUT", ADDR_INDIRECT},
    {"LDSPL", ADDR_NOOP},
    {"RCL", ADDR_NOOP},
    {"RCR", ADDR_NOOP},
    {"CMP", ADDR_INDIRECT},
    {"IN", ADDR_DIRECT},
    {"OUT", ADDR_DIRECT},
    {"LDSPH", ADDR_NOOP},
};

// Instruction mnemonics
std::set<std::string> mnemonics = {"JMP", "CLC", "STC", "CLI", "STI", "HLT", "INC", "DEC", "LOAD", "STORE", "ADD",
 "ADC", "SUB", "AND", "OR", "NOT", "NEG", "CMP", "NOP", "JO", "JNO", "JZ", "JE", "JNZ", "JNE", "JNAE", "JB", "JAE",
  "JNB", "JBE", "JNA", "JA", "JNBE", "JC", "JNC", "JS", "JNS", "SHL", "SHR", "ROL", "ROR", "RCL", "PUSH", "POP",
  "CALL", "RET", "INT", "RETI", "IRET", "STSP", "IN", "OUT", "LDSPL", "LDSPH"
};

// Directive identifiers (they all start with a '.')
std::set<std::string> directives = {"d8", "d16", "r8", "r16", "org", "macro", "endm", "equ", "ascii"};

// This is where all the source code is stored as tokens
std::vector<struct token_t> program;

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
                if(next == 'x' || next == 'X'){ // Hexadecimal
                    base = 16;
                    advance();
                    advance();
                }else if(next == 'b' || next == 'B'){ // Binario
                    base = 2;
                    advance();
                    advance();
                }else{ // Octal
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
                    tokens.push_back({TK_ENDLINE, "ENDLINE\n", line, col});
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
                }else{
                    std::cerr << "[Lexical error] Unexpected character: " << c << " on line " << line << ", position " << col << std::endl;
                    exit(1);
                    advance();
                }
            }

            tokens.push_back({TK_END, "TK_END", line, col});
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
                    default:
                        std::cout << "TK_END" << std::endl;
                    break;
                }

                if(token.type != TK_ENDLINE && token.type != TK_END)
                std::cout << token.value << " ";
            }
        }
};

// Intermediate representation node
enum ntype_t {NODE_INSTRUCTION, NODE_DIRECTIVE, NODE_BUFFER};
struct irnode_t{
    ntype_t type; // Node type
    
    /*
    This vector of strings holds all the values needed for the IR node:
    NODE_INSTRUCTION:   (0):Mnemonic    (1):Operand
    NODE_DIRECTIVE:     (0):Mnemonic    (1):Operand     (2 or more):Other operands if necessary...
    */
    std::vector<std::string> values;
    std::vector<uint8_t> buffer_numbers;

    addr_t addressing; // Addressing mode of the instruction (if it is an instruction)
    uint16_t address; // Calculated address
    size_t size; // Size in bytes of this node
};

class Parser{
private:
    std::map<std::string, size_t> unresolved_labels; // References the node in the vector of nodes that has the label unresolved
    std::vector<token_t> tokens;
    std::vector<irnode_t> nodes;
    token_t token;
    size_t pos;
    uint16_t location_counter;

    std::string to_string(uint16_t value, int base){
    if (base < 2 || base > 16) {
        error_invalid();
    }

    if (value == 0) return "0";

    std::string digits = "0123456789ABCDEF";
    std::string result;

    while (value > 0) {
        int digit = value % base;
        result.insert(result.begin(), digits[digit]);
        value /= base;
    }

    return result;
}

    bool advance(){
        if(pos < tokens.size()){
            pos++;
            token = tokens[pos];
            std::cout << " " << token.value;
            return true;
        }else{
            return false;
        }
    }

    bool expect(ttype_t t){
        if(token.type == t){
            return true;
        }else{
            error_unexpected();
            return false;
        }
    }

    void error_unexpected(){
        std::cerr << "[Parsing error] Unexpected token \"" << token.value << "\" at line " << token.line << ", column " << token.column << std::endl;
    }

    void error_invalid(){
        std::cerr << "[Parsing error] Invalid token \"" << token.value << "\" at line " << token.line << ", column " << token.column << std::endl;
    }

    void error_reserved(){
        std::cerr << "[Parsing error]: User-defined symbol " << token.value << " is a reserved word" << std::endl;
    }
public:
    Parser(std::vector<token_t> tokens):tokens(tokens), pos(0), location_counter(0){
        if(tokens.size() > 0){
            token = tokens[0];
        }
    }
    
    uint16_t parse_expression(uint16_t& accumulator){
        switch(token.type){
            case TK_PLUS:
                advance();
                accumulator += parse_expression(accumulator);
            break;
            case TK_MINUS:
                advance();
                accumulator -= parse_expression(accumulator);
            break;
            case TK_NUMBER:{
                token_t token_number = token;

                advance();
                if(token.type == TK_PLUS){
                    return strto16(token_number.value) + parse_expression(accumulator);
                }else if(token.type == TK_MINUS){
                    return strto16(token_number.value) - parse_expression(accumulator);
                }else if(token.type == TK_NUMBER){
                    return strto16(token_number.value);
                }else if(token.type != TK_ENDLINE){
                    error_unexpected();
                }
            }
            break;
            case TK_END:
            case TK_ENDLINE:
            advance();
                // Do nothing more
            break;
            default:
                advance();
                error_unexpected();
            break;
        }
        return 0;
    }

    std::vector<irnode_t> parse(){
        // First pass
        std::cout << "[Info]: Parsing token(s) " << token.value;
        while(token.type != TK_END){            
            switch(token.type){
                case TK_DIRECTIVE:
                    if(token.value == "equ"){
                        advance();
                        // First symbol must be an identifier and not a reserved word
                        if(token.type != TK_IDENTIFIER) error_unexpected();
                        if(mnemonics.find(token.value) != mnemonics.end()) error_reserved();
                        
                        // Symbol identifier
                        std::string symbol_identifer = token.value;
                        advance();

                        // Next symbol must be a label or a number
                        if(token.type == TK_IDENTIFIER){
                            // Convert the label to an address and add it to the symbol table
                            if(labels.find(token.value) == labels.end()){
                                // Label not found, resolve it later
                                unresolved_labels[token.value] = pos;
                            }else{
                                // Label found
                                symbol_table[symbol_identifer] = labels[token.value];
                            }
                        }else if(token.type == TK_NUMBER || token.type == TK_PLUS || token.type == TK_MINUS){
                            uint16_t number = 0;
                            parse_expression(number);
                        }else{
                            error_invalid();
                        }

                        advance();
                        //expect(TK_ENDLINE);
                    }else if(token.value == "org"){
                        advance();
                        if(token.type == TK_NUMBER){
                            location_counter = strto16(token.value);
                        }else{
                            error_unexpected();
                        }

                        advance();
                        expect(TK_ENDLINE);
                    }else if(token.value == "d8"){
                        // Get all bytes
                        uint16_t start_location_counter = location_counter;
                        advance();
                        std::vector<uint8_t> numbers;
                        while(token.type == TK_NUMBER){
                            numbers.push_back(strto8(token.value));
                            location_counter++;
                            advance();
                        }
                        nodes.push_back({NODE_BUFFER, {"d8"}, numbers, ADDR_DIRECT, start_location_counter, numbers.size()});
                    }else if(token.value == "d16"){
                        // Get all double bytes
                        uint16_t start_location_counter = location_counter;
                        advance();
                        std::vector<uint8_t> numbers;
                        while(token.type == TK_NUMBER){
                            numbers.push_back(strto8(token.value)&0xFF); // Low byte
                            location_counter++;
                            numbers.push_back((strto8(token.value) >> 8)&0xFF); // High byte
                            location_counter++;
                            advance();
                        }
                        nodes.push_back({NODE_BUFFER, {"d16"}, numbers, ADDR_DIRECT, start_location_counter, numbers.size()});
                    }
                break;
                case TK_IDENTIFIER:
                    advance();
                break;
                case TK_ENDLINE:
                    advance();
                break;
                case TK_LABEL:{
                    std::string label_value = token.value;
                    if(labels.find(label_value) == labels.end()){
                        labels[label_value] = location_counter;
                    }else{
                        std::cerr << "Label " << label_value << " already defined" << std::endl;
                    }

                    advance();
                }
                
                break;
                default:
                    error_unexpected();
                break;
            }
            
            std::cout << "[Info]: Parsing token(s)";
            advance();
        }
        return std::vector<irnode_t>();
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
    std::vector<irnode_t> nodes = parser.parse();

    // Assemble

    // TEST
    std::vector<token_t> subtokens(tokens.begin() + 161, tokens.begin() + 176);

    // Close files
    output_file.close();
}
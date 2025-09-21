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
enum type_t{TK_IDENTIFIER, TK_NUMBER, TK_STRING, TK_DIRECTIVE, TK_LABEL, TK_LBRACKET, TK_RBRACKET,
TK_ENDLINE, TK_ARITHMETIC_OP, TK_LPARENTHESIS, TK_RPARENTHESIS, TK_HASH, TK_END};

// Token struct
struct token_t{
    type_t type;
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
        if (!valid) throw std::invalid_argument("Invalid character for the base");
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
        if (!valid) throw std::invalid_argument("Carácter no válido para la base");
    }

    // Secure conversion
    uint32_t value = 0;
    for (size_t i = start; i < s.size(); ++i){
        char c = s[i];
        uint32_t digit = 0;
        if (std::isdigit(c)) digit = c - '0';
        else if (std::isalpha(c)) digit = std::tolower(c) - 'a' + 10;

        if (digit >= (uint32_t)base) throw std::invalid_argument("Dígito fuera de rango");

        if (value > (std::numeric_limits<uint16_t>::max() - digit) / base)
            throw std::overflow_error("Overflow: el número no cabe en 16 bits");

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
enum addr_t {DIRECT, IMMEDIATE, INDIRECT, NOOP, SIGNED, UNSIGNED};

// Label dictionary
std::map<std::string, uint16_t> labels;

// Macro dictionary
struct macro_t{
    std::vector<std::string> parameters;
    std::vector<token_t> body;
};
std::map<std::string, std::vector<token_t>> macros;

// Symbol table (labels, equs...)
struct symbol_t{
    size_t address;

};

std::map<std::string, symbol_t> symbol_table;

// Opcode list
std::map<std::pair<std::string, addr_t>, uint8_t> opcodes = {
    {{"JMP", DIRECT}, 0x00},
    {{"CLC", NOOP}, 0x01},
    {{"STC", NOOP}, 0x02},
    {{"CLI", NOOP}, 0x03},
    {{"STI", NOOP}, 0x04},
    {{"HLT", NOOP}, 0x05},
    {{"INC", NOOP}, 0x06},
    {{"DEC", NOOP}, 0x07},
    {{"LOAD", DIRECT}, 0x08},
    {{"LOAD", IMMEDIATE}, 0x09},
    {{"LOAD", INDIRECT}, 0x0A},
    {{"STORE", DIRECT}, 0x0B},
    {{"STORE", INDIRECT}, 0x0C},
    {{"ADD", DIRECT}, 0x0D},
    {{"ADD", IMMEDIATE}, 0x0E},
    {{"ADC", DIRECT}, 0x0F},
    {{"ADC", IMMEDIATE}, 0x10},
    {{"SUB", DIRECT}, 0x11},
    {{"SUB", IMMEDIATE}, 0x12},
    {{"AND", DIRECT}, 0x13},
    {{"AND", IMMEDIATE}, 0x14},
    {{"OR", DIRECT}, 0x15},
    {{"OR", IMMEDIATE}, 0x16},
    {{"NOT", NOOP}, 0x17},
    {{"NEG", NOOP}, 0x18},
    {{"CMP", DIRECT}, 0x19},
    {{"CMP", IMMEDIATE}, 0x1A},
    {{"NOP", NOOP}, 0x1B},
    {{"JO", DIRECT}, 0x1C},
    {{"JNO", DIRECT}, 0x1D},
    {{"JZ", DIRECT}, 0x1E},
    {{"JE", DIRECT}, 0x1E},
    {{"JNZ", DIRECT}, 0x1F},
    {{"JNE", DIRECT}, 0x1F},
    {{"JNAE", SIGNED}, 0x20},
    {{"JB", SIGNED}, 0x20},
    {{"JAE", SIGNED}, 0x21},
    {{"JNB", SIGNED}, 0x21},
    {{"JNA", SIGNED}, 0x22},
    {{"JBE", SIGNED}, 0x22},
    {{"JA", SIGNED}, 0x23},
    {{"JNBE", SIGNED}, 0x23},
    {{"JNAE", UNSIGNED}, 0x24},
    {{"JB", UNSIGNED}, 0x24},
    {{"JAE", UNSIGNED}, 0x25},
    {{"JNB", UNSIGNED}, 0x25},
    {{"JBE", UNSIGNED}, 0x26},
    {{"JNA", UNSIGNED}, 0x26},
    {{"JA", UNSIGNED}, 0x27},
    {{"JNBE", UNSIGNED}, 0x27},
    {{"JS", DIRECT}, 0x28},
    {{"JNS", DIRECT}, 0x29},
    {{"JMP", INDIRECT}, 0x2A},
    {{"SHL", NOOP}, 0x2B},
    {{"SHR", SIGNED}, 0x2C},
    {{"SHR", UNSIGNED}, 0x2D},
    {{"ROL", NOOP}, 0x2E},
    {{"ROR", NOOP}, 0x2F},
    {{"IN", INDIRECT}, 0x30},
    {{"PUSH", NOOP}, 0x31},
    {{"POP", NOOP}, 0x32},
    {{"CALL", NOOP}, 0x33},
    {{"RET", NOOP}, 0x34},
    {{"INT", DIRECT}, 0x35},
    {{"IRET", NOOP}, 0x36},
    {{"RETI", NOOP}, 0x36},
    {{"STSP", DIRECT}, 0x37},
    {{"OUT", INDIRECT}, 0x38},
    {{"LDSPL", NOOP}, 0x39},
    {{"RCL", NOOP}, 0x3A},
    {{"RCR", NOOP}, 0x3B},
    {{"CMP", INDIRECT}, 0x3C},
    {{"IN", DIRECT}, 0x3D},
    {{"OUT", DIRECT}, 0x3E},
    {{"LDSPH", NOOP}, 0x3F},
};

// Instruction identifiers
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
                std::cerr << "[Error] Found string with no closing quotes (\")" << std::endl;
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
                std::cerr << "[Error] Invalid number at line " << line << ", position " << col << std::endl;
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
                    tokens.push_back({TK_ENDLINE, "\n", line, col});
                    advance();
                }else if(c == '\"'){
                    // String
                    advance();
                    tokens.push_back(read_string());
                }else if(c == '#'){
                    // Immediate value
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
                }else if(c == '+' || c == '-' || c == '*' || c == '/'){
                    // Arithmetic operator
                    tokens.push_back({TK_ARITHMETIC_OP, std::string(1, c), line, col});
                    advance();
                }else{
                    std::cerr << "[Error] Unexpected character: " << c << " on line " << line << ", position " << col << std::endl;
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

                switch(token.type){
                    case TK_ARITHMETIC_OP:
                        std::cout << "TK_ARITHMETIC_OP:";
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
enum kind_t {NODE_INSTRUCTION, NODE_DIRECTIVE, NODE_LABEL};
struct irnode_t{
    kind_t kind;
    std::string mnemonic;
    addr_t addressing;
    std::string operand;
    size_t size;
};

class Parser{
    private:
        std::vector<token_t> tokens;
        size_t pos;
        size_t location_counter;

    public:
        Parser(std::vector<token_t> tokens):tokens(tokens), pos(0), location_counter(0){}

        void parse(){
            for(size_t i = 0; i < tokens.size(); i++){
                token_t token = tokens[i];

                switch(token.type){
                    case TK_DIRECTIVE:
                        if(token.value == "equ"){
                            
                        }
                    break;
                    case TK_IDENTIFIER:

                    break;
                    case TK_ENDLINE:

                    break;
                    case TK_LABEL:

                    break;
                    case TK_END:
                        // Do nothing, it's the end of the file
                    break;
                    default:
                        std::cerr << "[Error] Unexpected token \"" << token.value << "\" at line " << token.line << ", column " << token.column << std::endl;
                    break;
                }
            }
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
        std::cerr << "[Error] Cannot open output file: " << output_filename << std::endl;
        return 1;
    }

    // Tokenize
    Lexer lexer(source_filename);
    std::vector<token_t> tokens = lexer.tokenize();
    lexer.print();

    // Parse
    Parser parser(tokens);
    parser.parse();

    // Assemble

    // Close files
    output_file.close();
}
#include <iostream>
#include <fstream>
#include <map>
#include <vector>
#include <cctype>
#include <stdexcept>
#include <cstdint>
#include <limits>
#include <algorithm>
#include "Lexer.hpp"


// ########################################################
// ##################### Tables ###########################
// ########################################################

std::map<std::string, size_t> symbol_table; // Stores position of the start of the expression, to be evaluated after all labels are calculated
std::map<std::string, uint16_t> label_table;

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


std::string to_upper(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c){ return std::toupper(c); });
    return s;
}

bool is_instruction(std::string value){
    value = to_upper(value);
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

void error_addressing(token_t t){
    std::cerr << "(" << t.line << ", " << t.column << ") [Parsing error]: Incorrect addressing for instruction " << t.value << " at line " << t.line << ", column " << t.column << std::endl;
    exit(1);
}

uint8_t get_opcode(std::string mnemonic, addr_t addressing){
    for(size_t i = 0; i < instructions.size(); i++){
        if(instructions[i].mnemonic == mnemonic && instructions[i].addressing == addressing){
            return instructions[i].opcode;
        }
    }
    // Not found
    error_addressing();
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
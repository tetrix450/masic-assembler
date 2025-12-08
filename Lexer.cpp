#include "Lexer.hpp"
#include <iostream>
#include <fstream>

Lexer::Lexer(std::string source_filename){
    // Load source file
    std::ifstream source_file(source_filename);
    if(!source_file.is_open()){
        std::cerr << "Cannot open source file: " << source_filename << std::endl;
        exit(1);
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
int Lexer::advance(){
    if(src[pos] == '\n'){
        col = 1;
        line++;
    }else{
        col++;
    }
    return pos++;
}
token_t Lexer::readIdentifierOrLabel(){
    size_t start = pos;
    size_t start_col = col;

    // Move cursor until end of alfanumeric word
    while(pos < src.size() && (isalnum(src[pos]) || src[pos] == '_' || src[pos] == '.')){
        advance();
    }

    // Get the identifier/label string
    std::string value = src.substr(start, pos - start);

    // It is a label
    if(pos < src.size() && src[pos] == ':'){
        advance();
        return {TK_LABEL, value, line, start_col};
    }

    // It is an identifier
    return {TK_IDENTIFIER, value, line, start_col};
}
token_t Lexer::readDirective(){
    size_t start = pos;
    size_t start_col = col;
    
    // Move cursor until end of alfanumeric word
    while(pos < src.size() && (isalnum(src[pos]) || src[pos] == '_')){
        advance();
    }

    // Get the directive string
    std::string value = src.substr(start, pos - start);

    // Return the token
    return {TK_DIRECTIVE, value, line, start_col};
}
token_t Lexer::readString(){
    size_t start = pos;
    size_t start_col = col;
    
    // Move cursor until \"
    while(pos < src.size() && src[pos] != '\"'){
        advance();
    }

    if(pos == src.size() - 1){
        std::cerr << "(" << line << ", " << col << ") [Lexical error] Found string with no closing quotes (\")" << std::endl;
        exit(1);
    }else{
        advance();
    }

    // Get the directive string
    std::string value = src.substr(start, pos - start - 1);

    return {TK_STRING, value, line, start_col};
}
token_t Lexer::readNumber(){
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
        std::cerr << "(" << line << ", " << col << ") [Lexical error] Invalid number" << std::endl;
        exit(1);
    }

    std::string value = src.substr(start, pos - start);
    if(negative) value = "-" + value;

    return {TK_NUMBER, value, line, start_col};
}
std::vector<token_t> Lexer::tokenize(){
    // Read character by character and tokenize all the program
    while(pos < src.size()){
        char c = src[pos];

        if(c == ' ' || c == '\t'){
            // Skip spaces or tabs
            advance();
        }else if (isdigit(c)) {
            // If it starts with a number or a sign followed by a number, then it's a number
            tokens.push_back(readNumber());
        }else if(isalnum(c) || c == '_'){
            // Identifier or label
            tokens.push_back(readIdentifierOrLabel());
        }else if(c == '.'){
            // Directive
            advance();
            tokens.push_back(readDirective());
        }else if(c == '\n'){
            // Endline
            tokens.push_back({TK_ENDLINE, "\\n", line, col});
            advance();
        }else if(c == '\"'){
            // String
            advance();
            tokens.push_back(readString());
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
        }else if(c == '<'){
            advance();
            if(c == '<'){
                tokens.push_back({TK_SHL, "<<", line, col});
                advance();
            }else{
                std::cerr << "(" << line << ", " << col << ") [Lexical error] Expected two consecutive '<<' instead of just one '<'" << std::endl;
                exit(1);
            }
        }else if(c == '>'){
            advance();
            if(c == '>'){
                tokens.push_back({TK_SHR, ">>", line, col});
                advance();
            }else{
                std::cerr << "(" << line << ", " << col << ") [Lexical error] Expected two consecutive '>>' instead of just one '>'" << std::endl;
                exit(1);
            }
        }else if(c == ','){
            tokens.push_back({TK_COMMA, ",", line, col});
            advance();
        }else if(c == '%'){
            tokens.push_back({TK_PERCENT, "%", line, col});
            advance();
        }else if(c == '&'){
            tokens.push_back({TK_AMPERSAND, "&", line, col});
            advance();
        }else if(c == '|'){
            tokens.push_back({TK_BAR, "|", line, col});
            advance();
        }else if(c == '^'){
            tokens.push_back({TK_CIRCUMFLEX, "^", line, col});
            advance();
        }else{
            std::cerr << "(" << line << ", " << col << ") [Lexical error] Unexpected character: " << c << std::endl;
            exit(1);
        }
    }

    tokens.push_back({TK_END, "END\n", line, col});
    return tokens;
}
void Lexer::print(){
    std::string token_name[] = {"IDENTIFIER", "NUMBER", "STRING", "DIRECTIVE", "LABEL", "LBRACKET", "RBRACKET",
    "ENDLINE", "PLUS", "MINUS", "MULTIPLY", "DIVIDE", "LPAREN", "RPAREN", "HASH", "END",
    "COMMA", "PERCENT", "INVALID"
    };

    for(size_t i = 0; i < tokens.size(); i++){
        token_t token = tokens[i];
        std::cout << "[" << i << "]->" << token_name[tokens[i].type] << ":" << token.value << " ";
        if(tokens[i].type == TK_ENDLINE){
            std::cout << std::endl;
        }
    }
}
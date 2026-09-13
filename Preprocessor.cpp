#include "Preprocessor.hpp"
#include "Lexer.hpp"

#include <iostream>
#include <cstdlib>

Preprocessor::Preprocessor(const std::vector<token_t>& tokens):tokens(tokens){

}

void Preprocessor::error(const token_t& token, const std::string& message){
    std::cerr
        << "("
        << token.line
        << ", "
        << token.column
        << ") [Preprocessor error]: "
        << message
        << std::endl;

    std::exit(1);
}

std::vector<token_t> Preprocessor::process(){
    std::vector<token_t> output;

    for(size_t i = 0; i < tokens.size(); i++){
        token_t token = tokens[i];

        if(token.type == TK_DIRECTIVE && token.value == "include"){
            // Comprobar que existe argumento
            if(i + 1 >= tokens.size()){
                error(token, "Expected filename after .include");
            }

            token_t filename_token = tokens[i + 1];

            if(filename_token.type != TK_STRING){
                error(filename_token, "Expected string after .include");
            }

            // Leer archivo incluido
            Lexer lexer(filename_token.value);

            std::vector<token_t> included_tokens = lexer.tokenize();

            // Procesar includes recursivamente
            Preprocessor pp(included_tokens);

            included_tokens = pp.process();
            
            // Extraer el TK_END
            if(!included_tokens.empty() && included_tokens.back().type == TK_END){
                included_tokens.pop_back();
            }

            // Insertar tokens procesados
            output.insert(
                output.end(),
                included_tokens.begin(),
                included_tokens.end()
            );

            // Saltar nombre del fichero
            i++;

            // Saltar posible fin de línea
            if(i + 1 < tokens.size() && tokens[i + 1].type == TK_ENDLINE){
                i++;
            }
        }else{
            output.push_back(token);
        }
    }

    return output;
}

#include <iostream>
#include <fstream>
#include <vector>
#include <cctype>
#include "DataStructures.hpp"
#include "Lexer.cpp"
#include "Parser.cpp"

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
    std::vector<uint8_t> bytes = parser.parse();

    // Assemble
    for(size_t i = 0; i < bytes.size(); i++){
        char byte = bytes[i];
        output_file.write(&byte, 1);
    }

    std::cout << "[Success] " << source_filename << " assembled into " << output_filename << std::endl;

    // Close files
    output_file.close();
}
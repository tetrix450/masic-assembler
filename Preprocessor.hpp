#ifndef PREPROCESSOR_HPP
#define PREPROCESSOR_HPP

#include <vector>
#include <string>

#include "DataStructures.hpp"

class Preprocessor{
private:
    std::vector<token_t> tokens;

    void error(const token_t& token, const std::string& message);

public:
    Preprocessor(const std::vector<token_t>& tokens);

    std::vector<token_t> process();
};

#endif
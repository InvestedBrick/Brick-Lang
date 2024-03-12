#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include "headers/preprocessor.hpp"
#include "headers/generation.hpp"
//compile with "g++ preprocessor.cpp tokenization.cpp parsing.cpp generation.cpp main.cpp -std=c++17 -o brick.exe"
/*
    1. Make it work! (not optional)
    2. Make it beautiful (optional)
    3. Make it efficient (optional)
    LOGS:
    21.01.24: about 1800 Lines
    23.01.24: about 1940 lines
    30.01.24: about 2060 lines
    02.02.24: about 2200 lines
    08.02.24: about 2400 lines
    13.02.24: about 2700 lines (with moving to cpp)
    15.02.24: about 2900 lines
    27.02.24: about 3030 lines (got qsort sort working!!)
    11.03.24: about 3200 lines (now with includes)
    Needs to be refactored
    
    checklist:
    [X] Functions and return values
    [X] support negative numbers
    [X] Arrays (add as function argument --> doable with start ptr and length)
    [X] pointers
    [X] importing other files 
    [ ] logical operators for control statements
    [ ] structs / larger data types??
    [ ] change stringbuffers??
    [ ] floating points
    [ ] dynamic memory
*/
int main(int argc, char* argv[]) {
    if (argc != 2)
    {
        std::cerr << "Invalid amount of inputs, Correct Usage:" << std::endl;
        std::cerr << "brick <input.brick>" << std::endl;
        return 1;
    }
    std::string filename = argv[1];
    if (filename.substr(filename.find_last_of(".") + 1) != "brick") {
        std::cerr << "Invalid input file, has to have .brick extension!" << std::endl;
        return 1;
    }
    {
        std::ifstream f(argv[1]);
        if (!f.good()) {
            std::cerr << "Input file was not found!" << std::endl;
            return 1;
        }
    }
    
    std::string contents;
    {
        std::stringstream contents_stream;
        std::fstream input(argv[1], std::ios::in);
        contents_stream << input.rdbuf();//read the file to a string
        contents = contents_stream.str();
    }
    PreProcessor preprocessor(std::move(contents), argv[1]);
    std::string preprocessed_contents = preprocessor.pre_process();
    //std::cout << preprocessed_contents << std::endl;
    Tokenizer tokenizer(std::move(preprocessed_contents));
    std::vector<Token> tokens = tokenizer.tokenize();
    Parser parser(std::move(tokens));
    std::optional<node::_program> prog = parser.parse_program();
    if (!prog.has_value()) {
        std::cerr << "Invalid program" << std::endl;
        exit(1);
    }
    Generator generator(prog.value());

    std::stringstream output_filename;
    {
        std::fstream output;
        output_filename << filename.substr(0, filename.find_last_of(".")) << ".asm";
        output.open(output_filename.str(), std::ios::out);
        output << generator.gen_program();
    }
    std::string assemble_command = "C:\\masm32\\bin\\ml.exe /c /coff " + output_filename.str();
    std::string link_command = "C:\\masm32\\bin\\link.exe /subsystem:console /entry:_main " +
        filename.substr(0, filename.find_last_of(".")) + ".obj";

    system(assemble_command.c_str());
    system(link_command.c_str());
    //std::cout << assemble_command.c_str() << std::endl << link_command.c_str() << std::endl;
    return 0;
}
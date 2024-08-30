/*
The Compiler for the Brick Programming Language
    Copyright (C) 2024  Julian Brecht

    reachable at julianbrecht25@gmail.com

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.


*/

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <chrono>

#ifdef __linux__
#include "headers/optimizer.hpp"
#endif

#include "headers/preprocessor.hpp"
#include "headers/generation.hpp"
bool output_info = false;
#ifdef _WIN32
std::string nologo = "";
#endif
#ifdef __linux__
int optimization_level = 0;
#endif

bool handle_if_is_arg(std::string arg){
    if(arg == "-info"){
        output_info = true;
        return true;
    }
#ifdef _WIN32
    else if(arg == "-nomicrosoft"){
        nologo = "/nologo ";
        return true;
    }
#endif    
#ifdef __linux__
    else if(arg == "-O1"){
        optimization_level = 1;
        return true;
    }
#endif

    return false;

}
    
int main(int argc, char* argv[]) {

    auto start_time = std::chrono::high_resolution_clock::now();
    std::cout << "        The Brick-Lang Compiler    " << std::endl;
    std::cout << "    Copyright (C) 2024  Julian Brecht" << std::endl;
    std::cout << std::endl;
    if (argc < 2)
    {
        std::cerr << "Invalid amount of inputs, Correct Usage:" << std::endl;
        std::cerr << "brick <input.brick>" << std::endl;
        exit(EXIT_FAILURE);
    }
    int x = 2;
    std::string filename = argv[1];
    if (filename.substr(filename.find_last_of(".") + 1) != "brick") {
        std::cerr << "Invalid input file, has to have .brick extension!" << std::endl;
        return 1;
    }
    {
        std::ifstream f(argv[1]);
        if (!f.good()) {
            std::cerr << "Input file was not found!" << std::endl;
            exit(EXIT_FAILURE);
        }
    }
    if(argc > 2){
        for(int i = 2; i < argc; i++){
            if(!handle_if_is_arg(argv[i])){
                std::cerr << "Invalid Argment '" << argv[i] << "' was supplied" << std::endl;
                exit(EXIT_FAILURE);
            }
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
    if (output_info)
        std::cout << "Finished Preprocessing..." << std::endl;
    //std::cout << preprocessed_contents << std::endl;
    Tokenizer tokenizer(std::move(preprocessed_contents));
    std::vector<Token> tokens = tokenizer.tokenize();
    if (output_info)
        std::cout << "Finished Tokenizing..." << std::endl;
    Parser parser(std::move(tokens));
    std::optional<node::_program> prog = parser.parse_program();
    if (output_info)
        std::cout << "Finished Parsing..." << std::endl;
    if (!prog.has_value()) {
        std::cerr << "Invalid program" << std::endl;
        exit(EXIT_FAILURE);
    }
    Generator generator(prog.value());
#ifdef __linux__
    Optimizer optimizer(generator.gen_program(),optimization_level);
    if (output_info)
            std::cout << "Finished Generating..." << std::endl;
#endif

    std::stringstream output_filename;
    {
        std::fstream output;
        output_filename << filename.substr(0, filename.find_last_of(".")) << ".asm";
        output.open(output_filename.str(), std::ios::out);
#ifdef __Win32__
        output << generator.gen_program();
        if (output_info)
            std::cout << "Finished Generating..." << std::endl;
#elif __linux__
        output << optimizer.optimize();
        if (output_info)
            std::cout << "Finished Optimizing at level " << optimization_level << "..." << std::endl;
#endif
        output.close();
    }
    auto stop_time = std::chrono::high_resolution_clock::now();
    auto compile_time = std::chrono::duration_cast<std::chrono::milliseconds>(stop_time - start_time);
    std::cout << "Compilation finished successfully in " << compile_time.count() << "ms " << std::endl;

    std::string assemble_command, link_command;
#ifdef _WIN32    
    assemble_command = "C:\\masm32\\bin\\ml.exe /c /coff " + nologo + output_filename.str();
    link_command = "C:\\masm32\\bin\\link.exe /subsystem:console /entry:_main " + nologo + \
    filename.substr(0, filename.find_last_of(".")) + ".obj";

    system(assemble_command.c_str());
    system(link_command.c_str());
#elif __linux__
    assemble_command =  "nasm -f elf32 -o " + filename.substr(0,filename.find_last_of(".") ) + ".o"+ " " + output_filename.str();
    link_command = "ld -m elf_i386 -o " + filename.substr(0,filename.find_last_of(".")) + " " + filename.substr(0,filename.find_last_of(".") ) + ".o";
    
    //ret_val to stop O2 optimizer from complaining
    int ret_val = system(assemble_command.c_str());
    ret_val = system(link_command.c_str());
#endif
    return 0;
}
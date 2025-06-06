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
bool assemble_and_link = true;
bool link = true;
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
    else if (arg == "-S"){
        assemble_and_link = false;
        return true;
    }
    else if (arg == "-c"){
        link = false;
        return true;
    }
    else if(arg == "-h") {
        std::cout << "Usage: brick [options] file" << std::endl;
        std::cout << "Options: \n" \
                  << "   -info           Show compiling info, mainly debugging purposes\n"
                  << "   -S              Compile only; do not assemble or link\n"
                  << "   -c              Compile and assemble, but do not link\n"
                  << "   -h              Show this message\n"
                  << "   -----WINDOWS EXCLUSIVE-----\n" 
                  << "   -nomicrosoft    disable microsoft copyright notice when assembling\n"
                  << "\n"
                  << "   -----LINUX EXCLUSIVE-----\n" 
                  << "   -O1             Strip assembly of unused functions\n"
                  << "   -O2             Perform O1 and make some slight optimizations" << std::endl;
        exit(EXIT_SUCCESS);
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
    else if(arg == "-O2"){
        optimization_level = 2;
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
        std::cerr << "Type 'brick -h' for more help!" << std::endl;
        exit(EXIT_FAILURE);
    }
    if(argc >= 2){
        int i = 1;
        // never would I have thought to actually use do-while
        do{
            if(!handle_if_is_arg(argv[i])){
                std::cerr << "Invalid Argment '" << argv[i] << "' was supplied" << std::endl;
                exit(EXIT_FAILURE);
            }
            i++;
        } while ( i < argc - 1);
    }
    std::string filename = argv[argc - 1];
    if (filename.substr(filename.find_last_of(".") + 1) != "brick") {
        std::cerr << "Invalid input file, has to have .brick extension!" << std::endl;
        return EXIT_FAILURE;
    }
    {
        std::ifstream f(filename);
        if (!f.good()) {
            std::cerr << "Input file was not found!" << std::endl;
            exit(EXIT_FAILURE);
        }
    }
    
    std::string contents;
    {
        std::stringstream contents_stream;
        std::fstream input(filename, std::ios::in);
        contents_stream << input.rdbuf();//read the file to a string
        contents = contents_stream.str();
    }
    PreProcessor preprocessor(std::move(contents), filename);
    std::string preprocessed_contents = preprocessor.pre_process();
    if (output_info)
        std::cout << "Finished Preprocessing..." << std::endl;
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
        if (output_info && optimization_level > 0)
            std::cout << "Finished Optimizing at level " << optimization_level << "..." << std::endl;
#endif
        output.close();
    }
    auto stop_time = std::chrono::high_resolution_clock::now();
    auto compile_time = std::chrono::duration_cast<std::chrono::milliseconds>(stop_time - start_time);
    std::cout << "Compilation finished successfully in " << compile_time.count() << "ms " << std::endl;

    if (assemble_and_link){

        std::string assemble_command, link_command;
        #ifdef _WIN32    
        assemble_command = "C:\\masm32\\bin\\ml.exe /c /coff " + nologo + output_filename.str();
        link_command = "C:\\masm32\\bin\\link.exe /subsystem:console /entry:_main " + nologo + \
        filename.substr(0, filename.find_last_of(".")) + ".obj";
        
        #elif __linux__
        assemble_command =  "nasm -f elf32 -o " + filename.substr(0,filename.find_last_of(".") ) + ".o"+ " " + output_filename.str();
        link_command = "ld -m elf_i386 -o " + filename.substr(0,filename.find_last_of(".")) + " " + filename.substr(0,filename.find_last_of(".") ) + ".o";
        
        //ret_val to stop O2 optimizer from complaining
        #endif
        int ret_val = system(assemble_command.c_str());
        if (link){
            ret_val = system(link_command.c_str());
        }
    }
    return 0;
}
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include "headers/preprocessor.hpp"
#include "headers/generation.hpp"
bool output_info = true;
#ifdef _WIN32



bool is_arg(std::string arg){
    if(arg == "-noinfo"){
        output_info = false;
        return true;
    }else if(arg == "-nomicrosoft"){
        nologo = "/nologo ";
        return true;
    }
    return false;

}

void parse_commandline_args(int argc, char* argv[]){
    if(argc > 2){
        for(int i = 2; i < argc; i++){
            if(!is_arg(argv[i])){
                std::cerr << "Invalid Argment '" << argv[i] << "' was supplied" << std::endl;
                return 1; 
            }
        }
    }
}
    
#endif
int main(int argc, char* argv[]) {
    if (argc < 2)
    {
        std::cerr << "Invalid amount of inputs, Correct Usage:" << std::endl;
        std::cerr << "brick <input.brick>" << std::endl;
        return 1;
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
            return 1;
        }
    }
    #ifdef _WIN32
    parse_commandline_arguments(argc,argv)
    #endif
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
        exit(1);
    }
    Generator generator(prog.value());

    std::stringstream output_filename;
    {
        std::fstream output;
        output_filename << filename.substr(0, filename.find_last_of(".")) << ".asm";
        output.open(output_filename.str(), std::ios::out);
        output << generator.gen_program();
        if (output_info)
            std::cout << "Finished Generating..." << std::endl;
        output.close();
    }
    std::string assemble_command, link_command;
#ifdef _WIN32    
    assemble_command = "C:\\masm32\\bin\\ml.exe /c /coff " + nologo + output_filename.str();
    link_command = "C:\\masm32\\bin\\link.exe /subsystem:console /entry:_main " + nologo + \
    filename.substr(0, filename.find_last_of(".")) + ".obj";

    system(assemble_command.c_str());
    system(link_command.c_str());
#elif __linux__
    assemble_command =  "nasm -f elf64 -o " + filename.substr(0,filename.find_last_of(".") ) + ".o"+ " " + output_filename.str();
    link_command = "ld -o " + filename.substr(0,filename.find_last_of(".")) + " " + filename.substr(0,filename.find_last_of(".") ) + ".o";
    
    
    //NOT CURRENTLY IMPLEMENTED FOR LINUX
    //system(assemble_command.c_str());
    //system(link_command.c_str());
#endif
    return 0;
}
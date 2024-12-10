/*
    The Optimizer for the Brick Programming Language
    Copyright (C) 2024  Julian Brecht

    view main.cpp for more copyright related information

*/

#define SIZE_OF_SECTION_DOT_TEXT 13

#include "headers/optimizer.hpp"
#include <iostream>
#include <unordered_set>
#include <string>
bool is_numeric(const std::string& str) {
    for (char c : str) {
        if (!std::isdigit(c)) {
            return false;
        }
    }
    return true;
}
bool is_there_a_space_in_there(std::string& str){
    for (char c : str) {
        if (c == ' ') {
            return true;
        }
    }
    return false;
}
inline std::optional<char> Optimizer::peek(int offset) {
    if (this->m_idx + offset >= this->m_asm_code.length()) {
        return std::nullopt;
    }
    else {
        return this->m_asm_code.at(this->m_idx + offset);
    }
}

void Optimizer::peek_window(int n){
    std::cout << "|---------------------------------------------------------------------|" << std::endl;
    if(this->m_idx + n >= this->m_asm_code.length()){
        n = this->m_asm_code.length() - this->m_idx - 1;
    }
    std::cout << this->m_asm_code.substr(this->m_idx,n) << std::endl;
    std::cout << "|---------------------------------------------------------------------|" << std::endl;
}

inline char Optimizer::consume() {
    return this->m_asm_code.at(this->m_idx++);
}

std::string Optimizer::optimize()
{

    if (this->op_level >= 1){
        this->rem_unused_funcs();
    }

    return this->m_asm_code;

}
inline void Optimizer::jmp_spaces(){
    while (peek().has_value() && peek().value() == ' ')
    {
        consume();
    }
}
inline std::string Optimizer::consume_until_char_and_consume_char(char c){
    std::string str{};
    while (peek().has_value() && peek().value() != c)
    {
        str.push_back(peek().value());
    }
    consume();
    return str;
    
}
bool is_register(std::string& str){
    static const std::unordered_set<std::string> registers = {
        "eax", "ebx", "ecx", "edx", "esi", "edi", "esp", "ebp", // 32-bit
        "ax", "bx", "cx", "dx", "si", "di", "sp", "bp",         // 16-bit
        "al", "ah", "bl", "bh", "cl", "ch", "dl", "dh"          // 8-bit
    };
    return registers.find(str) != registers.end();
}



inline void Optimizer::tokenize_asm()
{
    std::string buf{};
    this->m_idx = this->m_asm_code.find("section .text");
    this->m_idx += SIZE_OF_SECTION_DOT_TEXT;

    while(peek().has_value()){
        while(peek().has_value() && (peek().value() == ' ' || peek().value() == '\n')){
            consume();
        }
        while(peek().has_value() && (peek().value() != ' ' || peek().value() == ',' || peek().value() == '\n')){
            buf.push_back(consume());
        }
        Operation op;
        op.idx = this->m_idx - buf.size();
        consume(); // space
        if (buf[buf.size() - 1] == ':'){ // Label
            continue; // we currently do not care
        }
        else if (buf == "mov"){
            op.op_type = OpType::_mov;
            
            op.op_1 = consume_until_char_and_consume_char(',');
            jmp_spaces();
            op.op_2 = consume_until_char_and_consume_char('\n');
            op.operand_1 = is_register(op.op_1.value()) ? OperandType::_register : OperandType::_data_offset;
            op.operand_2 = is_register(op.op_2.value()) ? OperandType::_register : 
                             is_numeric(op.op_2.value()) ? OperandType::_int_lit : 
                             is_there_a_space_in_there(op.op_2.value()) ? OperandType::_data_offset : OperandType::_byte_ref;
            operations.push_back(op);
            continue;
        }
        else if (buf == "lea"){
            op.op_type = OpType::_lea;
            op.op_1 = consume_until_char_and_consume_char(',');
            jmp_spaces();
            op.op_2 = consume_until_char_and_consume_char('\n');
            op.operand_1 = is_register(op.op_1.value()) ? OperandType::_register : OperandType::_data_offset;
            op.operand_2 = OperandType::_data_offset; 
            operations.push_back(op);
            continue;
        }
        else if (buf == "push"){
            op.op_type = OpType::_push;
            op.op_1 = consume_until_char_and_consume_char('\n');
            op.operand_1 = OperandType::_register; // we trust the programmer to not push numbers to the stack with inline assembly
            operations.push_back(op);
            continue;
        }
        else if (buf == "pop"){
            op.op_type = OpType::_pop;
            op.op_1 = consume_until_char_and_consume_char('\n');
            op.operand_1 = OperandType::_register;
            operations.push_back(op);
            continue;
        }
        else if (buf == "ret"){
            op.op_type = OpType::_ret;
            consume(); // newline after the space
            operations.push_back(op);
            continue;
        }
        else if (buf == "add" || buf == "sub"){
            op.op_type = buf == "add" ? OpType::_add : OpType::_sub;
            op.op_1 = consume_until_char_and_consume_char(',');
            jmp_spaces();
            op.op_2 = consume_until_char_and_consume_char('\n');
            op.operand_1 = is_register(op.op_1.value()) ? OperandType::_register : OperandType::_data_offset;
            op.operand_2 = is_register(op.op_2.value()) ? OperandType::_register : 
                             is_numeric(op.op_2.value()) ? OperandType::_int_lit : OperandType::_data_offset; 
            operations.push_back(op);
            continue;
        }



    }

}

inline void Optimizer::index_functions()
{
    size_t ret_idx;
    this->m_idx = this->m_asm_code.find("section .text");
    this->m_idx += SIZE_OF_SECTION_DOT_TEXT;

    while (true){
        this->m_idx++; // newline 
        function_info f_inf;
        while (peek().has_value() && peek().value() != ':'){
            f_inf.name += consume();
        }
        this->m_idx++;
        f_inf.start_idx = this->m_idx;

        size_t ret_idx = this->m_asm_code.find("ret ",this->m_idx,4);

        if (ret_idx == std::string::npos)
            break;

        if (ret_idx + 5 >= this->m_asm_code.length()){
            f_inf.end_idx = this->m_asm_code.length();
        }else{
            f_inf.end_idx = ret_idx + 5;
        }
        this->m_idx = f_inf.end_idx;
        this->func_infos.push_back(f_inf);
    }

}

inline void Optimizer::mark_funcs()
{
    this->func_infos[0].used = true; // _start
    this->m_idx = this->func_infos[0].start_idx + this->func_infos[0].name.length() + 2; // +2 bc of ':' and \n
    size_t start = this->func_infos[0].start_idx;
    size_t end = this->func_infos[0].end_idx;
    while(true){
        
        this->m_idx = this->m_asm_code.find("call",start);

        if (this->m_idx == std::string::npos || this->m_idx >= end){
            if (call_indices.empty()){
                break;
            }

            idx_pair p = call_indices.back();
            call_indices.pop_back();

            this->m_idx = p.idx;
            start = this->m_idx;
            end = p.func_end;
            continue;
        }

        this->m_idx += 5; // mov to idx && += "all" + space
        std::string buf;
        while(peek().has_value() && peek().value() != '\n'){
            buf += consume();
        }
        this->m_idx++;
        auto it = std::find_if(this->func_infos.begin(),this->func_infos.end(), [&buf](const function_info f){return f.name == buf;});
        //should be guaranteed to be safe
        if (it == this->func_infos.end()){
            std::cerr << "This should never happen" << std::endl;
            exit(EXIT_FAILURE);
        }
        idx_pair p;
        p.idx = this->m_idx;
        p.func_end = end;
        this->call_indices.push_back(p);


        this->m_idx = (*it).start_idx;
        start = (*it).start_idx;
        end = (*it).end_idx;
        (*it).used = true;

    }
}

inline void Optimizer::rem_unused_funcs()
{
    this->index_functions();

    this->mark_funcs();

    for (size_t i = this->func_infos.size(); i > 0; i--){
        size_t idx = i - 1;
        if (!func_infos[idx].used)
        {
            this->m_asm_code.erase(func_infos[idx].start_idx - (func_infos[idx].name.length() + 2),func_infos[idx].end_idx - func_infos[idx].start_idx + (func_infos[idx].name.length() + 2));
        }
    }

}
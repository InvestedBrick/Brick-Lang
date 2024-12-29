/*
    The Optimizer for the Brick Programming Language
    Copyright (C) 2024  Julian Brecht

    view main.cpp for more copyright related information

*/

#define SIZE_OF_SECTION_DOT_TEXT 13

#include "headers/optimizer.hpp"
#include <iostream>
#include <unordered_set>
#include <sstream>
#include <string>
bool _is_numeric(const std::string& str) {
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
    if (this->op_level >= 2){
        this->tokenize_asm();
        this->optimize_tokens();
        this->reassemble_asm();
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
    bool closed_bracket = false;
    while (peek().has_value() && peek().value() != c)
    {
        char val = peek().value();
        if (c == '\n' && val == ' ' && closed_bracket ){ // there is space before the newline
            while (peek().value() != '\n'){ // after that, remove EVERYTHING
                consume();
            }
            break;
        }
        if (val == ']'){
            closed_bracket = true;
        }
        str.push_back(val);
        consume();
    }
    consume();
    return str;
    
}
bool is_register(const std::string& str){
    static const std::unordered_set<std::string> registers = {
        "eax", "ebx", "ecx", "edx", "esi", "edi", "esp", "ebp", // 32-bit
        "ax", "bx", "cx", "dx", "si", "di", "sp", "bp",         // 16-bit
        "al", "ah", "bl", "bh", "cl", "ch", "dl", "dh"          // 8-bit
    };
    return registers.find(str) != registers.end();
}
bool is_jmp_instruc(std::string& str){
    static const std::unordered_set<std::string> jmps = {
        "jmp", "je", "jne", "jg", "jge", "jl", "jle", "jz", "jnz", "js","jns"
    };
    return jmps.find(str) != jmps.end();
}

inline void Optimizer::reassemble_asm(){
    this->m_idx = this->m_asm_code.find("section .text");
    this->m_idx += SIZE_OF_SECTION_DOT_TEXT;
    std::stringstream code;
    code << this->m_asm_code.substr(0,m_idx);
    code << "\n";
    for(const Operation& op : operations){
        if (!op.erased){
            if(op.op_type == OpType::_label){
                code << op.op_1.value() << ":\n";
                continue;
            }
            if (op.op_type == OpType::_jmp){
                code << "    " << op.op_2.value(); // the saved original jmp instruction
            }else{
                code << "    " << opTypeToString(op.op_type);
            }
            if (op.op_1.has_value()){
                code << " " << op.op_1.value();
                if (op.op_2.has_value() && op.op_type != OpType::_jmp){
                    code << ", " << op.op_2.value();
                }
            }
            if(op.op_type == OpType::_ret){
                code << "\n";
            }
            code << "\n";
        }else{
            std::cout << "Erased Operation: " << opTypeToString(op.op_type) << " " << op.op_1.value() << ", " << op.op_2.value() << std::endl; 
        }
    }
    this->m_asm_code = code.str();
}

inline void Optimizer::optimize_tokens()
{
    for (size_t i = 0; i < operations.size();i++){
        Operation& op = operations[i];
        switch (op.op_type)
        {
        case OpType::_mov:{
            if (op.operand_1 == OperandType::_register && op.operand_2 != OperandType::_data_offset){ // if we have a data offset, we might try to move data to data
                size_t j = i + 1;
                // Move along the code and replace all instances of op.op1 in operand2 with op.op2
                while (j < operations.size() && operations[j].operand_1 != op.operand_1 && operations[j].op_type != OpType::_ret){
                    if(operations[j].op_2.has_value() && operations[j].op_2.value() == op.op_1.value()){
                        operations[j].op_2 = op.op_2.value();
                        op.erased = true;           
                        operations[j].operand_2 == op.operand_2;
                    }
                    j++;
                }
            }
            break;
        }
        
        }      
    }
}


inline void Optimizer::tokenize_asm()
{
    std::string buf{};
    this->m_idx = this->m_asm_code.find("section .text");
    this->m_idx += SIZE_OF_SECTION_DOT_TEXT;
    
    while(peek().has_value()){
        buf.clear();
        while(peek().has_value() && (peek().value() == ' ' || peek().value() == '\n')){
            consume();
        }
        while(peek().has_value() && peek().value() != ' ' && peek().value() != '\n'){
            buf.push_back(consume());
        }
        Operation op;
        op.idx = this->m_idx - buf.size();
        if (this->m_idx < m_asm_code.length())
            consume(); // space
        std::cout << buf << std::endl;
        if (buf[buf.size() - 1] == ':'){ // Label
            op.op_type = OpType::_label;
            op.operand_1 = OperandType::_label;
            op.op_1 = buf.substr(0,buf.size() - 1);
            operations.push_back(op);
            continue;
        }
        else if (buf == "mov" || buf == "movsx" || buf == "movzx"){
            op.op_type = buf == "mov" ? OpType::_mov : buf == "movsx" ? OpType::_movsx : OpType::_movzx;
            
            op.op_1 = consume_until_char_and_consume_char(',');
            jmp_spaces();
            op.op_2 = consume_until_char_and_consume_char('\n');
            op.operand_1 = is_register(op.op_1.value()) ? OperandType::_register : OperandType::_data_offset;
            op.operand_2 = is_register(op.op_2.value()) ? OperandType::_register : 
                             _is_numeric(op.op_2.value()) ? OperandType::_int_lit : 
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
                             _is_numeric(op.op_2.value()) ? OperandType::_int_lit : OperandType::_data_offset; 
            operations.push_back(op);
            continue;
        }
        else if (buf == "mul" || buf == "div"){
            op.op_type = buf == "mul" ? OpType::_mul : OpType::_div;
            op.op_1 = consume_until_char_and_consume_char('\n');
            op.operand_1 = is_register(op.op_1.value()) ? OperandType::_register : OperandType::_data_offset;
            operations.push_back(op);
            continue;
        }
        else if(buf == "call" || is_jmp_instruc(buf)){
            op.op_type = is_jmp_instruc(buf) ? OpType::_jmp : OpType::_call;
            op.op_1 = consume_until_char_and_consume_char('\n');
            op.operand_1 = OperandType::_label;
            if (op.op_type == OpType::_jmp){
                op.op_2 = buf; // we save the original jump isntruction
            }
            //std::cout << "op1 at call: '" << op.op_1.value() << "'" << std::endl;
            operations.push_back(op);
            continue;
        }
        else if (buf == "int"){
            op.op_type = OpType::_int;
            op.op_1 = consume_until_char_and_consume_char('\n');
            op.operand_1 = OperandType::_int_lit;
            operations.push_back(op);
            continue;
        }
        else if (buf == "cmp" || buf == "test"){
            op.op_type = buf == "cmp" ? OpType::_cmp : OpType::_test;
            op.op_1 = consume_until_char_and_consume_char(',');
            jmp_spaces();
            op.op_2 = consume_until_char_and_consume_char('\n');

            op.operand_1 = is_register(op.op_1.value()) ? OperandType::_register : OperandType::_data_offset;
            op.operand_2 = is_register(op.op_2.value()) ? OperandType::_register : _is_numeric(op.op_2.value()) ? OperandType::_int_lit : OperandType::_data_offset;

            operations.push_back(op);
            continue;
        }
        else if (buf == "or" || buf == "xor" || buf == "and"){
            op.op_type = buf == "or" ? OpType::_or : 
                        buf == "xor" ? OpType::_xor : OpType::_and; 

            op.op_1 = consume_until_char_and_consume_char(',');
            jmp_spaces();
            op.op_2 = consume_until_char_and_consume_char('\n');

            op.operand_1 = is_register(op.op_1.value()) ? OperandType::_register : _is_numeric(op.op_1.value()) ? OperandType::_int_lit : OperandType::_data_offset;
            op.operand_2 = is_register(op.op_2.value()) ? OperandType::_register : _is_numeric(op.op_2.value()) ? OperandType::_int_lit : OperandType::_data_offset;

            operations.push_back(op);
            continue;
        }
        else if (buf == "dec" || buf == "inc" || buf == "neg" || buf == "not"){
            op.op_type = buf == "dec" ?  OpType::_dec :
                         buf == "inc" ? OpType::_inc :
                         buf == "neg" ? OpType::_neg : OpType::_not;

            op.op_1 = consume_until_char_and_consume_char('\n');

            op.operand_1 = is_register(op.op_1.value()) ?  OperandType::_register : OperandType::_data_offset;

            operations.push_back(op);
            continue;
        }
        else if (buf == "shl" || buf == "shr"){
            op.op_type = buf == "shl" ?  OpType::_shl : OpType::_shr;

            op.op_1 = consume_until_char_and_consume_char(',');
            jmp_spaces();
            op.op_2 = consume_until_char_and_consume_char('\n');

            op.operand_1 = is_register(op.op_1.value()) ? OperandType::_register : _is_numeric(op.op_1.value()) ? OperandType::_int_lit : OperandType::_data_offset;
            op.operand_2 = is_register(op.op_2.value()) ? OperandType::_register : _is_numeric(op.op_2.value()) ? OperandType::_int_lit : OperandType::_data_offset;

            operations.push_back(op);
            continue;
        }
        else if (buf == ";"){
            consume_until_char_and_consume_char('\n');
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

        if (!(*it).used){
            this->m_idx = (*it).start_idx;
            start = (*it).start_idx;
            end = (*it).end_idx;
            (*it).used = true;
        }else{
            this->m_idx += 4;
            start = this->m_idx; // skip the "call"
        }

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
/*
    The Generator for the Brick Programming Language
    Copyright (C) 2024  Julian Brecht

    view main.cpp for more copyright related information

*/

#include "headers/generation.hpp"

#ifdef _WIN32
#define PTR_KEYWORD " ptr "
#elif __linux__
#define PTR_KEYWORD ""
#endif
#define EBP_OFF " [ebp - "

#define EDX_OFF " [edx - "

bool is_numeric(const std::string& str) {
    for (char c : str) {
        if (!std::isdigit(c)) {
            return false;
        }
    }
    return true;
}
template <typename T>
void Generator::transferElements(std::vector<T>&& source, std::vector<std::variant<Var, string_buffer, String, Var_array,Struct>>& destination, int count) {
    for(int i = count; i > 0; i--) {
        destination.push_back(std::move(source.back()));
        source.pop_back();
    }
}
inline void Generator::line_err(const std::string& err_msg) {
    std::cerr << "Code failed in line " << this->line_counter << "(" << this->filestack.back() << "): " << err_msg << std::endl;
    exit(EXIT_FAILURE);
}
inline std::string Generator::mk_label() {
    std::stringstream ss;
    ss << "LBL_" << label_counter++;
    return ss.str();
}
inline std::string Generator::mk_str_lit() {
    std::stringstream ss;
    ss << "string_lit_" << string_counter;
    if (this->emit_var_gen_asm){string_counter++;}
    return ss.str();
}
inline std::string Generator::mk_str_buf() {
    std::stringstream ss;
    ss << "string_buffer_" << string_buffer_counter;
    if (this->emit_var_gen_asm){string_counter++;}
    return ss.str();
}

inline size_t Generator::asm_type_to_bytes(std::string str) {
    return str_bit_sizes.find(str)->second >> 3; // fast div by 8
}
#ifdef __linux__
inline void Generator::sys_read(size_t size,std::string ecx_and_edi,bool lea_or_mov){
    this->m_code << "    mov eax, 3 ;sys-read" << std::endl;
    this->m_code << "    xor ebx, ebx" << std::endl;
    this->m_code << "    " << (lea_or_mov ? "lea " : "mov ") << " ecx, " << ecx_and_edi << std::endl;
    this->m_code << "    mov edx, " << size  << std::endl;
    this->m_code << "    int 0x80" << std::endl;
    this->m_code << "    cmp eax, " << size << std::endl;
    std::string no_need_to_replace_null = this->mk_label();
    this->m_code << "    jg " << no_need_to_replace_null << std::endl; 
    flags.needs_nl_replace_func = true;
    this->m_code << "    " << (lea_or_mov ? "lea " : "mov ") << " edi, " << ecx_and_edi << std::endl;
    this->m_code << "    call sys~internal_replace_nl_null" << std::endl;
    this->m_code << no_need_to_replace_null << ": " << std::endl;

}
#endif
inline std::string Generator::get_mov_instruc(const std::string& dest, const std::string& source) {
    const auto source_it = str_bit_sizes.find(source);
    const auto dest_it = str_bit_sizes.find(dest);
    if (source_it->second > dest_it->second) {
        line_err("Something's wrong I can feel it (how tf did this happen)");
        exit(EXIT_FAILURE); //to make g++ happy
    }
    else if (source_it->second < dest_it->second) {
        return "movzx";
    }
    else {
        return "mov";
    }

}
//clear labels
inline void Generator::reset_labels(){
    this->initial_label_and.reset();
    this->initial_label_or.reset();
    this->ending_label.reset();
}
inline std::string Generator::get_correct_part_of_register(const std::string& source,bool edx)
{
    const auto  source_it = str_bit_sizes.find(source);
    if (source_it->second == 32) {
        return edx ? "edx" : "eax";
    }
    else if (source_it->second == 16) {
        return edx ? "dx" : "ax";
    }
    else if (source_it->second == 8) {
        return edx ? "dl" : "al";
    }
    else
    {
        line_err("Weird Input was given, you fd up");
        exit(EXIT_FAILURE);
    }
}
inline void Generator::scope_start(bool main_or_func_scope, size_t alloc) {
    //init a new stackframe
    if (main_or_func_scope && alloc != 0) {
        this->m_base_ptr_pos.push_back(this->m_base_ptr_off);
        this->m_base_ptr_off = 0;
        m_code << "    push ebp" << std::endl;
        m_code << "    mov ebp,esp" << std::endl;
        m_code << "    sub esp," << alloc << std::endl;

    }
    //save states
    m_scope_arrays.push_back(m_arrays.size());
    m_scopes.push_back(m_vars.size());//sets the scope position to the num of variables

    m_scope_str_bufs.push_back(m_str_bufs.size());

}
inline void Generator::scope_end(bool main_or_func_scope, size_t alloc) {
    const size_t str_bufs_dec = m_str_bufs.size() - this->m_scope_str_bufs.back();
    for (auto it = m_str_bufs.end() - str_bufs_dec; it != m_str_bufs.end(); ++it) { //free all the string buffers declared in the scope
        (*it).free = true;
    }
    m_scope_str_bufs.pop_back();
    

    const size_t vars_dec = m_vars.size() - this->m_scopes.back();
    for (size_t i = 0; i < vars_dec; i++) {
        m_vars.pop_back();
    }
    
    const size_t arrays_dec = m_arrays.size() - this->m_scope_arrays.back();
    for (size_t i = 0; i < arrays_dec; i++) {
        m_arrays.pop_back();
    }
    m_scopes.pop_back();
    //pop the stackframe
    if (main_or_func_scope && alloc != 0) {
        this->m_base_ptr_off = this->m_base_ptr_pos.back();
        this->m_base_ptr_pos.pop_back();
        m_code << "    add esp," << alloc << std::endl;
        m_code << "    pop ebp" << std::endl;
    }
}
inline std::string Generator::gen_str_lit(const std::string string_lit) {
    const auto it = std::find_if(this->m_strs.cbegin(), this->m_strs.cend(), [&](const String& str1) {return str1.str_val == string_lit; });
    if (it == this->m_strs.cend()) { // string lit value was not already declared
        String str;
        std::string generated = mk_str_lit();
        if (this->emit_var_gen_asm){
            this->m_data << "    " << generated << " db \"" << string_lit << "\",0" << std::endl;
#ifdef __linux__        
            this->m_data << "    " << generated << "_len equ $ - " << generated << std::endl;
#endif        

        }
        str.generated = generated;
        str.str_val = string_lit;
        this->generating_struct_vars ? (*this->generic_struct_vars).push_back(str) : this->m_strs.push_back(str);
        return generated;
    }
    else {
        return (*it).generated;
    }

}
inline void Generator::gen_scope(const node::_statement_scope* scope) {
    scope_start();
    for (const node::_statement* stmt : scope->statements) {
        gen_stmt(stmt);
    }
    scope_end();
}
inline std::optional<std::string> Generator::tok_to_instruc(const Token_type type, bool invert) {

    switch(type){
        case Token_type::_same_as:       { return invert ? "    jne " : "    je ";  }
        case Token_type::_not_same_as:   { return invert ? "    je "  : "    jne "; }
        case Token_type::_greater_as:    { return invert ? "    jle " : "    jg ";  }
        case Token_type::_greater_eq_as: { return invert ? "    jl "  : "    jge "; }
        case Token_type::_less_as:       { return invert ? "    jge " : "    jl ";  }
        case Token_type::_less_eq_as:    { return invert ? "    jg "  : "    jle "; }
        default:                         {return {};}
    }
} 
template<typename iterator, typename struct_ident>
std::string Generator::gen_term_struct(iterator struct_it, struct_ident struct_ident_,std::string base_string){
    
    const std::string expected = struct_ident_->item->ident.value.value();
    //traverse the linked list of structs (for declaration in another struct)
    bool moved = false;
    if(struct_ident_->item->item != nullptr){
        struct_ident_->item = struct_ident_->item->item;
        moved = true;
    }

    //find the first element in the vars vector of the struct that has the name of our searched item and return an iterator to it
    const auto it = std::find_if((*struct_it).vars.cbegin(), (*struct_it).vars.cend(), [&](const auto& var) {
    return std::visit([&](const auto& element) { return element.name == expected; }, var);
    });
    
    if(it == (*struct_it).vars.cend()){
        std::stringstream ss;
        ss << "Item with the name '" << expected << "' was not found"; 
        this->line_err(ss.str());
    }

    std::stringstream offset;
    if(std::holds_alternative<Var>(*it)){
        Var var = std::get<Var>(*it);
        if(var.is_struct_ptr && moved){
            offset << this->gen_term_struct_ptr(&var,struct_ident_,base_string);
        }
        else if (var.ptr && struct_ident_->index_expr != nullptr){
            std::string val = this->gen_expr(struct_ident_->index_expr).value();
            this->m_code << "    mov edx, dword" << PTR_KEYWORD << base_string << var.base_pointer_offset << "]" << std::endl;

            if(is_numeric(val)){
                this->m_code << "    " << this->get_mov_instruc("eax",var.ptr_type) << " eax, " << var.ptr_type <<  PTR_KEYWORD << " [edx + " << std::stoi(val) * this->asm_type_to_bytes(var.ptr_type) << "]" << std::endl;
            }else{
                if (val != "eax"){
                    this->m_code << "    " << this->get_mov_instruc("eax",val.substr(val.find_first_of(" "))) << " eax, " << val << std::endl;
                }
                this->m_code << "    " << this->get_mov_instruc("eax",var.ptr_type) << " eax, " << var.ptr_type << PTR_KEYWORD << " [edx + eax * " << this->asm_type_to_bytes(var.ptr_type) << "]" << std::endl;
            }
            offset << "eax";
        }else{
            offset << var.type << PTR_KEYWORD << base_string << (var.base_pointer_offset) << "]";
        }
        return offset.str();

    }
    else if(std::holds_alternative<string_buffer>(*it)){
        string_buffer buf = std::get<string_buffer>(*it);
        offset << "\"" << buf.generated;
        return offset.str();
    }
    else if(std::holds_alternative<String>(*it)){
        String str = std::get<String>(*it);
        offset << "\"" << str.generated;
        return offset.str();
    }
    else if(std::holds_alternative<Var_array>(*it)){
        Var_array arr = std::get<Var_array>(*it);
        //repetitive code is okay here because another template is not worth it
        std::string val = this->gen_expr(struct_ident_->index_expr).value();
        if (is_numeric(val)) {
            offset << arr.type << PTR_KEYWORD << base_string << arr.head_base_pointer_offset - std::stoi(val) * this->asm_type_to_bytes(arr.type) << "] " ;
        }
        else {
            if (val != "eax") {
                this->m_code << "    " << this->get_mov_instruc("eax", val.substr(0, val.find_first_of(" "))) << " eax, " << val << std::endl;
            }
            offset << arr.type <<  PTR_KEYWORD << base_string << arr.head_base_pointer_offset << " + eax * " << this->asm_type_to_bytes(arr.type) << "]";
        }
        return offset.str();
    }
    else if(std::holds_alternative<Struct>(*it)){
        Struct struct_ = std::get<Struct>(*it);
        //recursively descend the structs
        return this->gen_term_struct(&struct_,struct_ident_,base_string);
    }
    this->line_err("Struct item generation failed");
    exit(EXIT_FAILURE);
}
size_t Generator::find_offset(const std::vector<std::pair<std::string, size_t>>& vec,std::string input_string){
    const auto it = std::find_if(vec.cbegin(),vec.cend(), [&input_string] (const std::pair<std::string,size_t>& pair){return pair.first == input_string;});
    if(it == vec.cend()){
        std::stringstream ss;
        ss << "Variable with name " << input_string << " was not found";
        this->line_err(ss.str());
    }

    return it->second;
}

template<typename iterator, typename struct_ptr_ident>
std::string Generator::gen_term_struct_ptr(iterator struct_ptr_it, struct_ptr_ident struct_ptr_ident_,std::string base_string,bool lea){
    std::string op = lea ? "lea" : "mov";
    this->m_code << "    "<<op<<" edx, dword" << PTR_KEYWORD << base_string << (*struct_ptr_it).base_pointer_offset << "]" << std::endl;
    const auto struct_info = std::find_if(this->m_struct_infos.cbegin(),this->m_struct_infos.cend(),
     [&](const Struct_info& info){return info.name == (*struct_ptr_it).ptr_type;});

    if (struct_info == this->m_struct_infos.cend()){
        std::stringstream ss;
        ss << "Struct with name " << (*struct_ptr_it).ptr_type << " not found";
        this->line_err(ss.str());
    }
    
    std::string item_name = struct_ptr_ident_->item->ident.value.value();
    bool moved = false;
    if (struct_ptr_ident_->item->item != nullptr){
        struct_ptr_ident_->item = struct_ptr_ident_->item->item;
        moved = true;
    }
    const auto meta = std::find_if(struct_info->var_metadatas.cbegin(), struct_info->var_metadatas.cend(),
        [&item_name](const node::_var_metadata& metadata) {
            return metadata.name == item_name;
        });
    std::stringstream offset;
    if ((*meta).variable_kind == "number"){
        Var var;
        var.base_pointer_offset = find_offset(struct_info->var_name_to_offset,(*meta).name);
        var.type = var_type_to_str((*meta).type);
        if (var.ptr && struct_ptr_ident_->index_expr != nullptr){
            var.ptr_type = var.type;
            var.type = "dword";
            std::string val = this->gen_expr(struct_ptr_ident_->index_expr).value();
            this->m_code << "    mov edx, dword" << PTR_KEYWORD << " [edx - " << var.base_pointer_offset << "]" << std::endl;

            if(is_numeric(val)){
                this->m_code << "    " << this->get_mov_instruc("eax",var.ptr_type) << " eax, " << var.ptr_type <<  PTR_KEYWORD << " [edx + " << std::stoi(val) * this->asm_type_to_bytes(var.ptr_type) << "]" << std::endl;
            }else{
                if (val != "eax"){
                    this->m_code << "    " << this->get_mov_instruc("eax",val.substr(val.find_first_of(" "))) << " eax, " << val << std::endl;
                }
                this->m_code << "    " << this->get_mov_instruc("eax",var.ptr_type) << " eax, " << var.ptr_type << PTR_KEYWORD << " [edx + eax * " << this->asm_type_to_bytes(var.ptr_type) << "]" << std::endl;
            }
            offset << "eax";
        }else{
            offset << var.type << PTR_KEYWORD << " [edx - " << (var.base_pointer_offset) << "]";
        }
        return offset.str();
    }    
    else if ((*meta).variable_kind == "array"){
        Var_array arr;
        arr.size = (*meta)._array_size;
        arr.type = var_type_to_str((*meta).type);
        arr.head_base_pointer_offset = find_offset(struct_info->var_name_to_offset,(*meta).name) + (arr.size - 1) * this->asm_type_to_bytes(arr.type);
        std::string val = this->gen_expr(struct_ptr_ident_->index_expr).value();
        if (is_numeric(val)) {
            offset << arr.type << PTR_KEYWORD << " [edx - " << arr.head_base_pointer_offset - std::stoi(val) * this->asm_type_to_bytes(arr.type) << "] " ;
        }
        else {
            if (val != "eax") {
                this->m_code << "    " << this->get_mov_instruc("eax", val.substr(0, val.find_first_of(" "))) << " eax, " << val << std::endl;
            }
            offset << arr.type <<  PTR_KEYWORD << " [edx - " << arr.head_base_pointer_offset << " + eax * " << this->asm_type_to_bytes(arr.type) << "]";
        }
        return offset.str();
    }
    else if ((*meta).variable_kind == "struct ptr"){
        Var var;
        var.base_pointer_offset = find_offset(struct_info->var_name_to_offset,(*meta).name);
        var.ptr_type = (*meta).struct_name;
        if(moved){
            return this->gen_term_struct_ptr(&var,struct_ptr_ident_,EDX_OFF);
        }
        else{
            this->m_code << "    mov eax, dword" << PTR_KEYWORD << " [edx - " << var.base_pointer_offset << "]" << std::endl; 
            return "&eax";
        }

    }
    else if ((*meta).variable_kind == "struct"){
        Struct struct_;
        struct_.vars = struct_info->pre_gen_vars;
        const auto struct_it = std::find_if(struct_.vars.cbegin(), struct_.vars.cend(), 
                                    [&](const std::variant<Var, string_buffer, String, Var_array, Struct>& v) {
                                        if (std::holds_alternative<Struct>(v)) {
                                            const Struct& s = std::get<Struct>(v);
                                            return s.name == (*meta).name;
                                        }
                                        return false;
                                    });
        struct_ = std::get<Struct>(*struct_it);
        return this->gen_term_struct(&struct_,struct_ptr_ident_,EDX_OFF);
    }
    this->line_err("Struct pointer generation failed");
    exit(EXIT_FAILURE);

}

inline std::optional<std::string> Generator::gen_term(const node::_term* term) {
    struct term_visitor {
        Generator* gen;
        std::optional<std::string> ret_val;
        void operator()(const node::_term_int_lit* term_int_lit) {
            ret_val = term_int_lit->_int_lit.value.value();
        }
        void operator()(const node::_term_str_lit* str_lit) {
            std::stringstream ss;
            ss << "\"" << gen->gen_str_lit(str_lit->_str_lit.value.value());
            ret_val = ss.str();
        }
        void operator()(const node::_term_ident* term_ident) {
            auto generate_var_offset = [&](const Var& var) {
                std::stringstream offset;
                offset << var.type << PTR_KEYWORD << " [ebp - " << (var.base_pointer_offset) << "]";
                return offset.str();
            };

            auto handle_struct_var = [&](const std::variant<Var, string_buffer, String, Var_array, Struct>& var, auto& struct_it) -> std::optional<std::string> {
                std::stringstream offset;
                if (std::holds_alternative<Var>(var)) {
                    return generate_var_offset(std::get<Var>(var));
                } else if (std::holds_alternative<string_buffer>(var)) {
                    offset << "\"" << std::get<string_buffer>(var).generated;
                    return offset.str();
                } else if (std::holds_alternative<String>(var)) {
                    offset << "\"" << std::get<String>(var).generated;
                    return offset.str();
                } else if (std::holds_alternative<Var_array>(var)) {
                    Var_array v_a = std::get<Var_array>(var);
                    offset << v_a.type << PTR_KEYWORD << " [ebp - " << (v_a.head_base_pointer_offset) << "]";
                    return offset.str();
                } else if (std::holds_alternative<Struct>(var)) {
                    struct_it = std::find_if(gen->m_structs.begin(), gen->m_structs.end(),
                                             [&](const Struct& struct_) { return struct_.name == std::get<Struct>(var).name; });
                    return std::nullopt;  // Continue looping
                }
                return std::nullopt;
            };

            const auto it = std::find_if(gen->m_vars.cbegin(), gen->m_vars.cend(),
                                         [&](const Var& var) { return var.name == term_ident->ident.value.value(); });

            std::stringstream offset;

            if (it != gen->m_vars.cend()) {
                offset << generate_var_offset(*it);
            } else {
                auto arr_it = std::find_if(gen->m_arrays.cbegin(), gen->m_arrays.cend(),
                                              [&](const Var_array& arr) { return arr.name == term_ident->ident.value.value(); });

                if (arr_it != gen->m_arrays.cend()){
                    offset << (*arr_it).type << PTR_KEYWORD << " [ebp - " << ((*arr_it).head_base_pointer_offset) << "]";

                }else{                           
                    auto struct_it = std::find_if(gen->m_structs.begin(), gen->m_structs.end(),
                                                  [&](const Struct& struct_) { return struct_.name == term_ident->ident.value.value(); });

                    if (struct_it != gen->m_structs.end()) {
                        while (true) {
                            auto var = (*struct_it).vars[0];
                            auto result = handle_struct_var(var, struct_it);
                            if (result.has_value()) {
                                offset << result.value();
                                break;
                            }
                        }
                    } else {
                        const auto str_it = std::find_if(gen->m_strs.cbegin(), gen->m_strs.cend(),
                                                         [&](const String& var) { return var.name == term_ident->ident.value.value(); });

                        if (str_it != gen->m_strs.cend()) {
                            offset << "\"" << (*str_it).generated;
                        } else {
                            const auto str_buf_it = std::find_if(gen->m_str_bufs.cbegin(), gen->m_str_bufs.cend(),
                                                                 [&](const string_buffer& var) { return var.name == term_ident->ident.value.value(); });

                            if (str_buf_it != gen->m_str_bufs.cend()) {
                                offset << "\"" << (*str_buf_it).generated;
                            } else {
                                std::stringstream ss;
                                ss << "Identifier '" << term_ident->ident.value.value() << "' was not declared in this scope!" << std::endl;
                                gen->line_err(ss.str());
                                return;
                            }
                        }
                    }
                }
            }

            ret_val = offset.str();
        }

        void operator()(node::_term_struct_ident* struct_ident){
            const auto struct_it = std::find_if(gen->m_structs.cbegin(),gen->m_structs.cend(), [&](Struct _struct){return _struct.name == struct_ident->ident.value.value();});
            if(struct_it == gen->m_structs.cend()){
                const auto struct_ptr_it = std::find_if(gen->m_vars.cbegin(),gen->m_vars.cend(),[&](const Var& var){return var.name == struct_ident->ident.value.value();});

                if(struct_ptr_it == gen->m_vars.cend()){
                    std::stringstream ss;
                    ss << "Struct with the name '" << struct_ident->ident.value.value() << "' was not declared in this scope";
                    gen->line_err(ss.str());
                }
                if(!(*struct_ptr_it).is_struct_ptr){
                    gen->line_err("Cannot get struct values from non-struct pointer");
                }
                ret_val = gen->gen_term_struct_ptr(struct_ptr_it,struct_ident,EBP_OFF);

            }else{
                ret_val = gen->gen_term_struct(struct_it,struct_ident,EBP_OFF);
            }


        }
        void operator()(const node::_double_op* double_op) {
            const auto it = std::find_if(gen->m_vars.cbegin(), gen->m_vars.cend(), [&](const Var& var) {return var.name == double_op->ident.value.value(); });
            if (it == gen->m_vars.cend()) {
                std::stringstream ss;
                ss << "Identifier '" << double_op->ident.value.value() << "' was not declared in this scope!" << std::endl;
                gen->line_err(ss.str());
            }
            std::stringstream ss;
            
            ss << (*it).type <<  PTR_KEYWORD << " [ebp - " << (*it).base_pointer_offset << "]";
            gen->m_code << "    "<< gen->get_mov_instruc("eax",(*it).type) << " eax," << ss.str() << std::endl;
            //check if it is a ptr, if so increment by the ptr_type
            if ((*it).ptr) {
                const char* op_str = (double_op->op == Token_type::_d_add) ? "    add " : "    sub ";
                gen->m_code << op_str << ss.str() << ", " << gen->asm_type_to_bytes((*it).ptr_type) << std::endl;
            } else {
                const char* op_str = (double_op->op == Token_type::_d_add) ? "    inc " : "    dec ";
                gen->m_code << op_str << ss.str() << std::endl;
            }
            ret_val = "eax";
        }
        void operator()(const node::_function_call* fn_call) {
            const auto it = std::find_if(gen->m_funcs.cbegin(), gen->m_funcs.cend(), [&](const function& fn) {return fn.name == fn_call->ident.value.value(); });
            if (it == gen->m_funcs.cend()) {
                std::stringstream ss;
                ss << "Function '" << fn_call->ident.value.value() << "' was not found" << std::endl;
                gen->line_err(ss.str());
            }
            
            if ((*it).arguments.size() != fn_call->arguments.size()) {
                std::stringstream ss;
                ss << "Not same number of arguments for function '" << (*it).name << "' provided";
                gen->line_err(ss.str());
            }

            for (int i = 0; i < (*it).arguments.size(); i++) {
                const std::string expr = gen->gen_expr(fn_call->arguments[i]).value(); //expr can be: int_lit, eax, type ptr 
                if (expr == "eax" || expr == "&eax") {
                    gen->m_code << "    mov " << gen->m_func_registers[i] << ", eax" << std::endl;
                }
                else if (is_numeric(expr)) {
                    gen->m_code << "    mov " << gen->m_func_registers[i] << ", " << expr << std::endl;
                }
                else if (expr.substr(0, expr.find_first_of(" ")) == (*it).arguments[i].type) { 
                    gen->m_code << "    " << gen->get_mov_instruc("eax", (*it).arguments[i].type) << " " << gen->m_func_registers[i] << ", " << expr << std::endl; //eax used as a general 32-bit register
                }
                else {
                    gen->line_err("Invalid argument");
                }
            }
            
            gen->m_code << "    call " << (*it).name << std::endl;
            ret_val = (*it).ret_type_is_ptr ? "&eax" : "eax";
            
        }
        void operator()(const node::_term_paren* term_paren) {
            ret_val = gen->gen_expr(term_paren->expr);
        }

        void operator()(const node::_term_negate* term_neg) {
            const std::string expr = gen->gen_expr(term_neg->expr).value();

            if (expr != "eax") {
                if (is_numeric(expr)) {
                    gen->m_code << "    mov eax," << expr << std::endl;
                }
                else {
                    gen->m_code << "    " << gen->get_mov_instruc("eax", expr.substr(0, expr.find_first_of(' '))) << " eax," << expr << std::endl;
                }
            }
            gen->m_code << "    neg eax" << std::endl;
            ret_val = "eax";
        }
        void operator()(const node::_term_deref* deref) {
            const auto it = std::find_if(gen->m_vars.cbegin(), gen->m_vars.cend(), [&](const Var& var) {return var.name == deref->ident.value.value(); });
            if (it == gen->m_vars.cend()) {
                std::stringstream ss;
                ss << "Identifier '" << deref->ident.value.value() << "' was not declared in this scope";
                gen->line_err(ss.str());
            }
         
            if (!(*it).ptr) {
                std::stringstream ss;
                ss << "Cannot dereference variable '" << deref->ident.value.value() << "' due to it not being a pointer" << std::endl;
                gen->line_err(ss.str());
            }

            gen->m_code << "    mov eax, dword" << PTR_KEYWORD << " [ebp - " << (*it).base_pointer_offset << "]" << std::endl;
            gen->m_code << "    " << gen->get_mov_instruc("eax", (*it).ptr_type) << " eax, " << (*it).ptr_type <<  PTR_KEYWORD << " [eax]" << std::endl;
            ret_val = "eax";
        }
        void operator()(const node::_term_array_index* term_array_idx) {
            const auto it = std::find_if(gen->m_arrays.cbegin(), gen->m_arrays.cend(), [&](const Var_array& var) {return var.name == term_array_idx->ident.value.value(); });
            if (it == gen->m_arrays.cend()) {
                const auto var_it = std::find_if(gen->m_vars.cbegin(), gen->m_vars.cend(), [&](const Var& var) {return var.name == term_array_idx->ident.value.value(); });
                if (var_it == gen->m_vars.cend()) {
                    std::stringstream ss;
                    ss << "Array '" << term_array_idx->ident.value.value() << "' was not declared in this scope";
                    gen->line_err(ss.str());
                }
                
                std::stringstream ss;
                if(!(*var_it).ptr) {
                    ss << "Cannot index variable '" << term_array_idx->ident.value.value() << "' due to it not being a pointer" << std::endl;
                    gen->line_err(ss.str());
                }

                std::string val = gen->gen_expr(term_array_idx->index_expr).value();
                if(val.rfind("\"",0) == 0) {
                    gen->line_err("Cannot use string as an array index");
                }
                bool numeric = is_numeric(val);
                if(!numeric) {
                    gen->m_code << "    " << gen->get_mov_instruc("ebx", val.substr(0, val.find_first_of(' '))) << " ebx, " << val << std::endl;
                }
                gen->m_code << "    mov eax, dword" << PTR_KEYWORD << " [ebp - " << (*var_it).base_pointer_offset << "]" << std::endl;
                if(numeric) {
                    ss << (*var_it).ptr_type <<  PTR_KEYWORD << " [eax + " << std::stoi(val) *  gen->asm_type_to_bytes((*var_it).ptr_type) << "]";
                }else{
                    ss << (*var_it).ptr_type <<  PTR_KEYWORD << " [eax + ebx * " << gen->asm_type_to_bytes((*var_it).ptr_type) << "]";
                }
                ret_val = ss.str();
                
            }
            else {
                std::stringstream ss;
                std::string val = gen->gen_expr(term_array_idx->index_expr).value();
                if(val.rfind("\"",0) == 0) {
                    gen->line_err("Cannot use string as an array index");
                }
                if (is_numeric(val)) {
                    ss << (*it).type << PTR_KEYWORD << " [ebp - " << (*it).head_base_pointer_offset - std::stoi(val) * gen->asm_type_to_bytes((*it).type) << "] " ;
                }
                else {
                    if (val != "eax") {
                        gen->m_code << "    " << gen->get_mov_instruc("eax", val.substr(0, val.find_first_of(" "))) << " eax, " << val << std::endl;
                    }
                    ss << (*it).type <<  PTR_KEYWORD << " [ebp - " << (*it).head_base_pointer_offset << " + eax * " << gen->asm_type_to_bytes((*it).type) << "]";
                }
                ret_val = ss.str();
            }
        }
    };
    term_visitor visitor;
    visitor.gen = this;
    std::visit(visitor, term->var);
    return visitor.ret_val;
}
template <typename bin_expr_type>
inline std::string Generator::generic_bin_expr(bin_expr_type* bin_expr,std::string operation){
    std::string str_1 = this->gen_expr(bin_expr->left).value();
    if (str_1 == "eax" || str_1 == "&eax"){
        this->m_code << "    mov "<< this->m_bin_expr_registers[this->m_bin_expr_idx] <<", eax" << std::endl;
        str_1 = this->m_bin_expr_registers[this->m_bin_expr_idx++];
        if(this->m_bin_expr_idx > 4){
            this->line_err("Too many expressions, Sorry, need to fix this");
        }
    }
    const std::string str_2 = this->gen_expr(bin_expr->right).value();
    
    const bool str_1_numeric = is_numeric(str_1);
    const bool str_2_numeric = is_numeric(str_2);
    if (str_1_numeric && str_2_numeric)
    {   
        int num,num_1, num_2;
        num_1 = std::stoi(str_1);
        num_2 = std::stoi(str_2);
        if (operation == "add"){
            num = num_1 + num_2;
        }
        else if (operation == "sub"){
            num = num_1 - num_2;
        }
        else if (operation == "mul"){
            num = num_1 * num_2;
        }
        else if (operation == "div"){
            num = num_1 / num_2;
        } 
        else if (operation == "mod"){
            num = num_1 % num_2;
        }
        else if (operation == "xor"){
            num = num_1 ^ num_2;
        }
        else if (operation == "or"){
            num = num_1 | num_2;
        }
        else if (operation == "and"){
            num = num_1 & num_2;
        }
        return std::to_string(num);
    }
    else
    {
        if (operation == "mod"){
            operation = "div";
        }
        std::string mov_inst_1, mov_inst_2,str_2_type {};
        str_2_type = str_2.substr(0, str_2.find_first_of(' '));
        const bool str_2_is_dword = str_2_type == "dword";
        mov_inst_1 = str_1_numeric || str_1 == (this->m_bin_expr_idx == 0 ? "ecx" : this->m_bin_expr_registers[--this->m_bin_expr_idx]) ? "mov" : this->get_mov_instruc("eax", str_1.substr(0, str_1.find_first_of(' ')));
        mov_inst_2 = str_2_numeric || str_2 == "eax" ? "mov" : this->get_mov_instruc("eax", str_2_type);
        if(!str_2_is_dword){
            this->m_code << "    " << mov_inst_2 << " ebx, " << str_2 << std::endl;
        }
        this->m_code << "    " << mov_inst_1 << " eax, " << str_1 << std::endl;

        std::string mul_div_no_eax = " eax, ";
        if (operation == "div" || operation == "mul"){
            mul_div_no_eax = " ";
            if(operation == "div"){
                this->m_code << "    xor edx, edx" << std::endl;
            }
        }    

        this->m_code << "    "<<  operation << mul_div_no_eax << (str_2_is_dword ? str_2 : "ebx") << std::endl;
        return "eax";
    }
}

inline std::optional<std::string> Generator::gen_bin_expr(const node::_bin_expr* bin_expr) {
        struct bin_expr_visitor
        {
            Generator* gen;
            std::optional<std::string> ret_val;
            void operator()(const node::_bin_expr_add* bin_expr_add) {
                ret_val = gen->generic_bin_expr(bin_expr_add,"add");
            }
            void operator()(const node::_bin_expr_sub* bin_expr_sub) {
                ret_val = gen->generic_bin_expr(bin_expr_sub,"sub");
            }
            void operator()(const node::_bin_expr_mul* bin_expr_mul) {
                ret_val = gen->generic_bin_expr(bin_expr_mul,"mul");
            }
            void operator()(const node::_bin_expr_div* bin_expr_div) {
                ret_val = gen->generic_bin_expr(bin_expr_div,"div");
                if(bin_expr_div->_modulo){
                    gen->m_code << "    mov edx, eax" << std::endl;
                }
            }
            void operator()(const node::_bin_expr_xor* bin_expr_xor){
                ret_val = gen->generic_bin_expr(bin_expr_xor,"xor");
            }
            void operator()(const node::_bin_expr_or* bin_expr_or){
                ret_val = gen->generic_bin_expr(bin_expr_or,"or");
            }
            void operator()(const node::_bin_expr_and* bin_expr_and){
                ret_val = gen->generic_bin_expr(bin_expr_and,"and");
            }
        };
        bin_expr_visitor visitor;
        visitor.gen = this;
        std::visit(visitor, bin_expr->var);
        return visitor.ret_val;
    }
inline std::optional<std::string> Generator::gen_expr(const node::_expr* expr) {
    
    struct expr_visitor
    {
        Generator* gen;
        std::optional<std::string> ret_val;
        void operator()(const node::_term* term) {
            ret_val = gen->gen_term(term);
        }

        void operator()(const node::_bin_expr* bin_expr) {
            ret_val = gen->gen_bin_expr(bin_expr);
        }
        void operator()(const node::_expr_ref* ref) {
            std::string val = gen->gen_expr(ref->expr).value();
            if (is_numeric(val)) {
                gen->line_err("Cannot reference number");
            }
            if (val.rfind("\"", 0) == 0) {
                gen->line_err("Cannot reference string");
            }
            if (val == "eax" || val == "&eax") {
                gen->line_err("Invalid Reference Argument");
            }
            gen->m_code << "    lea eax, " << val << std::endl;
            
            ret_val = "&eax";//ampersand to show that a reference was returned
        }
    };
    expr_visitor visitor;
    visitor.gen = this;
    std::visit(visitor, expr->var);
    return visitor.ret_val;
}

inline Generator::logic_data_packet Generator::gen_logical_expr(const node::_logical_expr* logic_expr,std::optional<std::string> provided_scope_lbl, bool invert){
    struct logic_expr_visitor{
        Generator* gen;
        logic_data_packet data;
        bool invert_copy;
        std::optional<std::string> provided_scope_lbl;
        void operator()(const node::_logical_expr_and* logic_and){
            Token_type op = gen->gen_logical_stmt(logic_and->left,provided_scope_lbl,invert_copy).op;
            if(!gen->initial_label_and.has_value()){
                data.end_lbl = gen->mk_label();
                gen->initial_label_and = data.end_lbl;
            }else{
                data.end_lbl = gen->initial_label_and.value();
            }
            
            gen->m_code << gen->tok_to_instruc(op,true).value() << data.end_lbl << std::endl;
            Token_type op1 = gen->gen_logical_stmt(logic_and->right,provided_scope_lbl,invert_copy).op;
            std::string last_label = data.end_lbl;
            gen->ending_label = last_label;
            if(provided_scope_lbl.has_value()){
                last_label = provided_scope_lbl.value();
            }
            if(auto instruc = gen->tok_to_instruc(op1,invert_copy)){
                gen->m_code << instruc.value() << last_label << std::endl;
            }
        }
        void operator()(const node::_logical_expr_or* logic_or){
            Token_type op = gen->gen_logical_stmt(logic_or->left,provided_scope_lbl,invert_copy).op;
            if(!provided_scope_lbl.has_value()){
                data.scope_lbl = gen->mk_label();
                provided_scope_lbl = data.scope_lbl.value();
            }else{
                data.scope_lbl = provided_scope_lbl.value();
            }
            gen->m_code << gen->tok_to_instruc(op,false).value() << data.scope_lbl.value() << std::endl;

            Token_type op1 = gen->gen_logical_stmt(logic_or->right,provided_scope_lbl,invert_copy).op;
            if(!gen->ending_label.has_value()){
                data.end_lbl = gen->mk_label();
                gen->ending_label = data.end_lbl;
            }else{
                data.end_lbl = gen->ending_label.value();
            };
            
            if(auto instruc = gen->tok_to_instruc(op1,invert_copy)){
                gen->m_code << instruc.value() << data.end_lbl << std::endl;
            }
        }
    
    
    };
    logic_expr_visitor visitor;
    visitor.gen = this;
    visitor.invert_copy = invert;
    visitor.provided_scope_lbl = provided_scope_lbl;
    std::visit(visitor,logic_expr->var);
    return visitor.data;
}
inline Generator::logic_data_packet Generator::gen_logical_stmt(const node::_logical_stmt* logic_stmt,std::optional<std::string> provided_scope_lbl,bool invert){
    struct logic_stmt_visitor{
        logic_data_packet data;
        Generator* gen;
        bool invert_copy;
        std::optional<std::string> provided_scope_lbl;
        void operator()(const node::_logical_expr* logic_expr){
           data = gen->gen_logical_expr(logic_expr,provided_scope_lbl,invert_copy);
        }
        void operator()(const node::_boolean_expr* boolean_expr){
            bool moved = false;
            std::string expr1 = gen->gen_expr(boolean_expr->left).value();
            if (expr1 == "eax" || expr1 == "&eax"){
                gen->m_code << "    mov ecx, eax" << std::endl;
                expr1 = "ecx"; //store the expression in ecx, since it it unused and cannot be overwritten by other code
                moved = true;
            }
            std::string expr2 = gen->gen_expr(boolean_expr->right).value();
            if ((expr1.rfind("\"", 0) + expr2.rfind("\"", 0)) == 0) { //both are string literals

                if (!(boolean_expr->op == Token_type::_same_as || boolean_expr->op == Token_type::_not_same_as)) {
                    gen->line_err("Invalid comparison for strings");
                }
#ifdef _WIN32
                gen->m_code << "    push offset " << expr1.substr(1) << std::endl;
                gen->m_code << "    push offset " << expr2.substr(1) << std::endl;
                gen->m_code << "    call crt__stricmp" << std::endl;
                gen->m_code << "    cmp eax, 0" << std::endl;
#elif __linux__
                flags.needs_str_cout_func = true;
                gen->m_code << "    mov edi," << expr1.substr(1) << std::endl;
                gen->m_code << "    call sys~internal~str_buf_len" << std::endl;
                gen->m_code << "    mov eax, ecx" << std::endl;
                
                gen->m_code << "    mov edi," << expr2.substr(1) << std::endl;
                gen->m_code << "    call sys~internal~str_buf_len" << std::endl;
                gen->m_code << "    cmp eax,ecx" << std::endl;
                gen->m_code << "    jne " << provided_scope_lbl.value() << std::endl;

                gen->m_code << "    mov esi, "<< expr1.substr(1) << std::endl;
                gen->m_code << "    mov edi, " << expr2.substr(1) << std::endl;
                gen->m_code << "    repe cmpsb" << std::endl; 
#endif
            }
            else if (expr1.rfind("\"", 0) == std::string::npos && expr1.rfind("\"", 0) == std::string::npos) { //neither are string literals
                if (expr1 == "&eax") {
                    expr1 = "eax";
                }
                if (expr2 == "&eax") {
                    expr2 = "eax";
                }
                std::string mov_var1 = is_numeric(expr1) ? "mov" : gen->get_mov_instruc("ebx", expr1.substr(0, expr1.find_first_of(' ')));
                std::string mov_var2 = is_numeric(expr2) ? "mov" : gen->get_mov_instruc("eax", expr2.substr(0, expr2.find_first_of(' ')));
                if (moved) {
                    gen->m_code << "    mov ebx,ecx" << std::endl;
                }else{
                    gen->m_code << "    " << mov_var1 << " ebx, " << expr1 << std::endl;
                }
                if (expr2 != "eax") {
                    gen->m_code << "    " << mov_var2 << " eax, " << expr2 << std::endl;
                }
                gen->m_code << "    cmp ebx,eax\n";
            }
            else {
                gen->line_err("Invalid comparison expressions!");
            }
            data.op = boolean_expr->op;
        }
    
    
    };
    logic_stmt_visitor visitor;
    visitor.gen = this;
    visitor.invert_copy = invert;
    visitor.provided_scope_lbl = provided_scope_lbl;
    std::visit(visitor,logic_stmt->var);
    return visitor.data;
}
inline void Generator::gen_ctrl_statement(const node::_ctrl_statement* _ctrl) {
    struct _crtl_statement_visitor
    {
        Generator* gen;
        const node::_ctrl_statement* _ctrl;
        void operator()(const node::_statement_if* stmt_if) {
            logic_data_packet labels = gen->gen_logical_stmt(_ctrl->logic,std::nullopt,true);
            gen->reset_labels();
            if(labels.end_lbl == ""){ // if there is no and or or --> no returned label
                labels.end_lbl = gen->mk_label();
                gen->m_code  << gen->tok_to_instruc(labels.op,true).value() << " " << labels.end_lbl << std::endl;
            }
            if(labels.scope_lbl.has_value()){
                gen->m_code << labels.scope_lbl.value() << ":" << std::endl;
            }
            gen->gen_scope(_ctrl->scope);
            if (stmt_if->_else.has_value()) {
                std::string label_1 = gen->mk_label();
                gen->m_code << "    jmp " << label_1 << std::endl;
                gen->m_code << labels.end_lbl << ":" << std::endl;
                gen->gen_scope(stmt_if->_else.value()->scope);
                gen->m_code << label_1 << ":" << std::endl;
            }
            else {
                gen->m_code << labels.end_lbl << ":" << std::endl;
            }
        }
        void operator()(const node::_statement_while* stmt_while) {
            std::string scope_lbl = gen->mk_label();
            std::string start_jump = gen->mk_label();
            gen->m_code << "    jmp " << start_jump << std::endl;
            gen->m_code << scope_lbl << ":" << std::endl;
            gen->gen_scope(_ctrl->scope);
            gen->m_code << start_jump << ":" << std::endl;
            logic_data_packet labels = gen->gen_logical_stmt(_ctrl->logic,scope_lbl,false);
            if(labels.end_lbl == ""){ // if there is no and or or --> no returned label
                labels.end_lbl = gen->mk_label();
                gen->m_code  << gen->tok_to_instruc(labels.op,false).value() << " " << scope_lbl << std::endl;
            }
            gen->m_code << labels.end_lbl << ":" << std::endl;
        

        }
        void operator()(const node::_statement_for* stmt_for) {

            gen->gen_var_stmt(stmt_for->_stmt_var_dec);
            std::string scope_lbl = gen->mk_label();
            std::string start_jump = gen->mk_label();
            gen->m_code << "    jmp " << start_jump << std::endl;
            gen->m_code << scope_lbl << ":" << std::endl;
            gen->gen_scope(_ctrl->scope);
            gen->gen_expr(stmt_for->var_op);
            gen->m_code << start_jump << ":" << std::endl;
            logic_data_packet labels = gen->gen_logical_stmt(_ctrl->logic,scope_lbl,false);
            if(labels.end_lbl == ""){ // if there is no and or or --> no returned label
                labels.end_lbl = gen->mk_label();
                gen->m_code  << gen->tok_to_instruc(labels.op,false).value() << " " << scope_lbl << std::endl;
            }
            gen->m_code << labels.end_lbl << ":" << std::endl;
            gen->m_vars.pop_back();//pop the iterator value
        }
    };
    _crtl_statement_visitor visitor;
    visitor.gen = this;
    visitor._ctrl = _ctrl;
    std::visit(visitor, _ctrl->var);    
}
inline void Generator::gen_var_stmt(const node::_statement_var_dec* stmt_var_dec) {
    struct var_visitor {
        Generator* gen;
        void operator()(const node::_var_dec_num* var_num) {
            if(!gen->ignore_var_already_exists){
                const auto it  = std::find_if(gen->m_vars.cbegin(), gen->m_vars.cend(), [&](const Var& var) {return var.name == var_num->ident.value.value(); });
                if (it != gen->m_vars.cend()) {
                    std::stringstream ss;
                    ss << "Identifier '" << var_num->ident.value.value() << "' was already declared in this scope!" << std::endl;
                    gen->line_err(ss.str());
                }
            }
            std::string val;
            if (gen->emit_var_gen_asm){
                val = gen->gen_expr(var_num->expr).value();
            }
            Var var;
            var.type = var_type_to_str(var_num->type);
            var.immutable = var_num->_const;
            var.ptr = var_num->_ptr;
            var.bool_limit = var_num->type == Token_type::_bool;
            gen->m_base_ptr_off += gen->asm_type_to_bytes(var.type);
            if(var.ptr){
                var.ptr_type = var.type;
                var.type = "dword";
            }
            if (gen->emit_var_gen_asm){
                if (is_numeric(val)) {
                    int num = std::stoi(val);
                    if (var.bool_limit) {
                        num = (num != 0);
                    }
                    if (var_num->_ptr) {
                        if (num != 0) {
                            gen->line_err("Integer value for pointer may only be null (0)");
                        }
                    }
                    if (gen->emit_var_gen_asm){
                        gen->m_code << "    mov " << var.type <<  PTR_KEYWORD << " [ebp - " << gen->m_base_ptr_off << "]" << ", " << num << std::endl;
                    }
                }
                else {
                    if (var.bool_limit) {

                        gen->m_code << "    cmp " << val << ", 0" << std::endl;
                        gen->m_code << "    setne dl" << std::endl;
                        gen->m_code << "    mov " << var.type <<  PTR_KEYWORD << " [ebp - " << gen->m_base_ptr_off << "]" << ", dl" << std::endl;
                    }
                    else {
                        if (var_num->_ptr) {
                            if (val == "&eax") {
                                val = "eax";
                            }
                            else {
                                gen->line_err("Pointer cannot be assigned the value of a non-reference");
                            }
                        }
                        if (val == "&eax") {
                            gen->line_err("Cannot assign reference to non-pointer");
                        }

                        if (val != "eax") {
                            gen->m_code << "    " << gen->get_mov_instruc("eax", val.substr(0, val.find_first_of(' '))) << " eax, " << val << std::endl;
                        }
                        gen->m_code << "    mov " << var.type <<  PTR_KEYWORD << " [ebp - " << gen->m_base_ptr_off << "]" << ", " << gen->get_correct_part_of_register(var.type) << std::endl;

                    }
                }
            }
            var.name = var_num->ident.value.value();
            var.base_pointer_offset = gen->m_base_ptr_off;
            gen->generating_struct_vars ? (*gen->generic_struct_vars).push_back(var) : gen->m_vars.push_back(var);
            
        }
        void operator()(const node::_var_dec_str* var_str) {
            if(!gen->ignore_var_already_exists){
                const auto it  = std::find_if(gen->m_strs.cbegin(), gen->m_strs.cend(), [&](const String& str) {return str.name == var_str->ident.value.value(); });
                if (it != gen->m_strs.cend()) {
                    std::stringstream ss;
                    ss << "String with name '" << var_str->ident.value.value() << "' was already declared!" << std::endl;
                    gen->line_err(ss.str());
                }
            }
            
            gen->gen_expr(var_str->expr); //rest is handled by the gen_str_lit function
            gen->generating_struct_vars ? std::get<String>((*gen->generic_struct_vars).back()).name = var_str->ident.value.value() : gen->m_strs.back().name = var_str->ident.value.value();//manually add the name here 
            
        }
        void operator()(const node::_var_dec_str_buf* var_str_buf) {
            const auto it  = std::find_if(gen->m_str_bufs.cbegin(), gen->m_str_bufs.cend(), [&](const string_buffer& str_buf) {return str_buf.name == var_str_buf->ident.value.value(); });
            if (it != gen->m_str_bufs.cend()) {
                if (!(*it).free) {
                    std::stringstream ss;
                    ss << "String Buffer with name '" << var_str_buf->ident.value.value() << "' was already declared in this scope!" << std::endl;
                    gen->line_err(ss.str());
                }
                else {
                    string_buffer str_buf;
                    if ((*it).size >= stoi(var_str_buf->_int_lit.value.value())) {
                        str_buf.size = (*it).size;
                        str_buf.generated = (*it).generated;
                        str_buf.name = (*it).name;
                    }
                    else {
                        str_buf.size = stoi(var_str_buf->_int_lit.value.value()); //safe because it is guaranteed to be an integer literal
                        str_buf.name = var_str_buf->ident.value.value();
                        str_buf.generated = gen->mk_str_buf();
                        if (gen->emit_var_gen_asm){
#ifdef _WIN32                        
                            gen->m_data << "    " << str_buf.generated << " db " << str_buf.size + 1 << " dup(0)" << std::endl; // the + 1 to prevent buffer overflows
#elif __linux__
                            gen->m_bss << "    " << str_buf.generated << " resb " << str_buf.size + 1 << std::endl;
#endif           
                        }    
                    }
                    gen->generating_struct_vars ? (*gen->generic_struct_vars).push_back(str_buf) : gen->m_str_bufs.push_back(str_buf);
                }
            }
            else {
                const auto free_it = std::find_if(gen->m_str_bufs.begin(), gen->m_str_bufs.end(), [&](const string_buffer& str_buf) {return str_buf.free == true; });
                string_buffer str_buf;
                if (free_it != gen->m_str_bufs.cend() && free_it->size >= stoi(var_str_buf->_int_lit.value.value())) { // found a free string buffer
                    
                    str_buf.size = free_it->size;
                    str_buf.generated = free_it->generated;
                    str_buf.name = free_it->name;
                    free_it->free = false;
                    
                }
                else {
                    str_buf.size = stoi(var_str_buf->_int_lit.value.value()); //safe because it is guaranteed to be an integer literal
                    str_buf.name = var_str_buf->ident.value.value();
                    str_buf.generated = gen->mk_str_buf();
                    if (gen->emit_var_gen_asm){
#ifdef _WIN32                        
                        gen->m_data << "    " << str_buf.generated << " db " << str_buf.size + 1 << " dup(0)" << std::endl; // the + 1 to prevent buffer overflows
#elif __linux__
                        gen->m_bss << "    " << str_buf.generated << " resb " << str_buf.size + 1 << std::endl;
#endif
                    }
                }
                gen->m_str_bufs.push_back(str_buf);

            }
        }
        void operator()(const node::_var_dec_array* var_array) {
            if(!gen->ignore_var_already_exists){
                const auto it  = std::find_if(gen->m_arrays.cbegin(), gen->m_arrays.cend(), [&](const Var_array& arr) {return arr.name == var_array->ident.value.value(); });
                if (it != gen->m_arrays.cend()) {
                    std::stringstream ss;
                    ss << "Identifier '" << var_array->ident.value.value() << "' was already declared in this scope!" << std::endl;
                    gen->line_err(ss.str());
                }
            }
            Var_array arr;
            arr.name = var_array->ident.value.value();
            arr.type = var_type_to_str(var_array->type);
            arr.bool_limit = var_array->type == Token_type::_bool;
            arr.size = var_array->_array_size;
            arr.head_base_pointer_offset = gen->m_base_ptr_off + arr.size * gen->asm_type_to_bytes(arr.type);
            gen->m_base_ptr_off = arr.head_base_pointer_offset;
#ifdef __linux__
            if (gen->emit_var_gen_asm){
                if(var_array->init_str.has_value()){
                    const int str_len = var_array->init_str.value().length();
                    if(str_len+ 1 > arr.size){
                        gen->line_err("Length of string larger than size of array");
                    }

                    for(size_t i = 0;i < var_array->init_str.value().length(); i++){
                        gen->m_code << "    mov byte [ebp - " << arr.head_base_pointer_offset - i << "], " <<   static_cast<int> (var_array->init_str.value().at(i)) << std::endl;
                    }
                    gen->m_code << "    mov byte [ebp - "  << arr.head_base_pointer_offset - str_len   << "], 0" << std::endl;//manuallly null terminate the string

                }
            }
#endif
            //it currently makes no sense to init an array as const, because there is no initialization rn.
            arr.immutable = var_array->_const;
            gen->generating_struct_vars ? (*gen->generic_struct_vars).push_back(arr) : gen->m_arrays.push_back(arr);
        
        }
        void operator()(const node::_var_dec_struct* var_struct){
            const auto it  = std::find_if(gen->m_struct_infos.cbegin(), gen->m_struct_infos.cend(), [&](const Struct_info& inf) {return inf.name == var_struct->struct_name; });
            if(it == gen->m_struct_infos.cend()){
                gen->line_err("How did this happen??");
            }
            Struct struct_;
            bool generating_struct_vars_save = gen->generating_struct_vars;
            gen->generating_struct_vars = true;
            gen->ignore_var_already_exists = true;

            auto struct_vars_save = gen->generic_struct_vars;
            gen->generic_struct_vars = &struct_.vars;
            for (node::_statement_var_dec* var_dec : (*it).var_decs){
                gen->gen_var_stmt(var_dec);
            }
            gen->generating_struct_vars = generating_struct_vars_save;
            gen->generic_struct_vars = struct_vars_save;
            gen->ignore_var_already_exists = false;

            struct_.name = var_struct->ident.value.value();
            generating_struct_vars_save ? (*gen->generic_struct_vars).push_back(struct_) : gen->m_structs.push_back(struct_);
        }
        void operator()(const node::_var_dec_struct_ptr* var_dec_struct_ptr){
            if (!gen->ignore_var_already_exists){
                const auto it = std::find_if(gen->m_vars.cbegin(),gen->m_vars.cend(), [&] (const Var& var) {return var.name == var_dec_struct_ptr->ident.value.value();});
                if(it != gen->m_vars.cend()){
                    std::stringstream ss;
                    ss << "Identifier '" << var_dec_struct_ptr->ident.value.value() << "' was already declared in this scope!" << std::endl;
                    gen->line_err(ss.str());
                }
            }

            Var var;
            var.type = "dword";
            var.immutable = var_dec_struct_ptr->_const;
            var.ptr = true;
            var.is_struct_ptr = true;
            var.name = var_dec_struct_ptr->ident.value.value();
            var.ptr_type = var_dec_struct_ptr->struct_name;
            
            gen->m_base_ptr_off += 4; //size for a dword
            
            std::string val;
            if (gen->emit_var_gen_asm){
                val = gen->gen_expr(var_dec_struct_ptr->expr).value();

                if(is_numeric(val)){

                    if(std::stoi(val) != 0){
                        gen->line_err("Integer value for pointer may only be null (0)");
                    }

                    gen->m_code << "    mov " << var.type <<  PTR_KEYWORD << " [ebp - " << gen->m_base_ptr_off << "], 0"<< std::endl;

                }else{
                    if(val != "&eax"){
                        gen->line_err("Pointer cannot be assigned the value of a non-reference");
                    }

                    gen->m_code << "    mov dword" << PTR_KEYWORD << " [ebp - " << gen->m_base_ptr_off << "], eax"<< std::endl;
                }
            }

            var.base_pointer_offset = gen->m_base_ptr_off;
            gen->generating_struct_vars ? (*gen->generic_struct_vars).push_back(var) : gen->m_vars.push_back(var);
        }
    };
    if (this->valid_space) {
        var_visitor visitor;
        visitor.gen = this;
        std::visit(visitor, stmt_var_dec->var);
    }
    else {
        line_err("Can currently not declare variables outside of valid space like function");
    }
}
void Generator::var_set_str(){
    this->line_err("Strings are immutable at the current stage of development :(");
}

template<typename iterator,typename var_set>
void Generator::var_set_str_buf(iterator it,var_set var_str_buf,std::string base_string){
    std::string val = this->gen_expr(var_str_buf->expr).value();
    if (val.rfind("\"", 0) != 0) { 
        this->line_err("Stringbuffers can only be assigned strings");               
    }
#ifdef _WIN32    
    this->m_code << "    mov esi, offset " << val.substr(1) << std::endl;
    this->m_code << "    mov edi, offset " << (*it).generated << std::endl;
    this->m_code << "    mov ecx," << (*it).size << std::endl;
    this->m_code << "    rep movsb" << std::endl;
#elif __linux__
    flags.needs_str_cpy_func = true;
    this->m_code << "    mov esi, " << val.substr(1) << std::endl;
    this->m_code << "    mov edi, " << (*it).generated << std::endl;
    this->m_code << "    call sys~internal_strcpy" << std::endl;

#endif
}

template<typename iterator,typename var_set>
void Generator::var_set_number(iterator it,var_set var_num,std::string base_string){
    if ((*it).immutable) {
        this->line_err("Variable not mutable!");
    }
    if (var_num->deref && !(*it).ptr) {
        this->line_err("Cannot dereference non-pointer");
    }
    std::string val = this->gen_expr(var_num->expr).value();
    if(val.rfind("\"",0) == 0){
        line_err("Cannot assign string to number variable");
    }
    if (is_numeric(val)) {
        int num = std::stoi(val);
        if ((*it).bool_limit) {
            num = (num != 0);
        }
        if ((*it).ptr) {
            if (var_num->deref) { //manual dereference
                this->m_code << "    mov eax, dword" << PTR_KEYWORD << base_string << (*it).base_pointer_offset << "]" << std::endl;
                this->m_code << "    mov " << (*it).ptr_type <<  PTR_KEYWORD << " [eax], " << num << std::endl;
                return;
            }
            else if (num != 0) {
                this->line_err("Integer value for pointer may only be null (0)");
            }
        }
        this->m_code << "    mov " << (*it).type <<  PTR_KEYWORD << base_string << (*it).base_pointer_offset << "]" << "," << num << std::endl;
    }
    else {
        if ((*it).bool_limit && !var_num->deref) {
            this->m_code << "    cmp " << val << ", 0" << std::endl;
            this->m_code << "    setne dl" << std::endl;
            this->m_code << "    mov " << (*it).type <<  PTR_KEYWORD << base_string << (*it).base_pointer_offset << "]" << ", dl" << std::endl;
        }
        else {
            if ((*it).ptr) {
                if (val == "&eax" && !var_num->deref) {
                    val = "eax";
                }
                if (var_num->deref && val != "&eax") {
                    if (val == "eax") {
                        this->m_code << "    mov edx, eax" << std::endl;
                    }
                    else {
                        this->m_code << "    " << this->get_mov_instruc("eax", val.substr(0, val.find_first_of(' '))) << " edx, " << val << std::endl;
                    }
                    this->m_code << "    mov eax, dword" << PTR_KEYWORD <<  base_string << (*it).base_pointer_offset << "]" << std::endl;
                    this->m_code << "    mov " << (*it).ptr_type <<  PTR_KEYWORD << " [eax], " << this->get_correct_part_of_register((*it).ptr_type, true) << std::endl;
                }
                
            }
            if (val != "eax") {
                this->m_code << "    " << this->get_mov_instruc("eax", val.substr(0, val.find_first_of(' '))) << " eax, " << val << std::endl;
            }
            this->m_code << "    mov " << (*it).type <<  PTR_KEYWORD << base_string << (*it).base_pointer_offset << "]" << "," << this->get_correct_part_of_register((*it).type) << std::endl;
        }
    }
}
template<typename iterator,typename var_set>
void Generator::var_set_ptr_array(iterator it,var_set array_set,std::string base_string){
    if ((*it).immutable) {
        this->line_err("Array not mutable!");
    }
    std::string val = this->gen_expr(array_set->expr).value();
    if(val.rfind("\"",0) == 0){this->line_err("Cannot use string to index array");}
    if (is_numeric(val)) {
        std::string index_val = this->gen_expr(array_set->index_expr).value();
        int num = std::stoi(val);
        if ((*it).bool_limit) {
            num = (num != 0);
        }
        this->m_code << "    mov edx, dword" << PTR_KEYWORD << base_string << (*it).base_pointer_offset << "]" << std::endl;
        if(is_numeric(index_val)){
            this->m_code << "    " << this->get_mov_instruc((*it).ptr_type,"ebx") << " " << (*it).ptr_type <<  PTR_KEYWORD << " [edx + " << std::stoi(index_val) * this->asm_type_to_bytes((*it).ptr_type) << "], "  << num << std::endl;
        }else{
            if(index_val != "eax"){
                this->m_code << "    " << this->get_mov_instruc("eax", index_val.substr(0, index_val.find_first_of(" "))) << " eax, " << index_val << std::endl;
            }
            this->m_code << "    " << this->get_mov_instruc((*it).ptr_type,"eax") <<" " << (*it).ptr_type <<  PTR_KEYWORD << " [edx + eax * " << this->asm_type_to_bytes((*it).ptr_type) << "], "  << num << std::endl;
        }
        
    }else{
        if (val == "eax") { // to prevent the two expressions from overwriting if they are both eax
            this->m_code << "    mov edx, eax" << std::endl;
        }
        else {
            this->m_code << "    " << this->get_mov_instruc("edx", val.substr(0, val.find_first_of(" "))) << " edx, " << val << std::endl;
        }
        val = "edx";
        std::string index_val = this->gen_expr(array_set->index_expr).value();
        if ((*it).bool_limit) {
            this->m_code << "    cmp " << val << ", 0" << std::endl;
            this->m_code << "    setne bl" << std::endl;
        }
        this->m_code << "    mov ecx, dword" << PTR_KEYWORD << base_string << (*it).base_pointer_offset << "]" << std::endl;
        if (is_numeric(index_val)) {
            std::string mov_reg = "edx";
            if((*it).bool_limit){
                mov_reg = "bl";
            }
            this->m_code << "    " << this->get_mov_instruc((*it).ptr_type,mov_reg) << " " << (*it).ptr_type <<  PTR_KEYWORD << " [ecx + " << std::stoi(index_val) * this->asm_type_to_bytes((*it).ptr_type) << "], "  << mov_reg << std::endl;
        }
        else {
            std::string mov_reg =  "edx";
            if ((*it).bool_limit) {
                mov_reg = "bl";
            }
            
            if (index_val != "eax") {
                this->m_code << "    " << this->get_mov_instruc("eax", index_val.substr(0, index_val.find_first_of(" "))) << " eax, " << index_val << std::endl;
            } // now: index value in eax and expression value in edx or bl
            this->m_code << "    " << this->get_mov_instruc((*it).ptr_type,mov_reg) << " " << (*it).ptr_type <<  PTR_KEYWORD << " [ecx + eax * " << this->asm_type_to_bytes((*it).ptr_type) << "], "  << mov_reg << std::endl;
        }
    }
}    
template<typename iterator,typename var_set>
void Generator::var_set_array(iterator it,var_set array_set,std::string base_string){
    if ((*it).immutable) {
        this->line_err("Array not mutable!");
    }
    std::string val = this->gen_expr(array_set->expr).value();
    if(val.rfind("\"",0) == 0){this->line_err("Cannot use string to index array");}
    if (is_numeric(val)) {
        std::string index_val = this->gen_expr(array_set->index_expr).value();
        int num = std::stoi(val);
        if ((*it).bool_limit) {
            num = (num != 0);
        }
        if (is_numeric(index_val)) {
            this->m_code << "    mov " << (*it).type <<  PTR_KEYWORD << base_string << (*it).head_base_pointer_offset - std::stoi(index_val) * this->asm_type_to_bytes((*it).type) << "], " << num << std::endl;
        }
        else {
            if (index_val != "eax") {
                this->m_code << "    " << this->get_mov_instruc("eax", index_val.substr(0, index_val.find_first_of(" "))) << " eax, " << index_val << std::endl;
            }
            this->m_code << "    mov " << (*it).type <<  PTR_KEYWORD << base_string << (*it).head_base_pointer_offset << " + eax * " << this->asm_type_to_bytes((*it).type) << "], " << num << std::endl;
        }
    }
    else {
        
        if (val == "eax") { // to prevent the two expressions from overwriting if they are both eax
            this->m_code << "    mov edx, eax" << std::endl;
        }
        else {
            this->m_code << "    " << this->get_mov_instruc("edx", val.substr(0, val.find_first_of(" "))) << " edx, " << val << std::endl;
        }
        val = "edx";
        const std::string index_val = this->gen_expr(array_set->index_expr).value();
        if ((*it).bool_limit) {
            this->m_code << "    cmp " << val << ", 0" << std::endl;
            this->m_code << "    setne bl" << std::endl;
        }
        if (is_numeric(index_val)) {
            std::string mov_reg = "edx";
            if((*it).bool_limit){
                mov_reg = "bl";
            }
            this->m_code << "    " << this->get_mov_instruc((*it).type,mov_reg) << " " << (*it).type <<  PTR_KEYWORD << base_string << (*it).head_base_pointer_offset - std::stoi(index_val) * this->asm_type_to_bytes((*it).type) << "], " << mov_reg << std::endl;
            
        }
        else {
            std::string mov_reg =  "edx";
            if ((*it).bool_limit) {
                mov_reg = "bl";
            }
            
            if (index_val != "eax") {
                this->m_code << "    " << this->get_mov_instruc("eax", index_val.substr(0, index_val.find_first_of(" "))) << " eax, " << index_val << std::endl;
            } // now: index value in eax and expression value in edx or bl
            
            this->m_code << "    mov " << (*it).type <<  PTR_KEYWORD << base_string << (*it).head_base_pointer_offset << " + eax * " << this->asm_type_to_bytes((*it).type) << "], " << mov_reg << std::endl;
        }
        
    }
}


template<typename iterator,typename var_set>
void Generator::var_set_struct(iterator struct_it,var_set struct_set,std::string base_string){
    std::string expected = struct_set->item->ident.value.value();
    
    
    //find the first element in the vars vector of the struct that has the name of our searched item and return an iterator to it
    const auto it  = std::find_if((*struct_it).vars.cbegin(), (*struct_it).vars.cend(), [&](const auto& var) {
    return std::visit([&](const auto& element) { return element.name == expected; }, var);
    });
    
    if(struct_set->item->item != nullptr && !(std::holds_alternative<Var>(*it) && std::get<Var>(*it).is_struct_ptr)){
        struct_set->item = struct_set->item->item;
    }
    
    
    if(it == (*struct_it).vars.cend()){
        std::stringstream ss;
        this->line_err(ss.str());
    }    

    if(std::holds_alternative<Var>(*it)){
        Var var = std::get<Var>(*it);
        if (var.is_struct_ptr){
            this->var_set_struct_ptr(&var,struct_set,base_string,true);
        }else{
            if constexpr (std::is_same<std::remove_pointer_t<var_set>, node::_var_set_array>::value){
                this->var_set_ptr_array(&var,struct_set,base_string);
            }else{
                this->var_set_number(&var,struct_set,base_string);
            }
        }

    }
    else if(std::holds_alternative<string_buffer>(*it)){
        string_buffer buf = std::get<string_buffer>(*it);
        this->var_set_str_buf(&buf,struct_set,base_string);
    }
    else if(std::holds_alternative<String>(*it)){
        this->var_set_str(); //doesn't matter
    }
    else if(std::holds_alternative<Var_array>(*it)){
        Var_array arr = std::get<Var_array>(*it);
        this->var_set_array(&arr,struct_set,base_string);
    }
    else if(std::holds_alternative<Struct>(*it)){
        Struct struct_ = std::get<Struct>(*it);
        this->var_set_struct(&struct_,struct_set,base_string);
    }
}


template<typename iterator,typename var_set>
void Generator::var_set_struct_ptr(iterator it,var_set struct_ptr_set,std::string base_string,bool lea,bool ret){
    std::string op = lea ? "lea" : "mov" ;
    this->m_code << "    "<<op<<" edx, dword" << PTR_KEYWORD << base_string << (*it).base_pointer_offset << "]"<<std::endl;
    //get struct info for the struct type which the pointer is pointing to
    const auto struct_info = std::find_if(this->m_struct_infos.cbegin(),this->m_struct_infos.cend(), 
    [&](const Struct_info& info){return info.name == (*it).ptr_type;});
    
    if (struct_info == this->m_struct_infos.cend()){
        std::stringstream ss;
        ss << "Struct with name " << (*it).ptr_type << " not found";
        this->line_err(ss.str());
    }

    


    std::string item_name = struct_ptr_set->item->ident.value.value();
    bool moved = false;
    if (struct_ptr_set->item->item != nullptr){
        struct_ptr_set->item = struct_ptr_set->item->item;
        moved = true;
    }
    if((*it).ptr_type == struct_info->name && ret && !moved){
        return;
    }
    //     |          i
    // my_bdnl_ptr -> p -> null -> null -> ...
    //itemname = p

    //     |               i
    // my_bdnl_ptr -> p -> z -> null -> null -> ...
    //itemname = p -> z


    const auto meta = std::find_if(struct_info->var_metadatas.cbegin(), struct_info->var_metadatas.cend(),
        [&item_name](const node::_var_metadata& metadata) {
            return metadata.name == item_name;
        });

    if (meta == struct_info->var_metadatas.cend()){
        return;
    }    

    //instantiate fake variables that use the same memory space as real variables
    if ((*meta).variable_kind == "number"){
        Var var;
        var.base_pointer_offset = find_offset(struct_info->var_name_to_offset,(*meta).name);
        var.bool_limit = (*meta).type == Token_type::_bool;
        var.immutable = (*meta)._const;
        var.ptr = (*meta)._ptr;
        var.type = var_type_to_str((*meta).type);

        if(var.ptr){
            var.ptr_type = var.type;
            var.type = "dword";
        }

        if(var.ptr && struct_ptr_set->index_expr != nullptr){
            this->var_set_ptr_array(&var,struct_ptr_set,EDX_OFF);
        }else{
            this->var_set_number(&var,struct_ptr_set,EDX_OFF);
        }

    }else if((*meta).variable_kind == "array"){
        Var_array array;
        array.bool_limit = (*meta).type == Token_type::_bool;
        array.immutable = (*meta)._const;
        array.size = (*meta)._array_size;
        array.type = var_type_to_str((*meta).type);
        array.head_base_pointer_offset = find_offset(struct_info->var_name_to_offset,(*meta).name) + (array.size - 1) * this->asm_type_to_bytes(array.type);

        this->var_set_array(&array,struct_ptr_set,EDX_OFF);

    }else if((*meta).variable_kind == "struct ptr"){
        Var var;
        var.base_pointer_offset = find_offset(struct_info->var_name_to_offset,(*meta).name);
        var.ptr_type = (*meta).struct_name;
        std::string val;
        if(!(!moved && var.base_pointer_offset == 0)){
            this->var_set_struct_ptr(&var,struct_ptr_set,EDX_OFF,!moved,!moved);
        }
        if(!moved){ //ptr is the last thing in the chain
            val = this->gen_expr(struct_ptr_set->expr).value();
            if(is_numeric(val)){
                if(std::stoi(val) != 0){
                    this->line_err("Integer value for pointer may only be null (0)");
                }
                this->m_code << "    mov " << var.type <<  PTR_KEYWORD << " [edx], 0"<< std::endl;
            }else{
                if(val != "&eax"){
                    this->line_err("Pointer cannot be assigned the value of a non-reference");
                }
                this->m_code << "    mov dword" << PTR_KEYWORD << " [edx], eax"<< std::endl;
            }
        }

    }else if((*meta).variable_kind == "struct"){
        Struct struct_;
        struct_.vars = struct_info->pre_gen_vars;
        const auto struct_it = std::find_if(struct_.vars.cbegin(), struct_.vars.cend(), 
                                    [&](const std::variant<Var, string_buffer, String, Var_array, Struct>& v) {
                                        if (std::holds_alternative<Struct>(v)) {
                                            const Struct& s = std::get<Struct>(v);
                                            return s.name == (*meta).name;
                                        }
                                        return false;
                                    });
        Struct nested_struct = std::get<Struct>(*struct_it);
        this->var_set_struct(&nested_struct,struct_ptr_set,EDX_OFF);
    }

}

inline void Generator::gen_var_set(const node::_statement_var_set* stmt_var_set) {

    struct set_visitor {
        Generator* gen;
        void operator()(const node::_var_set_num* var_num) {
            const auto str_it = std::find_if(gen->m_strs.cbegin(), gen->m_strs.cend(), [&](const String& var) {return var.name == var_num->ident.value.value(); });
            if (str_it != gen->m_strs.cend()) {gen->var_set_str();return;}//in case I want to strings mutable one day and forget this
            
            const auto str_buf_it = std::find_if(gen->m_str_bufs.cbegin(), gen->m_str_bufs.cend(), [&](const string_buffer& var) {return var.name == var_num->ident.value.value(); });
            if(str_buf_it != gen->m_str_bufs.cend()){
                gen->var_set_str_buf(str_buf_it,var_num," ");
                return;
            }
            const auto it  = std::find_if(gen->m_vars.cbegin(), gen->m_vars.cend(), [&](const Var& var) {return var.name == var_num->ident.value.value(); });
            if (it == gen->m_vars.cend()) {
                std::stringstream ss;
                ss << "Identifier '" << var_num->ident.value.value() << "' was not declared in this scope!" << std::endl;
                gen->line_err(ss.str());
                
            }
            else {
                gen->var_set_number(it,var_num,EBP_OFF);
            }
        }
        void operator()(const node::_var_set_array* array_set) {
            const auto it  = std::find_if(gen->m_arrays.cbegin(), gen->m_arrays.cend(), [&](const Var_array& var) {return var.name == array_set->ident.value.value(); });
            if (it == gen->m_arrays.cend()) {
                const auto  ptr_it = std::find_if(gen->m_vars.cbegin(), gen->m_vars.cend(), [&](const Var& var) {return var.name == array_set->ident.value.value(); });
                if (ptr_it == gen->m_vars.cend()) {
                    std::stringstream ss;
                    ss << "Array with the name '" << array_set->ident.value.value() << "' was not declared in this scope";
                    gen->line_err(ss.str());
                }
                gen->var_set_ptr_array(ptr_it,array_set,EBP_OFF);
            }
            else {
                gen->var_set_array(it,array_set,EBP_OFF);
            }
        }
        void operator()(node::_var_set_struct* struct_set){
            const auto  struct_it = std::find_if(gen->m_structs.cbegin(),gen->m_structs.cend(), [&](const Struct& _struct){return _struct.name == struct_set->ident.value.value();});
            if(struct_it == gen->m_structs.cend()){
                const auto struct_ptr_it = std::find_if(gen->m_vars.cbegin(), gen->m_vars.cend(), [&](const Var& var){return var.name == struct_set->ident.value.value();});

                if (struct_ptr_it == gen->m_vars.cend()){

                    std::stringstream ss;
                    ss << "Struct with the name '" << struct_set->ident.value.value() << "' was not declared in this scope";
                    gen->line_err(ss.str());
                }else{  
                    if (!(*struct_ptr_it).is_struct_ptr){
                        gen->line_err("Cannot use non-struct-pointer to set a struct");
                    }

                    gen->var_set_struct_ptr(struct_ptr_it,struct_set,EBP_OFF);

                }
            }
            else{
                gen->var_set_struct(struct_it,struct_set,EBP_OFF);
            }
            
        }

    };
    if (this->valid_space) {
        set_visitor visitor;
        visitor.gen = this;
        std::visit(visitor, stmt_var_set->var);
    }
    else {
        line_err("Can currently not set variable values outside of functions");
    }


}
inline void Generator::intern_flags(const node::_null_stmt* null_stmt) {
    struct flags_visitor {
        Generator* gen;
        void operator()(const node::_newfile* newfile) {
            gen->filestack.push_back(newfile->filename);
        }
        void operator()(const node::_newline* newline) {
            gen->line_counter++;
        }
        void operator()(const node::_eof* eof) {
            gen->filestack.pop_back();
            gen->line_counter = 1;
        }

    };
    flags_visitor visitor;
    visitor.gen = this;
    std::visit(visitor, null_stmt->var);

}
inline void Generator::gen_stmt(const node::_statement* stmt) {
    struct stmt_visitor
    {
        Generator* gen;
        void operator()(const node::_statement_exit* stmt_exit) {
            std::string expr_val = gen->gen_expr(stmt_exit->expr).value();
            std::string mov_val;
            if (is_numeric(expr_val))
            {
                int num = std::stoi(expr_val);
                mov_val = "mov";
            }
#ifdef _WIN32
            else
            {
                mov_val = gen->get_mov_instruc("eax", expr_val.substr(0, expr_val.find_first_of(' ')));
            }

            gen->m_code << "    " << mov_val << " eax," << expr_val << std::endl;
            gen->m_code << "    invoke ExitProcess,eax" << std::endl;
#elif __linux__
            else{
                mov_val = "movzx";
            }
            gen->m_code << "    "<< mov_val <<" ebx, " << expr_val << std::endl;
            gen->m_code << "    mov eax, 1 ; sys-exit" << std::endl;
            gen->m_code << "    int 0x80" << std::endl;
#endif
        }
        void operator()(const node::_null_stmt* null_stmt) {
            gen->intern_flags(null_stmt);
        }
        void operator()(const node::_statement_var_dec* stmt_var_dec) {
            gen->gen_var_stmt(stmt_var_dec);
        }
        void operator()(const node::_statement_var_set* stmt_var_set) {
            gen->gen_var_set(stmt_var_set);
        }
        void operator()(const node::_asm_* _asm_) { 
            gen->m_code << _asm_->str_lit.value.value() << std::endl;
        }
        void operator()(const node::_statement_scope* statement_scope) {
            gen->gen_scope(statement_scope);
        }
        void operator()(const node::_main_scope* main_scope) {
            if (!gen->main_proc) {
                if (!gen->valid_space) {
                    gen->valid_space = true;
#ifdef _WIN32
                    gen->m_code << "_main proc\n";
#elif __linux__
                    gen->m_code << "_start:\n"; 
#endif
                    gen->main_proc = true;
                    gen->scope_start(true, main_scope->stack_space);
                    for (const node::_statement* stmt : main_scope->scope->statements) {
                        gen->gen_stmt(stmt);
                    }
                    gen->scope_end(true, main_scope->stack_space);
                    gen->valid_space = false;
                }
                else {
                    gen->line_err("Cannot define function inside of the main scope");
                }
            }
            else {
                gen->line_err("Main procedure already found!");
            }
        }
        void operator()(const node::_statement_function* stmt_func) {
            const auto it  = std::find_if(gen->m_funcs.cbegin(), gen->m_funcs.cend(), [&](const function& fn) {return fn.name == stmt_func->ident.value.value(); });
            if (it != gen->m_funcs.cend()) {
                std::stringstream ss;
                ss << "Function with the name '" << stmt_func->ident.value.value() << "' was already declared" << std::endl;
                gen->line_err(ss.str());
            }
            else {

                if (!gen->valid_space) {
                    gen->valid_space = true;
                    std::string save = gen->m_code.str();
                    gen->m_code.str(std::string()); //clear the stringstream
                    function func;
                    func.name = stmt_func->ident.value.value();
#ifdef _WIN32                    
                    gen->m_code << func.name << " proc" << std::endl;
#elif __linux__
                    gen->m_code << func.name << ":" << std::endl;
#endif
                    int stack_space_add = 0;
                    for (int i = 0; i < stmt_func->arguments.size(); i++) {
                        stack_space_add += (gen->str_bit_sizes.find(stmt_func->arguments[i]->type)->second) / 8; //get bit size and devide by 8 for bytes
                    }
                    gen->scope_start(true, stmt_func->stack_space + stack_space_add);
                    //for each argumend, generate a local var
                    for (int i = 0; i < stmt_func->arguments.size(); i++) {
                        Var var;
                        var.type = stmt_func->arguments[i]->type;
                        if (stmt_func->arguments[i]->_ptr) {
                            var.ptr_type = var.type;
                            stmt_func->arguments[i]->ptr_type = var.ptr_type;
                            var.type = "dword";
                            stmt_func->arguments[i]->type = "dword";
                            var.ptr = true;
                        }
                        var.name = stmt_func->arguments[i]->ident.value.value();
                        gen->m_base_ptr_off += gen->asm_type_to_bytes(stmt_func->arguments[i]->type);
                        gen->m_code << "    mov eax," << gen->m_func_registers[i] << std::endl;
                        gen->m_code << "    mov" << " " << var.type <<  PTR_KEYWORD << " [ebp - " << gen->m_base_ptr_off << "]" << ", " << gen->get_correct_part_of_register(var.type) << std::endl;
                        var.base_pointer_offset = gen->m_base_ptr_off;

                        gen->m_vars.push_back(var);
                        func.arguments.push_back(*stmt_func->arguments[i]);
                    }
                    func.ret_type_is_ptr = stmt_func->ret_type_is_ptr;
                    func.ret_lbl = gen->mk_label();
                    gen->curr_func_name.push_back(func.name);
                    gen->m_funcs.push_back(func);
                    for (const node::_statement* stmt : stmt_func->scope->statements) {
                        gen->gen_stmt(stmt);
                    }
                    gen->m_code << func.ret_lbl << ":" << std::endl;
                    gen->scope_end(true, stmt_func->stack_space + stack_space_add);
                    gen->m_code << "    ret" << std::endl;
#ifdef _WIN32                    
                    gen->m_code << func.name << " endp" << std::endl;
#endif
                    gen->m_func_space << gen->m_code.str() << std::endl;
                    gen->m_code.str(std::string());
                    gen->m_code << save;
                    gen->valid_space = false;
                    gen->curr_func_name.pop_back();
                }
                else {
                    gen->line_err("Cannot declare a function inside of a function");
                }
            }
        }
        void operator()(const node::_ctrl_statement* _ctrl) {
            gen->gen_ctrl_statement(_ctrl);
        }
        void operator()(const node::_op_equal* op_eq) {
            const auto it  = std::find_if(gen->m_vars.cbegin(), gen->m_vars.cend(), [&](const Var& var) {return var.name == op_eq->ident.value.value(); });
            if (it == gen->m_vars.cend()) {
                std::stringstream ss;
                ss << "Identifier '" << op_eq->ident.value.value() << "' was not declared in this scope!" << std::endl;
                gen->line_err(ss.str());
            }
            std::string val = gen->gen_expr(op_eq->expr).value();
            if (val.rfind("\"", 0) == 0) {
                gen->line_err("Cannot change value of string");
            }
            if (is_numeric(val)) {
                gen->m_code << "    mov eax, " << val << std::endl;
            }
            else if (val != "eax") {
                gen->m_code << "    " << gen->get_mov_instruc("eax", val.substr(0, val.find_first_of(" "))) << " eax, " << val << std::endl;
            }
            std::stringstream ss;
            ss << (*it).type <<  PTR_KEYWORD << " [ebp - " <<  (*it).base_pointer_offset << "]";
            if ((*it).ptr) {
                gen->m_code << "    imul eax, " << gen->asm_type_to_bytes((*it).ptr_type) << std::endl;
            }
            switch (op_eq->op)
            {
                case Token_type::_add_eq: {
                    gen->m_code << "    add " << ss.str() << ", " << gen->get_correct_part_of_register((*it).type) << std::endl;
                    break;
                }
                case Token_type::_sub_eq: {
                    gen->m_code << "    sub " << ss.str() << ", " << gen->get_correct_part_of_register((*it).type) << std::endl;
                    break;
                }
                case Token_type::_mul_eq: {
                    gen->m_code << "    " << gen->get_mov_instruc("edx", (*it).type) << " edx, " << ss.str() << std::endl;
                    gen->m_code << "    imul edx,eax" << std::endl;
                    gen->m_code << "    mov " << ss.str() << ", " << gen->get_correct_part_of_register((*it).type,true) << std::endl;
                    break;
                }
                case Token_type::_div_eq: {
                    gen->m_code << "    " << gen->get_mov_instruc("edx", (*it).type) << " edx, " << ss.str() << std::endl;
                    gen->m_code << "    idiv edx,eax" << std::endl;
                    gen->m_code << "    mov " << ss.str() << ", " << gen->get_correct_part_of_register((*it).type,true) << std::endl;
                    break;
                }
            }
        }
        void operator()(const node::_statement_output* stmt_output) {
            for (auto expr : stmt_output->expr_vec)
            {
                std::string val = gen->gen_expr(expr).value();
                if (val.rfind("\"", 0) == 0) { //Hard encoded shit here
#ifdef _WIN32                
                    gen->m_code << "    invoke StdOut, offset " << val.substr(1) << std::endl;
#elif __linux__
                    std::string len = val.substr(1) + "_len";
                    if(val.find("string_buffer_") != std::string::npos) // it is a stringbuffer
                    {
                        flags.needs_str_cout_func = true;
                        gen->m_code << "    mov edi, " << val.substr(1) << std::endl;
                        gen->m_code << "    call sys~internal~str_buf_len" << std::endl; //calculate lenght of strbuf and store in ecx
                        len = "ecx";
                    }
                    gen->m_code << "    mov eax, 4 ;sys-write" << std::endl;
                    gen->m_code << "    mov ebx, 1" << std::endl;
                    gen->m_code << "    mov ecx, " << val.substr(1) << std::endl;
                    gen->m_code << "    mov edx, " << len<< std::endl; //length was declared in gen_str_lit
                    gen->m_code << "    int 0x80" << std::endl;
#endif
                }
                else {
                    if (is_numeric(val)) {
#ifdef _WIN32                        
                        gen->m_code << "    mov eax, " << val << std::endl;
#elif __linux__
                        gen->m_code << "    mov dword [num_buffer], " << val << std::endl;
#endif
                        val = "eax";
                    }
                    if (val == "&eax") {
                        val = "eax";
                    }
                    else if (val != "eax") { //if val == eax no need to mov eax, eax                    
                        gen->m_code << "    " << gen->get_mov_instruc("eax", val.substr(0, val.find_first_of(' '))) << " eax," << val << std::endl;
#ifdef __linux__
                    gen->m_code << "    mov dword [num_buffer], eax" << std::endl;
#endif
                    }
#ifdef _WIN32                    
                    gen->m_code << "    invoke dwtoa,eax,offset buffer ;dword to ascii" << std::endl;
                    gen->m_code << "    invoke StdOut, offset buffer" << std::endl;
#elif __linux__
                    flags.needs_buffer = true;
                    flags.needs_str_cout_func = true;
                    //the programmer is expected to have called int_to_ascii from the stdlib
                    gen->m_code << "    mov edi, num_buffer" << std::endl;
                    gen->m_code << "    call sys~internal~str_buf_len" << std::endl;
                    gen->m_code << "    mov edi, ecx" << std::endl;
                    gen->m_code << "    mov eax, 4 ; sys-write" << std::endl;
                    gen->m_code << "    mov ebx, 1" << std::endl;
                    gen->m_code << "    mov ecx, num_buffer" << std::endl;
                    gen->m_code << "    mov edx, edi" << std::endl;
                    gen->m_code << "    int 0x80" << std::endl;
#endif
                }
            }
            if (!stmt_output->noend) {
#ifdef _WIN32                
                gen->m_code << "    invoke StdOut, offset backn" << std::endl;
#elif __linux__
                gen->m_code << "    mov eax, 4 ; sys-write" << std::endl;
                gen->m_code << "    mov ebx, 1" << std::endl;
                gen->m_code << "    mov ecx, newline" << std::endl;
                gen->m_code << "    mov edx, 1" << std::endl;
                gen->m_code << "    int 0x80" << std::endl;
#endif
            }
        }
        void operator()(const node::_statement_input* stmt_input) {
            const auto it  = std::find_if(gen->m_str_bufs.cbegin(), gen->m_str_bufs.cend(), [&](const string_buffer& var) {return var.name == stmt_input->ident.value.value(); });
            if (it == gen->m_str_bufs.cend()) {
#ifdef __linux__                
                const auto arr_it = std::find_if(gen->m_arrays.cbegin(),gen->m_arrays.cend(), [&](const Var_array& arr){return arr.name == stmt_input->ident.value.value();});
                if(arr_it != gen->m_arrays.cend()){
                    if((*arr_it).bool_limit || (*arr_it).type != "byte"){
                        gen->line_err("Can only input into byte arrays");
                    }
                    if((*arr_it).immutable){
                        gen->line_err("Cannot write to constant array");
                    }
                    // I know that when wring to arrays, they are not memory safe, but I dont want to manually write a null byte at the end when the buffer overflows
                    // get used to it :)
                    std::string ecx_and_edi = "byte [ebp - " + std::to_string((*arr_it).head_base_pointer_offset) + "] ";
                    gen->sys_read((*arr_it).size,ecx_and_edi,true);
                }else{       
#endif
                std::stringstream ss;
                ss << "String buffer '" << stmt_input->ident.value.value() << "' was not declared in this scope" << std::endl;
                gen->line_err(ss.str());
#ifdef __linux
                }
#endif
            }
            else {
#ifdef _WIN32                
                gen->m_code << "    invoke StdIn, offset " << (*it).generated << ", " << (*it).size << std::endl;
                gen->m_code << "    invoke StdIn, offset buffer, 255 ;clear the rest of the input buffer " << std::endl;
#elif __linux__
                std::string ecx_and_edi = (*it).generated;
                gen->sys_read((*it).size,ecx_and_edi,false);
#endif
            }
        }
        void operator()(const node::_statement_ret* stmt_ret) {
            if (gen->valid_space) {
                if (gen->curr_func_name.size() < 1) {
                    gen->line_err("Cannot use return in 'brick' function, consider using \"exit [exitcode]\" instead");
                }
                else {
                    std::string str_it = gen->curr_func_name.back();
                    const auto it  = std::find_if(gen->m_funcs.cbegin(), gen->m_funcs.cend(), [&](const function& fn) {return fn.name == str_it; });
                    if (it == gen->m_funcs.cend()) {
                        gen->line_err("Function name label failed");
                    }
                    std::string expr = gen->gen_expr(stmt_ret->expr).value();
                    if (is_numeric(expr) || expr == "edx") {
                        gen->m_code << "    mov eax," << expr << std::endl;
                    }
                    else if (expr != "eax") {
                        gen->m_code << "    " << gen->get_mov_instruc("eax", expr.substr(0, expr.find_first_of(" "))) << " eax," << expr << std::endl;
                    } // if expr == eax no need to move it
                    gen->m_code << "    jmp " << (*it).ret_lbl << std::endl;
                }
            }
            else {
                gen->line_err("Cannot return when outside a function");
            }
        }
        void operator()(const node::_statement_struct* stmt_struct){
            
            Struct_info struct_inf;
            struct_inf.name = stmt_struct->ident.value.value();
            struct_inf.var_decs = stmt_struct->vars;
            struct_inf.var_name_to_offset = stmt_struct->name_to_offsets;
            struct_inf.var_metadatas = stmt_struct->vars_metadata;
            
            gen->valid_space = true;
            gen->emit_var_gen_asm = false;
            gen->generating_struct_vars = true;
            gen->ignore_var_already_exists = true;
            size_t base_ptr_off_save = gen->m_base_ptr_off;
            gen->generic_struct_vars = &struct_inf.pre_gen_vars;
            for (node::_statement_var_dec* var_dec : struct_inf.var_decs){
                gen->gen_var_stmt(var_dec);
            }
            gen->m_base_ptr_off = base_ptr_off_save;
            gen->ignore_var_already_exists = false;
            gen->generating_struct_vars = false;
            gen->generic_struct_vars = nullptr;
            gen->emit_var_gen_asm = true;
            gen->valid_space = false;

            gen->line_counter += stmt_struct->n_lines;//I really dont know of a better way rn and I dont want to
            gen->m_struct_infos.push_back(struct_inf);
        }
        
        void operator()(const node::_statement_pure_expr* pure_expr) {
            gen->gen_expr(pure_expr->expr);
        }
    };
    stmt_visitor visitor;
    visitor.gen = this;
    std::visit(visitor, stmt->var); //matches based on variant type
}
std::string Generator::gen_program() {
    m_output << m_header;
    for (const node::_statement* stmt : m_prog.statements) {
        gen_stmt(stmt);
    }
    m_output << "\n";

#ifdef __linux__
    m_output << "section ";
#endif 
    m_output << ".data\n";
    if (flags.needs_buffer) {
#ifdef _WIN32        
        m_output << "    buffer dd 256 dup (0)\n";
        m_output << "    backn db 13,10 ;ASCII new line char\n";
#elif __linux__
        m_output << "    newline db 10 ; ASCII newline char\n";
#endif
    }
    m_output << m_data.str();

#ifdef __linux__
    m_output << "section .bss\n";

    if(flags.needs_buffer){
        m_output << "    num_buffer resb 12 " << std::endl;
    }
    m_output << m_bss.str();

#endif

#ifdef _WIN32    
    m_output << "\n.code\n";
#elif __linux__
    m_output << "\nsection .text\n"; 
#endif
    m_output << m_code.str();

#ifdef _WIN32
    m_output << "_main endp\n";
#endif    
    m_output << m_func_space.str();

#ifdef __linux__
if(flags.needs_str_cpy_func){
    m_output << "sys~internal_strcpy:          " << std::endl;
    m_output << "    mov al, [esi]             " << std::endl;
    m_output << "    mov [edi], al             " << std::endl;
    m_output << "    inc esi                   " << std::endl;
    m_output << "    inc edi                   " << std::endl;
    m_output << "    cmp al, 0                 " << std::endl;
    m_output << "    jne sys~internal_strcpy\n " << std::endl;
    m_output << "    ret" << std::endl;
}

if(flags.needs_str_cout_func){
    m_output << "sys~internal~str_buf_len: " << std::endl;
    m_output << "    xor ecx, ecx          " << std::endl;
    m_output << ".loop:                    " << std::endl; 
    m_output << "    cmp byte [edi + ecx],0" << std::endl;
    m_output << "    je .done              " << std::endl; 
    m_output << "    inc ecx               " << std::endl;
    m_output << "    jmp .loop             " << std::endl;
    m_output << ".done:                    " << std::endl;
    m_output << "    ret                   " << std::endl;
}

if(flags.needs_nl_replace_func){
m_output << "sys~internal_replace_nl_null:           " << std::endl;
m_output << "    ; Loop to find the newline character" << std::endl;
m_output << "    mov al, 10                          " << std::endl;
m_output << "find_newline:                           " << std::endl;
m_output << "    cmp byte [edi], al                  " << std::endl;
m_output << "    je replace_char                     " << std::endl;
m_output << "    inc edi                             " << std::endl;
m_output << "    jmp find_newline                    " << std::endl;
m_output << "replace_char:                           " << std::endl;
m_output << "    mov byte [edi], 0                   " << std::endl;
m_output << "    ret                                 " << std::endl;                   

}
#endif

#ifdef _WIN32
    m_output << "end _main\n";
#endif    
    return m_output.str();
}

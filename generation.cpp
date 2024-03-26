#include "headers/generation.hpp"
#include <optional>
bool is_numeric(const std::string& str) {
    for (char c : str) {
        if (!std::isdigit(c)) {
            return false;
        }
    }
    return true;
}

void Generator::push(const std::string& val) {
    m_code << "    push " << val << std::endl;
    m_stack_ptr++;
}
void Generator::pop(const std::string& reg) {
    m_code << "    pop " << reg << std::endl;
    m_stack_ptr--;
}
inline void Generator::line_err(const std::string& err_msg) {
    std::cerr << "Code failed in line " << this->line_counter << "(" << this->filestack.back() << "): " << err_msg << std::endl;
    exit(1);
}
inline std::string Generator::mk_label() {
    std::stringstream ss;
    ss << "LBL_" << label_counter++;
    return ss.str();
}
inline std::string Generator::mk_str_lit() {
    std::stringstream ss;
    ss << "string_lit_" << string_counter++;
    return ss.str();
}
inline std::string Generator::mk_str_buf() {
    std::stringstream ss;
    ss << "string_buffer_" << string_buffer_counter++;
    return ss.str();
}
inline std::string Generator::mk_func() {
    std::stringstream ss;
    ss << "func_" << func_counter++;
    return ss.str();
}

inline size_t Generator::asm_type_to_bytes(std::string str) {
    if (str == "dword") {return 4;}
    else if (str == "word") { return 2;}
    else { return 1; }
}
inline std::string Generator::get_mov_instruc(const std::string& dest, const std::string& source) {
    auto source_it = str_bit_sizes.find(source);
    auto dest_it = str_bit_sizes.find(dest);
    if (source_it->second > dest_it->second) {
        line_err("Something's wrong I can feel it (how tf did this happen)");
        exit(1); //to make g++ happy
    }
    else if (source_it->second < dest_it->second) {
        return "movsx";
    }
    else {
        return "mov";
    }

}
inline void Generator::reset_labels(){
    this->initial_label_and.reset();
    this->initial_label_or.reset();
    this->ending_label.reset();
}
inline std::string Generator::get_correct_part_of_register(const std::string& source,bool edx)
{
    auto source_it = str_bit_sizes.find(source);
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
        exit(1);
    }
}
inline void Generator::scope_start(bool main_or_func_scope, size_t alloc) {
    if (main_or_func_scope) {
        this->m_base_ptr_pos.push_back(this->m_base_ptr_off);
        this->m_base_ptr_off = 0;
        push("ebp");
        m_code << "    mov ebp,esp" << std::endl;
        m_code << "    sub esp," << alloc << std::endl;

    }
    m_scope_arrays.push_back(m_arrays.size());
    m_scope_str_bufs.push_back(m_str_bufs.size());
    m_scopes.push_back(m_vars.size());//sets the scope position to the num of variables

}
inline void Generator::scope_end(bool main_or_func_scope, size_t alloc) {
    size_t str_bufs_dec = m_str_bufs.size() - this->m_scope_str_bufs.back();
    for (auto it = m_str_bufs.end() - str_bufs_dec; it != m_str_bufs.end(); ++it) { //free all the string buffers declared in the scope
        (*it).free = true;
    }
    m_scope_str_bufs.pop_back();
    size_t vars_dec = m_vars.size() - this->m_scopes.back();
    for (size_t i = 0; i < vars_dec; i++) {
        m_vars.pop_back();
    }
    size_t arrays_dec = m_arrays.size() - this->m_scope_arrays.back();
    for (size_t i = 0; i < arrays_dec; i++) {
        m_arrays.pop_back();
    }
    m_scopes.pop_back();
    if (main_or_func_scope && alloc != 0) {
        std::vector<size_t>::iterator it = this->m_base_ptr_pos.end();
        this->m_base_ptr_off = *it;
        this->m_base_ptr_pos.pop_back();
        m_code << "    add esp," << alloc << std::endl;
        pop("ebp");
    }
}
inline std::string Generator::gen_str_lit(const std::string string_lit) {
    auto it = std::find_if(this->m_strs.cbegin(), this->m_strs.cend(), [&](const String& str1) {return str1.str_val == string_lit; });
    if (it == this->m_strs.cend()) { // string lit value was not already declared
        String str;
        std::string ident = mk_str_lit();
        this->m_data << "    " << ident << " db \"" << string_lit << "\" ,0" << std::endl;
        str.name = ident;
        str.str_val = string_lit;
        this->m_strs.push_back(str);
        return ident;
    }
    else {
        return (*it).name;
    }

}
inline void Generator::gen_scope(const node::_statement_scope* scope) {
    scope_start();
    for (const node::_statement* stmt : scope->statements) {
        gen_stmt(stmt);
    }
    scope_end();
}
inline std::optional<std::string> Generator::tok_to_instruc(Token_type type, bool invert) {

    if (invert) {
        if (type == Token_type::_same_as) { return "    jne "; }
        if (type == Token_type::_not_same_as) { return "    je "; }
        if (type == Token_type::_greater_as) { return "    jle "; }
        if (type == Token_type::_greater_eq_as) { return "    jl "; }
        if (type == Token_type::_less_as) { return "    jge "; }
        if (type == Token_type::_less_eq_as) { return "    jg "; }
        else {
            return {};
        }
    }
    else {
        if (type == Token_type::_same_as) { return "    je "; }
        if (type == Token_type::_not_same_as) { return "    jne "; }
        if (type == Token_type::_greater_as) { return "    jg "; }
        if (type == Token_type::_greater_eq_as) { return "    jge "; }
        if (type == Token_type::_less_as) { return "    jl "; }
        if (type == Token_type::_less_eq_as) { return "    jle "; }
        else {
            return {};
        }
    }
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
                auto it = std::find_if(gen->m_vars.cbegin(), gen->m_vars.cend(), [&](const Var& var) {return var.name == term_ident->ident.value.value(); });
                std::stringstream offset;
                if (it == gen->m_vars.cend()) {
                    auto str_it = std::find_if(gen->m_str_map.cbegin(), gen->m_str_map.cend(), [&](const str_map& var) {return var.ident == term_ident->ident.value.value(); });
                    if (str_it != gen->m_str_map.cend()) {
                        offset << (*str_it).generated;
                    }
                    else {
                        auto str_buf_it = std::find_if(gen->m_str_bufs.cbegin(), gen->m_str_bufs.cend(), [&](const string_buffer& var) {return var.name == term_ident->ident.value.value(); });
                        if (str_buf_it != gen->m_str_bufs.cend()) {
                            offset << "\"" << (*str_buf_it).generated;
                        }
                        else {
                            std::stringstream ss;
                            ss << "Identifier '" << term_ident->ident.value.value() << "' was not declared in this scope!" << std::endl;
                            gen->line_err(ss.str());
                        }
                    }
                }
                else {
                    offset << (*it).type << " ptr [ebp - " << ((*it).base_pointer_offset) << "]";
                }
                ret_val = offset.str();

            }
            void operator()(const node::_function_call* fn_call) {
                auto it = std::find_if(gen->m_funcs.cbegin(), gen->m_funcs.cend(), [&](const function& fn) {return fn.name == fn_call->ident.value.value(); });
                if (it == gen->m_funcs.cend()) {
                    std::stringstream ss;
                    ss << "Function '" << fn_call->ident.value.value() << "' was not found" << std::endl;
                    gen->line_err(ss.str());
                }
                else {
                    if ((*it).arguments.size() != fn_call->arguments.size()) {
                        std::stringstream ss;
                        ss << "Not same number of arguments for function '" << (*it).name << "' provided";
                        gen->line_err(ss.str());
                    }
                    for (int i = 0; i < (*it).arguments.size(); i++) {
                        std::string expr = gen->gen_expr(fn_call->arguments[i]).value(); //expr can be: int_lit, eax, type ptr 
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
                    gen->m_code << "    call " << (*it).generated << std::endl;
                    ret_val = "eax";
                }
            }
            void operator()(const node::_term_paren* term_paren) {
                ret_val = gen->gen_expr(term_paren->expr);
            }
            void operator()(const node::_term_negate* term_neg) {
                std::string expr = gen->gen_expr(term_neg->expr).value();
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
                auto it = std::find_if(gen->m_vars.cbegin(), gen->m_vars.cend(), [&](const Var& var) {return var.name == deref->ident.value.value(); });
                if (it == gen->m_vars.cend()) {
                    std::stringstream ss;
                    ss << "Identifier '" << deref->ident.value.value() << "' was not declared in this scope";
                    gen->line_err(ss.str());
                }
                else {
                    if (!(*it).ptr) {
                        std::stringstream ss;
                        ss << "Cannot dereference variable '" << deref->ident.value.value() << "' due to it not being a pointer" << std::endl;
                        gen->line_err(ss.str());
                    }
                    gen->m_code << "    mov eax, dword ptr [ebp - " << (*it).base_pointer_offset << "]" << std::endl;
                    gen->m_code << "    " << gen->get_mov_instruc("eax", (*it).ptr_type) << " eax, " << (*it).ptr_type << " ptr [eax]" << std::endl;
                    ret_val = "eax";
                }
            }
            void operator()(const node::_term_array_index* term_array_idx) {
                auto it = std::find_if(gen->m_arrays.cbegin(), gen->m_arrays.cend(), [&](const Var_array& var) {return var.name == term_array_idx->ident.value.value(); });
                if (it == gen->m_arrays.cend()) {
                    std::stringstream ss;
                    ss << "Array '" << term_array_idx->ident.value.value() << "' was not declared in this scope";
                    gen->line_err(ss.str());
                }
                else {
                    std::stringstream ss;
                    std::string val = gen->gen_expr(term_array_idx->index_expr).value();
                    if (is_numeric(val)) {
                        ss << (*it).type <<" ptr[ebp - " << (*it).head_base_pointer_offset - std::stoi(val) * gen->asm_type_to_bytes((*it).type) << "] " ;
                    }
                    else {
                        if (val != "eax") {
                            gen->m_code << "    " << gen->get_mov_instruc("eax", val.substr(0, val.find_first_of(" "))) << " eax, " << val << std::endl;
                        }
                        ss << (*it).type << " ptr [ebp - " << (*it).head_base_pointer_offset << " + eax * " << gen->asm_type_to_bytes((*it).type) << "]";
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

inline std::optional<std::string> Generator::gen_bin_expr(const node::_bin_expr* bin_expr) {
        struct bin_expr_visitor
        {
            Generator* gen;
            std::optional<std::string> ret_val;
            void operator()(const node::_bin_expr_add* bin_expr_add) {
                const std::string str_1 = gen->gen_expr(bin_expr_add->left).value();
                const std::string str_2 = gen->gen_expr(bin_expr_add->right).value();
                if (is_numeric(str_1) && is_numeric(str_2))
                {
                    int num = std::stoi(str_1) + std::stoi(str_2);
                    ret_val = std::to_string(num);
                }
                else
                {
                    std::string mov_inst_1, mov_inst_2;

                    mov_inst_2 = is_numeric(str_2) ? "mov" : gen->get_mov_instruc("eax", str_2.substr(0, str_2.find_first_of(' ')));
                    mov_inst_1 = is_numeric(str_1) ? "mov" : gen->get_mov_instruc("eax", str_1.substr(0, str_1.find_first_of(' ')));

                    gen->m_code << "    " << mov_inst_2 << " ebx, " << str_2 << std::endl;
                    if (str_1 != "eax") {
                        gen->m_code << "    " << mov_inst_1 << " eax, " << str_1 << std::endl;
                    }
                    gen->m_code << "    add eax,ebx" << std::endl;
                    ret_val = "eax";
                }
            }
            void operator()(const node::_bin_expr_sub* bin_expr_sub) {
                const std::string str_1 = gen->gen_expr(bin_expr_sub->left).value();
                const std::string str_2 = gen->gen_expr(bin_expr_sub->right).value();
                if (is_numeric(str_1) && is_numeric(str_2))
                {
                    int num = std::stoi(str_1) - std::stoi(str_2);
                    ret_val = std::to_string(num);
                }
                else
                {
                    std::string mov_inst_1, mov_inst_2;

                    mov_inst_1 = is_numeric(str_1) ? "mov" : gen->get_mov_instruc("eax", str_1.substr(0, str_1.find_first_of(' ')));
                    mov_inst_2 = is_numeric(str_2) ? "mov" : gen->get_mov_instruc("eax", str_2.substr(0, str_2.find_first_of(' ')));

                    gen->m_code << "    " << mov_inst_2 << " ebx, " << str_2 << std::endl;
                    if (str_1 != "eax") {
                        gen->m_code << "    " << mov_inst_1 << " eax, " << str_1 << std::endl;
                    }
                    gen->m_code << "    sub eax,ebx" << std::endl;
                    ret_val = "eax";
                }

            }
            void operator()(const node::_bin_expr_mul* bin_expr_mul) {
                const std::string str_1 = gen->gen_expr(bin_expr_mul->left).value();
                const std::string str_2 = gen->gen_expr(bin_expr_mul->right).value();
                if (is_numeric(str_1) && is_numeric(str_2))
                {
                    int num = std::stoi(str_1) * std::stoi(str_2);
                    ret_val = std::to_string(num);
                }
                else
                {
                    std::string mov_inst_1, mov_inst_2;

                    mov_inst_1 = is_numeric(str_1) ? "mov" : gen->get_mov_instruc("eax", str_1.substr(0, str_1.find_first_of(' ')));
                    mov_inst_2 = is_numeric(str_2) ? "mov" : gen->get_mov_instruc("eax", str_2.substr(0, str_2.find_first_of(' ')));

                    gen->m_code << "    " << mov_inst_2 << " ebx, " << str_2 << std::endl;
                    if (str_1 != "eax") {
                        gen->m_code << "    " << mov_inst_1 << " eax, " << str_1 << std::endl;
                    }
                    gen->m_code << "    imul eax, ebx" << std::endl;
                    ret_val = "eax";
                }
            }
            void operator()(const node::_bin_expr_div* bin_expr_div) {
                const std::string str_1 = gen->gen_expr(bin_expr_div->left).value();
                const std::string str_2 = gen->gen_expr(bin_expr_div->right).value();
                if (is_numeric(str_1) && is_numeric(str_2))
                {
                    int num;
                    if (bin_expr_div->_modulo) {
                        num = std::stoi(str_1) % std::stoi(str_2);
                    }
                    else {
                        num = std::stoi(str_1) / std::stoi(str_2);
                    }
                    ret_val = std::to_string(num);
                }
                else
                {
                    std::string mov_inst_1, mov_inst_2;

                    mov_inst_1 = is_numeric(str_1) ? "mov" : gen->get_mov_instruc("eax", str_1.substr(0, str_1.find_first_of(' ')));
                    mov_inst_2 = is_numeric(str_2) ? "mov" : gen->get_mov_instruc("eax", str_2.substr(0, str_2.find_first_of(' ')));

                    gen->m_code << "    " << mov_inst_2 << " ebx, " << str_2 << std::endl;
                    if (str_1 != "eax") {
                        gen->m_code << "    " << mov_inst_1 << " eax, " << str_1 << std::endl;
                    }
                    gen->m_code << "    xor edx,edx" << std::endl;
                    gen->m_code << "    idiv ebx" << std::endl;
                    if (bin_expr_div->_modulo) {
                        gen->m_code << "    mov eax,edx" << std::endl;
                        
                    }
                    ret_val = "eax";
                    
                }
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
            if (val == "eax") {
                gen->line_err("Invalid Reference Argument");
            }
            gen->m_code << "    lea eax, " << val << std::endl;
            
            ret_val = "&eax";//ampersand to show that a ref was returned
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
                expr1 = "ecx"; //store the expression in ecx, since it it unused and cannoit be overwritten by other code
                moved = true;
            }
            std::string expr2 = gen->gen_expr(boolean_expr->right).value();
            if ((expr1.rfind("\"", 0) + expr2.rfind("\"", 0)) == 0) { //both are string literals
                gen->m_code << "    push offset " << expr1.substr(1) << std::endl;
                gen->m_code << "    push offset " << expr2.substr(1) << std::endl;
                gen->m_code << "    call crt__stricmp" << std::endl;

                if (!(boolean_expr->op == Token_type::_same_as || boolean_expr->op == Token_type::_not_same_as)) {
                    gen->line_err("Invalid comparison for strings");
                }
                gen->m_code << "    cmp eax,0" << std::endl;
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

inline void Generator::gen_asm_expr(const node::_asm_* _asm_) {
        struct asm_visitor
        {
            Generator* gen;
            void operator()(const node::_asm_int_lit* asm_int_lit) { gen->m_code << asm_int_lit->_int_lit.value.value() << " "; }
            void operator()(const node::_asm_ident* asm_ident) { gen->m_code << asm_ident->ident.value.value() << " "; }
            void operator()(const node::_asm_comma* asm_comma) { gen->m_code << asm_comma->comma.value.value(); }

        };
        asm_visitor visitor;
        visitor.gen = this;
        std::visit(visitor, _asm_->var);
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
        //TODO fix only being able to use double expressions
        void operator()(const node::_statement_for* stmt_for) {

            //THIS CODE IS CURRENTLY UNDER MAINTENANCE
            gen->gen_var_stmt(stmt_for->_stmt_var_dec);
            std::string scope_lbl = gen->mk_label();
            std::string start_jump = gen->mk_label();
            gen->m_code << "    jmp " << start_jump << std::endl;
            gen->m_code << scope_lbl << ":" << std::endl;
            gen->gen_scope(_ctrl->scope);
            node::_statement stmt;
            stmt.var = stmt_for->_d_op;
            gen->gen_stmt(&stmt);
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
            auto it = std::find_if(gen->m_vars.cbegin(), gen->m_vars.cend(), [&](const Var& var) {return var.name == var_num->ident.value.value(); });
            if (it != gen->m_vars.cend()) {
                std::stringstream ss;
                ss << "Identifier '" << var_num->ident.value.value() << "' was already declared in this scope!" << std::endl;
                gen->line_err(ss.str());
            }
            else {
                std::string val = gen->gen_expr(var_num->expr).value();
                Var var;
                var.type = var_type_to_str(var_num->type);
                var.immutable = var_num->_const;
                if (is_numeric(val)) {
                    int num = std::stoi(val);
                    if (var_num->type == Token_type::_bool) {
                        var.bool_limit = true;
                        if (num != 0) {
                            num = 1;
                        }
                    }
                    if (var_num->_ptr) {
                        if (num != 0) {
                            gen->line_err("Integer value for pointer may only be null (0)");
                        }
                        var.ptr = true;
                        var.ptr_type = var.type;
                        var.type = "dword"; //pointers need to be dwords
                    }
                    gen->m_base_ptr_off += gen->asm_type_to_bytes(var.type);
                    gen->m_code << "    mov " << var.type << " ptr [ebp - " << gen->m_base_ptr_off << "]" << "," << num << std::endl;
                }
                else {
                    if (var_num->type == Token_type::_bool) {
                        var.bool_limit = true;
                        gen->m_code << "    cmp " << val << ", 0" << std::endl;
                        gen->m_code << "    setne dl" << std::endl;
                        gen->m_code << "    mov " << var.type << " ptr [ebp - " << gen->m_base_ptr_off << "]" << ", dl" << std::endl;
                    }
                    else {
                        if (var_num->_ptr) {
                            if (val == "&eax") {
                                var.ptr = true;
                                val = "eax";
                                var.ptr_type = var.type;
                                var.type = "dword"; //pointers need to be dwords
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
                        gen->m_base_ptr_off += gen->asm_type_to_bytes(var.type);
                        gen->m_code << "    mov " << var.type << " ptr [ebp - " << gen->m_base_ptr_off << "]" << ", " << gen->get_correct_part_of_register(var.type) << std::endl;
                    }
                }
                var.name = var_num->ident.value.value();
                var.base_pointer_offset = gen->m_base_ptr_off;
                gen->m_vars.push_back(var);
            }
        }
        void operator()(const node::_var_dec_str* var_str) {
            auto it = std::find_if(gen->m_strs.cbegin(), gen->m_strs.cend(), [&](const String& str) {return str.name == var_str->ident.value.value(); });
            if (it != gen->m_strs.cend()) {
                std::stringstream ss;
                ss << "String with name '" << var_str->ident.value.value() << "' was already declared!" << std::endl;
                gen->line_err(ss.str());
            }
            else {
                std::string val = gen->gen_expr(var_str->expr).value();
                str_map map;
                map.generated = val;
                map.ident = var_str->ident.value.value();
                gen->m_str_map.push_back(map);
            }
        }
        void operator()(const node::_var_dec_str_buf* var_str_buf) {
            auto it = std::find_if(gen->m_str_bufs.cbegin(), gen->m_str_bufs.cend(), [&](const string_buffer& str_buf) {return str_buf.name == var_str_buf->ident.value.value(); });
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
                        gen->m_data << "    " << str_buf.generated << " db " << str_buf.size + 1 << " dup(0)" << std::endl; // the + 1 to prevent buffer overflows
                    }
                    gen->m_str_bufs.push_back(str_buf);
                }
            }
            else {
                auto free_it = std::find_if(gen->m_str_bufs.begin(), gen->m_str_bufs.end(), [&](const string_buffer& str_buf) {return str_buf.free == true; });
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
                    gen->m_data << "    " << str_buf.generated << " db " << str_buf.size + 1 << " dup(0)" << std::endl; // the + 1 to prevent buffer overflows
                }
                gen->m_str_bufs.push_back(str_buf);
            }
        }
        void operator()(const node::_var_dec_array* var_array) {
            auto it = std::find_if(gen->m_arrays.cbegin(), gen->m_arrays.cend(), [&](const Var_array& arr) {return arr.name == var_array->ident.value.value(); });
            if (it != gen->m_arrays.cend()) {
                std::stringstream ss;
                ss << "Identifier '" << var_array->ident.value.value() << "' was already declared in this scope!" << std::endl;
                gen->line_err(ss.str());
            }
            else {
                Var_array arr;
                arr.name = var_array->ident.value.value();
                arr.type = var_type_to_str(var_array->type);
                if (var_array->type == Token_type::_bool) {
                    arr.bool_limit = true;
                }
                arr.size = var_array->_array_size;
                arr.head_base_pointer_offset = gen->m_base_ptr_off + arr.size * gen->asm_type_to_bytes(arr.type);
                gen->m_base_ptr_off = arr.head_base_pointer_offset;
                arr.immutable = var_array->_const;
                gen->m_arrays.push_back(arr);
            }
        
        }
    };
    if (this->in_func) {
        var_visitor visitor;
        visitor.gen = this;
        std::visit(visitor, stmt_var_dec->var);
    }
    else {
        line_err("Can currently not declare variables outside of functions");
    }
}
inline void Generator::gen_var_set(const node::_statement_var_set* stmt_var_set) {

    struct set_visitor {
        Generator* gen;
        void operator()(const node::_var_set_num* var_num) {
            auto str_it = std::find_if(gen->m_str_map.cbegin(), gen->m_str_map.cend(), [&](const str_map& var) {return var.ident == var_num->ident.value.value(); });
            if (str_it != gen->m_str_map.cend()) {
                std::stringstream ss;
                ss << "Strings are immutable at the current stage of development :(";
                gen->line_err(ss.str());
            }
            auto it = std::find_if(gen->m_vars.cbegin(), gen->m_vars.cend(), [&](const Var& var) {return var.name == var_num->ident.value.value(); });
            if (it == gen->m_vars.cend()) {
                std::stringstream ss;
                ss << "Identifier '" << var_num->ident.value.value() << "' was not declared in this scope!" << std::endl;
                gen->line_err(ss.str());
                
            }
            else {
                if ((*it).immutable) {
                    gen->line_err("Variable not mutable!");
                }
                if (var_num->deref && !(*it).ptr) {
                    gen->line_err("Cannot dereference non-pointer");
                }
                std::string val = gen->gen_expr(var_num->expr).value();
                if (is_numeric(val)) {
                    int num = std::stoi(val);
                    if ((*it).bool_limit) {
                        if (num != 0) {
                            num = 1;
                        }
                    }

                    if ((*it).ptr) {
                        if (var_num->deref) { //manual dereference
                            gen->m_code << "    mov eax, dword ptr [ebp -  " << (*it).base_pointer_offset << "]" << std::endl;
                            gen->m_code << "    mov " << (*it).ptr_type << " ptr [eax], " << num << std::endl;
                            return;
                        }
                        else if (num != 0) {
                            gen->line_err("Integer value for pointer may only be null (0)");
                        }
                    }
                    gen->m_code << "    mov " << (*it).type << " ptr [ebp - " << (*it).base_pointer_offset << "]" << "," << num << std::endl;
                }
                else {
                    if ((*it).bool_limit && !var_num->deref) {
                        gen->m_code << "    cmp " << val << ", 0" << std::endl;
                        gen->m_code << "    setne dl" << std::endl;
                        gen->m_code << "    mov " << (*it).type << " ptr [ebp - " << (*it).base_pointer_offset << "]" << ", dl" << std::endl;
                    }
                    else {
                        if ((*it).ptr) {
                            if (val == "&eax" && !var_num->deref) {
                                val = "eax";
                            }
                            if (var_num->deref && val != "&eax") {
                                if (val == "eax") {
                                    gen->m_code << "    mov edx, eax" << std::endl;
                                }
                                else {
                                    gen->m_code << "    " << gen->get_mov_instruc("eax", val.substr(0, val.find_first_of(' '))) << " edx, " << val << std::endl;
                                }
                                gen->m_code << "    mov eax, dword ptr [ebp -  " << (*it).base_pointer_offset << "]" << std::endl;
                                gen->m_code << "    mov " << (*it).ptr_type << " ptr [eax], " << gen->get_correct_part_of_register((*it).ptr_type, true) << std::endl;
                            }
                            else {
                                gen->line_err("Pointer cannot be assigned the value of a non-reference");
                            }
                        }
                        if (val != "eax") {
                            gen->m_code << "    " << gen->get_mov_instruc("eax", val.substr(0, val.find_first_of(' '))) << " eax, " << val << std::endl;
                        }
                        gen->m_code << "    mov " << (*it).type << " ptr [ebp - " << (*it).base_pointer_offset << "]" << "," << gen->get_correct_part_of_register((*it).type) << std::endl;
                    }
                }
            }
        }
        void operator()(const node::_var_set_array* array_set) {
            auto it = std::find_if(gen->m_arrays.cbegin(), gen->m_arrays.cend(), [&](const Var_array var) {return var.name == array_set->ident.value.value(); });
            if (it == gen->m_arrays.cend()) {
                std::stringstream ss;
                ss << "Array with the name '" << array_set->ident.value.value() << "' was not declared in this scope";
                gen->line_err(ss.str());
            }
            else {
                if ((*it).immutable) {
                    gen->line_err("Array not mutable!");
                }
                std::string val = gen->gen_expr(array_set->expr).value();
                if (is_numeric(val)) {
                    std::string index_val = gen->gen_expr(array_set->index_expr).value();
                    int num = std::stoi(val);
                    if ((*it).bool_limit) {
                        if (num != 0) {
                            num = 1;
                        }
                    }
                    if (is_numeric(index_val)) {
                        gen->m_code << "    mov " << (*it).type << " ptr [ebp - " << (*it).head_base_pointer_offset - std::stoi(index_val) * gen->asm_type_to_bytes((*it).type) << "], " << val << std::endl;
                    }
                    else {
                        if (index_val != "eax") {
                            gen->m_code << "    " << gen->get_mov_instruc("eax", index_val.substr(0, index_val.find_first_of(" "))) << " eax, " << index_val << std::endl;
                        }
                        gen->m_code << "    mov " << (*it).type << " ptr [ebp - " << (*it).head_base_pointer_offset << " + eax * " << gen->asm_type_to_bytes((*it).type) << "], " << val << std::endl;
                    }

                }
                else {
                    if (val == "eax") { // to prevent the two expressions from overwriting if they are both eax
                        gen->m_code << "    mov edx, eax" << std::endl;
                    }
                    else {
                        gen->m_code << "    " << gen->get_mov_instruc("edx", val.substr(0, val.find_first_of(" "))) << " edx, " << val << std::endl;
                    }
                    val = "edx";
                    std::string index_val = gen->gen_expr(array_set->index_expr).value();
                    if ((*it).bool_limit) {
                        gen->m_code << "    cmp " << val << ", 0" << std::endl;
                        gen->m_code << "    setne bl" << std::endl;
                    }
                    if (is_numeric(index_val)) {
                        std::string mov_reg;
                        if((*it).bool_limit){
                            mov_reg = "bl";
                        }
                        else {
                            mov_reg = "edx";
                        }
                        gen->m_code << "    mov " << (*it).type << " ptr [ebp - " << (*it).head_base_pointer_offset - std::stoi(index_val) * gen->asm_type_to_bytes((*it).type) << "], " << mov_reg << std::endl;
                    }
                    else {
                        std::string mov_reg =  "edx";
                        if ((*it).bool_limit) {
                            mov_reg = "bl";
                        }
                        else {
                            if (index_val != "eax") {
                                gen->m_code << "    " << gen->get_mov_instruc("eax", index_val.substr(0, index_val.find_first_of(" "))) << " eax, " << index_val << std::endl;
                            } // now: index value in eax and expression value in edx or bl
                            gen->m_code << "    mov " << (*it).type << " ptr [ebp - " << (*it).head_base_pointer_offset << " + eax * " << gen->asm_type_to_bytes((*it).type) << "], " << mov_reg << std::endl;
                        }
                    }
                }
            }
        }

    };
    if (this->in_func) {
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
                else
                {
                    mov_val = gen->get_mov_instruc("eax", expr_val.substr(0, expr_val.find_first_of(' ')));
                }
                gen->m_code << "    " << mov_val << " eax," << expr_val << std::endl;
                gen->m_code << "    invoke ExitProcess,eax" << std::endl;

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

            void operator()(const node::_asm_vec* _asm_vec) {
                gen->m_code << "    ";
                for (auto& asm_expr : _asm_vec->asm_asms)
                {
                    gen->gen_asm_expr(asm_expr);
                }
                gen->m_code << std::endl;
            }
            void operator()(const node::_statement_scope* statement_scope) {
                gen->gen_scope(statement_scope);
            }
            void operator()(const node::_main_scope* main_scope) {
                if (!gen->main_proc) {
                    if (!gen->in_func) {
                        gen->in_func = true;
                        gen->m_code << "_main proc\n";
                        gen->main_proc = true;
                        gen->scope_start(true, main_scope->stack_space);
                        for (const node::_statement* stmt : main_scope->scope->statements) {
                            gen->gen_stmt(stmt);
                        }
                        gen->scope_end(true, main_scope->stack_space);
                        gen->in_func = false;
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
                auto it = std::find_if(gen->m_funcs.cbegin(), gen->m_funcs.cend(), [&](const function& fn) {return fn.name == stmt_func->ident.value.value(); });
                if (it != gen->m_funcs.cend()) {
                    std::stringstream ss;
                    ss << "Function with the name '" << stmt_func->ident.value.value() << "' was already declared" << std::endl;
                    gen->line_err(ss.str());
                }
                else {
                    if (!gen->in_func) {
                        gen->in_func = true;
                        std::string save = gen->m_code.str();
                        gen->m_code.str(std::string()); //clear the stringstream
                        function func;
                        func.name = stmt_func->ident.value.value();
                        func.generated = gen->mk_func();
                        gen->m_code << func.generated << " proc" << std::endl;
                        int stack_space_add = 0;
                        for (int i = 0; i < stmt_func->arguments.size(); i++) {
                            stack_space_add += (gen->str_bit_sizes.find(stmt_func->arguments[i]->type)->second) / 8; //get bit size and devide by 8 for bytes
                        }
                        gen->scope_start(true, stmt_func->stack_space + stack_space_add);
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
                            gen->m_code << "    mov" << " " << var.type << " ptr [ebp - " << gen->m_base_ptr_off << "]" << ", " << gen->get_correct_part_of_register(var.type) << std::endl;
                            var.base_pointer_offset = gen->m_base_ptr_off;
                            gen->m_vars.push_back(var);

                            func.arguments.push_back(*stmt_func->arguments[i]);
                        }
                        func.ret_lbl = gen->mk_label();
                        gen->curr_func_name.push_back(func.name);
                        gen->m_funcs.push_back(func);
                        for (const node::_statement* stmt : stmt_func->scope->statements) {
                            gen->gen_stmt(stmt);
                        }
                        gen->m_code << func.ret_lbl << ":" << std::endl;
                        gen->scope_end(true, stmt_func->stack_space + stack_space_add);
                        gen->m_code << "    ret" << std::endl;
                        gen->m_code << func.generated << " endp" << std::endl;
                        gen->m_func_space << gen->m_code.str() << std::endl << std::endl;
                        gen->m_code.str(std::string());
                        gen->m_code << save;
                        gen->in_func = false;
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
            void operator()(const node::_double_op* double_op) {
                auto it = std::find_if(gen->m_vars.cbegin(), gen->m_vars.cend(), [&](const Var& var) {return var.name == double_op->ident.value.value(); });
                if (it == gen->m_vars.cend()) {
                    std::stringstream ss;
                    ss << "Identifier '" << double_op->ident.value.value() << "' was not declared in this scope!" << std::endl;
                    gen->line_err(ss.str());
                }
                std::stringstream ss;
                
                ss << (*it).type << " ptr [ebp - " << (*it).base_pointer_offset << "]";
                if (double_op->op == Token_type::_d_add) {
                    if ((*it).ptr) {
                        gen->m_code << "    add " << ss.str() << ", " << gen->asm_type_to_bytes((*it).ptr_type) << std::endl;
                    }
                    else {
                        gen->m_code << "    inc " << ss.str() << std::endl;
                    }
                }
                else if (double_op->op == Token_type::_d_sub) {
                    if ((*it).ptr) {
                        gen->m_code << "    sub " << ss.str() << ", " << gen->asm_type_to_bytes((*it).ptr_type) << std::endl;
                    }
                    else {
                        gen->m_code << "    dec " << ss.str() << std::endl;
                    }
                }
            }
            void operator()(const node::_op_equal* op_eq) {
                auto it = std::find_if(gen->m_vars.cbegin(), gen->m_vars.cend(), [&](const Var& var) {return var.name == op_eq->ident.value.value(); });
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
                ss << (*it).type << " ptr [ebp - " <<  (*it).base_pointer_offset << "]";
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
                        gen->m_code << "    " << gen->get_mov_instruc("ebx", (*it).type) << " edx, " << ss.str() << std::endl;
                        gen->m_code << "    imul edx,eax" << std::endl;
                        gen->m_code << "    mov " << ss.str() << ", " << gen->get_correct_part_of_register((*it).type,true) << std::endl;
                        break;
                    }
                    case Token_type::_div_eq: {
                        gen->m_code << "    " << gen->get_mov_instruc("ebx", (*it).type) << " edx, " << ss.str() << std::endl;
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
                        gen->m_code << "    invoke StdOut, offset " << val.substr(1) << std::endl;
                    }
                    else {
                        if (is_numeric(val)) {
                            gen->m_code << "    mov eax, " << val << std::endl;
                        }
                        if (val == "&eax") {
                            val = "eax";
                        }
                        else if (val != "eax") { //if val == eax no need to mov eax, eax
                            gen->m_code << "    " << gen->get_mov_instruc("eax", val.substr(0, val.find_first_of(' '))) << " eax," << val << std::endl;
                        }
                        gen->m_code << "    invoke dwtoa,eax,offset buffer ;dword to ascii" << std::endl;
                        gen->m_code << "    invoke StdOut, offset buffer" << std::endl;
                    }
                }
                if (!stmt_output->noend) {
                    gen->m_code << "    invoke StdOut, offset backn" << std::endl;
                }
            }
            void operator()(const node::_statement_input* stmt_input) {
                auto it = std::find_if(gen->m_str_bufs.cbegin(), gen->m_str_bufs.cend(), [&](const string_buffer& var) {return var.name == stmt_input->ident.value.value(); });
                if (it == gen->m_str_bufs.cend()) {
                    std::stringstream ss;
                    ss << "String buffer '" << stmt_input->ident.value.value() << "' was not declared in this scope" << std::endl;
                    gen->line_err(ss.str());
                }
                else {
                    gen->m_code << "    invoke StdIn, offset " << (*it).generated << ", " << (*it).size << std::endl;
                    gen->m_code << "    invoke StdIn, offset buffer, 255 ;clear the rest of the input buffer " << std::endl;
                }
            }

            void operator()(const node::_statement_ret* stmt_ret) {
                if (gen->in_func) {
                    if (gen->curr_func_name.size() < 1) {
                        gen->line_err("Cannot use return in 'brick' function, consider using \"exit [exitcode]\" instead");
                    }
                    else {
                        std::string str_it = gen->curr_func_name.back();
                        auto it = std::find_if(gen->m_funcs.cbegin(), gen->m_funcs.cend(), [&](const function& fn) {return fn.name == str_it; });
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
            void operator()(const node::_statement_pure_expr* pure_expr) {
                std::string expr = gen->gen_expr(pure_expr->expr).value();
                //expression goes to waste
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

        m_output << "\n.data\n";
        if (flags.needs_buffer) {
            m_output << "    buffer dd 256 dup (0)\n";
            m_output << "    backn db 13,10,0 ;ASCII new line char\n";
        }
        m_output << m_data.str();
        m_output << "\n.code\n";
        m_output << m_code.str();
        m_output << "_main endp\n";
        m_output << m_func_space.str();
        m_output << "end _main\n";
        std::cout << "Finished Generating..." << std::endl;
        return m_output.str();
}

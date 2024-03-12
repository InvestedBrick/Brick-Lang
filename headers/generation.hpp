#pragma once
#include <sstream>
//#include <cassert>
#include <algorithm>//for std::find_if
#include "parsing.hpp"

bool is_numeric(const std::string& str);
class Generator {
private:
    struct Var
    {
        std::string name;
        size_t base_pointer_offset;
        std::string type;
        bool bool_limit = false; //ah yes creative naming I know
        bool immutable = false;
        bool ptr = false;
        std::string ptr_type;
    };
    struct string_buffer {
        std::string name;
        std::string generated;
        size_t size;
        bool free = false;
    };
    struct String {
        std::string name;
        std::string str_val;
    };
    struct str_map {
        std::string ident;
        std::string generated;
    };
    struct function {
        std::string name;
        std::string generated;
        std::string ret_lbl;
        std::optional<Token_type> ret_type;
        std::vector<node::_argument> arguments{};
    };
    struct Var_array {
        size_t head_base_pointer_offset;
        size_t size;
        std::string name;
        std::string type;
        bool immutable = false;
        bool bool_limit = false;
    };
    bool main_proc = false;
    bool in_func = false;
    size_t label_counter = 0;
    size_t string_counter = 0;
    size_t string_buffer_counter = 0;
    size_t func_counter = 0;
    const node::_program m_prog;
    std::stringstream m_data;
    std::stringstream m_code;
    std::stringstream m_output;
    std::stringstream m_func_space;
    std::string m_header = ".686p\noption casemap:none\ninclude <C:\\masm32\\include\\masm32rt.inc>\n\n";
    size_t m_stack_ptr = 0;
    size_t m_base_ptr_off = 0;
    size_t line_counter = 1;
    std::vector<std::string> curr_func_name{};
    std::vector<size_t> m_base_ptr_pos{};
    std::vector<Var> m_vars{};
    std::vector<str_map> m_str_map{}; //merge with m_strs
    std::vector<String> m_strs{};
    std::vector<string_buffer> m_str_bufs{};
    std::vector<size_t> m_scopes{};
    std::vector<size_t> m_scope_str_bufs{};
    std::vector<function> m_funcs{};
    std::vector<Var_array> m_arrays{};
    std::vector<size_t> m_scope_arrays{};
    std::vector<std::string> filestack{};
    std::string m_func_registers[4] = {"edi","esi","edx","ecx"};
    std::unordered_map<std::string, size_t> str_bit_sizes = {
    {"eax",32},
    {"ebx",32},
    {"ecx",32},
    {"edx",32},
    {"dword",32},
    {"word",16},
    {"byte",8},
    {"al",8},
    {"bl",8},
    {"cl",8},
    {"dl",8},
    };


    //methods
    void push(const std::string& val);
    void pop(const std::string& reg);
    inline void line_err(const std::string& err_msg);
    inline std::string mk_label();
    inline std::string mk_str_lit();
    inline std::string mk_str_buf();
    inline std::string mk_func();
    inline std::string get_mov_instruc(const std::string& dest, const std::string& source);
    size_t asm_type_to_bytes(std::string str);
    inline std::string get_correct_part_of_register(const std::string& source,bool edx = false);
    inline void scope_start(bool main_or_func_scope = false, size_t alloc = 0);
    inline void scope_end(bool main_or_func_scope = false, size_t alloc = 0);
    inline std::string gen_str_lit(const std::string string_lit);
    inline void gen_scope(const node::_statement_scope* scope);
    inline std::optional<std::string> tok_to_instruc(Token_type type, bool invert = false);

public:
    inline void intern_flags(const node::_null_stmt* null_stmt);
    inline explicit Generator(node::_program root) : m_prog(std::move(root)) {};
    inline std::optional<std::string> gen_term(const node::_term* term);
    inline std::optional<std::string> gen_bin_expr(const node::_bin_expr* bin_expr);
    inline std::optional<std::string> gen_expr(const node::_expr* expr);
    inline void gen_asm_expr(const node::_asm_* _asm_);
    inline void gen_ctrl_statement(const node::_ctrl_statement* _ctrl);
    inline void gen_var_stmt(const node::_statement_var_dec* stmt_var_dec);
    inline void gen_var_set(const node::_statement_var_set* stmt_var_set);
    inline void gen_stmt(const node::_statement* stmt);
    std::string gen_program();
};
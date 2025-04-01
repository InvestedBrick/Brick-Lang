#pragma once
#include <sstream>
#ifdef _WIN32
#include <cassert>
#endif
#include <queue>
#include "parsing.hpp"
bool is_numeric(const std::string& str);
class Generator {
private:
    struct Var
    {
        bool bool_limit = false; //ah yes creative naming I know
        bool immutable = false;
        bool ptr = false;
        bool is_struct_ptr = false;
        size_t base_pointer_offset;
        std::string name;
        std::string ptr_type;
        std::string type;
    };
    struct logic_data_packet{
        std::string end_lbl;
        std::optional<std::string> scope_lbl;
        std::optional<std::string> start_lbl;//currently not in use
        Token_type op;
    };
    struct string_buffer {
        std::string name;
        std::string generated;
        size_t size;
        bool free = false;
    };
    struct String {
        std::string name;
        std::string generated;
        std::string str_val;
    };
    struct function {
        std::string name;
        std::string ret_lbl;
        std::optional<Token_type> ret_type;
        std::vector<node::_argument> arguments{};
        bool ret_type_is_ptr;
    };
    struct Var_array {
        size_t head_base_pointer_offset;
        size_t size;
        std::string name;
        std::string type;
        bool immutable = false;
        bool bool_limit = false;
    };
    struct Struct{
        std::string name;
        std::vector<std::variant<Var,string_buffer,String,Var_array,Struct>> vars; 
    };
    struct Struct_info {
        std::string name;
        std::vector<node::_statement_var_dec*> var_decs; 
        std::vector<std::pair<std::string,size_t>> var_name_to_offset;
        std::vector<node::_var_metadata> var_metadatas;
        std::vector<std::variant<Var,string_buffer,String,Var_array,Struct>> pre_gen_vars;
    
    };

    std::vector<std::variant<Var,string_buffer,String,Var_array,Struct>> m_global_vars;
    
    bool main_proc = false;
    bool modified_expr_regs = false;
    bool valid_space = false;
    bool ctrl_space = false;
    bool only_allow_int_exprs = false;
    bool generating_struct_vars = false;
    bool ignore_var_already_exists = false;
    bool emit_var_gen_asm = true;
    bool overwrite_flags = false; // this is the point when you know, that you have too many flags 
    std::optional<std::string> initial_label_or;
    std::optional<std::string> initial_label_and;
    std::optional<std::string> ending_label;
    std::vector<std::pair<std::string,std::string>> ctrl_labels {};
    size_t label_counter = 0;
    size_t string_counter = 0;
    size_t string_buffer_counter = 0;
    size_t m_base_ptr_off = 0;
    size_t line_counter = 1;
    std::queue<size_t> global_nums;
    const node::_program m_prog;
#ifdef __linux    
    std::stringstream m_bss;
#endif    
    std::stringstream m_output;
    std::stringstream m_data;
    std::stringstream m_code;
    std::stringstream m_func_space;
    std::stringstream m_globals;
#ifdef _WIN32
    std::string m_header = ".686p\noption casemap:none\ninclude <C:\\masm32\\include\\masm32rt.inc>\n\n";
#elif __linux__
    std::string m_header = "global _start\n";
#endif
    std::vector<std::variant<Var,string_buffer,String,Var_array,Struct>>* generic_struct_vars = nullptr;
    std::vector<std::string> curr_func_name{};
    std::vector<size_t> m_base_ptr_pos{};
    std::vector<Var> m_vars{};
    std::vector<String> m_strs{};
    std::vector<string_buffer> m_str_bufs{};
    std::vector<Struct_info> m_struct_infos{};
    std::vector<Struct> m_structs{};
    std::vector<size_t> m_scopes{};
    std::vector<size_t> m_scope_str_bufs{};
    std::vector<function> m_funcs{};
    std::vector<Var_array> m_arrays{};
    std::vector<size_t> m_scope_arrays{};
    std::vector<std::string> filestack{};
    #define CTRL_WHILE true
    #define CTRL_FOR true
    #define CTRL_IF false
    std::vector<bool> ctrl_stack {};
    const std::string m_bin_expr_registers[4] = {"ecx","edx","esi","edi"};
    const std::string m_func_registers[4] = {"edi","esi","edx","ecx"};
    uint m_bin_expr_idx = 0;
    uint m_max_bin_expr_idx = 0;
    std::unordered_map<std::string, size_t> str_bit_sizes = {
    {"eax",32},
    {"ebx",32},
    {"ecx",32},
    {"edx",32},
    {"esi",32},
    {"edi",32},
    {"dword",32},
    {"word",16},
    {"byte",8},
    {"al",8},
    {"bl",8},
    {"cl",8},
    {"dl",8},
    };

    //methods
    inline std::string get_expr_reg();
    inline void free_expr_regs();
    template <typename T>
    void transferElements(std::vector<T>&& source, std::vector<std::variant<Var, string_buffer, String, Var_array, Struct>>& destination, int count);
    void var_set_str();
    template<typename iterator,typename var_set>
    void var_set_str_buf(iterator it,var_set var_str_buf,std::string base_string);
    template<typename iterator,typename var_set>
    void var_set_number(iterator it,var_set var_num,std::string base_string);
    template<typename iterator,typename var_set>
    void var_set_array(iterator it,var_set array_set,std::string base_string);
    template<typename iterator,typename var_set>
    void var_set_struct(iterator it,var_set struct_set,std::string base_string);
    template<typename iterator,typename var_set>
    void var_set_ptr_array(iterator it,var_set array_set,std::string base_string);
    template<typename iterator,typename var_set>
    void var_set_struct_ptr(iterator it,var_set struct_ptr_set,std::string base_string,bool lea = false,bool ret = false);
    inline void line_err(const std::string& err_msg);
    inline std::string mk_label();
    inline std::string mk_str_lit();
    inline std::string mk_str_buf();
    inline void insert_extra_push_pop();
    inline void reset_labels();
#ifdef __linux__
    
    inline void sys_read(size_t size,std::string ecx_and_edi,bool lea_or_mov);
#endif
    template <typename bin_expr_type>
    inline std::string generic_bin_expr(bin_expr_type* bin_expr,std::string operation);

    inline std::string get_mov_instruc(const std::string& dest, const std::string& source);
    inline size_t asm_type_to_bytes(std::string str);
    inline std::string get_correct_part_of_register(const std::string& source,bool edx = false);
    inline void scope_start(bool main_or_func_scope = false, size_t alloc = 0);
    inline void scope_end(bool main_or_func_scope = false, size_t alloc = 0);
    inline std::string gen_str_lit(const std::string string_lit);
    inline void gen_scope(const node::_statement_scope* scope);
    inline std::optional<std::string> tok_to_instruc(Token_type type, bool invert = false);
    
    size_t find_offset(const std::vector<std::pair<std::string, size_t>>& vec,std::string input_string);
    
    template<typename iterator, typename struct_ident>
    std::string gen_term_struct(iterator struct_it, struct_ident struct_ident_,std::string base_string);
    template<typename iterator, typename struct_ptr_ident>
    std::string gen_term_struct_ptr(iterator struct_ptr_it, struct_ptr_ident struct_ptr_ident_,std::string base_string,bool lea = false);
    inline void intern_flags(const node::_null_stmt* null_stmt);
    inline std::optional<std::string> gen_term(const node::_term* term);
    inline std::optional<std::string> gen_bin_expr(const node::_bin_expr* bin_expr);
    inline std::optional<std::string> gen_expr(const node::_expr* expr);

    inline void gen_loop_ctrl(const node::_statement_break_next* break_next);
    inline logic_data_packet gen_logical_expr(const node::_logical_expr* logic_expr,std::optional<std::string> provided_scope_lbl,bool invert = false);
    inline logic_data_packet gen_logical_stmt(const node::_logical_stmt* logic_stmt,std::optional<std::string> provided_scope_lbl,bool invert = false);
    inline void gen_ctrl_statement(const node::_ctrl_statement* _ctrl);
    inline void gen_var_stmt(const node::_statement_var_dec* stmt_var_dec);
    inline void gen_var_set(const node::_statement_var_set* stmt_var_set);
    template <typename variant_item>
    inline void gen_global_vars_recursive(const variant_item last_var);
    inline void gen_global_vars(const node::_statement_globals* globals);
    inline void gen_stmt(const node::_statement* stmt);
public:
    inline explicit Generator(node::_program root) : m_prog(std::move(root)) {};
    std::string gen_program();
};
#pragma once
namespace node {

    struct _expr;
    struct _statement;
    struct _logical_stmt;
    struct _term_int_lit
    {
        Token _int_lit;
    };
    struct _term_str_lit {
        Token _str_lit;
    };
    struct _term_paren {
        _expr* expr;
    };
    struct _term_ident {
        Token ident;
    };
    struct _bin_expr_xor{
        _expr* left;
        _expr* right;
    };
    struct _bin_expr_or{
        _expr* left;
        _expr* right;
    };
    struct _bin_expr_and{
        _expr* left;
        _expr* right;
    };
    struct _bin_expr_add
    {
        _expr* left;
        _expr* right;
    };
    struct _bin_expr_mul
    {
        _expr* left;
        _expr* right;
    };
    struct _bin_expr_sub
    {
        _expr* left;
        _expr* right;
    };
    struct _bin_expr_div
    {
        _expr* left;
        _expr* right;
        bool _modulo;
    };
    struct _bin_expr
    {
        std::variant<_bin_expr_add*, _bin_expr_mul*, _bin_expr_sub*, _bin_expr_div*,_bin_expr_xor*,_bin_expr_or*,_bin_expr_and*> var;
    };
    struct _term_deref {
        Token ident;
    };
    struct _function_call {
        Token ident;
        std::vector<_expr*> arguments{};
    };
    struct _term_negate {
        _expr* expr;
    };
    struct _term_array_index {
        Token ident;
        _expr* index_expr;
    };
    struct _double_op
    {
        Token ident;
        Token_type op;
    };
    struct _term_struct_ident{
        Token ident;
        _term_struct_ident* item = nullptr;
        _expr* index_expr;
    };
    struct _term
    {
        std::variant<_term_int_lit*, _term_ident*, _term_paren*, _term_str_lit*, _function_call*,_term_negate*,_term_deref*, _term_array_index*,_double_op*,_term_struct_ident*> var;
    };
    struct _expr_ref {
        _expr* expr;
    };
    struct _boolean_expr {
        _expr* left;
        _expr* right;
        Token_type op;
    };
    struct _logical_expr_and {
        _logical_stmt* left;
        _logical_stmt* right;
    };
    struct _logical_expr_or {
        _logical_stmt* left;
        _logical_stmt* right;
    };
    struct _logical_expr{
        std::variant<_logical_expr_and*, _logical_expr_or*> var;
    };
    struct _logical_stmt {
        std::variant<_logical_expr*,_boolean_expr*> var;
    };
    struct _expr {
        std::variant<_term*, _bin_expr*, _expr_ref*> var;
    };

    struct _asm_ {
        Token str_lit;
    };
    struct _statement_exit
    {
        _expr* expr;
    };
    struct _var_dec_num {
        Token_type type;
        Token ident;
        _expr* expr;
        bool _const = false;
        bool _ptr = false;
    };
    struct _var_dec_array {
        Token_type type;
        Token ident;
        size_t _array_size;
        bool _const = false;
        std::optional<std::string> init_str;
    };
    struct _var_dec_str {
        Token ident;
        _expr* expr;
    };
    struct _var_dec_str_buf {
        Token ident;
        Token _int_lit;
    };
    struct _var_dec_struct{
        Token ident;
        std::string struct_name;
    };
    struct _var_dec_struct_ptr{
        bool _const;
        Token ident;
        std::string struct_name;
        _expr* expr;
    };
    struct _statement_var_dec
    {
        std::variant<_var_dec_num*, _var_dec_str*, _var_dec_str_buf*, _var_dec_array*,_var_dec_struct*,_var_dec_struct_ptr*> var;
    };
    struct _var_set_num {
        Token ident;
        _expr* expr;
        bool deref = false;
    };
    struct _var_set_array {
        Token ident;
        _expr* expr;
        _expr* index_expr;
    };
    struct _var_set_struct{
        Token ident;
        _var_set_struct* item = nullptr;
        _expr* expr;
        _expr* index_expr = nullptr;
        bool deref = false;

    };
    struct _statement_var_set
    {
        std::variant<_var_set_array*, _var_set_num*,_var_set_struct*> var;
    };

    struct _statement_scope
    {
        std::vector<_statement*> statements;
    };
    struct _statement_while
    {
    };
    struct _if_else {
        _statement_scope* scope;
    };
    struct _statement_if
    {
        std::optional<_if_else*> _else;
    };
    
    struct _op_equal {
        Token ident;
        Token_type op;
        _expr* expr;
    };
    struct _statement_for
    {
        _statement_var_dec* _stmt_var_dec;
        _expr* var_op;
    };
    struct _ctrl_statement
    {
        _logical_stmt* logic;
        _statement_scope* scope;
        Token_type type;
        std::variant<_statement_if*, _statement_while*, _statement_for*> var;

    };
    struct _newline {

    };
    struct _eof {

    };
    struct _newfile {
        std::string filename;
    };
    struct _null_stmt {
        std::variant<_newline*, _newfile*, _eof*> var;
    };
    struct _main_scope
    {
        size_t stack_space;
        _statement_scope* scope;
    };
    struct _statement_output
    {
        std::vector<_expr*> expr_vec;
        bool noend = false;
    };
    struct _statement_input
    {
        Token ident;
    };
    struct _argument {
        std::string type;
        std::string ptr_type;
        Token ident;
        bool _ptr = false;
    };
    struct _statement_function {
        size_t stack_space;
        Token ident;
        _statement_scope* scope;
        std::optional<Token_type> ret_type;
        std::vector<_argument*> arguments{};
        bool ret_type_is_ptr = false;

    };
    struct _statement_pure_expr {
        _expr* expr;
    };
    struct _statement_ret {
        _expr* expr;
    };
    struct _var_metadata{
        bool _ptr;
        bool _const;
        Token_type type;
        size_t _array_size;
        std::string struct_name;
        std::string name;
        std::string variable_kind;
    };
    struct _statement_struct{
        Token ident;
        std::vector<_statement_var_dec*>  vars{};
        int n_lines;
        std::vector<std::pair<std::string,size_t>> name_to_offsets;
        std::vector<_var_metadata> vars_metadata;
    };
    struct _statement
    {
        std::variant<_statement_exit*, _statement_var_dec*, _statement_var_set*, _asm_*, _statement_scope*, _ctrl_statement*, _main_scope*, _null_stmt*, _statement_output*, _statement_input*, _statement_function*, _statement_ret*, _statement_pure_expr*, _op_equal*,_statement_struct*> var;
    };

    struct _program {
        std::vector<_statement*> statements;
    };

}
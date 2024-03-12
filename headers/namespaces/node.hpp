namespace node {

    struct _expr;
    struct _statement;
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
    struct _asm_comma
    {
        Token comma;
    };

    struct _term_ident {
        Token ident;
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
        std::variant<_bin_expr_add*, _bin_expr_mul*, _bin_expr_sub*, _bin_expr_div*> var;
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
    struct _term
    {
        std::variant<_term_int_lit*, _term_ident*, _term_paren*, _term_str_lit*, _function_call*,_term_negate*,_term_deref*, _term_array_index*> var;
    };
    struct _expr_ref {
        _expr* expr;
    };
    struct _expr {
        std::variant<_term*, _bin_expr*, _expr_ref*> var;
    };
    struct _asm_int_lit
    {
        Token _int_lit;
    };
    struct _asm_ident {
        Token ident;
    };
    struct _asm_ {
        std::variant<_asm_int_lit*, _asm_ident*, _asm_comma*> var;
    };
    struct _statement_exit
    {
        _expr* expr;
    };
    struct _asm_vec
    {
        std::vector<_asm_*> asm_asms;
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
    };
    struct _var_dec_str {
        Token ident;
        _expr* expr;
    };
    struct _var_dec_str_buf {
        Token ident;
        Token _int_lit;
    };
    struct _statement_var_dec
    {
        std::variant<_var_dec_num*, _var_dec_str*, _var_dec_str_buf*, _var_dec_array*> var;
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
    struct _statement_var_set
    {
        std::variant<_var_set_array*, _var_set_num*> var;
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
    struct _double_op
    {
        Token ident;
        Token_type op;
    };
    struct _op_equal {
        Token ident;
        Token_type op;
        _expr* expr;
    };
    struct _statement_for
    {
        _statement_var_dec* _stmt_var_dec;
        _double_op* _d_op;
    };
    struct _ctrl_statement
    {
        _expr* expr1;
        _expr* expr2;
        Token_type op;
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

    };
    struct _statement_pure_expr {
        _expr* expr;
    };
    struct _statement_ret {
        _expr* expr;
    };
    struct _statement
    {
        std::variant<_statement_exit*, _statement_var_dec*, _statement_var_set*, _asm_vec*, _statement_scope*, _ctrl_statement*, _double_op*, _main_scope*, _null_stmt*, _statement_output*, _statement_input*, _statement_function*, _statement_ret*, _statement_pure_expr*, _op_equal*> var;
    };

    struct _program {
        std::vector<_statement*> statements;
    };

}
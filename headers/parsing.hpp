#pragma once
#include <variant>
#include "arena_allocator.hpp"
#include "tokenization.hpp"
#include "namespaces/node.hpp"
//sorry for global variable but I need it
struct _flags
{
    bool needs_buffer = false;
};
inline _flags flags;
const int MAX_FUNC_ARGS = 4;
void print_token(Token token);
bool is_data_type(Token_type type);
size_t var_type_to_bytes(Token_type type);
std::string var_type_to_str(Token_type type);
class Parser {
private:
    bool m_debug = false;
    const std::vector<Token> m_tokens{};
    std::vector<std::string> filestack;
    size_t m_idx = 0;
    size_t line_counter = 1;
    bool in_func = false;
    size_t alloc_size = 0;
    Arena_allocator m_Allocator;

    bool is_logical_operator(Token_type type);
    bool is_operator(Token_type type);
    node::_statement* mk_stmt(std::variant<node::_statement_exit*, node::_statement_var_dec*, node::_statement_var_set*, node::_asm_vec*, node::_statement_scope*, node::_ctrl_statement*, node::_main_scope*, node::_null_stmt*, node::_statement_output*, node::_statement_input*, node::_statement_function*, node::_statement_ret*, node::_statement_pure_expr*, node::_op_equal*> var);
    inline node::_null_stmt* mk_null_stmt(std::variant<node::_newline*, node::_newfile*, node::_eof*> var);
    inline void line_err(const std::string& err_msg);
    inline Token try_consume(Token_type type, const std::string& err_msg);
    inline std::optional<Token> try_consume(Token_type type);
    inline std::optional<Token> peek(size_t offset = 0) const;
    inline Token consume();
    inline bool peek_type(Token_type type, size_t offset = 0);
    inline std::optional<node::_argument*> try_parse_argument();

public:
    inline explicit Parser(std::vector<Token> tokens) : m_tokens(std::move(tokens)),
        m_Allocator(1024 * 1024 * 2) //2 Megabytes
    {}
    inline std::optional<node::_boolean_expr*> parse_boolean_expr();
    inline std::optional<node::_logical_stmt*> parse_logical_stmt();
    inline std::optional<node::_term*> parse_term();
    inline std::optional<node::_expr*> parse_expr(int min_prec = 0);
    inline std::optional<node::_statement_var_dec*> parse_var_dec();
    inline std::optional<node::_asm_*> parse_asm();
    inline std::optional<node::_statement_scope*> parse_scope();
    inline std::optional<node::_double_op*> parse_d_op();
    inline std::optional<node::_statement*>parse_statement();
    std::optional<node::_program> parse_program();
};


/*

    (expr == expr) and (expr != expr) and (x < y)
     boolean_expr  and  boolean_expr and boolean_expr ...
     -------> logical_expr , logical_expr <--------


*/

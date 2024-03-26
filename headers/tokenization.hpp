#pragma once
#include <iostream>
#include <string>
#include <vector>
#include <optional>
#include <unordered_map>
enum class Token_type {
    _exit,
    _int_lit,
    _semicolon,
    _open_paren,
    _close_paren,
    _ident,
    _dec,
    _equal,
    _open_cur_brac,
    _close_cur_brac,
    _open_sq_brac,
    _close_sq_brac,
    _comma,
    _asm_tok,
    _add,
    _sub,
    _mul,
    _div,
    _set,
    _if,
    _same_as,
    _not_same_as,
    _greater_as,
    _greater_eq_as,
    _less_as,
    _less_eq_as,
    _d_add,
    _d_sub,
    _while,
    _main_scope,
    _for,
    _back_n,
    _else,
    _mod,
    _output,
    _right_arrow,
    _int,
    _short,
    _byte,
    _bool,
    _str_lit,
    _string,
    _const,
    _noend,
    _str_buffer,
    _input,
    _func,
    _colon,
    _eq_right_arrow,
    _return,
    _ptr,
    _ampersand,
    _deref,
    _array,
    _add_eq,
    _sub_eq,
    _mul_eq,
    _div_eq,
    _new_file,
    _eof,
    _logical_and,
    _logical_or
};
inline std::unordered_map<Token_type, std::string> tokenTypeToString = {
    {Token_type::_exit, "_exit"},
    {Token_type::_int_lit, "_int_lit"},
    {Token_type::_semicolon, "_semicolon"},
    {Token_type::_open_paren, "_open_paren"},
    {Token_type::_close_paren, "_close_paren"},
    {Token_type::_ident, "_identifier"},
    {Token_type::_dec, "_dec"},
    {Token_type::_equal, "_equals"},
    {Token_type::_open_cur_brac, "_open_cur_brac"},
    {Token_type::_close_cur_brac, "_close_cur_brac"},
    {Token_type::_open_sq_brac, "_open_sq_brac"},
    {Token_type::_close_sq_brac, "_close_sq_brac"},
    {Token_type::_comma, "_comma"},
    {Token_type::_asm_tok, "_ASM"},
    {Token_type::_add, "_add"},
    {Token_type::_sub, "_sub"},
    {Token_type::_mul, "_mul"},
    {Token_type::_div, "_div"},
    {Token_type::_set, "_set"},
    {Token_type::_if, "_if"},
    {Token_type::_same_as, "_same_as"},
    {Token_type::_not_same_as, "_not_same_as"},
    {Token_type::_greater_as, "_greater_as"},
    {Token_type::_greater_eq_as, "_greater_eq_as"},
    {Token_type::_less_as, "_less_as"},
    {Token_type::_less_eq_as, "_less_eq_as"},
    {Token_type::_d_add, "_d_add"},
    {Token_type::_d_sub, "_d_sub"},
    {Token_type::_while, "_while"},
    {Token_type::_main_scope, "_main scope"},
    {Token_type::_for, "_for"},
    {Token_type::_back_n, "_back_n"},
    {Token_type::_else, "_else"},
    {Token_type::_mod, "_mod"},
    {Token_type::_output, "_output"},
    {Token_type::_right_arrow, "_right arrow"},
    {Token_type::_int, "_int"},
    {Token_type::_short, "_short"},
    {Token_type::_byte, "_byte"},
    {Token_type::_bool, "_bool"},
    {Token_type::_str_lit, "_str_lit"},
    {Token_type::_string, "_string"},
    {Token_type::_const, "_const"},
    {Token_type::_noend, "_noend"},
    {Token_type::_str_buffer, "_str_buffer"},
    {Token_type::_input, "_input"},
    {Token_type::_func, "FUNCTION---------------------------------------         _func"},
    {Token_type::_colon, "_colon"},
    {Token_type::_eq_right_arrow, "_eq_right_arrow"},
    {Token_type::_return, "_return"},
    {Token_type::_ptr, "_ptr"},
    {Token_type::_ampersand, "_ampersand"},
    {Token_type::_deref, "_deref"},
    {Token_type::_array, "_array"},
    {Token_type::_add_eq, "_add_eq"},
    {Token_type::_sub_eq, "_sub_eq"},
    {Token_type::_mul_eq, "_mul_eq"},
    {Token_type::_div_eq, "_div_eq"},
    {Token_type::_new_file, "_new_file"},
    {Token_type::_eof, "_eof"},
    {Token_type::_logical_and, "_logical_and"},
    {Token_type::_logical_or, "_logical_or"},

};
std::optional<int> bin_prec(Token_type type);
struct Token
{
    Token_type type;
    std::optional<std::string> value{};
};

Token mk_tok(Token_type tok_type, std::optional<std::string> value = {});

class Tokenizer {
private:
    size_t line_counter = 1;
    const std::string m_str;
    std::vector<std::string> filestack{};
    size_t m_idx = 0;
    inline std::optional<char> peek(int offset = 0) const;
    inline char consume();
    void line_err(const std::string& err_msg);
public:
    inline explicit Tokenizer(std::string str) : m_str(std::move(str)){}

    std::vector<Token> tokenize();

};

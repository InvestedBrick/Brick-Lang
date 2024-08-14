#include "headers/tokenization.hpp"
std::optional<int> bin_prec(Token_type type) {
    switch (type)
    {
    case Token_type::_add:
        return 1;
    case Token_type::_sub:
        return 1;
    case Token_type::_mul:
        return 2;
    case Token_type::_div:
        return 2;
    case Token_type::_mod:
        return 2;
    default:
        return {};
    }
}
Token mk_tok(Token_type tok_type, std::optional<std::string> value) {
    Token t;
    t.type = tok_type;
    if (value.has_value()) {
        t.value = value;
    }
    return t;
}

inline std::optional<char> Tokenizer::peek(int offset) const {
    if (m_idx + offset >= m_str.length()) {
        return std::nullopt;
    }
    else {
        return m_str.at(this->m_idx + offset);
    }
}
inline char Tokenizer::consume() {
    return m_str.at(this->m_idx++);
}
void Tokenizer::line_err(const std::string& err_msg) {
    std::cerr << "Code failed in line " << this->line_counter << "(" << this->filestack.back() << "): " << err_msg << std::endl;
    exit(EXIT_FAILURE);
}

std::vector<Token> Tokenizer::tokenize()
{
    std::vector<Token> token_arr;
    std::string buf;
    while (peek().has_value())
    {
        if (std::isalpha(peek().value()) || peek().value() == '_')
        {
            buf.push_back(consume());
            while (peek().has_value() && std::isalnum(peek().value()) || peek().has_value() && peek().value() == '_')
            {
                buf.push_back(consume());
            }

            if (buf == "exit") {
                token_arr.push_back(mk_tok(Token_type::_exit));
                buf.clear();
                continue;
            }
            else if (buf == "dec") {
                token_arr.push_back(mk_tok(Token_type::_dec));
                buf.clear();
                continue;
            }
            else if (buf == "set") {
                token_arr.push_back(mk_tok(Token_type::_set));
                buf.clear();
                continue;
            }
            else if (buf == "as" || buf == "to") {
                token_arr.push_back(mk_tok(Token_type::_equal));
                buf.clear();
                continue;
            }
            else if (buf == "else") {
                token_arr.push_back(mk_tok(Token_type::_else));
                buf.clear();
                continue;
            }
            else if (buf == "if") {
                token_arr.push_back(mk_tok(Token_type::_if));
                buf.clear();
                continue;
            }
            else if (buf == "for") {
                token_arr.push_back(mk_tok(Token_type::_for));
                buf.clear();
                continue;
            }
            else if (buf == "__asm__") {
                token_arr.push_back(mk_tok(Token_type::_asm_tok));
                buf.clear();
                continue;
            }
            else if (buf == "while") {
                token_arr.push_back(mk_tok(Token_type::_while));
                buf.clear();
                continue;
            }
            else if (buf == "main") {
                token_arr.push_back(mk_tok(Token_type::_main_scope));
                buf.clear();
                continue;
            }
            else if (buf == "output") {
                token_arr.push_back(mk_tok(Token_type::_output));
                buf.clear();
                continue;
            }
            else if (buf == "input") {
                token_arr.push_back(mk_tok(Token_type::_input));
                buf.clear();
                continue;
            }
            else if (buf == "return") {
                token_arr.push_back(mk_tok(Token_type::_return));
                buf.clear();
                continue;
            }
            else if (buf == "int") {
                token_arr.push_back(mk_tok(Token_type::_int));
                buf.clear();
                continue;
            }
            else if (buf == "short") {
                token_arr.push_back(mk_tok(Token_type::_short));
                buf.clear();
                continue;
            }
            else if (buf == "byte") {
                token_arr.push_back(mk_tok(Token_type::_byte));
                buf.clear();
                continue;
            }
            else if (buf == "bool") {
                token_arr.push_back(mk_tok(Token_type::_bool));
                buf.clear();
                continue;
            }
            else if (buf == "string") {
                token_arr.push_back(mk_tok(Token_type::_string));
                buf.clear();
                continue;
            }
            else if (buf == "strbuf") {
                token_arr.push_back(mk_tok(Token_type::_str_buffer));
                buf.clear();
                continue;
            }
            else if (buf == "noend") {
                token_arr.push_back(mk_tok(Token_type::_noend));
                buf.clear();
                continue;
            }
            else if (buf == "brick") {
                token_arr.push_back(mk_tok(Token_type::_func));
                buf.clear();
                continue;
            }
            else if (buf == "ptr") {
                token_arr.push_back(mk_tok(Token_type::_ptr));
                buf.clear();
                continue;
            }
            else if (buf == "and") {
                token_arr.push_back(mk_tok(Token_type::_logical_and));
                buf.clear();
                continue;
            }
            else if (buf == "or") {
                token_arr.push_back(mk_tok(Token_type::_logical_or));
                buf.clear();
                continue;
            }
            else if (buf == "array") {
                token_arr.push_back(mk_tok(Token_type::_array));
                buf.clear();
                continue;
            }
            else if (buf == "const") {
                token_arr.push_back(mk_tok(Token_type::_const));
                buf.clear();
                continue;
            }
            else if (buf == "bundle") {
                token_arr.push_back(mk_tok(Token_type::_struct));
                buf.clear();
                continue;
            }
            else if (buf == "true") {
                token_arr.push_back(mk_tok(Token_type::_int_lit, "1"));
                buf.clear();
                continue;
            }
            else if (buf == "false") {
                token_arr.push_back(mk_tok(Token_type::_int_lit, "0"));
                buf.clear();
                continue;
            }
            else if (buf == "null") {
                token_arr.push_back(mk_tok(Token_type::_int_lit, "0"));
                buf.clear();
                continue;
            }
            else if (buf == "FILE") {
                std::string buffer;
                consume(); //consume the empty space after FILE
                while (peek().value() != ' ') {
                    buffer.push_back(consume());
                }
                token_arr.push_back(mk_tok(Token_type::_new_file, buffer));
                this->filestack.push_back(buffer);
                buf.clear();
                continue;
                }
            else if (buf == "EOF") {
                token_arr.push_back(mk_tok(Token_type::_eof));
                this->filestack.pop_back();
                this->line_counter = 1;
                buf.clear();
                continue;
            }
            else {
                token_arr.push_back(mk_tok(Token_type::_ident, buf));
                buf.clear();
                continue;
                
            }

        }
        else if (std::isdigit(peek().value()))
        {
            buf.push_back(consume());
            while (peek().has_value() && std::isdigit(peek().value())) {
                buf.push_back(consume());
            }
            token_arr.push_back(mk_tok(Token_type::_int_lit, buf));
            buf.clear();
            continue;
        }

    switch (peek().value()) {
    case '(':
        {consume();
        token_arr.push_back(mk_tok(Token_type::_open_paren));
        break;}
    case '{':
        {consume();
        token_arr.push_back(mk_tok(Token_type::_open_cur_brac));
        break;}
    case '}':
        {consume();
        token_arr.push_back(mk_tok(Token_type::_close_cur_brac));
        break;}
    case '[':
        {consume();
        token_arr.push_back(mk_tok(Token_type::_open_sq_brac));
        break;}
    case ']':
        {consume();
        token_arr.push_back(mk_tok(Token_type::_close_sq_brac));
        break;}
    case ')':
        {consume();
        token_arr.push_back(mk_tok(Token_type::_close_paren));
        break;}
    case ';':
        {consume();
        token_arr.push_back(mk_tok(Token_type::_semicolon));
        break;}
    case ':':
        {consume();
        token_arr.push_back(mk_tok(Token_type::_colon));
        break;}
    case '.':
        {consume();
        token_arr.push_back(mk_tok(Token_type::_dot));
        break;}
    case '+':
        {consume();
        if (peek().value() == '+') {
            consume();
            token_arr.push_back(mk_tok(Token_type::_d_add));
        } else if (peek().value() == '=') {
            consume();
            token_arr.push_back(mk_tok(Token_type::_add_eq));
        } else {
            token_arr.push_back(mk_tok(Token_type::_add));
        }
        break;}
    case '-':
        {consume();
        if (peek().value() == '-') {
            consume();
            token_arr.push_back(mk_tok(Token_type::_d_sub));
        } else if (peek().value() == '>') {
            consume();
            token_arr.push_back(mk_tok(Token_type::_right_arrow));
        } else if (peek().value() == '=') {
            consume();
            token_arr.push_back(mk_tok(Token_type::_sub_eq));
        } else {
            token_arr.push_back(mk_tok(Token_type::_sub));
        }
        break;}
    case '*':
        {consume();
        if (peek().value() == '=') {
            consume();
            token_arr.push_back(mk_tok(Token_type::_mul_eq));
        } else {
            token_arr.push_back(mk_tok(Token_type::_mul));
        }
        break;}
    case '/':
        {if (peek(1).has_value() && peek(1).value() == '*') {
            consume();
            consume();
            while (true) {
                if (peek().value() == '\n') {
                    token_arr.push_back(mk_tok(Token_type::_back_n));
                    this->line_counter++;
                } else if (peek().has_value() && peek().value() == '*' && peek(1).has_value() && peek(1).value() == '/') {
                    break;
                }
                consume();
            }
            consume();
            consume();
        } else if (peek(1).has_value() && peek(1).value() == '/') {
            consume();
            consume();
            while (peek().has_value() && peek().value() != '\n') {
                consume();
            }
            consume();
            token_arr.push_back(mk_tok(Token_type::_back_n));
            this->line_counter++;
        } else if (peek(1).value() == '=') {
            consume();
            token_arr.push_back(mk_tok(Token_type::_div_eq));
        } else {
            consume();
            token_arr.push_back(mk_tok(Token_type::_div));
        }
        break;}
    case '%':
        {consume();
        token_arr.push_back(mk_tok(Token_type::_mod));
        break;}
    case '&':
        {consume();
        token_arr.push_back(mk_tok(Token_type::_ampersand));
        break;}
    case ',':
        {consume();
        token_arr.push_back(mk_tok(Token_type::_comma, ","));
        break;}
    case '$':
        {consume();
        token_arr.push_back(mk_tok(Token_type::_deref));
        break;}
    case '!':
        {if (peek(1).value() == '=') {
            consume();
            consume();
            token_arr.push_back(mk_tok(Token_type::_not_same_as));
        } else {
            line_err("Invalid Syntax!");
            exit(1);
        }
        break;}
    case '=':
        {consume();
        if (peek().value() == '=') {
            consume();
            token_arr.push_back(mk_tok(Token_type::_same_as));
        } else if (peek().value() == '>') {
            consume();
            token_arr.push_back(mk_tok(Token_type::_eq_right_arrow));
        } else {
            token_arr.push_back(mk_tok(Token_type::_equal));
        }
        break;}
    case '>':
        {consume();
        if (peek().value() == '=') {
            consume();
            token_arr.push_back(mk_tok(Token_type::_greater_eq_as));
        } else {
            token_arr.push_back(mk_tok(Token_type::_greater_as));
        }
        break;}
    case '<':
        {consume();
        if (peek().value() == '=') {
            consume();
            token_arr.push_back(mk_tok(Token_type::_less_eq_as));
        } else {
            token_arr.push_back(mk_tok(Token_type::_less_as));
        }
        break;}
    case '"':
        {consume();
        buf.push_back(consume());
        while (peek().has_value() && peek().value() != '"') {
            buf.push_back(consume());
        }
        consume();
        token_arr.push_back(mk_tok(Token_type::_str_lit, buf));
        buf.clear();
        break;}
    case '\'':
        {consume();
        const char c = consume();
        if(peek().value() != '\''){line_err("Can only have single character between ' ' ");}
        token_arr.push_back(mk_tok(Token_type::_int_lit,std::to_string(static_cast<int>(c))));
        consume();
        buf.clear();
        break; }
    case '\n':
        {consume();
        token_arr.push_back(mk_tok(Token_type::_back_n));
        this->line_counter++;
        break;}
    default:
        {if (std::isspace(peek().value())) {
            consume();
        } else {
            line_err("Invalid Syntax!");
            exit(EXIT_FAILURE);
        }
        break;}
    }

    }
    this->m_idx = 0;
    

    return token_arr;
}

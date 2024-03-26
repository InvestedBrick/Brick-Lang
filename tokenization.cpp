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
    exit(1);
}

std::vector<Token> Tokenizer::tokenize()
{
    std::vector<Token> token_arr;
    std::string buf;
    while (peek().has_value())
    {
        if (std::isalpha(peek().value()))
        {
            buf.push_back(consume());
            while (peek().has_value() && std::isalnum(peek().value()) || peek().has_value() && peek().value() == '_')
            {
                buf.push_back(consume());
            }

            if (buf == "exit") {
                token_arr.push_back(mk_tok(Token_type::_exit));
                buf.clear();
            }
            else if (buf == "dec") {
                token_arr.push_back(mk_tok(Token_type::_dec));
                buf.clear();
            }
            else if (buf == "set") {
                token_arr.push_back(mk_tok(Token_type::_set));
                buf.clear();
            }
            else if (buf == "as" || buf == "to") {
                token_arr.push_back(mk_tok(Token_type::_equal));
                buf.clear();
            }
            else if (buf == "else") {
                token_arr.push_back(mk_tok(Token_type::_else));
                buf.clear();
            }
            else if (buf == "if") {
                token_arr.push_back(mk_tok(Token_type::_if));
                buf.clear();
            }
            else if (buf == "for") {
                token_arr.push_back(mk_tok(Token_type::_for));
                buf.clear();
            }
            else if (buf == "ASM") {
                token_arr.push_back(mk_tok(Token_type::_asm_tok));
                buf.clear();
            }
            else if (buf == "while") {
                token_arr.push_back(mk_tok(Token_type::_while));
                buf.clear();
            }
            else if (buf == "main") {
                token_arr.push_back(mk_tok(Token_type::_main_scope));
                buf.clear();
            }
            else if (buf == "output") {
                token_arr.push_back(mk_tok(Token_type::_output));
                buf.clear();
            }
            else if (buf == "input") {
                token_arr.push_back(mk_tok(Token_type::_input));
                buf.clear();
            }
            else if (buf == "return") {
                token_arr.push_back(mk_tok(Token_type::_return));
                buf.clear();
            }
            else if (buf == "int") {
                token_arr.push_back(mk_tok(Token_type::_int));
                buf.clear();
            }
            else if (buf == "short") {
                token_arr.push_back(mk_tok(Token_type::_short));
                buf.clear();
            }
            else if (buf == "byte") {
                token_arr.push_back(mk_tok(Token_type::_byte));
                buf.clear();
            }
            else if (buf == "bool") {
                token_arr.push_back(mk_tok(Token_type::_bool));
                buf.clear();
            }
            else if (buf == "string") {
                token_arr.push_back(mk_tok(Token_type::_string));
                buf.clear();
            }
            else if (buf == "strbuf") {
                token_arr.push_back(mk_tok(Token_type::_str_buffer));
                buf.clear();
            }
            else if (buf == "noend") {
                token_arr.push_back(mk_tok(Token_type::_noend));
                buf.clear();
            }
            else if (buf == "brick") {
                token_arr.push_back(mk_tok(Token_type::_func));
                buf.clear();
            }
            else if (buf == "ptr") {
                token_arr.push_back(mk_tok(Token_type::_ptr));
                buf.clear();
            }
            else if (buf == "and") {
                token_arr.push_back(mk_tok(Token_type::_logical_and));
                buf.clear();
            }
            else if (buf == "or") {
                token_arr.push_back(mk_tok(Token_type::_logical_or));
                buf.clear();
            }
            else if (buf == "array") {
                token_arr.push_back(mk_tok(Token_type::_array));
                buf.clear();
            }
            else if (buf == "const") {
                token_arr.push_back(mk_tok(Token_type::_const));
                buf.clear();
            }
            else if (buf == "true") {
                token_arr.push_back(mk_tok(Token_type::_int_lit, "1"));
                buf.clear();
            }
            else if (buf == "false") {
                token_arr.push_back(mk_tok(Token_type::_int_lit, "0"));
                buf.clear();
            }
            else if (buf == "null") {
                token_arr.push_back(mk_tok(Token_type::_int_lit, "0"));
                buf.clear();
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
                }
            else if (buf == "EOF") {
                token_arr.push_back(mk_tok(Token_type::_eof));
                this->filestack.pop_back();
                this->line_counter = 1;
                buf.clear();
            }
            else {
                token_arr.push_back(mk_tok(Token_type::_ident, buf));
                buf.clear();
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
        }

        else if (peek().value() == '(') {
            consume();
            token_arr.push_back(mk_tok(Token_type::_open_paren));
        }
        else if (peek().value() == '{') {
            consume();
            token_arr.push_back(mk_tok(Token_type::_open_cur_brac));
        }
        else if (peek().value() == '}') {
            consume();
            token_arr.push_back(mk_tok(Token_type::_close_cur_brac));
        }
        else if (peek().value() == '[') {
            consume();
            token_arr.push_back(mk_tok(Token_type::_open_sq_brac));
        }
        else if (peek().value() == ']') {
            consume();
            token_arr.push_back(mk_tok(Token_type::_close_sq_brac));
        }
        else if (peek().value() == ')') {
            consume();
            token_arr.push_back(mk_tok(Token_type::_close_paren));
        }
        else if (peek().value() == ';')
        {
            consume();
            token_arr.push_back(mk_tok(Token_type::_semicolon));
        }
        else if (peek().value() == ':')
        {
            consume();
            token_arr.push_back(mk_tok(Token_type::_colon));
        }
        else if (peek().value() == '+')
        {
            consume();
            if (peek().value() == '+') {
                consume();
                token_arr.push_back(mk_tok(Token_type::_d_add));
            }
            else if (peek().value() == '=') {
                consume();
                token_arr.push_back(mk_tok(Token_type::_add_eq));
            }
            else {
                token_arr.push_back(mk_tok(Token_type::_add));
            }
        }
        else if (peek().value() == '-')
        {
            consume();
            if (peek().value() == '-') {
                consume();
                token_arr.push_back(mk_tok(Token_type::_d_sub));
            }
            else if (peek().value() == '>') {
                consume();
                token_arr.push_back(mk_tok(Token_type::_right_arrow));
            }
            else if (peek().value() == '=') {
                consume();
                token_arr.push_back(mk_tok(Token_type::_sub_eq));
            }
            else {
                token_arr.push_back(mk_tok(Token_type::_sub));
            }
        }
        else if (peek().value() == '*')
        {
            consume();
            if (peek().value() == '=') {
                consume();
                token_arr.push_back(mk_tok(Token_type::_mul_eq));
            }
            else {
                token_arr.push_back(mk_tok(Token_type::_mul));
            }
        }
        else if (peek().value() == '/')
        {
            if (peek(1).has_value() && peek(1).value() == '*') {
                consume();  //consume the starting characters
                consume();
                while (true)
                {
                    if (peek().value() == '\n') {
                        token_arr.push_back(mk_tok(Token_type::_back_n));
                        this->line_counter++;
                    }
                    //it works, I dont know why, if I test in the while statements it doesnt, dark fucking magic is at the work here
                    else if (peek().has_value() && peek().value() == '*' && peek(1).has_value() && peek(1).value() == '/') {
                        break;
                    }
                    consume();
                }
                consume();//consume the ending characters
                consume();
            }
            else if (peek(1).has_value() && peek(1).value() == '/') {
                consume();  //consume the starting characters
                consume();
                while (peek().has_value() && peek().value() != '\n')
                {
                    consume();
                }
                consume(); //consume the new line
                token_arr.push_back(mk_tok(Token_type::_back_n));
                this->line_counter++;
            }
            else if (peek(1).value() == '=') {
                consume();
                token_arr.push_back(mk_tok(Token_type::_div_eq));
            }
            else {
                consume();
                token_arr.push_back(mk_tok(Token_type::_div));
            }

        }
        else if (peek().value() == '%')
        {
            consume();
            token_arr.push_back(mk_tok(Token_type::_mod));
        }      
        else if (peek().value() == '&')
        {
            consume();
            token_arr.push_back(mk_tok(Token_type::_ampersand));
            
        }
        else if (peek().value() == ',')
        {
            std::string str = ",";
            consume();
            token_arr.push_back(mk_tok(Token_type::_comma, str));
        }
        else if (peek().value() == '$')
        {
            consume();
            token_arr.push_back(mk_tok(Token_type::_deref));
        }
        else if (peek().value() == '!' && peek(1).value() == '=') {
            consume();
            consume();
            token_arr.push_back(mk_tok(Token_type::_not_same_as));
        }
        else if (peek().value() == '=')
        {
            consume();
            if (peek().value() == '=') {
                consume();
                token_arr.push_back(mk_tok(Token_type::_same_as));
            }
            else if (peek().value() == '>') {
                consume();
                token_arr.push_back(mk_tok(Token_type::_eq_right_arrow));
            }
            else {
                token_arr.push_back(mk_tok(Token_type::_equal));
            }
        }
        else if (peek().value() == '>')
        {
            consume();
            if (peek().value() == '=') {
                consume();
                token_arr.push_back(mk_tok(Token_type::_greater_eq_as));
            }
            else {
                token_arr.push_back(mk_tok(Token_type::_greater_as));
            }
        }
        else if (peek().value() == '<')
        {
            consume();
            if (peek().value() == '=') {
                consume();
                token_arr.push_back(mk_tok(Token_type::_less_eq_as));
            }
            else {
                token_arr.push_back(mk_tok(Token_type::_less_as));
            }
        }
        else if (peek().value() == '"') {
            consume();
            buf.push_back(consume());
            while (peek().has_value() && peek().value() != '"') {
                buf.push_back(consume());
            }
            consume();
            token_arr.push_back(mk_tok(Token_type::_str_lit, buf));
            buf.clear();
        }
        else if (peek().value() == '\n') {
            consume();
            token_arr.push_back(mk_tok(Token_type::_back_n));
            this->line_counter++;
        }
        else if (std::isspace(peek().value())) {
            consume();
        }
        else {
            line_err("Invalid Syntax!");
            exit(1);
        }
    }
    this->m_idx = 0;
    std::cout << "Finished Tokenizing..." << std::endl;

    return token_arr;
}

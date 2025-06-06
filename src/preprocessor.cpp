/*
    The Preprocessor for the Brick Programming Language
    Copyright (C) 2024  Julian Brecht

    view main.cpp for more copyright related information

*/
#include "headers/preprocessor.hpp"
inline std::optional<char> PreProcessor::peek(int offset) const {
    if (m_idx + offset >= m_str.length()) {
        return std::nullopt;
    }
    else {
        return m_str.at(this->m_idx + offset);
    }
}
inline char PreProcessor::consume() {
    return m_str.at(this->m_idx++);
}

void PreProcessor::line_err(const std::string& err_msg) {
    std::cerr << "Code failed in line " << this->line_counter << /*"(" << this->filestack.back() << ")*/": " << err_msg << std::endl;
    exit(1);
}

int PreProcessor::consume_spaces(){
    int spaces_consumed = 0;
    while(peek().has_value() && peek().value() == ' '){
        spaces_consumed++;
        consume();
    }
    return spaces_consumed;
}

void replace_all(std::string& str, std::string& target, std::string& replacement, size_t start_idx = 0){
    auto idx = str.find(target, start_idx);
    if(idx == std::string::npos){
        return;
    }
    bool left_ok = true, right_ok = true;
    if(idx > 0) {
        if(isalnum(str[idx - 1]) || str[idx - 1] == '_') left_ok = false;
    }
    // Check right boundary
    if(idx + target.length() < str.length()) {
        if(isalnum(str[idx + target.length()]) || str[idx + target.length()] == '_') right_ok = false;
    }

    if(left_ok && right_ok){
        str.erase(idx, target.length());
        str.insert(idx, replacement);
        replace_all(str, target, replacement, idx + replacement.length());
    } else {
        replace_all(str, target, replacement, idx + target.length());
    }
    return;

}

inline void PreProcessor::pre_process_directives() {
    while (peek().has_value())
    {
        if (peek().value() == '\n') {
            consume();
            line_counter++;
        }
        else if (peek().value() == '#') {
            size_t index = m_idx;
            consume();
            std::string buf;
            while (peek().has_value() && peek().value() != ' ')//consume until a space char
            {
                if (peek().value() == '\n') {
                    line_err("Invalid Preprocessor Argument");
                }
                buf.push_back(consume());
            }
            consume(); //consume empty space
            if (buf == "include") {
                buf.clear();
                while (peek().has_value() && peek().value() != '\n' && peek().value() != ' ') { //consume Filename until end of line etc
                    buf.push_back(consume());
                }
                bool add_stdlib = false;
                //this is to get rid of things in stdlib from including each other and messing that up

                std::string path = ""; // adjust for when calling the compiler from another dir
                auto r_idx = filename.rfind("/");
                if (r_idx != std::string::npos){
                    path += filename.erase(r_idx + 1);
                }
                path = path + buf;
                std::cout << path << std::endl;
                std::ifstream f(path);   
                if (!f.good()) {
                    std::ifstream ifs("stdlib/"+path);
                    if (!ifs.good()){
                        line_err("Input file was not found!");
                    }
                    add_stdlib = true;
                    
                }
                std::string contents;
                {
                    std::stringstream contents_stream;
                    std::fstream input(add_stdlib ? "stdlib/" + path: path, std::ios::in);
                    contents_stream << input.rdbuf();//read the file to a string
                    contents = contents_stream.str();
                }
                contents += "EOF";
                this->m_str.erase(index, 9 + buf.length());//include + space + hastag + length of buffer to be removed
                this->m_str.insert(index, contents); //insert contents of file
                this->m_str.insert(index, " FILE " + buf + ' ');
                //std::cout << this->m_str << std::endl;
                //this->filestack.push_back(buf);
            }
            else if (buf == "define"){
                buf.clear();
                std::string expr;
                int spaces_consumed = 0;
                spaces_consumed += consume_spaces();
                // We dont want numbers in our define vars
                while(peek().has_value() && (std::isalpha(peek().value()) || peek().value() == '_')){
                    buf.push_back(consume());
                }

                // spaces between var and expr
                spaces_consumed += consume_spaces();
                
                //expr until end of line
                while(peek().has_value() && peek().value() != '\n'){
                    expr.push_back(consume());
                }

                this->m_str.erase(index,8 + spaces_consumed + buf.length() + expr.length());
                // replace all instances of buf with expr which are not part of a var name
                replace_all(this->m_str,buf,expr);

            }
            else {
                line_err("Invalid PreProcessor Argument");
            }
        }
        else {
            consume();
        }
    }
}
void PreProcessor::rem_included_main_funcs() {
    if (this->m_str.length() < 4) return;
    
    size_t idx = this->m_str.rfind("brick main");
    if (idx == std::string::npos) return;

    while (true) {
        size_t prev_idx = this->m_str.rfind("brick main", idx - 1);
        if (prev_idx == std::string::npos) break;
        
        size_t func_start = this->m_str.find('{', prev_idx);
        if (func_start == std::string::npos) break;

        size_t func_end = func_start;
        int brace_count = 1;
        while (brace_count > 0 && func_end < this->m_str.length() - 1) {
            func_end++;
            if (this->m_str[func_end] == '{') brace_count++;
            else if (this->m_str[func_end] == '}') brace_count--;
        }

        if (brace_count == 0) {
            this->m_str.erase(prev_idx, func_end - prev_idx + 1);
        } else {
            break;
        }

        // Update idx to the position of the last kept main function
        idx = this->m_str.rfind("brick main");
        if (idx == std::string::npos) break;
    }
}

std::string PreProcessor::pre_process() {
    this->m_str.insert(0, " FILE " + this->filename + ' ');
    while (this->m_str.find('#') != std::string::npos) {
        this->m_idx = 0;
        this->pre_process_directives();

    }   
    this->rem_included_main_funcs();
    return this->m_str;
}
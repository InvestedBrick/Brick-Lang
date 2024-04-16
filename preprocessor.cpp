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
    std::cerr << "Code failed in line " << this->line_counter << /*"(" << this->filestack.back() << ")�*/": " << err_msg << std::endl;
    exit(1);
}
inline void PreProcessor::pre_process_include() {
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
                std::ifstream f(buf);
                if (!f.good()) {
                    line_err("Input file was not found!");
                }
                std::string contents;
                {
                    std::stringstream contents_stream;
                    std::fstream input(buf, std::ios::in);
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
            else {
                line_err("Invalid PreProcessor Argument");
            }
        }
        else {
            consume();
        }
    }
}

std::string PreProcessor::pre_process() {
    this->m_str.insert(0, " FILE " + this->filename + ' ');
    while (this->m_str.find('#') != std::string::npos) {
        this->m_idx = 0;
        this->pre_process_include();

    }   
    return this->m_str;
}
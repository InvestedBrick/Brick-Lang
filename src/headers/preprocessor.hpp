#pragma once
#include <string>
#include <optional>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
class PreProcessor {
private:
	std::string m_str;
	//std::vector<std::string> filestack{};
	std::string filename;
	size_t line_counter = 1;
	size_t m_idx = 0;
	inline void pre_process_directives();
	void rem_included_main_funcs();
	inline std::optional<char> peek(int offset = 0) const;
	inline char consume();
	void line_err(const std::string& err_msg);
	int consume_spaces();
public:

	inline explicit PreProcessor(std::string str, std::string filename) : m_str(std::move(str)) { this->filename = filename; }

	std::string pre_process();
};

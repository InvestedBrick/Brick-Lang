#pragma once
#include <string>
#include <vector>
#include <optional>
#include <algorithm>
class Optimizer{
private:
    int op_level = 0;
    std::string m_asm_code {};

    struct function_info{
        std::string name = "";
        size_t start_idx{};
        size_t end_idx{};
        bool used = false;
    };

    struct idx_pair{
        size_t idx;
        size_t func_end;
    };

    std::vector<function_info> func_infos;
    std::vector<idx_pair> call_indices;
    size_t m_idx;

    inline std::optional<char> peek(int offset = 0);
    inline char consume();
    inline void index_functions();
    inline void mark_funcs();
    inline std::string rem_unused_funcs();
    void peek_window(int n); //temporary
public:

    inline explicit Optimizer(std::string generated_asm_code,int optimization_level) : m_asm_code(std::move(generated_asm_code)){
        this->op_level = optimization_level;
    };

    std::string optimize();

};
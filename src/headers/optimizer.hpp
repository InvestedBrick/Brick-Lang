#pragma once
#include <string>
#include <vector>
#include <optional>
#include <algorithm>
class Optimizer{
private:

    enum class OpType{
        _mov,
        _lea,
        _push,
        _pop,
        _ret,
        _add,
        _sub,
        _mul,
        _div,
        _call,
        _movsx,
        _movzx,
        _int,
        _cmp,
        _jmp,
        _test,
        _inc,
        _dec,
        _xor,
        _or,
        _and,
        _shr,
        _shl,
        _neg,
        _not

    };

    enum class OperandType{
        _data_offset, // stuff like dword [ebp - x]
        _int_lit,
        _register,
        _byte_ref, // strings etc
        _label
    };


    struct Operation{
        OpType op_type;
        OperandType operand_1;
        OperandType operand_2;
        uint idx;
        std::optional<std::string> op_1{};
        std::optional<std::string> op_2{};
    };

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

    std::vector<function_info> func_infos {};
    std::vector<idx_pair> call_indices {};
    std::vector<Operation> operations {};
    size_t m_idx;

    inline std::optional<char> peek(int offset = 0);
    inline char consume();
    inline void index_functions();
    inline void mark_funcs();
    inline void rem_unused_funcs();
    inline void tokenize_asm();
    inline void jmp_spaces();
    inline std::string consume_until_char_and_consume_char(char c);
    void peek_window(int n); //temporary
public:

    inline explicit Optimizer(std::string generated_asm_code,int optimization_level) : m_asm_code(std::move(generated_asm_code)){
        this->op_level = optimization_level;
    };

    std::string optimize();

};
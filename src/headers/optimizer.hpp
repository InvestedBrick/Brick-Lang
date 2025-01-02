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
        _not,
        _imul,
        _idiv,
        _label

    };

    std::string opTypeToString(OpType opType) {
    switch (opType) {
        case OpType::_mov:   return "mov";
        case OpType::_lea:   return "lea";
        case OpType::_push:  return "push";
        case OpType::_pop:   return "pop";
        case OpType::_ret:   return "ret";
        case OpType::_add:   return "add";
        case OpType::_sub:   return "sub";
        case OpType::_mul:   return "mul";
        case OpType::_div:   return "div";
        case OpType::_call:  return "call";
        case OpType::_movsx: return "movsx";
        case OpType::_movzx: return "movzx";
        case OpType::_int:   return "int";
        case OpType::_cmp:   return "cmp";
        case OpType::_jmp:   return "jmp";
        case OpType::_test:  return "test";
        case OpType::_inc:   return "inc";
        case OpType::_dec:   return "dec";
        case OpType::_xor:   return "xor";
        case OpType::_or:    return "or";
        case OpType::_and:   return "and";
        case OpType::_shr:   return "shr";
        case OpType::_shl:   return "shl";
        case OpType::_neg:   return "neg";
        case OpType::_not:   return "not";
        case OpType::_imul:   return "_imul";
        case OpType::_idiv:   return "_idiv";
        default:             exit(EXIT_FAILURE); // should not happen
    }
}

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
        bool erased = false;
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
    inline void optimize_tokens();
    inline void reassemble_asm();
    inline std::string consume_until_char_and_consume_char(char c);
    bool is_end_operation(OpType op);
    void peek_window(int n); //temporary
public:

    inline explicit Optimizer(std::string generated_asm_code,int optimization_level) : m_asm_code(std::move(generated_asm_code)){
        this->op_level = optimization_level;
    };

    std::string optimize();

};
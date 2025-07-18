/*
    The Parser for the Brick Programming Language
    Copyright (C) 2024  Julian Brecht

    view main.cpp for more copyright related information

*/
#include "headers/parsing.hpp"
#include <sstream>

bool is_data_type(Token_type type) {
    switch (type)
    {
    case Token_type::_int:
        return true;
    case Token_type::_short:
        return true;
    case Token_type::_byte:
        return true;
    case Token_type::_bool:
        return true;
    default:
        return false;
    }
}
size_t var_type_to_bytes(Token_type type) {
    switch (type)
    {
    case Token_type::_int:
        return 4;
        break;
    case Token_type::_short:
        return 2;
        break;
    case Token_type::_byte:
        return 1;
        break;
    case Token_type::_bool:
        return 1;
        break;
    default:
        return 4; //to make g++ happy
        break;
    }
}
std::string var_type_to_str(Token_type type) {
    switch (type)
    {
    case Token_type::_int:
        return "dword";
        break;
    case Token_type::_short:
        return "word";
        break;
    case Token_type::_byte:
        return "byte";
        break;
    case Token_type::_bool:
        return "byte";
        break;
    default:
        return "dword"; 
        break;
    }
}
#ifdef DEBUG
void print_token(Token token) {
    auto it = tokenTypeToString.find(token.type);
    if (it != tokenTypeToString.end()) {
        std::cout << it->second << '\n';
    }
    else {
        std::cout << "Could not identify TokenType\n";
    }
}
#endif

bool Parser::is_operator(Token_type type) {
    switch (type)
    {
    case Token_type::_same_as:
        return true;
    case Token_type::_not_same_as:
        return true;
    case Token_type::_greater_eq_as:
        return true;
    case Token_type::_greater_as:
        return true;
    case Token_type::_less_as:
        return true;
    case Token_type::_less_eq_as:
        return true;
    default:
        return false;
    }
}
bool Parser::is_logical_operator(Token_type type) {
    switch (type)
    {
    case Token_type::_logical_and:
        return true;
    case Token_type::_logical_or:
        return true;
    default:
        return false;
    }
}
node::_statement* Parser::mk_stmt(std::variant<
  node::_statement_exit*
, node::_statement_var_dec*
, node::_statement_var_set*
, node::_asm_*
, node::_statement_scope*
, node::_ctrl_statement*
, node::_main_scope*
, node::_null_stmt*
, node::_statement_output*
, node::_statement_input*
, node::_statement_function*
, node::_statement_ret*
, node::_statement_pure_expr*
, node::_op_equal*
, node::_statement_struct*
, node::_statement_globals*
, node::_statement_break_next*
> var)
{
    auto stmt = m_Allocator.alloc<node::_statement>();
    stmt->var = var;
    return stmt;
}
inline void Parser::line_err(const std::string& err_msg) {
    std::cerr << "Code failed in line " << this->line_counter << "(" << this->filestack.back() << "): " << err_msg << std::endl;
    exit(1);
}
inline Token Parser::try_consume(Token_type type, const std::string& err_msg) {
    if (peek().has_value() && peek().value().type == type) {
        return consume();
    }
    else {
        line_err(err_msg);
        exit(1); //doesnt do anything, but g++ complains
    }
}
inline std::optional<Token> Parser::try_consume(Token_type type) {
    if (peek().has_value() && peek().value().type == type) {
        return consume();
    }
    else {
        return {};
    }
}
inline std::optional<Token> Parser::peek(size_t offset) const {
    if (m_idx + offset >= m_tokens.size()) {
        return {};
    }
    else {
        return m_tokens.at(this->m_idx + offset);
    }
}
inline Token Parser::consume() {
    return m_tokens.at(this->m_idx++);
}
inline std::optional<node::_argument*> Parser::try_parse_argument() {
    if (peek().has_value() && is_data_type(peek().value().type)) {
        auto arg = m_Allocator.alloc<node::_argument>();

        std::string waste = var_type_to_str(consume().type); //doesnt work when I do direct assignment idk why
        arg->type = waste;
        if (try_consume(Token_type::_ptr)) {
            arg->_ptr = true;
        }
        try_consume(Token_type::_right_arrow, "Expected '->'");
        arg->ident = try_consume(Token_type::_ident, "Expected Identifier");
        return arg;
    }
    else {
        return {};
    }
}
inline bool Parser::peek_type(Token_type type, size_t offset) {
    if (peek(offset).has_value() && peek(offset).value().type == type) {
        return true;
    }
    return false;
}
inline std::optional<node::_term*> Parser::parse_term() {
    auto term = m_Allocator.alloc< node::_term>();
    if (peek_type(Token_type::_globals) && peek_type(Token_type::_colon,1) && peek_type(Token_type::_colon,2) && peek_type(Token_type::_ident,3)){
        consume();
        consume();
        consume();
        term->var_is_global = true;
    }
    if (auto int_lit = try_consume(Token_type::_int_lit)) {
        auto il = m_Allocator.alloc<node::_term_int_lit>();
        il->_int_lit = int_lit.value();
        term->var = il;
        return term;
    }
    else if (auto str_lit = try_consume(Token_type::_str_lit)) {
        auto strl = m_Allocator.alloc<node::_term_str_lit>();
        strl->_str_lit = str_lit.value();
        term->var = strl;
        return term;
    }
    else if (peek_type(Token_type::_ident) && peek_type(Token_type::_open_sq_brac, 1)) { // check for arrays first
        auto arr_idx = m_Allocator.alloc<node::_term_array_index>();
        arr_idx->ident = consume();
        consume();
        if (const auto expr = parse_expr()) {
            arr_idx->index_expr = expr.value();
        }
        else {
            line_err("Invalid Expression");
        }
        try_consume(Token_type::_close_sq_brac, "Expected ']'");
        term->var = arr_idx;
        return term;
    }
    else if (peek_type(Token_type::_ident) && (peek_type(Token_type::_d_add, 1) || peek_type(Token_type::_d_sub, 1))) {
        auto _d_op = parse_d_op();
        if (!_d_op.has_value()) {
            line_err("Unable to parse double operation");
        }
        term->var = _d_op.value();
        return term;
    }
    else if (peek_type(Token_type::_ident) && peek_type(Token_type::_dot,1))//it's a struct
    {
        auto term_struct = m_Allocator.alloc<node::_term_struct_ident>();
        term_struct->ident = consume();
        node::_term_struct_ident* current = term_struct;

        while(try_consume(Token_type::_dot)){ //traverse the struct(s)
            auto next_struct = m_Allocator.alloc<node::_term_struct_ident>();
            next_struct->ident = try_consume(Token_type::_ident,"Expected Identifier");
            current->item = next_struct;
            current = next_struct;
        }

        if (try_consume(Token_type::_open_sq_brac)) {
            if (const auto expr = parse_expr()) {
                term_struct->index_expr = expr.value();
            }
            else {
                line_err("Invalid expression");
            }
            try_consume(Token_type::_close_sq_brac, "Expected ']'");
        }

        term->var = term_struct;
        return term;
    }
    else if (peek_type(Token_type::_ident) && !peek_type(Token_type::_open_paren, 1)) { // check to ensure, that it's not a function
        auto ident = consume();
        auto id = m_Allocator.alloc<node::_term_ident>();
        id->ident = ident;
        term->var = id;
        return term;
    }
    else if (try_consume(Token_type::_open_paren)) {
        auto term_paren = m_Allocator.alloc<node::_term_paren>();
        const auto expr = parse_expr();
        if (!expr.has_value()) {
            line_err("Invalid Expression");
        }
        try_consume(Token_type::_close_paren, "Expected ')'");
        term_paren->expr = expr.value();
        term->var = term_paren;
        return term;
    }
    else if (peek_type(Token_type::_sub)) {
        consume();
        auto neg = m_Allocator.alloc<node::_term_negate>();
        if (const auto expr = parse_expr()) {
            neg->expr = expr.value();
        }
        else {
            line_err("Invalid Expression");
        }
        term->var = neg;
        return term;
    }
    else if (peek_type(Token_type::_bitwise_not)) {
        consume();
        auto not_ = m_Allocator.alloc<node::_term_bitwise_not>();
        if (const auto expr = parse_expr()) {
            not_->expr = expr.value();
        }
        else {
            line_err("Invalid Expression");
        }
        term->var = not_;
        return term;
    }
    else if (peek_type(Token_type::_deref)) {
        consume();
        auto deref = m_Allocator.alloc<node::_term_deref>();
        if (peek_type(Token_type::_globals) && peek_type(Token_type::_colon,1) && peek_type(Token_type::_colon,2)){
        consume();
        consume();
        consume();
        term->var_is_global = true;
    }
        deref->ident = try_consume(Token_type::_ident, "Expected variable name after '$'");
        term->var = deref;
        return term;
    }
    else if (peek_type(Token_type::_ident) && peek_type(Token_type::_open_paren, 1)) {
        auto call = m_Allocator.alloc<node::_function_call>();
        call->ident = try_consume(Token_type::_ident, "Expected function name");
        try_consume(Token_type::_open_paren, "Expected '('");
        while (const auto expr = parse_expr()) {
            call->arguments.push_back(expr.value());
            if (try_consume(Token_type::_comma)) {
                continue;
            }
            else {
                break;
            }
        }
        if (call->arguments.size() > MAX_FUNC_ARGS) {
            std::stringstream ss;
            ss << "Functions with more than " << MAX_FUNC_ARGS << " arguments are currently not supported";
            line_err(ss.str());
        }

        try_consume(Token_type::_close_paren, "Expected ')'");
        term->var = call;
        return term;
    }
    else {
        return {};
    }
}

inline std::optional<node::_expr*> Parser::parse_expr(int min_prec) {

    std::optional<node::_term*> term_left = parse_term();
    const auto expr_left = m_Allocator.alloc<node::_expr>();
    if (!term_left.has_value()) { // try for ampersand
        if (try_consume(Token_type::_ampersand)) {
            auto ref = m_Allocator.alloc<node::_expr_ref>();
            if (const auto expr = parse_expr()) {
                ref->expr = expr.value();
            }
            else {
                line_err("Expected Expression after '&'");
            }
            expr_left->var = ref;
        }
        else {
            return {};
        }
    }
    else {
        expr_left->var = term_left.value();
    }
    while (true)
    {
        std::optional<Token> curr_tok = peek();
        std::optional<int> prec;
        if (curr_tok.has_value()) {
            prec = bin_prec(curr_tok->type);
            if (!prec.has_value() || prec < min_prec) {
                break;
            }
        }
        else {
            break;
        }
        Token op = consume();
        int next_min_prec = prec.value() + 1;
        const auto expr_right = parse_expr(next_min_prec);

        if (!expr_right.has_value()) {
            line_err("Unable to parse expression");
        }
        auto bin_expr = m_Allocator.alloc<node::_bin_expr>();

        const auto expr_left_2 = m_Allocator.alloc<node::_expr>();//sorry for the naming
        switch (op.type){
            case Token_type::_add: {
                // I have decided to not make each of these an individual function, because too many arguments would be needed
                auto add = m_Allocator.alloc<node::_bin_expr_add>();
                expr_left_2->var = expr_left->var;
                add->left = expr_left_2;
                add->right = expr_right.value();
                bin_expr->var = add;
                break;
            }
            case Token_type::_sub: {
                auto sub = m_Allocator.alloc<node::_bin_expr_sub>();
                expr_left_2->var = expr_left->var;
                sub->left = expr_left_2;
                sub->right = expr_right.value();
                bin_expr->var = sub;
                break;
            }
            case Token_type::_mul: {
                auto mul = m_Allocator.alloc<node::_bin_expr_mul>();
                expr_left_2->var = expr_left->var;
                mul->left = expr_left_2;
                mul->right = expr_right.value();
                bin_expr->var = mul;
                break;
            }
            case Token_type::_div: 
            case Token_type::_mod: {
                auto div = m_Allocator.alloc<node::_bin_expr_div>();
                expr_left_2->var = expr_left->var;
                div->left = expr_left_2;
                div->right = expr_right.value();
                if (op.type == Token_type::_mod) {
                    div->_modulo = true;
                }
                else {
                    div->_modulo = false;
                }
                bin_expr->var = div;
                break;
            }
            case Token_type::_xor:{
                auto xor_ = m_Allocator.alloc<node::_bin_expr_xor>();
                expr_left_2->var = expr_left->var;
                xor_->left = expr_left_2;
                xor_->right = expr_right.value();
                bin_expr->var = xor_;
                break;
            }
            case Token_type::_or:{
                auto or_ = m_Allocator.alloc<node::_bin_expr_or>();
                expr_left_2->var = expr_left->var;
                or_->left = expr_left_2;
                or_->right = expr_right.value();
                bin_expr->var = or_;
                break;
            }
            case Token_type::_ampersand:{
                auto and_ = m_Allocator.alloc<node::_bin_expr_and>();
                expr_left_2->var = expr_left->var;
                and_->left = expr_left_2;
                and_->right = expr_right.value();
                bin_expr->var = and_;
                break;
            }
            case Token_type::_shift_left:{
                auto shl = m_Allocator.alloc<node::_bin_expr_shift_left>();
                expr_left_2->var = expr_left->var;
                shl->left = expr_left_2;
                shl->right = expr_right.value();
                bin_expr->var = shl;
                break;
            }
            case Token_type::_shift_right:{
                auto shr = m_Allocator.alloc<node::_bin_expr_shift_right>();
                expr_left_2->var = expr_left->var;
                shr->left = expr_left_2;
                shr->right = expr_right.value();
                bin_expr->var = shr;
                break;
            }
        }

        expr_left->var = bin_expr;
    }
    return expr_left;
}
inline std::optional<node::_boolean_expr*> Parser::parse_boolean_expr() {
    auto bool_expr = m_Allocator.alloc<node::_boolean_expr>();
    if(const auto expr = parse_expr()){
        bool_expr->left = expr.value();
    }else{
        line_err("Invalid expression");
    }
    if(peek().has_value() && is_operator(peek().value().type)){
        bool_expr->op = consume().type;
    }else{
        line_err("Expected operator");
    }
    if(const auto expr = parse_expr()){
        bool_expr->right = expr.value();
        
    }else{
        line_err("Invalid expression");
    }
    return bool_expr;

}
template<typename Node>
inline void Parser::parse_expr_and_set(Node& node, bool isArray) {
    
    if constexpr (std::is_same<std::remove_pointer_t<Node>,node::_var_set_struct>::value || std::is_same<std::remove_pointer_t<Node>, node::_var_set_array>::value) {
        if (isArray) {
            if (const auto expr = parse_expr()) {
                node->index_expr = expr.value();
            }
            else {
                line_err("Invalid expression");
            }
            try_consume(Token_type::_close_sq_brac, "Expected ']'");
        }
    }
    try_consume(Token_type::_equal, "Expected equal sign");
    if (const auto expr = parse_expr()) {
        node->expr = expr.value();
    }
    else {
        line_err("Invalid expression");
    }
    try_consume(Token_type::_semicolon, "Expected ';'");
}
inline std::optional<node::_logical_stmt*> Parser::parse_logical_stmt() {
    //If you are confused about the variable naming, then you feel how I felt writing this code
    std::optional<node::_boolean_expr*> bool_expr = parse_boolean_expr();
    auto logical_left = m_Allocator.alloc<node::_logical_stmt>();
    if (!bool_expr.has_value()) {
        return {};
    }else{
        logical_left->var = bool_expr.value();
    }
    while(true){
        Token_type logic_op;
        if(!(peek_type(Token_type::_logical_and) || peek_type(Token_type::_logical_or))){
            break;
        }else{
            logic_op = consume().type;
        }
        auto logical_right = parse_logical_stmt();
        if (!logical_right.has_value()) {
            line_err("Unable to parse expression");
        }
        auto logic_expr = m_Allocator.alloc<node::_logical_expr>();

        auto logical_left_2 = m_Allocator.alloc<node::_logical_stmt>();
        if(logic_op == Token_type::_logical_and){
            auto logic_and = m_Allocator.alloc<node::_logical_expr_and>();
            logical_left_2->var = logical_left->var;
            logic_and->left = logical_left_2;
            logic_and->right = logical_right.value();
            logic_expr->var = logic_and;
        }
        else if(logic_op == Token_type::_logical_or){
            auto logic_or = m_Allocator.alloc<node::_logical_expr_or>();
            logical_left_2->var = logical_left->var;
            logic_or->left = logical_left_2;
            logic_or->right = logical_right.value();
            logic_expr->var = logic_or;
        }
        logical_left->var = logic_expr;
    }
    return logical_left;
}
inline node::_statement_var_dec* Parser::parse_var_dec() {
    consume();
    auto it = this->struct_name_alloc_map.find(peek().value().value.value());//ah yes peak programming
    if(it != this->struct_name_alloc_map.end()){
        line_err("Cannot use struct name as identifier");
    }
    auto stmt_dec = m_Allocator.alloc<node::_statement_var_dec>();
    if (!peek(2).has_value()){
        this->line_err("Could not find Variable type");
    }
    Token t = peek(2).value(); // peek token and check if its a variable type
    if (t.type == Token_type::_int || t.type == Token_type::_short || t.type == Token_type::_byte || t.type == Token_type::_bool) {
        auto var_num = m_Allocator.alloc<node::_var_dec_num>();
        var_num->ident = consume();
        
        if (add_struct_info){
            if ((*generic_name_to_offsets).empty()){
                struct_size = -var_type_to_bytes(t.type);
            }
            struct_size += var_type_to_bytes(t.type); 
            (*generic_name_to_offsets).push_back(std::make_pair(var_num->ident.value.value(),struct_size));
        }
        if (in_func) {
            alloc_size += var_type_to_bytes(t.type);
        }
        var_num->type = t.type;
        consume();
        consume();//consume the peeked token
        if (try_consume(Token_type::_ptr)) {
            var_num->_ptr = true;
            if (t.type == Token_type::_short) {
                alloc_size += 2;
            }
            else if (t.type == Token_type::_byte || t.type == Token_type::_bool) {
                alloc_size += 3;
            }
        }
        try_consume(Token_type::_right_arrow, "Expected '->'");
        if (const auto expr = parse_expr()) {
            var_num->expr = expr.value();
        }
        else {
            line_err("Invalid expression");
        }
        if (try_consume(Token_type::_const))
        {
            var_num->_const = true;
        }

        if(add_struct_info){
            node::_var_metadata item;
            item.name = var_num->ident.value.value();
            item._ptr = var_num->_ptr;
            item._const = var_num->_const;
            item.type = var_num->type;
            item.variable_kind = "number";
            generic_var_metadata->push_back(item);
        }
        
        stmt_dec->var = var_num;
    }
    else if (t.type == Token_type::_array) {
        auto var_array = m_Allocator.alloc<node::_var_dec_array>();
        var_array->ident = consume();
        consume();
        consume();
        if (is_data_type(peek().value().type)) {
            var_array->type = consume().type;
        }
        else {
            line_err("Expected data type of array");
        }
        try_consume(Token_type::_right_arrow, "Expected '->'");
        var_array->_array_size = std::stoi(try_consume(Token_type::_int_lit, "Expected integer literal for size of array").value.value());
        if (var_array->_array_size < 1) {
            line_err("Array size 0 or negative");
        }
        if (add_struct_info){
            if ((*generic_name_to_offsets).empty()){
                struct_size = -(var_array->_array_size * var_type_to_bytes(var_array->type));
            }
            struct_size += var_array->_array_size * var_type_to_bytes(var_array->type); 
            (*generic_name_to_offsets).push_back(std::make_pair(var_array->ident.value.value(),struct_size));
        }
#ifdef __linux__        
        //initialization of byte array with a string
        if(var_array->type == Token_type::_byte && try_consume(Token_type::_open_cur_brac)){
            if(!peek_type(Token_type::_str_lit)){
                line_err("Expected string literal to initialize byte array with");
            }

            var_array->init_str = consume().value.value();
            try_consume(Token_type::_close_cur_brac,"Expected '}'");
        }
#endif
        if (try_consume(Token_type::_const))
        {
            var_array->_const = true;
        }
        if (in_func) {
            alloc_size += var_array->_array_size * var_type_to_bytes(var_array->type);
        }

        if(add_struct_info){
            node::_var_metadata item;        
            item._const = var_array->_const;
            item.name = var_array->ident.value.value();
            item.type = var_array->type;
            item._array_size = var_array->_array_size;
            item.variable_kind = "array";
            generic_var_metadata->push_back(item);
        }
        

        stmt_dec->var = var_array;
    }
    else if (t.type == Token_type::_string) {
        auto var_str = m_Allocator.alloc<node::_var_dec_str>();
        var_str->ident = consume();
        consume();
        consume();
        try_consume(Token_type::_right_arrow, "Expected '->'");
        if (const auto expr = parse_expr()) {
            var_str->expr = expr.value();
        }
        else {
            line_err("Invalid expression");
        }
        stmt_dec->var = var_str;
    }
    else if (t.type == Token_type::_str_buffer) {
        auto str_buf = m_Allocator.alloc<node::_var_dec_str_buf>();
        str_buf->ident = consume();
        consume();
        consume();//consume the peeked token
        try_consume(Token_type::_right_arrow, "Expected '->'");
        str_buf->_int_lit = try_consume(Token_type::_int_lit, "Expected integer literal for size of string buffer!");
        stmt_dec->var = str_buf;
    }
    else if (t.type == Token_type::_ident){
        const auto it = this->struct_name_alloc_map.find(t.value.value());
        

        if (peek(3).has_value() && peek(3).value().type == Token_type::_ptr){
            
            std::string struct_name;
            if(it == this->struct_name_alloc_map.end() ){
                if(!this->struct_names.empty() && this->struct_names.back() == t.value.value()){
                    struct_name = t.value.value();
                }else{
                    line_err("Invalid Struct name");
                }
            }else{
                struct_name = (*it).first;
            }
            auto var_struct_ptr = m_Allocator.alloc<node::_var_dec_struct_ptr>();

            var_struct_ptr->ident = consume();
            if (add_struct_info){
                if (generic_name_to_offsets->empty()){
                    struct_size = -4;
                }
                struct_size += 4;
                generic_name_to_offsets->push_back(std::make_pair(var_struct_ptr->ident.value.value(),struct_size));
            }

            if(in_func){
                alloc_size += 4;
            }
            
            var_struct_ptr->struct_name = struct_name;
            consume(); // =
            consume(); // struct name
            consume(); // ptr

            try_consume(Token_type::_right_arrow, "Expected '->'");

            if (const auto expr = parse_expr()) {
                var_struct_ptr->expr = expr.value();
            }
            else {
                line_err("Invalid expression");
            }

            if (try_consume(Token_type::_const))
            {
                var_struct_ptr->_const = true;
            }
            if(add_struct_info){
                node::_var_metadata item;
                item.name = var_struct_ptr->ident.value.value();
                item._ptr = true;
                item._const = var_struct_ptr->_const;
                item.type = Token_type::_int;
                item.struct_name = var_struct_ptr->struct_name;
                item.variable_kind = "struct ptr";
                generic_var_metadata->push_back(item);
            }
            stmt_dec->var = var_struct_ptr;

        }else{
            if (it == this->struct_name_alloc_map.end()){
                line_err("Invalid Struct name");
            }
            auto var_struct = m_Allocator.alloc<node::_var_dec_struct>();
            var_struct->ident = consume();
            if (add_struct_info){
                generic_name_to_offsets->push_back(std::make_pair(var_struct->ident.value.value(),alloc_size));
            }

            if (in_func){
                alloc_size += (*it).second;
            }

            var_struct->struct_name = (*it).first;
            consume();
            consume();
            if(add_struct_info){
                node::_var_metadata item;
                item.name = var_struct->ident.value.value();
                item.struct_name = var_struct->struct_name;
                item.variable_kind = "struct";
                generic_var_metadata->push_back(item);
            }
            stmt_dec->var = var_struct;
        }
    }
    else
    {
        line_err("Expected variable type");
    }

    try_consume(Token_type::_semicolon, "Expected ';'");
    return stmt_dec;
}

inline node::_statement_var_set* Parser::parse_var_set(){
    consume();
    auto stmt_set = m_Allocator.alloc<node::_statement_var_set>();
    bool deref = false;
    if (try_consume(Token_type::_deref)) {
        deref = true;
    }
    Token t;

    if (peek_type(Token_type::_globals) && peek_type(Token_type::_colon,1) && peek_type(Token_type::_colon,2)){
        consume();
        consume();
        consume();
        stmt_set->var_is_global = true;
    }

    if(peek_type(Token_type::_ident) && (peek_type(Token_type::_dot,1) )){
        auto set_struct = m_Allocator.alloc<node::_var_set_struct>();
        set_struct->ident = consume();
        node::_var_set_struct* current = set_struct;
        while(try_consume(Token_type::_dot)){
            auto next_struct = m_Allocator.alloc<node::_var_set_struct>();
            next_struct->ident = try_consume(Token_type::_ident,"Expected Identifier");
            current->item = next_struct;
            current = next_struct;
        }

        set_struct->deref = deref;
        bool is_array = false;
        if(try_consume(Token_type::_open_sq_brac)){
            is_array = true;
        }

        parse_expr_and_set(set_struct, is_array);
        stmt_set->var = set_struct;
        return stmt_set;
    }



    t = try_consume(Token_type::_ident,"Expected Indentifier");

    if (try_consume(Token_type::_open_sq_brac)) {
        auto set_array = m_Allocator.alloc<node::_var_set_array>();
        set_array->ident = t;
        parse_expr_and_set(set_array, true);
        stmt_set->var = set_array;
    }
    else {
        auto set_num = m_Allocator.alloc<node::_var_set_num>();
        set_num->ident = t;
        set_num->deref = deref;
        parse_expr_and_set(set_num,false);
        stmt_set->var = set_num;
    }
    return stmt_set;
}

inline std::optional<node::_statement_scope*> Parser::parse_scope() {
    if (!try_consume(Token_type::_open_cur_brac).has_value()) { return {}; }

    auto scope = m_Allocator.alloc<node::_statement_scope>();
    while (auto stmt = parse_statement()) {
        scope->statements.push_back(stmt.value());
    }
    try_consume(Token_type::_close_cur_brac, "Expected '}'");
    return scope;
}
inline std::optional<node::_double_op*> Parser::parse_d_op() {
    auto _d_op = m_Allocator.alloc<node::_double_op>();
    _d_op->ident = consume();
    _d_op->op = consume().type;
    return _d_op;
}
inline node::_null_stmt* Parser::mk_null_stmt(std::variant<node::_newline*, node::_newfile*, node::_eof*> var) {
    auto stmt = m_Allocator.alloc<node::_null_stmt>();
    stmt->var = var;
    return stmt;
}

inline std::optional<node::_statement*> Parser::parse_statement() {
    if (peek_type(Token_type::_back_n)) {
        auto newline = m_Allocator.alloc<node::_newline>();
        consume();
        this->line_counter++;
        return mk_stmt(mk_null_stmt(newline));
    }
    else if (peek_type(Token_type::_eof)) {
        auto EOF_ = m_Allocator.alloc<node::_eof>();
        consume();
        this->filestack.pop_back();
        this->line_counter = 1;
        return mk_stmt(mk_null_stmt(EOF_));
    }
    else if (peek_type(Token_type::_new_file)) {
        auto newfile = m_Allocator.alloc<node::_newfile>();
        std::string file = consume().value.value();
        this->filestack.push_back(file);
        newfile->filename = file;
        return mk_stmt(mk_null_stmt(newfile));
    }
    else if (peek_type(Token_type::_exit)) {
        consume();
        auto stmt_exit = m_Allocator.alloc<node::_statement_exit>();
        if (auto node_expr = parse_expr()) {
            stmt_exit->expr = node_expr.value();
        }
        else {
            line_err("Invalid expression");
        }
        try_consume(Token_type::_semicolon, "Expected ';'");
        return mk_stmt(stmt_exit);
    }
    /*
        I hereby declared the use of assembly tokens as extremely unsafe and if you use them then that's your risk
    */
    else if (peek_type(Token_type::_asm_tok)) {
        consume();
        auto stmt_asm = m_Allocator.alloc< node::_asm_>();
        if (try_consume(Token_type::_volatile)){
            stmt_asm->is_volatile = true;    
        }
        try_consume(Token_type::_open_cur_brac, "Expected '{'");
        stmt_asm->str_lit = try_consume(Token_type::_str_lit, "Expected string literal");
        try_consume(Token_type::_close_cur_brac, "Expected '}'");
        try_consume(Token_type::_semicolon, "Expected ';'");
        return mk_stmt(stmt_asm);
    }
    else if (peek_type(Token_type::_ident) && (peek_type(Token_type::_add_eq, 1) || peek_type(Token_type::_sub_eq, 1) || peek_type(Token_type::_mul_eq, 1) || peek_type(Token_type::_div_eq, 1))) {
        auto _op_eq = m_Allocator.alloc<node::_op_equal>();
        _op_eq->ident = consume();
        _op_eq->op = consume().type;
        if (const auto expr = parse_expr()) {
            _op_eq->expr = expr.value();
        }
        else {
            line_err("Unable to parse expression");
        }
        try_consume(Token_type::_semicolon, "Expected ';'");
        return mk_stmt(_op_eq);
    }
    else if (peek_type(Token_type::_dec) &&
        peek_type(Token_type::_ident, 1) &&
        peek_type(Token_type::_equal, 2))
    {

        return mk_stmt(parse_var_dec());

    }
    else if (peek_type(Token_type::_set)) {
        
        return mk_stmt(parse_var_set());
    }
    else if (peek_type(Token_type::_globals)){
        consume();
        try_consume(Token_type::_colon,"Expected ':' after 'globals'");
        try_consume(Token_type::_open_sq_brac, "Expected '[' ");

        auto globals = m_Allocator.alloc<node::_statement_globals>();
        this->in_func = true;
        size_t alloc_save = this->alloc_size;
        while(true){
            if(try_consume(Token_type::_back_n)){
                globals->n_lines++; //I really dont know of a better way rn
                line_counter++;
            }
            else if(peek_type(Token_type::_dec) && peek_type(Token_type::_ident, 1) && peek_type(Token_type::_equal, 2)){
            globals->vars.push_back(parse_var_dec());
            }else{break;}
        }
        this->alloc_size = alloc_save;
        try_consume(Token_type::_close_sq_brac, "Expected ']' ");
        this->in_func = false;
        return mk_stmt(globals);

    }
    else if (peek_type(Token_type::_func) && peek_type(Token_type::_main_scope, 1)) {
        consume();
        consume();
        in_func = true;
        auto main_scope = m_Allocator.alloc<node::_main_scope>();

        // adding argc and argv
        if (try_consume(Token_type::_colon)){
            main_scope->has_args = true;
            auto tok = try_consume(Token_type::_ident);
            if (!tok.has_value()){
                line_err("Expected identifier for argument count");
            }
            main_scope->args[0] = tok.value().value.value();
            try_consume(Token_type::_comma, "Expected ',' to seperate main arguments");

            auto tok1 = try_consume(Token_type::_ident);
            if (!tok1.has_value()){
                line_err("Expected identifier for argument pointer");
            }
            main_scope->args[1] = tok1.value().value.value();
        }

        if (auto scope = parse_scope()) {

            main_scope->scope = scope.value();
        }
        else {
            line_err("Invalid main scope");
        }
        main_scope->stack_space = alloc_size;
        if (main_scope->has_args){main_scope->stack_space += 8;}
        alloc_size = 0;
        in_func = false;
        return mk_stmt(main_scope);

    }
    else if (peek_type(Token_type::_func) && peek_type(Token_type::_ident, 1) && !peek_type(Token_type::_main_scope, 1)) {
        consume();
        in_func = true;
        auto func = m_Allocator.alloc<node::_statement_function>();
        func->ident = consume();

        if (try_consume(Token_type::_colon)) { //there are arguments following

            while (auto arg = try_parse_argument()) {
                func->arguments.push_back(arg.value());
                if (try_consume(Token_type::_comma)) {
                    continue;
                }
                else {
                    break;
                }
            }
            if (func->arguments.size() > MAX_FUNC_ARGS) {
                std::stringstream ss;
                ss << "Functions with more than " << MAX_FUNC_ARGS << " arguments are currently not supported";
                line_err(ss.str());
            }
        }

        if (auto scope = parse_scope()) {
            func->scope = scope.value();
        }
        else {
            line_err("Function scope needs to be directly after arguments. Not even a new line. Sorry, my programming language my rules!");
        }

        if (try_consume(Token_type::_eq_right_arrow)) {
            if (peek().has_value() && is_data_type(peek().value().type))
            {
                func->ret_type = consume().type;
                if(try_consume(Token_type::_ptr)){
                    func->ret_type_is_ptr = true;
                }
            }
            else {
                line_err("Invalid or missing return type");
            }
        }
        func->stack_space = alloc_size;
        alloc_size = 0;
        in_func = false;
        return mk_stmt(func);
    }
    
    else if (peek_type(Token_type::_return)) {
        consume();
        auto stmt_ret = m_Allocator.alloc<node::_statement_ret>();
        if (const auto expr = parse_expr()) {
            stmt_ret->expr = expr.value();
            try_consume(Token_type::_semicolon, "Expected ';'");
            return mk_stmt(stmt_ret);
        }
        else {
            line_err("Invalid expression in return statement!");
        }

    }
    else if (peek_type(Token_type::_break) || peek_type(Token_type::_next)){
        auto break_next = m_Allocator.alloc<node::_statement_break_next>();
        if (peek_type(Token_type::_break)){
            break_next->var = m_Allocator.alloc<node::_statement_break>();
        }else{
            break_next->var = m_Allocator.alloc<node::_statement_next>();
        }
        consume();
        try_consume(Token_type::_semicolon, "Expected ';'");
        return mk_stmt(break_next);
    }
    else if (peek_type(Token_type::_open_cur_brac)) {
        if (auto scope = parse_scope()) {
            auto stmt = m_Allocator.alloc<node::_statement>();
            stmt->var = scope.value();
            return stmt;
        }
        else {
            line_err("Invalid scope");
        }
    }
    else if (peek_type(Token_type::_if) || peek_type(Token_type::_while) || peek_type(Token_type::_for)) {
        consume();
        auto stmt_crtl = m_Allocator.alloc<node::_ctrl_statement>();
        if (peek(-1).value().type == Token_type::_if) {
            auto _if = m_Allocator.alloc<node::_statement_if>();
            stmt_crtl->type = Token_type::_if;
            stmt_crtl->var = _if;
        }
        else if (peek(-1).value().type == Token_type::_while) {
            auto _while = m_Allocator.alloc<node::_statement_while>();
            stmt_crtl->type = Token_type::_while;
            stmt_crtl->var = _while;
        }
        else if (peek(-1).value().type == Token_type::_for) {
            auto _for = m_Allocator.alloc<node::_statement_for>();
            stmt_crtl->type = Token_type::_for;
            if (peek().has_value() && peek().value().type == Token_type::_dec) {
                if (auto stmt_dec = parse_var_dec()) {
                    _for->_stmt_var_dec = stmt_dec;
                }
                else {
                    line_err("Unable to parse variable declaration");
                }
            }
            else {
                line_err("No variable declaration in for statement found");
            }
            stmt_crtl->var = _for;
        }
        if(auto logic = parse_logical_stmt()){
            stmt_crtl->logic = logic.value();
        }else{
            line_err("Unable to parse logical statement");
        }
        if (stmt_crtl->type == Token_type::_for) {
            auto _for = std::get<node::_statement_for*>(stmt_crtl->var);
            try_consume(Token_type::_semicolon, "Expected ';' after comparison in for statement");
            if (auto var_expr = parse_expr())
            {
                _for->var_op = var_expr.value();
            }
            else {
                line_err("Unable to parse operation in for statement");
            }
            try_consume(Token_type::_semicolon, "Expected ';' after variable operation in for statement");
        }

        if (auto scope = parse_scope()) {
            stmt_crtl->scope = scope.value();
        }
        else {
            line_err("Expected Scope");
        }
        if (stmt_crtl->type == Token_type::_if && peek_type(Token_type::_else)) {
            consume();
            auto _if = std::get<node::_statement_if*>(stmt_crtl->var);
            auto _else = m_Allocator.alloc<node::_if_else>();
            if (auto scope = parse_scope()) {
                _else->scope = scope.value();
            }
            else {
                line_err("Expected Scope");
            }
            _if->_else = _else;
        }
        return mk_stmt(stmt_crtl);
    }
    else if (peek_type(Token_type::_output)) {
        consume();
        auto stmt_out = m_Allocator.alloc<node::_statement_output>();
        flags.needs_buffer = true;
        while (const auto expr = parse_expr())
        {
            stmt_out->expr_vec.push_back(expr.value());
            if (try_consume(Token_type::_comma)) {
                continue;
            }
            else {
                break;
            }
        }
        if (try_consume(Token_type::_noend)) {
            stmt_out->noend = true;
        }
        try_consume(Token_type::_semicolon, "Expected ';'");
        return mk_stmt(stmt_out);
    }
    else if (peek_type(Token_type::_input)) {
        consume();
        flags.needs_buffer = true;
        auto stmt_input = m_Allocator.alloc<node::_statement_input>();
        stmt_input->ident = try_consume(Token_type::_ident, "Expected string buffer");
        try_consume(Token_type::_semicolon, "Expected ';'");
        return mk_stmt(stmt_input);
    }
    else if (peek_type(Token_type::_struct) && peek_type(Token_type::_ident,1)){
        consume();
        if(in_func){
            line_err("Cannot create Struct inside of function or other struct");
        }
        auto struct_ = m_Allocator.alloc<node::_statement_struct>();
        in_func = true;
        add_struct_info = true;
        generic_name_to_offsets = &struct_->name_to_offsets;
        generic_var_metadata = &struct_->vars_metadata;
        struct_->ident = consume();
        if(this->struct_name_alloc_map.find(struct_->ident.value.value()) != this->struct_name_alloc_map.end()){
            line_err("Struct with that name already exists");
        }
        try_consume(Token_type::_open_cur_brac,"Expected '{'");
        this->struct_names.push_back(struct_->ident.value.value());
        while(true){
            if(try_consume(Token_type::_back_n)){
                struct_->n_lines++; //I really dont know of a better way rn
                line_counter++;
            }
            else if(peek_type(Token_type::_dec) && peek_type(Token_type::_ident, 1) && peek_type(Token_type::_equal, 2)){
            struct_->vars.push_back(parse_var_dec());
            }else{break;}
        }
        this->struct_names.pop_back();
        this->struct_name_alloc_map.insert(std::make_pair(struct_->ident.value.value(),alloc_size)); 
        try_consume(Token_type::_close_cur_brac,"Expected '}'");
        in_func = false;
        struct_size = 0;
        alloc_size = 0;
        add_struct_info = false;
        generic_name_to_offsets = nullptr;
        generic_var_metadata = nullptr;

        return mk_stmt(struct_);
    }
    else if (const auto expr = parse_expr()) {//try to parse an expression freely
        auto pure_expr = m_Allocator.alloc<node::_statement_pure_expr>();
        pure_expr->expr = expr.value();
        try_consume(Token_type::_semicolon, "Expected ';'");
        return mk_stmt(pure_expr);
    }
    return {};
}
std::optional<node::_program> Parser::parse_program() {
    while (peek().has_value()) {
        if (auto stmt = parse_statement()) {
            prog.statements.push_back(stmt.value());
        }
        else {
            line_err("Invalid statement");
        }
    }
    return prog;
}

#pragma once

#include <variant>

#include "./arena.h"
#include "./tokenization.h"

struct NodeTermIntLit {
    Token int_lit;
};

struct NodeTermIdent {
    Token ident;
};

struct NodeExpr;

struct NodeTermParen {
    NodeExpr *expr;
};

struct NodeBinExprAdd {
    NodeExpr *lhs;
    NodeExpr *rhs;
};

struct NodeBinExprMulti {
    NodeExpr *lhs;
    NodeExpr *rhs;
};

struct NodeBinExprSub {
    NodeExpr *lhs;
    NodeExpr *rhs;
};

struct NodeBinExprDiv {
    NodeExpr *lhs;
    NodeExpr *rhs;
};

struct NodeBinExprEq {
    NodeExpr *lhs;
    NodeExpr *rhs;
};

struct NodeBinExprLet {
    NodeExpr *lhs;
    NodeExpr *rhs;
};

struct NodeBinExprBet {
    NodeExpr *lhs;
    NodeExpr *rhs;
};

struct NodeBinExprLetEq {
    NodeExpr *lhs;
    NodeExpr *rhs;
};

struct NodeBinExprBetEq {
    NodeExpr *lhs;
    NodeExpr *rhs;
};

struct NodeBinExprNoEq {
    NodeExpr *lhs;
    NodeExpr *rhs;
};

struct NodeBinExprAnd {
    NodeExpr *lhs;
    NodeExpr *rhs;
};

struct NodeBinExprOr {
    NodeExpr *lhs;
    NodeExpr *rhs;
};

struct NodeSinExprNot {
    NodeExpr *expr;
};

struct NodeSinExprNeg {
    NodeExpr *expr;
};

struct NodeSinExpr {
    std::variant<NodeSinExprNot *, NodeSinExprNeg *> var;
};

struct NodeBinExpr {
    std::variant<NodeBinExprAdd *, NodeBinExprMulti *, NodeBinExprSub *, NodeBinExprDiv *, NodeBinExprEq *,
        NodeBinExprLet *, NodeBinExprBet *, NodeBinExprLetEq *, NodeBinExprBetEq *, NodeBinExprNoEq *, NodeBinExprAnd *,
        NodeBinExprOr *> var;
};

struct NodeTermPlusPlus {
    Token ident{};
};

struct NodeTermMinusMinus {
    Token ident{};
};

struct NodeTerm {
    std::variant<NodeTermIntLit *, NodeTermIdent *, NodeTermParen *, NodeTermPlusPlus *, NodeTermMinusMinus *> var;
};

struct NodeExpr {
    std::variant<NodeTerm *, NodeBinExpr *, NodeSinExpr *> var;
};

struct NodeStmtExit {
    NodeExpr *expr;
};

struct NodeStmtLet {
    Token ident;
    NodeExpr *expr{};
};

struct NodeStmt;

struct NodeStmtMacrosCommand;

struct NodeScope {
    std::vector<NodeStmt *> stmts;
    bool is_outline = false;
};

struct NodeIfPred;

struct NodeIfPredElif {
    NodeExpr *expr{};
    NodeScope *scope{};
    std::optional<NodeIfPred *> pred;
};

struct NodeIfPredElse {
    NodeScope *scope;
};

struct NodeIfPred {
    std::variant<NodeIfPredElif *, NodeIfPredElse *> var;
};

struct NodeStmtIf {
    NodeExpr *expr{};
    NodeScope *scope{};
    std::optional<NodeIfPred *> pred;
};

struct NodeStmtAssign {
    Token ident;
    NodeExpr *expr{};
};

struct NodeStmtMinecraftCommand {
    Token command;
};

struct NodeStmtMacrosCommand {
    std::vector<Token> commands;
    std::vector<NodeExpr *> vars;
};

struct NodeStmtWhile {
    NodeExpr *expr{};
    NodeScope *scope{};
};

struct NodeStmtFor {
    NodeStmt *let{};
    NodeExpr *expr{};
    NodeExpr *end{};
    NodeStmtWhile *while_{};
};

struct NodeStmt {
    std::variant<NodeStmtExit *, NodeStmtLet *, NodeScope *, NodeStmtIf *, NodeStmtAssign *, NodeStmtMinecraftCommand *,
        NodeStmtMacrosCommand *, NodeStmtWhile *, NodeStmtFor *> var;
};

struct NodeProg {
    std::vector<NodeStmt *> stmts;
};

class Parser {
public:
    explicit Parser(std::vector<Token> &tokens, const std::string &file_path)
        : m_tokens(std::move(tokens)),
          m_file_path(file_path),
          m_allocator(1024 * 1024 * 4) {
    }

    void error_expected(const std::string &msg) const {
        error_msg("Expected " + msg);
    }

    void error_msg(const std::string &msg) const {
        std::cerr << "A parse Error : " << msg << " on line " << (peek(-1).has_value()
                                                                      ? peek(-1).value().line
                                                                      : 1) << " in file " << m_file_path <<
                std::endl;
        exit(EXIT_FAILURE);
    }

    std::optional<NodeTerm *> parse_term() {
        if (const auto int_lit = try_consume(TokenType::int_)) {
            auto term_int_lit = m_allocator.alloc<NodeTermIntLit>();
            term_int_lit->int_lit = int_lit.value();
            auto term = m_allocator.alloc<NodeTerm>();
            term->var = term_int_lit;
            return term;
        }
        if (const auto ident = try_consume(TokenType::ident)) {
            if (peek().has_value() && peek().value().type == TokenType::plus_plus) {
                consume();
                auto trem_plus_plus = m_allocator.alloc<NodeTermPlusPlus>();
                trem_plus_plus->ident = ident.value();
                auto term = m_allocator.alloc<NodeTerm>();
                term->var = trem_plus_plus;
                return term;
            }
            if (peek().has_value() && peek().value().type == TokenType::minus_minus) {
                consume();
                auto term_minus_minus = m_allocator.alloc<NodeTermMinusMinus>();
                term_minus_minus->ident = ident.value();
                auto term = m_allocator.alloc<NodeTerm>();
                term->var = term_minus_minus;
                return term;
            }
            auto trem_ident = m_allocator.alloc<NodeTermIdent>();
            trem_ident->ident = ident.value();
            auto term = m_allocator.alloc<NodeTerm>();
            term->var = trem_ident;
            return term;
        }
        if (const auto open_paren = try_consume(TokenType::open_paren)) {
            const auto expr = pares_expr();
            if (!expr.has_value()) {
                error_expected("expression");
            }
            try_consume_err(TokenType::close_paren);
            auto term_paren = m_allocator.alloc<NodeTermParen>();
            term_paren->expr = expr.value();
            auto term = m_allocator.alloc<NodeTerm>();
            term->var = term_paren;
            return term;
        }
        return {};
    }

    std::optional<NodeExpr *> parse_sin_expr() {
        const std::optional<Token> curr_tok = peek();
        const std::optional<int> prec = curr_tok.has_value() ? sin_prec(curr_tok->type) : std::nullopt;

        if (prec.has_value()) {
            const auto [type, line, col, value] = consume();

            const auto inner_expr = parse_sin_expr();
            if (!inner_expr.has_value()) {
                error_expected("expression");
            }

            auto sin_node = m_allocator.alloc<NodeSinExpr>();

            if (type == TokenType::minus) {
                auto neg = m_allocator.alloc<NodeSinExprNeg>();
                neg->expr = inner_expr.value();
                sin_node->var = neg;
            } else if (type == TokenType::not_) {
                auto not_ = m_allocator.alloc<NodeSinExprNot>();
                not_->expr = inner_expr.value();
                sin_node->var = not_;
            }

            auto expr = m_allocator.alloc<NodeExpr>();
            expr->var = sin_node;
            return expr;
        }

        auto term = parse_term();
        if (!term.has_value()) {
            return {};
        }

        auto expr = m_allocator.alloc<NodeExpr>();
        expr->var = term.value();
        return expr;
    }

    std::optional<NodeExpr *> pares_expr(const int min_prec = 0) {
        std::optional<NodeExpr *> expr_lhs = parse_sin_expr();
        if (!expr_lhs.has_value()) {
            return {};
        }

        while (true) {
            std::optional<Token> curr_tok = peek();
            std::optional<int> prec;
            if (curr_tok.has_value()) {
                prec = bin_prec(curr_tok->type);
                if (!prec.has_value() || prec < min_prec) {
                    break;
                }
            } else {
                break;
            }
            const auto [type, line, col, value] = consume();
            const int next_min_prec = prec.value() + 1;
            auto expr_rhs = pares_expr(next_min_prec);
            if (!expr_rhs.has_value()) {
                error_expected("expression");
            }
            auto expr = m_allocator.alloc<NodeBinExpr>();
            const auto expr_lhs2 = m_allocator.alloc<NodeExpr>();
            if (type == TokenType::plus) {
                auto add = m_allocator.alloc<NodeBinExprAdd>();
                expr_lhs2->var = expr_lhs.value()->var;
                add->lhs = expr_lhs2;
                add->rhs = expr_rhs.value();
                expr->var = add;
            } else if (type == TokenType::star) {
                auto multi = m_allocator.alloc<NodeBinExprMulti>();
                expr_lhs2->var = expr_lhs.value()->var;
                multi->lhs = expr_lhs2;
                multi->rhs = expr_rhs.value();
                expr->var = multi;
            } else if (type == TokenType::minus) {
                auto sub = m_allocator.alloc<NodeBinExprSub>();
                expr_lhs2->var = expr_lhs.value()->var;
                sub->lhs = expr_lhs2;
                sub->rhs = expr_rhs.value();
                expr->var = sub;
            } else if (type == TokenType::fslash) {
                auto div = m_allocator.alloc<NodeBinExprDiv>();
                expr_lhs2->var = expr_lhs.value()->var;
                div->lhs = expr_lhs2;
                div->rhs = expr_rhs.value();
                expr->var = div;
            } else if (type == TokenType::test_eq) {
                auto eq = m_allocator.alloc<NodeBinExprEq>();
                expr_lhs2->var = expr_lhs.value()->var;
                eq->lhs = expr_lhs2;
                eq->rhs = expr_rhs.value();
                expr->var = eq;
            } else if (type == TokenType::letter) {
                auto let = m_allocator.alloc<NodeBinExprLet>();
                expr_lhs2->var = expr_lhs.value()->var;
                let->lhs = expr_lhs2;
                let->rhs = expr_rhs.value();
                expr->var = let;
            } else if (type == TokenType::better) {
                auto bet = m_allocator.alloc<NodeBinExprBet>();
                expr_lhs2->var = expr_lhs.value()->var;
                bet->lhs = expr_lhs2;
                bet->rhs = expr_rhs.value();
                expr->var = bet;
            } else if (type == TokenType::let_eq) {
                auto let_eq = m_allocator.alloc<NodeBinExprLetEq>();
                expr_lhs2->var = expr_lhs.value()->var;
                let_eq->lhs = expr_lhs2;
                let_eq->rhs = expr_rhs.value();
                expr->var = let_eq;
            } else if (type == TokenType::bet_eq) {
                auto bet_eq = m_allocator.alloc<NodeBinExprBetEq>();
                expr_lhs2->var = expr_lhs.value()->var;
                bet_eq->lhs = expr_lhs2;
                bet_eq->rhs = expr_rhs.value();
                expr->var = bet_eq;
            } else if (type == TokenType::no_eq) {
                auto no_eq = m_allocator.alloc<NodeBinExprNoEq>();
                expr_lhs2->var = expr_lhs.value()->var;
                no_eq->lhs = expr_lhs2;
                no_eq->rhs = expr_rhs.value();
                expr->var = no_eq;
            } else if (type == TokenType::and_) {
                auto and_ = m_allocator.alloc<NodeBinExprAnd>();
                expr_lhs2->var = expr_lhs.value()->var;
                and_->lhs = expr_lhs2;
                and_->rhs = expr_rhs.value();
                expr->var = and_;
            } else if (type == TokenType::or_) {
                auto or_ = m_allocator.alloc<NodeBinExprOr>();
                expr_lhs2->var = expr_lhs.value()->var;
                or_->lhs = expr_lhs2;
                or_->rhs = expr_rhs.value();
                expr->var = or_;
            }
            expr_lhs.value()->var = expr;
        }
        return expr_lhs.value();
    }

    std::optional<NodeScope *> parse_scope() {
        auto scope = m_allocator.alloc<NodeScope>();
        if (try_consume(TokenType::outline).has_value()) {
            scope->is_outline = true;
        }
        if (!try_consume(TokenType::open_curly).has_value()) {
            return {};
        }

        while (auto stmt = parse_stmt()) {
            scope->stmts.push_back(stmt.value());
        }
        try_consume_err(TokenType::close_curly);
        return scope;
    }

    std::optional<NodeIfPred *> parse_if_pred() {
        if (try_consume(TokenType::elif).has_value()) {
            try_consume_err(TokenType::open_paren);
            const auto elif = m_allocator.alloc<NodeIfPredElif>();
            if (const auto expr = pares_expr()) {
                elif->expr = expr.value();
            } else {
                error_expected("expression");
            }
            try_consume_err(TokenType::close_paren);
            if (const auto scope = parse_scope()) {
                elif->scope = scope.value();
            } else {
                error_expected("scope");
            }
            elif->pred = parse_if_pred();
            auto pred = m_allocator.emplace<NodeIfPred>(elif);
            return pred;
        }
        if (try_consume(TokenType::else_)) {
            const auto else_ = m_allocator.alloc<NodeIfPredElse>();
            if (const auto scope = parse_scope()) {
                else_->scope = scope.value();
            } else {
                error_expected("scope");
            }
            auto pred = m_allocator.emplace<NodeIfPred>(else_);
            return pred;
        }
        return {};
    }

    std::optional<NodeStmt *> parse_stmt() {
        if (peek().has_value() && peek().value().type == TokenType::exit && peek(1).has_value() && peek(1).value().type
            ==
            TokenType::open_paren) {
            consume();
            consume();
            auto stmt_exit = m_allocator.alloc<NodeStmtExit>();
            if (const auto node_expr = pares_expr()) {
                stmt_exit->expr = node_expr.value();
            } else {
                print_error("Invalid expression");
            }
            try_consume_err(TokenType::close_paren);
            try_consume_err(TokenType::semi);
            auto stmt = m_allocator.alloc<NodeStmt>();
            stmt->var = stmt_exit;
            return stmt;
        }
        if (peek().has_value() && peek().value().type == TokenType::let &&
            peek(1).has_value() && peek(1).value().type == TokenType::ident &&
            peek(2).has_value() && peek(2).value().type == TokenType::eq) {
            consume();
            auto stmt_let = m_allocator.alloc<NodeStmtLet>();
            stmt_let->ident = consume();
            consume();
            if (const auto expr = pares_expr()) {
                stmt_let->expr = expr.value();
            } else {
                print_error("Invalid expression");
            }
            try_consume_err(TokenType::semi);
            auto stmt = m_allocator.alloc<NodeStmt>();
            stmt->var = stmt_let;
            return stmt;
        }
        if (peek().has_value() && peek().value().type == TokenType::ident && peek(1).has_value() && peek(1).value().type
            == TokenType::eq) {
            const auto assign = m_allocator.alloc<NodeStmtAssign>();
            assign->ident = consume();
            consume();
            if (const auto expr = pares_expr()) {
                assign->expr = expr.value();
            } else {
                error_expected("expression");
            }
            try_consume_err(TokenType::semi);
            auto stmt = m_allocator.emplace<NodeStmt>(assign);
            return stmt;
        }
        if (peek().has_value() && peek().value().type == TokenType::open_curly) {
            if (auto scope = parse_scope()) {
                auto stmt = m_allocator.alloc<NodeStmt>();
                stmt->var = scope.value();
                return stmt;
            }
            print_error("Invalid scope");
        }
        if (peek().has_value() && peek().value().type == TokenType::outline && peek_after(TokenType::open_curly)) {
            if (auto scope = parse_scope()) {
                auto stmt = m_allocator.alloc<NodeStmt>();
                stmt->var = scope.value();
                return stmt;
            }
            print_error("Invalid scope");
        }
        if (auto if_ = try_consume(TokenType::if_)) {
            try_consume_err(TokenType::open_paren);
            auto stmt_if = m_allocator.alloc<NodeStmtIf>();
            if (const auto expr = pares_expr()) {
                stmt_if->expr = expr.value();
            } else {
                print_error("Invalid expression");
            }
            try_consume_err(TokenType::close_paren);
            if (const auto scope = parse_scope()) {
                stmt_if->scope = scope.value();
            } else {
                print_error("Invalid scope");
            }
            stmt_if->pred = parse_if_pred();
            auto stmt = m_allocator.alloc<NodeStmt>();
            stmt->var = stmt_if;
            return stmt;
        }
        if (const auto command = try_consume(TokenType::commands)) {
            auto stmt_command = m_allocator.alloc<NodeStmtMinecraftCommand>();
            stmt_command->command = command.value();
            try_consume_err(TokenType::semi);
            auto stmt = m_allocator.alloc<NodeStmt>();
            stmt->var = stmt_command;
            return stmt;
        }
        if (const auto command = try_consume(TokenType::macros_start)) {
            auto stmt_macros_commands = m_allocator.alloc<NodeStmtMacrosCommand>();
            while (peek().has_value() && peek().value().type != TokenType::semi) {
                if (peek().value().type == TokenType::macros_var) {
                    stmt_macros_commands->commands.push_back(consume());
                    if (const auto expr = pares_expr()) {
                        stmt_macros_commands->vars.push_back(expr.value());
                        try_consume(TokenType::close_paren);
                    } else {
                        error_msg("Invalid expression");
                    }
                } else {
                    stmt_macros_commands->commands.push_back(consume());
                }
            }
            try_consume_err(TokenType::semi);
            auto stmt = m_allocator.alloc<NodeStmt>();
            stmt->var = stmt_macros_commands;
            return stmt;
        }
        if (const auto while_ = try_consume(TokenType::while_)) {
            try_consume_err(TokenType::open_paren);
            auto stmt_while = m_allocator.alloc<NodeStmtWhile>();
            if (const auto expr = pares_expr()) {
                stmt_while->expr = expr.value();
            } else {
                error_msg("Invalid expression");
            }
            try_consume_err(TokenType::close_paren);
            if (const auto scope = parse_scope()) {
                scope.value()->is_outline = true;
                stmt_while->scope = scope.value();
            } else {
                error_msg("Invalid scope");
            }
            auto stmt = m_allocator.alloc<NodeStmt>();
            stmt->var = stmt_while;
            return stmt;
        }
        if (const auto for_ = try_consume(TokenType::for_)) {
            try_consume_err(TokenType::open_paren);
            auto stmt_for = m_allocator.alloc<NodeStmtFor>();
            if (const auto stmt = parse_stmt()) {
                 stmt_for->let = stmt.value();
            } else {
                error_expected("statement");
            }
            if (const auto expr = pares_expr()) {
                stmt_for->expr = expr.value();
            } else {
                error_msg("Invalid expression");
            }
            try_consume_err(TokenType::semi);
            if (const auto expr = pares_expr()) {
                stmt_for->end = expr.value();
            } else {
                error_msg("Invalid expression");
            }
            try_consume_err(TokenType::close_paren);
            auto stmt_while = m_allocator.alloc<NodeStmtWhile>();
            if (const auto scope = parse_scope()) {
                scope.value()->is_outline = true;
                stmt_while->scope = scope.value();
                stmt_for->while_ = stmt_while;
            } else {
                print_error("Invalid scope");
            }
            auto stmt = m_allocator.alloc<NodeStmt>();
            stmt->var = stmt_for;
            return stmt;
        }
        return {};
    }

    std::optional<NodeProg> parse_prog() {
        NodeProg prog;
        while (peek().has_value()) {
            if (const auto stmt = parse_stmt()) {
                prog.stmts.push_back(stmt.value());
            } else {
                error_msg("Invalid statement");
            }
        }
        return prog;
    }

private:
    [[nodiscard]] std::optional<Token> peek(const int offset = 0) const {
        if (m_index + offset >= m_tokens.size()) {
            return {};
        }
        return m_tokens.at(m_index + offset);
    }

    [[nodiscard]] bool peek_after(const TokenType type) const {
        int count = 1;
        while (peek(count).has_value() && peek(count).value().type != type) {
            count++;
        }
        return peek(count).has_value();
    }

    Token consume() {
        return m_tokens.at(m_index++);
    }

    std::optional<Token> try_consume(const TokenType type) {
        if (peek().has_value() && peek().value().type == type) {
            return consume();
        }
        return {};
    }

    std::optional<Token> try_consume_err(const TokenType type) {
        if (peek().has_value() && peek().value().type == type) {
            return consume();
        }
        error_expected(to_string(type));
        return {};
    }

    const std::vector<Token> &m_tokens;
    const std::string &m_file_path;
    size_t m_index{};
    ArenaAllocator m_allocator;
};

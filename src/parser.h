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

struct NodeBinExpr {
    std::variant<NodeBinExprAdd *, NodeBinExprMulti *, NodeBinExprSub *, NodeBinExprDiv *, NodeBinExprEq *,
        NodeBinExprLet *, NodeBinExprBet *, NodeBinExprLetEq *, NodeBinExprBetEq *, NodeBinExprNoEq *, NodeBinExprAnd *,
        NodeBinExprOr *> var;
};

struct NodeSinExpr {

};

struct NodeTerm {
    std::variant<NodeTermIntLit *, NodeTermIdent *, NodeTermParen *> var;
};

struct NodeExpr {
    std::variant<NodeTerm *, NodeBinExpr *> var;
};

struct NodeStmtExit {
    NodeExpr *expr;
};

struct NodeStmtLet {
    Token ident;
    NodeExpr *expr{};
};

struct NodeStmt;

struct NodeScope {
    std::vector<NodeStmt *> stmts;
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

struct NodeStmt {
    std::variant<NodeStmtExit *, NodeStmtLet *, NodeScope *, NodeStmtIf *, NodeStmtAssign *> var;
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
        std::cerr << "A parse Error : Expected " << msg << " on line " << (peek(-1).has_value()
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

    std::optional<NodeExpr *> pares_expr(const int min_prec = 0) {
        std::optional<NodeTerm *> term_lhs = parse_term();
        if (!term_lhs.has_value()) {
            return {};
        }
        auto expr_lhs = m_allocator.alloc<NodeExpr>();
        expr_lhs->var = term_lhs.value();

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
                expr_lhs2->var = expr_lhs->var;
                add->lhs = expr_lhs2;
                add->rhs = expr_rhs.value();
                expr->var = add;
            } else if (type == TokenType::star) {
                auto multi = m_allocator.alloc<NodeBinExprMulti>();
                expr_lhs2->var = expr_lhs->var;
                multi->lhs = expr_lhs2;
                multi->rhs = expr_rhs.value();
                expr->var = multi;
            } else if (type == TokenType::minus) {
                auto sub = m_allocator.alloc<NodeBinExprSub>();
                expr_lhs2->var = expr_lhs->var;
                sub->lhs = expr_lhs2;
                sub->rhs = expr_rhs.value();
                expr->var = sub;
            } else if (type == TokenType::fslash) {
                auto div = m_allocator.alloc<NodeBinExprDiv>();
                expr_lhs2->var = expr_lhs->var;
                div->lhs = expr_lhs2;
                div->rhs = expr_rhs.value();
                expr->var = div;
            } else if (type == TokenType::test_eq) {
                auto eq = m_allocator.alloc<NodeBinExprEq>();
                expr_lhs2->var = expr_lhs->var;
                eq->lhs = expr_lhs2;
                eq->rhs = expr_rhs.value();
                expr->var = eq;
            } else if (type == TokenType::letter) {
                auto let = m_allocator.alloc<NodeBinExprLet>();
                expr_lhs2->var = expr_lhs->var;
                let->lhs = expr_lhs2;
                let->rhs = expr_rhs.value();
                expr->var = let;
            } else if (type == TokenType::better) {
                auto bet = m_allocator.alloc<NodeBinExprBet>();
                expr_lhs2->var = expr_lhs->var;
                bet->lhs = expr_lhs2;
                bet->rhs = expr_rhs.value();
                expr->var = bet;
            } else if (type == TokenType::let_eq) {
                auto let_eq = m_allocator.alloc<NodeBinExprLetEq>();
                expr_lhs2->var = expr_lhs->var;
                let_eq->lhs = expr_lhs2;
                let_eq->rhs = expr_rhs.value();
                expr->var = let_eq;
            } else if (type == TokenType::bet_eq) {
                auto bet_eq = m_allocator.alloc<NodeBinExprBetEq>();
                expr_lhs2->var = expr_lhs->var;
                bet_eq->lhs = expr_lhs2;
                bet_eq->rhs = expr_rhs.value();
                expr->var = bet_eq;
            } else if (type == TokenType::no_eq) {
                auto no_eq = m_allocator.alloc<NodeBinExprNoEq>();
                expr_lhs2->var = expr_lhs->var;
                no_eq->lhs = expr_lhs2;
                no_eq->rhs = expr_rhs.value();
                expr->var = no_eq;
            } else if (type == TokenType::and_) {
                auto and_ = m_allocator.alloc<NodeBinExprAnd>();
                expr_lhs2->var = expr_lhs->var;
                and_->lhs = expr_lhs2;
                and_->rhs = expr_rhs.value();
                expr->var = and_;
            } else if (type == TokenType::or_) {
                auto or_ = m_allocator.alloc<NodeBinExprOr>();
                expr_lhs2->var = expr_lhs->var;
                or_->lhs = expr_lhs2;
                or_->rhs = expr_rhs.value();
                expr->var = or_;
            }
            expr_lhs->var = expr;
        }
        return expr_lhs;
    }

    std::optional<NodeScope *> parse_scope() {
        if (!try_consume(TokenType::open_curly).has_value()) {
            return {};
        }
        auto scope = m_allocator.alloc<NodeScope>();
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
        return {};
    }

    std::optional<NodeProg> parse_prog() {
        NodeProg prog;
        while (peek().has_value()) {
            if (const auto stmt = parse_stmt()) {
                prog.stmts.push_back(stmt.value());
            } else {
                print_error("Invalid statement");
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

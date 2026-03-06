#pragma once

#include <algorithm>
#include <cassert>

#include "parser.h"

inline auto VERSION = "v0.1.0";

class Generator {
public:
    explicit Generator(NodeProg prog, std::string file_name, std::string file_path)
        : m_prog(std::move(prog)), m_file_name(std::move(file_name)), m_file_path(std::move(file_path)) {
        std::fstream os(m_file_path + "main.mcfunction", std::ios::out);
        m_output = std::move(os);
    }

    void gen_term(const NodeTerm *term, const std::optional<std::string> &conditions = {}) {
        struct TermVisitor {
            Generator &gen;
            const std::optional<std::string> &conditions = {};

            void operator()(const NodeTermIntLit *term_int_lit) const {
                gen.push_int(term_int_lit->int_lit.value.value(), conditions);
            }

            void operator()(const NodeTermIdent *term_ident) const {
                const auto it = std::find_if(
                    gen.m_vars.cbegin(),
                    gen.m_vars.cend(),
                    [&](const Var &var) {
                        return var.name == term_ident->ident.value.value();
                    });
                if (it == gen.m_vars.cend()) {
                    print_error("Undeclared identifier: " + term_ident->ident.value.value());
                }
                std::stringstream offset;
                offset << "__stack[-" << (gen.m_stack_size - it->stack_loc) << "]";
                gen.push(offset.str(), conditions);
            }

            void operator()(const NodeTermParen *term_paren) const {
                gen.gen_expr(term_paren->expr, conditions);
            }

            void operator()(const NodeTermPlusPlus *term_plus_plus) const {
                const auto ident = term_plus_plus->ident;
                const auto it = std::find_if(
                    gen.m_vars.cbegin(),
                    gen.m_vars.cend(),
                    [&](const Var &var) {
                        return var.name == ident.value.value();
                    });
                if (it == gen.m_vars.cend()) {
                    print_error("Undeclared identifier: " + ident.value.value());
                }
                std::stringstream offset;
                offset << "__stack[-" << (gen.m_stack_size - it->stack_loc) << "]";
                gen.output(
                    "execute store result score pp __" + gen.m_file_name + " run data get storage minecraft:__" + gen.
                    m_file_name + " " +
                    offset.str() +
                    "\n", conditions);
                gen.output("scoreboard players add pp __" + gen.m_file_name + " 1\n", conditions);
                gen.to_nbt("pp", conditions);
                gen.output(
                    "data modify storage minecraft:__" + gen.m_file_name + " " + offset.str() +
                    " set from storage minecraft:__" + gen.m_file_name + " pp\n",
                    conditions);
                gen.push("pp");
            }

            void operator()(const NodeTermMinusMinus *term_minus_minus) const {
                const auto ident = term_minus_minus->ident;
                const auto it = std::find_if(
                    gen.m_vars.cbegin(),
                    gen.m_vars.cend(),
                    [&](const Var &var) {
                        return var.name == ident.value.value();
                    });
                if (it == gen.m_vars.cend()) {
                    print_error("Undeclared identifier: " + ident.value.value());
                }
                std::stringstream offset;
                offset << "__stack[-" << (gen.m_stack_size - it->stack_loc) << "]";
                gen.output(
                    "execute store result score pp __" + gen.m_file_name + " run data get storage minecraft:__" + gen.
                    m_file_name + " " +
                    offset.str() +
                    "\n", conditions);
                gen.output("scoreboard players add pp __" + gen.m_file_name + " -1\n", conditions);
                gen.to_nbt("pp", conditions);
                gen.output(
                    "data modify storage minecraft:__" + gen.m_file_name + " " + offset.str() +
                    " set from storage minecraft:__" + gen.m_file_name + " pp\n",
                    conditions);
                gen.push("pp");
            }
        };
        TermVisitor visitor({.gen = *this, .conditions = conditions});
        std::visit(visitor, term->var);
    }

    void gen_bin_expr(const NodeBinExpr *bin_expr, const std::optional<std::string> &conditions = {}) {
        struct BinExprVisitor {
            Generator &gen;
            const std::optional<std::string> &conditions = {};

            void operator()(const NodeBinExprSub *sub) const {
                gen.gen_expr(sub->rhs, conditions);
                gen.gen_expr(sub->lhs, conditions);
                gen.pop("rax", conditions);
                gen.pop("rbx", conditions);
                gen.output("scoreboard players operation rax __" + gen.m_file_name + " -= rbx __" + gen.
                           m_file_name + "\n", conditions);
                gen.to_nbt("rax", conditions);
                gen.push("rax", conditions);
            }

            void operator()(const NodeBinExprAdd *add) const {
                gen.gen_expr(add->rhs, conditions);
                gen.gen_expr(add->lhs, conditions);
                gen.pop("rax", conditions);
                gen.pop("rbx", conditions);
                gen.output("scoreboard players operation rax __" + gen.m_file_name + " += rbx __" + gen.
                           m_file_name + "\n", conditions);
                gen.to_nbt("rax", conditions);
                gen.push("rax", conditions);
            }

            void operator()(const NodeBinExprMulti *multi) const {
                gen.gen_expr(multi->rhs, conditions);
                gen.gen_expr(multi->lhs, conditions);
                gen.pop("rax", conditions);
                gen.pop("rbx", conditions);
                gen.output("scoreboard players operation rax __" + gen.m_file_name + " *= rbx __" + gen.
                           m_file_name + "\n", conditions);
                gen.to_nbt("rax", conditions);
                gen.push("rax", conditions);
            }

            void operator()(const NodeBinExprDiv *div) const {
                gen.gen_expr(div->rhs, conditions);
                gen.gen_expr(div->lhs, conditions);
                gen.pop("rax", conditions);
                gen.pop("rbx", conditions);
                gen.output("scoreboard players operation rax __" + gen.m_file_name + " /= rbx __" + gen.
                           m_file_name + "\n", conditions);
                gen.to_nbt("rax", conditions);
                gen.push("rax", conditions);
            }

            void operator()(const NodeBinExprEq *eq) const {
                gen.gen_expr(eq->rhs, conditions);
                gen.gen_expr(eq->lhs, conditions);
                gen.pop("rax", conditions);
                gen.pop("rbx", conditions);
                gen.output("scoreboard players set r __" + gen.m_file_name + " 0\n", conditions);
                gen.output(
                    "execute if score rax __" + gen.m_file_name + " = rbx __" + gen.m_file_name +
                    " run scoreboard players set r __" + gen.m_file_name + " 1\n", conditions);
                gen.score_move("r", "rax", conditions);
                gen.to_nbt("rax", conditions);
                gen.push("rax", conditions);
            }

            void operator()(const NodeBinExprLet *let) const {
                gen.gen_expr(let->rhs, conditions);
                gen.gen_expr(let->lhs, conditions);
                gen.pop("rax", conditions);
                gen.pop("rbx", conditions);
                gen.output("scoreboard players set r __" + gen.m_file_name + " 0\n", conditions);
                gen.output(
                    "execute if score rax __" + gen.m_file_name + " < rbx __" + gen.m_file_name +
                    " run scoreboard players set r __" + gen.m_file_name + " 1\n", conditions);
                gen.score_move("r", "rax", conditions);
                gen.to_nbt("rax", conditions);
                gen.push("rax", conditions);
            }

            void operator()(const NodeBinExprBet *bet) const {
                gen.gen_expr(bet->rhs, conditions);
                gen.gen_expr(bet->lhs, conditions);
                gen.pop("rax", conditions);
                gen.pop("rbx", conditions);
                gen.output("scoreboard players set r __" + gen.m_file_name + " 0\n", conditions);
                gen.output(
                    "execute if score rax __" + gen.m_file_name + " > rbx __" + gen.m_file_name +
                    " run scoreboard players set r __" + gen.m_file_name + " 1\n", conditions);
                gen.score_move("r", "rax", conditions);
                gen.to_nbt("rax", conditions);
                gen.push("rax", conditions);
            }

            void operator()(const NodeBinExprLetEq *let_eq) const {
                gen.gen_expr(let_eq->rhs, conditions);
                gen.gen_expr(let_eq->lhs, conditions);
                gen.pop("rax", conditions);
                gen.pop("rbx", conditions);
                gen.output("scoreboard players set r __" + gen.m_file_name + " 0\n", conditions);
                gen.output(
                    "execute if score rax __" + gen.m_file_name + " <= rbx __" + gen.m_file_name +
                    " run scoreboard players set r __" + gen.m_file_name + " 1\n", conditions);
                gen.score_move("r", "rax", conditions);
                gen.to_nbt("rax", conditions);
                gen.push("rax", conditions);
            }

            void operator()(const NodeBinExprBetEq *bet_eq) const {
                gen.gen_expr(bet_eq->rhs, conditions);
                gen.gen_expr(bet_eq->lhs, conditions);
                gen.pop("rax", conditions);
                gen.pop("rbx", conditions);
                gen.output("scoreboard players set r __" + gen.m_file_name + " 0\n", conditions);
                gen.output(
                    "execute if score rax __" + gen.m_file_name + " >= rbx __" + gen.m_file_name +
                    " run scoreboard players set r __" + gen.m_file_name + " 1\n", conditions);
                gen.score_move("r", "rax", conditions);
                gen.to_nbt("rax", conditions);
                gen.push("rax", conditions);
            }

            void operator()(const NodeBinExprNoEq *no_eq) const {
                gen.gen_expr(no_eq->rhs, conditions);
                gen.gen_expr(no_eq->lhs, conditions);
                gen.pop("rax", conditions);
                gen.pop("rbx", conditions);
                gen.output("scoreboard players set r __" + gen.m_file_name + " 1\n", conditions);
                gen.output(
                    "execute if score rax __" + gen.m_file_name + " = rbx __" + gen.m_file_name +
                    " run scoreboard players set r __" + gen.m_file_name + " 0\n", conditions);
                gen.score_move("r", "rax", conditions);
                gen.to_nbt("rax", conditions);
                gen.push("rax", conditions);
            }

            void operator()(const NodeBinExprAnd *and_) const {
                gen.gen_expr(and_->rhs, conditions);
                gen.gen_expr(and_->lhs, conditions);
                gen.pop("rax", conditions);
                gen.pop("rbx", conditions);
                gen.output("scoreboard players set r __" + gen.m_file_name + " 0\n", conditions);
                gen.output(
                    "execute unless score rax __" + gen.m_file_name + " matches 0 unless score rbx __" + gen.m_file_name
                    + " matches 0 run scoreboard players set r __" + gen.m_file_name + " 1\n",
                    conditions);
                gen.score_move("r", "rax", conditions);
                gen.to_nbt("rax", conditions);
                gen.push("rax", conditions);
            }

            void operator()(const NodeBinExprOr *or_) const {
                gen.gen_expr(or_->rhs, conditions);
                gen.gen_expr(or_->lhs, conditions);
                gen.pop("rax", conditions);
                gen.pop("rbx", conditions);
                gen.output("scoreboard players set r __" + gen.m_file_name + " 0\n", conditions);
                gen.output(
                    "execute unless score rax __" + gen.m_file_name + " matches 0 run scoreboard players set r __" + gen
                    .m_file_name + " 1\n",
                    conditions);
                gen.output(
                    "execute unless score rbx __" + gen.m_file_name + " matches 0 run scoreboard players set r __" + gen
                    .m_file_name + " 1\n",
                    conditions);
                gen.score_move("r", "rax", conditions);
                gen.to_nbt("rax", conditions);
                gen.push("rax", conditions);
            }
        };

        BinExprVisitor visitor{.gen = *this, .conditions = conditions};
        std::visit(visitor, bin_expr->var);
    }

    void gen_sin_expr(const NodeSinExpr *sin_expr, const std::optional<std::string> &conditions = {}) {
        struct SinExprVisitor {
            Generator &gen;
            const std::optional<std::string> &conditions = {};

            void operator()(const NodeSinExprNot *not_) const {
                gen.gen_expr(not_->expr, conditions);
                gen.pop("rax", conditions);
                gen.output(
                    "execute if score rax __" + gen.m_file_name + " matches 1 run scoreboard players set r __" + gen.
                    m_file_name + " 0\n", conditions);
                gen.output(
                    "execute if score rax __" + gen.m_file_name + " matches 0 run scoreboard players set r __" + gen.
                    m_file_name + " 1\n", conditions);
                gen.score_move("r", "rax", conditions);
                gen.to_nbt("rax", conditions);
                gen.push("rax", conditions);
            }

            void operator()(const NodeSinExprNeg *not_) const {
                gen.gen_expr(not_->expr, conditions);
                gen.pop("rax", conditions);
                gen.output("scoreboard players set r __" + gen.m_file_name + " 0\n", conditions);
                gen.output(
                    "scoreboard players operation r __" + gen.m_file_name + " -= rax __" + gen.m_file_name + "\n",
                    conditions);
                gen.score_move("r", "rax", conditions);
                gen.to_nbt("rax", conditions);
                gen.push("rax", conditions);
            }
        };

        SinExprVisitor visitor{.gen = *this, .conditions = conditions};
        std::visit(visitor, sin_expr->var);
    }

    void gen_expr(const NodeExpr *expr, const std::optional<std::string> &conditions = {}) {
        struct ExprVisitor {
            Generator &gen;
            const std::optional<std::string> &conditions = {};

            void operator()(const NodeTerm *term) const {
                gen.gen_term(term, conditions);
            }

            void operator()(const NodeBinExpr *bin_expr) const {
                gen.gen_bin_expr(bin_expr, conditions);
            }

            void operator()(const NodeSinExpr *sin_expr) const {
                gen.gen_sin_expr(sin_expr, conditions);
            }
        };

        ExprVisitor visitor{.gen = *this, .conditions = conditions};
        std::visit(visitor, expr->var);
    }

    void gen_scope(const NodeScope *scope, const std::optional<std::string> &conditions = {}) {
        if (scope->is_outline) {
            std::fstream old_fstream = std::move(m_output);
            m_output = create_new_function_file();
            std::string function_name = get_function_file_name();
            begin_scope();
            for (const NodeStmt *stmt: scope->stmts) {
                gen_stmt(stmt, {}, {}, conditions);
            }
            end_scope();
            m_output.close();
            m_output = std::move(old_fstream);
            output("function " + m_file_name + ":" + function_name + "\n", conditions);
        } else {
            begin_scope();
            for (const NodeStmt *stmt: scope->stmts) {
                gen_stmt(stmt, {}, {}, conditions);
            }
            end_scope();
        }
    }

    void gen_while(const NodeExpr *expr, const NodeScope *scope, const std::optional<NodeExpr *> &expr_conditions = {}, const std::optional<std::string> &conditions = {}) {
        std::fstream old_fstream = std::move(m_output);
        m_output = create_new_function_file();
        const std::string function_name = get_function_file_name();
        gen_expr(expr, conditions);
        pop("rax");
        output(
            "execute if score rax __" + m_file_name + " matches 0 run return 0\n", conditions);
        begin_scope();
        for (const NodeStmt *stmt: scope->stmts) {
            gen_stmt(stmt, {}, {}, conditions);
        }
        if (expr_conditions.has_value()) {
            gen_expr(expr_conditions.value(), conditions);
            only_pop(conditions);
        }
        end_scope();
        output("function " + m_file_name + ":" + function_name + "\n", conditions);
        m_output.close();
        m_output = std::move(old_fstream);
        output("function " + m_file_name + ":" + function_name + "\n", conditions);
    }

    void gen_if_pred(const NodeIfPred *pred, const std::vector<std::string> &labels,
                     const std::optional<std::string> &conditions = {}) {
        struct PredVisitor {
            Generator &gen;
            std::vector<std::string> labels;
            const std::optional<std::string> &conditions;

            void operator()(const NodeIfPredElif *elif) {
                gen.gen_expr(elif->expr, conditions);
                gen.pop("rax", conditions);
                const std::string label = gen.create_label();
                gen.score_move("rax", label, conditions);
                std::stringstream test_label;
                if (conditions.has_value()) {
                    test_label << conditions.value();
                }
                for (const std::string &l: labels) {
                    test_label << "if score " << l << " __" << gen.m_file_name << " matches 0 ";
                }
                test_label << "unless score " << label << " __" << gen.m_file_name << " matches 0 ";
                gen.gen_scope(elif->scope, test_label.str());
                labels.push_back(label);
                if (elif->pred.has_value()) {
                    gen.gen_if_pred(elif->pred.value(), labels, conditions);
                }
                gen.released_label();
            }

            void operator()(const NodeIfPredElse *else_) const {
                std::stringstream test_label;
                if (conditions.has_value()) {
                    test_label << conditions.value();
                }
                for (const std::string &l: labels) {
                    test_label << "if score " << l << " __" << gen.m_file_name << " matches 0 ";
                }
                gen.gen_scope(else_->scope, test_label.str());
            }
        };

        PredVisitor visitor({.gen = *this, .labels = labels, .conditions = conditions});
        std::visit(visitor, pred->var);
    }

    void gen_stmt(const NodeStmt *stmt, std::optional<std::fstream> ff = {},
                  std::optional<int> lc = {},
                  const std::optional<std::string> &conditions = {}) {
        struct StmtVisitor {
            Generator &gen;
            std::optional<std::fstream> &ff;
            std::optional<int> &lc;
            const std::optional<std::string> conditions;

            void operator()(const NodeStmtExit *stmt_exit) const {
                gen.gen_expr(stmt_exit->expr, conditions);
                gen.pop("rax", conditions);
                gen.output("return run scoreboard players get rax __" + gen.m_file_name + "\n", conditions);
                gen.m_output << "# program is stop.\n";
            }

            void operator()(const NodeStmtLet *stmt_let) const {
                const auto it = std::find_if(
                    gen.m_vars.cbegin(),
                    gen.m_vars.cend(),
                    [&](const Var &var) {
                        return var.name == stmt_let->ident.value.value();
                    });
                if (it != gen.m_vars.cend()) {
                    print_error("Identifier already used:" + stmt_let->ident.value.value());
                }
                gen.m_vars.push_back({.name = stmt_let->ident.value.value(), .stack_loc = gen.m_stack_size});
                gen.gen_expr(stmt_let->expr, conditions);
            }

            void operator()(const NodeStmtAssign *stmt_assign) const {
                const auto it = std::find_if(
                    gen.m_vars.cbegin(),
                    gen.m_vars.cend(),
                    [&](const Var &var) {
                        return var.name == stmt_assign->ident.value.value();
                    });
                if (it == gen.m_vars.cend()) {
                    print_error("Undeclared identifier: " + stmt_assign->ident.value.value());
                }
                gen.gen_expr(stmt_assign->expr, conditions);
                std::stringstream offset;
                offset << "__stack[-" << (gen.m_stack_size - it->stack_loc) << "]";
                gen.output(
                    "data modify storage minecraft:__" + gen.m_file_name + " " + offset.str() +
                    " set from storage minecraft:__"
                    + gen.m_file_name + " __stack[-1]\n", conditions);
                gen.only_pop(conditions);
            }

            void operator()(const NodeScope *scope) const {
                gen.gen_scope(scope, conditions);
            }

            void operator()(const NodeStmtIf *stmt_if) const {
                gen.gen_expr(stmt_if->expr, conditions);
                gen.pop("rax", conditions);
                const std::string label = gen.create_label();
                gen.score_move("rax", label, conditions);
                std::string test_label;
                if (conditions.has_value()) {
                    test_label = conditions.value() + "unless score " + label + " __" + gen.m_file_name + " matches 0 ";
                } else {
                    test_label = "unless score " + label + " __" + gen.m_file_name + " matches 0 ";
                }
                gen.gen_scope(stmt_if->scope, test_label);
                if (stmt_if->pred.has_value()) {
                    std::vector<std::string> labels;
                    labels.push_back(label);
                    gen.gen_if_pred(stmt_if->pred.value(), labels, conditions);
                }
                gen.released_label();
            }

            void operator()(const NodeStmtMinecraftCommand *stmt_minecraft_command) const {
                gen.output(stmt_minecraft_command->command.value.value() + "\n", conditions);
            }

            void operator()(const NodeStmtMacrosCommand *stmt_macros_command) const {
                if (ff.has_value()) {
                    gen.write_macros_function(ff.value(), lc.value(), stmt_macros_command, conditions);
                    gen.output("data modify storage minecraft:__" + gen.m_file_name + " r set value {}\n", conditions);
                    for (int i = 0; i < lc; ++i) {
                        gen.output(
                            "data modify storage minecraft:__" + gen.m_file_name + " r." + gen.get_label(i) +
                            " set from storage minecraft:__"
                            + gen.m_file_name + " " + gen.get_label(i) + "\n", conditions);
                    }
                } else {
                    std::fstream function_file = gen.create_new_function_file();
                    int label_count = 0;
                    gen.write_macros_function(function_file, label_count, stmt_macros_command, conditions);
                    function_file.close();
                    gen.output("data modify storage minecraft:__" + gen.m_file_name + " r set value {}\n", conditions);
                    for (int i = 0; i < label_count; ++i) {
                        gen.output(
                            "data modify storage minecraft:__" + gen.m_file_name + " r." + gen.get_label(i) +
                            " set from storage minecraft:__"
                            + gen.m_file_name + " " + gen.get_label(i) + "\n", conditions);
                    }
                    gen.output(
                        "function " + gen.m_file_name + ":" + gen.get_function_file_name() +
                        " with storage minecraft:__" +
                        gen.m_file_name + " r\n",
                        conditions);
                    for (int i = 0; i < label_count; ++i) gen.released_label();
                }
            }

            void operator()(const NodeStmtWhile *stmt_while) const {
                gen.gen_while(stmt_while->expr, stmt_while->scope, {}, conditions);
            }

            void operator()(const NodeStmtFor *stmt_for) const {
                gen.gen_stmt(stmt_for->let, {}, {}, conditions);
                gen.gen_while(stmt_for->expr, stmt_for->while_->scope, stmt_for->end, conditions);
            }
        };

        StmtVisitor visitor{.gen = *this, .ff = ff, .lc = lc, .conditions = conditions};
        std::visit(visitor, stmt->var);
    }

    static std::string extract_x_from_dollar_format(const std::string &a) {
        if (a.length() < 3) {
            throw std::runtime_error("Input string is too short, expected format: $(x)");
        }
        if (const std::string prefix = "$("; a.substr(0, 2) != prefix) {
            throw std::runtime_error("Input string does not start with '$(': " + a);
        }
        if (a.back() != ')') {
            throw std::runtime_error("Input string does not end with ')': " + a);
        }
        return a.substr(2, a.length() - 3);
    }

    void gen_prog() {
        m_output << "# Generated by RedScript " << VERSION << "\n";
        m_output << "data modify storage minecraft:__" << m_file_name << " __stack set value []\n";
        m_output << "scoreboard objectives add __test dummy\n";
        m_output << "scoreboard objectives add _int_ dummy\n";

        for (const NodeStmt *stmt: m_prog.stmts) {
            gen_stmt(stmt);
        }

        m_output << "return 0\n";
        m_output << "# program is stop.";
        m_output.close();
    }

private:
    void push(const std::string &reg, const std::optional<std::string> &conditions = {}) {
        output(
            "data modify storage minecraft:__" + m_file_name + " __stack append from storage minecraft:__" +
            m_file_name + " " + reg + "\n", conditions);
        m_stack_size++;
    }

    void push_int(const std::string &value, const std::optional<std::string> &conditions = {}) {
        output("data modify storage minecraft:__test __stack append value " + value + "\n", conditions);
        m_stack_size++;
    }

    void pop(const std::string &reg, const std::optional<std::string> &conditions = {}) {
        output(
            "execute store result score " + reg + " __" + m_file_name + " run data get storage minecraft:__" +
            m_file_name +
            " __stack[-1] 1\n", conditions);
        output("data remove storage minecraft:__" + m_file_name + " __stack[-1]\n", conditions);
        m_stack_size--;
    }

    void pop_to_nbt(const std::string &reg, const std::optional<std::string> &conditions = {}) {
        output(
            "execute store result storage minecraft:__" + m_file_name + " " + reg +
            " int 1 run data get storage minecraft:__" +
            m_file_name + " __stack[-1] 1\n", conditions);
        output("data remove storage minecraft:__" + m_file_name + " __stack[-1]\n", conditions);
        m_stack_size--;
    }

    void only_pop(const std::optional<std::string> &conditions = {}) {
        output("data remove storage minecraft:__" + m_file_name + " __stack[-1]\n", conditions);
        m_stack_size--;
    }

    void move_int(const std::string &reg, const std::string &value, const std::optional<std::string> &conditions = {}) {
        m_output << "data modify storage minecraft:__" << m_file_name << " " << reg << " set value " <<
                value << "\n";
        output("scoreboard players set " + reg + " __" + m_file_name + " " + value + "\n", conditions);
    }

    void move_reg(const std::string &reg, const std::string &ret) {
        m_output << "data modify storage minecraft:__" << m_file_name << " " << reg << " set from storage minecraft:__"
                << m_file_name << " " << ret << "\n";
    }

    void move_reg_add(const std::string &reg, const std::string &ret, const std::string &offset) {
        m_output << "execute store result score r __" << m_file_name << " run data get storage minecraft:__" <<
                m_file_name << " " << ret << " 1 \n";
        load_int(offset);
        m_output << "scoreboard players operation r __" << m_file_name << " += " << offset << " _int_\n";
        m_output << "execute store result storage minecraft:__" << m_file_name <<
                " r int 1 run scoreboard players get r __" << m_file_name << "\n";
        move_reg(reg, "r");
    }

    void load_int(const std::string &i) {
        m_output << "scoreboard players set " << i << " _int_ " << i << "\n";
    }

    void get_value(const std::string &reg, const std::string &ret) {
        m_output << "data modify storage minecraft:__" << m_file_name << " r set value {\"ret\":" << ret << "\"}\n";
        m_output << "execute store result storage minecraft:__" << m_file_name <<
                " r.index int 1 run data get storage minecraft:__" << m_file_name << " " << reg << "\n";
        m_output << "function test:__util/get_data with storage minecraft:__" << m_file_name << " r\n";
    }

    void score_move(const std::string &reg, const std::string &label,
                    const std::optional<std::string> &conditions = {}) {
        output("scoreboard players operation " + label + " __" + m_file_name + " = " + reg + " __" + m_file_name + "\n",
               conditions);
    }

    void to_nbt(const std::string &reg, const std::optional<std::string> &conditions = {}) {
        output("execute store result storage minecraft:__" + m_file_name +
               " " + reg + " int 1 run scoreboard players get " + reg + " __" + m_file_name + "\n", conditions);
    }

    void output(const std::string &comm, const std::optional<std::string> &conditions = {}) {
        if (conditions.has_value()) {
            m_output << "execute " << conditions.value() << "run " << comm;
        } else {
            m_output << comm;
        }
    }

    static void output(std::fstream &stream, const std::string &comm,
                       const std::optional<std::string> &conditions = {}) {
        if (conditions.has_value()) {
            stream << "execute " << conditions.value() << "run " << comm;
        } else {
            stream << comm;
        }
    }

    void begin_scope() {
        m_scopes.push_back(m_vars.size());
    }

    void end_scope() {
        const size_t pop_count = m_vars.size() - m_scopes.back();
        for (int i = 0; i < pop_count; i++) {
            only_pop();
            m_vars.pop_back();
        }
        m_scopes.pop_back();
    }

    std::string create_label() {
        std::stringstream label_tag;
        label_tag << "main_" << m_label_count;
        m_label_count++;
        return label_tag.str();
    }

    std::string get_label(const int index = 0) const {
        std::stringstream label_tag;
        label_tag << "main_" << m_label_count - 1 - index;
        return label_tag.str();
    }

    std::fstream create_new_function_file() {
        std::stringstream function_file_name;
        function_file_name << m_file_path << m_file_name << "_" << m_function_file_count << ".mcfunction";
        std::fstream function_file(function_file_name.str(), std::ios::out);
        m_function_file_count++;
        return function_file;
    }

    void back_function_file() {
        m_function_file_count--;
    }

    std::string get_function_file_name() const {
        std::stringstream function_file_name;
        function_file_name << m_file_name << "_" << (m_function_file_count - 1);
        return function_file_name.str();
    }

    void write_macros_function(std::fstream &function_file, int &label_count,
                               const NodeStmtMacrosCommand *stmt_macros_command,
                               const std::optional<std::string> &conditions = {}) {
        output(function_file, "$", conditions);
        for (const Token &command: stmt_macros_command->commands) {
            if (command.type == TokenType::macros_var) {
                gen_expr(stmt_macros_command->vars.at(label_count));
                std::string label = create_label();
                label_count++;
                pop_to_nbt(label);
                output(function_file, "$(" + label);
            } else {
                output(function_file, command.value.value());
            }
        }
        output(function_file, "\n");
    }

    void released_label() {
        m_label_count--;
    }

    struct Var {
        std::string name;
        size_t stack_loc;
    };

    const NodeProg m_prog;
    const std::string m_file_name;
    const std::string m_file_path;
    std::fstream m_output;
    size_t m_stack_size = 0;
    std::vector<Var> m_vars{};
    std::vector<size_t> m_scopes{};
    int m_label_count = 0;
    int m_function_file_count = 0;
};

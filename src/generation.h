#pragma once

#include <algorithm>
#include <cassert>

#include "parser.h"

const inline auto VERSION = "v0.5.5";

class MemoryManagement {
public:
    struct MemoryBlock {
        size_t size;
        size_t point = -1;
    };

    explicit MemoryManagement(std::string file_name)
        : m_file_name(std::move(file_name)) {
    }

    MemoryBlock new_memory(const size_t len) {
        MemoryBlock memory_block{.size = len};
        if (free_memory.empty()) {
            memory_block.point = m_heap_size;
            using_memory.push_back(memory_block);
            m_heap_size += len;
        } else {
            MemoryBlock &min_memory_block = free_memory.at(0);
            for (const MemoryBlock block: free_memory) {
                if (block.size >= len) {
                    if (min_memory_block.size > block.size) {
                        min_memory_block = block;
                    }
                }
            }
            if (min_memory_block.size == len) {
                using_memory.push_back(min_memory_block);
                std::swap(min_memory_block, free_memory.back());
                free_memory.pop_back();
                memory_block = min_memory_block;
            } else {
                min_memory_block.size -= len;
                memory_block.point = min_memory_block.point;
                min_memory_block.point += len;
            }
        }
        return memory_block;
    }

    [[nodiscard]] size_t get_heap_size() const {
        return m_heap_size;
    }

private:
    std::string m_file_name;
    size_t m_heap_size = 1;
    std::vector<MemoryBlock> using_memory{};
    std::vector<MemoryBlock> free_memory{};
};

class Generator {
public:
    struct Var {
        std::string name;
        std::string type;
        size_t stack_loc;
        MemoryManagement::MemoryBlock memory_block{};
        std::vector<Var> vars{};
        std::vector<NodeExpr *> expr{};
        std::vector<NodeStmtDefinition> def{};
    };

    explicit Generator(NodeProg prog, std::string file_name, std::string file_path)
        : m_prog(std::move(prog)), m_file_name(std::move(file_name)), m_file_path(std::move(file_path)) {
        std::fstream os(m_file_path + "main.mcfunction", std::ios::out);
        m_output_stack.push_back(std::move(os));
        m_vars.push_back(m_int_type);
        m_vars.push_back(m_struct_type);
    }

    void gen_term_attribute(const NodeTermAttribute &term_attribute,
                            const std::vector<Var> &att_vars,
                            const std::optional<std::string> &conditions = {},
                            const bool point_offset = false) {
        struct TermVisitor {
            Generator &gen;
            const NodeTermAttribute &term_attribute;
            const std::optional<std::string> &conditions = {};
            const bool point_offset;
            const std::vector<Var> &att_vars;

            void operator()(const NodeTermIdent *term_ident) const {
                const auto obj = std::find_if(
                    att_vars.cbegin(),
                    att_vars.cend(),
                    [&](const Var &var) {
                        return var.name == term_attribute.ident.value.value();
                    });
                if (obj == att_vars.cend()) {
                    print_error("Undeclared identifier: " + term_ident->ident.value.value());
                }
                if (point_offset) {
                    gen.push_int("0", conditions);
                } else {
                    std::stringstream offset;
                    offset << "__stack[-" << (gen.m_stack_size - obj->stack_loc) << "]";
                    gen.push(offset.str(), conditions);
                }
                const auto ty = std::find_if(
                    gen.m_vars.cbegin(),
                    gen.m_vars.cend(),
                    [&](const Var &var) {
                        return var.name == obj->type;
                    });
                if (ty == gen.m_vars.cend()) {
                    print_error("Undeclared type: " + obj->type);
                }
                for (const auto &v: ty->vars) {
                    if (v.name == term_ident->ident.value.value()) {
                        goto has_var;
                    }
                    gen.push_int("1", conditions);
                    gen.stack_add(conditions);
                }
                print_error("Undeclared identifier: " + term_ident->ident.value.value());
            has_var:
                if (point_offset == 0) {
                    gen.output("data modify storage minecraft:__" + gen.m_file_name + " hg set value {}\n",
                               conditions);
                    gen.pop_to_nbt("hg.index", conditions);
                    gen.output(
                        "execute store result storage minecraft:__" + gen.m_file_name + " rax int 1 run function " +
                        gen.m_file_name + ":__util/get_heap_value with storage minecraft:__" + gen.m_file_name +
                        " hg\n",
                        conditions);
                    gen.push("rax", conditions);
                } else {
                    gen.stack_add(conditions);
                    gen.output("data modify storage minecraft:__" + gen.m_file_name + " hg set value {}\n",
                               conditions);
                    gen.pop_to_nbt("hg.index", conditions);
                    gen.output(
                        "execute store result storage minecraft:__" + gen.m_file_name + " rax int 1 run function " + gen
                        .m_file_name + ":__util/get_heap_value with storage minecraft:__" + gen.m_file_name + " hg\n",
                        conditions);
                    gen.push("rax", conditions);
                }
            }

            void operator()(const NodeTermAttribute *term_attribute_) const {
                const auto obj = std::find_if(
                    att_vars.cbegin(),
                    att_vars.cend(),
                    [&](const Var &var) {
                        return var.name == term_attribute.ident.value.value();
                    });
                if (obj == att_vars.cend()) {
                    print_error("Undeclared identifier: " + term_attribute_->ident.value.value());
                }
                size_t point = point_offset + obj->memory_block.point;
                const auto ty = std::find_if(
                    gen.m_vars.cbegin(),
                    gen.m_vars.cend(),
                    [&](const Var &var) {
                        return var.name == obj->type;
                    });
                if (ty == gen.m_vars.cend()) {
                    print_error("Undeclared type: " + obj->type);
                }
                for (const auto &v: ty->vars) {
                    if (v.name == term_attribute_->ident.value.value()) {
                        goto has_var;
                    }
                    point++;
                }
                print_error("Undeclared identifier: " + term_attribute_->ident.value.value());
            has_var:
                if (point_offset == 0) {
                    std::stringstream offset;
                    offset << "__heap[" << point << "]";
                    gen.push(offset.str(), conditions);
                    gen.gen_term_attribute(*term_attribute_, ty->vars, conditions, true);
                } else {
                    if (point != 0) {
                        gen.push_int(std::to_string(point), conditions);
                        gen.stack_add(conditions);
                        gen.output("data modify storage minecraft:__" + gen.m_file_name + " hg set value {}\n",
                                   conditions);
                    }
                    gen.pop_to_nbt("hg.index", conditions);
                    gen.output(
                        "execute store result storage minecraft:__" + gen.m_file_name + " rax int 1 run function " + gen
                        .m_file_name + ":__util/get_heap_value with storage minecraft:__" + gen.m_file_name + " hg\n",
                        conditions);
                    gen.push("rax", conditions);
                    gen.gen_term_attribute(*term_attribute_, ty->vars, conditions, true);
                }
            }
        };
        TermVisitor visitor({
            .gen = *this, .term_attribute = term_attribute, .conditions = conditions,
            .point_offset = point_offset, .att_vars = att_vars
        });
        std::visit(visitor, term_attribute.var->var);
    }

    void push_var_point(const NodeTermAttribute &term_attribute,
                        const std::vector<Var> &att_vars,
                        const std::optional<std::string> &conditions = {},
                        const bool point_offset = false) {
        struct TermVisitor {
            Generator &gen;
            const NodeTermAttribute &term_attribute;
            const std::optional<std::string> &conditions = {};
            const bool point_offset;
            const std::vector<Var> &att_vars;

            void operator()(const NodeTermIdent *term_ident) const {
                const auto obj = std::find_if(
                    att_vars.cbegin(),
                    att_vars.cend(),
                    [&](const Var &var) {
                        return var.name == term_attribute.ident.value.value();
                    });
                if (obj == att_vars.cend()) {
                    print_error("Undeclared identifier: " + term_ident->ident.value.value());
                }
                if (point_offset) {
                    gen.push_int("0", conditions);
                } else {
                    std::stringstream offset;
                    offset << "__stack[-" << (gen.m_stack_size - obj->stack_loc) << "]";
                    gen.push(offset.str(), conditions);
                }
                const auto ty = std::find_if(
                    gen.m_vars.cbegin(),
                    gen.m_vars.cend(),
                    [&](const Var &var) {
                        return var.name == obj->type;
                    });
                if (ty == gen.m_vars.cend()) {
                    print_error("Undeclared type: " + obj->type);
                }
                for (const auto &v: ty->vars) {
                    if (v.name == term_ident->ident.value.value()) {
                        goto has_var;
                    }
                    gen.push_int("1", conditions);
                    gen.stack_add(conditions);
                }
                print_error("Undeclared identifier: " + term_ident->ident.value.value());
            has_var:
                if (point_offset) {
                    gen.stack_add(conditions);
                    gen.output("data modify storage minecraft:__" + gen.m_file_name + " hg set value {}\n",
                               conditions);
                }
            }

            void operator()(const NodeTermAttribute *term_attribute_) const {
                const auto obj = std::find_if(
                    att_vars.cbegin(),
                    att_vars.cend(),
                    [&](const Var &var) {
                        return var.name == term_attribute.ident.value.value();
                    });
                if (obj == att_vars.cend()) {
                    print_error("Undeclared identifier: " + term_attribute_->ident.value.value());
                }
                std::stringstream offset;
                offset << "__stack[-" << (gen.m_stack_size - obj->stack_loc) << "]";
                gen.push(offset.str(), conditions);
                const auto ty = std::find_if(
                    gen.m_vars.cbegin(),
                    gen.m_vars.cend(),
                    [&](const Var &var) {
                        return var.name == obj->type;
                    });
                if (ty == gen.m_vars.cend()) {
                    print_error("Undeclared type: " + obj->type);
                }
                for (const auto &v: ty->vars) {
                    if (v.name == term_attribute_->ident.value.value()) {
                        goto has_var;
                    }
                    gen.push_int("1", conditions);
                    gen.stack_add(conditions);
                }
                print_error("Undeclared identifier: " + term_attribute_->ident.value.value());
            has_var:
                if (point_offset == 0) {
                    gen.output("data modify storage minecraft:__" + gen.m_file_name + " hg set value {}\n",
                               conditions);
                    gen.pop_to_nbt("hg.index", conditions);
                    gen.output(
                        "execute store result storage minecraft:__" + gen.m_file_name + " rax int 1 run function " +
                        gen.m_file_name + ":__util/get_heap_value with storage minecraft:__" + gen.m_file_name +
                        " hg\n",
                        conditions);
                    gen.push("rax", conditions);
                    gen.push_var_point(*term_attribute_, ty->vars, conditions, true);
                } else {
                    gen.stack_add(conditions);
                    gen.output("data modify storage minecraft:__" + gen.m_file_name + " hg set value {}\n",
                               conditions);
                    gen.pop_to_nbt("hg.index", conditions);
                    gen.output(
                        "execute store result storage minecraft:__" + gen.m_file_name + " rax int 1 run function " + gen
                        .m_file_name + ":__util/get_heap_value with storage minecraft:__" + gen.m_file_name + " hg\n",
                        conditions);
                    gen.push("rax", conditions);
                    gen.push_var_point(*term_attribute_, ty->vars, conditions, true);
                }
            }
        };
        TermVisitor visitor({
            .gen = *this, .term_attribute = term_attribute, .conditions = conditions,
            .point_offset = point_offset, .att_vars = att_vars
        });
        std::visit(visitor, term_attribute.var->var);
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
                struct TermVariableVisitor {
                    Generator &gen;
                    const std::optional<std::string> &conditions = {};

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
                        gen.output(
                            "execute store result score pp __" + gen.m_file_name + " run data get storage minecraft:__"
                            + gen.
                            m_file_name + " " +
                            offset.str() +
                            "\n", conditions);
                        gen.output("scoreboard players add pp __" + gen.m_file_name + " 1\n", conditions);
                        gen.to_nbt("pp", conditions);
                        gen.output(
                            "data modify storage minecraft:__" + gen.m_file_name + " " + offset.str() +
                            " set from storage minecraft:__" + gen.m_file_name + " pp\n",
                            conditions);
                        gen.push("pp", conditions);
                    }

                    void operator()(const NodeTermAttribute *term_attribute) const {
                        assert(false);
                    }
                };

                TermVariableVisitor visitor{.gen = gen, .conditions = conditions};
                std::visit(visitor, term_plus_plus->ident->var);
            }

            void operator()(const NodeTermMinusMinus *term_minus_minus) const {
                struct TermVariableVisitor {
                    Generator &gen;
                    const std::optional<std::string> &conditions = {};

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
                        gen.output(
                            "execute store result score pp __" + gen.m_file_name + " run data get storage minecraft:__"
                            + gen.
                            m_file_name + " " +
                            offset.str() +
                            "\n", conditions);
                        gen.load_int("1");
                        gen.output("scoreboard players operation pp __" + gen.m_file_name + " -= 1 _int_\n",
                                   conditions);
                        gen.to_nbt("pp", conditions);
                        gen.output(
                            "data modify storage minecraft:__" + gen.m_file_name + " " + offset.str() +
                            " set from storage minecraft:__" + gen.m_file_name + " pp\n",
                            conditions);
                        gen.push("pp", conditions);
                    }

                    void operator()(const NodeTermAttribute *term_attribute) const {
                        assert(false);
                    }
                };

                TermVariableVisitor visitor{.gen = gen, .conditions = conditions};
                std::visit(visitor, term_minus_minus->ident->var);
            }

            void operator()(const NodeTermTime *term_time) const {
                gen.to_nbt("__time", conditions);
                gen.push("__time", conditions);
            }

            void operator()(const NodeTermNull *term_null) const {
                gen.push_int("0", conditions);
            }

            void operator()(const NodeTermAttribute *term_attribute) const {
                gen.gen_term_attribute(*term_attribute, gen.m_vars, conditions);
            }

            void operator()(const NodeTermMinecraftCommand *term_minecraft_command) const {
                gen.output(
                    "execute store result storage minecraft:__" + gen.m_file_name +
                    " cr int 1 run execute as @a[tag=__" + gen.m_file_name + ",limit=1] at @s run " +
                    term_minecraft_command->command.value.value() + "\n", conditions);
                gen.push("cr", conditions);
            }

            void operator()(const NodeTermMacrosCommand *term_macros_command) const {
                gen.create_new_function_file_no_call();
                const std::string file_name = gen.get_function_file_name();
                int label_count = 0;
                gen.write_macros_function(label_count, term_macros_command, conditions);
                gen.only_close_function_file();
                gen.output("data modify storage minecraft:__" + gen.m_file_name + " r set value {}\n", conditions);
                for (int i = 0; i < label_count; ++i) {
                    gen.output("data modify storage minecraft:__" + gen.m_file_name + " r." + gen.get_label(i) +
                               " set from storage minecraft:__"
                               + gen.m_file_name + " " + gen.get_label(i) + "\n", conditions);
                }
                gen.output("function " + gen.m_file_name + ":" + file_name +
                           " with storage minecraft:__" +
                           gen.m_file_name + " r\n",
                           conditions);
                for (int i = 0; i < label_count; ++i) gen.released_label();
            }

            void operator()(NodeTermVariable *term_variable) const {
                struct TermVariableVisitor {
                    Generator &gen;
                    const std::optional<std::string> &conditions;

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

                    void operator()(const NodeTermAttribute *term_attribute) const {
                        gen.gen_term_attribute(*term_attribute, gen.m_vars, conditions);
                    }
                };

                TermVariableVisitor visitor{.gen = gen, .conditions = conditions};
                std::visit(visitor, term_variable->var);
            }

            void operator()(const NodeTermFunctionCall *term_function_call) const {
                struct TermVariableVisitor {
                    Generator &gen;
                    const std::optional<std::string> &conditions = {};

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
                        gen.output(
                            "execute store result storage minecraft:__" + gen.m_file_name +
                            " rax int 1 run function " + gen.m_file_name + ":__func/" + term_ident->ident.value.value()
                            + "\n",
                            conditions);
                        gen.push("rax", conditions);
                    }

                    void operator()(const NodeTermAttribute *term_attribute) const {
                        assert(false);
                    }
                };

                TermVariableVisitor visitor{.gen = gen, .conditions = conditions};
                std::visit(visitor, term_function_call->func->var);
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

            void operator()(const NodeBinExprMod *mod) const {
                gen.gen_expr(mod->rhs, conditions);
                gen.gen_expr(mod->lhs, conditions);
                gen.pop("rax", conditions);
                gen.pop("rbx", conditions);
                gen.output("scoreboard players operation rax __" + gen.m_file_name + " %= rbx __" + gen.
                           m_file_name + "\n", conditions);
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

    void gen_expr(const NodeExpr *expr, const std::optional<std::string> &conditions) {
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
            create_new_function_file();
            const std::string function_name = get_function_file_name();
            begin_scope();
            for (const NodeStmt *stmt: scope->stmts) {
                gen_stmt(stmt, {}, {}, conditions);
            }
            end_scope();
            close_function_file(true, 1);
        } else {
            begin_scope();
            for (const NodeStmt *stmt: scope->stmts) {
                gen_stmt(stmt, {}, {}, conditions);
            }
            end_scope();
        }
    }

    void gen_while(const NodeExpr *expr, const std::optional<NodeScope *> scope,
                   const std::optional<NodeStmt *> &stmt_conditions = {},
                   const std::optional<std::string> &conditions = {}) {
        if (!scope.value()->has_wait) {
            create_new_function_file();
            const std::string function_name = get_function_file_name();
            gen_expr(expr, conditions);
            pop("rax", conditions);
            output(
                "execute if score rax __" + m_file_name + " matches 0 run return 0\n", conditions);
            begin_scope();
            if (scope.has_value()) {
                for (const NodeStmt *stmt: scope.value()->stmts) {
                    gen_stmt(stmt, {}, {}, conditions);
                }
            }
            if (stmt_conditions.has_value()) {
                gen_stmt(stmt_conditions.value(), {}, {}, conditions);
            }
            end_scope();
            output("function " + m_file_name + ":" + function_name + "\n", conditions);
            close_function_file();
        } else {
            create_new_function_file();
            const std::string function_name = get_function_file_name();
            gen_expr(expr, conditions);
            pop("rax", conditions);
            no_close_function_file_no_push();
            output(
                "execute if score rax __" + m_file_name + " matches 0 run return run function " + m_file_name + ":" +
                get_function_file_name() + "\n", conditions);
            begin_scope();
            if (scope.has_value()) {
                for (const NodeStmt *stmt: scope.value()->stmts) {
                    gen_stmt(stmt, {}, {}, conditions);
                }
            }
            if (stmt_conditions.has_value()) {
                gen_stmt(stmt_conditions.value(), {}, {}, conditions);
            }
            end_scope();
            output("function " + m_file_name + ":" + function_name + "\n", conditions);
            only_close_function_file();
        }
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

    void gen_stmt_struct(const NodeStmtStruct *stmt_struct, const std::optional<std::string> &conditions = {}) {
        const auto it = std::find_if(
            m_vars.cbegin(),
            m_vars.cend(),
            [&](const Var &var) {
                return var.name == stmt_struct->ident.value.value();
            });
        if (it != m_vars.cend()) {
            print_error("Identifier already used:" + stmt_struct->ident.value.value());
        }
        std::vector<Var> names;
        std::vector<NodeExpr *> exprs;
        std::vector<NodeStmtDefinition> def;
        size_t len = 0;
        gen_stmt_struct_attribute(names, exprs, def, stmt_struct->attributes, len);
        m_vars.push_back({
            .name = stmt_struct->ident.value.value(),
            .type = m_struct_type.name,
            .stack_loc = m_stack_size,
            .memory_block = MemoryManagement::MemoryBlock{.size = len},
            .vars = names,
            .expr = exprs,
            .def = def
        });
    }

    void gen_stmt_definition(const NodeStmtDefinition *definition, const std::optional<std::string> &conditions = {},
                             const bool no_var = false) {
        struct DefinitionVisitor {
            Generator &gen;
            const bool no_var;
            const std::optional<std::string> &conditions;

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
                if (!no_var) {
                    gen.m_vars.push_back({
                        .name = stmt_let->ident.value.value(),
                        .stack_loc = gen.m_stack_size
                    });
                }
                gen.gen_expr(stmt_let->expr, conditions);
            }

            void operator()(const NodeStmtStruct *stmt_struct) const {
                gen.gen_stmt_struct(stmt_struct, conditions);
            }

            void operator()(const NodeStmtCustomType *stmt_custom) const {
                assert(false && "fuck your mother and father");
            }

            void operator()(const NodeStmtDeclarationVariable *stmt_declaration_variable) const {
                const auto it = std::find_if(
                    gen.m_vars.cbegin(),
                    gen.m_vars.cend(),
                    [&](const Var &var) {
                        return var.name == stmt_declaration_variable->ident.value.value();
                    });
                if (it != gen.m_vars.cend()) {
                    print_error("Identifier already used:" + stmt_declaration_variable->ident.value.value());
                }
                if (stmt_declaration_variable->ident_type.type == TokenType::ty_int) {
                    gen.m_vars.push_back({
                        .name = stmt_declaration_variable->ident.value.value(),
                        .type = gen.m_int_type.name,
                        .stack_loc = gen.m_stack_size
                    });
                    gen.push_int("0", conditions);
                } else {
                    const auto it_type = std::find_if(
                        gen.m_vars.cbegin(),
                        gen.m_vars.cend(),
                        [stmt_declaration_variable](const Var &var) {
                            return var.name == stmt_declaration_variable->ident_type.value.value();
                        });
                    if (it_type == gen.m_vars.cend()) {
                        print_error("Undefined type: " + stmt_declaration_variable->ident_type.value.value());
                    }
                    const Var v({
                        .name = stmt_declaration_variable->ident.value.value(),
                        .type = it_type->name,
                        .stack_loc = gen.m_stack_size,
                        .memory_block = gen.m_memory_management.new_memory(it_type->memory_block.size)
                    });
                    gen.push_int(std::to_string(v.memory_block.point), conditions);
                    for (int i = 0; i < it_type->vars.size(); i++) {
                        if (it_type->expr.at(i) != nullptr) {
                            gen.gen_expr(it_type->expr.at(i), conditions);
                        } else {
                            gen.gen_stmt_definition(&it_type->def.at(i), conditions, true);
                        }
                        gen.pop_to_nbt("rax", conditions);
                        gen.set_heap_reg(v.memory_block.point + i, "rax", conditions);
                    }
                    if (!no_var) {
                        gen.m_vars.push_back(v);
                    }
                }
            }
        };

        DefinitionVisitor visitor({.gen = *this, .no_var = no_var, .conditions = conditions});
        std::visit(visitor, definition->var);
    }

    void gen_stmt_struct_attribute(std::vector<Var> &vars, std::vector<NodeExpr *> &values,
                                   std::vector<NodeStmtDefinition> &def,
                                   const std::vector<NodeStmtDefinition *> &attributes,
                                   size_t &len,
                                   const std::optional<std::string> &conditions = {}) {
        struct DefinitionVisitor {
            Generator &gen;
            std::vector<Var> &vars;
            std::vector<NodeExpr *> &values;
            std::vector<NodeStmtDefinition> &def;
            size_t &len;
            const std::optional<std::string> &conditions;

            void operator()(const NodeStmtLet *stmt_let) const {
                const auto it = std::find_if(
                    vars.cbegin(),
                    vars.cend(),
                    [&](const Var &var) {
                        return var.name == stmt_let->ident.value.value();
                    });
                if (it != vars.cend()) {
                    print_error("Identifier already used:" + stmt_let->ident.value.value());
                }
                len++;
                vars.push_back({
                    .name = stmt_let->ident.value.value(),
                    .type = gen.m_int_type.name,
                    .stack_loc = gen.m_stack_size
                });
                values.push_back(stmt_let->expr);
                def.emplace_back();
            }

            void operator()(const NodeStmtStruct *stmt_struct) const {
                print_error("You cannot define a struct in a struct");
            }

            void operator()(const NodeStmtCustomType *stmt_custom) const {
                assert(false);
            }

            void operator()(NodeStmtDeclarationVariable *stmt_declaration_variable) const {
                const auto it = std::find_if(
                    vars.cbegin(),
                    vars.cend(),
                    [&](const Var &var) {
                        return var.name == stmt_declaration_variable->ident.value.value();
                    });
                if (it != vars.cend()) {
                    print_error("Identifier already used: " + stmt_declaration_variable->ident.value.value());
                }
                const auto ty = std::find_if(
                    gen.m_vars.cbegin(),
                    gen.m_vars.cend(),
                    [&](const Var &var) {
                        return var.name == stmt_declaration_variable->ident_type.value.value();
                    });
                if (ty == gen.m_vars.cend()) {
                    print_error("Undefined type: " + stmt_declaration_variable->ident_type.value.value());
                }
                len++;
                vars.push_back({
                    .name = stmt_declaration_variable->ident.value.value(),
                    .type = stmt_declaration_variable->ident_type.value.value(),
                    .stack_loc = gen.m_stack_size
                });
                values.push_back(nullptr);
                def.push_back({.var = stmt_declaration_variable});
            }
        };

        DefinitionVisitor visitor({
            .gen = *this, .vars = vars, .values = values, .def = def, .len = len, .conditions = conditions
        });

        for (auto &attribute: attributes) {
            std::visit(visitor, attribute->var);
        }
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
                gen.m_output_stack.back() << "# program is stop.\n";
            }

            void operator()(const NodeStmtDefinition *stmt_definition) const {
                gen.gen_stmt_definition(stmt_definition, conditions);
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

            void operator()(const NodeStmtWhile *stmt_while) const {
                gen.gen_while(stmt_while->expr, stmt_while->scope, {}, conditions);
            }

            void operator()(const NodeStmtFor *stmt_for) const {
                gen.gen_stmt(stmt_for->let, {}, {}, conditions);
                gen.gen_while(stmt_for->expr, stmt_for->while_->scope, stmt_for->end, conditions);
            }

            void operator()(const NodeStmtWait *stmt_wait) const {
                const std::string new_file_name = gen.m_file_name + ":" + gen.get_function_file_name(1);
                gen.gen_expr(stmt_wait->expr, conditions);
                gen.output("data modify storage minecraft:__" + gen.m_file_name + " w set value {}\n", conditions);
                gen.output(
                    "data modify storage minecraft:__" + gen.m_file_name + " w.func set value \"" + new_file_name +
                    "\"\n",
                    conditions);
                gen.pop("rax", conditions);
                gen.to_nbt("rax", conditions);
                gen.move_reg("w.time", "rax", conditions);
                gen.output(
                    "execute unless score rax __" + gen.m_file_name + " matches 0 run function " + gen.m_file_name +
                    ":__util/wait_func with storage minecraft:__" + gen.m_file_name +
                    " w\n", conditions);
                gen.output(
                    "execute if score rax __" + gen.m_file_name + " matches 0 run function " + new_file_name + "\n",
                    conditions);
                gen.close_function_file_no_push();
            }

            void operator()(const NodeStmtAssert *stmt_assert) const {
                gen.gen_expr(stmt_assert->expr, conditions);
                gen.pop("rax", conditions);
                gen.output(
                    "execute if score rax __" + gen.m_file_name +
                    R"( matches 0 run tellraw @a {"color":"red","text":"Assertion triggered at line )"
                    + std::to_string(stmt_assert->line) + " in file " + stmt_assert->file_name + "\"}\n", conditions);
                gen.output("execute if score rax __" + gen.m_file_name + " matches 0 run return fail\n", conditions);
            }

            void operator()(const NodeStmtNullAssign *stmt_null_assign) const {
                gen.gen_expr(stmt_null_assign->expr, conditions);
                gen.only_pop(conditions);
            }

            void operator()(const NodeStmtAssignAttribute *stmt_assign_attribute) const {
                gen.push_var_point(
                    *std::get<NodeTermAttribute *>(stmt_assign_attribute->ident->var),
                    gen.m_vars,
                    conditions);
                gen.output("data modify storage minecraft:__" + gen.m_file_name + " hg set value {}\n",
                           conditions);
                gen.pop_to_nbt("hg.index", conditions);
                gen.gen_expr(stmt_assign_attribute->expr, conditions);
                gen.pop_to_nbt("hg.value", conditions);
                gen.output(
                    "function " + gen.m_file_name + ":__util/set_heap_value with storage minecraft:__" + gen.m_file_name
                    + " hg\n", conditions);
            }

            void operator()(const NodeStmtFunction *stmt_function) const {
                std::stringstream function_file_name;
                function_file_name << gen.m_file_path << "__func/" << stmt_function->ident.value.value() <<
                        ".mcfunction";
                std::fstream function_file(function_file_name.str(), std::ios::out);
                function_file << "# Generated by RedScript " << VERSION << "\n";
                function_file << "# Function " << stmt_function->ident.value.value() << "\n";
                gen.m_output_stack.push_back(std::move(function_file));
                gen.gen_scope(stmt_function->scope, conditions);
                gen.m_output_stack.back() << "return 0";
                gen.only_close_function_file();
                gen.m_vars.push_back({
                    .name = stmt_function->ident.value.value(),
                    .type = gen.m_function_type.name,
                    .stack_loc = gen.m_stack_size,
                    .memory_block = gen.m_memory_management.new_memory(gen.m_function_type.memory_block.size),
                    .vars = {},
                    .expr = {},
                    .def = {}
                });
            }

            void operator()(const NodeStmtReturn *stmt_return) const {
                gen.only_end_scope();
                if (stmt_return->expr.has_value()) {
                    gen.gen_expr(stmt_return->expr.value(), conditions);
                    gen.pop_to_nbt("rax", conditions);
                    gen.output("return run data get storage minecraft:__" + gen.m_file_name + " rax 1\n", conditions);
                } else {
                    gen.output("return 0\n", conditions);
                }
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
        m_output_stack.back() << "# Generated by RedScript " << VERSION << "\n";
        m_output_stack.back() << "data modify storage minecraft:__" << m_file_name << " __stack set value []\n";
        m_output_stack.back() << "data modify storage minecraft:__" << m_file_name << " __heap set value []\n";
        m_output_stack.back() << "data modify storage minecraft:__" << m_file_name << " __heap append value 0\n";
        m_output_stack.back() << "scoreboard objectives add __" << m_file_name << " dummy\n";
        m_output_stack.back() << "scoreboard players set __time __" << m_file_name << " 0\n";
        m_output_stack.back() << "scoreboard objectives add _int_ dummy\n";
        m_output_stack.back() << "function " + m_file_name + ":__util/reset_heap\n";
        m_output_stack.back() << "execute as @e[tag=__" + m_file_name + "] run tag @s remove __" + m_file_name + "\n";
        m_output_stack.back() << "tag @s add __" + m_file_name + "\n";

        for (const NodeStmt *stmt: m_prog.stmts) {
            gen_stmt(stmt);
        }

        for (auto &i: m_output_stack) {
            i.close();
        }
        m_output_stack.clear();
    }

    ~Generator() {
        for (auto &i: m_output_stack) {
            i.close();
        }
    }

private:
    Var m_int_type{
        .name = "int",
        .memory_block = MemoryManagement::MemoryBlock{.size = 1}
    };

    Var m_struct_type{
        .name = "struct",
        .memory_block = MemoryManagement::MemoryBlock{.size = 1}
    };

    Var m_function_type{
        .name = "function",
        .memory_block = MemoryManagement::MemoryBlock{.size = 1}
    };

    struct FuncFile {
        std::string file_name;
        std::fstream &file_stream;
    };

    void push(const std::string &reg, const std::optional<std::string> &conditions) {
        output(
            "data modify storage minecraft:__" + m_file_name + " __stack append from storage minecraft:__" +
            m_file_name + " " + reg + "\n", conditions);
        m_stack_size++;
    }

    void push_int(const std::string &value, const std::optional<std::string> &conditions) {
        output("data modify storage minecraft:__" + m_file_name + " __stack append value " + value + "\n", conditions);
        m_stack_size++;
    }

    void pop(const std::string &reg, const std::optional<std::string> &conditions) {
        output(
            "execute store result score " + reg + " __" + m_file_name + " run data get storage minecraft:__" +
            m_file_name +
            " __stack[-1] 1\n", conditions);
        output("data remove storage minecraft:__" + m_file_name + " __stack[-1]\n", conditions);
        m_stack_size--;
    }

    void set_heap_value(const size_t address, const std::string &value,
                        const std::optional<std::string> &conditions) {
        output(
            "data modify storage minecraft:__" + m_file_name + " __heap[" + std::to_string(address) + "] set value "
            +
            value + "\n",
            conditions);
    }

    void set_heap_reg(const size_t address, const std::string &reg,
                      const std::optional<std::string> &conditions) {
        output(
            "data modify storage minecraft:__" + m_file_name + " __heap[" + std::to_string(address) + "]" +
            " set from storage minecraft:__" + m_file_name + " " +
            reg + "\n",
            conditions);
    }

    void add_heap_value(const std::optional<std::string> &conditions) {
        output(
            "data modify storage minecraft:__" + m_file_name + " __heap append value []\n",
            conditions);
    }

    void pop_to_nbt(const std::string &reg, const std::optional<std::string> &conditions) {
        output(
            "execute store result storage minecraft:__" + m_file_name + " " + reg +
            " int 1 run data get storage minecraft:__" +
            m_file_name + " __stack[-1] 1\n", conditions);
        output("data remove storage minecraft:__" + m_file_name + " __stack[-1]\n", conditions);
        m_stack_size--;
    }

    void only_pop(const std::optional<std::string> &conditions) {
        output("data remove storage minecraft:__" + m_file_name + " __stack[-1]\n", conditions);
        m_stack_size--;
    }

    void move_int(const std::string &reg, const std::string &value, const std::optional<std::string> &conditions) {
        m_output_stack.back() << "data modify storage minecraft:__" << m_file_name << " " << reg << " set value " <<
                value << "\n";
        output("scoreboard players set " + reg + " __" + m_file_name + " " + value + "\n", conditions);
    }

    void move_reg(const std::string &reg, const std::string &ret, const std::optional<std::string> &conditions) {
        output("data modify storage minecraft:__" + m_file_name + " " + reg + " set from storage minecraft:__"
               + m_file_name + " " + ret + "\n", conditions);
    }

    void move_reg_add(const std::string &reg, const std::string &ret, const std::string &offset) {
        m_output_stack.back() << "execute store result score r __" << m_file_name <<
                " run data get storage minecraft:__" <<
                m_file_name << " " << ret << " 1 \n";
        load_int(offset);
        m_output_stack.back() << "scoreboard players operation r __" << m_file_name << " += " << offset << " _int_\n";
        m_output_stack.back() << "execute store result storage minecraft:__" << m_file_name <<
                " r int 1 run scoreboard players get r __" << m_file_name << "\n";
        move_reg(reg, "r", {});
    }

    void load_int(const std::string &i) {
        m_output_stack.back() << "scoreboard players set " << i << " _int_ " << i << "\n";
    }

    void get_value(const std::string &reg, const std::string &ret) {
        m_output_stack.back() << "data modify storage minecraft:__" << m_file_name << " r set value {\"ret\":" << ret <<
                "\"}\n";
        m_output_stack.back() << "execute store result storage minecraft:__" << m_file_name <<
                " r.index int 1 run data get storage minecraft:__" << m_file_name << " " << reg << "\n";
        m_output_stack.back() << "function " + m_file_name + ":__util/get_data with storage minecraft:__" << m_file_name
                << " r\n";
    }

    void score_move(const std::string &reg, const std::string &label,
                    const std::optional<std::string> &conditions) {
        output("scoreboard players operation " + label + " __" + m_file_name + " = " + reg + " __" + m_file_name + "\n",
               conditions);
    }

    void to_nbt(const std::string &reg, const std::optional<std::string> &conditions) {
        output("execute store result storage minecraft:__" + m_file_name +
               " " + reg + " int 1 run scoreboard players get " + reg + " __" + m_file_name + "\n", conditions);
    }

    void stack_add(const std::optional<std::string> &conditions) {
        pop("rax", conditions);
        pop("rbx", conditions);
        output("scoreboard players operation rax __" + m_file_name + " += rbx __" +
               m_file_name + "\n", conditions);
        to_nbt("rax", conditions);
        push("rax", conditions);
    }

    void output(const std::string &comm, const std::optional<std::string> &conditions) {
        if (conditions.has_value()) {
            m_output_stack.back() << "execute " << conditions.value() << "run " << comm;
        } else {
            m_output_stack.back() << comm;
        }
        m_command_count++;
        if (m_command_count > 65400) {
            only_close_function_file();
            create_new_function_file(1);
            std::cerr << "Warring:The command exceeds the limit";
        }
    }

    void output_dol(const std::string &comm,
                    const std::optional<std::string> &conditions) {
        std::stringstream command;
        if (conditions.has_value()) {
            if (comm.at(0) == '$') {
                command << "$execute " << conditions.value() << "run " << comm.substr(1);
            } else {
                command << "execute " << conditions.value() << "run " << comm;
            }
        } else {
            command << comm;
        }
        output(command.str(), {});
    }

    void begin_scope() {
        m_scopes.push_back(m_vars.size());
    }

    void end_scope() {
        const size_t pop_count = m_vars.size() - m_scopes.back();
        for (int i = 0; i < pop_count; i++) {
            only_pop({});
            m_vars.pop_back();
        }
        m_scopes.pop_back();
    }

    void only_end_scope() {
        const size_t pop_count = m_vars.size() - m_scopes.back();
        for (int i = 0; i < pop_count; i++) {
            only_pop({});
        }
    }

    std::string create_label() {
        std::stringstream label_tag;
        label_tag << "main_" << m_label_count;
        m_label_count++;
        return label_tag.str();
    }

    [[nodiscard]] std::string get_label(const int index = 0) const {
        std::stringstream label_tag;
        label_tag << "main_" << m_label_count - 1 - index;
        return label_tag.str();
    }

    void create_new_function_file(const size_t time = 0) {
        std::stringstream function_file_name;
        function_file_name << m_file_name << "_" << m_function_file_count;
        std::fstream function_file(m_file_path + function_file_name.str() + ".mcfunction", std::ios::out);
        function_file << "# Generated by RedScript " << VERSION << "\n";
        if (time == 0) {
            m_output_stack.back() << "function " << m_file_name << ":" << function_file_name.str() << "\n";
        } else {
            m_output_stack.back() << "schedule function " << m_file_name << ":" << function_file_name.str()
                    << " " << time << "t append \n";
        }
        m_output_stack.push_back(std::move(function_file));
        m_function_file_count++;
    }

    void create_new_function_file_no_call(const size_t time = 0) {
        std::stringstream function_file_name;
        function_file_name << m_file_name << "_" << m_function_file_count;
        std::fstream function_file(m_file_path + function_file_name.str() + ".mcfunction", std::ios::out);
        function_file << "# Generated by RedScript " << VERSION << "\n";
        m_output_stack.push_back(std::move(function_file));
        m_function_file_count++;
    }

    FuncFile create_new_function_file_ret() {
        std::stringstream function_file_name;
        function_file_name << m_file_name << "_" << m_function_file_count;
        std::fstream function_file(m_file_path + function_file_name.str() + ".mcfunction", std::ios::out);
        function_file << "# Generated by RedScript " << VERSION << "\n";
        // if (time == 0) {
        //     m_output_stack.back() << "function " << m_file_name << ":" << function_file_name.str() << "\n";
        // } else {
        //     m_output_stack.back() << "schedule function " << m_file_name << ":" << function_file_name.str()
        //             << " " << time << "t append \n";
        // }
        m_function_file_count++;
        return {
            .file_name = function_file_name.str(),
            .file_stream = function_file
        };
    }

    void close_function_file(const bool is_scheduled = false, const size_t time = 0) {
        if (!is_scheduled) {
            m_output_stack.back().close();
            m_output_stack.pop_back();
        } else {
            std::stringstream function_file_name;
            function_file_name << m_file_name << "_" << m_function_file_count;
            std::fstream function_file(m_file_path + function_file_name.str() + ".mcfunction", std::ios::out);
            function_file << "# Generated by RedScript " << VERSION << "\n";
            if (time == 0) {
                m_output_stack.back() << "function " << m_file_name << ":" << function_file_name.str() << "\n";
            } else {
                m_output_stack.back() << "schedule function " << m_file_name << ":" << function_file_name.str()
                        << " " << time << "t append \n";
            }
            m_function_file_count++;
            m_output_stack.back().close();
            m_output_stack.pop_back();
            m_output_stack.push_back(std::move(function_file));
        }
    }

    void close_function_file_no_push() {
        std::stringstream function_file_name;
        function_file_name << m_file_name << "_" << m_function_file_count;
        std::fstream function_file(m_file_path + function_file_name.str() + ".mcfunction", std::ios::out);
        function_file << "# Generated by RedScript " << VERSION << "\n";
        m_function_file_count++;
        m_output_stack.back().close();
        m_output_stack.pop_back();
        m_output_stack.push_back(std::move(function_file));
    }

    void no_close_function_file_no_push() {
        std::stringstream function_file_name;
        function_file_name << m_file_name << "_" << m_function_file_count;
        std::fstream function_file(m_file_path + function_file_name.str() + ".mcfunction", std::ios::out);
        function_file << "# Generated by RedScript " << VERSION << "\n";
        m_function_file_count++;
        m_output_stack.insert(m_output_stack.end() - 1, std::move(function_file));
    }

    void only_close_function_file() {
        m_output_stack.back().close();
        m_output_stack.pop_back();
    }

    void back_function_file() {
        m_function_file_count--;
    }

    [[nodiscard]] std::string get_function_file_name(const size_t offset = 0) const {
        return m_file_name + "_" + std::to_string(m_function_file_count - 1 + offset);
    }

    void write_macros_function(int &label_count,
                               const NodeTermMacrosCommand *stmt_macros_command,
                               const std::optional<std::string> &conditions = {}) {
        output_dol("$execute store result storage minecraft:__" + m_file_name +
                   " cr int 1 run execute as @a[tag=__" + m_file_name + ",limit=1] at @s run ", conditions);
        for (const Token &command: stmt_macros_command->commands) {
            if (command.type == TokenType::macros_var) {
                std::swap(*(m_output_stack.rbegin()), *(m_output_stack.rbegin() + 1));
                gen_expr(stmt_macros_command->vars.at(label_count), {});
                std::string label = create_label();
                label_count++;
                pop_to_nbt(label, {});
                std::swap(*(m_output_stack.rbegin()), *(m_output_stack.rbegin() + 1));
                output_dol("$(" + label + ")", {});
            } else {
                output_dol(command.value.value(), {});
            }
        }
        output_dol("\n", {});
        push("cr", conditions);
    }

    void released_label() {
        m_label_count--;
    }

    const NodeProg m_prog;
    const std::string m_file_name;
    const std::string m_file_path;
    std::vector<std::fstream> m_output_stack{};
    size_t m_stack_size = 0;
    MemoryManagement m_memory_management{m_file_name};
    std::vector<Var> m_vars{};
    std::vector<size_t> m_scopes{};
    int m_label_count = 0;
    int m_command_count = 20;
    int m_function_file_count = 0;
};

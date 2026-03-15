#pragma once

#include <string>
#include <vector>

enum class TokenType {
    exit,
    int_,
    semi,
    open_paren,
    close_paren,
    ident,
    let,
    eq,
    plus,
    star,
    minus,
    fslash,
    open_curly,
    close_curly,
    if_,
    elif,
    else_,
    test_eq,
    letter,
    better,
    let_eq,
    bet_eq,
    no_eq,
    and_,
    or_,
    not_,
    commands,
    macros_command,
    macros_var,
    macros_start,
    outline,
    while_,
    plus_plus,
    minus_minus,
    for_,
    ty_int,
    wait,
    mod,
    struct_,
    dot,
    separation,
    assert,
    null,
    void_,
    return_,
};

inline std::string to_string(const TokenType type) {
    switch (type) {
        case TokenType::exit:
            return "'exit'";
        case TokenType::int_:
            return "int literal";
        case TokenType::semi:
            return "';'";
        case TokenType::open_paren:
            return "'('";
        case TokenType::close_paren:
            return "')'";
        case TokenType::ident:
            return "identifier";
        case TokenType::let:
            return "'let'";
        case TokenType::eq:
            return "'='";
        case TokenType::plus:
            return "'+'";
        case TokenType::star:
            return "'*'";
        case TokenType::minus:
            return "'-'";
        case TokenType::fslash:
            return "'/'";
        case TokenType::open_curly:
            return "'{'";
        case TokenType::close_curly:
            return "'}'";
        case TokenType::if_:
            return "'if'";
        case TokenType::elif:
            return "'elif'";
        case TokenType::else_:
            return "'else'";
        case TokenType::test_eq:
            return "'=='";
        case TokenType::letter:
            return "'<'";
        case TokenType::better:
            return "'>'";
        case TokenType::let_eq:
            return "'<='";
        case TokenType::bet_eq:
            return "'>='";
        case TokenType::no_eq:
            return "'!='";
        case TokenType::and_:
            return "'&&'";
        case TokenType::or_:
            return "'||'";
        case TokenType::not_:
            return "'!'";
        case TokenType::commands:
            return "commands";
        case TokenType::macros_command:
            return "/macros_command";
        case TokenType::macros_var:
            return "$(macros_var)";
        case TokenType::macros_start:
            return "$/";
        case TokenType::outline:
            return "outline";
        case TokenType::while_:
            return "while";
        case TokenType::plus_plus:
            return "++";
        case TokenType::minus_minus:
            return "--";
        case TokenType::for_:
            return "for";
        case TokenType::ty_int:
            return "int";
        case TokenType::wait:
            return "wait";
        case TokenType::mod:
            return "%";
        case TokenType::struct_:
            return "struct";
        case TokenType::dot:
            return ".";
        case TokenType::separation:
            return ",";
        case TokenType::assert:
            return "assert";
        case TokenType::null:
            return "null";
        case TokenType::void_:
            return "void";
        case TokenType::return_:
            return "return";
        default:
            return "";
    }
    assert(false);
}

inline std::optional<int> bin_prec(const TokenType type) {
    switch (type) {
        case TokenType::and_:
        case TokenType::or_:
            return 0;
        case TokenType::test_eq:
        case TokenType::letter:
        case TokenType::better:
        case TokenType::let_eq:
        case TokenType::bet_eq:
        case TokenType::no_eq:
            return 1;
        case TokenType::minus:
        case TokenType::plus:
        case TokenType::mod:
            return 2;
        case TokenType::fslash:
        case TokenType::star:
            return 3;
        default:
            return {};
    }
}

inline std::optional<int> sin_prec(const TokenType type) {
    switch (type) {
        case TokenType::minus:
        case TokenType::not_:
            return 2;
        default:
            return {};
    }
}

struct Token {
    TokenType type;
    int line;
    int col;
    std::optional<std::string> value;
};

inline void print_error(const std::string &reason) {
    std::cerr << "An error : " << reason << "\n";
    exit(EXIT_FAILURE);
}

class Tokenizer {
public:
    explicit Tokenizer(std::string &src)
        : m_src(std::move(src)) {
    }

    std::vector<Token> tokenize() {
        std::vector<Token> tokens;
        std::string buf;
        int line_count = 1;
        while (peek().has_value()) {
            check(tokens, buf, line_count);
        }
        m_index = 0;
        return tokens;
    }

private:
    void check(std::vector<Token> &tokens, std::string &buf, int &line_count) {
        if (std::isalpha(peek().value()) || peek().value() == '_') {
            buf.push_back(consume());
            while (peek().has_value() && std::isalnum(peek().value()) || peek().value() == '_') {
                buf.push_back(consume());
            }
            if (buf == "exit") {
                tokens.push_back({.type = TokenType::exit, .line = line_count});
                buf.clear();
            } else if (buf == "let") {
                tokens.push_back({.type = TokenType::let, .line = line_count});
                buf.clear();
            } else if (buf == "if") {
                tokens.push_back({.type = TokenType::if_, .line = line_count});
                buf.clear();
            } else if (buf == "elif") {
                tokens.push_back({.type = TokenType::elif, .line = line_count});
                buf.clear();
            } else if (buf == "else") {
                tokens.push_back({.type = TokenType::else_, .line = line_count});
                buf.clear();
            } else if (buf == "outline") {
                tokens.push_back({.type = TokenType::outline, .line = line_count});
                buf.clear();
            } else if (buf == "while") {
                tokens.push_back({.type = TokenType::while_, .line = line_count});
                buf.clear();
            } else if (buf == "for") {
                tokens.push_back({.type = TokenType::for_, .line = line_count});
                buf.clear();
            } else if (buf == "int") {
                tokens.push_back({.type = TokenType::ty_int, .line = line_count});
                buf.clear();
            } else if (buf == "wait") {
                tokens.push_back({.type = TokenType::wait, .line = line_count});
                buf.clear();
            } else if (buf == "struct") {
                tokens.push_back({.type = TokenType::struct_, .line = line_count});
                buf.clear();
            } else if (buf == "assert") {
                tokens.push_back({.type = TokenType::assert, .line = line_count});
                buf.clear();
            } else if (buf == "null") {
                tokens.push_back({.type = TokenType::null, .line = line_count});
                buf.clear();
            } else if (buf == "void") {
                tokens.push_back({.type = TokenType::void_, .line = line_count});
                buf.clear();
            } else if (buf == "return") {
                tokens.push_back({.type = TokenType::return_, .line = line_count});
                buf.clear();
            } else {
                tokens.push_back({.type = TokenType::ident, .line = line_count, .value = buf});
                buf.clear();
            }
        } else if (std::isdigit(peek().value())) {
            buf.push_back(consume());
            while (peek().has_value() && std::isdigit(peek().value())) {
                buf.push_back(consume());
            }
            tokens.push_back({.type = TokenType::int_, .line = line_count, .value = buf});
            buf.clear();
        } else if (peek().value() == '/' && peek(1).has_value() && peek(1).value() == '/') {
            consume();
            consume();
            while (peek().has_value() && peek().value() != '\n') {
                consume();
            }
        } else if (peek().value() == '/' && peek(1).has_value() && peek(1).value() == '*') {
            consume();
            consume();
            while (peek().has_value()) {
                if (peek().value() == '*' && peek(1).has_value() && peek(1).value() == '/') {
                    break;
                }
                consume();
            }
            if (peek().has_value()) {
                consume();
            }
            if (peek().has_value()) {
                consume();
            }
            if (peek(1).has_value()) {
                consume();
            }
        } else if (peek().value() == '=' && peek(1).has_value() && peek(1).value() == '=') {
            consume();
            consume();
            tokens.push_back({.type = TokenType::test_eq, .line = line_count});
        } else if (peek().value() == '<' && peek(1).has_value() && peek(1).value() == '=') {
            consume();
            consume();
            tokens.push_back({.type = TokenType::let_eq, .line = line_count});
        } else if (peek().value() == '>' && peek(1).has_value() && peek(1).value() == '=') {
            consume();
            consume();
            tokens.push_back({.type = TokenType::bet_eq, .line = line_count});
        } else if (peek().value() == '!' && peek(1).has_value() && peek(1).value() == '=') {
            consume();
            consume();
            tokens.push_back({.type = TokenType::no_eq, .line = line_count});
        } else if (peek().value() == '&' && peek(1).has_value() && peek(1).value() == '&') {
            consume();
            consume();
            tokens.push_back({.type = TokenType::and_, .line = line_count});
        } else if (peek().value() == '|' && peek(1).has_value() && peek(1).value() == '|') {
            consume();
            consume();
            tokens.push_back({.type = TokenType::or_, .line = line_count});
        } else if (peek().value() == '-' && peek(1).has_value() && peek(1).value() == '/') {
            consume();
            consume();
            std::string com;
            while (peek().has_value() && peek().value() != ';') {
                com.push_back(consume());
            }
            tokens.push_back({.type = TokenType::commands, .line = line_count, .value = com});
        } else if (peek().value() == '$' && peek(1).has_value() && peek(1).value() == '/') {
            consume();
            consume();
            tokens.push_back({.type = TokenType::macros_start, .line = line_count});
            std::string com;
            while (peek().has_value() && peek().value() != ';') {
                if (peek().value() == '$' && peek(1).has_value() && peek(1).value() == '(') {
                    tokens.push_back({.type = TokenType::macros_command, .line = line_count, .value = com});
                    com.clear();
                    com.push_back(consume());
                    com.push_back(consume());
                    tokens.push_back({.type = TokenType::macros_var, .line = line_count});
                    int count = 1;
                    while (count > 0) {
                        if (peek().has_value() && peek().value() == '(') count++;
                        if (peek().has_value() && peek().value() == ')') count--;
                        check(tokens, buf, line_count);
                    }
                    com.clear();
                } else {
                    com.push_back(consume());
                }
            }
            if (!com.empty()) tokens.push_back({.type = TokenType::macros_command, .line = line_count, .value = com});
        } else if (peek().value() == '+' && peek(1).has_value() && peek(1).value() == '+') {
            consume();
            consume();
            tokens.push_back({.type = TokenType::plus_plus, .line = line_count});
        } else if (peek().value() == '-' && peek(1).has_value() && peek(1).value() == '-') {
            consume();
            consume();
            tokens.push_back({.type = TokenType::minus_minus, .line = line_count});
        } else if (peek().value() == '(') {
            consume();
            tokens.push_back({.type = TokenType::open_paren, .line = line_count});
            buf.clear();
        } else if (peek().value() == ')') {
            consume();
            tokens.push_back({.type = TokenType::close_paren, .line = line_count});
            buf.clear();
        } else if (peek().value() == ';') {
            consume();
            tokens.push_back({.type = TokenType::semi, .line = line_count});
        } else if (peek().value() == '=') {
            consume();
            tokens.push_back({.type = TokenType::eq, .line = line_count});
        } else if (peek().value() == '+') {
            consume();
            tokens.push_back({.type = TokenType::plus, .line = line_count});
        } else if (peek().value() == '*') {
            consume();
            tokens.push_back({.type = TokenType::star, .line = line_count});
        } else if (peek().value() == '-') {
            consume();
            tokens.push_back({.type = TokenType::minus, .line = line_count});
        } else if (peek().value() == '/') {
            consume();
            tokens.push_back({.type = TokenType::fslash, .line = line_count});
        } else if (peek().value() == '{') {
            consume();
            tokens.push_back({.type = TokenType::open_curly, .line = line_count});
        } else if (peek().value() == '}') {
            consume();
            tokens.push_back({.type = TokenType::close_curly, .line = line_count});
        } else if (peek().value() == '<') {
            consume();
            tokens.push_back({.type = TokenType::letter, .line = line_count});
        } else if (peek().value() == '>') {
            consume();
            tokens.push_back({.type = TokenType::better, .line = line_count});
        } else if (peek().value() == '!') {
            consume();
            tokens.push_back({.type = TokenType::not_, .line = line_count});
        } else if (peek().value() == '%') {
            consume();
            tokens.push_back({.type = TokenType::mod, .line = line_count});
        } else if (peek().value() == '.') {
            consume();
            tokens.push_back({.type = TokenType::dot, .line = line_count});
        } else if (peek().value() == ',') {
            consume();
            tokens.push_back({.type = TokenType::separation, .line = line_count});
        } else if (peek().value() == '\n') {
            consume();
            line_count++;
        } else if (std::isspace(peek().value())) {
            consume();
        } else {
            std::stringstream ss;
            ss << "Unknown keywords, variables, or expressions '" << peek().value() << "' at line " << std::to_string(line_count);
            print_error(ss.str());
        }
    }

    [[nodiscard]] std::optional<char> peek(const int offset = 0) const {
        if (m_index + offset >= m_src.length()) {
            return {};
        }
        return m_src.at(m_index + offset);
    }

    char consume() {
        return m_src.at(m_index++);
    }

    const std::string m_src;
    int m_index = 0;
};

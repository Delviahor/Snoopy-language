#include <iostream>
#include <vector>
#include <string>
#include <cctype>
#include <memory>
#include <utility>
#include <iomanip>
#include <unordered_map>

// Memoria simulada: nombre ? valor
std::unordered_map<std::string, double> memory;

// Token structure
struct Token {
    std::string type;
    std::string value;
};

// Lexer: convert text in tokens
std::vector<Token> lex(const std::string& source) {
    std::vector<Token> tokens;
    size_t i = 0;

    while (i < source.size()) {
        char c = source[i];

        if (std::isspace(c)) {
            ++i;
            continue;
        }
        /*
        if (std::isdigit(c)) {
            std::string number;
            while (i < source.size() && std::isdigit(source[i])) {
                number += source[i++];
            }
            tokens.push_back({ "NUMBER", number });
            continue;
        }
        */
        if (std::isdigit(c) || (c == '.' && i + 1 < source.size() && std::isdigit(source[i + 1]))) {

            
            std::string number;
            bool hasDecimal = false;

            while (i < source.size() && (std::isdigit(source[i]) || source[i] == '.')) {
                if (source[i] == '.') {
                    if (hasDecimal) break; // two points => number ends
                    hasDecimal = true;
                }
                number += source[i++];
            }

            tokens.push_back({ "NUMBER", number });
            continue;
        }
		// *** KEYWORDS ***
        if (std::isalpha(c)) {
            std::string word;
            while (i < source.size() && (std::isalnum(source[i]) || source[i] == '_')) {
                word += source[i++];
            }
            if (word == "PRINT" || word == "ADD" || word == "READ" || word == "INT" || word == "DOUBLE") {
                tokens.push_back({ "KEYWORD", word });
            }
            else {
                tokens.push_back({ "IDENTIFIER", word });
            }
            continue;
        }

        if (c == '"') {
            ++i;
            std::string text;
            while (i < source.size() && source[i] != '"') {
                text += source[i++];
            }
            ++i; // skip closing quote
            tokens.push_back({ "STRING", text });
            continue;
        }

        if (c == '+' || c == '-' || c == '=') {
            tokens.push_back({ "OPERATOR", std::string(1, c) });
            ++i;
            continue;
        }

        if (c == '(' || c == ')') {
            tokens.push_back({ "PAREN", std::string(1, c) });
            ++i;
            continue;
        }

        std::cerr << "Unrecognized character: " << c << std::endl;
        ++i;
    }

    return tokens;
}

// AST node structure
struct ASTNode {

    std::string type;
    std::string value;
    std::vector<std::shared_ptr<ASTNode>> children;

    ASTNode(const std::string& t, const std::string& v = "") : type(t), value(v) {}
};

// Parser: converts tokens to AST
std::pair<std::shared_ptr<ASTNode>, size_t> parse_statement(const std::vector<Token>& tokens, size_t i) {
    
    if (i >= tokens.size()) return { nullptr, i };
    const Token& token = tokens[i];

    if (token.type != "KEYWORD") {
        std::cerr << "Expected KEYWORD token at position " << i << std::endl;
        return { nullptr, i };
    }

    auto node = std::make_shared<ASTNode>(token.value);
    ++i;

    if (node->type == "PRINT") {
        if (i < tokens.size() && (
            tokens[i].type == "STRING" ||
            tokens[i].type == "NUMBER" ||
            tokens[i].type == "IDENTIFIER")) {

            node->children.push_back(std::make_shared<ASTNode>(tokens[i].type, tokens[i].value));
            ++i;
        }
        else {
            std::cerr << "ALERT! PRINT expects STRING, NUMBER or IDENTIFIER argument." << std::endl;
            return { nullptr, i };
        }
    }

    else if (node->type == "ADD") {
        if (i + 1 < tokens.size() && tokens[i].type == "NUMBER" && tokens[i + 1].type == "NUMBER") {
            node->children.push_back(std::make_shared<ASTNode>(tokens[i].type, tokens[i].value));
            node->children.push_back(std::make_shared<ASTNode>(tokens[i + 1].type, tokens[i + 1].value));
            i += 2;
        }
        else {
            std::cerr << "ALERT! Function ADD expects two NUMBER arguments." << std::endl;
            return { nullptr, i };
        }
    }

    else if (node->type == "INT") {
        if (i + 2 < tokens.size() &&
            tokens[i].type == "IDENTIFIER" &&
            tokens[i + 1].type == "OPERATOR" && tokens[i + 1].value == "=" &&
            tokens[i + 2].type == "NUMBER") {

            const std::string& varName = tokens[i].value;
            const std::string& numberValue = tokens[i + 2].value;

            // Validar entero (sin punto)
            if (numberValue.find('.') != std::string::npos) {
                std::cerr << "ALERT! INT must be assigned an integer." << std::endl;
                return { nullptr, i };
            }

            node->children.push_back(std::make_shared<ASTNode>("IDENTIFIER", varName));
            node->children.push_back(std::make_shared<ASTNode>("NUMBER", numberValue));
            i += 3;
        }
        else {
            std::cerr << "Expected: INT <name> = <integer>" << std::endl;
            return { nullptr, i };
        }
    }


    else {
        std::cerr << "ERROR! Unknown command: " << node->type << std::endl;
        return { nullptr, i };
    }

    return { node, i };
}

std::vector<std::shared_ptr<ASTNode>> parse(const std::vector<Token>& tokens) {
    std::vector<std::shared_ptr<ASTNode>> ast;
    size_t i = 0;

    while (i < tokens.size()) {
        //auto [node, nextPos] = parse_statement(tokens, i);
        auto result = parse_statement(tokens, i);
        auto node = result.first;
        auto nextPos = result.second;

        if (!node) break;
        ast.push_back(node);
        i = nextPos;
    }

    return ast;
}

// Interpr AST
void interpret_ast(const std::vector<std::shared_ptr<ASTNode>>& ast) {
    for (const auto& node : ast) {
        if (node->type == "PRINT") {
            if (!node->children.empty()) {
                const auto& child = node->children[0];

                if (child->type == "STRING") {
                    std::cout << child->value << std::endl;
                }
                else if (child->type == "IDENTIFIER") {
                    if (memory.count(child->value)) {
                        std::cout << memory[child->value] << std::endl;
                    }
                    else {
                        std::cerr << "ERROR: Variable '" << child->value << "' not found in memory." << std::endl;
                    }
                }
            }
        }
        else if (node->type == "ADD") {

            if (node->children.size() == 2) {
                double a = std::stod(node->children[0]->value);
                double b = std::stod(node->children[1]->value);
                std::cout << (a + b) << std::endl;
            }
        }

        else if (node->type == "INT") {
            const std::string& varName = node->children[0]->value;
            int value = std::stoi(node->children[1]->value);
            memory[varName] = value;
			std::cout << "Variable " << varName << " initialized with value: " << value << std::endl; // delete this line in production
        }

    }
}

int main() {
    std::cout << "Welcome to Snoopy-language REPL. Type 'EXIT' to quit.\n";

    std::string line;
    while (true) {
        std::cout << ">> ";
        std::getline(std::cin, line);

        if (line == "EXIT") break;
        if (line.empty()) continue;

        try {
            auto tokens = lex(line);
            auto ast = parse(tokens);
            interpret_ast(ast);
        }
        catch (const std::exception& e) {
            std::cerr << "Runtime error: " << e.what() << std::endl;
        }
    }

    return 0;
}

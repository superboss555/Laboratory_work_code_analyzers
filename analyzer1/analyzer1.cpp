#define _CRT_SECURE_NO_WARNINGS

#include <ctype.h>
#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <windows.h>

#ifdef _MSC_VER
#define STRDUP_FN _strdup
#else
#define STRDUP_FN strdup
#endif


//обработка ошибок синтаксического анализатора
//
//
//
//

#define EXPECT(type, value, msg) expect(type, value, msg)
void print_syntax_error(const char* message, int lineNumber, const char* details) {
    // Для ошибок о пропущенной точке с запятой уменьшаем номер строки на 1
    if (strstr(message, "Ожидается ';'") != NULL) {
        printf("Синтаксическая ошибка в строке %d: Ожидается ';'\n", lineNumber - 1);
    }
    else if (details) {
        printf("Синтаксическая ошибка в строке %d: %s: '%s'\n", lineNumber, message, details);
    }
    else {
        printf("Синтаксическая ошибка в строке %d: %s\n", lineNumber, message);
    }
}


bool is_valid_type(const char* type) {
    const char* valid_types[] = {
        "int", "void", "float", "double", "char", "short", "long",
        "signed", "unsigned", "struct", "union", "enum", NULL
    };

    for (int i = 0; valid_types[i] != NULL; i++) {
        if (strcmp(type, valid_types[i]) == 0) {
            return true;
        }
    }
    return false;
}







// создаем файл и записываем в него код для анализа
void create_test_file(const char* filename) {
    FILE* file;
    errno_t err = fopen_s(&file, filename, "w");
    if (err != 0 || !file) {
        perror("Ошибка создания файла");
        exit(EXIT_FAILURE);
    }

    fprintf(file, "#include <stdio.h>\n\n");
    fprintf(file, "ine sum(int a, int b) {\n");
    fprintf(file, "    return a + b;\n");
    fprintf(file, "}\n\n");
    fprintf(file, "int main() {\n");
    fprintf(file, "    flot x = 5;\n");
    fprintf(file, "    printf(\"c: %%d\\n\", c)\n");
    fprintf(file, "    int y = 10;\n");
    fprintf(file, "    doubll result = sum(x, y);\n");
    fprintf(file, "    printf(\"result: %%d\\n\", result);\n");
    fprintf(file, "    printf(\"z: %%d\\n\", z);\n");
    fprintf(file, "    printf(\"w: %%d\\n\", w);\n");
    fprintf(file, "    int t = v + 5;\n");
    fprintf(file, "    printf(\"t: %%d\\n\", t);\n");
    fprintf(file, "    return 0\n");
    fprintf(file, "}\n");

    fclose(file);
}

// открываем файл и считываем весь текст
void print_file_content(const char* filename) {
    FILE* file = fopen(filename, "r");
    if (!file) {
        perror("Ошибка открытия файла");
        return;
    }

    char buffer[1024];
    int line_number = 1;
    while (fgets(buffer, sizeof(buffer), file)) {
        printf("%3d: %s", line_number, buffer);
        line_number++;
    }
    fclose(file);
}

// Типы токенов
typedef enum {
    MY_KEYWORD,
    MY_IDENTIFIER,
    MY_OPERATOR,
    MY_INTEGER,
    MY_STRING_LITERAL,
    MY_PREPROCESSOR_DIRECTIVE,
    MY_SPECIAL_SYMBOL,
    MY_UNKNOWN
} MyTokenType;

// Структура токена
typedef struct {
    MyTokenType type;
    char* value;
    int lineNumber;
} Token;

// Массив для хранения токенов
Token tokens[1024];
int tokenCount = 0;

// Функция для добавления токена в массив
void addToken(MyTokenType type, const char* value, int lineNumber) {
    tokens[tokenCount].type = type;
    tokens[tokenCount].value = STRDUP_FN(value);
    tokens[tokenCount].lineNumber = lineNumber;
    tokenCount++;
}

// Состояния конечного автомата
typedef enum {
    STATE_START,
    STATE_IDENTIFIER,
    STATE_NUMBER,
    STATE_OPERATOR,
    STATE_STRING,
    STATE_PREPROCESSOR,
    STATE_SPECIAL,
    STATE_COMMENT,
    STATE_LINECOMMENT
} State;

const char* keywords[] = {
    // Типы данных
    "int", "void", "float", "double", "char", "short", "long",
    "signed", "unsigned", "struct", "union", "enum",

    // Модификаторы типов
    "const", "volatile", "static", "extern", "register", "auto",

    // Управляющие конструкции
    "if", "else", "switch", "case", "default", "for", "while", "do",
    "break", "continue", "goto", "return",

    // Операторы
    "sizeof",

    // Препроцессор
    "include", "define", "ifdef", "ifndef", "endif", "else", "elif",

    // Другие
    "typedef", "main", "printf", "scanf",

    NULL
};

// Проверка, является ли строка ключевым словом
bool isKeyword(const char* str) {
    for (int i = 0; keywords[i] != NULL; i++) {
        if (strcmp(str, keywords[i]) == 0) {
            return true;
        }
    }
    return false;
}

// Операторы
const char operators[] = "+-*/%=<>!&|^";

// Специальные символы
const char specials[] = ";,(){}[]";

// Лексический анализатор на основе конечного автомата
void tokenizeFileAutomata(const char* filename) {
    FILE* file = fopen(filename, "r");
    if (!file) {
        perror("Ошибка открытия файла");
        return;
    }

    char buffer[1024];
    int lineNumber = 1;
    State currentState = STATE_START;
    char tokenValue[256] = { 0 };
    int tokenPos = 0;
    bool inString = false;
    char stringQuote = '\0';

    while (fgets(buffer, sizeof(buffer), file)) {
        for (int i = 0; buffer[i] != '\0'; i++) {
            char c = buffer[i];

            // Обработка комментариев
            if (currentState == STATE_COMMENT) {
                if (c == '*' && buffer[i + 1] == '/') {
                    currentState = STATE_START;
                    i++; // Пропускаем '/'
                }
                continue;
            }

            if (currentState == STATE_LINECOMMENT) {
                if (c == '\n') {
                    currentState = STATE_START;
                }
                continue;
            }

            // Обработка строковых литералов
            if (inString) {
                tokenValue[tokenPos++] = c;
                if (c == stringQuote) {
                    tokenValue[tokenPos] = '\0';
                    addToken(MY_STRING_LITERAL, tokenValue, lineNumber);
                    tokenPos = 0;
                    inString = false;
                    currentState = STATE_START;
                }
                continue;
            }

            switch (currentState) {
            case STATE_START:
                if (isalpha(c) || c == '_') {
                    currentState = STATE_IDENTIFIER;
                    tokenValue[tokenPos++] = c;
                }
                else if (isdigit(c)) {
                    currentState = STATE_NUMBER;
                    tokenValue[tokenPos++] = c;
                }
                else if (c == '"' || c == '\'') {
                    inString = true;
                    stringQuote = c;
                    tokenValue[tokenPos++] = c;
                }
                else if (c == '#') {
                    currentState = STATE_PREPROCESSOR;
                    tokenValue[tokenPos++] = c;
                }
                else if (strchr(operators, c) != NULL) {
                    currentState = STATE_OPERATOR;
                    tokenValue[tokenPos++] = c;
                }
                else if (strchr(specials, c) != NULL) {
                    tokenValue[tokenPos++] = c;
                    tokenValue[tokenPos] = '\0';
                    addToken(MY_SPECIAL_SYMBOL, tokenValue, lineNumber);
                    tokenPos = 0;
                }
                else if (c == '/' && buffer[i + 1] == '*') {
                    currentState = STATE_COMMENT;
                    i++; // Пропускаем '*'
                }
                else if (c == '/' && buffer[i + 1] == '/') {
                    currentState = STATE_LINECOMMENT;
                    i++; // Пропускаем второй '/'
                }
                else if (!isspace(c)) {
                    tokenValue[tokenPos++] = c;
                    tokenValue[tokenPos] = '\0';
                    addToken(MY_UNKNOWN, tokenValue, lineNumber);
                    tokenPos = 0;
                }
                break;

            case STATE_IDENTIFIER:
                if (isalnum(c) || c == '_') {
                    tokenValue[tokenPos++] = c;
                }
                else {
                    tokenValue[tokenPos] = '\0';
                    if (isKeyword(tokenValue)) {
                        addToken(MY_KEYWORD, tokenValue, lineNumber);
                    }
                    else {
                        addToken(MY_IDENTIFIER, tokenValue, lineNumber);
                    }
                    tokenPos = 0;
                    currentState = STATE_START;
                    i--; // Повторно обработаем текущий символ
                }
                break;

            case STATE_NUMBER:
                if (isdigit(c)) {
                    tokenValue[tokenPos++] = c;
                }
                else {
                    tokenValue[tokenPos] = '\0';
                    addToken(MY_INTEGER, tokenValue, lineNumber);
                    tokenPos = 0;
                    currentState = STATE_START;
                    i--; // Повторно обработаем текущий символ
                }
                break;

            case STATE_OPERATOR:
                tokenValue[tokenPos] = '\0';
                addToken(MY_OPERATOR, tokenValue, lineNumber);
                tokenPos = 0;
                currentState = STATE_START;
                i--; // Повторно обработаем текущий символ
                break;

            case STATE_PREPROCESSOR:
                if (c == '\n') {
                    tokenValue[tokenPos] = '\0';
                    addToken(MY_PREPROCESSOR_DIRECTIVE, tokenValue, lineNumber);
                    tokenPos = 0;
                    currentState = STATE_START;
                }
                else {
                    tokenValue[tokenPos++] = c;
                }
                break;

            default:
                break;
            }
        }

        // Обработка конца строки
        if (currentState == STATE_PREPROCESSOR || currentState == STATE_LINECOMMENT) {
            tokenValue[tokenPos] = '\0';
            if (currentState == STATE_PREPROCESSOR) {
                addToken(MY_PREPROCESSOR_DIRECTIVE, tokenValue, lineNumber);
            }
            tokenPos = 0;
            currentState = STATE_START;
        }

        lineNumber++;
    }

    // Обработка оставшихся данных в буфере
    if (tokenPos > 0) {
        tokenValue[tokenPos] = '\0';
        switch (currentState) {
        case STATE_IDENTIFIER:
            if (isKeyword(tokenValue)) {
                addToken(MY_KEYWORD, tokenValue, lineNumber);
            }
            else {
                addToken(MY_IDENTIFIER, tokenValue, lineNumber);
            }
            break;
        case STATE_NUMBER:
            addToken(MY_INTEGER, tokenValue, lineNumber);
            break;
        case STATE_OPERATOR:
            addToken(MY_OPERATOR, tokenValue, lineNumber);
            break;
        default:
            addToken(MY_UNKNOWN, tokenValue, lineNumber);
            break;
        }
    }

    fclose(file);
}

// Оригинальный "ручной" лексический анализатор (оставлен для проверки)
void tokenizeFileManual(const char* filename) {
    FILE* file = fopen(filename, "r");
    if (!file) {
        perror("Ошибка открытия файла");
        return;
    }

    char buffer[1024];
    int lineNumber = 0;

    while (fgets(buffer, sizeof(buffer), file)) {
        lineNumber++;
        char* ptr = buffer;
        while (*ptr) {
            if (isspace(*ptr)) {
                ptr++;
                continue;
            }

            if (*ptr == '#') {
                char dirBuffer[100];
                int dirLen = 0;
                ptr++;
                while (*ptr != '\n' && *ptr != '\0') {
                    dirBuffer[dirLen++] = *ptr;
                    ptr++;
                }
                dirBuffer[dirLen] = '\0';
                addToken(MY_PREPROCESSOR_DIRECTIVE, dirBuffer, lineNumber);
                continue;
            }

            if (isdigit(*ptr)) {
                char numBuffer[20];
                int numLen = 0;
                while (isdigit(*ptr)) {
                    numBuffer[numLen++] = *ptr;
                    ptr++;
                }
                numBuffer[numLen] = '\0';
                addToken(MY_INTEGER, numBuffer, lineNumber);
                continue;
            }

            if (isalpha(*ptr)) {
                char idBuffer[50];
                int idLen = 0;
                while (isalnum(*ptr)) {
                    idBuffer[idLen++] = *ptr;
                    ptr++;
                }
                idBuffer[idLen] = '\0';

                if (strcmp(idBuffer, "int") == 0 ||
                    strcmp(idBuffer, "return") == 0 || strcmp(idBuffer, "if") == 0 ||
                    strcmp(idBuffer, "else") == 0 || strcmp(idBuffer, "while") == 0 ||
                    strcmp(idBuffer, "for") == 0 || strcmp(idBuffer, "break") == 0 ||
                    strcmp(idBuffer, "continue") == 0) {
                    addToken(MY_KEYWORD, idBuffer, lineNumber);
                }
                else {
                    addToken(MY_IDENTIFIER, idBuffer, lineNumber);
                }
                continue;
            }

            if (*ptr == '=' || *ptr == '+' || *ptr == '-' || *ptr == '*' ||
                *ptr == '/' || *ptr == '%' || *ptr == '!' || *ptr == '<' ||
                *ptr == '>' || *ptr == '&' || *ptr == '|' || *ptr == '^') {
                char opBuffer[2] = { *ptr, '\0' };
                addToken(MY_OPERATOR, opBuffer, lineNumber);
                ptr++;
                continue;
            }

            if (*ptr == ';' || *ptr == ',' || *ptr == '(' || *ptr == ')' ||
                *ptr == '{' || *ptr == '}' || *ptr == '[' || *ptr == ']') {
                char specBuffer[2] = { *ptr, '\0' };
                addToken(MY_SPECIAL_SYMBOL, specBuffer, lineNumber);
                ptr++;
                continue;
            }

            if (*ptr == '"') {
                char strBuffer[100];
                int strLen = 0;
                ptr++;
                while (*ptr != '"' && *ptr != '\0') {
                    strBuffer[strLen++] = *ptr;
                    ptr++;
                }
                strBuffer[strLen] = '\0';
                addToken(MY_STRING_LITERAL, strBuffer, lineNumber);
                ptr++;
                continue;
            }

            char unknownBuffer[2] = { *ptr, '\0' };
            addToken(MY_UNKNOWN, unknownBuffer, lineNumber);
            ptr++;
        }
    }
    fclose(file);
}

void print_token_table() {
    printf("\nТаблица токенов:\n");
    printf("%-5s | %-20s | %-20s | %-10s\n", "№", "Тип токена", "Лексема", "Строка");
    printf("---------------------------------------------------------------\n");
    for (int i = 0; i < tokenCount; i++) {
        const char* typeStr = "";
        switch (tokens[i].type) {
        case MY_KEYWORD: typeStr = "KEYWORD"; break;
        case MY_IDENTIFIER: typeStr = "IDENTIFIER"; break;
        case MY_OPERATOR: typeStr = "OPERATOR"; break;
        case MY_INTEGER: typeStr = "INTEGER"; break;
        case MY_STRING_LITERAL: typeStr = "STRING_LITERAL"; break;
        case MY_PREPROCESSOR_DIRECTIVE: typeStr = "PREPROCESSOR"; break;
        case MY_SPECIAL_SYMBOL: typeStr = "SPECIAL_SYMBOL"; break;
        case MY_UNKNOWN: typeStr = "UNKNOWN"; break;
        default: typeStr = "???"; break;
        }
        printf("%-5d | %-20s | %-20s | %-10d\n", i + 1, typeStr, tokens[i].value, tokens[i].lineNumber);
    }
}

void free_memory() {
    for (int i = 0; i < tokenCount; i++) {
        if (tokens[i].value != NULL) {
            free(tokens[i].value);
            tokens[i].value = NULL;
        }
    }
}



// Типы узлов AST (абстрактного синтаксического дерева)
typedef enum {
    NODE_PROGRAM,
    NODE_FUNCTION_DEF,
    NODE_VAR_DECL,
    NODE_ASSIGNMENT,
    NODE_FUNCTION_CALL,
    NODE_EXPRESSION,
    NODE_STATEMENT,
    NODE_PARAM_LIST,
    NODE_ARG_LIST,
    NODE_RETURN,
    NODE_TYPE,
    NODE_IDENTIFIER,
    NODE_OPERATOR,
    NODE_LITERAL
} NodeType;

// Структура узла AST
typedef struct ASTNode {
    NodeType type;
    char* value;
    struct ASTNode** children;
    int children_count;
    int lineNumber;
} ASTNode;

// Функции для работы с AST
ASTNode* create_node(NodeType type, const char* value, int lineNumber) {
    ASTNode* node = (ASTNode*)malloc(sizeof(ASTNode));
    if (node) {
        node->type = type;
        node->value = value ? STRDUP_FN(value) : NULL;
        node->children = NULL;
        node->children_count = 0;
        node->lineNumber = lineNumber;
    }
    return node;
}

void add_child(ASTNode* parent, ASTNode* child) {
    if (parent && child) {
        parent->children_count++;
        ASTNode** new_children = (ASTNode**)realloc(parent->children, parent->children_count * sizeof(ASTNode*));
        if (new_children) {
            parent->children = new_children;
            parent->children[parent->children_count - 1] = child;
        }
    }
}

void free_ast(ASTNode* node) {
    if (node) {
        if (node->value) {
            free(node->value);
        }
        for (int i = 0; i < node->children_count; i++) {
            free_ast(node->children[i]);
        }
        if (node->children) {
            free(node->children);
        }
        free(node);
    }
}

// Синтаксический анализатор на основе рекурсивного спуска
int currentTokenIndex = 0;

Token* peekToken() {
    if (currentTokenIndex < tokenCount) {
        return &tokens[currentTokenIndex];
    }
    return NULL;
}

Token* getNextToken() {
    if (currentTokenIndex < tokenCount) {
        return &tokens[currentTokenIndex++];
    }
    return NULL;
}

bool match(MyTokenType type, const char* value) {
    Token* token = peekToken();
    if (token && token->type == type && (value == NULL || strcmp(token->value, value) == 0)) {
        return true;
    }
    return false;
}

bool expect(MyTokenType type, const char* value, const char* expected_msg) {
    if (currentTokenIndex >= tokenCount) {
        print_syntax_error("Ожидается токен, но достигнут конец файла", 0, NULL);
        return false;
    }
    
    Token* token = peekToken();
    if (token && token->type == type && (value == NULL || strcmp(token->value, value) == 0)) {
        currentTokenIndex++;
        return true;
    }

    if (token) {
        if (expected_msg) {
            print_syntax_error(expected_msg, token->lineNumber, token->value);
        }
        else {
            char error_msg[256];
            if (value) {
                snprintf(error_msg, sizeof(error_msg), "Ожидается '%s'", value);
                print_syntax_error(error_msg, token->lineNumber, token->value);
            }
            else {
                print_syntax_error("Ожидается другой токен", token->lineNumber, token->value);
            }
        }
    }

    return false;
}

ASTNode* parseExpression();
ASTNode* parseFunctionCall();
ASTNode* parseArgumentList();

ASTNode* parsePrimaryExpression() {
    Token* token = peekToken();
    if (!token) return NULL;

    ASTNode* node = NULL;

    if (match(MY_IDENTIFIER, NULL)) {
        token = getNextToken();
        if (match(MY_SPECIAL_SYMBOL, "(")) {
            // Это вызов функции
            currentTokenIndex--; // Возвращаемся к идентификатору
            node = parseFunctionCall();
        }
        else {
            node = create_node(NODE_IDENTIFIER, token->value, token->lineNumber);
        }
    }
    else if (match(MY_INTEGER, NULL)) {
        token = getNextToken();
        node = create_node(NODE_LITERAL, token->value, token->lineNumber);
    }
    else if (match(MY_STRING_LITERAL, NULL)) {
        token = getNextToken();
        node = create_node(NODE_LITERAL, token->value, token->lineNumber);
    }
    else if (match(MY_SPECIAL_SYMBOL, "(")) {
        getNextToken(); // Пропускаем '('
        node = parseExpression();
        if (!EXPECT(MY_SPECIAL_SYMBOL, ")", "Ожидается ')'")) {
            return NULL;
        }
    }

    return node;
}

ASTNode* parseBinaryExpression(int minPrecedence) {
    ASTNode* left = parsePrimaryExpression();
    if (!left) return NULL;

    while (1) {
        Token* opToken = peekToken();
        if (!opToken || opToken->type != MY_OPERATOR) break;

        int precedence = 0;
        if (strchr("*/", opToken->value[0])) precedence = 3;
        else if (strchr("+-", opToken->value[0])) precedence = 2;
        else if (strchr("=<>!&|", opToken->value[0])) precedence = 1;
        else break;

        if (precedence < minPrecedence) break;

        getNextToken(); // Пропускаем оператор
        ASTNode* right = parseBinaryExpression(precedence + 1);
        if (!right) {
            free_ast(left);
            return NULL;
        }

        ASTNode* opNode = create_node(NODE_OPERATOR, opToken->value, opToken->lineNumber);
        ASTNode* exprNode = create_node(NODE_EXPRESSION, NULL, opToken->lineNumber);
        add_child(exprNode, left);
        add_child(exprNode, opNode);
        add_child(exprNode, right);

        left = exprNode;
    }

    return left;
}

ASTNode* parseExpression() {
    return parseBinaryExpression(0);
}

ASTNode* parseArgumentList() {
    ASTNode* listNode = create_node(NODE_ARG_LIST, NULL, 0);

    if (match(MY_SPECIAL_SYMBOL, ")")) {
        return listNode; // Пустой список аргументов
    }

    while (1) {
        ASTNode* arg = parseExpression();
        if (!arg) break;

        add_child(listNode, arg);

        if (!match(MY_SPECIAL_SYMBOL, ",")) break;
        getNextToken(); // Пропускаем запятую
    }

    return listNode;
}

ASTNode* parseFunctionCall() {
    Token* idToken = getNextToken();
    if (!idToken) return NULL;

    if (!EXPECT(MY_SPECIAL_SYMBOL, "(", "Ожидается '(' после идентификатора функции")) {
        // Пытаемся восстановиться
        while (peekToken() && !match(MY_SPECIAL_SYMBOL, ";") && !match(MY_SPECIAL_SYMBOL, "}")) {
            currentTokenIndex++;
        }
        return NULL;
    }

    ASTNode* callNode = create_node(NODE_FUNCTION_CALL, idToken->value, idToken->lineNumber);
    ASTNode* argsNode = parseArgumentList();

    add_child(callNode, argsNode);

    if (!EXPECT(MY_SPECIAL_SYMBOL, ")", "Ожидается ')' после аргументов функции")) {
        // Пытаемся восстановиться
        while (peekToken() && !match(MY_SPECIAL_SYMBOL, ";") && !match(MY_SPECIAL_SYMBOL, "}")) {
            currentTokenIndex++;
        }
        free_ast(callNode);
        return NULL;
    }

    return callNode;
}

ASTNode* parseVarDeclaration() {
    // Ожидаем тип переменной (должен быть ключевым словом)
    if (!match(MY_KEYWORD, NULL)) {
        Token* token = peekToken();
        if (token) {
            if (token->type == MY_IDENTIFIER) {
                print_syntax_error("Unknown type", token->lineNumber, token->value);
                // Пропускаем до конца statement
                while (peekToken() && !match(MY_SPECIAL_SYMBOL, ";")) {
                    currentTokenIndex++;
                }
                // Пропускаем точку с запятой
                if (match(MY_SPECIAL_SYMBOL, ";")) {
                    getNextToken();
                }
            }
            else {
                print_syntax_error("Unknown type", token->lineNumber, token->value);
            }
        }
        return NULL;
    }

    Token* typeToken = getNextToken();

    // Проверяем, что тип - это допустимое ключевое слово
    if (!is_valid_type(typeToken->value)) {
        print_syntax_error("Unknown type", typeToken->lineNumber, typeToken->value);
        // Пытаемся восстановиться, пропуская до конца объявления
        while (peekToken() && !match(MY_SPECIAL_SYMBOL, ";")) {
            currentTokenIndex++;
        }
        // Пропускаем точку с запятой
        if (match(MY_SPECIAL_SYMBOL, ";")) {
            getNextToken();
        }
        return NULL;
    }

    Token* idToken = peekToken();
    if (!idToken || idToken->type != MY_IDENTIFIER) {
        print_syntax_error("Ожидается идентификатор переменной", idToken ? idToken->lineNumber : typeToken->lineNumber, NULL);
        return NULL;
    }
    idToken = getNextToken();

    ASTNode* varNode = create_node(NODE_VAR_DECL, NULL, typeToken->lineNumber);
    ASTNode* typeNode = create_node(NODE_TYPE, typeToken->value, typeToken->lineNumber);
    ASTNode* idNode = create_node(NODE_IDENTIFIER, idToken->value, idToken->lineNumber);

    add_child(varNode, typeNode);
    add_child(varNode, idNode);

    // Обработка инициализации
    if (match(MY_OPERATOR, "=")) {
        getNextToken(); // Пропускаем =
        ASTNode* expr = parseExpression();
        if (expr) {
            add_child(varNode, expr);
        }
    }

    if (!EXPECT(MY_SPECIAL_SYMBOL, ";", "Ожидается ';' после объявления переменной")) {
        free_ast(varNode);
        return NULL;
    }

    return varNode;
}

ASTNode* parseAssignment() {
    Token* idToken = getNextToken();
    if (!idToken || idToken->type != MY_IDENTIFIER) return NULL;

    if (!EXPECT(MY_SPECIAL_SYMBOL, "=", "Ожидается '=' после идентификатора")) {
        return NULL;
    }

    ASTNode* assignNode = create_node(NODE_ASSIGNMENT, NULL, idToken->lineNumber);
    ASTNode* idNode = create_node(NODE_IDENTIFIER, idToken->value, idToken->lineNumber);
    ASTNode* exprNode = parseExpression();

    if (!exprNode) {
        free_ast(assignNode);
        free_ast(idNode);
        return NULL;
    }

    add_child(assignNode, idNode);
    add_child(assignNode, exprNode);

    if (!EXPECT(MY_SPECIAL_SYMBOL, ";", "Ожидается ';' после присваивания")) {
        return NULL;
    }

    return assignNode;
}

ASTNode* parseReturnStatement() {
    Token* returnToken = getNextToken();
    if (!returnToken || returnToken->type != MY_KEYWORD || strcmp(returnToken->value, "return") != 0) {
        return NULL;
    }

    ASTNode* returnNode = create_node(NODE_RETURN, NULL, returnToken->lineNumber);

    // Если следующий токен - не точка с запятой и не закрывающая скобка, парсим выражение
    if (peekToken() && !match(MY_SPECIAL_SYMBOL, ";") && !match(MY_SPECIAL_SYMBOL, "}")) {
        ASTNode* exprNode = parseExpression();
        if (exprNode) {
            add_child(returnNode, exprNode);
        }
    }

    // Всегда ожидаем точку с запятой после return
    if (!EXPECT(MY_SPECIAL_SYMBOL, ";", "Ожидается ';'")) {
        // Если нет точки с запятой, пытаемся восстановиться
        while (peekToken() && !match(MY_SPECIAL_SYMBOL, ";") &&
            !match(MY_SPECIAL_SYMBOL, "}") && !match(MY_SPECIAL_SYMBOL, "{")) {
            currentTokenIndex++;
        }
        if (match(MY_SPECIAL_SYMBOL, ";")) {
            getNextToken();
        }
    }

    return returnNode;
}


Token* peekTokenAt(int offset) {
    if (currentTokenIndex + offset < tokenCount) {
        return &tokens[currentTokenIndex + offset];
    }
    return NULL;
}

ASTNode* parseStatement() {
    ASTNode* statement = NULL;
    Token* token = peekToken();

    // Пропускаем пустые statement (точки с запятой)
    while (match(MY_SPECIAL_SYMBOL, ";")) {
        getNextToken();
        token = peekToken();
    }

    if (!token) return NULL;

    // Проверяем, является ли токен допустимым типом
    if (token->type == MY_KEYWORD && is_valid_type(token->value)) {
        statement = parseVarDeclaration();
    }
    else if (match(MY_KEYWORD, "return")) {
        statement = parseReturnStatement();
    }
    else if (match(MY_IDENTIFIER, NULL)) {
        Token* next = peekTokenAt(1); // Смотрим на следующий токен
        if (next && next->type == MY_IDENTIFIER) {
            // Два идентификатора подряд - вероятно, ошибка в типе
            print_syntax_error("Unknown type", token->lineNumber, token->value);
            // Пропускаем до конца statement
            while (peekToken() && !match(MY_SPECIAL_SYMBOL, ";")) {
                currentTokenIndex++;
            }
            // Пропускаем точку с запятой, если есть
            if (match(MY_SPECIAL_SYMBOL, ";")) {
                getNextToken();
            }
            return NULL;
        }
        else if (next && next->type == MY_OPERATOR && strcmp(next->value, "=") == 0) {
            currentTokenIndex--; // Возвращаемся к идентификатору
            statement = parseAssignment();
        }
        else if (next && next->type == MY_SPECIAL_SYMBOL && strcmp(next->value, "(") == 0) {
            // Это вызов функции
            statement = parseFunctionCall();
            if (statement && !EXPECT(MY_SPECIAL_SYMBOL, ";", "Ожидается ';' после вызова функции")) {
                return NULL;
            }
        }
        else {
            // Непонятный идентификатор
            print_syntax_error("Непонятный statement", token->lineNumber, token->value);
            currentTokenIndex++;
        }
    }
    else if (match(MY_SPECIAL_SYMBOL, ";")) {
        // Пустой statement (просто точка с запятой)
        getNextToken();
        return create_node(NODE_STATEMENT, ";", token->lineNumber);
    }
    else {
        // Непонятный токен
        print_syntax_error("Непонятный statement", token->lineNumber, token->value);
        currentTokenIndex++;
    }

    return statement;
}




ASTNode* parseStatements() {
    ASTNode* statementsNode = create_node(NODE_STATEMENT, NULL, 0);

    while (currentTokenIndex < tokenCount) {
        if (match(MY_SPECIAL_SYMBOL, "}")) {
            break; // Конец блока
        }

        int previousTokenIndex = currentTokenIndex;
        ASTNode* statement = parseStatement();
        if (statement) {
            add_child(statementsNode, statement);
        }
        else {
            // Защита от бесконечного цикла
            if (currentTokenIndex <= previousTokenIndex) {
                currentTokenIndex++;
            }
        }
    }

    return statementsNode;
}


ASTNode* parseParameterList() {
    ASTNode* paramList = create_node(NODE_PARAM_LIST, NULL, 0);

    // Если сразу закрывающая скобка, возвращаем пустой список
    if (match(MY_SPECIAL_SYMBOL, ")")) {
        return paramList;
    }

    while (1) {
        // Ожидаем тип параметра (должен быть ключевым словом)
        if (!match(MY_KEYWORD, NULL)) {
            Token* token = peekToken();
            if (token) {
                if (token->type == MY_IDENTIFIER) {
                    print_syntax_error("Unknown type", token->lineNumber, token->value);
                }
                else {
                    print_syntax_error("Unknown type", token->lineNumber, token->value);
                }
                // Пытаемся восстановиться, пропуская токены до запятой или закрывающей скобки
                while (peekToken() && !match(MY_SPECIAL_SYMBOL, ",") && !match(MY_SPECIAL_SYMBOL, ")")) {
                    currentTokenIndex++;
                }
            }
            break;
        }

        Token* typeToken = getNextToken();

        // Проверяем, что тип - это допустимое ключевое слово
        if (!is_valid_type(typeToken->value)) {
            print_syntax_error("Unknown type", typeToken->lineNumber, typeToken->value);
            // Пытаемся продолжить разбор, пропуская до запятой или закрывающей скобки
            while (peekToken() && !match(MY_SPECIAL_SYMBOL, ",") && !match(MY_SPECIAL_SYMBOL, ")")) {
                currentTokenIndex++;
            }
            if (match(MY_SPECIAL_SYMBOL, ",")) {
                getNextToken();
                continue;
            }
            break;
        }

        Token* idToken = peekToken();
        if (!idToken || idToken->type != MY_IDENTIFIER) {
            print_syntax_error("Ожидается идентификатор параметра", idToken ? idToken->lineNumber : typeToken->lineNumber, NULL);
            break;
        }
        idToken = getNextToken();

        ASTNode* param = create_node(NODE_VAR_DECL, NULL, typeToken->lineNumber);
        ASTNode* typeNode = create_node(NODE_TYPE, typeToken->value, typeToken->lineNumber);
        ASTNode* idNode = create_node(NODE_IDENTIFIER, idToken->value, idToken->lineNumber);

        add_child(param, typeNode);
        add_child(param, idNode);
        add_child(paramList, param);

        if (!match(MY_SPECIAL_SYMBOL, ",")) {
            break;
        }
        getNextToken(); // Пропускаем запятую
    }

    return paramList;
}

ASTNode* parseFunctionDefinition() {
    // Пропускаем препроцессорные директивы
    while (match(MY_PREPROCESSOR_DIRECTIVE, NULL)) {
        currentTokenIndex++;
    }

    // Ожидаем тип возвращаемого значения (должен быть ключевым словом)
    if (!match(MY_KEYWORD, NULL)) {
        Token* token = peekToken();
        if (token) {
            if (token->type == MY_IDENTIFIER) {
                print_syntax_error("Unknown type", token->lineNumber, token->value);
                // Пытаемся восстановиться, пропуская токены до конца строки или до {
                int currentLine = token->lineNumber;
                while (peekToken() && peekToken()->lineNumber == currentLine &&
                    !match(MY_SPECIAL_SYMBOL, "{")) {
                    currentTokenIndex++;
                }
                return NULL;
            }
            else {
                print_syntax_error("Unknown type", token->lineNumber, token->value);
                return NULL;
            }
        }
        return NULL;
    }

    Token* typeToken = getNextToken();

    // Проверяем, что тип - это допустимое ключевое слово
    if (!is_valid_type(typeToken->value)) {
        print_syntax_error("Unknown type", typeToken->lineNumber, typeToken->value);
        // Пытаемся продолжить разбор, пропуская до конца строки или до {
        int currentLine = typeToken->lineNumber;
        while (peekToken() && peekToken()->lineNumber == currentLine &&
            !match(MY_SPECIAL_SYMBOL, "{")) {
            currentTokenIndex++;
        }
        return NULL;
    }

    Token* idToken = peekToken();
    if (!idToken || idToken->type != MY_IDENTIFIER) {
        print_syntax_error("Ожидается идентификатор функции", idToken ? idToken->lineNumber : typeToken->lineNumber, NULL);
        return NULL;
    }
    idToken = getNextToken();

    if (!EXPECT(MY_SPECIAL_SYMBOL, "(", "Ожидается '(' после имени функции")) {
        // Если нет открывающей скобки, пытаемся восстановиться
        // Пропускаем токены до конца строки или до найденной '{'
        while (peekToken() && peekToken()->lineNumber == idToken->lineNumber &&
            !match(MY_SPECIAL_SYMBOL, "{")) {
            currentTokenIndex++;
        }
        return NULL;
    }

    ASTNode* funcNode = create_node(NODE_FUNCTION_DEF, idToken->value, idToken->lineNumber);
    ASTNode* typeNode = create_node(NODE_TYPE, typeToken->value, typeToken->lineNumber);
    ASTNode* idNode = create_node(NODE_IDENTIFIER, idToken->value, idToken->lineNumber);
    ASTNode* paramsNode = parseParameterList();

    add_child(funcNode, typeNode);
    add_child(funcNode, idNode);
    add_child(funcNode, paramsNode);

    if (!EXPECT(MY_SPECIAL_SYMBOL, ")", "Ожидается ')' после параметров функции")) {
        free_ast(funcNode);
        return NULL;
    }

    if (!EXPECT(MY_SPECIAL_SYMBOL, "{", "Ожидается '{' перед телом функции")) {
        free_ast(funcNode);
        return NULL;
    }

    ASTNode* bodyNode = parseStatements();
    add_child(funcNode, bodyNode);

    if (!EXPECT(MY_SPECIAL_SYMBOL, "}", "Ожидается '}' после тела функции")) {
        free_ast(funcNode);
        return NULL;
    }

    return funcNode;
}

ASTNode* parseProgram() {
    ASTNode* programNode = create_node(NODE_PROGRAM, NULL, 0);

    while (currentTokenIndex < tokenCount) {
        ASTNode* function = parseFunctionDefinition();
        if (function) {
            add_child(programNode, function);
        }
        else {
            // Пропускаем непонятные токены до начала следующей возможной функции
            // или до конца файла
            while (currentTokenIndex < tokenCount) {
                Token* token = peekToken();
                if (token && token->type == MY_KEYWORD &&
                    (strcmp(token->value, "int") == 0 ||
                        strcmp(token->value, "void") == 0 ||
                        strcmp(token->value, "float") == 0 ||
                        strcmp(token->value, "char") == 0)) {
                    // Нашли начало следующей функции, прерываем цикл пропуска
                    break;
                }
                currentTokenIndex++;
            }
        }
    }

    return programNode;
}



// Функция для печати AST
void print_ast(ASTNode* node, int depth) {
    if (!node) return;

    for (int i = 0; i < depth; i++) printf("  ");

    const char* type_str = "";
    switch (node->type) {
    case NODE_PROGRAM: type_str = "PROGRAM"; break;
    case NODE_FUNCTION_DEF: type_str = "FUNCTION"; break;
    case NODE_VAR_DECL: type_str = "VAR_DECL"; break;
    case NODE_ASSIGNMENT: type_str = "ASSIGNMENT"; break;
    case NODE_FUNCTION_CALL: type_str = "FUNCTION_CALL"; break;
    case NODE_EXPRESSION: type_str = "EXPRESSION"; break;
    case NODE_STATEMENT: type_str = "STATEMENT"; break;
    case NODE_PARAM_LIST: type_str = "PARAM_LIST"; break;
    case NODE_ARG_LIST: type_str = "ARG_LIST"; break;
    case NODE_RETURN: type_str = "RETURN"; break;
    case NODE_TYPE: type_str = "TYPE"; break;
    case NODE_IDENTIFIER: type_str = "IDENTIFIER"; break;
    case NODE_OPERATOR: type_str = "OPERATOR"; break;
    case NODE_LITERAL: type_str = "LITERAL"; break;
    }

    if (node->value) {
        printf("%s: %s (line %d)\n", type_str, node->value, node->lineNumber);
    }
    else {
        printf("%s (line %d)\n", type_str, node->lineNumber);
    }

    for (int i = 0; i < node->children_count; i++) {
        print_ast(node->children[i], depth + 1);
    }
}



//вывод в разных окнах
// 
// 
// 
// 
// Функция для открытия файла в отдельном окне cmd
void open_file_in_cmd(const char* title, const char* filename) {
    char command[1024];
    // Формируем команду с установкой заголовка окна и выводом заголовка в консоли
    sprintf(command,
        "start \"%s\" cmd /k \""
        "echo. && echo. && echo. && "
        "echo    ============================================ && "
        "echo    ============  %s  ============ && "
        "echo    ============================================ && "
        "echo. && echo. && "
        "type %s && "
        "echo. && echo. && echo Press any key to close the window... && "
        "pause >nul\"",
        title, title, filename);
    system(command);
}

// Функция для записи содержимого файла с нумерацией строк в другой файл
void write_file_content_with_lines(const char* filename, const char* output_filename) {
    FILE* file = fopen(filename, "r");
    FILE* output = fopen(output_filename, "w");
    if (!file || !output) {
        perror("Error opening file");
        return;
    }

    char buffer[1024];
    int line_number = 1;
    while (fgets(buffer, sizeof(buffer), file)) {
        fprintf(output, "%3d: %s", line_number, buffer);
        line_number++;
    }
    fclose(file);
    fclose(output);
}

// Функция для записи таблицы токенов в файл
void write_token_table_to_file(const char* filename) {
    FILE* file = fopen(filename, "w");
    if (!file) {
        perror("Error opening file for writing");
        return;
    }

    fprintf(file, "Token table:\n");
    fprintf(file, "%-5s | %-20s | %-20s | %-10s\n", "No", "Token type", "Lexeme", "Line");
    fprintf(file, "---------------------------------------------------------------\n");
    for (int i = 0; i < tokenCount; i++) {
        const char* typeStr = "";
        switch (tokens[i].type) {
        case MY_KEYWORD: typeStr = "KEYWORD"; break;
        case MY_IDENTIFIER: typeStr = "IDENTIFIER"; break;
        case MY_OPERATOR: typeStr = "OPERATOR"; break;
        case MY_INTEGER: typeStr = "INTEGER"; break;
        case MY_STRING_LITERAL: typeStr = "STRING_LITERAL"; break;
        case MY_PREPROCESSOR_DIRECTIVE: typeStr = "PREPROCESSOR"; break;
        case MY_SPECIAL_SYMBOL: typeStr = "SPECIAL_SYMBOL"; break;
        case MY_UNKNOWN: typeStr = "UNKNOWN"; break;
        default: typeStr = "???"; break;
        }
        fprintf(file, "%-5d | %-20s | %-20s | %-10d\n", i + 1, typeStr, tokens[i].value, tokens[i].lineNumber);
    }
    fclose(file);
}

// Функция для записи AST в файл
void write_ast_to_file(ASTNode* node, int depth, FILE* file) {
    if (!node) return;

    for (int i = 0; i < depth; i++) fprintf(file, "  ");

    const char* type_str = "";
    switch (node->type) {
    case NODE_PROGRAM: type_str = "PROGRAM"; break;
    case NODE_FUNCTION_DEF: type_str = "FUNCTION"; break;
    case NODE_VAR_DECL: type_str = "VAR_DECL"; break;
    case NODE_ASSIGNMENT: type_str = "ASSIGNMENT"; break;
    case NODE_FUNCTION_CALL: type_str = "FUNCTION_CALL"; break;
    case NODE_EXPRESSION: type_str = "EXPRESSION"; break;
    case NODE_STATEMENT: type_str = "STATEMENT"; break;
    case NODE_PARAM_LIST: type_str = "PARAM_LIST"; break;
    case NODE_ARG_LIST: type_str = "ARG_LIST"; break;
    case NODE_RETURN: type_str = "RETURN"; break;
    case NODE_TYPE: type_str = "TYPE"; break;
    case NODE_IDENTIFIER: type_str = "IDENTIFIER"; break;
    case NODE_OPERATOR: type_str = "OPERATOR"; break;
    case NODE_LITERAL: type_str = "LITERAL"; break;
    }

    if (node->value) {
        fprintf(file, "%s: %s (line %d)\n", type_str, node->value, node->lineNumber);
    }
    else {
        fprintf(file, "%s (line %d)\n", type_str, node->lineNumber);
    }

    for (int i = 0; i < node->children_count; i++) {
        write_ast_to_file(node->children[i], depth + 1, file);
    }
}

void save_ast_to_file(ASTNode* ast, const char* filename) {
    FILE* file = fopen(filename, "w");
    if (!file) {
        perror("Error opening file for AST writing");
        return;
    }
    write_ast_to_file(ast, 0, file);
    fclose(file);
}

int main() {
    setlocale(LC_ALL, "Russian");
    create_test_file("test_error.c");
    printf("File test_error.c created\n");

    write_file_content_with_lines("test_error.c", "source_code.txt");

    printf("\n\n=== AUTOMATIC LEXICAL ANALYZER ===\n");
    tokenizeFileAutomata("test_error.c");
    write_token_table_to_file("auto_lexer.txt");
    free_memory();
    tokenCount = 0;

    printf("\n\n=== MANUAL LEXICAL ANALYZER ===\n");
    tokenizeFileManual("test_error.c");
    write_token_table_to_file("manual_lexer.txt");

    printf("\n\n=== SYNTAX ANALYSIS (RECURSIVE DESCENT) ===\n");
    ASTNode* ast = parseProgram();
    save_ast_to_file(ast, "ast.txt");

    // Открываем файлы с указанием типа контента в заголовках
    printf("\nOpening results in separate windows...\n");

    Sleep(1000);
    open_file_in_cmd("SOURCE CODE", "source_code.txt");

    Sleep(1000);
    open_file_in_cmd("AUTOMATIC LEXER RESULTS", "auto_lexer.txt");

    Sleep(1000);
    open_file_in_cmd("MANUAL LEXER RESULTS", "manual_lexer.txt");

    Sleep(1000);
    open_file_in_cmd("AST RESULTS", "ast.txt");

    free_ast(ast);
    free_memory();

    return 0;
}
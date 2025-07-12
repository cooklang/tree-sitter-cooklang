#include "tree_sitter/parser.h"
#include <stdbool.h>
#include <string.h>

enum TokenType {
    NEWLINE,
    INGREDIENT_TEXT,
    COOKWARE_TEXT,
    TIMER_TEXT,
    PLAIN_TEXT
};

void *tree_sitter_cooklang_external_scanner_create() { return NULL; }

static bool is_name_char(int32_t c) {
    return (c >= 'a' && c <= 'z') ||
           (c >= 'A' && c <= 'Z') ||
           (c >= '0' && c <= '9') ||
           c == '_' || c == '-';
}

bool tree_sitter_cooklang_external_scanner_scan(void *payload, TSLexer *lexer,
                                                const bool *valid_symbols) {
    // Handle newlines
    if (lexer->lookahead == '\n' && valid_symbols[NEWLINE]) {
        lexer->advance(lexer, false);
        lexer->result_symbol = NEWLINE;
        return true;
    }

    // Handle ingredient text - this is text that follows @ up to { or whitespace or end
    if (valid_symbols[INGREDIENT_TEXT] && lexer->lookahead != '{' && lexer->lookahead != '(' &&
        lexer->lookahead != ' ' && lexer->lookahead != '\t' && lexer->lookahead != '\n' &&
        lexer->lookahead != 0) {
        // Check for recipe reference
        if (lexer->lookahead == '.') {
            lexer->advance(lexer, false);
            if (lexer->lookahead == '/' || lexer->lookahead == '\\') {
                lexer->advance(lexer, false);
                // Consume path characters
                while (lexer->lookahead != 0 && lexer->lookahead != '\n' &&
                       lexer->lookahead != '{' && lexer->lookahead != '(' &&
                       lexer->lookahead != ' ' && lexer->lookahead != '\t') {
                    lexer->advance(lexer, false);
                }
                lexer->result_symbol = INGREDIENT_TEXT;
                return true;
            }
        }

        // Regular ingredient name
        bool has_content = false;
        while (is_name_char(lexer->lookahead)) {
            has_content = true;
            lexer->advance(lexer, false);
        }

        if (has_content) {
            lexer->result_symbol = INGREDIENT_TEXT;
            return true;
        }
    }

    // Handle cookware text
    if (valid_symbols[COOKWARE_TEXT] && is_name_char(lexer->lookahead)) {
        while (is_name_char(lexer->lookahead)) {
            lexer->advance(lexer, false);
        }
        lexer->result_symbol = COOKWARE_TEXT;
        return true;
    }

    // Handle timer text
    if (valid_symbols[TIMER_TEXT] && is_name_char(lexer->lookahead)) {
        while (is_name_char(lexer->lookahead)) {
            lexer->advance(lexer, false);
        }
        lexer->result_symbol = TIMER_TEXT;
        return true;
    }

    // Handle plain text - everything that's not a special character
    if (valid_symbols[PLAIN_TEXT]) {
        bool has_content = false;

        while (lexer->lookahead != 0 && lexer->lookahead != '\n' &&
               lexer->lookahead != '@' && lexer->lookahead != '#' &&
               lexer->lookahead != '~' && lexer->lookahead != '{' &&
               lexer->lookahead != '}' && lexer->lookahead != '(' &&
               lexer->lookahead != ')') {
            has_content = true;
            lexer->advance(lexer, false);
        }

        if (has_content) {
            lexer->result_symbol = PLAIN_TEXT;
            return true;
        }
    }

    return false;
}

unsigned tree_sitter_cooklang_external_scanner_serialize(void *payload,
                                                         char *buffer) {
    return 0;
}

void tree_sitter_cooklang_external_scanner_deserialize(void *payload,
                                                       const char *buffer,
                                                       unsigned length) {}

void tree_sitter_cooklang_external_scanner_destroy(void *payload) {}

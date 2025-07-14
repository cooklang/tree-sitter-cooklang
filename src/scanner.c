#include "tree_sitter/parser.h"
#include <stdbool.h>
#include <string.h>

enum TokenType {
    NEWLINE,
    INGREDIENT_TEXT,
    COOKWARE_TEXT,
    TIMER_TEXT,
    PLAIN_TEXT,
    NOTE_TEXT,
    SECTION_HEADER
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
    // Handle section headers first - must start at column 0
    if (valid_symbols[SECTION_HEADER] && lexer->get_column(lexer) == 0 && lexer->lookahead == '=') {
        // Consume initial equals signs
        int equals_count = 0;
        while (lexer->lookahead == '=') {
            equals_count++;
            lexer->advance(lexer, false);
        }

        // Must have at least one equals sign
        if (equals_count == 0) {
            return false;
        }

        // Skip whitespace
        while (lexer->lookahead == ' ' || lexer->lookahead == '\t') {
            lexer->advance(lexer, false);
        }

        // Consume the name (optional)
        bool has_name = false;
        int last_non_space = -1;
        int pos = 0;

        while (lexer->lookahead != '\n' && lexer->lookahead != 0) {
            if (lexer->lookahead == '=') {
                // We've hit trailing equals, stop here
                break;
            }
            if (lexer->lookahead != ' ' && lexer->lookahead != '\t') {
                last_non_space = pos;
            }
            has_name = true;
            lexer->advance(lexer, false);
            pos++;
        }

        // Mark end at last non-space character if we have a name
        if (has_name && last_non_space >= 0) {
            // The lexer has advanced past where we want to end
            // We'll include everything up to the current position
        }

        // Optionally consume trailing equals signs and spaces
        while (lexer->lookahead == '=' || lexer->lookahead == ' ' || lexer->lookahead == '\t') {
            if (lexer->lookahead == '\n') break;
            lexer->advance(lexer, false);
        }

        lexer->result_symbol = SECTION_HEADER;
        return true;
    }

    // Handle newlines
    if (lexer->lookahead == '\n' && valid_symbols[NEWLINE]) {
        lexer->advance(lexer, false);
        lexer->result_symbol = NEWLINE;
        return true;
    }

    // Handle ingredient text - this is text that follows @ up to { or whitespace or end
    if (valid_symbols[INGREDIENT_TEXT]) {
        // Don't parse ingredient text at the start of a line
        if (lexer->get_column(lexer) == 0) {
            return false;
        }

        // Only start parsing ingredient text if it's a valid start character
        if (lexer->lookahead == '.' ||
            (lexer->lookahead >= 'a' && lexer->lookahead <= 'z') ||
            (lexer->lookahead >= 'A' && lexer->lookahead <= 'Z') ||
            (lexer->lookahead >= '0' && lexer->lookahead <= '9') ||
            lexer->lookahead == '_' || lexer->lookahead == '-') {
            // Continue with ingredient parsing
        } else {
            return false;
        }
        // Check for recipe reference
        if (lexer->lookahead == '.') {
            lexer->advance(lexer, false);
            if (lexer->lookahead == '/' || lexer->lookahead == '\\') {
                lexer->advance(lexer, false);
                // Consume path characters (including spaces)
                while (lexer->lookahead != 0 && lexer->lookahead != '\n' &&
                       lexer->lookahead != '{' && lexer->lookahead != '(') {
                    lexer->advance(lexer, false);
                }
                lexer->result_symbol = INGREDIENT_TEXT;
                return true;
            }
        }

        // Regular ingredient name (can be multiword)
        bool has_content = false;
        bool in_whitespace = false;

        while (lexer->lookahead != 0 && lexer->lookahead != '\n' &&
               lexer->lookahead != '{' && lexer->lookahead != '(' &&
               lexer->lookahead != '@' && lexer->lookahead != '#' &&
               lexer->lookahead != '~') {

            if (lexer->lookahead == ' ' || lexer->lookahead == '\t') {
                if (!has_content) {
                    // Skip leading whitespace
                    lexer->advance(lexer, false);
                    continue;
                }
                // Mark that we're in whitespace but keep consuming
                in_whitespace = true;
                lexer->advance(lexer, false);
            } else {
                // Non-whitespace character
                if (in_whitespace) {
                    // We had whitespace and now have content, so this is part of a multiword name
                    in_whitespace = false;
                }
                has_content = true;
                lexer->advance(lexer, false);
            }
        }

        // If we ended on whitespace, we need to back up
        if (in_whitespace && has_content) {
            // The lexer has already advanced past the whitespace
            // Just mark where we are
            lexer->mark_end(lexer);
        }

        if (has_content) {
            lexer->result_symbol = INGREDIENT_TEXT;
            return true;
        }
    }

    // Handle cookware text (can be multiword)
    if (valid_symbols[COOKWARE_TEXT]) {
        // Only start if valid character
        if (!((lexer->lookahead >= 'a' && lexer->lookahead <= 'z') ||
              (lexer->lookahead >= 'A' && lexer->lookahead <= 'Z') ||
              (lexer->lookahead >= '0' && lexer->lookahead <= '9') ||
              lexer->lookahead == '_' || lexer->lookahead == '-')) {
            return false;
        }
        bool has_content = false;
        bool in_whitespace = false;

        while (lexer->lookahead != 0 && lexer->lookahead != '\n' &&
               lexer->lookahead != '{' && lexer->lookahead != '(' &&
               lexer->lookahead != '@' && lexer->lookahead != '#' &&
               lexer->lookahead != '~') {

            if (lexer->lookahead == ' ' || lexer->lookahead == '\t') {
                if (!has_content) {
                    lexer->advance(lexer, false);
                    continue;
                }
                in_whitespace = true;
                lexer->advance(lexer, false);
            } else {
                if (in_whitespace) {
                    in_whitespace = false;
                }
                has_content = true;
                lexer->advance(lexer, false);
            }
        }

        if (in_whitespace && has_content) {
            lexer->mark_end(lexer);
        }

        if (has_content) {
            lexer->result_symbol = COOKWARE_TEXT;
            return true;
        }
    }

    // Handle timer text (can be multiword)
    if (valid_symbols[TIMER_TEXT]) {
        // Only start if valid character
        if (!((lexer->lookahead >= 'a' && lexer->lookahead <= 'z') ||
              (lexer->lookahead >= 'A' && lexer->lookahead <= 'Z') ||
              (lexer->lookahead >= '0' && lexer->lookahead <= '9') ||
              lexer->lookahead == '_' || lexer->lookahead == '-')) {
            return false;
        }
        bool has_content = false;
        bool in_whitespace = false;

        while (lexer->lookahead != 0 && lexer->lookahead != '\n' &&
               lexer->lookahead != '{' && lexer->lookahead != '(' &&
               lexer->lookahead != '@' && lexer->lookahead != '#' &&
               lexer->lookahead != '~') {

            if (lexer->lookahead == ' ' || lexer->lookahead == '\t') {
                if (!has_content) {
                    lexer->advance(lexer, false);
                    continue;
                }
                in_whitespace = true;
                lexer->advance(lexer, false);
            } else {
                if (in_whitespace) {
                    in_whitespace = false;
                }
                has_content = true;
                lexer->advance(lexer, false);
            }
        }

        if (in_whitespace && has_content) {
            lexer->mark_end(lexer);
        }

        if (has_content) {
            lexer->result_symbol = TIMER_TEXT;
            return true;
        }
    }

    // Handle plain text - everything that's not a special character
    if (valid_symbols[PLAIN_TEXT]) {
        bool has_content = false;

        // Check for special patterns at the start of line
        if (lexer->get_column(lexer) == 0) {
            // Don't consume lines starting with "---", "-", ">", ">>", "=", or "["
            if (lexer->lookahead == '-' || lexer->lookahead == '>' || 
                lexer->lookahead == '=' || lexer->lookahead == '[') {
                return false;
            }
        }

        while (lexer->lookahead != 0 && lexer->lookahead != '\n' &&
               lexer->lookahead != '@' && lexer->lookahead != '#' &&
               lexer->lookahead != '~' && lexer->lookahead != '{' &&
               lexer->lookahead != '}' && lexer->lookahead != '(' &&
               lexer->lookahead != ')' && lexer->lookahead != '[') {
            // Also stop if we hit "--" anywhere (for inline comments)
            if (lexer->lookahead == '-') {
                lexer->mark_end(lexer);
                lexer->advance(lexer, false);
                if (lexer->lookahead == '-') {
                    // This is "--", end the plain text here
                    return has_content;
                }
                has_content = true;
                continue;
            }

            has_content = true;
            lexer->advance(lexer, false);
        }

        if (has_content) {
            lexer->result_symbol = PLAIN_TEXT;
            return true;
        }
    }

    // Handle note text - text inside parentheses (with nested parentheses support)
    if (valid_symbols[NOTE_TEXT] && lexer->lookahead != ')' && lexer->lookahead != 0) {
        bool has_content = false;
        int paren_depth = 0;

        while (lexer->lookahead != 0 && lexer->lookahead != '\n') {
            if (lexer->lookahead == '(') {
                paren_depth++;
            } else if (lexer->lookahead == ')') {
                if (paren_depth == 0) {
                    // This is the closing parenthesis for the note
                    break;
                }
                paren_depth--;
            }
            has_content = true;
            lexer->advance(lexer, false);
        }

        if (has_content) {
            lexer->result_symbol = NOTE_TEXT;
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

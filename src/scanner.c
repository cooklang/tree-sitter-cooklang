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
           c == '_' ||
           c >= 128; // Allow unicode characters
}

bool tree_sitter_cooklang_external_scanner_scan(void *payload, TSLexer *lexer,
                                                const bool *valid_symbols) {
    // If we've reached EOF, return false
    if (lexer->eof(lexer)) {
        return false;
    }

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
            lexer->lookahead == '_') {
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
                int path_count = 0;
                while (path_count < 1000 &&
                       lexer->lookahead != 0 && lexer->lookahead != '\n' &&
                       lexer->lookahead != '{' && lexer->lookahead != '(') {
                    lexer->advance(lexer, false);
                    path_count++;
                }
                lexer->result_symbol = INGREDIENT_TEXT;
                return true;
            }
        }

        // Regular ingredient name
        bool has_content = false;

        // First, consume initial word characters
        int char_count = 0;
        while (is_name_char(lexer->lookahead) && char_count < 1000) {
            has_content = true;
            lexer->advance(lexer, false);
            char_count++;
        }

        if (!has_content) {
            return false;
        }

        // Now check if we should continue for multiword
        if (lexer->lookahead == '{') {
            // We're done - single word before brace
            lexer->result_symbol = INGREDIENT_TEXT;
            return true;
        }

        // Mark position after first word as potential end
        lexer->mark_end(lexer);

        // Look ahead to see if there's a brace
        bool found_brace = false;
        bool consumed_space = false;
        int safety_counter = 0;
        const int MAX_LOOKAHEAD = 1000;

        while (safety_counter < MAX_LOOKAHEAD &&
               lexer->lookahead != 0 && lexer->lookahead != '\n' &&
               lexer->lookahead != '(' && lexer->lookahead != '@' &&
               lexer->lookahead != '#' && lexer->lookahead != '~') {

            safety_counter++;

            if (lexer->lookahead == '{') {
                found_brace = true;
                break;
            }

            if (lexer->lookahead == ' ' || lexer->lookahead == '\t') {
                consumed_space = true;
                lexer->advance(lexer, false);
            } else if (consumed_space && is_name_char(lexer->lookahead)) {
                // Continue consuming for potential multiword
                while (is_name_char(lexer->lookahead) && safety_counter < MAX_LOOKAHEAD) {
                    lexer->advance(lexer, false);
                    safety_counter++;
                }
                consumed_space = false;
            } else {
                // Non-name character after space, stop here
                break;
            }
        }

        // If we found '{', this is multiword - use current position
        if (found_brace && safety_counter < MAX_LOOKAHEAD) {
            // Trim trailing whitespace if any
            while ((lexer->lookahead == ' ' || lexer->lookahead == '\t') && safety_counter < MAX_LOOKAHEAD) {
                lexer->advance(lexer, false);
                safety_counter++;
            }
            lexer->mark_end(lexer);
        }
        // Otherwise use the saved position (single word)

        lexer->result_symbol = INGREDIENT_TEXT;
        return true;
    }

    // Handle cookware text (can be multiword)
    if (valid_symbols[COOKWARE_TEXT]) {
        // Only start if valid character
        if (!((lexer->lookahead >= 'a' && lexer->lookahead <= 'z') ||
              (lexer->lookahead >= 'A' && lexer->lookahead <= 'Z') ||
              (lexer->lookahead >= '0' && lexer->lookahead <= '9') ||
              lexer->lookahead == '_')) {
            return false;
        }
        bool has_content = false;

        // First, consume initial word characters
        int char_count = 0;
        while (is_name_char(lexer->lookahead) && char_count < 1000) {
            has_content = true;
            lexer->advance(lexer, false);
            char_count++;
        }

        if (!has_content) {
            return false;
        }

        // Now check if we should continue for multiword
        if (lexer->lookahead == '{') {
            // We're done - single word before brace
            lexer->result_symbol = COOKWARE_TEXT;
            return true;
        }

        // Mark position after first word as potential end
        lexer->mark_end(lexer);

        // Look ahead to see if there's a brace
        bool found_brace = false;
        bool consumed_space = false;
        int safety_counter = 0;
        const int MAX_LOOKAHEAD = 1000;

        while (safety_counter < MAX_LOOKAHEAD &&
               lexer->lookahead != 0 && lexer->lookahead != '\n' &&
               lexer->lookahead != '(' && lexer->lookahead != '@' &&
               lexer->lookahead != '#' && lexer->lookahead != '~') {

            safety_counter++;

            if (lexer->lookahead == '{') {
                found_brace = true;
                break;
            }

            if (lexer->lookahead == ' ' || lexer->lookahead == '\t') {
                consumed_space = true;
                lexer->advance(lexer, false);
            } else if (consumed_space && is_name_char(lexer->lookahead)) {
                // Continue consuming for potential multiword
                while (is_name_char(lexer->lookahead) && safety_counter < MAX_LOOKAHEAD) {
                    lexer->advance(lexer, false);
                    safety_counter++;
                }
                consumed_space = false;
            } else {
                // Non-name character after space, stop here
                break;
            }
        }

        // If we found '{', this is multiword - use current position
        if (found_brace && safety_counter < MAX_LOOKAHEAD) {
            // Trim trailing whitespace if any
            while ((lexer->lookahead == ' ' || lexer->lookahead == '\t') && safety_counter < MAX_LOOKAHEAD) {
                lexer->advance(lexer, false);
                safety_counter++;
            }
            lexer->mark_end(lexer);
        }
        // Otherwise use the saved position (single word)

        lexer->result_symbol = COOKWARE_TEXT;
        return true;
    }

    // Handle timer text (can be multiword)
    if (valid_symbols[TIMER_TEXT]) {
        // Only start if valid character
        if (!((lexer->lookahead >= 'a' && lexer->lookahead <= 'z') ||
              (lexer->lookahead >= 'A' && lexer->lookahead <= 'Z') ||
              (lexer->lookahead >= '0' && lexer->lookahead <= '9') ||
              lexer->lookahead == '_')) {
            return false;
        }
        bool has_content = false;

        // First, consume initial word characters
        int char_count = 0;
        while (is_name_char(lexer->lookahead) && char_count < 1000) {
            has_content = true;
            lexer->advance(lexer, false);
            char_count++;
        }

        if (!has_content) {
            return false;
        }

        // Now check if we should continue for multiword
        if (lexer->lookahead == '{') {
            // We're done - single word before brace
            lexer->result_symbol = TIMER_TEXT;
            return true;
        }

        // Mark position after first word as potential end
        lexer->mark_end(lexer);

        // Look ahead to see if there's a brace
        bool found_brace = false;
        bool consumed_space = false;
        int safety_counter = 0;
        const int MAX_LOOKAHEAD = 1000;

        while (safety_counter < MAX_LOOKAHEAD &&
               lexer->lookahead != 0 && lexer->lookahead != '\n' &&
               lexer->lookahead != '(' && lexer->lookahead != '@' &&
               lexer->lookahead != '#' && lexer->lookahead != '~') {

            safety_counter++;

            if (lexer->lookahead == '{') {
                found_brace = true;
                break;
            }

            if (lexer->lookahead == ' ' || lexer->lookahead == '\t') {
                consumed_space = true;
                lexer->advance(lexer, false);
            } else if (consumed_space && is_name_char(lexer->lookahead)) {
                // Continue consuming for potential multiword
                while (is_name_char(lexer->lookahead) && safety_counter < MAX_LOOKAHEAD) {
                    lexer->advance(lexer, false);
                    safety_counter++;
                }
                consumed_space = false;
            } else {
                // Non-name character after space, stop here
                break;
            }
        }

        // If we found '{', this is multiword - use current position
        if (found_brace && safety_counter < MAX_LOOKAHEAD) {
            // Trim trailing whitespace if any
            while ((lexer->lookahead == ' ' || lexer->lookahead == '\t') && safety_counter < MAX_LOOKAHEAD) {
                lexer->advance(lexer, false);
                safety_counter++;
            }
            lexer->mark_end(lexer);
        }
        // Otherwise use the saved position (single word)

        lexer->result_symbol = TIMER_TEXT;
        return true;
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

        // Continue consuming plain text
        int chars_consumed = 0;
        while (chars_consumed < 10000 &&
               lexer->lookahead != 0 && lexer->lookahead != '\n' &&
               lexer->lookahead != '@' && lexer->lookahead != '#' &&
               lexer->lookahead != '~' && lexer->lookahead != '{' &&
               lexer->lookahead != '}' && lexer->lookahead != '(' &&
               lexer->lookahead != ')' && lexer->lookahead != '[') {

            // Check for comment start "--"
            if (lexer->lookahead == '-') {
                lexer->advance(lexer, false);
                chars_consumed++;
                has_content = true;

                if (lexer->lookahead == '-') {
                    // This is "--", we need to exclude the first '-' we just consumed
                    // Use mark_end to set the end position before the hyphen we consumed
                    lexer->result_symbol = PLAIN_TEXT;
                    // Only return true if we had content before the hyphen
                    return chars_consumed > 1;
                }
                // Single hyphen, continue normally
                continue;
            }

            has_content = true;
            chars_consumed++;
            lexer->advance(lexer, false);
        }

        if (has_content && chars_consumed > 0) {
            lexer->result_symbol = PLAIN_TEXT;
            return true;
        }
    }

    // Handle note text - text inside parentheses (with nested parentheses support)
    if (valid_symbols[NOTE_TEXT] && lexer->lookahead != ')' && lexer->lookahead != 0) {
        bool has_content = false;
        int paren_depth = 0;

        int note_count = 0;
        while (note_count < 10000 &&
               lexer->lookahead != 0 && lexer->lookahead != '\n') {
            note_count++;
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

#include "tree_sitter/parser.h"
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <wctype.h>
#include <stdlib.h>

enum TokenType {
    NEWLINE,
    INGREDIENT_NAME,
    COOKWARE_NAME,
    TIMER_NAME,
    TEXT_CONTENT,
    NOTE_CONTENT,
    METADATA_KEY,
    METADATA_VALUE,
    SECTION_NAME,
    COMMENT_LINE,
    COMMENT_BLOCK,
    RECIPE_NOTE_TEXT,
    WHITESPACE_TOKEN,
    EOF
};

typedef struct {
    uint32_t length;
    uint32_t capacity;
    char *data;
} Buffer;

typedef struct {
    Buffer buffer;
    bool in_metadata;
    bool at_line_start;
    int paren_depth;
} Scanner;

static inline void buffer_init(Buffer *buffer) {
    buffer->length = 0;
    buffer->capacity = 16;
    buffer->data = malloc(buffer->capacity);
}

static inline void buffer_free(Buffer *buffer) {
    free(buffer->data);
}

static inline void buffer_grow(Buffer *buffer, uint32_t min_capacity) {
    uint32_t new_capacity = buffer->capacity;
    while (new_capacity < min_capacity) {
        new_capacity *= 2;
    }
    buffer->data = realloc(buffer->data, new_capacity);
    buffer->capacity = new_capacity;
}

static inline void buffer_push(Buffer *buffer, char c) {
    if (buffer->length + 1 >= buffer->capacity) {
        buffer_grow(buffer, buffer->length + 2);
    }
    buffer->data[buffer->length++] = c;
}

static inline void buffer_clear(Buffer *buffer) {
    buffer->length = 0;
}

static inline bool is_word_char(int32_t c) {
    return (c >= 'a' && c <= 'z') ||
           (c >= 'A' && c <= 'Z') ||
           (c >= '0' && c <= '9') ||
           c == '_' || c == '-' ||
           c == '\'' || c == '"' ||
           c > 127; // Unicode characters
}

static inline bool is_whitespace(int32_t c) {
    return c == ' ' || c == '\t';
}

static bool scan_word(TSLexer *lexer, Buffer *buffer) {
    buffer_clear(buffer);

    if (!is_word_char(lexer->lookahead) && lexer->lookahead != '.') {
        return false;
    }

    while (is_word_char(lexer->lookahead) || lexer->lookahead == '.') {
        buffer_push(buffer, lexer->lookahead);
        lexer->advance(lexer, false);
    }

    return buffer->length > 0;
}

static bool scan_multiword(TSLexer *lexer, Buffer *buffer) {
    buffer_clear(buffer);

    // First word
    if (!is_word_char(lexer->lookahead)) {
        return false;
    }

    while (is_word_char(lexer->lookahead)) {
        buffer_push(buffer, lexer->lookahead);
        lexer->advance(lexer, false);
    }

    // Check if we have a quantity/unit pattern immediately following
    if (lexer->lookahead == '{') {
        return true;
    }

    // Save position after first word
    lexer->mark_end(lexer);

    // Look ahead for more words
    while (is_whitespace(lexer->lookahead)) {
        buffer_push(buffer, lexer->lookahead);
        lexer->advance(lexer, false);

        // After whitespace, check for another word
        if (is_word_char(lexer->lookahead)) {
            // Continue collecting the word
            while (is_word_char(lexer->lookahead)) {
                buffer_push(buffer, lexer->lookahead);
                lexer->advance(lexer, false);
            }

            // Update end position if we find a quantity
            if (lexer->lookahead == '{') {
                lexer->mark_end(lexer);
            }
        } else {
            break;
        }
    }

    return buffer->length > 0;
}

static bool scan_text_until(TSLexer *lexer, Buffer *buffer, const char *delimiters) {
    buffer_clear(buffer);
    bool has_content = false;

    while (!lexer->eof(lexer) && lexer->lookahead != '\n') {
        // Check for block comment start [-
        if (lexer->lookahead == '[') {
            lexer->advance(lexer, false);
            if (lexer->lookahead == '-') {
                // It's a block comment, backtrack
                return has_content;
            }
            // Not a block comment, include the [
            buffer_push(buffer, '[');
            has_content = true;
            continue;
        }

        // Check if we hit any delimiter
        bool is_delimiter = false;
        for (const char *d = delimiters; *d; d++) {
            if (lexer->lookahead == *d) {
                is_delimiter = true;
                break;
            }
        }

        if (is_delimiter) {
            break;
        }

        // Check for comment start
        if (lexer->lookahead == '-') {
            lexer->advance(lexer, false);
            if (lexer->lookahead == '-') {
                // Backtrack by not including the dash
                return has_content;
            }
            buffer_push(buffer, '-');
            has_content = true;
            continue;
        }

        buffer_push(buffer, lexer->lookahead);
        has_content = true;
        lexer->advance(lexer, false);
    }

    return has_content;
}

void *tree_sitter_cooklang_external_scanner_create() {
    Scanner *scanner = malloc(sizeof(Scanner));
    buffer_init(&scanner->buffer);
    scanner->in_metadata = false;
    scanner->at_line_start = true;
    scanner->paren_depth = 0;
    return scanner;
}

void tree_sitter_cooklang_external_scanner_destroy(void *payload) {
    Scanner *scanner = (Scanner *)payload;
    buffer_free(&scanner->buffer);
    free(scanner);
}

unsigned tree_sitter_cooklang_external_scanner_serialize(void *payload, char *buffer) {
    Scanner *scanner = (Scanner *)payload;
    buffer[0] = scanner->in_metadata;
    buffer[1] = scanner->at_line_start;
    buffer[2] = scanner->paren_depth;
    return 3;
}

void tree_sitter_cooklang_external_scanner_deserialize(void *payload, const char *buffer, unsigned length) {
    Scanner *scanner = (Scanner *)payload;
    if (length >= 3) {
        scanner->in_metadata = buffer[0];
        scanner->at_line_start = buffer[1];
        scanner->paren_depth = buffer[2];
    }
}

bool tree_sitter_cooklang_external_scanner_scan(void *payload, TSLexer *lexer, const bool *valid_symbols) {
    Scanner *scanner = (Scanner *)payload;

    // Handle block comments FIRST - they have highest priority and can appear anywhere
    if (lexer->lookahead == '[' && valid_symbols[COMMENT_BLOCK]) {
        lexer->advance(lexer, false);
        if (lexer->lookahead == '-') {
            lexer->advance(lexer, false);
            
            // Block comment - scan until -]
            buffer_clear(&scanner->buffer);
            while (!lexer->eof(lexer)) {
                if (lexer->lookahead == '-') {
                    lexer->advance(lexer, false);
                    if (lexer->lookahead == ']') {
                        lexer->advance(lexer, false);
                        break;
                    }
                    buffer_push(&scanner->buffer, '-');
                } else {
                    buffer_push(&scanner->buffer, lexer->lookahead);
                    lexer->advance(lexer, false);
                }
            }
            
            // Don't update at_line_start for block comments
            lexer->result_symbol = COMMENT_BLOCK;
            return true;
        }
    }

    // Handle whitespace as a token (for extras)
    if (is_whitespace(lexer->lookahead) && valid_symbols[WHITESPACE_TOKEN]) {
        lexer->result_symbol = WHITESPACE_TOKEN;
        lexer->advance(lexer, false);
        while (is_whitespace(lexer->lookahead)) {
            lexer->advance(lexer, false);
        }
        return true;
    }
    
    // Skip whitespace if not handling it as a token
    while (is_whitespace(lexer->lookahead)) {
        lexer->advance(lexer, true);
    }

    // Track line starts
    if (lexer->get_column(lexer) == 0) {
        scanner->at_line_start = true;
    }

    // Handle EOF
    if (lexer->eof(lexer)) {
        if (valid_symbols[EOF]) {
            lexer->result_symbol = EOF;
            return true;
        }
        return false;
    }

    // Handle newlines
    if (lexer->lookahead == '\n') {
        if (valid_symbols[NEWLINE]) {
            lexer->advance(lexer, false);
            scanner->at_line_start = true;
            lexer->result_symbol = NEWLINE;
            return true;
        }
        return false;
    }

    // Handle recipe notes (at start of line with single >)
    if (scanner->at_line_start && lexer->lookahead == '>' && valid_symbols[RECIPE_NOTE_TEXT]) {
        lexer->advance(lexer, false);

        // If it's not >>, it's a recipe note
        if (lexer->lookahead != '>') {
            // Skip optional whitespace
            while (is_whitespace(lexer->lookahead)) {
                lexer->advance(lexer, false);
            }

            // Scan until end of line
            buffer_clear(&scanner->buffer);
            while (!lexer->eof(lexer) && lexer->lookahead != '\n') {
                buffer_push(&scanner->buffer, lexer->lookahead);
                lexer->advance(lexer, false);
            }

            scanner->at_line_start = false;
            lexer->result_symbol = RECIPE_NOTE_TEXT;
            return true;
        } else {
            // Put back the > we consumed
            return false;
        }
    }

    // Handle metadata (at start of line with >>)
    if (scanner->at_line_start && lexer->lookahead == '>' && valid_symbols[METADATA_KEY]) {
        lexer->advance(lexer, false);
        if (lexer->lookahead == '>') {
            lexer->advance(lexer, false);

            // Skip whitespace
            while (is_whitespace(lexer->lookahead)) {
                lexer->advance(lexer, false);
            }

            // Scan metadata key (can be multi-word)
            buffer_clear(&scanner->buffer);
            bool has_key = false;
            while (!lexer->eof(lexer) && lexer->lookahead != ':' && lexer->lookahead != '\n') {
                buffer_push(&scanner->buffer, lexer->lookahead);
                has_key = true;
                lexer->advance(lexer, false);
            }

            // Trim trailing whitespace
            while (scanner->buffer.length > 0 &&
                   is_whitespace(scanner->buffer.data[scanner->buffer.length - 1])) {
                scanner->buffer.length--;
            }

            if (has_key && scanner->buffer.length > 0) {
                scanner->in_metadata = true;
                scanner->at_line_start = false;
                lexer->result_symbol = METADATA_KEY;
                return true;
            }
        }
    }

    // Handle metadata value (after colon in metadata line)
    if (scanner->in_metadata && valid_symbols[METADATA_VALUE]) {
        // Skip the colon if present
        if (lexer->lookahead == ':') {
            lexer->advance(lexer, false);
        }

        // Skip whitespace
        while (is_whitespace(lexer->lookahead)) {
            lexer->advance(lexer, false);
        }

        // Scan until end of line
        buffer_clear(&scanner->buffer);
        while (!lexer->eof(lexer) && lexer->lookahead != '\n') {
            buffer_push(&scanner->buffer, lexer->lookahead);
            lexer->advance(lexer, false);
        }

        if (scanner->buffer.length > 0) {
            scanner->in_metadata = false;
            lexer->result_symbol = METADATA_VALUE;
            return true;
        }
    }

    // Handle section headers (at start of line with =)
    if (scanner->at_line_start && lexer->lookahead == '=' && valid_symbols[SECTION_NAME]) {
        int equals_count = 0;
        while (lexer->lookahead == '=') {
            equals_count++;
            lexer->advance(lexer, false);
        }

        if (equals_count > 0) {
            // Skip whitespace
            while (is_whitespace(lexer->lookahead)) {
                lexer->advance(lexer, false);
            }

            // Scan section name
            buffer_clear(&scanner->buffer);
            while (!lexer->eof(lexer) && lexer->lookahead != '\n' && lexer->lookahead != '=') {
                buffer_push(&scanner->buffer, lexer->lookahead);
                lexer->advance(lexer, false);
            }

            // Trim trailing whitespace from buffer
            while (scanner->buffer.length > 0 &&
                   is_whitespace(scanner->buffer.data[scanner->buffer.length - 1])) {
                scanner->buffer.length--;
            }

            // Skip trailing equals
            while (lexer->lookahead == '=' || is_whitespace(lexer->lookahead)) {
                if (lexer->lookahead == '\n') break;
                lexer->advance(lexer, false);
            }

            scanner->at_line_start = false;
            lexer->result_symbol = SECTION_NAME;
            return true;
        }
    }

    // Handle comments (both at line start and inline)
    if (lexer->lookahead == '-' && valid_symbols[COMMENT_LINE]) {
        lexer->advance(lexer, false);
        if (lexer->lookahead == '-') {
            lexer->advance(lexer, false);
            
            // Skip optional space after --
            while (is_whitespace(lexer->lookahead)) {
                lexer->advance(lexer, false);
            }

            // Line comment
            buffer_clear(&scanner->buffer);
            while (!lexer->eof(lexer) && lexer->lookahead != '\n') {
                buffer_push(&scanner->buffer, lexer->lookahead);
                lexer->advance(lexer, false);
            }

            scanner->at_line_start = false;
            lexer->result_symbol = COMMENT_LINE;
            return true;
        }
    }


    // Handle ingredient names (after @)
    if (valid_symbols[INGREDIENT_NAME]) {
        // Check for recipe reference (starts with . and / or \)
        if (lexer->lookahead == '.') {
            buffer_clear(&scanner->buffer);
            buffer_push(&scanner->buffer, lexer->lookahead);
            lexer->advance(lexer, false);

            if (lexer->lookahead == '/' || lexer->lookahead == '\\') {
                buffer_push(&scanner->buffer, lexer->lookahead);
                lexer->advance(lexer, false);

                // Consume path characters
                while (!lexer->eof(lexer) && lexer->lookahead != '{' &&
                       lexer->lookahead != '(' && lexer->lookahead != '\n' &&
                       lexer->lookahead != '@' && lexer->lookahead != '#' &&
                       lexer->lookahead != '~') {
                    buffer_push(&scanner->buffer, lexer->lookahead);
                    lexer->advance(lexer, false);
                }

                scanner->at_line_start = false;
                lexer->result_symbol = INGREDIENT_NAME;
                return true;
            }
        }

        // Regular ingredient name
        if (scan_multiword(lexer, &scanner->buffer)) {
            scanner->at_line_start = false;
            lexer->result_symbol = INGREDIENT_NAME;
            return true;
        }
    }

    // Handle cookware names (after #)
    if (valid_symbols[COOKWARE_NAME]) {
        if (scan_multiword(lexer, &scanner->buffer)) {
            scanner->at_line_start = false;
            lexer->result_symbol = COOKWARE_NAME;
            return true;
        }
    }

    // Handle timer names (after ~)
    if (valid_symbols[TIMER_NAME]) {
        if (scan_multiword(lexer, &scanner->buffer)) {
            scanner->at_line_start = false;
            lexer->result_symbol = TIMER_NAME;
            return true;
        }
    }

    // Handle note content (inside parentheses)
    if (valid_symbols[NOTE_CONTENT]) {
        buffer_clear(&scanner->buffer);
        int paren_depth = scanner->paren_depth;

        while (!lexer->eof(lexer)) {
            if (lexer->lookahead == '(') {
                paren_depth++;
                buffer_push(&scanner->buffer, lexer->lookahead);
                lexer->advance(lexer, false);
            } else if (lexer->lookahead == ')') {
                if (paren_depth == 0) {
                    // End of note
                    break;
                }
                paren_depth--;
                buffer_push(&scanner->buffer, lexer->lookahead);
                lexer->advance(lexer, false);
            } else if (lexer->lookahead == '\n') {
                // Notes can't span lines in standard Cooklang
                break;
            } else {
                buffer_push(&scanner->buffer, lexer->lookahead);
                lexer->advance(lexer, false);
            }
        }

        if (scanner->buffer.length > 0) {
            scanner->at_line_start = false;
            lexer->result_symbol = NOTE_CONTENT;
            return true;
        }
    }

    // Handle plain text content
    if (valid_symbols[TEXT_CONTENT]) {
        // Don't start text with special line starters
        if (scanner->at_line_start) {
            if (lexer->lookahead == '-' || lexer->lookahead == '=' || lexer->lookahead == '[') {
                return false;
            }
            // For '>', only stop if it's a single > (not >>)
            if (lexer->lookahead == '>') {
                lexer->advance(lexer, false);
                if (lexer->lookahead != '>') {
                    // Single >, not text
                    return false;
                }
                // It's >>, backtrack and continue as text
                // But we can't backtrack, so just include the > in text
                buffer_push(&scanner->buffer, '>');
            }
        }
        
        if (scan_text_until(lexer, &scanner->buffer, "@#~{}()")) {
            scanner->at_line_start = false;
            lexer->result_symbol = TEXT_CONTENT;
            return true;
        }
    }

    return false;
}

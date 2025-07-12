//! This crate provides Cooklang language support for the [tree-sitter][] parsing library.
//!
//! Typically, you will use the [language][language func] function to add this language to a
//! tree-sitter [Parser][], and then use the parser to parse some code:
//!
//! ```
//! let code = "";
//! let mut parser = tree_sitter::Parser::new();
//! parser.set_language(&tree_sitter_cooklang::language()).expect("Error loading Cooklang grammar");
//! let tree = parser.parse(code, None).unwrap();
//! ```
//!
//! [Language]: https://docs.rs/tree-sitter/*/tree_sitter/struct.Language.html
//! [language func]: fn.language.html
//! [Parser]: https://docs.rs/tree-sitter/*/tree_sitter/struct.Parser.html
//! [tree-sitter]: https://tree-sitter.github.io/

use tree_sitter::Language;

extern "C" {
    fn tree_sitter_cooklang() -> Language;
}

/// Get the tree-sitter [Language][] for this grammar.
///
/// [Language]: https://docs.rs/tree-sitter/*/tree_sitter/struct.Language.html
pub fn language() -> Language {
    unsafe { tree_sitter_cooklang() }
}

/// The content of the [`node-types.json`][] file for this grammar.
///
/// [`node-types.json`]: https://tree-sitter.github.io/tree-sitter/using-parsers#static-node-types
pub const NODE_TYPES: &str = include_str!("../../src/node-types.json");

/// The syntax highlighting query for this grammar.
pub const HIGHLIGHTS_QUERY: &str = include_str!("../../queries/highlights.scm");

/// The injection query for this grammar (e.g., for YAML in frontmatter).
pub const INJECTIONS_QUERY: &str = include_str!("../../queries/injections.scm");

/// The local variables query for this grammar.
pub const LOCALS_QUERY: &str = include_str!("../../queries/locals.scm");

/// The tags query for this grammar (for symbol navigation).
pub const TAGS_QUERY: &str = include_str!("../../queries/tags.scm");

/// The code folding query for this grammar.
pub const FOLDS_QUERY: &str = include_str!("../../queries/folds.scm");

/// The indentation query for this grammar.
pub const INDENTS_QUERY: &str = include_str!("../../queries/indents.scm");

#[cfg(test)]
mod tests {
    #[test]
    fn test_can_load_grammar() {
        let mut parser = tree_sitter::Parser::new();
        parser
            .set_language(&super::language())
            .expect("Error loading Cooklang grammar");
    }

    #[test]
    fn test_query_constants_are_accessible() {
        // Verify that all query constants are non-empty
        assert!(!super::HIGHLIGHTS_QUERY.is_empty());
        assert!(!super::INJECTIONS_QUERY.is_empty());
        assert!(!super::LOCALS_QUERY.is_empty());
        assert!(!super::TAGS_QUERY.is_empty());
        assert!(!super::FOLDS_QUERY.is_empty());
        assert!(!super::INDENTS_QUERY.is_empty());
        
        // Verify that queries can be parsed
        let language = super::language();
        let highlights_query = tree_sitter::Query::new(&language, super::HIGHLIGHTS_QUERY)
            .expect("Failed to parse highlights query");
        assert!(highlights_query.capture_names().len() > 0);
    }
}

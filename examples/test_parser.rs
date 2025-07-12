use tree_sitter::{Language, Parser};
use tree_sitter_cooklang;

fn main() {
    let language = tree_sitter_cooklang::language();
    let mut parser = Parser::new();
    parser
        .set_language(&language)
        .expect("Error loading Cooklang grammar");

    let source_code = r#"---
title: Simple Recipe
tags:
  - easy
  - quick
---

Mix @flour{2%cups} with @water{1%cup}.
Bake for ~{30%minutes} at @temperature{180%degC}."#;

    let tree = parser.parse(source_code, None).unwrap();
    let root_node = tree.root_node();

    println!("Parse successful!");
    println!("Tree-sitter version: {}", tree_sitter::LANGUAGE_VERSION);
    println!("Root node kind: {}", root_node.kind());
    println!("Root node child count: {}", root_node.child_count());
    println!("Source code length: {} bytes", source_code.len());
    println!("Tree root range: {:?}", root_node.range());

    // Print first few children
    for i in 0..root_node.child_count().min(5) {
        if let Some(child) = root_node.child(i) {
            println!("  Child {}: {} [{:?}]", i, child.kind(), child.range());
        }
    }
}

use tree_sitter::Parser;
use tree_sitter_cooklang::LANGUAGE;

fn main() {
    let code = r#"
Creamy Mushroom Risotto

In a large #pan{}, sauté @shallots{2} in @olive oil{2%tbsp} until soft.
Add @arborio rice{1.5%cups} and stir for ~{2%minutes}.
Add @white wine{1/2%cup} and stir until absorbed.
Gradually add @vegetable stock{4%cups}, stirring frequently for ~{20%minutes}.
Stir in @parmesan{1/2%cup} and @butter{2%tbsp}.
Season with @salt and @pepper to taste.
    "#;

    let mut parser = Parser::new();
    
    // Using the LANGUAGE constant
    let language = LANGUAGE.into();
    parser.set_language(&language).expect("Error loading Cooklang grammar");
    
    let tree = parser.parse(code, None).unwrap();
    println!("Root node: {:?}", tree.root_node().kind());
    println!("Child count: {}", tree.root_node().child_count());
}
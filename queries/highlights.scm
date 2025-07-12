; Cooklang syntax highlighting queries

; Comments
(comment) @comment
(block_comment) @comment
(note) @comment

; Sections
(section_header) @keyword

; Metadata
(metadata) @keyword

; Frontmatter
(frontmatter) @keyword
(yaml_content) @string

; Ingredients
"@" @punctuation.special
(ingredient name: (ingredient_text) @variable)

; Cookware
"#" @punctuation.special
(cookware name: (cookware_text) @function)

; Timer
"~" @punctuation.special
(timer name: (timer_text) @constant)

; Amounts
(quantity) @number
(units) @type

; Operators
"%" @operator

; Brackets
"{" @punctuation.bracket
"}" @punctuation.bracket
"(" @punctuation.bracket
")" @punctuation.bracket

; Delimiters
"---" @punctuation.delimiter

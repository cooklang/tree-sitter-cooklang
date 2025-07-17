; Cooklang syntax highlighting queries

; Comments
(comment) @comment
(block_comment) @comment
(note) @comment

; Sections
(section_name) @keyword

; Metadata
(metadata) @keyword

; Frontmatter
(frontmatter) @keyword
(frontmatter_content) @string

; Ingredients
"@" @punctuation.special
(ingredient name: (ingredient_name) @variable)

; Cookware
"#" @punctuation.special
(cookware name: (cookware_name) @function)

; Timer
"~" @punctuation.special
(timer name: (timer_name) @constant)

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

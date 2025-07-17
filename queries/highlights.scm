; Cooklang syntax highlighting queries

; Comments
(comment) @comment
(block_comment) @comment
(note) @comment

; Sections
(section_name) @text.title

; Metadata
(metadata) @keyword

; Frontmatter
(frontmatter) @keyword
(frontmatter_content) @string

; Ingredients
"@" @punctuation.special
(ingredient name: (ingredient_name) @constant)

; Cookware
"#" @punctuation.special
(cookware name: (cookware_name) @constant)

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

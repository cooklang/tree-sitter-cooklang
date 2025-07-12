; Cooklang syntax highlighting queries

; Comments
(comment) @comment
(block_comment) @comment
(note) @comment.documentation

; Sections
(section_header) @markup.heading
(section_header name: (_) @markup.heading.1)

; Metadata
(metadata) @keyword
(metadata (_) @string)

; Frontmatter
(frontmatter) @markup.raw
(yaml_content) @string.special

; Ingredients
(ingredient "@" @punctuation.special)
(ingredient (name) @variable)
(ingredient (recipe_reference) @string.special.path)
(ingredient (amount) @number)
(ingredient (amount (units) @type))
(ingredient note: (_) @comment.documentation)

; Cookware
(cookware "#" @punctuation.special)
(cookware (name) @variable.parameter)
(cookware (amount) @number)
(cookware (amount (units) @type))
(cookware note: (_) @comment.documentation)

; Timer
(timer "~" @punctuation.special)
(timer (name) @variable.builtin)
(timer (amount) @number)
(timer (amount (units) @type))
(timer note: (_) @comment.documentation)

; Special punctuation
"{" @punctuation.bracket
"}" @punctuation.bracket
"(" @punctuation.bracket
")" @punctuation.bracket
"%" @operator

; Numbers
(quantity) @number
(_number) @number
(_integer) @number
(_decimal) @number.float
(_fractional) @number

; Step text (default text color, no special highlighting)
(step) @none
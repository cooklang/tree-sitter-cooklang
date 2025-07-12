; Local variable queries for Cooklang

; Define ingredients
(ingredient (name) @definition.ingredient)
(ingredient (recipe_reference) @definition.recipe)

; Define cookware
(cookware (name) @definition.cookware)

; Define timers
(timer (name) @definition.timer)

; Scopes
(recipe) @scope
(section) @scope
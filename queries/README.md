# Tree-sitter Cooklang Queries

This directory contains query files that enable various editor features for Cooklang files.

## Query Files

### `highlights.scm`
Provides syntax highlighting for:
- Comments (single line, block, and notes)
- Section headers
- Metadata
- Ingredients, cookware, and timers
- Recipe references (with special path highlighting)
- Numbers and units
- Punctuation

### `injections.scm`
- Injects YAML syntax highlighting into frontmatter sections

### `folds.scm`
Enables code folding for:
- Sections (fold entire section with its content)
- Frontmatter
- Block comments

### `indents.scm`
- Provides automatic indentation for section content

### `locals.scm`
- Defines scopes and definitions for ingredients, cookware, and timers
- Helps with symbol resolution and refactoring

### `tags.scm`
- Enables symbol navigation for sections and metadata
- Allows "Go to Symbol" functionality in editors

## Usage

These queries are automatically used by editors that support tree-sitter, including:
- Neovim (with nvim-treesitter)
- Helix
- Zed
- Emacs (with tree-sitter modes)

## Customization

You can customize the highlighting by modifying the capture names in `highlights.scm`. 
Common capture names include:
- `@comment`
- `@string`
- `@number`
- `@variable`
- `@keyword`
- `@punctuation`
- `@type`

Refer to your editor's documentation for the full list of supported capture names.

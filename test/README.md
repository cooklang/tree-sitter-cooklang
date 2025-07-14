# Tree-sitter Test Corpus Issues

## Problem

The tree-sitter test corpus format uses `===` as test delimiters, which conflicts with Cooklang's section syntax that uses `=` characters. This causes the parser to misinterpret test content as sections.

### Example of the conflict:

```
==================
Test Name Here
==================

Test content here

---

(expected output)
```

Cooklang interprets lines starting with `=` as section headers, so the test delimiter is parsed as a section instead of being ignored.

## Current Status

- Direct parsing of `.cook` files works correctly
- Test corpus files fail when they contain content that doesn't start with special characters (@, #, ~)
- The suffix approach documented in tree-sitter docs (e.g., `===|||`) doesn't seem to work with our setup

## Workaround

Use the `test/verify_parsing.sh` script to test actual `.cook` files instead of relying on the corpus format.

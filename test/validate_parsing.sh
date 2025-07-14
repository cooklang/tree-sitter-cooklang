#!/bin/bash
# Simple validation that files parse without errors

PARSER="./node_modules/.bin/tree-sitter parse"
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[0;33m'
NC='\033[0m'

PASS=0
FAIL=0

echo "Validating tree-sitter-cooklang parsing"
echo "======================================"
echo

for dir in test/individual_tests/*/; do
    if [ -d "$dir" ]; then
        dir_name=$(basename "$dir")
        echo
        echo "Testing $dir_name:"
        echo "----------------------------------------"
        
        for cook_file in "$dir"*.cook; do
            if [ -f "$cook_file" ]; then
                test_name=$(basename "$cook_file" .cook)
                
                # Try to parse
                if $PARSER "$cook_file" >/dev/null 2>&1; then
                    echo -e "  ${GREEN}✓${NC} $test_name"
                    ((PASS++))
                else
                    echo -e "  ${RED}✗${NC} $test_name"
                    ((FAIL++))
                fi
            fi
        done
    fi
done

echo
echo "======================================"
echo "Summary:"
echo "  Parsed successfully: $PASS"
echo "  Parse errors: $FAIL"
echo "  Total: $((PASS + FAIL))"
echo
if [ $FAIL -eq 0 ]; then
    echo -e "${GREEN}All files parsed successfully!${NC}"
else
    echo -e "Success rate: $((PASS * 100 / (PASS + FAIL)))%"
fi
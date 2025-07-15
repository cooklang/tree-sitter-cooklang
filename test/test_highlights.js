const Parser = require('tree-sitter');
const Cooklang = require('../bindings/node');
const fs = require('fs');
const path = require('path');

// Load highlight queries
const highlightQueries = fs.readFileSync(path.join(__dirname, '../queries/highlights.scm'), 'utf8');

// Test cases with expected highlights
const testCases = [
  {
    name: "Ingredient with amount",
    input: "Add @flour{2%cups} to bowl.",
    expected: [
      { text: "@", highlight: "punctuation.special" },
      { text: "flour", highlight: "variable" },
      { text: "2", highlight: "number" },
      { text: "%", highlight: "operator" },
      { text: "cups", highlight: "type" },
      { text: "{", highlight: "punctuation.bracket" },
      { text: "}", highlight: "punctuation.bracket" }
    ]
  },
  {
    name: "Ingredient with references",
    input: "Add @./Components/Sauce{2%cups} to bowl.",
    expected: [
      { text: "@", highlight: "punctuation.special" },
      { text: "./Components/Sauce", highlight: "variable" },
      { text: "2", highlight: "number" },
      { text: "%", highlight: "operator" },
      { text: "cups", highlight: "type" },
      { text: "{", highlight: "punctuation.bracket" },
      { text: "}", highlight: "punctuation.bracket" }
    ]
  },
  {
    name: "Cookware",
    input: "Mix in #bowl.",
    expected: [
      { text: "#", highlight: "punctuation.special" },
      { text: "bowl", highlight: "function" }
    ]
  },
  {
    name: "Timer",
    input: "Cook for ~{5%minutes}.",
    expected: [
      { text: "~", highlight: "punctuation.special" },
      { text: "5", highlight: "number" },
      { text: "%", highlight: "operator" },
      { text: "minutes", highlight: "type" },
      { text: "{", highlight: "punctuation.bracket" },
      { text: "}", highlight: "punctuation.bracket" }
    ]
  },
  {
    name: "Punctuation handling - cookware with period",
    input: "Heat #pan on medium.",
    expected: [
      { text: "#", highlight: "punctuation.special" },
      { text: "pan", highlight: "function" }
    ]
  },
  {
    name: "Punctuation handling - ingredient with comma", 
    input: "Add @salt, @pepper, and @herbs.",
    expected: [
      { text: "@", highlight: "punctuation.special" },
      { text: "salt", highlight: "variable" },
      { text: "@", highlight: "punctuation.special" },
      { text: "pepper", highlight: "variable" },
      { text: "@", highlight: "punctuation.special" },
      { text: "herbs", highlight: "variable" }
    ]
  },
  {
    name: "Multiword ingredients",
    input: "Add @olive oil{2%tbsp}.",
    expected: [
      { text: "@", highlight: "punctuation.special" },
      { text: "olive oil", highlight: "variable" },
      { text: "2", highlight: "number" },
      { text: "%", highlight: "operator" },
      { text: "tbsp", highlight: "type" },
      { text: "{", highlight: "punctuation.bracket" },
      { text: "}", highlight: "punctuation.bracket" }
    ]
  }
];

// Create parser
const parser = new Parser();
parser.setLanguage(Cooklang);

// Map tree-sitter highlight names to our expected names
const highlightMap = {
  "@": "punctuation.special",
  "#": "punctuation.special", 
  "~": "punctuation.special",
  "{": "punctuation.bracket",
  "}": "punctuation.bracket",
  "(": "punctuation.bracket",
  ")": "punctuation.bracket",
  "%": "operator",
  "ingredient_text": "variable",
  "cookware_text": "function",
  "timer_text": "constant",
  "quantity": "number",
  "units": "type",
  "comment": "comment",
  "note": "comment"
};

function getHighlights(tree, source) {
  const highlights = [];
  const cursor = tree.walk();
  
  function visit() {
    const node = cursor.currentNode;
    const highlight = highlightMap[node.type];
    
    // Special handling for text nodes that are highlighted
    if (node.type === 'ingredient_text' || node.type === 'cookware_text' || node.type === 'timer_text') {
      highlights.push({
        text: source.substring(node.startIndex, node.endIndex),
        highlight: highlightMap[node.type],
        start: node.startIndex,
        end: node.endIndex
      });
    } else if (highlight && node.childCount === 0) {
      highlights.push({
        text: source.substring(node.startIndex, node.endIndex),
        highlight: highlight,
        start: node.startIndex,
        end: node.endIndex
      });
    }
    
    if (cursor.gotoFirstChild()) {
      do {
        visit();
      } while (cursor.gotoNextSibling());
      cursor.gotoParent();
    }
  }
  
  visit();
  return highlights.sort((a, b) => a.start - b.start);
}

// Run tests
let passed = 0;
let failed = 0;

console.log("Testing Cooklang Syntax Highlighting");
console.log("=====================================\n");

testCases.forEach(test => {
  const tree = parser.parse(test.input);
  const highlights = getHighlights(tree, test.input);
  
  // Filter to only the expected highlight types
  const relevantHighlights = highlights.filter(h => 
    test.expected.some(e => e.text === h.text)
  );
  
  let success = true;
  
  // Check each expected highlight
  test.expected.forEach(expected => {
    const found = relevantHighlights.find(h => 
      h.text === expected.text && h.highlight === expected.highlight
    );
    
    if (!found) {
      success = false;
      console.log(`❌ ${test.name}`);
      console.log(`   Missing highlight: "${expected.text}" as ${expected.highlight}`);
      
      // Show what we actually found for this text
      const actualForText = highlights.filter(h => h.text === expected.text);
      if (actualForText.length > 0) {
        console.log(`   Actually highlighted as: ${actualForText.map(h => h.highlight).join(', ')}`);
      } else {
        console.log(`   Text "${expected.text}" not found in highlights`);
      }
    }
  });
  
  if (success) {
    console.log(`✅ ${test.name}`);
    passed++;
  } else {
    failed++;
    console.log(`   Input: "${test.input}"`);
    console.log(`   All highlights found:`, highlights.map(h => `${h.text}(${h.highlight})`).join(', '));
  }
  
  console.log();
});

console.log("=====================================");
console.log(`Passed: ${passed}/${testCases.length}`);
console.log(`Failed: ${failed}/${testCases.length}`);

if (failed > 0) {
  process.exit(1);
}

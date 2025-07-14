const PREC = {
  section: 1,
  step: 2,
  modifier: 3,
  amount: 4,
  ingredient_with_amount: 5,
  multiword: 10,
  quantity: 11,
}

module.exports = grammar({
  name: 'cooklang',

  externals: $ => [
    $._newline,
    $.ingredient_text,
    $.cookware_text,
    $.timer_text,
    $.plain_text,
    $.note_text,
    $.section_header
  ],

  conflicts: $ => [
    [$._integer, $._digit],
    [$._fractional, $._number],
    [$._unreserved_symbol, $._symbol],
    [$.comment, $._alphabetic],
    [$.metadata, $.metadata], // TODO: is this really necessary?
    [$._multiword],
    [$._name_multiword],
    [$.step],
    [$._content_line, $.step],
  ],

  rules: {
    recipe: $ => seq(
      optional($.frontmatter),
      repeat(choice(
        seq($.metadata, $._newline),
        seq($.section, $._newline),
        seq($._content_line, $._newline),
        $._newline  // Allow blank lines
      )),
      optional(choice(
        $._content_line,
        $.section
      )), // Allow content without trailing newline
      optional(/\u0000/), // Strange EOF thing
    ),

    _content_line: $ => choice(
      $.step,
      $.comment,
      $.block_comment,
      $.note,
    ),

    frontmatter: $ => seq(
      "---",
      $._newline,
      field('content', optional(alias($._frontmatter_content, $.yaml_content))),
      "---",
      $._newline
    ),

    _frontmatter_content: $ => repeat1(
      seq(
        /[^\n]+/,
        $._newline
      )
    ),

    metadata:           $ => seq(">>", $._word, ":", $._whitespace, choice($._text_item, $._number, $.amount)),
    step:               $ => repeat1(choice(
      $.ingredient,
      $.cookware,
      $.timer,
      $.plain_text,
      $.block_comment
    )),
    comment:            $ => seq("-", "-", /.*/),
    block_comment:      $ => token(
      seq(
        '[', '-',
        repeat(choice(
          /[^\-\]]/,
          seq('-', /[^\]]/)
        )),
        '-', ']'
      )
    ),
    note:               $ => seq(">", /.*/),
    section:            $ => seq(
      field('header', $.section_header)
    ),

    ingredient:         $ => seq(
      "@",
      field('name', $.ingredient_text),
      optional(seq(
        "{",
        optional($.amount),
        "}"
      )),
      optional(seq("(", field('note', $.note_text), ")"))
    ),
    cookware:           $ => seq(
      "#",
      field('name', $.cookware_text),
      optional(seq(
        "{",
        optional($.amount),
        "}"
      )),
      optional(seq("(", field('note', $.note_text), ")"))
    ),
    timer:              $ => seq(
      "~",
      optional(field('name', $.timer_text)),
      optional(seq(
        "{",
        optional($.amount),
        "}"
      )),
      optional(seq("(", field('note', $.note_text), ")"))
    ),

    // Ref: https://github.com/cooklang/spec/blob/main/EBNF.md
    name:               $ => prec.left(choice($._name_word, $._name_multiword)),
    _name_word:         $ => repeat1(choice($._alphabetic, $._digit, /[_\-\\]/)),
    _name_multiword:    $ => seq(repeat1(prec.left(PREC.multiword, seq($._name_word, repeat1($._whitespace)))), optional($._name_word)),
    recipe_reference:   $ => /\.[\/\\][^{}\n(]*/,  // Matches paths starting with ./ or .\ (Windows)
    amount:             $ => prec.left(PREC.amount, choice($.quantity, seq($.quantity, repeat($._whitespace), "%", repeat($._whitespace), $.units))),
    quantity:           $ => prec(PREC.quantity, choice(
      $._number,
      $._name_multiword,
      seq($._integer, repeat1($._whitespace), $._fractional)  // Mixed fraction like "1 1/2"
    )),
    units:              $ => choice($._name_word, $._name_multiword),

    _multiword:         $ => seq(repeat1(prec.left(PREC.multiword, seq($._word, repeat1($._whitespace)))), optional($._word)),
    _word:              $ => repeat1(choice($._alphabetic, $._digit, /[_\-\\]/)),
    _text_item:         $ => repeat1(choice($._alphabetic, $._digit, $._symbol, $._punctuation, $._whitespace)),

    _number:            $ => choice($._integer, $._fractional, $._decimal),
    _fractional:        $ => seq($._integer, repeat($._whitespace), "/", repeat($._whitespace), $._integer),
    _decimal:           $ => seq($._integer, ".", $._integer),
    _integer:           $ => choice($._zero, seq($._non_zero_digit, repeat($._digit))),
    _digit:             $ => choice($._zero, $._non_zero_digit),
    _non_zero_digit:    $ => /[1-9]/,
    _zero:              $ => "0",

    _alphabetic:        $ => choice(/\p{L}/, /\p{M}/, "-"), // NOTE: "-" is not in the official grammar.
    _whitespace:        $ => choice(/\p{Zs}/, /\u0009/),
    _punctuation:       $ => choice(/\p{P}/),
    _unreserved_symbol: $ => choice(/\p{S}/), // TODO: remove bad symbol
    _symbol:            $ => choice(/\p{S}/),
  }
});

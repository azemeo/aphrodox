options {
	language = "CSharp";
	namespace = "Aphrodox.DocBlock";
}

class DocBlockParser extends Parser;
options {
	buildAST = true;
}

parse returns [Documentation doc]
{
	doc = new Documentation();
}
	:	docBlock[doc]
	;

docBlock[Documentation doc]
	:	TEXT
	{
		doc.Summary.InnerText = #docBlock.ToString();
	}
	;

class DocBlockLexer extends Lexer;
options { k = 1; }

TEXT
	:	(WHITESPACE!)? INNER_TEXT (WHITESPACE!)?
	;

protected INNER_TEXT
	:	WORD (WHITESPACE)? (INNER_TEXT)?
	;
	
protected WORD
	:	WORD_CHAR (WORD_CHAR)*
	;

protected WORD_CHAR
	:	LETTER | PUNCTUATION | ACCENT
	;

protected LETTER
	:	'A'..'Z' | 'a'..'z'
	;

protected PUNCTUATION
	:	'.' | '!' | '\''
	;
	
protected ACCENT
	:	'\u00C0'..'\u0259'
	;

protected WHITESPACE
	:	SPACE
	;

protected COLLAPSED_WHITESPACE
	:	WHITESPACE
	{
		$setText(" ");
	}
	;

protected SPACE
	:	' ' | '\t'
	;

protected NL
	:	(DOS_NL) => DOS_NL | UNIX_NL | MAC_NL
	;

protected DOS_NL	:	"\r\n"	{ newline(); };
protected UNIX_NL	:	'\n'	{ newline(); };
protected MAC_NL	:	'\r'	{ newline(); };


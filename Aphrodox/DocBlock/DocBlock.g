options {
	language = "CSharp";
	namespace = "Aphrodox.DocBlock";
}

{
	using System.Xml;
}
class DocBlockParser extends Parser;
options { buildAST = true; k = 1; }

parse returns [Documentation doc] {
	doc = new Documentation();
}
	: docBlock[doc] {
	}
	;

docBlock[Documentation doc]
	: (summary[doc.Summary])?
	;

summary[XmlDocumentFragment summaryFragment]
	:	x:WORD (SPACE WORD)*
	{
		summaryFragment.AppendChild(summaryFragment.OwnerDocument.CreateTextNode(x.getText()));
		Console.WriteLine( "Nih: " + #summary.getText() + #summary.getNumberOfChildren() + "x" );
		Console.WriteLine( "Mah: " + #summary.ToStringList() );
	}
	;
	
remarks
	:	"Dummy"
	;

tagBlock
	:	"another dummy"
	;

class DocBlockLexer extends Lexer;
options {
	k = 4;
	charVocabulary = '\u0000'..'\u00FF';
}

protected
NEWLINE
    :	(
			options { generateAmbigWarnings = false; }
		:	'\r' '\n'   { newline(); }	// DOS
		|   '\n'        { newline(); }	// UNIX
		|	'\r'		{ newline(); }	// MAC
		)
    ;

protected
SPACE
	:	(
			' '
		|	'\u0009'	// tab
		|	'\u000B'	// vertical tab
		|	'\u000C'	// form feed
		)+
	;

protected
WHITESPACE
	:	SPACE
	|	NEWLINE;

protected
IGNORED_WHITESPACE
	:	WHITESPACE
	|	NEWLINE
	{
		$setType(Token.SKIP);
	}
	;

protected
COLLAPSED_SPACE
	:	SPACE
	{
	}
	;

TEXT_FRAGMENT
	:	WORD (SPACE | WORD)*
	;

protected
WORD
/*	:	(~('<'|'>'|'@'|' '|'\r'|'\n'|'\t'|'\u000B'|'\u000C'))* */
	:	('A'..'Z'|'a'..'z'|'!')+
	;

CODE
	:	"<c>"! ( options { greedy = false; } : . )* "</c>"!
	|	"<code>"! ( options { greedy = false; } : . )* "</code>"!
	;

TAG
	:	'@' ('a'..'z')+
	;

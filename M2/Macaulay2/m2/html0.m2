--		Copyright 1993-2003 by Daniel R. Grayson

-----------------------------------------------------------------------------
-- html input
-----------------------------------------------------------------------------

nonnull = x -> select(x, i -> i =!= null)

html = method(SingleArgumentDispatch=>true, TypicalValue => String)
-- text used to be one of the conversion functions, but now we just use "net"
tex = method(SingleArgumentDispatch=>true, TypicalValue => String)
texMath = method(SingleArgumentDispatch=>true, TypicalValue => String)
mathML = method(SingleArgumentDispatch=>true, TypicalValue => String)
info = method(SingleArgumentDispatch=>true, TypicalValue => String)

MarkUpList = new Type of BasicList
MarkUpList.synonym = "mark-up list"

MarkUpListParagraph = new Type of MarkUpList
MarkUpListParagraph.synonym = "mark-up list paragraph"

     MarkUpType = new Type of Type
MarkUpType.synonym = "mark-up type"

options MarkUpType := X -> X.Options

MarkUpTypeWithOptions = new Type of MarkUpType
MarkUpTypeWithOptions.synonym = "mark-up type with options"

     MarkUpType Sequence := 
     MarkUpType List := (h,y) -> new h from y
     MarkUpType Thing := (h,y) -> new h from {y}
     MarkUpType\List := (h,y) -> (i -> h i) \ y
     List/MarkUpType := (y,h) -> y / (i -> h i)

IntermediateMarkUpType = new Type of MarkUpType	    -- this is for things like MENU, which do not correspond to an html entity, but have a recipe for translation into html
IntermediateMarkUpType.synonym = "intermediate mark-up type"

makeList := method()
makeList MarkUpType := X -> toString X
makeList Type       := X -> concatenate("new ", toString X, " from ")
toExternalString MarkUpList := s -> concatenate(makeList class s, toExternalString toList s)
toString         MarkUpList := s -> concatenate(makeList class s, toString         toList s)

lower := "abcdefghijklmnopqrstuvwxyz"
upper := "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
tolower := new HashTable from apply(characters upper,characters lower,identity)
toupper := new HashTable from apply(characters lower,characters upper,identity)
toLower = s -> concatenate apply(characters s, c -> if tolower#?c then tolower#c else c)
toUpper = s -> concatenate apply(characters s, c -> if toupper#?c then toupper#c else c)

htmlMarkUpType := s -> (
     on := "\n<" | s | ">";
     off := "</" | s | ">\n";
     t -> concatenate(on, apply(t,html), off))

htmlMarkUpTypeWithOptions := opts -> s -> (
     off := "</" | s | ">\n";
     t -> (
	  o := "";
	  (opts',u) := override(opts, toSequence t);
	  scanPairs(opts', (k,v) -> if v =!= null then o = concatenate(o, " ", k, "=", format v));
	  concatenate("\n<", s, o, ">", apply(sequence u,html), off)
	  ))

MarkUpType.GlobalAssignHook = (X,x) -> (
     if not x.?qname then x.qname = toLower toString X;
     if not ReverseDictionary#?x then (
	  ReverseDictionary#x = X;
     	  html x := htmlMarkUpType x.qname;
	  );
     )

IntermediateMarkUpType.GlobalAssignHook = globalAssignFunction -- no qname, no default method for producing html

MarkUpTypeWithOptions.GlobalAssignHook = (X,x) -> (
     if not x.?qname then x.qname = toLower toString X;
     if not ReverseDictionary#?x then (
	  ReverseDictionary#x = X;
     	  html x := (htmlMarkUpTypeWithOptions options x) x.qname;
	  );
     )

withOptions := (v,x) -> (x.Options = new OptionTable from apply(v,val -> if class val === Option then val else val=>null); x)
withQname   := (q,x) -> (x.qname = q; x)
requirestrings := x -> ( scan(x, line -> if class line =!= String then error ("expected a string, but encountered ", toString line)); x)
trimfront := x -> apply(x, line -> (
	  s := lines line;
	  concatenate between(newline, prepend(replace("^[[:space:]]+","",s#0), drop(s,1)))))
nonempty := x -> select(x, i -> i =!= "")

new MarkUpType := x -> error "obsolete 'new' method called"

BR         = new MarkUpType of MarkUpListParagraph
br         = BR{}

NOINDENT   = new IntermediateMarkUpType of MarkUpList

HR         = new MarkUpType of MarkUpListParagraph
hr         = HR{}

new NOINDENT from List := 
new HR from List := 
new BR from List := (X,x) -> if #x>0 then error "expected empty list" else x

PARA       = withQname_"p" new MarkUpType of MarkUpListParagraph	    -- double spacing inside

ExampleItem = withQname_"code" new MarkUpType of MarkUpList
EXAMPLE = method(SingleArgumentDispatch => true)
EXAMPLE VisibleList := x -> TABLE splice { "class" => "examples", apply(trimfront nonempty requirestrings nonnull toSequence x, item -> TR TD ExampleItem item) }
EXAMPLE String := x -> EXAMPLE {x}

PRE        = new MarkUpType of MarkUpListParagraph
TITLE      = new MarkUpType of MarkUpListParagraph
HEAD       = new MarkUpType of MarkUpListParagraph
BODY       = new MarkUpType of MarkUpListParagraph
IMG	   = withOptions_{"src","alt"} new MarkUpTypeWithOptions of MarkUpList
HTML       = new MarkUpType of MarkUpList
HEADER1    = withQname_"h1" new MarkUpType of MarkUpListParagraph
HEADER2    = withQname_"h2" new MarkUpType of MarkUpListParagraph
HEADER3    = withQname_"h3" new MarkUpType of MarkUpListParagraph
HEADER4    = withQname_"h4" new MarkUpType of MarkUpListParagraph
HEADER5    = withQname_"h5" new MarkUpType of MarkUpListParagraph
HEADER6    = withQname_"h6" new MarkUpType of MarkUpListParagraph
SUBSECTION = HEADER2
LISTING    = new MarkUpType of MarkUpListParagraph
LITERAL    = new IntermediateMarkUpType of MarkUpList
BLOCKQUOTE = new MarkUpType of MarkUpList
STRONG     = new MarkUpType of MarkUpList
SMALL      = new MarkUpType of MarkUpList
SUB        = new MarkUpType of MarkUpList
SUP        = new MarkUpType of MarkUpList
ITALIC     = withQname_"i" new MarkUpType of MarkUpList
TEX	   = withQname_"#PCDATA" new MarkUpType of MarkUpList -- TEX really needs to be processed further so its output can be checked, too!
SPAN       = new MarkUpType of MarkUpList
TT         = new MarkUpType of MarkUpList
LI         = new MarkUpType of MarkUpList
EM         = new MarkUpType of MarkUpList
LABEL      = new MarkUpType of MarkUpList
BOLD       = withQname_"b" new MarkUpType of MarkUpList
CODE       = new MarkUpType of MarkUpList

HREF       = withQname_"a" new IntermediateMarkUpType of MarkUpList
new HREF from List := (HREF,x) -> (
     if #x > 2 or #x == 0 then error "HREF list should have length 1 or 2";
     if class x#0 =!= String then error "HREF expected URL to be a string";
     x)

LINK       = new MarkUpType of MarkUpList
ANCHOR     = withQname_"a" new MarkUpType of MarkUpList

UL         = new MarkUpType of MarkUpListParagraph
new UL from List := (UL,x) -> (
     x = nonnull splice x;
     if #x == 0 then error("empty element of type ", format toString UL, " encountered");
     apply(x, e -> (
	       if class e === TO then LI{TOH{e#0}}
	       else if class e === LI then e
	       else LI e)))

DIV        = withOptions_{"class"} new MarkUpTypeWithOptions of MarkUpListParagraph
DIV1       = withOptions_{"class"=>"single"} withQname_"div" new MarkUpTypeWithOptions of MarkUpListParagraph

TABLE      = withOptions_{"class"} new MarkUpTypeWithOptions of MarkUpListParagraph
TR         = new MarkUpType of MarkUpList
TD         = new MarkUpType of MarkUpList
ButtonTABLE  = new MarkUpType of MarkUpListParagraph

TO2        = withQname_"a" new IntermediateMarkUpType of MarkUpList
new TO2 from Sequence := 
new TO2 from List := (TO2,x) -> { makeDocumentTag x#0, concatenate drop(toSequence x,1) }

TO         = withQname_"a" new IntermediateMarkUpType of MarkUpList
new TO from List := (TO,x) -> if x#?1 then { makeDocumentTag x#0, concatenate drop(toSequence x,1) } else { makeDocumentTag x#0 }

TOH        = withQname_"span" new IntermediateMarkUpType of MarkUpList
new TOH from List := (TOH,x) -> { makeDocumentTag x#0 }

new TO from Sequence := new TOH from Sequence := (TO,x) -> new TO from {x} -- document tags can be sequences, so keep them intact

MENU       = withQname_"div" new IntermediateMarkUpType of MarkUpListParagraph	            -- like "* Menu:" of "info"

-- Local Variables:
-- compile-command: "make -C $M2BUILDDIR/Macaulay2/m2 "
-- End:

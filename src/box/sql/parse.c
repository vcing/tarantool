/*
** 2000-05-29
**
** The author disclaims copyright to this source code.  In place of
** a legal notice, here is a blessing:
**
**    May you do good and not evil.
**    May you find forgiveness for yourself and forgive others.
**    May you share freely, never taking more than you give.
**
*************************************************************************
** Driver template for the LEMON parser generator.
**
** The "lemon" program processes an LALR(1) input grammar file, then uses
** this template to construct a parser.  The "lemon" program inserts text
** at each "%%" line.  Also, any "P-a-r-s-e" identifer prefix (without the
** interstitial "-" characters) contained in this template is changed into
** the value of the %name directive from the grammar.  Otherwise, the content
** of this template is copied straight through into the generate parser
** source file.
**
** The following is the concatenation of all %include directives from the
** input grammar file:
*/
#include <stdio.h>
/************ Begin %include sections from the grammar ************************/
#line 48 "parse.y"

#include "sqliteInt.h"

/*
** Disable all error recovery processing in the parser push-down
** automaton.
*/
#define YYNOERRORRECOVERY 1

/*
** Make yytestcase() the same as testcase()
*/
#define yytestcase(X) testcase(X)

/*
** Indicate that sqlite3ParserFree() will never be called with a null
** pointer.
*/
#define YYPARSEFREENEVERNULL 1

/*
** Alternative datatype for the argument to the malloc() routine passed
** into sqlite3ParserAlloc().  The default is size_t.
*/
#define YYMALLOCARGTYPE  u64

/*
** An instance of this structure holds information about the
** LIMIT clause of a SELECT statement.
*/
struct LimitVal {
  Expr *pLimit;    /* The LIMIT expression.  NULL if there is no limit */
  Expr *pOffset;   /* The OFFSET expression.  NULL if there is none */
};

/*
** An instance of the following structure describes the event of a
** TRIGGER.  "a" is the event type, one of TK_UPDATE, TK_INSERT,
** TK_DELETE, or TK_INSTEAD.  If the event is of the form
**
**      UPDATE ON (a,b,c)
**
** Then the "b" IdList records the list "a,b,c".
*/
struct TrigEvent { int a; IdList * b; };

/*
** Disable lookaside memory allocation for objects that might be
** shared across database connections.
*/
static void disableLookaside(Parse *pParse){
  pParse->disableLookaside++;
  pParse->db->lookaside.bDisable++;
}

#line 399 "parse.y"

  /*
  ** For a compound SELECT statement, make sure p->pPrior->pNext==p for
  ** all elements in the list.  And make sure list length does not exceed
  ** SQLITE_LIMIT_COMPOUND_SELECT.
  */
  static void parserDoubleLinkSelect(Parse *pParse, Select *p){
    if( p->pPrior ){
      Select *pNext = 0, *pLoop;
      int mxSelect, cnt = 0;
      for(pLoop=p; pLoop; pNext=pLoop, pLoop=pLoop->pPrior, cnt++){
        pLoop->pNext = pNext;
        pLoop->selFlags |= SF_Compound;
      }
      if( (p->selFlags & SF_MultiValue)==0 && 
        (mxSelect = pParse->db->aLimit[SQLITE_LIMIT_COMPOUND_SELECT])>0 &&
        cnt>mxSelect
      ){
        sqlite3ErrorMsg(pParse, "Too many UNION or EXCEPT or INTERSECT operations");
      }
    }
  }
#line 836 "parse.y"

  /* This is a utility routine used to set the ExprSpan.zStart and
  ** ExprSpan.zEnd values of pOut so that the span covers the complete
  ** range of text beginning with pStart and going to the end of pEnd.
  */
  static void spanSet(ExprSpan *pOut, Token *pStart, Token *pEnd){
    pOut->zStart = pStart->z;
    pOut->zEnd = &pEnd->z[pEnd->n];
  }

  /* Construct a new Expr object from a single identifier.  Use the
  ** new Expr to populate pOut.  Set the span of pOut to be the identifier
  ** that created the expression.
  */
  static void spanExpr(ExprSpan *pOut, Parse *pParse, int op, Token t){
    Expr *p = sqlite3DbMallocRawNN(pParse->db, sizeof(Expr)+t.n+1);
    if( p ){
      memset(p, 0, sizeof(Expr));
      p->op = (u8)op;
      p->flags = EP_Leaf;
      p->iAgg = -1;
      p->u.zToken = (char*)&p[1];
      memcpy(p->u.zToken, t.z, t.n);
      p->u.zToken[t.n] = 0;
      if( sqlite3Isquote(p->u.zToken[0]) ){
        if( p->u.zToken[0]=='"' ) p->flags |= EP_DblQuoted;
        sqlite3Dequote(p->u.zToken);
      }
#if SQLITE_MAX_EXPR_DEPTH>0
      p->nHeight = 1;
#endif  
    }
    pOut->pExpr = p;
    pOut->zStart = t.z;
    pOut->zEnd = &t.z[t.n];
  }
#line 953 "parse.y"

  /* This routine constructs a binary expression node out of two ExprSpan
  ** objects and uses the result to populate a new ExprSpan object.
  */
  static void spanBinaryExpr(
    Parse *pParse,      /* The parsing context.  Errors accumulate here */
    int op,             /* The binary operation */
    ExprSpan *pLeft,    /* The left operand, and output */
    ExprSpan *pRight    /* The right operand */
  ){
    pLeft->pExpr = sqlite3PExpr(pParse, op, pLeft->pExpr, pRight->pExpr);
    pLeft->zEnd = pRight->zEnd;
  }

  /* If doNot is true, then add a TK_NOT Expr-node wrapper around the
  ** outside of *ppExpr.
  */
  static void exprNot(Parse *pParse, int doNot, ExprSpan *pSpan){
    if( doNot ){
      pSpan->pExpr = sqlite3PExpr(pParse, TK_NOT, pSpan->pExpr, 0);
    }
  }
#line 1027 "parse.y"

  /* Construct an expression node for a unary postfix operator
  */
  static void spanUnaryPostfix(
    Parse *pParse,         /* Parsing context to record errors */
    int op,                /* The operator */
    ExprSpan *pOperand,    /* The operand, and output */
    Token *pPostOp         /* The operand token for setting the span */
  ){
    pOperand->pExpr = sqlite3PExpr(pParse, op, pOperand->pExpr, 0);
    pOperand->zEnd = &pPostOp->z[pPostOp->n];
  }                           
#line 1044 "parse.y"

  /* A routine to convert a binary TK_IS or TK_ISNOT expression into a
  ** unary TK_ISNULL or TK_NOTNULL expression. */
  static void binaryToUnaryIfNull(Parse *pParse, Expr *pY, Expr *pA, int op){
    sqlite3 *db = pParse->db;
    if( pA && pY && pY->op==TK_NULL ){
      pA->op = (u8)op;
      sqlite3ExprDelete(db, pA->pRight);
      pA->pRight = 0;
    }
  }
#line 1072 "parse.y"

  /* Construct an expression node for a unary prefix operator
  */
  static void spanUnaryPrefix(
    ExprSpan *pOut,        /* Write the new expression node here */
    Parse *pParse,         /* Parsing context to record errors */
    int op,                /* The operator */
    ExprSpan *pOperand,    /* The operand */
    Token *pPreOp         /* The operand token for setting the span */
  ){
    pOut->zStart = pPreOp->z;
    pOut->pExpr = sqlite3PExpr(pParse, op, pOperand->pExpr, 0);
    pOut->zEnd = pOperand->zEnd;
  }
#line 1284 "parse.y"

  /* Add a single new term to an ExprList that is used to store a
  ** list of identifiers.  Report an error if the ID list contains
  ** a COLLATE clause or an ASC or DESC keyword, except ignore the
  ** error while parsing a legacy schema.
  */
  static ExprList *parserAddExprIdListTerm(
    Parse *pParse,
    ExprList *pPrior,
    Token *pIdToken,
    int hasCollate,
    int sortOrder
  ){
    ExprList *p = sqlite3ExprListAppend(pParse, pPrior, 0);
    if( (hasCollate || sortOrder!=SQLITE_SO_UNDEFINED)
        && pParse->db->init.busy==0
    ){
      sqlite3ErrorMsg(pParse, "syntax error after column name \"%.*s\"",
                         pIdToken->n, pIdToken->z);
    }
    sqlite3ExprListSetName(pParse, p, pIdToken, 1);
    return p;
  }
#line 231 "parse.c"
/**************** End of %include directives **********************************/
/* These constants specify the various numeric values for terminal symbols
** in a format understandable to "makeheaders".  This section is blank unless
** "lemon" is run with the "-m" command-line option.
***************** Begin makeheaders token definitions *************************/
/**************** End makeheaders token definitions ***************************/

/* The next sections is a series of control #defines.
** various aspects of the generated parser.
**    YYCODETYPE         is the data type used to store the integer codes
**                       that represent terminal and non-terminal symbols.
**                       "unsigned char" is used if there are fewer than
**                       256 symbols.  Larger types otherwise.
**    YYNOCODE           is a number of type YYCODETYPE that is not used for
**                       any terminal or nonterminal symbol.
**    YYFALLBACK         If defined, this indicates that one or more tokens
**                       (also known as: "terminal symbols") have fall-back
**                       values which should be used if the original symbol
**                       would not parse.  This permits keywords to sometimes
**                       be used as identifiers, for example.
**    YYACTIONTYPE       is the data type used for "action codes" - numbers
**                       that indicate what to do in response to the next
**                       token.
**    sqlite3ParserTOKENTYPE     is the data type used for minor type for terminal
**                       symbols.  Background: A "minor type" is a semantic
**                       value associated with a terminal or non-terminal
**                       symbols.  For example, for an "ID" terminal symbol,
**                       the minor type might be the name of the identifier.
**                       Each non-terminal can have a different minor type.
**                       Terminal symbols all have the same minor type, though.
**                       This macros defines the minor type for terminal 
**                       symbols.
**    YYMINORTYPE        is the data type used for all minor types.
**                       This is typically a union of many types, one of
**                       which is sqlite3ParserTOKENTYPE.  The entry in the union
**                       for terminal symbols is called "yy0".
**    YYSTACKDEPTH       is the maximum depth of the parser's stack.  If
**                       zero the stack is dynamically sized using realloc()
**    sqlite3ParserARG_SDECL     A static variable declaration for the %extra_argument
**    sqlite3ParserARG_PDECL     A parameter declaration for the %extra_argument
**    sqlite3ParserARG_STORE     Code to store %extra_argument into yypParser
**    sqlite3ParserARG_FETCH     Code to extract %extra_argument from yypParser
**    YYERRORSYMBOL      is the code number of the error symbol.  If not
**                       defined, then do no error processing.
**    YYNSTATE           the combined number of states.
**    YYNRULE            the number of rules in the grammar
**    YY_MAX_SHIFT       Maximum value for shift actions
**    YY_MIN_SHIFTREDUCE Minimum value for shift-reduce actions
**    YY_MAX_SHIFTREDUCE Maximum value for shift-reduce actions
**    YY_MIN_REDUCE      Maximum value for reduce actions
**    YY_ERROR_ACTION    The yy_action[] code for syntax error
**    YY_ACCEPT_ACTION   The yy_action[] code for accept
**    YY_NO_ACTION       The yy_action[] code for no-op
*/
#ifndef INTERFACE
# define INTERFACE 1
#endif
/************* Begin control #defines *****************************************/
#define YYCODETYPE unsigned char
#define YYNOCODE 246
#define YYACTIONTYPE unsigned short int
#define YYWILDCARD 95
#define sqlite3ParserTOKENTYPE Token
typedef union {
  int yyinit;
  sqlite3ParserTOKENTYPE yy0;
  TriggerStep* yy7;
  int yy32;
  struct {int value; int mask;} yy47;
  ExprSpan yy132;
  Select* yy149;
  struct TrigEvent yy160;
  With* yy241;
  SrcList* yy287;
  Expr* yy342;
  IdList* yy440;
  ExprList* yy462;
  struct LimitVal yy474;
} YYMINORTYPE;
#ifndef YYSTACKDEPTH
#define YYSTACKDEPTH 100
#endif
#define sqlite3ParserARG_SDECL Parse *pParse;
#define sqlite3ParserARG_PDECL ,Parse *pParse
#define sqlite3ParserARG_FETCH Parse *pParse = yypParser->pParse
#define sqlite3ParserARG_STORE yypParser->pParse = pParse
#define YYFALLBACK 1
#define YYNSTATE             436
#define YYNRULE              319
#define YY_MAX_SHIFT         435
#define YY_MIN_SHIFTREDUCE   641
#define YY_MAX_SHIFTREDUCE   959
#define YY_MIN_REDUCE        960
#define YY_MAX_REDUCE        1278
#define YY_ERROR_ACTION      1279
#define YY_ACCEPT_ACTION     1280
#define YY_NO_ACTION         1281
/************* End control #defines *******************************************/

/* Define the yytestcase() macro to be a no-op if is not already defined
** otherwise.
**
** Applications can choose to define yytestcase() in the %include section
** to a macro that can assist in verifying code coverage.  For production
** code the yytestcase() macro should be turned off.  But it is useful
** for testing.
*/
#ifndef yytestcase
# define yytestcase(X)
#endif


/* Next are the tables used to determine what action to take based on the
** current state and lookahead token.  These tables are used to implement
** functions that take a state number and lookahead value and return an
** action integer.  
**
** Suppose the action integer is N.  Then the action is determined as
** follows
**
**   0 <= N <= YY_MAX_SHIFT             Shift N.  That is, push the lookahead
**                                      token onto the stack and goto state N.
**
**   N between YY_MIN_SHIFTREDUCE       Shift to an arbitrary state then
**     and YY_MAX_SHIFTREDUCE           reduce by rule N-YY_MIN_SHIFTREDUCE.
**
**   N between YY_MIN_REDUCE            Reduce by rule N-YY_MIN_REDUCE
**     and YY_MAX_REDUCE
**
**   N == YY_ERROR_ACTION               A syntax error has occurred.
**
**   N == YY_ACCEPT_ACTION              The parser accepts its input.
**
**   N == YY_NO_ACTION                  No such action.  Denotes unused
**                                      slots in the yy_action[] table.
**
** The action table is constructed as a single large table named yy_action[].
** Given state S and lookahead X, the action is computed as either:
**
**    (A)   N = yy_action[ yy_shift_ofst[S] + X ]
**    (B)   N = yy_default[S]
**
** The (A) formula is preferred.  The B formula is used instead if:
**    (1)  The yy_shift_ofst[S]+X value is out of range, or
**    (2)  yy_lookahead[yy_shift_ofst[S]+X] is not equal to X, or
**    (3)  yy_shift_ofst[S] equal YY_SHIFT_USE_DFLT.
** (Implementation note: YY_SHIFT_USE_DFLT is chosen so that
** YY_SHIFT_USE_DFLT+X will be out of range for all possible lookaheads X.
** Hence only tests (1) and (2) need to be evaluated.)
**
** The formulas above are for computing the action when the lookahead is
** a terminal symbol.  If the lookahead is a non-terminal (as occurs after
** a reduce action) then the yy_reduce_ofst[] array is used in place of
** the yy_shift_ofst[] array and YY_REDUCE_USE_DFLT is used in place of
** YY_SHIFT_USE_DFLT.
**
** The following are the tables generated in this section:
**
**  yy_action[]        A single table containing all actions.
**  yy_lookahead[]     A table containing the lookahead for each entry in
**                     yy_action.  Used to detect hash collisions.
**  yy_shift_ofst[]    For each state, the offset into yy_action for
**                     shifting terminals.
**  yy_reduce_ofst[]   For each state, the offset into yy_action for
**                     shifting non-terminals after a reduce.
**  yy_default[]       Default action for each state.
**
*********** Begin parsing tables **********************************************/
#define YY_ACTTAB_COUNT (1470)
static const YYACTIONTYPE yy_action[] = {
 /*     0 */    91,   92,  303,   82,  812,  812,  824,  827,  816,  816,
 /*    10 */    89,   89,   90,   90,   90,   90,   66,   88,   88,   88,
 /*    20 */    88,   87,   87,   86,   86,   86,   85,  328,   90,   90,
 /*    30 */    90,   90,   83,   88,   88,   88,   88,   87,   87,   86,
 /*    40 */    86,   86,   85,  328,  278,  390,  195,  724,  724,  939,
 /*    50 */   642,  331,   91,   92,  303,   82,  812,  812,  824,  827,
 /*    60 */   816,  816,   89,   89,   90,   90,   90,   90,  683,   88,
 /*    70 */    88,   88,   88,   87,   87,   86,   86,   86,   85,  328,
 /*    80 */    87,   87,   86,   86,   86,   85,  328,   90,   90,   90,
 /*    90 */    90,  939,   88,   88,   88,   88,   87,   87,   86,   86,
 /*   100 */    86,   85,  328,  328,  434,  434,  108,  761,  732,  124,
 /*   110 */   234,  668,  122, 1280,  435,    2,  762,  719,  670,   84,
 /*   120 */    81,  169,   91,   92,  303,   82,  812,  812,  824,  827,
 /*   130 */   816,  816,   89,   89,   90,   90,   90,   90,  422,   88,
 /*   140 */    88,   88,   88,   87,   87,   86,   86,   86,   85,  328,
 /*   150 */    22,  920,  920,  668,   91,   92,  303,   82,  812,  812,
 /*   160 */   824,  827,  816,  816,   89,   89,   90,   90,   90,   90,
 /*   170 */    67,   88,   88,   88,   88,   87,   87,   86,   86,   86,
 /*   180 */    85,  328,   88,   88,   88,   88,   87,   87,   86,   86,
 /*   190 */    86,   85,  328,  921,  922,  369,  332,   93,  366,  257,
 /*   200 */    84,   81,  169,   91,   92,  303,   82,  812,  812,  824,
 /*   210 */   827,  816,  816,   89,   89,   90,   90,   90,   90,  673,
 /*   220 */    88,   88,   88,   88,   87,   87,   86,   86,   86,   85,
 /*   230 */   328,  672,   91,   92,  303,   82,  812,  812,  824,  827,
 /*   240 */   816,  816,   89,   89,   90,   90,   90,   90,  678,   88,
 /*   250 */    88,   88,   88,   87,   87,   86,   86,   86,   85,  328,
 /*   260 */   360,   91,   92,  303,   82,  812,  812,  824,  827,  816,
 /*   270 */   816,   89,   89,   90,   90,   90,   90,  671,   88,   88,
 /*   280 */    88,   88,   87,   87,   86,   86,   86,   85,  328,  931,
 /*   290 */    91,   92,  303,   82,  812,  812,  824,  827,  816,  816,
 /*   300 */    89,   89,   90,   90,   90,   90,  803,   88,   88,   88,
 /*   310 */    88,   87,   87,   86,   86,   86,   85,  328,  428,   91,
 /*   320 */    92,  303,   82,  812,  812,  824,  827,  816,  816,   89,
 /*   330 */    89,   90,   90,   90,   90,  788,   88,   88,   88,   88,
 /*   340 */    87,   87,   86,   86,   86,   85,  328,   91,   92,  303,
 /*   350 */    82,  812,  812,  824,  827,  816,  816,   89,   89,   90,
 /*   360 */    90,   90,   90,  196,   88,   88,   88,   88,   87,   87,
 /*   370 */    86,   86,   86,   85,  328,   91,   92,  303,   82,  812,
 /*   380 */   812,  824,  827,  816,  816,   89,   89,   90,   90,   90,
 /*   390 */    90,  401,   88,   88,   88,   88,   87,   87,   86,   86,
 /*   400 */    86,   85,  328,   85,  328,   91,   92,  303,   82,  812,
 /*   410 */   812,  824,  827,  816,  816,   89,   89,   90,   90,   90,
 /*   420 */    90,  149,   88,   88,   88,   88,   87,   87,   86,   86,
 /*   430 */    86,   85,  328,  233,  391,   91,   80,  303,   82,  812,
 /*   440 */   812,  824,  827,  816,  816,   89,   89,   90,   90,   90,
 /*   450 */    90,   70,   88,   88,   88,   88,   87,   87,   86,   86,
 /*   460 */    86,   85,  328,  167,   92,  303,   82,  812,  812,  824,
 /*   470 */   827,  816,  816,   89,   89,   90,   90,   90,   90,   73,
 /*   480 */    88,   88,   88,   88,   87,   87,   86,   86,   86,   85,
 /*   490 */   328,  303,   82,  812,  812,  824,  827,  816,  816,   89,
 /*   500 */    89,   90,   90,   90,   90,   78,   88,   88,   88,   88,
 /*   510 */    87,   87,   86,   86,   86,   85,  328,  429,   78,   86,
 /*   520 */    86,   86,   85,  328,   75,   76,  920,  920,  745,  186,
 /*   530 */   148,   77,  381,  378,  377,   10,   10,   75,   76,  429,
 /*   540 */   298,  706,  685,  376,   77,  276,  424,    3, 1160,  314,
 /*   550 */   706,  894,  329,  329,  783,  863,  320,   10,   10,  424,
 /*   560 */     3,  124,  783,  427,  429,  329,  329,  236,  921,  922,
 /*   570 */   186,  316,  429,  381,  378,  377,  427,  250,  355,  249,
 /*   580 */   413,  317,   48,   48,  376,  250,  343,  237,  339,  124,
 /*   590 */    48,   48,  802,  413,  431,  430,  233,  391,  789,  783,
 /*   600 */   215,  215,  124,  339,  338,  802,  124,  431,  430,  903,
 /*   610 */  1273,  789,  394, 1273,  373,   78,  190,  407,  392,  429,
 /*   620 */   711,  711,  250,  355,  249,  407,  397,  163,  388,  794,
 /*   630 */   794,  796,  797,   18,   75,   76,  733,   30,   30,   78,
 /*   640 */   745,   77,  794,  794,  796,  797,   18,  203,  160,  267,
 /*   650 */   384,  262,  383,  191,  787,  901,  424,    3,   75,   76,
 /*   660 */   260,  111,  329,  329,  339,   77,  429,  813,  813,  825,
 /*   670 */   828,  124,  389,  427,  358,  927,  745,  429,  885,  219,
 /*   680 */   424,    3,  429,  680,   48,   48,  329,  329,  395,  787,
 /*   690 */   413,  408,  279,  886,  745,   48,   48,  427, 1223, 1223,
 /*   700 */    48,   48,  802,   64,  431,  430,    9,    9,  789,  887,
 /*   710 */   802,  712,  795,  307,  413,  335,  789,  349,  260,  407,
 /*   720 */   406,  713,  709,  709,  111,  315,  802,  429,  431,  430,
 /*   730 */   407,  387,  789,  220,   68,  407,  409,  882,  218,  794,
 /*   740 */   794,  796,  797,   18,  279,   10,   10,  794,  794,  796,
 /*   750 */   721,  140,  311,   75,   76,  429,  920,  920,  817,  224,
 /*   760 */    77,  394,  333,  794,  794,  796,  797,   18,  326,  325,
 /*   770 */   111,  920,  920,   34,   34,  424,    3,  324,  217,  215,
 /*   780 */   215,  329,  329,  293,  292,  291,  206,  289,  364,  429,
 /*   790 */   657,  394,  427,  356,  956,  296,  920,  920,  921,  922,
 /*   800 */   145,  429,  358,  171,  341,  232,   23,   48,   48,  413,
 /*   810 */   255,  354,  429,  921,  922,   54,  148,  162,  161,   10,
 /*   820 */    10,  802,  288,  431,  430,  174,  299,  789,  857, 1249,
 /*   830 */    47,   47,  701,  404,  760,  124,  173,  278,  921,  922,
 /*   840 */    95,  946,  321,  857,  859,  674,  674,   74,  944,   72,
 /*   850 */   945,  893,  246,  759,  254,  347,  882,  304,  794,  794,
 /*   860 */   796,  797,   18,  141,  920,  920,  222,  847,  920,  920,
 /*   870 */   734,   24,  221,  947,  787,  947,  920,  920,  920,  920,
 /*   880 */    84,   81,  169,  277,  336,  802,  891,  795,    1,  111,
 /*   890 */   429,  789,  396,  920,  920,  212,  920,  920,  746,   84,
 /*   900 */    81,  169,  326,  325,  857,  398,  921,  922,   48,   48,
 /*   910 */   921,  922,  356,  692,  242,  689,  745,  148,  921,  922,
 /*   920 */   921,  922,  794,  794,  796,  111,  382,  302,  903, 1274,
 /*   930 */   365,  418, 1274,  748,  693,  921,  922,  312,  921,  922,
 /*   940 */   192,  166,  168,  402,  429,  885,  225,  747,  225,    5,
 /*   950 */   745,  195,  429,  399,  939,  888,  239,  190,  241,  318,
 /*   960 */   886,  429,   35,   35,  124,  429,  266,  745,  746,  429,
 /*   970 */    36,   36,  340,  938,  901,  429,  887,  265,  412,   37,
 /*   980 */    37,  429,  337,   38,   38,  429,  745,   26,   26,  344,
 /*   990 */   426,  426,  426,   27,   27,  688,  939,  429,  310,   29,
 /*  1000 */    29,  429,  780,   39,   39,  429,  718,  947,  240,  947,
 /*  1010 */   429,  745,  429,  761,  429,   40,   40,  855,  429,   41,
 /*  1020 */    41,  429,  762,   11,   11,  714,  228,  429,   42,   42,
 /*  1030 */    97,   97,   43,   43,  429,  111,   44,   44,  429,   31,
 /*  1040 */    31,  429,  158,  429,  924,   45,   45,  429,    7,  429,
 /*  1050 */   245,  429,   46,   46,  307,  429,   32,   32,  429,  113,
 /*  1060 */   113,  114,  114,  429,  787,  115,  115,   52,   52,   33,
 /*  1070 */    33,  429,  386,   98,   98,  429,   49,   49,  429,  364,
 /*  1080 */   429,   99,   99,  429,  364,  429,  924,  429,  746,  100,
 /*  1090 */   100,  429,  787,   96,   96,  429,  112,  112,  110,  110,
 /*  1100 */   429,  104,  104,  103,  103,  101,  101,  429,  899,  102,
 /*  1110 */   102,  429,  195,   51,   51,  939,  429,  346,   53,   53,
 /*  1120 */   869,  869,  350,  880,  866,   50,   50,  313,  865,   25,
 /*  1130 */    25,  362,  168,  210,   28,   28,  327,  327,  327,  647,
 /*  1140 */   648,  649,  348,  295,  958,  295,  902,  189,  188,  187,
 /*  1150 */   664,  308,  717,  425,  308,  322,  111,  939,  746,  729,
 /*  1160 */   661,  415,  419,  248,  728,  423,  309,  125,  223,  214,
 /*  1170 */   166,  729,  109,   20,  153,  898,  728,  785,  297,  363,
 /*  1180 */   194,  357,  681,  359,  194,  111,  194,  251,  216,  959,
 /*  1190 */    66,  959,  854,  111,  111,  111,  374,  258,  111,  199,
 /*  1200 */    66,  691,  690,  698,  699,  726,   19,  798,   69,  755,
 /*  1210 */   850,  687,  194,  199,  862,  861,  862,  861,  666,  877,
 /*  1220 */   876,  107,  367,  368,  681,  253,  656,  256,  702,  686,
 /*  1230 */   261,  739,  753,  411,  854,  786,  735,  410,  280,  281,
 /*  1240 */   793,  669,  663,  654,  353,  653,  209,  655,  914,  798,
 /*  1250 */   150,  269,  294,  271,  273,  775,  247,  879,  361,  342,
 /*  1260 */  1239,  238,  275,  286,  414,  159,  917,  953,  127,  138,
 /*  1270 */   147,  379,  121,   64,  345,  372,  264,  685,  852,  851,
 /*  1280 */   164,  177,  864,  352,   55,  178,  244,  182,  146,  183,
 /*  1290 */   151,  129,  370,  300,  385,  184,  319,  683,  131,  132,
 /*  1300 */   133,  142,  134,  782,  400,  705,  772,  696,   63,  301,
 /*  1310 */     6,  704,  881,  703,  846,  677,  323,   71,   94,  405,
 /*  1320 */   263,  676,  675,   65,  929,  403,  204,  915,   21,  695,
 /*  1330 */   208,  644,  660,  743,  744,  205,  432,  207,  433,  417,
 /*  1340 */   651,  330,  650,  157,  421,  170,  645,  235,  116,  334,
 /*  1350 */   105,  172,  860,  268,  858,  781,  117,  119,  175,  106,
 /*  1360 */   128,  118,  130,  715,  243,  176,  867,  270,  135,  194,
 /*  1370 */   742,  284,  272,  725,  741,  274,  282,  283,  226,  285,
 /*  1380 */   351,  875,  949,  832,  136,  137,   56,  229,  139,   57,
 /*  1390 */    58,   59,  120,  179,  230,  231,  878,  180,  874,    8,
 /*  1400 */    12,  252,  659,  181,  152,  371,  143,  185,   60,  265,
 /*  1410 */   375,   13,  380,  259,  227,   14,   61,  694,  305,  123,
 /*  1420 */   306,  801,  800,  126,  754,  723,  830,   15,  165,  727,
 /*  1430 */    62,    4,  834,  211,  393,  213,  144,  749,  201,   69,
 /*  1440 */   641,  845,   66,  831,  829,  884,  202,  193,  198,  197,
 /*  1450 */   883,   16,  907,   17,  154,  416,  200,  908,  155,  420,
 /*  1460 */   833,  156, 1228,  799,  667,   79, 1241, 1240,  287,  290,
};
static const YYCODETYPE yy_lookahead[] = {
 /*     0 */     5,    6,    7,    8,    9,   10,   11,   12,   13,   14,
 /*    10 */    15,   16,   17,   18,   19,   20,   53,   22,   23,   24,
 /*    20 */    25,   26,   27,   28,   29,   30,   31,   32,   17,   18,
 /*    30 */    19,   20,   21,   22,   23,   24,   25,   26,   27,   28,
 /*    40 */    29,   30,   31,   32,  150,  114,   51,  116,  117,   54,
 /*    50 */     1,    2,    5,    6,    7,    8,    9,   10,   11,   12,
 /*    60 */    13,   14,   15,   16,   17,   18,   19,   20,  105,   22,
 /*    70 */    23,   24,   25,   26,   27,   28,   29,   30,   31,   32,
 /*    80 */    26,   27,   28,   29,   30,   31,   32,   17,   18,   19,
 /*    90 */    20,   96,   22,   23,   24,   25,   26,   27,   28,   29,
 /*   100 */    30,   31,   32,   32,  146,  147,   49,   60,  206,   91,
 /*   110 */   152,   54,  154,  143,  144,  145,   69,  159,  168,  217,
 /*   120 */   218,  219,    5,    6,    7,    8,    9,   10,   11,   12,
 /*   130 */    13,   14,   15,   16,   17,   18,   19,   20,  244,   22,
 /*   140 */    23,   24,   25,   26,   27,   28,   29,   30,   31,   32,
 /*   150 */   192,   54,   55,   96,    5,    6,    7,    8,    9,   10,
 /*   160 */    11,   12,   13,   14,   15,   16,   17,   18,   19,   20,
 /*   170 */    53,   22,   23,   24,   25,   26,   27,   28,   29,   30,
 /*   180 */    31,   32,   22,   23,   24,   25,   26,   27,   28,   29,
 /*   190 */    30,   31,   32,   96,   97,  224,  238,   80,  227,   50,
 /*   200 */   217,  218,  219,    5,    6,    7,    8,    9,   10,   11,
 /*   210 */    12,   13,   14,   15,   16,   17,   18,   19,   20,  168,
 /*   220 */    22,   23,   24,   25,   26,   27,   28,   29,   30,   31,
 /*   230 */    32,  168,    5,    6,    7,    8,    9,   10,   11,   12,
 /*   240 */    13,   14,   15,   16,   17,   18,   19,   20,   50,   22,
 /*   250 */    23,   24,   25,   26,   27,   28,   29,   30,   31,   32,
 /*   260 */   150,    5,    6,    7,    8,    9,   10,   11,   12,   13,
 /*   270 */    14,   15,   16,   17,   18,   19,   20,   50,   22,   23,
 /*   280 */    24,   25,   26,   27,   28,   29,   30,   31,   32,  181,
 /*   290 */     5,    6,    7,    8,    9,   10,   11,   12,   13,   14,
 /*   300 */    15,   16,   17,   18,   19,   20,   50,   22,   23,   24,
 /*   310 */    25,   26,   27,   28,   29,   30,   31,   32,  150,    5,
 /*   320 */     6,    7,    8,    9,   10,   11,   12,   13,   14,   15,
 /*   330 */    16,   17,   18,   19,   20,   50,   22,   23,   24,   25,
 /*   340 */    26,   27,   28,   29,   30,   31,   32,    5,    6,    7,
 /*   350 */     8,    9,   10,   11,   12,   13,   14,   15,   16,   17,
 /*   360 */    18,   19,   20,  150,   22,   23,   24,   25,   26,   27,
 /*   370 */    28,   29,   30,   31,   32,    5,    6,    7,    8,    9,
 /*   380 */    10,   11,   12,   13,   14,   15,   16,   17,   18,   19,
 /*   390 */    20,  150,   22,   23,   24,   25,   26,   27,   28,   29,
 /*   400 */    30,   31,   32,   31,   32,    5,    6,    7,    8,    9,
 /*   410 */    10,   11,   12,   13,   14,   15,   16,   17,   18,   19,
 /*   420 */    20,   51,   22,   23,   24,   25,   26,   27,   28,   29,
 /*   430 */    30,   31,   32,  118,  119,    5,    6,    7,    8,    9,
 /*   440 */    10,   11,   12,   13,   14,   15,   16,   17,   18,   19,
 /*   450 */    20,  137,   22,   23,   24,   25,   26,   27,   28,   29,
 /*   460 */    30,   31,   32,  150,    6,    7,    8,    9,   10,   11,
 /*   470 */    12,   13,   14,   15,   16,   17,   18,   19,   20,  137,
 /*   480 */    22,   23,   24,   25,   26,   27,   28,   29,   30,   31,
 /*   490 */    32,    7,    8,    9,   10,   11,   12,   13,   14,   15,
 /*   500 */    16,   17,   18,   19,   20,    7,   22,   23,   24,   25,
 /*   510 */    26,   27,   28,   29,   30,   31,   32,  150,    7,   28,
 /*   520 */    29,   30,   31,   32,   26,   27,   54,   55,  150,   98,
 /*   530 */   150,   33,  101,  102,  103,  168,  169,   26,   27,  150,
 /*   540 */   160,  175,  176,  112,   33,  150,   48,   49,   50,  182,
 /*   550 */   184,  150,   54,   55,   84,   40,    7,  168,  169,   48,
 /*   560 */    49,   91,   84,   65,  150,   54,   55,  189,   96,   97,
 /*   570 */    98,  182,  150,  101,  102,  103,   65,  107,  108,  109,
 /*   580 */    82,   32,  168,  169,  112,  107,  108,  109,  150,   91,
 /*   590 */   168,  169,   94,   82,   96,   97,  118,  119,  100,   84,
 /*   600 */   190,  191,   91,  165,  166,   94,   91,   96,   97,   49,
 /*   610 */    50,  100,  202,   53,    7,    7,    9,  203,  204,  150,
 /*   620 */   186,  187,  107,  108,  109,  203,  204,   53,  159,  131,
 /*   630 */   132,  133,  134,  135,   26,   27,   28,  168,  169,    7,
 /*   640 */   150,   33,  131,  132,  133,  134,  135,   98,   99,  100,
 /*   650 */   101,  102,  103,  104,  150,   95,   48,   49,   26,   27,
 /*   660 */   111,  192,   54,   55,  226,   33,  150,    9,   10,   11,
 /*   670 */    12,   91,  203,   65,  150,  167,  150,  150,   41,  189,
 /*   680 */    48,   49,  150,  175,  168,  169,   54,   55,  150,  150,
 /*   690 */    82,  159,  150,   56,  150,  168,  169,   65,  118,  119,
 /*   700 */   168,  169,   94,  129,   96,   97,  168,  169,  100,   72,
 /*   710 */    94,   74,   96,  106,   82,  189,  100,  213,  111,  203,
 /*   720 */   204,   84,  186,  187,  192,  183,   94,  150,   96,   97,
 /*   730 */   203,  204,  100,  189,    7,  203,  204,  159,  214,  131,
 /*   740 */   132,  133,  134,  135,  150,  168,  169,  131,  132,  133,
 /*   750 */   191,   49,  213,   26,   27,  150,   54,   55,  100,  182,
 /*   760 */    33,  202,  236,  131,  132,  133,  134,  135,   26,   27,
 /*   770 */   192,   54,   55,  168,  169,   48,   49,  183,   34,  190,
 /*   780 */   191,   54,   55,   39,   40,   41,   42,   43,  150,  150,
 /*   790 */    46,  202,   65,  215,  241,  242,   54,   55,   96,   97,
 /*   800 */    83,  150,  150,   59,  215,  195,  228,  168,  169,   82,
 /*   810 */    45,  233,  150,   96,   97,  205,  150,   26,   27,  168,
 /*   820 */   169,   94,  156,   96,   97,   81,  160,  100,  150,   50,
 /*   830 */   168,  169,   53,  182,  171,   91,   92,  150,   96,   97,
 /*   840 */    49,   99,  203,  165,  166,   54,   55,  136,  106,  138,
 /*   850 */   108,  150,   87,  171,   89,   90,  159,  113,  131,  132,
 /*   860 */   133,  134,  135,   49,   54,   55,  214,  102,   54,   55,
 /*   870 */    28,   49,  234,  131,  150,  133,   54,   55,   54,   55,
 /*   880 */   217,  218,  219,  221,  140,   94,  150,   96,   49,  192,
 /*   890 */   150,  100,  159,   54,   55,   50,   54,   55,   53,  217,
 /*   900 */   218,  219,   26,   27,  226,    7,   96,   97,  168,  169,
 /*   910 */    96,   97,  215,   64,   45,  177,  150,  150,   96,   97,
 /*   920 */    96,   97,  131,  132,  133,  192,   77,  160,   49,   50,
 /*   930 */   233,  244,   53,  123,   85,   96,   97,  213,   96,   97,
 /*   940 */   207,  208,   97,  203,  150,   41,  179,  123,  181,   49,
 /*   950 */   150,   51,  150,   55,   54,  189,   87,    9,   89,  110,
 /*   960 */    56,  150,  168,  169,   91,  150,  100,  150,  123,  150,
 /*   970 */   168,  169,   99,   53,   95,  150,   72,  111,   74,  168,
 /*   980 */   169,  150,  150,  168,  169,  150,  150,  168,  169,  189,
 /*   990 */   164,  165,  166,  168,  169,  177,   96,  150,  150,  168,
 /*  1000 */   169,  150,  159,  168,  169,  150,  189,  131,  139,  133,
 /*  1010 */   150,  150,  150,   60,  150,  168,  169,  150,  150,  168,
 /*  1020 */   169,  150,   69,  168,  169,  189,  206,  150,  168,  169,
 /*  1030 */   168,  169,  168,  169,  150,  192,  168,  169,  150,  168,
 /*  1040 */   169,  150,  122,  150,   54,  168,  169,  150,  194,  150,
 /*  1050 */   189,  150,  168,  169,  106,  150,  168,  169,  150,  168,
 /*  1060 */   169,  168,  169,  150,  150,  168,  169,  168,  169,  168,
 /*  1070 */   169,  150,   28,  168,  169,  150,  168,  169,  150,  150,
 /*  1080 */   150,  168,  169,  150,  150,  150,   96,  150,   53,  168,
 /*  1090 */   169,  150,  150,  168,  169,  150,  168,  169,  168,  169,
 /*  1100 */   150,  168,  169,  168,  169,  168,  169,  150,  150,  168,
 /*  1110 */   169,  150,   51,  168,  169,   54,  150,  150,  168,  169,
 /*  1120 */   107,  108,  109,  159,   58,  168,  169,  213,   62,  168,
 /*  1130 */   169,    7,   97,  150,  168,  169,  164,  165,  166,   36,
 /*  1140 */    37,   38,   76,   49,   50,   49,   50,  107,  108,  109,
 /*  1150 */   162,  163,  159,  162,  163,  213,  192,   96,  123,  115,
 /*  1160 */   159,  159,  159,  234,  120,  159,  239,  240,  234,  207,
 /*  1170 */   208,  115,   49,   16,   51,   50,  120,   50,   53,   55,
 /*  1180 */    53,   50,   54,   50,   53,  192,   53,   50,   49,   95,
 /*  1190 */    53,   95,   54,  192,  192,  192,   50,   50,  192,   53,
 /*  1200 */    53,   99,  100,   36,   37,   50,   49,   54,   53,   50,
 /*  1210 */    50,  177,   53,   53,  131,  131,  133,  133,   50,  150,
 /*  1220 */   150,   53,  150,  150,   96,  150,  150,  150,  150,  150,
 /*  1230 */   150,  209,  150,  187,   96,  150,  150,  150,  150,  150,
 /*  1240 */   150,  150,  150,  150,  230,  150,  229,  150,  150,   96,
 /*  1250 */   193,  206,  148,  206,  206,  197,  235,  197,  235,  210,
 /*  1260 */   121,  210,  210,  196,  223,  194,  153,   66,  237,   49,
 /*  1270 */   216,  172,    5,  129,   47,   47,  171,  176,  171,  171,
 /*  1280 */   180,  155,  232,   73,  136,  155,  231,  155,   49,  155,
 /*  1290 */   216,  185,  173,  173,  106,  155,   75,  105,  188,  188,
 /*  1300 */   188,  185,  188,  185,  124,  170,  197,  178,  106,  173,
 /*  1310 */    49,  170,  197,  170,  197,  170,   32,  136,  128,  125,
 /*  1320 */   170,  172,  170,  127,  170,  126,   52,   42,   53,  178,
 /*  1330 */    35,    4,  158,  212,  212,  151,  157,  151,  149,  173,
 /*  1340 */   149,    3,  149,   49,  173,   44,  149,  141,  161,   93,
 /*  1350 */    45,  106,   50,  211,   50,  119,  161,  110,  106,  174,
 /*  1360 */   130,  161,  122,   48,   45,  124,   79,  211,   79,   53,
 /*  1370 */   212,  198,  211,  201,  212,  211,  200,  199,  174,  197,
 /*  1380 */    71,    1,   86,  220,  106,  122,   16,  222,  130,   16,
 /*  1390 */    16,   16,  110,   63,  225,  225,   55,  121,    1,   34,
 /*  1400 */    49,  139,   48,  106,   51,    7,   49,  104,   49,  111,
 /*  1410 */    78,   49,   78,   50,   78,   49,   49,   57,  243,   67,
 /*  1420 */   243,   50,   50,  240,   55,  115,   50,   49,  121,   50,
 /*  1430 */    53,   49,   40,   50,   53,   50,   49,  123,  121,   53,
 /*  1440 */     1,   50,   53,   50,   50,   50,  121,   63,   49,   53,
 /*  1450 */    50,   63,   50,   63,   49,   51,   53,   50,   49,   51,
 /*  1460 */    50,   49,    0,   50,   50,   49,  121,  121,   50,   44,
};
#define YY_SHIFT_USE_DFLT (1470)
#define YY_SHIFT_COUNT    (435)
#define YY_SHIFT_MIN      (-69)
#define YY_SHIFT_MAX      (1462)
static const short yy_shift_ofst[] = {
 /*     0 */    49,  498,  744,  511,  632,  632,  632,  632,  470,   -5,
 /*    10 */    47,   47,  632,  632,  632,  632,  632,  632,  632,  742,
 /*    20 */   742,  472,  478,  515,  580,  117,  149,  198,  227,  256,
 /*    30 */   285,  314,  342,  370,  400,  400,  400,  400,  400,  400,
 /*    40 */   400,  400,  400,  400,  400,  400,  400,  400,  400,  430,
 /*    50 */   400,  458,  484,  484,  608,  632,  632,  632,  632,  632,
 /*    60 */   632,  632,  632,  632,  632,  632,  632,  632,  632,  632,
 /*    70 */   632,  632,  632,  632,  632,  632,  632,  632,  632,  632,
 /*    80 */   632,  632,  727,  632,  632,  632,  632,  632,  632,  632,
 /*    90 */   632,  632,  632,  632,  632,  632,   11,   70,   70,   70,
 /*   100 */    70,   70,  160,   54,  491,   97,  607,  876,  876,   97,
 /*   110 */   372,  315,   71, 1470, 1470, 1470,  549,  549,  549,  702,
 /*   120 */   702,  637,  765,  637,  717,  560,  879,   97,   97,   97,
 /*   130 */    97,   97,   97,   97,   97,   97,   97,   97,   97,   97,
 /*   140 */    97,   97,   97,   97,   97,   97,   97,  873,  990,  990,
 /*   150 */   315,   18,   18,   18,   18,   18,   18, 1470, 1470, 1470,
 /*   160 */   791,  616,  616,  814,  431,  842,  822,  810,  824,  839,
 /*   170 */    97,   97,   97,   97,   97,   97,   97,   97,   97,   97,
 /*   180 */    97,   97,   97,   97,   97,   97,   97,  849,  849,  849,
 /*   190 */    97,   97,  845,   97,   97,   97,  900,   97,  904,   97,
 /*   200 */    97,   97,   97,   97,   97,   97,   97,   97,   97, 1013,
 /*   210 */  1066, 1061, 1061, 1061, 1035,  -69, 1044, 1103,  574,  898,
 /*   220 */   898, 1124,  574, 1124,  -37,  779,  948,  953,  898,  711,
 /*   230 */   953,  953,  920, 1056, 1123, 1201, 1220, 1267, 1144, 1227,
 /*   240 */  1227, 1227, 1227, 1228, 1148, 1210, 1228, 1144, 1220, 1267,
 /*   250 */  1267, 1144, 1228, 1239, 1228, 1228, 1239, 1188, 1188, 1188,
 /*   260 */  1221, 1239, 1188, 1192, 1188, 1221, 1188, 1188, 1180, 1202,
 /*   270 */  1180, 1202, 1180, 1202, 1180, 1202, 1261, 1181, 1239, 1284,
 /*   280 */  1284, 1239, 1190, 1194, 1196, 1199, 1144, 1274, 1275, 1285,
 /*   290 */  1285, 1295, 1295, 1295, 1295, 1470, 1470, 1470, 1470, 1470,
 /*   300 */  1470, 1470, 1470,  658,  869, 1094, 1096, 1040,   57, 1125,
 /*   310 */  1157, 1127, 1131, 1133, 1137, 1146, 1147, 1128, 1102, 1167,
 /*   320 */   866, 1155, 1159, 1138, 1160, 1083, 1084, 1168, 1153, 1139,
 /*   330 */  1327, 1338, 1294, 1206, 1301, 1256, 1305, 1245, 1302, 1304,
 /*   340 */  1236, 1230, 1247, 1240, 1252, 1315, 1241, 1319, 1287, 1316,
 /*   350 */  1289, 1296, 1309, 1278, 1380, 1263, 1258, 1370, 1373, 1374,
 /*   360 */  1375, 1282, 1341, 1330, 1276, 1397, 1365, 1351, 1297, 1262,
 /*   370 */  1353, 1354, 1398, 1298, 1303, 1357, 1332, 1359, 1362, 1363,
 /*   380 */  1366, 1334, 1360, 1367, 1336, 1352, 1371, 1372, 1376, 1377,
 /*   390 */  1310, 1378, 1379, 1382, 1381, 1307, 1383, 1385, 1369, 1384,
 /*   400 */  1387, 1314, 1386, 1388, 1389, 1390, 1391, 1386, 1393, 1394,
 /*   410 */  1395, 1396, 1400, 1399, 1392, 1402, 1405, 1404, 1403, 1407,
 /*   420 */  1409, 1408, 1403, 1410, 1412, 1413, 1414, 1416, 1317, 1325,
 /*   430 */  1345, 1346, 1418, 1425, 1439, 1462,
};
#define YY_REDUCE_USE_DFLT (-107)
#define YY_REDUCE_COUNT (302)
#define YY_REDUCE_MIN   (-106)
#define YY_REDUCE_MAX   (1204)
static const short yy_reduce_ofst[] = {
 /*     0 */   -30,  532,  -42,  469,  414,  422,  516,  527,  578,  -98,
 /*    10 */   663,  682,  367,  389,  577,  639,  740,  651,  662,  438,
 /*    20 */   678,  767,  589,  697,  733,  -17,  -17,  -17,  -17,  -17,
 /*    30 */   -17,  -17,  -17,  -17,  -17,  -17,  -17,  -17,  -17,  -17,
 /*    40 */   -17,  -17,  -17,  -17,  -17,  -17,  -17,  -17,  -17,  -17,
 /*    50 */   -17,  -17,  -17,  -17,  538,  605,  794,  802,  811,  815,
 /*    60 */   819,  825,  831,  835,  847,  851,  855,  860,  862,  864,
 /*    70 */   868,  871,  877,  884,  888,  891,  893,  897,  899,  901,
 /*    80 */   905,  908,  913,  921,  925,  928,  930,  933,  935,  937,
 /*    90 */   941,  945,  950,  957,  961,  966,  -17,  -17,  -17,  -17,
 /*   100 */   -17,  -17,  -17,  -17,  -17,  526,  366,  826,  972,  666,
 /*   110 */   -17,  410,  -17,  -17,  -17,  -17,  508,  508,  508,  524,
 /*   120 */   652,  434,  -29,  536, -106,  553,  553,  380,  378,  490,
 /*   130 */   544,  766,  800,  817,  836,  504,  861,  638,  539,  929,
 /*   140 */   724,  914,  934,  542,  942,  687,  594,  843,  988,  991,
 /*   150 */   559,  964,  993, 1001, 1002, 1003, 1006,  927,  962,  610,
 /*   160 */   -50,   51,   63,  110,  108,  168,  213,  241,  313,  395,
 /*   170 */   401,  701,  736,  832,  848,  867,  958,  967,  983, 1069,
 /*   180 */  1070, 1072, 1073, 1075, 1076, 1077, 1078,  738,  818, 1034,
 /*   190 */  1079, 1080, 1022, 1082, 1085, 1086,  820, 1087, 1046, 1088,
 /*   200 */  1089, 1090,  168, 1091, 1092, 1093, 1095, 1097, 1098, 1014,
 /*   210 */  1017, 1045, 1047, 1048, 1022, 1057,  854, 1104, 1058, 1049,
 /*   220 */  1051, 1021, 1060, 1023, 1099, 1100, 1101, 1105, 1052, 1041,
 /*   230 */  1107, 1108, 1067, 1071, 1113, 1031, 1054, 1106, 1109, 1110,
 /*   240 */  1111, 1112, 1114, 1126, 1050, 1055, 1130, 1115, 1074, 1116,
 /*   250 */  1118, 1117, 1132, 1119, 1134, 1140, 1120, 1135, 1141, 1143,
 /*   260 */  1129, 1136, 1145, 1149, 1150, 1151, 1152, 1154, 1121, 1142,
 /*   270 */  1122, 1156, 1158, 1161, 1162, 1164, 1163, 1165, 1166, 1169,
 /*   280 */  1170, 1171, 1172, 1176, 1178, 1173, 1182, 1174, 1179, 1184,
 /*   290 */  1186, 1189, 1191, 1193, 1197, 1175, 1177, 1183, 1187, 1195,
 /*   300 */  1185, 1204, 1200,
};
static const YYACTIONTYPE yy_default[] = {
 /*     0 */  1229, 1223, 1223, 1223, 1160, 1160, 1160, 1160, 1223, 1055,
 /*    10 */  1082, 1082, 1279, 1279, 1279, 1279, 1279, 1279, 1159, 1279,
 /*    20 */  1279, 1279, 1279, 1223, 1059, 1088, 1279, 1279, 1279, 1161,
 /*    30 */  1162, 1279, 1279, 1279, 1192, 1098, 1097, 1096, 1095, 1069,
 /*    40 */  1093, 1086, 1090, 1161, 1155, 1156, 1154, 1158, 1162, 1279,
 /*    50 */  1089, 1124, 1139, 1123, 1279, 1279, 1279, 1279, 1279, 1279,
 /*    60 */  1279, 1279, 1279, 1279, 1279, 1279, 1279, 1279, 1279, 1279,
 /*    70 */  1279, 1279, 1279, 1279, 1279, 1279, 1279, 1279, 1279, 1279,
 /*    80 */  1279, 1279, 1279, 1279, 1279, 1279, 1279, 1279, 1279, 1279,
 /*    90 */  1279, 1279, 1279, 1279, 1279, 1279, 1133, 1138, 1145, 1137,
 /*   100 */  1134, 1126, 1125, 1127, 1128, 1279, 1026, 1279, 1279, 1279,
 /*   110 */  1129, 1279, 1130, 1142, 1141, 1140, 1214, 1238, 1237, 1279,
 /*   120 */  1279, 1279, 1167, 1279, 1279, 1279, 1279, 1279, 1279, 1279,
 /*   130 */  1279, 1279, 1279, 1279, 1279, 1279, 1279, 1279, 1279, 1279,
 /*   140 */  1279, 1279, 1279, 1279, 1279, 1279, 1279, 1223,  984,  984,
 /*   150 */  1279, 1223, 1223, 1223, 1223, 1223, 1223, 1219, 1059, 1050,
 /*   160 */  1279, 1279, 1279, 1279, 1279, 1279, 1279, 1279, 1279, 1279,
 /*   170 */  1279, 1211, 1279, 1208, 1279, 1279, 1279, 1279, 1279, 1279,
 /*   180 */  1279, 1279, 1279, 1279, 1279, 1279, 1279, 1279, 1279, 1279,
 /*   190 */  1279, 1279, 1279, 1279, 1279, 1279, 1055, 1279, 1279, 1279,
 /*   200 */  1279, 1279, 1279, 1279, 1279, 1279, 1279, 1279, 1232, 1279,
 /*   210 */  1187, 1055, 1055, 1055, 1057, 1039, 1049,  965, 1092, 1071,
 /*   220 */  1071, 1270, 1092, 1270, 1001, 1252,  998, 1082, 1071, 1157,
 /*   230 */  1082, 1082, 1056, 1049, 1279, 1271, 1103, 1029, 1092, 1035,
 /*   240 */  1035, 1035, 1035,  977, 1191, 1267,  977, 1092, 1103, 1029,
 /*   250 */  1029, 1092,  977, 1168,  977,  977, 1168, 1027, 1027, 1027,
 /*   260 */  1016, 1168, 1027, 1001, 1027, 1016, 1027, 1027, 1075, 1070,
 /*   270 */  1075, 1070, 1075, 1070, 1075, 1070, 1163, 1279, 1168, 1172,
 /*   280 */  1172, 1168, 1087, 1076, 1085, 1083, 1092,  981, 1019, 1235,
 /*   290 */  1235, 1231, 1231, 1231, 1231, 1276, 1276, 1219, 1247, 1247,
 /*   300 */  1003, 1003, 1247, 1279, 1279, 1279, 1279, 1279, 1242, 1279,
 /*   310 */  1175, 1279, 1279, 1279, 1279, 1279, 1279, 1279, 1279, 1279,
 /*   320 */  1279, 1279, 1279, 1279, 1279, 1279, 1279, 1279, 1279, 1109,
 /*   330 */  1279,  962, 1216, 1279, 1279, 1215, 1279, 1209, 1279, 1279,
 /*   340 */  1279, 1279, 1279, 1279, 1279, 1279, 1279, 1279, 1279, 1190,
 /*   350 */  1189, 1279, 1279, 1279, 1279, 1279, 1279, 1279, 1279, 1279,
 /*   360 */  1279, 1279, 1279, 1279, 1269, 1279, 1279, 1279, 1279, 1279,
 /*   370 */  1279, 1279, 1279, 1279, 1279, 1279, 1279, 1279, 1279, 1279,
 /*   380 */  1279, 1279, 1279, 1279, 1279, 1279, 1279, 1279, 1279, 1279,
 /*   390 */  1041, 1279, 1279, 1279, 1256, 1279, 1279, 1279, 1279, 1279,
 /*   400 */  1279, 1279, 1084, 1279, 1077, 1279, 1279, 1260, 1279, 1279,
 /*   410 */  1279, 1279, 1279, 1279, 1279, 1279, 1279, 1279, 1225, 1279,
 /*   420 */  1279, 1279, 1224, 1279, 1279, 1279, 1279, 1279, 1111, 1279,
 /*   430 */  1110, 1114, 1279,  971, 1279, 1279,
};
/********** End of lemon-generated parsing tables *****************************/

/* The next table maps tokens (terminal symbols) into fallback tokens.  
** If a construct like the following:
** 
**      %fallback ID X Y Z.
**
** appears in the grammar, then ID becomes a fallback token for X, Y,
** and Z.  Whenever one of the tokens X, Y, or Z is input to the parser
** but it does not parse, the type of the token is changed to ID and
** the parse is retried before an error is thrown.
**
** This feature can be used, for example, to cause some keywords in a language
** to revert to identifiers if they keyword does not apply in the context where
** it appears.
*/
#ifdef YYFALLBACK
static const YYCODETYPE yyFallback[] = {
    0,  /*          $ => nothing */
    0,  /*       SEMI => nothing */
   54,  /*    EXPLAIN => ID */
   54,  /*      QUERY => ID */
   54,  /*       PLAN => ID */
    0,  /*         OR => nothing */
    0,  /*        AND => nothing */
    0,  /*        NOT => nothing */
    0,  /*         IS => nothing */
   54,  /*      MATCH => ID */
   54,  /*    LIKE_KW => ID */
    0,  /*    BETWEEN => nothing */
    0,  /*         IN => nothing */
    0,  /*     ISNULL => nothing */
    0,  /*    NOTNULL => nothing */
    0,  /*         NE => nothing */
    0,  /*         EQ => nothing */
    0,  /*         GT => nothing */
    0,  /*         LE => nothing */
    0,  /*         LT => nothing */
    0,  /*         GE => nothing */
    0,  /*     ESCAPE => nothing */
    0,  /*     BITAND => nothing */
    0,  /*      BITOR => nothing */
    0,  /*     LSHIFT => nothing */
    0,  /*     RSHIFT => nothing */
    0,  /*       PLUS => nothing */
    0,  /*      MINUS => nothing */
    0,  /*       STAR => nothing */
    0,  /*      SLASH => nothing */
    0,  /*        REM => nothing */
    0,  /*     CONCAT => nothing */
    0,  /*    COLLATE => nothing */
    0,  /*     BITNOT => nothing */
   54,  /*      BEGIN => ID */
    0,  /* TRANSACTION => nothing */
   54,  /*   DEFERRED => ID */
   54,  /*  IMMEDIATE => ID */
   54,  /*  EXCLUSIVE => ID */
    0,  /*     COMMIT => nothing */
   54,  /*        END => ID */
   54,  /*   ROLLBACK => ID */
   54,  /*  SAVEPOINT => ID */
   54,  /*    RELEASE => ID */
    0,  /*         TO => nothing */
    0,  /*      TABLE => nothing */
    0,  /*     CREATE => nothing */
   54,  /*         IF => ID */
    0,  /*     EXISTS => nothing */
    0,  /*         LP => nothing */
    0,  /*         RP => nothing */
    0,  /*         AS => nothing */
   54,  /*    WITHOUT => ID */
    0,  /*      COMMA => nothing */
    0,  /*         ID => nothing */
    0,  /*    INDEXED => nothing */
   54,  /*      ABORT => ID */
   54,  /*     ACTION => ID */
   54,  /*      AFTER => ID */
   54,  /*    ANALYZE => ID */
   54,  /*        ASC => ID */
   54,  /*     ATTACH => ID */
   54,  /*     BEFORE => ID */
   54,  /*         BY => ID */
   54,  /*    CASCADE => ID */
   54,  /*       CAST => ID */
   54,  /*   COLUMNKW => ID */
   54,  /*   CONFLICT => ID */
   54,  /*   DATABASE => ID */
   54,  /*       DESC => ID */
   54,  /*     DETACH => ID */
   54,  /*       EACH => ID */
   54,  /*       FAIL => ID */
   54,  /*        FOR => ID */
   54,  /*     IGNORE => ID */
   54,  /*  INITIALLY => ID */
   54,  /*    INSTEAD => ID */
   54,  /*         NO => ID */
   54,  /*        KEY => ID */
   54,  /*         OF => ID */
   54,  /*     OFFSET => ID */
   54,  /*     PRAGMA => ID */
   54,  /*      RAISE => ID */
   54,  /*  RECURSIVE => ID */
   54,  /*    REPLACE => ID */
   54,  /*   RESTRICT => ID */
   54,  /*        ROW => ID */
   54,  /*    TRIGGER => ID */
   54,  /*     VACUUM => ID */
   54,  /*       VIEW => ID */
   54,  /*    VIRTUAL => ID */
   54,  /*       WITH => ID */
   54,  /*    REINDEX => ID */
   54,  /*     RENAME => ID */
   54,  /*   CTIME_KW => ID */
};
#endif /* YYFALLBACK */

/* The following structure represents a single element of the
** parser's stack.  Information stored includes:
**
**   +  The state number for the parser at this level of the stack.
**
**   +  The value of the token stored at this level of the stack.
**      (In other words, the "major" token.)
**
**   +  The semantic value stored at this level of the stack.  This is
**      the information used by the action routines in the grammar.
**      It is sometimes called the "minor" token.
**
** After the "shift" half of a SHIFTREDUCE action, the stateno field
** actually contains the reduce action for the second half of the
** SHIFTREDUCE.
*/
struct yyStackEntry {
  YYACTIONTYPE stateno;  /* The state-number, or reduce action in SHIFTREDUCE */
  YYCODETYPE major;      /* The major token value.  This is the code
                         ** number for the token at this stack level */
  YYMINORTYPE minor;     /* The user-supplied minor token value.  This
                         ** is the value of the token  */
};
typedef struct yyStackEntry yyStackEntry;

/* The state of the parser is completely contained in an instance of
** the following structure */
struct yyParser {
  yyStackEntry *yytos;          /* Pointer to top element of the stack */
#ifdef YYTRACKMAXSTACKDEPTH
  int yyhwm;                    /* High-water mark of the stack */
#endif
#ifndef YYNOERRORRECOVERY
  int yyerrcnt;                 /* Shifts left before out of the error */
#endif
  sqlite3ParserARG_SDECL                /* A place to hold %extra_argument */
#if YYSTACKDEPTH<=0
  int yystksz;                  /* Current side of the stack */
  yyStackEntry *yystack;        /* The parser's stack */
  yyStackEntry yystk0;          /* First stack entry */
#else
  yyStackEntry yystack[YYSTACKDEPTH];  /* The parser's stack */
#endif
};
typedef struct yyParser yyParser;

#ifndef NDEBUG
#include <stdio.h>
static FILE *yyTraceFILE = 0;
static char *yyTracePrompt = 0;
#endif /* NDEBUG */

#ifndef NDEBUG
/* 
** Turn parser tracing on by giving a stream to which to write the trace
** and a prompt to preface each trace message.  Tracing is turned off
** by making either argument NULL 
**
** Inputs:
** <ul>
** <li> A FILE* to which trace output should be written.
**      If NULL, then tracing is turned off.
** <li> A prefix string written at the beginning of every
**      line of trace output.  If NULL, then tracing is
**      turned off.
** </ul>
**
** Outputs:
** None.
*/
void sqlite3ParserTrace(FILE *TraceFILE, char *zTracePrompt){
  yyTraceFILE = TraceFILE;
  yyTracePrompt = zTracePrompt;
  if( yyTraceFILE==0 ) yyTracePrompt = 0;
  else if( yyTracePrompt==0 ) yyTraceFILE = 0;
}
#endif /* NDEBUG */

#ifndef NDEBUG
/* For tracing shifts, the names of all terminals and nonterminals
** are required.  The following table supplies these names */
static const char *const yyTokenName[] = { 
  "$",             "SEMI",          "EXPLAIN",       "QUERY",       
  "PLAN",          "OR",            "AND",           "NOT",         
  "IS",            "MATCH",         "LIKE_KW",       "BETWEEN",     
  "IN",            "ISNULL",        "NOTNULL",       "NE",          
  "EQ",            "GT",            "LE",            "LT",          
  "GE",            "ESCAPE",        "BITAND",        "BITOR",       
  "LSHIFT",        "RSHIFT",        "PLUS",          "MINUS",       
  "STAR",          "SLASH",         "REM",           "CONCAT",      
  "COLLATE",       "BITNOT",        "BEGIN",         "TRANSACTION", 
  "DEFERRED",      "IMMEDIATE",     "EXCLUSIVE",     "COMMIT",      
  "END",           "ROLLBACK",      "SAVEPOINT",     "RELEASE",     
  "TO",            "TABLE",         "CREATE",        "IF",          
  "EXISTS",        "LP",            "RP",            "AS",          
  "WITHOUT",       "COMMA",         "ID",            "INDEXED",     
  "ABORT",         "ACTION",        "AFTER",         "ANALYZE",     
  "ASC",           "ATTACH",        "BEFORE",        "BY",          
  "CASCADE",       "CAST",          "COLUMNKW",      "CONFLICT",    
  "DATABASE",      "DESC",          "DETACH",        "EACH",        
  "FAIL",          "FOR",           "IGNORE",        "INITIALLY",   
  "INSTEAD",       "NO",            "KEY",           "OF",          
  "OFFSET",        "PRAGMA",        "RAISE",         "RECURSIVE",   
  "REPLACE",       "RESTRICT",      "ROW",           "TRIGGER",     
  "VACUUM",        "VIEW",          "VIRTUAL",       "WITH",        
  "REINDEX",       "RENAME",        "CTIME_KW",      "ANY",         
  "STRING",        "JOIN_KW",       "CONSTRAINT",    "DEFAULT",     
  "NULL",          "PRIMARY",       "UNIQUE",        "CHECK",       
  "REFERENCES",    "AUTOINCR",      "ON",            "INSERT",      
  "DELETE",        "UPDATE",        "SET",           "DEFERRABLE",  
  "FOREIGN",       "DROP",          "UNION",         "ALL",         
  "EXCEPT",        "INTERSECT",     "SELECT",        "VALUES",      
  "DISTINCT",      "DOT",           "FROM",          "JOIN",        
  "USING",         "ORDER",         "GROUP",         "HAVING",      
  "LIMIT",         "WHERE",         "INTO",          "FLOAT",       
  "BLOB",          "INTEGER",       "VARIABLE",      "CASE",        
  "WHEN",          "THEN",          "ELSE",          "INDEX",       
  "ALTER",         "ADD",           "error",         "input",       
  "ecmd",          "explain",       "cmdx",          "cmd",         
  "transtype",     "trans_opt",     "nm",            "savepoint_opt",
  "create_table",  "create_table_args",  "createkw",      "ifnotexists", 
  "columnlist",    "conslist_opt",  "table_options",  "select",      
  "columnname",    "carglist",      "typetoken",     "typename",    
  "signed",        "plus_num",      "minus_num",     "ccons",       
  "term",          "expr",          "onconf",        "sortorder",   
  "autoinc",       "eidlist_opt",   "refargs",       "defer_subclause",
  "refarg",        "refact",        "init_deferred_pred_opt",  "conslist",    
  "tconscomma",    "tcons",         "sortlist",      "eidlist",     
  "defer_subclause_opt",  "orconf",        "resolvetype",   "raisetype",   
  "ifexists",      "fullname",      "selectnowith",  "oneselect",   
  "with",          "multiselect_op",  "distinct",      "selcollist",  
  "from",          "where_opt",     "groupby_opt",   "having_opt",  
  "orderby_opt",   "limit_opt",     "values",        "nexprlist",   
  "exprlist",      "sclp",          "as",            "seltablist",  
  "stl_prefix",    "joinop",        "indexed_opt",   "on_opt",      
  "using_opt",     "idlist",        "setlist",       "insert_cmd",  
  "idlist_opt",    "likeop",        "between_op",    "in_op",       
  "paren_exprlist",  "case_operand",  "case_exprlist",  "case_else",   
  "uniqueflag",    "collate",       "nmnum",         "trigger_decl",
  "trigger_cmd_list",  "trigger_time",  "trigger_event",  "foreach_clause",
  "when_clause",   "trigger_cmd",   "trnm",          "tridxby",     
  "add_column_fullname",  "kwcolumn_opt",  "create_vtab",   "vtabarglist", 
  "vtabarg",       "vtabargtoken",  "lp",            "anylist",     
  "wqlist",      
};
#endif /* NDEBUG */

#ifndef NDEBUG
/* For tracing reduce actions, the names of all rules are required.
*/
static const char *const yyRuleName[] = {
 /*   0 */ "ecmd ::= explain cmdx SEMI",
 /*   1 */ "ecmd ::= SEMI",
 /*   2 */ "explain ::= EXPLAIN",
 /*   3 */ "explain ::= EXPLAIN QUERY PLAN",
 /*   4 */ "cmd ::= BEGIN transtype trans_opt",
 /*   5 */ "transtype ::=",
 /*   6 */ "transtype ::= DEFERRED",
 /*   7 */ "transtype ::= IMMEDIATE",
 /*   8 */ "transtype ::= EXCLUSIVE",
 /*   9 */ "cmd ::= COMMIT trans_opt",
 /*  10 */ "cmd ::= END trans_opt",
 /*  11 */ "cmd ::= ROLLBACK trans_opt",
 /*  12 */ "cmd ::= SAVEPOINT nm",
 /*  13 */ "cmd ::= RELEASE savepoint_opt nm",
 /*  14 */ "cmd ::= ROLLBACK trans_opt TO savepoint_opt nm",
 /*  15 */ "create_table ::= createkw TABLE ifnotexists nm",
 /*  16 */ "createkw ::= CREATE",
 /*  17 */ "ifnotexists ::=",
 /*  18 */ "ifnotexists ::= IF NOT EXISTS",
 /*  19 */ "create_table_args ::= LP columnlist conslist_opt RP table_options",
 /*  20 */ "create_table_args ::= AS select",
 /*  21 */ "table_options ::=",
 /*  22 */ "table_options ::= WITHOUT nm",
 /*  23 */ "columnname ::= nm typetoken",
 /*  24 */ "typetoken ::=",
 /*  25 */ "typetoken ::= typename LP signed RP",
 /*  26 */ "typetoken ::= typename LP signed COMMA signed RP",
 /*  27 */ "typename ::= typename ID|STRING",
 /*  28 */ "ccons ::= CONSTRAINT nm",
 /*  29 */ "ccons ::= DEFAULT term",
 /*  30 */ "ccons ::= DEFAULT LP expr RP",
 /*  31 */ "ccons ::= DEFAULT PLUS term",
 /*  32 */ "ccons ::= DEFAULT MINUS term",
 /*  33 */ "ccons ::= DEFAULT ID|INDEXED",
 /*  34 */ "ccons ::= NOT NULL onconf",
 /*  35 */ "ccons ::= PRIMARY KEY sortorder onconf autoinc",
 /*  36 */ "ccons ::= UNIQUE onconf",
 /*  37 */ "ccons ::= CHECK LP expr RP",
 /*  38 */ "ccons ::= REFERENCES nm eidlist_opt refargs",
 /*  39 */ "ccons ::= defer_subclause",
 /*  40 */ "ccons ::= COLLATE ID|STRING",
 /*  41 */ "autoinc ::=",
 /*  42 */ "autoinc ::= AUTOINCR",
 /*  43 */ "refargs ::=",
 /*  44 */ "refargs ::= refargs refarg",
 /*  45 */ "refarg ::= MATCH nm",
 /*  46 */ "refarg ::= ON INSERT refact",
 /*  47 */ "refarg ::= ON DELETE refact",
 /*  48 */ "refarg ::= ON UPDATE refact",
 /*  49 */ "refact ::= SET NULL",
 /*  50 */ "refact ::= SET DEFAULT",
 /*  51 */ "refact ::= CASCADE",
 /*  52 */ "refact ::= RESTRICT",
 /*  53 */ "refact ::= NO ACTION",
 /*  54 */ "defer_subclause ::= NOT DEFERRABLE init_deferred_pred_opt",
 /*  55 */ "defer_subclause ::= DEFERRABLE init_deferred_pred_opt",
 /*  56 */ "init_deferred_pred_opt ::=",
 /*  57 */ "init_deferred_pred_opt ::= INITIALLY DEFERRED",
 /*  58 */ "init_deferred_pred_opt ::= INITIALLY IMMEDIATE",
 /*  59 */ "conslist_opt ::=",
 /*  60 */ "tconscomma ::= COMMA",
 /*  61 */ "tcons ::= CONSTRAINT nm",
 /*  62 */ "tcons ::= PRIMARY KEY LP sortlist autoinc RP onconf",
 /*  63 */ "tcons ::= UNIQUE LP sortlist RP onconf",
 /*  64 */ "tcons ::= CHECK LP expr RP onconf",
 /*  65 */ "tcons ::= FOREIGN KEY LP eidlist RP REFERENCES nm eidlist_opt refargs defer_subclause_opt",
 /*  66 */ "defer_subclause_opt ::=",
 /*  67 */ "onconf ::=",
 /*  68 */ "onconf ::= ON CONFLICT resolvetype",
 /*  69 */ "orconf ::=",
 /*  70 */ "orconf ::= OR resolvetype",
 /*  71 */ "resolvetype ::= IGNORE",
 /*  72 */ "resolvetype ::= REPLACE",
 /*  73 */ "cmd ::= DROP TABLE ifexists fullname",
 /*  74 */ "ifexists ::= IF EXISTS",
 /*  75 */ "ifexists ::=",
 /*  76 */ "cmd ::= createkw VIEW ifnotexists nm eidlist_opt AS select",
 /*  77 */ "cmd ::= DROP VIEW ifexists fullname",
 /*  78 */ "cmd ::= select",
 /*  79 */ "select ::= with selectnowith",
 /*  80 */ "selectnowith ::= selectnowith multiselect_op oneselect",
 /*  81 */ "multiselect_op ::= UNION",
 /*  82 */ "multiselect_op ::= UNION ALL",
 /*  83 */ "multiselect_op ::= EXCEPT|INTERSECT",
 /*  84 */ "oneselect ::= SELECT distinct selcollist from where_opt groupby_opt having_opt orderby_opt limit_opt",
 /*  85 */ "values ::= VALUES LP nexprlist RP",
 /*  86 */ "values ::= values COMMA LP exprlist RP",
 /*  87 */ "distinct ::= DISTINCT",
 /*  88 */ "distinct ::= ALL",
 /*  89 */ "distinct ::=",
 /*  90 */ "sclp ::=",
 /*  91 */ "selcollist ::= sclp expr as",
 /*  92 */ "selcollist ::= sclp STAR",
 /*  93 */ "selcollist ::= sclp nm DOT STAR",
 /*  94 */ "as ::= AS nm",
 /*  95 */ "as ::=",
 /*  96 */ "from ::=",
 /*  97 */ "from ::= FROM seltablist",
 /*  98 */ "stl_prefix ::= seltablist joinop",
 /*  99 */ "stl_prefix ::=",
 /* 100 */ "seltablist ::= stl_prefix nm as indexed_opt on_opt using_opt",
 /* 101 */ "seltablist ::= stl_prefix nm LP exprlist RP as on_opt using_opt",
 /* 102 */ "seltablist ::= stl_prefix LP select RP as on_opt using_opt",
 /* 103 */ "seltablist ::= stl_prefix LP seltablist RP as on_opt using_opt",
 /* 104 */ "fullname ::= nm",
 /* 105 */ "joinop ::= COMMA|JOIN",
 /* 106 */ "joinop ::= JOIN_KW JOIN",
 /* 107 */ "joinop ::= JOIN_KW nm JOIN",
 /* 108 */ "joinop ::= JOIN_KW nm nm JOIN",
 /* 109 */ "on_opt ::= ON expr",
 /* 110 */ "on_opt ::=",
 /* 111 */ "indexed_opt ::=",
 /* 112 */ "indexed_opt ::= INDEXED BY nm",
 /* 113 */ "indexed_opt ::= NOT INDEXED",
 /* 114 */ "using_opt ::= USING LP idlist RP",
 /* 115 */ "using_opt ::=",
 /* 116 */ "orderby_opt ::=",
 /* 117 */ "orderby_opt ::= ORDER BY sortlist",
 /* 118 */ "sortlist ::= sortlist COMMA expr sortorder",
 /* 119 */ "sortlist ::= expr sortorder",
 /* 120 */ "sortorder ::= ASC",
 /* 121 */ "sortorder ::= DESC",
 /* 122 */ "sortorder ::=",
 /* 123 */ "groupby_opt ::=",
 /* 124 */ "groupby_opt ::= GROUP BY nexprlist",
 /* 125 */ "having_opt ::=",
 /* 126 */ "having_opt ::= HAVING expr",
 /* 127 */ "limit_opt ::=",
 /* 128 */ "limit_opt ::= LIMIT expr",
 /* 129 */ "limit_opt ::= LIMIT expr OFFSET expr",
 /* 130 */ "limit_opt ::= LIMIT expr COMMA expr",
 /* 131 */ "cmd ::= with DELETE FROM fullname indexed_opt where_opt",
 /* 132 */ "where_opt ::=",
 /* 133 */ "where_opt ::= WHERE expr",
 /* 134 */ "cmd ::= with UPDATE orconf fullname indexed_opt SET setlist where_opt",
 /* 135 */ "setlist ::= setlist COMMA nm EQ expr",
 /* 136 */ "setlist ::= setlist COMMA LP idlist RP EQ expr",
 /* 137 */ "setlist ::= nm EQ expr",
 /* 138 */ "setlist ::= LP idlist RP EQ expr",
 /* 139 */ "cmd ::= with insert_cmd INTO fullname idlist_opt select",
 /* 140 */ "cmd ::= with insert_cmd INTO fullname idlist_opt DEFAULT VALUES",
 /* 141 */ "insert_cmd ::= INSERT orconf",
 /* 142 */ "insert_cmd ::= REPLACE",
 /* 143 */ "idlist_opt ::=",
 /* 144 */ "idlist_opt ::= LP idlist RP",
 /* 145 */ "idlist ::= idlist COMMA nm",
 /* 146 */ "idlist ::= nm",
 /* 147 */ "expr ::= LP expr RP",
 /* 148 */ "term ::= NULL",
 /* 149 */ "expr ::= ID|INDEXED",
 /* 150 */ "expr ::= JOIN_KW",
 /* 151 */ "expr ::= nm DOT nm",
 /* 152 */ "expr ::= nm DOT nm DOT nm",
 /* 153 */ "term ::= FLOAT|BLOB",
 /* 154 */ "term ::= STRING",
 /* 155 */ "term ::= INTEGER",
 /* 156 */ "expr ::= VARIABLE",
 /* 157 */ "expr ::= expr COLLATE ID|STRING",
 /* 158 */ "expr ::= CAST LP expr AS typetoken RP",
 /* 159 */ "expr ::= ID|INDEXED LP distinct exprlist RP",
 /* 160 */ "expr ::= ID|INDEXED LP STAR RP",
 /* 161 */ "term ::= CTIME_KW",
 /* 162 */ "expr ::= LP nexprlist COMMA expr RP",
 /* 163 */ "expr ::= expr AND expr",
 /* 164 */ "expr ::= expr OR expr",
 /* 165 */ "expr ::= expr LT|GT|GE|LE expr",
 /* 166 */ "expr ::= expr EQ|NE expr",
 /* 167 */ "expr ::= expr BITAND|BITOR|LSHIFT|RSHIFT expr",
 /* 168 */ "expr ::= expr PLUS|MINUS expr",
 /* 169 */ "expr ::= expr STAR|SLASH|REM expr",
 /* 170 */ "expr ::= expr CONCAT expr",
 /* 171 */ "likeop ::= LIKE_KW|MATCH",
 /* 172 */ "likeop ::= NOT LIKE_KW|MATCH",
 /* 173 */ "expr ::= expr likeop expr",
 /* 174 */ "expr ::= expr likeop expr ESCAPE expr",
 /* 175 */ "expr ::= expr ISNULL|NOTNULL",
 /* 176 */ "expr ::= expr NOT NULL",
 /* 177 */ "expr ::= expr IS expr",
 /* 178 */ "expr ::= expr IS NOT expr",
 /* 179 */ "expr ::= NOT expr",
 /* 180 */ "expr ::= BITNOT expr",
 /* 181 */ "expr ::= MINUS expr",
 /* 182 */ "expr ::= PLUS expr",
 /* 183 */ "between_op ::= BETWEEN",
 /* 184 */ "between_op ::= NOT BETWEEN",
 /* 185 */ "expr ::= expr between_op expr AND expr",
 /* 186 */ "in_op ::= IN",
 /* 187 */ "in_op ::= NOT IN",
 /* 188 */ "expr ::= expr in_op LP exprlist RP",
 /* 189 */ "expr ::= LP select RP",
 /* 190 */ "expr ::= expr in_op LP select RP",
 /* 191 */ "expr ::= expr in_op nm paren_exprlist",
 /* 192 */ "expr ::= EXISTS LP select RP",
 /* 193 */ "expr ::= CASE case_operand case_exprlist case_else END",
 /* 194 */ "case_exprlist ::= case_exprlist WHEN expr THEN expr",
 /* 195 */ "case_exprlist ::= WHEN expr THEN expr",
 /* 196 */ "case_else ::= ELSE expr",
 /* 197 */ "case_else ::=",
 /* 198 */ "case_operand ::= expr",
 /* 199 */ "case_operand ::=",
 /* 200 */ "exprlist ::=",
 /* 201 */ "nexprlist ::= nexprlist COMMA expr",
 /* 202 */ "nexprlist ::= expr",
 /* 203 */ "paren_exprlist ::=",
 /* 204 */ "paren_exprlist ::= LP exprlist RP",
 /* 205 */ "cmd ::= createkw uniqueflag INDEX ifnotexists nm ON nm LP sortlist RP where_opt",
 /* 206 */ "uniqueflag ::= UNIQUE",
 /* 207 */ "uniqueflag ::=",
 /* 208 */ "eidlist_opt ::=",
 /* 209 */ "eidlist_opt ::= LP eidlist RP",
 /* 210 */ "eidlist ::= eidlist COMMA nm collate sortorder",
 /* 211 */ "eidlist ::= nm collate sortorder",
 /* 212 */ "collate ::=",
 /* 213 */ "collate ::= COLLATE ID|STRING",
 /* 214 */ "cmd ::= DROP INDEX ifexists fullname ON nm",
 /* 215 */ "cmd ::= PRAGMA nm",
 /* 216 */ "cmd ::= PRAGMA nm EQ nmnum",
 /* 217 */ "cmd ::= PRAGMA nm LP nmnum RP",
 /* 218 */ "cmd ::= PRAGMA nm EQ minus_num",
 /* 219 */ "cmd ::= PRAGMA nm LP minus_num RP",
 /* 220 */ "plus_num ::= PLUS INTEGER|FLOAT",
 /* 221 */ "minus_num ::= MINUS INTEGER|FLOAT",
 /* 222 */ "cmd ::= createkw trigger_decl BEGIN trigger_cmd_list END",
 /* 223 */ "trigger_decl ::= TRIGGER ifnotexists nm trigger_time trigger_event ON fullname foreach_clause when_clause",
 /* 224 */ "trigger_time ::= BEFORE",
 /* 225 */ "trigger_time ::= AFTER",
 /* 226 */ "trigger_time ::= INSTEAD OF",
 /* 227 */ "trigger_time ::=",
 /* 228 */ "trigger_event ::= DELETE|INSERT",
 /* 229 */ "trigger_event ::= UPDATE",
 /* 230 */ "trigger_event ::= UPDATE OF idlist",
 /* 231 */ "when_clause ::=",
 /* 232 */ "when_clause ::= WHEN expr",
 /* 233 */ "trigger_cmd_list ::= trigger_cmd_list trigger_cmd SEMI",
 /* 234 */ "trigger_cmd_list ::= trigger_cmd SEMI",
 /* 235 */ "trnm ::= nm DOT nm",
 /* 236 */ "tridxby ::= INDEXED BY nm",
 /* 237 */ "tridxby ::= NOT INDEXED",
 /* 238 */ "trigger_cmd ::= UPDATE orconf trnm tridxby SET setlist where_opt",
 /* 239 */ "trigger_cmd ::= insert_cmd INTO trnm idlist_opt select",
 /* 240 */ "trigger_cmd ::= DELETE FROM trnm tridxby where_opt",
 /* 241 */ "trigger_cmd ::= select",
 /* 242 */ "expr ::= RAISE LP IGNORE RP",
 /* 243 */ "expr ::= RAISE LP raisetype COMMA nm RP",
 /* 244 */ "raisetype ::= ROLLBACK",
 /* 245 */ "raisetype ::= ABORT",
 /* 246 */ "raisetype ::= FAIL",
 /* 247 */ "cmd ::= DROP TRIGGER ifexists fullname",
 /* 248 */ "cmd ::= REINDEX",
 /* 249 */ "cmd ::= REINDEX nm",
 /* 250 */ "cmd ::= REINDEX nm ON nm",
 /* 251 */ "cmd ::= ANALYZE",
 /* 252 */ "cmd ::= ANALYZE nm",
 /* 253 */ "cmd ::= ALTER TABLE fullname RENAME TO nm",
 /* 254 */ "cmd ::= ALTER TABLE add_column_fullname ADD kwcolumn_opt columnname carglist",
 /* 255 */ "add_column_fullname ::= fullname",
 /* 256 */ "cmd ::= create_vtab",
 /* 257 */ "cmd ::= create_vtab LP vtabarglist RP",
 /* 258 */ "create_vtab ::= createkw VIRTUAL TABLE ifnotexists nm USING nm",
 /* 259 */ "vtabarg ::=",
 /* 260 */ "vtabargtoken ::= ANY",
 /* 261 */ "vtabargtoken ::= lp anylist RP",
 /* 262 */ "lp ::= LP",
 /* 263 */ "with ::=",
 /* 264 */ "with ::= WITH wqlist",
 /* 265 */ "with ::= WITH RECURSIVE wqlist",
 /* 266 */ "wqlist ::= nm eidlist_opt AS LP select RP",
 /* 267 */ "wqlist ::= wqlist COMMA nm eidlist_opt AS LP select RP",
 /* 268 */ "input ::= ecmd",
 /* 269 */ "explain ::=",
 /* 270 */ "cmdx ::= cmd",
 /* 271 */ "trans_opt ::=",
 /* 272 */ "trans_opt ::= TRANSACTION",
 /* 273 */ "trans_opt ::= TRANSACTION nm",
 /* 274 */ "savepoint_opt ::= SAVEPOINT",
 /* 275 */ "savepoint_opt ::=",
 /* 276 */ "cmd ::= create_table create_table_args",
 /* 277 */ "columnlist ::= columnlist COMMA columnname carglist",
 /* 278 */ "columnlist ::= columnname carglist",
 /* 279 */ "nm ::= ID|INDEXED",
 /* 280 */ "nm ::= STRING",
 /* 281 */ "nm ::= JOIN_KW",
 /* 282 */ "typetoken ::= typename",
 /* 283 */ "typename ::= ID|STRING",
 /* 284 */ "signed ::= plus_num",
 /* 285 */ "signed ::= minus_num",
 /* 286 */ "carglist ::= carglist ccons",
 /* 287 */ "carglist ::=",
 /* 288 */ "ccons ::= NULL onconf",
 /* 289 */ "conslist_opt ::= COMMA conslist",
 /* 290 */ "conslist ::= conslist tconscomma tcons",
 /* 291 */ "conslist ::= tcons",
 /* 292 */ "tconscomma ::=",
 /* 293 */ "defer_subclause_opt ::= defer_subclause",
 /* 294 */ "resolvetype ::= raisetype",
 /* 295 */ "selectnowith ::= oneselect",
 /* 296 */ "oneselect ::= values",
 /* 297 */ "sclp ::= selcollist COMMA",
 /* 298 */ "as ::= ID|STRING",
 /* 299 */ "expr ::= term",
 /* 300 */ "exprlist ::= nexprlist",
 /* 301 */ "nmnum ::= plus_num",
 /* 302 */ "nmnum ::= nm",
 /* 303 */ "nmnum ::= ON",
 /* 304 */ "nmnum ::= DELETE",
 /* 305 */ "nmnum ::= DEFAULT",
 /* 306 */ "plus_num ::= INTEGER|FLOAT",
 /* 307 */ "foreach_clause ::=",
 /* 308 */ "foreach_clause ::= FOR EACH ROW",
 /* 309 */ "trnm ::= nm",
 /* 310 */ "tridxby ::=",
 /* 311 */ "kwcolumn_opt ::=",
 /* 312 */ "kwcolumn_opt ::= COLUMNKW",
 /* 313 */ "vtabarglist ::= vtabarg",
 /* 314 */ "vtabarglist ::= vtabarglist COMMA vtabarg",
 /* 315 */ "vtabarg ::= vtabarg vtabargtoken",
 /* 316 */ "anylist ::=",
 /* 317 */ "anylist ::= anylist LP anylist RP",
 /* 318 */ "anylist ::= anylist ANY",
};
#endif /* NDEBUG */


#if YYSTACKDEPTH<=0
/*
** Try to increase the size of the parser stack.  Return the number
** of errors.  Return 0 on success.
*/
static int yyGrowStack(yyParser *p){
  int newSize;
  int idx;
  yyStackEntry *pNew;

  newSize = p->yystksz*2 + 100;
  idx = p->yytos ? (int)(p->yytos - p->yystack) : 0;
  if( p->yystack==&p->yystk0 ){
    pNew = malloc(newSize*sizeof(pNew[0]));
    if( pNew ) pNew[0] = p->yystk0;
  }else{
    pNew = realloc(p->yystack, newSize*sizeof(pNew[0]));
  }
  if( pNew ){
    p->yystack = pNew;
    p->yytos = &p->yystack[idx];
#ifndef NDEBUG
    if( yyTraceFILE ){
      fprintf(yyTraceFILE,"%sStack grows from %d to %d entries.\n",
              yyTracePrompt, p->yystksz, newSize);
    }
#endif
    p->yystksz = newSize;
  }
  return pNew==0; 
}
#endif

/* Datatype of the argument to the memory allocated passed as the
** second argument to sqlite3ParserAlloc() below.  This can be changed by
** putting an appropriate #define in the %include section of the input
** grammar.
*/
#ifndef YYMALLOCARGTYPE
# define YYMALLOCARGTYPE size_t
#endif

/* 
** This function allocates a new parser.
** The only argument is a pointer to a function which works like
** malloc.
**
** Inputs:
** A pointer to the function used to allocate memory.
**
** Outputs:
** A pointer to a parser.  This pointer is used in subsequent calls
** to sqlite3Parser and sqlite3ParserFree.
*/
void *sqlite3ParserAlloc(void *(*mallocProc)(YYMALLOCARGTYPE)){
  yyParser *pParser;
  pParser = (yyParser*)(*mallocProc)( (YYMALLOCARGTYPE)sizeof(yyParser) );
  if( pParser ){
#ifdef YYTRACKMAXSTACKDEPTH
    pParser->yyhwm = 0;
#endif
#if YYSTACKDEPTH<=0
    pParser->yytos = NULL;
    pParser->yystack = NULL;
    pParser->yystksz = 0;
    if( yyGrowStack(pParser) ){
      pParser->yystack = &pParser->yystk0;
      pParser->yystksz = 1;
    }
#endif
#ifndef YYNOERRORRECOVERY
    pParser->yyerrcnt = -1;
#endif
    pParser->yytos = pParser->yystack;
    pParser->yystack[0].stateno = 0;
    pParser->yystack[0].major = 0;
  }
  return pParser;
}

/* The following function deletes the "minor type" or semantic value
** associated with a symbol.  The symbol can be either a terminal
** or nonterminal. "yymajor" is the symbol code, and "yypminor" is
** a pointer to the value to be deleted.  The code used to do the 
** deletions is derived from the %destructor and/or %token_destructor
** directives of the input grammar.
*/
static void yy_destructor(
  yyParser *yypParser,    /* The parser */
  YYCODETYPE yymajor,     /* Type code for object to destroy */
  YYMINORTYPE *yypminor   /* The object to be destroyed */
){
  sqlite3ParserARG_FETCH;
  switch( yymajor ){
    /* Here is inserted the actions which take place when a
    ** terminal or non-terminal is destroyed.  This can happen
    ** when the symbol is popped from the stack during a
    ** reduce or during error processing or when a parser is 
    ** being destroyed before it is finished parsing.
    **
    ** Note: during a reduce, the only symbols destroyed are those
    ** which appear on the RHS of the rule, but which are *not* used
    ** inside the C code.
    */
/********* Begin destructor definitions ***************************************/
    case 159: /* select */
    case 190: /* selectnowith */
    case 191: /* oneselect */
    case 202: /* values */
{
#line 393 "parse.y"
sqlite3SelectDelete(pParse->db, (yypminor->yy149));
#line 1534 "parse.c"
}
      break;
    case 168: /* term */
    case 169: /* expr */
{
#line 834 "parse.y"
sqlite3ExprDelete(pParse->db, (yypminor->yy132).pExpr);
#line 1542 "parse.c"
}
      break;
    case 173: /* eidlist_opt */
    case 182: /* sortlist */
    case 183: /* eidlist */
    case 195: /* selcollist */
    case 198: /* groupby_opt */
    case 200: /* orderby_opt */
    case 203: /* nexprlist */
    case 204: /* exprlist */
    case 205: /* sclp */
    case 214: /* setlist */
    case 220: /* paren_exprlist */
    case 222: /* case_exprlist */
{
#line 1282 "parse.y"
sqlite3ExprListDelete(pParse->db, (yypminor->yy462));
#line 1560 "parse.c"
}
      break;
    case 189: /* fullname */
    case 196: /* from */
    case 207: /* seltablist */
    case 208: /* stl_prefix */
{
#line 621 "parse.y"
sqlite3SrcListDelete(pParse->db, (yypminor->yy287));
#line 1570 "parse.c"
}
      break;
    case 192: /* with */
    case 244: /* wqlist */
{
#line 1540 "parse.y"
sqlite3WithDelete(pParse->db, (yypminor->yy241));
#line 1578 "parse.c"
}
      break;
    case 197: /* where_opt */
    case 199: /* having_opt */
    case 211: /* on_opt */
    case 221: /* case_operand */
    case 223: /* case_else */
    case 232: /* when_clause */
{
#line 743 "parse.y"
sqlite3ExprDelete(pParse->db, (yypminor->yy342));
#line 1590 "parse.c"
}
      break;
    case 212: /* using_opt */
    case 213: /* idlist */
    case 216: /* idlist_opt */
{
#line 655 "parse.y"
sqlite3IdListDelete(pParse->db, (yypminor->yy440));
#line 1599 "parse.c"
}
      break;
    case 228: /* trigger_cmd_list */
    case 233: /* trigger_cmd */
{
#line 1394 "parse.y"
sqlite3DeleteTriggerStep(pParse->db, (yypminor->yy7));
#line 1607 "parse.c"
}
      break;
    case 230: /* trigger_event */
{
#line 1380 "parse.y"
sqlite3IdListDelete(pParse->db, (yypminor->yy160).b);
#line 1614 "parse.c"
}
      break;
/********* End destructor definitions *****************************************/
    default:  break;   /* If no destructor action specified: do nothing */
  }
}

/*
** Pop the parser's stack once.
**
** If there is a destructor routine associated with the token which
** is popped from the stack, then call it.
*/
static void yy_pop_parser_stack(yyParser *pParser){
  yyStackEntry *yytos;
  assert( pParser->yytos!=0 );
  assert( pParser->yytos > pParser->yystack );
  yytos = pParser->yytos--;
#ifndef NDEBUG
  if( yyTraceFILE ){
    fprintf(yyTraceFILE,"%sPopping %s\n",
      yyTracePrompt,
      yyTokenName[yytos->major]);
  }
#endif
  yy_destructor(pParser, yytos->major, &yytos->minor);
}

/* 
** Deallocate and destroy a parser.  Destructors are called for
** all stack elements before shutting the parser down.
**
** If the YYPARSEFREENEVERNULL macro exists (for example because it
** is defined in a %include section of the input grammar) then it is
** assumed that the input pointer is never NULL.
*/
void sqlite3ParserFree(
  void *p,                    /* The parser to be deleted */
  void (*freeProc)(void*)     /* Function used to reclaim memory */
){
  yyParser *pParser = (yyParser*)p;
#ifndef YYPARSEFREENEVERNULL
  if( pParser==0 ) return;
#endif
  while( pParser->yytos>pParser->yystack ) yy_pop_parser_stack(pParser);
#if YYSTACKDEPTH<=0
  if( pParser->yystack!=&pParser->yystk0 ) free(pParser->yystack);
#endif
  (*freeProc)((void*)pParser);
}

/*
** Return the peak depth of the stack for a parser.
*/
#ifdef YYTRACKMAXSTACKDEPTH
int sqlite3ParserStackPeak(void *p){
  yyParser *pParser = (yyParser*)p;
  return pParser->yyhwm;
}
#endif

/*
** Find the appropriate action for a parser given the terminal
** look-ahead token iLookAhead.
*/
static unsigned int yy_find_shift_action(
  yyParser *pParser,        /* The parser */
  YYCODETYPE iLookAhead     /* The look-ahead token */
){
  int i;
  int stateno = pParser->yytos->stateno;
 
  if( stateno>=YY_MIN_REDUCE ) return stateno;
  assert( stateno <= YY_SHIFT_COUNT );
  do{
    i = yy_shift_ofst[stateno];
    assert( iLookAhead!=YYNOCODE );
    i += iLookAhead;
    if( i<0 || i>=YY_ACTTAB_COUNT || yy_lookahead[i]!=iLookAhead ){
#ifdef YYFALLBACK
      YYCODETYPE iFallback;            /* Fallback token */
      if( iLookAhead<sizeof(yyFallback)/sizeof(yyFallback[0])
             && (iFallback = yyFallback[iLookAhead])!=0 ){
#ifndef NDEBUG
        if( yyTraceFILE ){
          fprintf(yyTraceFILE, "%sFALLBACK %s => %s\n",
             yyTracePrompt, yyTokenName[iLookAhead], yyTokenName[iFallback]);
        }
#endif
        assert( yyFallback[iFallback]==0 ); /* Fallback loop must terminate */
        iLookAhead = iFallback;
        continue;
      }
#endif
#ifdef YYWILDCARD
      {
        int j = i - iLookAhead + YYWILDCARD;
        if( 
#if YY_SHIFT_MIN+YYWILDCARD<0
          j>=0 &&
#endif
#if YY_SHIFT_MAX+YYWILDCARD>=YY_ACTTAB_COUNT
          j<YY_ACTTAB_COUNT &&
#endif
          yy_lookahead[j]==YYWILDCARD && iLookAhead>0
        ){
#ifndef NDEBUG
          if( yyTraceFILE ){
            fprintf(yyTraceFILE, "%sWILDCARD %s => %s\n",
               yyTracePrompt, yyTokenName[iLookAhead],
               yyTokenName[YYWILDCARD]);
          }
#endif /* NDEBUG */
          return yy_action[j];
        }
      }
#endif /* YYWILDCARD */
      return yy_default[stateno];
    }else{
      return yy_action[i];
    }
  }while(1);
}

/*
** Find the appropriate action for a parser given the non-terminal
** look-ahead token iLookAhead.
*/
static int yy_find_reduce_action(
  int stateno,              /* Current state number */
  YYCODETYPE iLookAhead     /* The look-ahead token */
){
  int i;
#ifdef YYERRORSYMBOL
  if( stateno>YY_REDUCE_COUNT ){
    return yy_default[stateno];
  }
#else
  assert( stateno<=YY_REDUCE_COUNT );
#endif
  i = yy_reduce_ofst[stateno];
  assert( i!=YY_REDUCE_USE_DFLT );
  assert( iLookAhead!=YYNOCODE );
  i += iLookAhead;
#ifdef YYERRORSYMBOL
  if( i<0 || i>=YY_ACTTAB_COUNT || yy_lookahead[i]!=iLookAhead ){
    return yy_default[stateno];
  }
#else
  assert( i>=0 && i<YY_ACTTAB_COUNT );
  assert( yy_lookahead[i]==iLookAhead );
#endif
  return yy_action[i];
}

/*
** The following routine is called if the stack overflows.
*/
static void yyStackOverflow(yyParser *yypParser){
   sqlite3ParserARG_FETCH;
#ifndef NDEBUG
   if( yyTraceFILE ){
     fprintf(yyTraceFILE,"%sStack Overflow!\n",yyTracePrompt);
   }
#endif
   while( yypParser->yytos>yypParser->yystack ) yy_pop_parser_stack(yypParser);
   /* Here code is inserted which will execute if the parser
   ** stack every overflows */
/******** Begin %stack_overflow code ******************************************/
#line 37 "parse.y"

  sqlite3ErrorMsg(pParse, "parser stack overflow");
#line 1787 "parse.c"
/******** End %stack_overflow code ********************************************/
   sqlite3ParserARG_STORE; /* Suppress warning about unused %extra_argument var */
}

/*
** Print tracing information for a SHIFT action
*/
#ifndef NDEBUG
static void yyTraceShift(yyParser *yypParser, int yyNewState){
  if( yyTraceFILE ){
    if( yyNewState<YYNSTATE ){
      fprintf(yyTraceFILE,"%sShift '%s', go to state %d\n",
         yyTracePrompt,yyTokenName[yypParser->yytos->major],
         yyNewState);
    }else{
      fprintf(yyTraceFILE,"%sShift '%s'\n",
         yyTracePrompt,yyTokenName[yypParser->yytos->major]);
    }
  }
}
#else
# define yyTraceShift(X,Y)
#endif

/*
** Perform a shift action.
*/
static void yy_shift(
  yyParser *yypParser,          /* The parser to be shifted */
  int yyNewState,               /* The new state to shift in */
  int yyMajor,                  /* The major token to shift in */
  sqlite3ParserTOKENTYPE yyMinor        /* The minor token to shift in */
){
  yyStackEntry *yytos;
  yypParser->yytos++;
#ifdef YYTRACKMAXSTACKDEPTH
  if( (int)(yypParser->yytos - yypParser->yystack)>yypParser->yyhwm ){
    yypParser->yyhwm++;
    assert( yypParser->yyhwm == (int)(yypParser->yytos - yypParser->yystack) );
  }
#endif
#if YYSTACKDEPTH>0 
  if( yypParser->yytos>=&yypParser->yystack[YYSTACKDEPTH] ){
    yypParser->yytos--;
    yyStackOverflow(yypParser);
    return;
  }
#else
  if( yypParser->yytos>=&yypParser->yystack[yypParser->yystksz] ){
    if( yyGrowStack(yypParser) ){
      yypParser->yytos--;
      yyStackOverflow(yypParser);
      return;
    }
  }
#endif
  if( yyNewState > YY_MAX_SHIFT ){
    yyNewState += YY_MIN_REDUCE - YY_MIN_SHIFTREDUCE;
  }
  yytos = yypParser->yytos;
  yytos->stateno = (YYACTIONTYPE)yyNewState;
  yytos->major = (YYCODETYPE)yyMajor;
  yytos->minor.yy0 = yyMinor;
  yyTraceShift(yypParser, yyNewState);
}

/* The following table contains information about every rule that
** is used during the reduce.
*/
static const struct {
  YYCODETYPE lhs;         /* Symbol on the left-hand side of the rule */
  unsigned char nrhs;     /* Number of right-hand side symbols in the rule */
} yyRuleInfo[] = {
  { 144, 3 },
  { 144, 1 },
  { 145, 1 },
  { 145, 3 },
  { 147, 3 },
  { 148, 0 },
  { 148, 1 },
  { 148, 1 },
  { 148, 1 },
  { 147, 2 },
  { 147, 2 },
  { 147, 2 },
  { 147, 2 },
  { 147, 3 },
  { 147, 5 },
  { 152, 4 },
  { 154, 1 },
  { 155, 0 },
  { 155, 3 },
  { 153, 5 },
  { 153, 2 },
  { 158, 0 },
  { 158, 2 },
  { 160, 2 },
  { 162, 0 },
  { 162, 4 },
  { 162, 6 },
  { 163, 2 },
  { 167, 2 },
  { 167, 2 },
  { 167, 4 },
  { 167, 3 },
  { 167, 3 },
  { 167, 2 },
  { 167, 3 },
  { 167, 5 },
  { 167, 2 },
  { 167, 4 },
  { 167, 4 },
  { 167, 1 },
  { 167, 2 },
  { 172, 0 },
  { 172, 1 },
  { 174, 0 },
  { 174, 2 },
  { 176, 2 },
  { 176, 3 },
  { 176, 3 },
  { 176, 3 },
  { 177, 2 },
  { 177, 2 },
  { 177, 1 },
  { 177, 1 },
  { 177, 2 },
  { 175, 3 },
  { 175, 2 },
  { 178, 0 },
  { 178, 2 },
  { 178, 2 },
  { 157, 0 },
  { 180, 1 },
  { 181, 2 },
  { 181, 7 },
  { 181, 5 },
  { 181, 5 },
  { 181, 10 },
  { 184, 0 },
  { 170, 0 },
  { 170, 3 },
  { 185, 0 },
  { 185, 2 },
  { 186, 1 },
  { 186, 1 },
  { 147, 4 },
  { 188, 2 },
  { 188, 0 },
  { 147, 7 },
  { 147, 4 },
  { 147, 1 },
  { 159, 2 },
  { 190, 3 },
  { 193, 1 },
  { 193, 2 },
  { 193, 1 },
  { 191, 9 },
  { 202, 4 },
  { 202, 5 },
  { 194, 1 },
  { 194, 1 },
  { 194, 0 },
  { 205, 0 },
  { 195, 3 },
  { 195, 2 },
  { 195, 4 },
  { 206, 2 },
  { 206, 0 },
  { 196, 0 },
  { 196, 2 },
  { 208, 2 },
  { 208, 0 },
  { 207, 6 },
  { 207, 8 },
  { 207, 7 },
  { 207, 7 },
  { 189, 1 },
  { 209, 1 },
  { 209, 2 },
  { 209, 3 },
  { 209, 4 },
  { 211, 2 },
  { 211, 0 },
  { 210, 0 },
  { 210, 3 },
  { 210, 2 },
  { 212, 4 },
  { 212, 0 },
  { 200, 0 },
  { 200, 3 },
  { 182, 4 },
  { 182, 2 },
  { 171, 1 },
  { 171, 1 },
  { 171, 0 },
  { 198, 0 },
  { 198, 3 },
  { 199, 0 },
  { 199, 2 },
  { 201, 0 },
  { 201, 2 },
  { 201, 4 },
  { 201, 4 },
  { 147, 6 },
  { 197, 0 },
  { 197, 2 },
  { 147, 8 },
  { 214, 5 },
  { 214, 7 },
  { 214, 3 },
  { 214, 5 },
  { 147, 6 },
  { 147, 7 },
  { 215, 2 },
  { 215, 1 },
  { 216, 0 },
  { 216, 3 },
  { 213, 3 },
  { 213, 1 },
  { 169, 3 },
  { 168, 1 },
  { 169, 1 },
  { 169, 1 },
  { 169, 3 },
  { 169, 5 },
  { 168, 1 },
  { 168, 1 },
  { 168, 1 },
  { 169, 1 },
  { 169, 3 },
  { 169, 6 },
  { 169, 5 },
  { 169, 4 },
  { 168, 1 },
  { 169, 5 },
  { 169, 3 },
  { 169, 3 },
  { 169, 3 },
  { 169, 3 },
  { 169, 3 },
  { 169, 3 },
  { 169, 3 },
  { 169, 3 },
  { 217, 1 },
  { 217, 2 },
  { 169, 3 },
  { 169, 5 },
  { 169, 2 },
  { 169, 3 },
  { 169, 3 },
  { 169, 4 },
  { 169, 2 },
  { 169, 2 },
  { 169, 2 },
  { 169, 2 },
  { 218, 1 },
  { 218, 2 },
  { 169, 5 },
  { 219, 1 },
  { 219, 2 },
  { 169, 5 },
  { 169, 3 },
  { 169, 5 },
  { 169, 4 },
  { 169, 4 },
  { 169, 5 },
  { 222, 5 },
  { 222, 4 },
  { 223, 2 },
  { 223, 0 },
  { 221, 1 },
  { 221, 0 },
  { 204, 0 },
  { 203, 3 },
  { 203, 1 },
  { 220, 0 },
  { 220, 3 },
  { 147, 11 },
  { 224, 1 },
  { 224, 0 },
  { 173, 0 },
  { 173, 3 },
  { 183, 5 },
  { 183, 3 },
  { 225, 0 },
  { 225, 2 },
  { 147, 6 },
  { 147, 2 },
  { 147, 4 },
  { 147, 5 },
  { 147, 4 },
  { 147, 5 },
  { 165, 2 },
  { 166, 2 },
  { 147, 5 },
  { 227, 9 },
  { 229, 1 },
  { 229, 1 },
  { 229, 2 },
  { 229, 0 },
  { 230, 1 },
  { 230, 1 },
  { 230, 3 },
  { 232, 0 },
  { 232, 2 },
  { 228, 3 },
  { 228, 2 },
  { 234, 3 },
  { 235, 3 },
  { 235, 2 },
  { 233, 7 },
  { 233, 5 },
  { 233, 5 },
  { 233, 1 },
  { 169, 4 },
  { 169, 6 },
  { 187, 1 },
  { 187, 1 },
  { 187, 1 },
  { 147, 4 },
  { 147, 1 },
  { 147, 2 },
  { 147, 4 },
  { 147, 1 },
  { 147, 2 },
  { 147, 6 },
  { 147, 7 },
  { 236, 1 },
  { 147, 1 },
  { 147, 4 },
  { 238, 7 },
  { 240, 0 },
  { 241, 1 },
  { 241, 3 },
  { 242, 1 },
  { 192, 0 },
  { 192, 2 },
  { 192, 3 },
  { 244, 6 },
  { 244, 8 },
  { 143, 1 },
  { 145, 0 },
  { 146, 1 },
  { 149, 0 },
  { 149, 1 },
  { 149, 2 },
  { 151, 1 },
  { 151, 0 },
  { 147, 2 },
  { 156, 4 },
  { 156, 2 },
  { 150, 1 },
  { 150, 1 },
  { 150, 1 },
  { 162, 1 },
  { 163, 1 },
  { 164, 1 },
  { 164, 1 },
  { 161, 2 },
  { 161, 0 },
  { 167, 2 },
  { 157, 2 },
  { 179, 3 },
  { 179, 1 },
  { 180, 0 },
  { 184, 1 },
  { 186, 1 },
  { 190, 1 },
  { 191, 1 },
  { 205, 2 },
  { 206, 1 },
  { 169, 1 },
  { 204, 1 },
  { 226, 1 },
  { 226, 1 },
  { 226, 1 },
  { 226, 1 },
  { 226, 1 },
  { 165, 1 },
  { 231, 0 },
  { 231, 3 },
  { 234, 1 },
  { 235, 0 },
  { 237, 0 },
  { 237, 1 },
  { 239, 1 },
  { 239, 3 },
  { 240, 2 },
  { 243, 0 },
  { 243, 4 },
  { 243, 2 },
};

static void yy_accept(yyParser*);  /* Forward Declaration */

/*
** Perform a reduce action and the shift that must immediately
** follow the reduce.
*/
static void yy_reduce(
  yyParser *yypParser,         /* The parser */
  unsigned int yyruleno        /* Number of the rule by which to reduce */
){
  int yygoto;                     /* The next state */
  int yyact;                      /* The next action */
  yyStackEntry *yymsp;            /* The top of the parser's stack */
  int yysize;                     /* Amount to pop the stack */
  sqlite3ParserARG_FETCH;
  yymsp = yypParser->yytos;
#ifndef NDEBUG
  if( yyTraceFILE && yyruleno<(int)(sizeof(yyRuleName)/sizeof(yyRuleName[0])) ){
    yysize = yyRuleInfo[yyruleno].nrhs;
    fprintf(yyTraceFILE, "%sReduce [%s], go to state %d.\n", yyTracePrompt,
      yyRuleName[yyruleno], yymsp[-yysize].stateno);
  }
#endif /* NDEBUG */

  /* Check that the stack is large enough to grow by a single entry
  ** if the RHS of the rule is empty.  This ensures that there is room
  ** enough on the stack to push the LHS value */
  if( yyRuleInfo[yyruleno].nrhs==0 ){
#ifdef YYTRACKMAXSTACKDEPTH
    if( (int)(yypParser->yytos - yypParser->yystack)>yypParser->yyhwm ){
      yypParser->yyhwm++;
      assert( yypParser->yyhwm == (int)(yypParser->yytos - yypParser->yystack));
    }
#endif
#if YYSTACKDEPTH>0 
    if( yypParser->yytos>=&yypParser->yystack[YYSTACKDEPTH-1] ){
      yyStackOverflow(yypParser);
      return;
    }
#else
    if( yypParser->yytos>=&yypParser->yystack[yypParser->yystksz-1] ){
      if( yyGrowStack(yypParser) ){
        yyStackOverflow(yypParser);
        return;
      }
      yymsp = yypParser->yytos;
    }
#endif
  }

  switch( yyruleno ){
  /* Beginning here are the reduction cases.  A typical example
  ** follows:
  **   case 0:
  **  #line <lineno> <grammarfile>
  **     { ... }           // User supplied code
  **  #line <lineno> <thisfile>
  **     break;
  */
/********** Begin reduce actions **********************************************/
        YYMINORTYPE yylhsminor;
      case 0: /* ecmd ::= explain cmdx SEMI */
#line 107 "parse.y"
{ sqlite3FinishCoding(pParse); }
#line 2246 "parse.c"
        break;
      case 1: /* ecmd ::= SEMI */
#line 108 "parse.y"
{
  sqlite3ErrorMsg(pParse, "syntax error: empty request");
}
#line 2253 "parse.c"
        break;
      case 2: /* explain ::= EXPLAIN */
#line 113 "parse.y"
{ pParse->explain = 1; }
#line 2258 "parse.c"
        break;
      case 3: /* explain ::= EXPLAIN QUERY PLAN */
#line 114 "parse.y"
{ pParse->explain = 2; }
#line 2263 "parse.c"
        break;
      case 4: /* cmd ::= BEGIN transtype trans_opt */
#line 146 "parse.y"
{sqlite3BeginTransaction(pParse, yymsp[-1].minor.yy32);}
#line 2268 "parse.c"
        break;
      case 5: /* transtype ::= */
#line 151 "parse.y"
{yymsp[1].minor.yy32 = TK_DEFERRED;}
#line 2273 "parse.c"
        break;
      case 6: /* transtype ::= DEFERRED */
      case 7: /* transtype ::= IMMEDIATE */ yytestcase(yyruleno==7);
      case 8: /* transtype ::= EXCLUSIVE */ yytestcase(yyruleno==8);
#line 152 "parse.y"
{yymsp[0].minor.yy32 = yymsp[0].major; /*A-overwrites-X*/}
#line 2280 "parse.c"
        break;
      case 9: /* cmd ::= COMMIT trans_opt */
      case 10: /* cmd ::= END trans_opt */ yytestcase(yyruleno==10);
#line 155 "parse.y"
{sqlite3CommitTransaction(pParse);}
#line 2286 "parse.c"
        break;
      case 11: /* cmd ::= ROLLBACK trans_opt */
#line 157 "parse.y"
{sqlite3RollbackTransaction(pParse);}
#line 2291 "parse.c"
        break;
      case 12: /* cmd ::= SAVEPOINT nm */
#line 161 "parse.y"
{
  sqlite3Savepoint(pParse, SAVEPOINT_BEGIN, &yymsp[0].minor.yy0);
}
#line 2298 "parse.c"
        break;
      case 13: /* cmd ::= RELEASE savepoint_opt nm */
#line 164 "parse.y"
{
  sqlite3Savepoint(pParse, SAVEPOINT_RELEASE, &yymsp[0].minor.yy0);
}
#line 2305 "parse.c"
        break;
      case 14: /* cmd ::= ROLLBACK trans_opt TO savepoint_opt nm */
#line 167 "parse.y"
{
  sqlite3Savepoint(pParse, SAVEPOINT_ROLLBACK, &yymsp[0].minor.yy0);
}
#line 2312 "parse.c"
        break;
      case 15: /* create_table ::= createkw TABLE ifnotexists nm */
#line 174 "parse.y"
{
   sqlite3StartTable(pParse,&yymsp[0].minor.yy0,0,0,0,yymsp[-1].minor.yy32);
}
#line 2319 "parse.c"
        break;
      case 16: /* createkw ::= CREATE */
#line 177 "parse.y"
{disableLookaside(pParse);}
#line 2324 "parse.c"
        break;
      case 17: /* ifnotexists ::= */
      case 21: /* table_options ::= */ yytestcase(yyruleno==21);
      case 41: /* autoinc ::= */ yytestcase(yyruleno==41);
      case 56: /* init_deferred_pred_opt ::= */ yytestcase(yyruleno==56);
      case 66: /* defer_subclause_opt ::= */ yytestcase(yyruleno==66);
      case 75: /* ifexists ::= */ yytestcase(yyruleno==75);
      case 89: /* distinct ::= */ yytestcase(yyruleno==89);
      case 212: /* collate ::= */ yytestcase(yyruleno==212);
#line 180 "parse.y"
{yymsp[1].minor.yy32 = 0;}
#line 2336 "parse.c"
        break;
      case 18: /* ifnotexists ::= IF NOT EXISTS */
#line 181 "parse.y"
{yymsp[-2].minor.yy32 = 1;}
#line 2341 "parse.c"
        break;
      case 19: /* create_table_args ::= LP columnlist conslist_opt RP table_options */
#line 183 "parse.y"
{
  sqlite3EndTable(pParse,&yymsp[-2].minor.yy0,&yymsp[-1].minor.yy0,yymsp[0].minor.yy32,0);
}
#line 2348 "parse.c"
        break;
      case 20: /* create_table_args ::= AS select */
#line 186 "parse.y"
{
  sqlite3EndTable(pParse,0,0,0,yymsp[0].minor.yy149);
  sqlite3SelectDelete(pParse->db, yymsp[0].minor.yy149);
}
#line 2356 "parse.c"
        break;
      case 22: /* table_options ::= WITHOUT nm */
#line 192 "parse.y"
{
  if( yymsp[0].minor.yy0.n==5 && sqlite3_strnicmp(yymsp[0].minor.yy0.z,"rowid",5)==0 ){
    yymsp[-1].minor.yy32 = TF_WithoutRowid | TF_NoVisibleRowid;
  }else{
    yymsp[-1].minor.yy32 = 0;
    sqlite3ErrorMsg(pParse, "unknown table option: %.*s", yymsp[0].minor.yy0.n, yymsp[0].minor.yy0.z);
  }
}
#line 2368 "parse.c"
        break;
      case 23: /* columnname ::= nm typetoken */
#line 202 "parse.y"
{sqlite3AddColumn(pParse,&yymsp[-1].minor.yy0,&yymsp[0].minor.yy0);}
#line 2373 "parse.c"
        break;
      case 24: /* typetoken ::= */
      case 59: /* conslist_opt ::= */ yytestcase(yyruleno==59);
      case 95: /* as ::= */ yytestcase(yyruleno==95);
#line 243 "parse.y"
{yymsp[1].minor.yy0.n = 0; yymsp[1].minor.yy0.z = 0;}
#line 2380 "parse.c"
        break;
      case 25: /* typetoken ::= typename LP signed RP */
#line 245 "parse.y"
{
  yymsp[-3].minor.yy0.n = (int)(&yymsp[0].minor.yy0.z[yymsp[0].minor.yy0.n] - yymsp[-3].minor.yy0.z);
}
#line 2387 "parse.c"
        break;
      case 26: /* typetoken ::= typename LP signed COMMA signed RP */
#line 248 "parse.y"
{
  yymsp[-5].minor.yy0.n = (int)(&yymsp[0].minor.yy0.z[yymsp[0].minor.yy0.n] - yymsp[-5].minor.yy0.z);
}
#line 2394 "parse.c"
        break;
      case 27: /* typename ::= typename ID|STRING */
#line 253 "parse.y"
{yymsp[-1].minor.yy0.n=yymsp[0].minor.yy0.n+(int)(yymsp[0].minor.yy0.z-yymsp[-1].minor.yy0.z);}
#line 2399 "parse.c"
        break;
      case 28: /* ccons ::= CONSTRAINT nm */
      case 61: /* tcons ::= CONSTRAINT nm */ yytestcase(yyruleno==61);
#line 262 "parse.y"
{pParse->constraintName = yymsp[0].minor.yy0;}
#line 2405 "parse.c"
        break;
      case 29: /* ccons ::= DEFAULT term */
      case 31: /* ccons ::= DEFAULT PLUS term */ yytestcase(yyruleno==31);
#line 263 "parse.y"
{sqlite3AddDefaultValue(pParse,&yymsp[0].minor.yy132);}
#line 2411 "parse.c"
        break;
      case 30: /* ccons ::= DEFAULT LP expr RP */
#line 264 "parse.y"
{sqlite3AddDefaultValue(pParse,&yymsp[-1].minor.yy132);}
#line 2416 "parse.c"
        break;
      case 32: /* ccons ::= DEFAULT MINUS term */
#line 266 "parse.y"
{
  ExprSpan v;
  v.pExpr = sqlite3PExpr(pParse, TK_UMINUS, yymsp[0].minor.yy132.pExpr, 0);
  v.zStart = yymsp[-1].minor.yy0.z;
  v.zEnd = yymsp[0].minor.yy132.zEnd;
  sqlite3AddDefaultValue(pParse,&v);
}
#line 2427 "parse.c"
        break;
      case 33: /* ccons ::= DEFAULT ID|INDEXED */
#line 273 "parse.y"
{
  ExprSpan v;
  spanExpr(&v, pParse, TK_STRING, yymsp[0].minor.yy0);
  sqlite3AddDefaultValue(pParse,&v);
}
#line 2436 "parse.c"
        break;
      case 34: /* ccons ::= NOT NULL onconf */
#line 283 "parse.y"
{sqlite3AddNotNull(pParse, yymsp[0].minor.yy32);}
#line 2441 "parse.c"
        break;
      case 35: /* ccons ::= PRIMARY KEY sortorder onconf autoinc */
#line 285 "parse.y"
{sqlite3AddPrimaryKey(pParse,0,yymsp[-1].minor.yy32,yymsp[0].minor.yy32,yymsp[-2].minor.yy32);}
#line 2446 "parse.c"
        break;
      case 36: /* ccons ::= UNIQUE onconf */
#line 286 "parse.y"
{sqlite3CreateIndex(pParse,0,0,0,yymsp[0].minor.yy32,0,0,0,0,
                                   SQLITE_IDXTYPE_UNIQUE);}
#line 2452 "parse.c"
        break;
      case 37: /* ccons ::= CHECK LP expr RP */
#line 288 "parse.y"
{sqlite3AddCheckConstraint(pParse,yymsp[-1].minor.yy132.pExpr);}
#line 2457 "parse.c"
        break;
      case 38: /* ccons ::= REFERENCES nm eidlist_opt refargs */
#line 290 "parse.y"
{sqlite3CreateForeignKey(pParse,0,&yymsp[-2].minor.yy0,yymsp[-1].minor.yy462,yymsp[0].minor.yy32);}
#line 2462 "parse.c"
        break;
      case 39: /* ccons ::= defer_subclause */
#line 291 "parse.y"
{sqlite3DeferForeignKey(pParse,yymsp[0].minor.yy32);}
#line 2467 "parse.c"
        break;
      case 40: /* ccons ::= COLLATE ID|STRING */
#line 292 "parse.y"
{sqlite3AddCollateType(pParse, &yymsp[0].minor.yy0);}
#line 2472 "parse.c"
        break;
      case 42: /* autoinc ::= AUTOINCR */
#line 297 "parse.y"
{yymsp[0].minor.yy32 = 1;}
#line 2477 "parse.c"
        break;
      case 43: /* refargs ::= */
#line 305 "parse.y"
{ yymsp[1].minor.yy32 = OE_None*0x0101; /* EV: R-19803-45884 */}
#line 2482 "parse.c"
        break;
      case 44: /* refargs ::= refargs refarg */
#line 306 "parse.y"
{ yymsp[-1].minor.yy32 = (yymsp[-1].minor.yy32 & ~yymsp[0].minor.yy47.mask) | yymsp[0].minor.yy47.value; }
#line 2487 "parse.c"
        break;
      case 45: /* refarg ::= MATCH nm */
#line 308 "parse.y"
{ yymsp[-1].minor.yy47.value = 0;     yymsp[-1].minor.yy47.mask = 0x000000; }
#line 2492 "parse.c"
        break;
      case 46: /* refarg ::= ON INSERT refact */
#line 309 "parse.y"
{ yymsp[-2].minor.yy47.value = 0;     yymsp[-2].minor.yy47.mask = 0x000000; }
#line 2497 "parse.c"
        break;
      case 47: /* refarg ::= ON DELETE refact */
#line 310 "parse.y"
{ yymsp[-2].minor.yy47.value = yymsp[0].minor.yy32;     yymsp[-2].minor.yy47.mask = 0x0000ff; }
#line 2502 "parse.c"
        break;
      case 48: /* refarg ::= ON UPDATE refact */
#line 311 "parse.y"
{ yymsp[-2].minor.yy47.value = yymsp[0].minor.yy32<<8;  yymsp[-2].minor.yy47.mask = 0x00ff00; }
#line 2507 "parse.c"
        break;
      case 49: /* refact ::= SET NULL */
#line 313 "parse.y"
{ yymsp[-1].minor.yy32 = OE_SetNull;  /* EV: R-33326-45252 */}
#line 2512 "parse.c"
        break;
      case 50: /* refact ::= SET DEFAULT */
#line 314 "parse.y"
{ yymsp[-1].minor.yy32 = OE_SetDflt;  /* EV: R-33326-45252 */}
#line 2517 "parse.c"
        break;
      case 51: /* refact ::= CASCADE */
#line 315 "parse.y"
{ yymsp[0].minor.yy32 = OE_Cascade;  /* EV: R-33326-45252 */}
#line 2522 "parse.c"
        break;
      case 52: /* refact ::= RESTRICT */
#line 316 "parse.y"
{ yymsp[0].minor.yy32 = OE_Restrict; /* EV: R-33326-45252 */}
#line 2527 "parse.c"
        break;
      case 53: /* refact ::= NO ACTION */
#line 317 "parse.y"
{ yymsp[-1].minor.yy32 = OE_None;     /* EV: R-33326-45252 */}
#line 2532 "parse.c"
        break;
      case 54: /* defer_subclause ::= NOT DEFERRABLE init_deferred_pred_opt */
#line 319 "parse.y"
{yymsp[-2].minor.yy32 = 0;}
#line 2537 "parse.c"
        break;
      case 55: /* defer_subclause ::= DEFERRABLE init_deferred_pred_opt */
      case 70: /* orconf ::= OR resolvetype */ yytestcase(yyruleno==70);
      case 141: /* insert_cmd ::= INSERT orconf */ yytestcase(yyruleno==141);
#line 320 "parse.y"
{yymsp[-1].minor.yy32 = yymsp[0].minor.yy32;}
#line 2544 "parse.c"
        break;
      case 57: /* init_deferred_pred_opt ::= INITIALLY DEFERRED */
      case 74: /* ifexists ::= IF EXISTS */ yytestcase(yyruleno==74);
      case 184: /* between_op ::= NOT BETWEEN */ yytestcase(yyruleno==184);
      case 187: /* in_op ::= NOT IN */ yytestcase(yyruleno==187);
      case 213: /* collate ::= COLLATE ID|STRING */ yytestcase(yyruleno==213);
#line 323 "parse.y"
{yymsp[-1].minor.yy32 = 1;}
#line 2553 "parse.c"
        break;
      case 58: /* init_deferred_pred_opt ::= INITIALLY IMMEDIATE */
#line 324 "parse.y"
{yymsp[-1].minor.yy32 = 0;}
#line 2558 "parse.c"
        break;
      case 60: /* tconscomma ::= COMMA */
#line 330 "parse.y"
{pParse->constraintName.n = 0;}
#line 2563 "parse.c"
        break;
      case 62: /* tcons ::= PRIMARY KEY LP sortlist autoinc RP onconf */
#line 334 "parse.y"
{sqlite3AddPrimaryKey(pParse,yymsp[-3].minor.yy462,yymsp[0].minor.yy32,yymsp[-2].minor.yy32,0);}
#line 2568 "parse.c"
        break;
      case 63: /* tcons ::= UNIQUE LP sortlist RP onconf */
#line 336 "parse.y"
{sqlite3CreateIndex(pParse,0,0,yymsp[-2].minor.yy462,yymsp[0].minor.yy32,0,0,0,0,
                                       SQLITE_IDXTYPE_UNIQUE);}
#line 2574 "parse.c"
        break;
      case 64: /* tcons ::= CHECK LP expr RP onconf */
#line 339 "parse.y"
{sqlite3AddCheckConstraint(pParse,yymsp[-2].minor.yy132.pExpr);}
#line 2579 "parse.c"
        break;
      case 65: /* tcons ::= FOREIGN KEY LP eidlist RP REFERENCES nm eidlist_opt refargs defer_subclause_opt */
#line 341 "parse.y"
{
    sqlite3CreateForeignKey(pParse, yymsp[-6].minor.yy462, &yymsp[-3].minor.yy0, yymsp[-2].minor.yy462, yymsp[-1].minor.yy32);
    sqlite3DeferForeignKey(pParse, yymsp[0].minor.yy32);
}
#line 2587 "parse.c"
        break;
      case 67: /* onconf ::= */
      case 69: /* orconf ::= */ yytestcase(yyruleno==69);
#line 355 "parse.y"
{yymsp[1].minor.yy32 = OE_Default;}
#line 2593 "parse.c"
        break;
      case 68: /* onconf ::= ON CONFLICT resolvetype */
#line 356 "parse.y"
{yymsp[-2].minor.yy32 = yymsp[0].minor.yy32;}
#line 2598 "parse.c"
        break;
      case 71: /* resolvetype ::= IGNORE */
#line 360 "parse.y"
{yymsp[0].minor.yy32 = OE_Ignore;}
#line 2603 "parse.c"
        break;
      case 72: /* resolvetype ::= REPLACE */
      case 142: /* insert_cmd ::= REPLACE */ yytestcase(yyruleno==142);
#line 361 "parse.y"
{yymsp[0].minor.yy32 = OE_Replace;}
#line 2609 "parse.c"
        break;
      case 73: /* cmd ::= DROP TABLE ifexists fullname */
#line 365 "parse.y"
{
  sqlite3DropTable(pParse, yymsp[0].minor.yy287, 0, yymsp[-1].minor.yy32);
}
#line 2616 "parse.c"
        break;
      case 76: /* cmd ::= createkw VIEW ifnotexists nm eidlist_opt AS select */
#line 376 "parse.y"
{
  sqlite3CreateView(pParse, &yymsp[-6].minor.yy0, &yymsp[-3].minor.yy0, yymsp[-2].minor.yy462, yymsp[0].minor.yy149, yymsp[-4].minor.yy32);
}
#line 2623 "parse.c"
        break;
      case 77: /* cmd ::= DROP VIEW ifexists fullname */
#line 379 "parse.y"
{
  sqlite3DropTable(pParse, yymsp[0].minor.yy287, 1, yymsp[-1].minor.yy32);
}
#line 2630 "parse.c"
        break;
      case 78: /* cmd ::= select */
#line 386 "parse.y"
{
  SelectDest dest = {SRT_Output, 0, 0, 0, 0, 0};
  sqlite3Select(pParse, yymsp[0].minor.yy149, &dest);
  sqlite3SelectDelete(pParse->db, yymsp[0].minor.yy149);
}
#line 2639 "parse.c"
        break;
      case 79: /* select ::= with selectnowith */
#line 423 "parse.y"
{
  Select *p = yymsp[0].minor.yy149;
  if( p ){
    p->pWith = yymsp[-1].minor.yy241;
    parserDoubleLinkSelect(pParse, p);
  }else{
    sqlite3WithDelete(pParse->db, yymsp[-1].minor.yy241);
  }
  yymsp[-1].minor.yy149 = p; /*A-overwrites-W*/
}
#line 2653 "parse.c"
        break;
      case 80: /* selectnowith ::= selectnowith multiselect_op oneselect */
#line 436 "parse.y"
{
  Select *pRhs = yymsp[0].minor.yy149;
  Select *pLhs = yymsp[-2].minor.yy149;
  if( pRhs && pRhs->pPrior ){
    SrcList *pFrom;
    Token x;
    x.n = 0;
    parserDoubleLinkSelect(pParse, pRhs);
    pFrom = sqlite3SrcListAppendFromTerm(pParse,0,0,0,&x,pRhs,0,0);
    pRhs = sqlite3SelectNew(pParse,0,pFrom,0,0,0,0,0,0,0);
  }
  if( pRhs ){
    pRhs->op = (u8)yymsp[-1].minor.yy32;
    pRhs->pPrior = pLhs;
    if( ALWAYS(pLhs) ) pLhs->selFlags &= ~SF_MultiValue;
    pRhs->selFlags &= ~SF_MultiValue;
    if( yymsp[-1].minor.yy32!=TK_ALL ) pParse->hasCompound = 1;
  }else{
    sqlite3SelectDelete(pParse->db, pLhs);
  }
  yymsp[-2].minor.yy149 = pRhs;
}
#line 2679 "parse.c"
        break;
      case 81: /* multiselect_op ::= UNION */
      case 83: /* multiselect_op ::= EXCEPT|INTERSECT */ yytestcase(yyruleno==83);
#line 459 "parse.y"
{yymsp[0].minor.yy32 = yymsp[0].major; /*A-overwrites-OP*/}
#line 2685 "parse.c"
        break;
      case 82: /* multiselect_op ::= UNION ALL */
#line 460 "parse.y"
{yymsp[-1].minor.yy32 = TK_ALL;}
#line 2690 "parse.c"
        break;
      case 84: /* oneselect ::= SELECT distinct selcollist from where_opt groupby_opt having_opt orderby_opt limit_opt */
#line 464 "parse.y"
{
#if SELECTTRACE_ENABLED
  Token s = yymsp[-8].minor.yy0; /*A-overwrites-S*/
#endif
  yymsp[-8].minor.yy149 = sqlite3SelectNew(pParse,yymsp[-6].minor.yy462,yymsp[-5].minor.yy287,yymsp[-4].minor.yy342,yymsp[-3].minor.yy462,yymsp[-2].minor.yy342,yymsp[-1].minor.yy462,yymsp[-7].minor.yy32,yymsp[0].minor.yy474.pLimit,yymsp[0].minor.yy474.pOffset);
#if SELECTTRACE_ENABLED
  /* Populate the Select.zSelName[] string that is used to help with
  ** query planner debugging, to differentiate between multiple Select
  ** objects in a complex query.
  **
  ** If the SELECT keyword is immediately followed by a C-style comment
  ** then extract the first few alphanumeric characters from within that
  ** comment to be the zSelName value.  Otherwise, the label is #N where
  ** is an integer that is incremented with each SELECT statement seen.
  */
  if( yymsp[-8].minor.yy149!=0 ){
    const char *z = s.z+6;
    int i;
    sqlite3_snprintf(sizeof(yymsp[-8].minor.yy149->zSelName), yymsp[-8].minor.yy149->zSelName, "#%d",
                     ++pParse->nSelect);
    while( z[0]==' ' ) z++;
    if( z[0]=='/' && z[1]=='*' ){
      z += 2;
      while( z[0]==' ' ) z++;
      for(i=0; sqlite3Isalnum(z[i]); i++){}
      sqlite3_snprintf(sizeof(yymsp[-8].minor.yy149->zSelName), yymsp[-8].minor.yy149->zSelName, "%.*s", i, z);
    }
  }
#endif /* SELECTRACE_ENABLED */
}
#line 2724 "parse.c"
        break;
      case 85: /* values ::= VALUES LP nexprlist RP */
#line 498 "parse.y"
{
  yymsp[-3].minor.yy149 = sqlite3SelectNew(pParse,yymsp[-1].minor.yy462,0,0,0,0,0,SF_Values,0,0);
}
#line 2731 "parse.c"
        break;
      case 86: /* values ::= values COMMA LP exprlist RP */
#line 501 "parse.y"
{
  Select *pRight, *pLeft = yymsp[-4].minor.yy149;
  pRight = sqlite3SelectNew(pParse,yymsp[-1].minor.yy462,0,0,0,0,0,SF_Values|SF_MultiValue,0,0);
  if( ALWAYS(pLeft) ) pLeft->selFlags &= ~SF_MultiValue;
  if( pRight ){
    pRight->op = TK_ALL;
    pRight->pPrior = pLeft;
    yymsp[-4].minor.yy149 = pRight;
  }else{
    yymsp[-4].minor.yy149 = pLeft;
  }
}
#line 2747 "parse.c"
        break;
      case 87: /* distinct ::= DISTINCT */
#line 518 "parse.y"
{yymsp[0].minor.yy32 = SF_Distinct;}
#line 2752 "parse.c"
        break;
      case 88: /* distinct ::= ALL */
#line 519 "parse.y"
{yymsp[0].minor.yy32 = SF_All;}
#line 2757 "parse.c"
        break;
      case 90: /* sclp ::= */
      case 116: /* orderby_opt ::= */ yytestcase(yyruleno==116);
      case 123: /* groupby_opt ::= */ yytestcase(yyruleno==123);
      case 200: /* exprlist ::= */ yytestcase(yyruleno==200);
      case 203: /* paren_exprlist ::= */ yytestcase(yyruleno==203);
      case 208: /* eidlist_opt ::= */ yytestcase(yyruleno==208);
#line 532 "parse.y"
{yymsp[1].minor.yy462 = 0;}
#line 2767 "parse.c"
        break;
      case 91: /* selcollist ::= sclp expr as */
#line 533 "parse.y"
{
   yymsp[-2].minor.yy462 = sqlite3ExprListAppend(pParse, yymsp[-2].minor.yy462, yymsp[-1].minor.yy132.pExpr);
   if( yymsp[0].minor.yy0.n>0 ) sqlite3ExprListSetName(pParse, yymsp[-2].minor.yy462, &yymsp[0].minor.yy0, 1);
   sqlite3ExprListSetSpan(pParse,yymsp[-2].minor.yy462,&yymsp[-1].minor.yy132);
}
#line 2776 "parse.c"
        break;
      case 92: /* selcollist ::= sclp STAR */
#line 538 "parse.y"
{
  Expr *p = sqlite3Expr(pParse->db, TK_ASTERISK, 0);
  yymsp[-1].minor.yy462 = sqlite3ExprListAppend(pParse, yymsp[-1].minor.yy462, p);
}
#line 2784 "parse.c"
        break;
      case 93: /* selcollist ::= sclp nm DOT STAR */
#line 542 "parse.y"
{
  Expr *pRight = sqlite3PExpr(pParse, TK_ASTERISK, 0, 0);
  Expr *pLeft = sqlite3ExprAlloc(pParse->db, TK_ID, &yymsp[-2].minor.yy0, 1);
  Expr *pDot = sqlite3PExpr(pParse, TK_DOT, pLeft, pRight);
  yymsp[-3].minor.yy462 = sqlite3ExprListAppend(pParse,yymsp[-3].minor.yy462, pDot);
}
#line 2794 "parse.c"
        break;
      case 94: /* as ::= AS nm */
      case 220: /* plus_num ::= PLUS INTEGER|FLOAT */ yytestcase(yyruleno==220);
      case 221: /* minus_num ::= MINUS INTEGER|FLOAT */ yytestcase(yyruleno==221);
#line 553 "parse.y"
{yymsp[-1].minor.yy0 = yymsp[0].minor.yy0;}
#line 2801 "parse.c"
        break;
      case 96: /* from ::= */
#line 567 "parse.y"
{yymsp[1].minor.yy287 = sqlite3DbMallocZero(pParse->db, sizeof(*yymsp[1].minor.yy287));}
#line 2806 "parse.c"
        break;
      case 97: /* from ::= FROM seltablist */
#line 568 "parse.y"
{
  yymsp[-1].minor.yy287 = yymsp[0].minor.yy287;
  sqlite3SrcListShiftJoinType(yymsp[-1].minor.yy287);
}
#line 2814 "parse.c"
        break;
      case 98: /* stl_prefix ::= seltablist joinop */
#line 576 "parse.y"
{
   if( ALWAYS(yymsp[-1].minor.yy287 && yymsp[-1].minor.yy287->nSrc>0) ) yymsp[-1].minor.yy287->a[yymsp[-1].minor.yy287->nSrc-1].fg.jointype = (u8)yymsp[0].minor.yy32;
}
#line 2821 "parse.c"
        break;
      case 99: /* stl_prefix ::= */
#line 579 "parse.y"
{yymsp[1].minor.yy287 = 0;}
#line 2826 "parse.c"
        break;
      case 100: /* seltablist ::= stl_prefix nm as indexed_opt on_opt using_opt */
#line 581 "parse.y"
{
  yymsp[-5].minor.yy287 = sqlite3SrcListAppendFromTerm(pParse,yymsp[-5].minor.yy287,&yymsp[-4].minor.yy0,0,&yymsp[-3].minor.yy0,0,yymsp[-1].minor.yy342,yymsp[0].minor.yy440);
  sqlite3SrcListIndexedBy(pParse, yymsp[-5].minor.yy287, &yymsp[-2].minor.yy0);
}
#line 2834 "parse.c"
        break;
      case 101: /* seltablist ::= stl_prefix nm LP exprlist RP as on_opt using_opt */
#line 586 "parse.y"
{
  yymsp[-7].minor.yy287 = sqlite3SrcListAppendFromTerm(pParse,yymsp[-7].minor.yy287,&yymsp[-6].minor.yy0,0,&yymsp[-2].minor.yy0,0,yymsp[-1].minor.yy342,yymsp[0].minor.yy440);
  sqlite3SrcListFuncArgs(pParse, yymsp[-7].minor.yy287, yymsp[-4].minor.yy462);
}
#line 2842 "parse.c"
        break;
      case 102: /* seltablist ::= stl_prefix LP select RP as on_opt using_opt */
#line 592 "parse.y"
{
    yymsp[-6].minor.yy287 = sqlite3SrcListAppendFromTerm(pParse,yymsp[-6].minor.yy287,0,0,&yymsp[-2].minor.yy0,yymsp[-4].minor.yy149,yymsp[-1].minor.yy342,yymsp[0].minor.yy440);
  }
#line 2849 "parse.c"
        break;
      case 103: /* seltablist ::= stl_prefix LP seltablist RP as on_opt using_opt */
#line 596 "parse.y"
{
    if( yymsp[-6].minor.yy287==0 && yymsp[-2].minor.yy0.n==0 && yymsp[-1].minor.yy342==0 && yymsp[0].minor.yy440==0 ){
      yymsp[-6].minor.yy287 = yymsp[-4].minor.yy287;
    }else if( yymsp[-4].minor.yy287->nSrc==1 ){
      yymsp[-6].minor.yy287 = sqlite3SrcListAppendFromTerm(pParse,yymsp[-6].minor.yy287,0,0,&yymsp[-2].minor.yy0,0,yymsp[-1].minor.yy342,yymsp[0].minor.yy440);
      if( yymsp[-6].minor.yy287 ){
        struct SrcList_item *pNew = &yymsp[-6].minor.yy287->a[yymsp[-6].minor.yy287->nSrc-1];
        struct SrcList_item *pOld = yymsp[-4].minor.yy287->a;
        pNew->zName = pOld->zName;
        pNew->zDatabase = pOld->zDatabase;
        pNew->pSelect = pOld->pSelect;
        pOld->zName = pOld->zDatabase = 0;
        pOld->pSelect = 0;
      }
      sqlite3SrcListDelete(pParse->db, yymsp[-4].minor.yy287);
    }else{
      Select *pSubquery;
      sqlite3SrcListShiftJoinType(yymsp[-4].minor.yy287);
      pSubquery = sqlite3SelectNew(pParse,0,yymsp[-4].minor.yy287,0,0,0,0,SF_NestedFrom,0,0);
      yymsp[-6].minor.yy287 = sqlite3SrcListAppendFromTerm(pParse,yymsp[-6].minor.yy287,0,0,&yymsp[-2].minor.yy0,pSubquery,yymsp[-1].minor.yy342,yymsp[0].minor.yy440);
    }
  }
#line 2875 "parse.c"
        break;
      case 104: /* fullname ::= nm */
#line 623 "parse.y"
{yymsp[0].minor.yy287 = sqlite3SrcListAppend(pParse->db,0,&yymsp[0].minor.yy0,0); /*A-overwrites-X*/}
#line 2880 "parse.c"
        break;
      case 105: /* joinop ::= COMMA|JOIN */
#line 626 "parse.y"
{ yymsp[0].minor.yy32 = JT_INNER; }
#line 2885 "parse.c"
        break;
      case 106: /* joinop ::= JOIN_KW JOIN */
#line 628 "parse.y"
{yymsp[-1].minor.yy32 = sqlite3JoinType(pParse,&yymsp[-1].minor.yy0,0,0);  /*X-overwrites-A*/}
#line 2890 "parse.c"
        break;
      case 107: /* joinop ::= JOIN_KW nm JOIN */
#line 630 "parse.y"
{yymsp[-2].minor.yy32 = sqlite3JoinType(pParse,&yymsp[-2].minor.yy0,&yymsp[-1].minor.yy0,0); /*X-overwrites-A*/}
#line 2895 "parse.c"
        break;
      case 108: /* joinop ::= JOIN_KW nm nm JOIN */
#line 632 "parse.y"
{yymsp[-3].minor.yy32 = sqlite3JoinType(pParse,&yymsp[-3].minor.yy0,&yymsp[-2].minor.yy0,&yymsp[-1].minor.yy0);/*X-overwrites-A*/}
#line 2900 "parse.c"
        break;
      case 109: /* on_opt ::= ON expr */
      case 126: /* having_opt ::= HAVING expr */ yytestcase(yyruleno==126);
      case 133: /* where_opt ::= WHERE expr */ yytestcase(yyruleno==133);
      case 196: /* case_else ::= ELSE expr */ yytestcase(yyruleno==196);
#line 636 "parse.y"
{yymsp[-1].minor.yy342 = yymsp[0].minor.yy132.pExpr;}
#line 2908 "parse.c"
        break;
      case 110: /* on_opt ::= */
      case 125: /* having_opt ::= */ yytestcase(yyruleno==125);
      case 132: /* where_opt ::= */ yytestcase(yyruleno==132);
      case 197: /* case_else ::= */ yytestcase(yyruleno==197);
      case 199: /* case_operand ::= */ yytestcase(yyruleno==199);
#line 637 "parse.y"
{yymsp[1].minor.yy342 = 0;}
#line 2917 "parse.c"
        break;
      case 111: /* indexed_opt ::= */
#line 650 "parse.y"
{yymsp[1].minor.yy0.z=0; yymsp[1].minor.yy0.n=0;}
#line 2922 "parse.c"
        break;
      case 112: /* indexed_opt ::= INDEXED BY nm */
#line 651 "parse.y"
{yymsp[-2].minor.yy0 = yymsp[0].minor.yy0;}
#line 2927 "parse.c"
        break;
      case 113: /* indexed_opt ::= NOT INDEXED */
#line 652 "parse.y"
{yymsp[-1].minor.yy0.z=0; yymsp[-1].minor.yy0.n=1;}
#line 2932 "parse.c"
        break;
      case 114: /* using_opt ::= USING LP idlist RP */
#line 656 "parse.y"
{yymsp[-3].minor.yy440 = yymsp[-1].minor.yy440;}
#line 2937 "parse.c"
        break;
      case 115: /* using_opt ::= */
      case 143: /* idlist_opt ::= */ yytestcase(yyruleno==143);
#line 657 "parse.y"
{yymsp[1].minor.yy440 = 0;}
#line 2943 "parse.c"
        break;
      case 117: /* orderby_opt ::= ORDER BY sortlist */
      case 124: /* groupby_opt ::= GROUP BY nexprlist */ yytestcase(yyruleno==124);
#line 671 "parse.y"
{yymsp[-2].minor.yy462 = yymsp[0].minor.yy462;}
#line 2949 "parse.c"
        break;
      case 118: /* sortlist ::= sortlist COMMA expr sortorder */
#line 672 "parse.y"
{
  yymsp[-3].minor.yy462 = sqlite3ExprListAppend(pParse,yymsp[-3].minor.yy462,yymsp[-1].minor.yy132.pExpr);
  sqlite3ExprListSetSortOrder(yymsp[-3].minor.yy462,yymsp[0].minor.yy32);
}
#line 2957 "parse.c"
        break;
      case 119: /* sortlist ::= expr sortorder */
#line 676 "parse.y"
{
  yymsp[-1].minor.yy462 = sqlite3ExprListAppend(pParse,0,yymsp[-1].minor.yy132.pExpr); /*A-overwrites-Y*/
  sqlite3ExprListSetSortOrder(yymsp[-1].minor.yy462,yymsp[0].minor.yy32);
}
#line 2965 "parse.c"
        break;
      case 120: /* sortorder ::= ASC */
#line 683 "parse.y"
{yymsp[0].minor.yy32 = SQLITE_SO_ASC;}
#line 2970 "parse.c"
        break;
      case 121: /* sortorder ::= DESC */
#line 684 "parse.y"
{yymsp[0].minor.yy32 = SQLITE_SO_DESC;}
#line 2975 "parse.c"
        break;
      case 122: /* sortorder ::= */
#line 685 "parse.y"
{yymsp[1].minor.yy32 = SQLITE_SO_UNDEFINED;}
#line 2980 "parse.c"
        break;
      case 127: /* limit_opt ::= */
#line 710 "parse.y"
{yymsp[1].minor.yy474.pLimit = 0; yymsp[1].minor.yy474.pOffset = 0;}
#line 2985 "parse.c"
        break;
      case 128: /* limit_opt ::= LIMIT expr */
#line 711 "parse.y"
{yymsp[-1].minor.yy474.pLimit = yymsp[0].minor.yy132.pExpr; yymsp[-1].minor.yy474.pOffset = 0;}
#line 2990 "parse.c"
        break;
      case 129: /* limit_opt ::= LIMIT expr OFFSET expr */
#line 713 "parse.y"
{yymsp[-3].minor.yy474.pLimit = yymsp[-2].minor.yy132.pExpr; yymsp[-3].minor.yy474.pOffset = yymsp[0].minor.yy132.pExpr;}
#line 2995 "parse.c"
        break;
      case 130: /* limit_opt ::= LIMIT expr COMMA expr */
#line 715 "parse.y"
{yymsp[-3].minor.yy474.pOffset = yymsp[-2].minor.yy132.pExpr; yymsp[-3].minor.yy474.pLimit = yymsp[0].minor.yy132.pExpr;}
#line 3000 "parse.c"
        break;
      case 131: /* cmd ::= with DELETE FROM fullname indexed_opt where_opt */
#line 732 "parse.y"
{
  sqlite3WithPush(pParse, yymsp[-5].minor.yy241, 1);
  sqlite3SrcListIndexedBy(pParse, yymsp[-2].minor.yy287, &yymsp[-1].minor.yy0);
  sqlSubProgramsRemaining = SQL_MAX_COMPILING_TRIGGERS;
  /* Instruct SQL to initate Tarantool's transaction.  */
  pParse->initiateTTrans = true;
  sqlite3DeleteFrom(pParse,yymsp[-2].minor.yy287,yymsp[0].minor.yy342);
}
#line 3012 "parse.c"
        break;
      case 134: /* cmd ::= with UPDATE orconf fullname indexed_opt SET setlist where_opt */
#line 765 "parse.y"
{
  sqlite3WithPush(pParse, yymsp[-7].minor.yy241, 1);
  sqlite3SrcListIndexedBy(pParse, yymsp[-4].minor.yy287, &yymsp[-3].minor.yy0);
  sqlite3ExprListCheckLength(pParse,yymsp[-1].minor.yy462,"set list"); 
  sqlSubProgramsRemaining = SQL_MAX_COMPILING_TRIGGERS;
  /* Instruct SQL to initate Tarantool's transaction.  */
  pParse->initiateTTrans = true;
  sqlite3Update(pParse,yymsp[-4].minor.yy287,yymsp[-1].minor.yy462,yymsp[0].minor.yy342,yymsp[-5].minor.yy32);
}
#line 3025 "parse.c"
        break;
      case 135: /* setlist ::= setlist COMMA nm EQ expr */
#line 779 "parse.y"
{
  yymsp[-4].minor.yy462 = sqlite3ExprListAppend(pParse, yymsp[-4].minor.yy462, yymsp[0].minor.yy132.pExpr);
  sqlite3ExprListSetName(pParse, yymsp[-4].minor.yy462, &yymsp[-2].minor.yy0, 1);
}
#line 3033 "parse.c"
        break;
      case 136: /* setlist ::= setlist COMMA LP idlist RP EQ expr */
#line 783 "parse.y"
{
  yymsp[-6].minor.yy462 = sqlite3ExprListAppendVector(pParse, yymsp[-6].minor.yy462, yymsp[-3].minor.yy440, yymsp[0].minor.yy132.pExpr);
}
#line 3040 "parse.c"
        break;
      case 137: /* setlist ::= nm EQ expr */
#line 786 "parse.y"
{
  yylhsminor.yy462 = sqlite3ExprListAppend(pParse, 0, yymsp[0].minor.yy132.pExpr);
  sqlite3ExprListSetName(pParse, yylhsminor.yy462, &yymsp[-2].minor.yy0, 1);
}
#line 3048 "parse.c"
  yymsp[-2].minor.yy462 = yylhsminor.yy462;
        break;
      case 138: /* setlist ::= LP idlist RP EQ expr */
#line 790 "parse.y"
{
  yymsp[-4].minor.yy462 = sqlite3ExprListAppendVector(pParse, 0, yymsp[-3].minor.yy440, yymsp[0].minor.yy132.pExpr);
}
#line 3056 "parse.c"
        break;
      case 139: /* cmd ::= with insert_cmd INTO fullname idlist_opt select */
#line 796 "parse.y"
{
  sqlite3WithPush(pParse, yymsp[-5].minor.yy241, 1);
  sqlSubProgramsRemaining = SQL_MAX_COMPILING_TRIGGERS;
  /* Instruct SQL to initate Tarantool's transaction.  */
  pParse->initiateTTrans = true;
  sqlite3Insert(pParse, yymsp[-2].minor.yy287, yymsp[0].minor.yy149, yymsp[-1].minor.yy440, yymsp[-4].minor.yy32);
}
#line 3067 "parse.c"
        break;
      case 140: /* cmd ::= with insert_cmd INTO fullname idlist_opt DEFAULT VALUES */
#line 804 "parse.y"
{
  sqlite3WithPush(pParse, yymsp[-6].minor.yy241, 1);
  sqlSubProgramsRemaining = SQL_MAX_COMPILING_TRIGGERS;
  /* Instruct SQL to initate Tarantool's transaction.  */
  pParse->initiateTTrans = true;
  sqlite3Insert(pParse, yymsp[-3].minor.yy287, 0, yymsp[-2].minor.yy440, yymsp[-5].minor.yy32);
}
#line 3078 "parse.c"
        break;
      case 144: /* idlist_opt ::= LP idlist RP */
#line 822 "parse.y"
{yymsp[-2].minor.yy440 = yymsp[-1].minor.yy440;}
#line 3083 "parse.c"
        break;
      case 145: /* idlist ::= idlist COMMA nm */
#line 824 "parse.y"
{yymsp[-2].minor.yy440 = sqlite3IdListAppend(pParse->db,yymsp[-2].minor.yy440,&yymsp[0].minor.yy0);}
#line 3088 "parse.c"
        break;
      case 146: /* idlist ::= nm */
#line 826 "parse.y"
{yymsp[0].minor.yy440 = sqlite3IdListAppend(pParse->db,0,&yymsp[0].minor.yy0); /*A-overwrites-Y*/}
#line 3093 "parse.c"
        break;
      case 147: /* expr ::= LP expr RP */
#line 876 "parse.y"
{spanSet(&yymsp[-2].minor.yy132,&yymsp[-2].minor.yy0,&yymsp[0].minor.yy0); /*A-overwrites-B*/  yymsp[-2].minor.yy132.pExpr = yymsp[-1].minor.yy132.pExpr;}
#line 3098 "parse.c"
        break;
      case 148: /* term ::= NULL */
      case 153: /* term ::= FLOAT|BLOB */ yytestcase(yyruleno==153);
      case 154: /* term ::= STRING */ yytestcase(yyruleno==154);
#line 877 "parse.y"
{spanExpr(&yymsp[0].minor.yy132,pParse,yymsp[0].major,yymsp[0].minor.yy0);/*A-overwrites-X*/}
#line 3105 "parse.c"
        break;
      case 149: /* expr ::= ID|INDEXED */
      case 150: /* expr ::= JOIN_KW */ yytestcase(yyruleno==150);
#line 878 "parse.y"
{spanExpr(&yymsp[0].minor.yy132,pParse,TK_ID,yymsp[0].minor.yy0); /*A-overwrites-X*/}
#line 3111 "parse.c"
        break;
      case 151: /* expr ::= nm DOT nm */
#line 880 "parse.y"
{
  Expr *temp1 = sqlite3ExprAlloc(pParse->db, TK_ID, &yymsp[-2].minor.yy0, 1);
  Expr *temp2 = sqlite3ExprAlloc(pParse->db, TK_ID, &yymsp[0].minor.yy0, 1);
  spanSet(&yymsp[-2].minor.yy132,&yymsp[-2].minor.yy0,&yymsp[0].minor.yy0); /*A-overwrites-X*/
  yymsp[-2].minor.yy132.pExpr = sqlite3PExpr(pParse, TK_DOT, temp1, temp2);
}
#line 3121 "parse.c"
        break;
      case 152: /* expr ::= nm DOT nm DOT nm */
#line 886 "parse.y"
{
  Expr *temp1 = sqlite3ExprAlloc(pParse->db, TK_ID, &yymsp[-4].minor.yy0, 1);
  Expr *temp2 = sqlite3ExprAlloc(pParse->db, TK_ID, &yymsp[-2].minor.yy0, 1);
  Expr *temp3 = sqlite3ExprAlloc(pParse->db, TK_ID, &yymsp[0].minor.yy0, 1);
  Expr *temp4 = sqlite3PExpr(pParse, TK_DOT, temp2, temp3);
  spanSet(&yymsp[-4].minor.yy132,&yymsp[-4].minor.yy0,&yymsp[0].minor.yy0); /*A-overwrites-X*/
  yymsp[-4].minor.yy132.pExpr = sqlite3PExpr(pParse, TK_DOT, temp1, temp4);
}
#line 3133 "parse.c"
        break;
      case 155: /* term ::= INTEGER */
#line 896 "parse.y"
{
  yylhsminor.yy132.pExpr = sqlite3ExprAlloc(pParse->db, TK_INTEGER, &yymsp[0].minor.yy0, 1);
  yylhsminor.yy132.zStart = yymsp[0].minor.yy0.z;
  yylhsminor.yy132.zEnd = yymsp[0].minor.yy0.z + yymsp[0].minor.yy0.n;
  if( yylhsminor.yy132.pExpr ) yylhsminor.yy132.pExpr->flags |= EP_Leaf;
}
#line 3143 "parse.c"
  yymsp[0].minor.yy132 = yylhsminor.yy132;
        break;
      case 156: /* expr ::= VARIABLE */
#line 902 "parse.y"
{
  if( !(yymsp[0].minor.yy0.z[0]=='#' && sqlite3Isdigit(yymsp[0].minor.yy0.z[1])) ){
    u32 n = yymsp[0].minor.yy0.n;
    spanExpr(&yymsp[0].minor.yy132, pParse, TK_VARIABLE, yymsp[0].minor.yy0);
    sqlite3ExprAssignVarNumber(pParse, yymsp[0].minor.yy132.pExpr, n);
  }else{
    /* When doing a nested parse, one can include terms in an expression
    ** that look like this:   #1 #2 ...  These terms refer to registers
    ** in the virtual machine.  #N is the N-th register. */
    Token t = yymsp[0].minor.yy0; /*A-overwrites-X*/
    assert( t.n>=2 );
    spanSet(&yymsp[0].minor.yy132, &t, &t);
    if( pParse->nested==0 ){
      sqlite3ErrorMsg(pParse, "near \"%T\": syntax error", &t);
      yymsp[0].minor.yy132.pExpr = 0;
    }else{
      yymsp[0].minor.yy132.pExpr = sqlite3PExpr(pParse, TK_REGISTER, 0, 0);
      if( yymsp[0].minor.yy132.pExpr ) sqlite3GetInt32(&t.z[1], &yymsp[0].minor.yy132.pExpr->iTable);
    }
  }
}
#line 3169 "parse.c"
        break;
      case 157: /* expr ::= expr COLLATE ID|STRING */
#line 923 "parse.y"
{
  yymsp[-2].minor.yy132.pExpr = sqlite3ExprAddCollateToken(pParse, yymsp[-2].minor.yy132.pExpr, &yymsp[0].minor.yy0, 1);
  yymsp[-2].minor.yy132.zEnd = &yymsp[0].minor.yy0.z[yymsp[0].minor.yy0.n];
}
#line 3177 "parse.c"
        break;
      case 158: /* expr ::= CAST LP expr AS typetoken RP */
#line 928 "parse.y"
{
  spanSet(&yymsp[-5].minor.yy132,&yymsp[-5].minor.yy0,&yymsp[0].minor.yy0); /*A-overwrites-X*/
  yymsp[-5].minor.yy132.pExpr = sqlite3ExprAlloc(pParse->db, TK_CAST, &yymsp[-1].minor.yy0, 1);
  sqlite3ExprAttachSubtrees(pParse->db, yymsp[-5].minor.yy132.pExpr, yymsp[-3].minor.yy132.pExpr, 0);
}
#line 3186 "parse.c"
        break;
      case 159: /* expr ::= ID|INDEXED LP distinct exprlist RP */
#line 934 "parse.y"
{
  if( yymsp[-1].minor.yy462 && yymsp[-1].minor.yy462->nExpr>pParse->db->aLimit[SQLITE_LIMIT_FUNCTION_ARG] ){
    sqlite3ErrorMsg(pParse, "too many arguments on function %T", &yymsp[-4].minor.yy0);
  }
  yylhsminor.yy132.pExpr = sqlite3ExprFunction(pParse, yymsp[-1].minor.yy462, &yymsp[-4].minor.yy0);
  spanSet(&yylhsminor.yy132,&yymsp[-4].minor.yy0,&yymsp[0].minor.yy0);
  if( yymsp[-2].minor.yy32==SF_Distinct && yylhsminor.yy132.pExpr ){
    yylhsminor.yy132.pExpr->flags |= EP_Distinct;
  }
}
#line 3200 "parse.c"
  yymsp[-4].minor.yy132 = yylhsminor.yy132;
        break;
      case 160: /* expr ::= ID|INDEXED LP STAR RP */
#line 944 "parse.y"
{
  yylhsminor.yy132.pExpr = sqlite3ExprFunction(pParse, 0, &yymsp[-3].minor.yy0);
  spanSet(&yylhsminor.yy132,&yymsp[-3].minor.yy0,&yymsp[0].minor.yy0);
}
#line 3209 "parse.c"
  yymsp[-3].minor.yy132 = yylhsminor.yy132;
        break;
      case 161: /* term ::= CTIME_KW */
#line 948 "parse.y"
{
  yylhsminor.yy132.pExpr = sqlite3ExprFunction(pParse, 0, &yymsp[0].minor.yy0);
  spanSet(&yylhsminor.yy132, &yymsp[0].minor.yy0, &yymsp[0].minor.yy0);
}
#line 3218 "parse.c"
  yymsp[0].minor.yy132 = yylhsminor.yy132;
        break;
      case 162: /* expr ::= LP nexprlist COMMA expr RP */
#line 977 "parse.y"
{
  ExprList *pList = sqlite3ExprListAppend(pParse, yymsp[-3].minor.yy462, yymsp[-1].minor.yy132.pExpr);
  yylhsminor.yy132.pExpr = sqlite3PExpr(pParse, TK_VECTOR, 0, 0);
  if( yylhsminor.yy132.pExpr ){
    yylhsminor.yy132.pExpr->x.pList = pList;
    spanSet(&yylhsminor.yy132, &yymsp[-4].minor.yy0, &yymsp[0].minor.yy0);
  }else{
    sqlite3ExprListDelete(pParse->db, pList);
  }
}
#line 3233 "parse.c"
  yymsp[-4].minor.yy132 = yylhsminor.yy132;
        break;
      case 163: /* expr ::= expr AND expr */
      case 164: /* expr ::= expr OR expr */ yytestcase(yyruleno==164);
      case 165: /* expr ::= expr LT|GT|GE|LE expr */ yytestcase(yyruleno==165);
      case 166: /* expr ::= expr EQ|NE expr */ yytestcase(yyruleno==166);
      case 167: /* expr ::= expr BITAND|BITOR|LSHIFT|RSHIFT expr */ yytestcase(yyruleno==167);
      case 168: /* expr ::= expr PLUS|MINUS expr */ yytestcase(yyruleno==168);
      case 169: /* expr ::= expr STAR|SLASH|REM expr */ yytestcase(yyruleno==169);
      case 170: /* expr ::= expr CONCAT expr */ yytestcase(yyruleno==170);
#line 988 "parse.y"
{spanBinaryExpr(pParse,yymsp[-1].major,&yymsp[-2].minor.yy132,&yymsp[0].minor.yy132);}
#line 3246 "parse.c"
        break;
      case 171: /* likeop ::= LIKE_KW|MATCH */
#line 1001 "parse.y"
{yymsp[0].minor.yy0=yymsp[0].minor.yy0;/*A-overwrites-X*/}
#line 3251 "parse.c"
        break;
      case 172: /* likeop ::= NOT LIKE_KW|MATCH */
#line 1002 "parse.y"
{yymsp[-1].minor.yy0=yymsp[0].minor.yy0; yymsp[-1].minor.yy0.n|=0x80000000; /*yymsp[-1].minor.yy0-overwrite-yymsp[0].minor.yy0*/}
#line 3256 "parse.c"
        break;
      case 173: /* expr ::= expr likeop expr */
#line 1003 "parse.y"
{
  ExprList *pList;
  int bNot = yymsp[-1].minor.yy0.n & 0x80000000;
  yymsp[-1].minor.yy0.n &= 0x7fffffff;
  pList = sqlite3ExprListAppend(pParse,0, yymsp[0].minor.yy132.pExpr);
  pList = sqlite3ExprListAppend(pParse,pList, yymsp[-2].minor.yy132.pExpr);
  yymsp[-2].minor.yy132.pExpr = sqlite3ExprFunction(pParse, pList, &yymsp[-1].minor.yy0);
  exprNot(pParse, bNot, &yymsp[-2].minor.yy132);
  yymsp[-2].minor.yy132.zEnd = yymsp[0].minor.yy132.zEnd;
  if( yymsp[-2].minor.yy132.pExpr ) yymsp[-2].minor.yy132.pExpr->flags |= EP_InfixFunc;
}
#line 3271 "parse.c"
        break;
      case 174: /* expr ::= expr likeop expr ESCAPE expr */
#line 1014 "parse.y"
{
  ExprList *pList;
  int bNot = yymsp[-3].minor.yy0.n & 0x80000000;
  yymsp[-3].minor.yy0.n &= 0x7fffffff;
  pList = sqlite3ExprListAppend(pParse,0, yymsp[-2].minor.yy132.pExpr);
  pList = sqlite3ExprListAppend(pParse,pList, yymsp[-4].minor.yy132.pExpr);
  pList = sqlite3ExprListAppend(pParse,pList, yymsp[0].minor.yy132.pExpr);
  yymsp[-4].minor.yy132.pExpr = sqlite3ExprFunction(pParse, pList, &yymsp[-3].minor.yy0);
  exprNot(pParse, bNot, &yymsp[-4].minor.yy132);
  yymsp[-4].minor.yy132.zEnd = yymsp[0].minor.yy132.zEnd;
  if( yymsp[-4].minor.yy132.pExpr ) yymsp[-4].minor.yy132.pExpr->flags |= EP_InfixFunc;
}
#line 3287 "parse.c"
        break;
      case 175: /* expr ::= expr ISNULL|NOTNULL */
#line 1041 "parse.y"
{spanUnaryPostfix(pParse,yymsp[0].major,&yymsp[-1].minor.yy132,&yymsp[0].minor.yy0);}
#line 3292 "parse.c"
        break;
      case 176: /* expr ::= expr NOT NULL */
#line 1042 "parse.y"
{spanUnaryPostfix(pParse,TK_NOTNULL,&yymsp[-2].minor.yy132,&yymsp[0].minor.yy0);}
#line 3297 "parse.c"
        break;
      case 177: /* expr ::= expr IS expr */
#line 1063 "parse.y"
{
  spanBinaryExpr(pParse,TK_IS,&yymsp[-2].minor.yy132,&yymsp[0].minor.yy132);
  binaryToUnaryIfNull(pParse, yymsp[0].minor.yy132.pExpr, yymsp[-2].minor.yy132.pExpr, TK_ISNULL);
}
#line 3305 "parse.c"
        break;
      case 178: /* expr ::= expr IS NOT expr */
#line 1067 "parse.y"
{
  spanBinaryExpr(pParse,TK_ISNOT,&yymsp[-3].minor.yy132,&yymsp[0].minor.yy132);
  binaryToUnaryIfNull(pParse, yymsp[0].minor.yy132.pExpr, yymsp[-3].minor.yy132.pExpr, TK_NOTNULL);
}
#line 3313 "parse.c"
        break;
      case 179: /* expr ::= NOT expr */
      case 180: /* expr ::= BITNOT expr */ yytestcase(yyruleno==180);
#line 1091 "parse.y"
{spanUnaryPrefix(&yymsp[-1].minor.yy132,pParse,yymsp[-1].major,&yymsp[0].minor.yy132,&yymsp[-1].minor.yy0);/*A-overwrites-B*/}
#line 3319 "parse.c"
        break;
      case 181: /* expr ::= MINUS expr */
#line 1095 "parse.y"
{spanUnaryPrefix(&yymsp[-1].minor.yy132,pParse,TK_UMINUS,&yymsp[0].minor.yy132,&yymsp[-1].minor.yy0);/*A-overwrites-B*/}
#line 3324 "parse.c"
        break;
      case 182: /* expr ::= PLUS expr */
#line 1097 "parse.y"
{spanUnaryPrefix(&yymsp[-1].minor.yy132,pParse,TK_UPLUS,&yymsp[0].minor.yy132,&yymsp[-1].minor.yy0);/*A-overwrites-B*/}
#line 3329 "parse.c"
        break;
      case 183: /* between_op ::= BETWEEN */
      case 186: /* in_op ::= IN */ yytestcase(yyruleno==186);
#line 1100 "parse.y"
{yymsp[0].minor.yy32 = 0;}
#line 3335 "parse.c"
        break;
      case 185: /* expr ::= expr between_op expr AND expr */
#line 1102 "parse.y"
{
  ExprList *pList = sqlite3ExprListAppend(pParse,0, yymsp[-2].minor.yy132.pExpr);
  pList = sqlite3ExprListAppend(pParse,pList, yymsp[0].minor.yy132.pExpr);
  yymsp[-4].minor.yy132.pExpr = sqlite3PExpr(pParse, TK_BETWEEN, yymsp[-4].minor.yy132.pExpr, 0);
  if( yymsp[-4].minor.yy132.pExpr ){
    yymsp[-4].minor.yy132.pExpr->x.pList = pList;
  }else{
    sqlite3ExprListDelete(pParse->db, pList);
  } 
  exprNot(pParse, yymsp[-3].minor.yy32, &yymsp[-4].minor.yy132);
  yymsp[-4].minor.yy132.zEnd = yymsp[0].minor.yy132.zEnd;
}
#line 3351 "parse.c"
        break;
      case 188: /* expr ::= expr in_op LP exprlist RP */
#line 1118 "parse.y"
{
    if( yymsp[-1].minor.yy462==0 ){
      /* Expressions of the form
      **
      **      expr1 IN ()
      **      expr1 NOT IN ()
      **
      ** simplify to constants 0 (false) and 1 (true), respectively,
      ** regardless of the value of expr1.
      */
      sqlite3ExprDelete(pParse->db, yymsp[-4].minor.yy132.pExpr);
      yymsp[-4].minor.yy132.pExpr = sqlite3ExprAlloc(pParse->db, TK_INTEGER,&sqlite3IntTokens[yymsp[-3].minor.yy32],1);
    }else if( yymsp[-1].minor.yy462->nExpr==1 ){
      /* Expressions of the form:
      **
      **      expr1 IN (?1)
      **      expr1 NOT IN (?2)
      **
      ** with exactly one value on the RHS can be simplified to something
      ** like this:
      **
      **      expr1 == ?1
      **      expr1 <> ?2
      **
      ** But, the RHS of the == or <> is marked with the EP_Generic flag
      ** so that it may not contribute to the computation of comparison
      ** affinity or the collating sequence to use for comparison.  Otherwise,
      ** the semantics would be subtly different from IN or NOT IN.
      */
      Expr *pRHS = yymsp[-1].minor.yy462->a[0].pExpr;
      yymsp[-1].minor.yy462->a[0].pExpr = 0;
      sqlite3ExprListDelete(pParse->db, yymsp[-1].minor.yy462);
      /* pRHS cannot be NULL because a malloc error would have been detected
      ** before now and control would have never reached this point */
      if( ALWAYS(pRHS) ){
        pRHS->flags &= ~EP_Collate;
        pRHS->flags |= EP_Generic;
      }
      yymsp[-4].minor.yy132.pExpr = sqlite3PExpr(pParse, yymsp[-3].minor.yy32 ? TK_NE : TK_EQ, yymsp[-4].minor.yy132.pExpr, pRHS);
    }else{
      yymsp[-4].minor.yy132.pExpr = sqlite3PExpr(pParse, TK_IN, yymsp[-4].minor.yy132.pExpr, 0);
      if( yymsp[-4].minor.yy132.pExpr ){
        yymsp[-4].minor.yy132.pExpr->x.pList = yymsp[-1].minor.yy462;
        sqlite3ExprSetHeightAndFlags(pParse, yymsp[-4].minor.yy132.pExpr);
      }else{
        sqlite3ExprListDelete(pParse->db, yymsp[-1].minor.yy462);
      }
      exprNot(pParse, yymsp[-3].minor.yy32, &yymsp[-4].minor.yy132);
    }
    yymsp[-4].minor.yy132.zEnd = &yymsp[0].minor.yy0.z[yymsp[0].minor.yy0.n];
  }
#line 3406 "parse.c"
        break;
      case 189: /* expr ::= LP select RP */
#line 1169 "parse.y"
{
    spanSet(&yymsp[-2].minor.yy132,&yymsp[-2].minor.yy0,&yymsp[0].minor.yy0); /*A-overwrites-B*/
    yymsp[-2].minor.yy132.pExpr = sqlite3PExpr(pParse, TK_SELECT, 0, 0);
    sqlite3PExprAddSelect(pParse, yymsp[-2].minor.yy132.pExpr, yymsp[-1].minor.yy149);
  }
#line 3415 "parse.c"
        break;
      case 190: /* expr ::= expr in_op LP select RP */
#line 1174 "parse.y"
{
    yymsp[-4].minor.yy132.pExpr = sqlite3PExpr(pParse, TK_IN, yymsp[-4].minor.yy132.pExpr, 0);
    sqlite3PExprAddSelect(pParse, yymsp[-4].minor.yy132.pExpr, yymsp[-1].minor.yy149);
    exprNot(pParse, yymsp[-3].minor.yy32, &yymsp[-4].minor.yy132);
    yymsp[-4].minor.yy132.zEnd = &yymsp[0].minor.yy0.z[yymsp[0].minor.yy0.n];
  }
#line 3425 "parse.c"
        break;
      case 191: /* expr ::= expr in_op nm paren_exprlist */
#line 1180 "parse.y"
{
    SrcList *pSrc = sqlite3SrcListAppend(pParse->db, 0,&yymsp[-1].minor.yy0,0);
    Select *pSelect = sqlite3SelectNew(pParse, 0,pSrc,0,0,0,0,0,0,0);
    if( yymsp[0].minor.yy462 )  sqlite3SrcListFuncArgs(pParse, pSelect ? pSrc : 0, yymsp[0].minor.yy462);
    yymsp[-3].minor.yy132.pExpr = sqlite3PExpr(pParse, TK_IN, yymsp[-3].minor.yy132.pExpr, 0);
    sqlite3PExprAddSelect(pParse, yymsp[-3].minor.yy132.pExpr, pSelect);
    exprNot(pParse, yymsp[-2].minor.yy32, &yymsp[-3].minor.yy132);
    yymsp[-3].minor.yy132.zEnd = &yymsp[-1].minor.yy0.z[yymsp[-1].minor.yy0.n];
  }
#line 3438 "parse.c"
        break;
      case 192: /* expr ::= EXISTS LP select RP */
#line 1189 "parse.y"
{
    Expr *p;
    spanSet(&yymsp[-3].minor.yy132,&yymsp[-3].minor.yy0,&yymsp[0].minor.yy0); /*A-overwrites-B*/
    p = yymsp[-3].minor.yy132.pExpr = sqlite3PExpr(pParse, TK_EXISTS, 0, 0);
    sqlite3PExprAddSelect(pParse, p, yymsp[-1].minor.yy149);
  }
#line 3448 "parse.c"
        break;
      case 193: /* expr ::= CASE case_operand case_exprlist case_else END */
#line 1198 "parse.y"
{
  spanSet(&yymsp[-4].minor.yy132,&yymsp[-4].minor.yy0,&yymsp[0].minor.yy0);  /*A-overwrites-C*/
  yymsp[-4].minor.yy132.pExpr = sqlite3PExpr(pParse, TK_CASE, yymsp[-3].minor.yy342, 0);
  if( yymsp[-4].minor.yy132.pExpr ){
    yymsp[-4].minor.yy132.pExpr->x.pList = yymsp[-1].minor.yy342 ? sqlite3ExprListAppend(pParse,yymsp[-2].minor.yy462,yymsp[-1].minor.yy342) : yymsp[-2].minor.yy462;
    sqlite3ExprSetHeightAndFlags(pParse, yymsp[-4].minor.yy132.pExpr);
  }else{
    sqlite3ExprListDelete(pParse->db, yymsp[-2].minor.yy462);
    sqlite3ExprDelete(pParse->db, yymsp[-1].minor.yy342);
  }
}
#line 3463 "parse.c"
        break;
      case 194: /* case_exprlist ::= case_exprlist WHEN expr THEN expr */
#line 1211 "parse.y"
{
  yymsp[-4].minor.yy462 = sqlite3ExprListAppend(pParse,yymsp[-4].minor.yy462, yymsp[-2].minor.yy132.pExpr);
  yymsp[-4].minor.yy462 = sqlite3ExprListAppend(pParse,yymsp[-4].minor.yy462, yymsp[0].minor.yy132.pExpr);
}
#line 3471 "parse.c"
        break;
      case 195: /* case_exprlist ::= WHEN expr THEN expr */
#line 1215 "parse.y"
{
  yymsp[-3].minor.yy462 = sqlite3ExprListAppend(pParse,0, yymsp[-2].minor.yy132.pExpr);
  yymsp[-3].minor.yy462 = sqlite3ExprListAppend(pParse,yymsp[-3].minor.yy462, yymsp[0].minor.yy132.pExpr);
}
#line 3479 "parse.c"
        break;
      case 198: /* case_operand ::= expr */
#line 1225 "parse.y"
{yymsp[0].minor.yy342 = yymsp[0].minor.yy132.pExpr; /*A-overwrites-X*/}
#line 3484 "parse.c"
        break;
      case 201: /* nexprlist ::= nexprlist COMMA expr */
#line 1236 "parse.y"
{yymsp[-2].minor.yy462 = sqlite3ExprListAppend(pParse,yymsp[-2].minor.yy462,yymsp[0].minor.yy132.pExpr);}
#line 3489 "parse.c"
        break;
      case 202: /* nexprlist ::= expr */
#line 1238 "parse.y"
{yymsp[0].minor.yy462 = sqlite3ExprListAppend(pParse,0,yymsp[0].minor.yy132.pExpr); /*A-overwrites-Y*/}
#line 3494 "parse.c"
        break;
      case 204: /* paren_exprlist ::= LP exprlist RP */
      case 209: /* eidlist_opt ::= LP eidlist RP */ yytestcase(yyruleno==209);
#line 1246 "parse.y"
{yymsp[-2].minor.yy462 = yymsp[-1].minor.yy462;}
#line 3500 "parse.c"
        break;
      case 205: /* cmd ::= createkw uniqueflag INDEX ifnotexists nm ON nm LP sortlist RP where_opt */
#line 1253 "parse.y"
{
  sqlite3CreateIndex(pParse, &yymsp[-6].minor.yy0, 
                     sqlite3SrcListAppend(pParse->db,0,&yymsp[-4].minor.yy0,0), yymsp[-2].minor.yy462, yymsp[-9].minor.yy32,
                      &yymsp[-10].minor.yy0, yymsp[0].minor.yy342, SQLITE_SO_ASC, yymsp[-7].minor.yy32, SQLITE_IDXTYPE_APPDEF);
}
#line 3509 "parse.c"
        break;
      case 206: /* uniqueflag ::= UNIQUE */
      case 245: /* raisetype ::= ABORT */ yytestcase(yyruleno==245);
#line 1260 "parse.y"
{yymsp[0].minor.yy32 = OE_Abort;}
#line 3515 "parse.c"
        break;
      case 207: /* uniqueflag ::= */
#line 1261 "parse.y"
{yymsp[1].minor.yy32 = OE_None;}
#line 3520 "parse.c"
        break;
      case 210: /* eidlist ::= eidlist COMMA nm collate sortorder */
#line 1311 "parse.y"
{
  yymsp[-4].minor.yy462 = parserAddExprIdListTerm(pParse, yymsp[-4].minor.yy462, &yymsp[-2].minor.yy0, yymsp[-1].minor.yy32, yymsp[0].minor.yy32);
}
#line 3527 "parse.c"
        break;
      case 211: /* eidlist ::= nm collate sortorder */
#line 1314 "parse.y"
{
  yymsp[-2].minor.yy462 = parserAddExprIdListTerm(pParse, 0, &yymsp[-2].minor.yy0, yymsp[-1].minor.yy32, yymsp[0].minor.yy32); /*A-overwrites-Y*/
}
#line 3534 "parse.c"
        break;
      case 214: /* cmd ::= DROP INDEX ifexists fullname ON nm */
#line 1325 "parse.y"
{
    sqlite3DropIndex(pParse, yymsp[-2].minor.yy287, &yymsp[0].minor.yy0, yymsp[-3].minor.yy32);
}
#line 3541 "parse.c"
        break;
      case 215: /* cmd ::= PRAGMA nm */
#line 1337 "parse.y"
{sqlite3Pragma(pParse,&yymsp[0].minor.yy0,0,0,0);}
#line 3546 "parse.c"
        break;
      case 216: /* cmd ::= PRAGMA nm EQ nmnum */
#line 1338 "parse.y"
{sqlite3Pragma(pParse,&yymsp[-2].minor.yy0,0,&yymsp[0].minor.yy0,0);}
#line 3551 "parse.c"
        break;
      case 217: /* cmd ::= PRAGMA nm LP nmnum RP */
#line 1339 "parse.y"
{sqlite3Pragma(pParse,&yymsp[-3].minor.yy0,0,&yymsp[-1].minor.yy0,0);}
#line 3556 "parse.c"
        break;
      case 218: /* cmd ::= PRAGMA nm EQ minus_num */
#line 1341 "parse.y"
{sqlite3Pragma(pParse,&yymsp[-2].minor.yy0,0,&yymsp[0].minor.yy0,1);}
#line 3561 "parse.c"
        break;
      case 219: /* cmd ::= PRAGMA nm LP minus_num RP */
#line 1343 "parse.y"
{sqlite3Pragma(pParse,&yymsp[-3].minor.yy0,0,&yymsp[-1].minor.yy0,1);}
#line 3566 "parse.c"
        break;
      case 222: /* cmd ::= createkw trigger_decl BEGIN trigger_cmd_list END */
#line 1359 "parse.y"
{
  Token all;
  all.z = yymsp[-3].minor.yy0.z;
  all.n = (int)(yymsp[0].minor.yy0.z - yymsp[-3].minor.yy0.z) + yymsp[0].minor.yy0.n;
  sqlite3FinishTrigger(pParse, yymsp[-1].minor.yy7, &all);
}
#line 3576 "parse.c"
        break;
      case 223: /* trigger_decl ::= TRIGGER ifnotexists nm trigger_time trigger_event ON fullname foreach_clause when_clause */
#line 1368 "parse.y"
{
  sqlite3BeginTrigger(pParse, &yymsp[-6].minor.yy0, yymsp[-5].minor.yy32, yymsp[-4].minor.yy160.a, yymsp[-4].minor.yy160.b, yymsp[-2].minor.yy287, yymsp[0].minor.yy342, yymsp[-7].minor.yy32);
  yymsp[-8].minor.yy0 = yymsp[-6].minor.yy0; /*yymsp[-8].minor.yy0-overwrites-T*/
}
#line 3584 "parse.c"
        break;
      case 224: /* trigger_time ::= BEFORE */
#line 1374 "parse.y"
{ yymsp[0].minor.yy32 = TK_BEFORE; }
#line 3589 "parse.c"
        break;
      case 225: /* trigger_time ::= AFTER */
#line 1375 "parse.y"
{ yymsp[0].minor.yy32 = TK_AFTER;  }
#line 3594 "parse.c"
        break;
      case 226: /* trigger_time ::= INSTEAD OF */
#line 1376 "parse.y"
{ yymsp[-1].minor.yy32 = TK_INSTEAD;}
#line 3599 "parse.c"
        break;
      case 227: /* trigger_time ::= */
#line 1377 "parse.y"
{ yymsp[1].minor.yy32 = TK_BEFORE; }
#line 3604 "parse.c"
        break;
      case 228: /* trigger_event ::= DELETE|INSERT */
      case 229: /* trigger_event ::= UPDATE */ yytestcase(yyruleno==229);
#line 1381 "parse.y"
{yymsp[0].minor.yy160.a = yymsp[0].major; /*A-overwrites-X*/ yymsp[0].minor.yy160.b = 0;}
#line 3610 "parse.c"
        break;
      case 230: /* trigger_event ::= UPDATE OF idlist */
#line 1383 "parse.y"
{yymsp[-2].minor.yy160.a = TK_UPDATE; yymsp[-2].minor.yy160.b = yymsp[0].minor.yy440;}
#line 3615 "parse.c"
        break;
      case 231: /* when_clause ::= */
#line 1390 "parse.y"
{ yymsp[1].minor.yy342 = 0; }
#line 3620 "parse.c"
        break;
      case 232: /* when_clause ::= WHEN expr */
#line 1391 "parse.y"
{ yymsp[-1].minor.yy342 = yymsp[0].minor.yy132.pExpr; }
#line 3625 "parse.c"
        break;
      case 233: /* trigger_cmd_list ::= trigger_cmd_list trigger_cmd SEMI */
#line 1395 "parse.y"
{
  assert( yymsp[-2].minor.yy7!=0 );
  yymsp[-2].minor.yy7->pLast->pNext = yymsp[-1].minor.yy7;
  yymsp[-2].minor.yy7->pLast = yymsp[-1].minor.yy7;
}
#line 3634 "parse.c"
        break;
      case 234: /* trigger_cmd_list ::= trigger_cmd SEMI */
#line 1400 "parse.y"
{ 
  assert( yymsp[-1].minor.yy7!=0 );
  yymsp[-1].minor.yy7->pLast = yymsp[-1].minor.yy7;
}
#line 3642 "parse.c"
        break;
      case 235: /* trnm ::= nm DOT nm */
#line 1411 "parse.y"
{
  yymsp[-2].minor.yy0 = yymsp[0].minor.yy0;
  sqlite3ErrorMsg(pParse, 
        "qualified table names are not allowed on INSERT, UPDATE, and DELETE "
        "statements within triggers");
}
#line 3652 "parse.c"
        break;
      case 236: /* tridxby ::= INDEXED BY nm */
#line 1423 "parse.y"
{
  sqlite3ErrorMsg(pParse,
        "the INDEXED BY clause is not allowed on UPDATE or DELETE statements "
        "within triggers");
}
#line 3661 "parse.c"
        break;
      case 237: /* tridxby ::= NOT INDEXED */
#line 1428 "parse.y"
{
  sqlite3ErrorMsg(pParse,
        "the NOT INDEXED clause is not allowed on UPDATE or DELETE statements "
        "within triggers");
}
#line 3670 "parse.c"
        break;
      case 238: /* trigger_cmd ::= UPDATE orconf trnm tridxby SET setlist where_opt */
#line 1441 "parse.y"
{yymsp[-6].minor.yy7 = sqlite3TriggerUpdateStep(pParse->db, &yymsp[-4].minor.yy0, yymsp[-1].minor.yy462, yymsp[0].minor.yy342, yymsp[-5].minor.yy32);}
#line 3675 "parse.c"
        break;
      case 239: /* trigger_cmd ::= insert_cmd INTO trnm idlist_opt select */
#line 1445 "parse.y"
{yymsp[-4].minor.yy7 = sqlite3TriggerInsertStep(pParse->db, &yymsp[-2].minor.yy0, yymsp[-1].minor.yy440, yymsp[0].minor.yy149, yymsp[-4].minor.yy32);/*A-overwrites-R*/}
#line 3680 "parse.c"
        break;
      case 240: /* trigger_cmd ::= DELETE FROM trnm tridxby where_opt */
#line 1449 "parse.y"
{yymsp[-4].minor.yy7 = sqlite3TriggerDeleteStep(pParse->db, &yymsp[-2].minor.yy0, yymsp[0].minor.yy342);}
#line 3685 "parse.c"
        break;
      case 241: /* trigger_cmd ::= select */
#line 1453 "parse.y"
{yymsp[0].minor.yy7 = sqlite3TriggerSelectStep(pParse->db, yymsp[0].minor.yy149); /*A-overwrites-X*/}
#line 3690 "parse.c"
        break;
      case 242: /* expr ::= RAISE LP IGNORE RP */
#line 1456 "parse.y"
{
  spanSet(&yymsp[-3].minor.yy132,&yymsp[-3].minor.yy0,&yymsp[0].minor.yy0);  /*A-overwrites-X*/
  yymsp[-3].minor.yy132.pExpr = sqlite3PExpr(pParse, TK_RAISE, 0, 0); 
  if( yymsp[-3].minor.yy132.pExpr ){
    yymsp[-3].minor.yy132.pExpr->affinity = OE_Ignore;
  }
}
#line 3701 "parse.c"
        break;
      case 243: /* expr ::= RAISE LP raisetype COMMA nm RP */
#line 1463 "parse.y"
{
  spanSet(&yymsp[-5].minor.yy132,&yymsp[-5].minor.yy0,&yymsp[0].minor.yy0);  /*A-overwrites-X*/
  yymsp[-5].minor.yy132.pExpr = sqlite3ExprAlloc(pParse->db, TK_RAISE, &yymsp[-1].minor.yy0, 1); 
  if( yymsp[-5].minor.yy132.pExpr ) {
    yymsp[-5].minor.yy132.pExpr->affinity = (char)yymsp[-3].minor.yy32;
  }
}
#line 3712 "parse.c"
        break;
      case 244: /* raisetype ::= ROLLBACK */
#line 1473 "parse.y"
{yymsp[0].minor.yy32 = OE_Rollback;}
#line 3717 "parse.c"
        break;
      case 246: /* raisetype ::= FAIL */
#line 1475 "parse.y"
{yymsp[0].minor.yy32 = OE_Fail;}
#line 3722 "parse.c"
        break;
      case 247: /* cmd ::= DROP TRIGGER ifexists fullname */
#line 1480 "parse.y"
{
  sqlite3DropTrigger(pParse,yymsp[0].minor.yy287,yymsp[-1].minor.yy32);
}
#line 3729 "parse.c"
        break;
      case 248: /* cmd ::= REINDEX */
#line 1487 "parse.y"
{sqlite3Reindex(pParse, 0, 0);}
#line 3734 "parse.c"
        break;
      case 249: /* cmd ::= REINDEX nm */
#line 1488 "parse.y"
{sqlite3Reindex(pParse, &yymsp[0].minor.yy0, 0);}
#line 3739 "parse.c"
        break;
      case 250: /* cmd ::= REINDEX nm ON nm */
#line 1489 "parse.y"
{sqlite3Reindex(pParse, &yymsp[-2].minor.yy0, &yymsp[0].minor.yy0);}
#line 3744 "parse.c"
        break;
      case 251: /* cmd ::= ANALYZE */
#line 1494 "parse.y"
{sqlite3Analyze(pParse, 0);}
#line 3749 "parse.c"
        break;
      case 252: /* cmd ::= ANALYZE nm */
#line 1495 "parse.y"
{sqlite3Analyze(pParse, &yymsp[0].minor.yy0);}
#line 3754 "parse.c"
        break;
      case 253: /* cmd ::= ALTER TABLE fullname RENAME TO nm */
#line 1500 "parse.y"
{
  sqlite3AlterRenameTable(pParse,yymsp[-3].minor.yy287,&yymsp[0].minor.yy0);
}
#line 3761 "parse.c"
        break;
      case 254: /* cmd ::= ALTER TABLE add_column_fullname ADD kwcolumn_opt columnname carglist */
#line 1504 "parse.y"
{
  yymsp[-1].minor.yy0.n = (int)(pParse->sLastToken.z-yymsp[-1].minor.yy0.z) + pParse->sLastToken.n;
  sqlite3AlterFinishAddColumn(pParse, &yymsp[-1].minor.yy0);
}
#line 3769 "parse.c"
        break;
      case 255: /* add_column_fullname ::= fullname */
#line 1508 "parse.y"
{
  disableLookaside(pParse);
  sqlite3AlterBeginAddColumn(pParse, yymsp[0].minor.yy287);
}
#line 3777 "parse.c"
        break;
      case 256: /* cmd ::= create_vtab */
#line 1518 "parse.y"
{sqlite3VtabFinishParse(pParse,0);}
#line 3782 "parse.c"
        break;
      case 257: /* cmd ::= create_vtab LP vtabarglist RP */
#line 1519 "parse.y"
{sqlite3VtabFinishParse(pParse,&yymsp[0].minor.yy0);}
#line 3787 "parse.c"
        break;
      case 258: /* create_vtab ::= createkw VIRTUAL TABLE ifnotexists nm USING nm */
#line 1521 "parse.y"
{
    sqlite3VtabBeginParse(pParse, &yymsp[-2].minor.yy0, 0, &yymsp[0].minor.yy0, yymsp[-3].minor.yy32);
}
#line 3794 "parse.c"
        break;
      case 259: /* vtabarg ::= */
#line 1526 "parse.y"
{sqlite3VtabArgInit(pParse);}
#line 3799 "parse.c"
        break;
      case 260: /* vtabargtoken ::= ANY */
      case 261: /* vtabargtoken ::= lp anylist RP */ yytestcase(yyruleno==261);
      case 262: /* lp ::= LP */ yytestcase(yyruleno==262);
#line 1528 "parse.y"
{sqlite3VtabArgExtend(pParse,&yymsp[0].minor.yy0);}
#line 3806 "parse.c"
        break;
      case 263: /* with ::= */
#line 1543 "parse.y"
{yymsp[1].minor.yy241 = 0;}
#line 3811 "parse.c"
        break;
      case 264: /* with ::= WITH wqlist */
#line 1545 "parse.y"
{ yymsp[-1].minor.yy241 = yymsp[0].minor.yy241; }
#line 3816 "parse.c"
        break;
      case 265: /* with ::= WITH RECURSIVE wqlist */
#line 1546 "parse.y"
{ yymsp[-2].minor.yy241 = yymsp[0].minor.yy241; }
#line 3821 "parse.c"
        break;
      case 266: /* wqlist ::= nm eidlist_opt AS LP select RP */
#line 1548 "parse.y"
{
  yymsp[-5].minor.yy241 = sqlite3WithAdd(pParse, 0, &yymsp[-5].minor.yy0, yymsp[-4].minor.yy462, yymsp[-1].minor.yy149); /*A-overwrites-X*/
}
#line 3828 "parse.c"
        break;
      case 267: /* wqlist ::= wqlist COMMA nm eidlist_opt AS LP select RP */
#line 1551 "parse.y"
{
  yymsp[-7].minor.yy241 = sqlite3WithAdd(pParse, yymsp[-7].minor.yy241, &yymsp[-5].minor.yy0, yymsp[-4].minor.yy462, yymsp[-1].minor.yy149);
}
#line 3835 "parse.c"
        break;
      default:
      /* (268) input ::= ecmd */ yytestcase(yyruleno==268);
      /* (269) explain ::= */ yytestcase(yyruleno==269);
      /* (270) cmdx ::= cmd (OPTIMIZED OUT) */ assert(yyruleno!=270);
      /* (271) trans_opt ::= */ yytestcase(yyruleno==271);
      /* (272) trans_opt ::= TRANSACTION */ yytestcase(yyruleno==272);
      /* (273) trans_opt ::= TRANSACTION nm */ yytestcase(yyruleno==273);
      /* (274) savepoint_opt ::= SAVEPOINT */ yytestcase(yyruleno==274);
      /* (275) savepoint_opt ::= */ yytestcase(yyruleno==275);
      /* (276) cmd ::= create_table create_table_args */ yytestcase(yyruleno==276);
      /* (277) columnlist ::= columnlist COMMA columnname carglist */ yytestcase(yyruleno==277);
      /* (278) columnlist ::= columnname carglist */ yytestcase(yyruleno==278);
      /* (279) nm ::= ID|INDEXED */ yytestcase(yyruleno==279);
      /* (280) nm ::= STRING */ yytestcase(yyruleno==280);
      /* (281) nm ::= JOIN_KW */ yytestcase(yyruleno==281);
      /* (282) typetoken ::= typename */ yytestcase(yyruleno==282);
      /* (283) typename ::= ID|STRING */ yytestcase(yyruleno==283);
      /* (284) signed ::= plus_num (OPTIMIZED OUT) */ assert(yyruleno!=284);
      /* (285) signed ::= minus_num (OPTIMIZED OUT) */ assert(yyruleno!=285);
      /* (286) carglist ::= carglist ccons */ yytestcase(yyruleno==286);
      /* (287) carglist ::= */ yytestcase(yyruleno==287);
      /* (288) ccons ::= NULL onconf */ yytestcase(yyruleno==288);
      /* (289) conslist_opt ::= COMMA conslist */ yytestcase(yyruleno==289);
      /* (290) conslist ::= conslist tconscomma tcons */ yytestcase(yyruleno==290);
      /* (291) conslist ::= tcons (OPTIMIZED OUT) */ assert(yyruleno!=291);
      /* (292) tconscomma ::= */ yytestcase(yyruleno==292);
      /* (293) defer_subclause_opt ::= defer_subclause (OPTIMIZED OUT) */ assert(yyruleno!=293);
      /* (294) resolvetype ::= raisetype (OPTIMIZED OUT) */ assert(yyruleno!=294);
      /* (295) selectnowith ::= oneselect (OPTIMIZED OUT) */ assert(yyruleno!=295);
      /* (296) oneselect ::= values */ yytestcase(yyruleno==296);
      /* (297) sclp ::= selcollist COMMA */ yytestcase(yyruleno==297);
      /* (298) as ::= ID|STRING */ yytestcase(yyruleno==298);
      /* (299) expr ::= term (OPTIMIZED OUT) */ assert(yyruleno!=299);
      /* (300) exprlist ::= nexprlist */ yytestcase(yyruleno==300);
      /* (301) nmnum ::= plus_num (OPTIMIZED OUT) */ assert(yyruleno!=301);
      /* (302) nmnum ::= nm (OPTIMIZED OUT) */ assert(yyruleno!=302);
      /* (303) nmnum ::= ON */ yytestcase(yyruleno==303);
      /* (304) nmnum ::= DELETE */ yytestcase(yyruleno==304);
      /* (305) nmnum ::= DEFAULT */ yytestcase(yyruleno==305);
      /* (306) plus_num ::= INTEGER|FLOAT */ yytestcase(yyruleno==306);
      /* (307) foreach_clause ::= */ yytestcase(yyruleno==307);
      /* (308) foreach_clause ::= FOR EACH ROW */ yytestcase(yyruleno==308);
      /* (309) trnm ::= nm */ yytestcase(yyruleno==309);
      /* (310) tridxby ::= */ yytestcase(yyruleno==310);
      /* (311) kwcolumn_opt ::= */ yytestcase(yyruleno==311);
      /* (312) kwcolumn_opt ::= COLUMNKW */ yytestcase(yyruleno==312);
      /* (313) vtabarglist ::= vtabarg */ yytestcase(yyruleno==313);
      /* (314) vtabarglist ::= vtabarglist COMMA vtabarg */ yytestcase(yyruleno==314);
      /* (315) vtabarg ::= vtabarg vtabargtoken */ yytestcase(yyruleno==315);
      /* (316) anylist ::= */ yytestcase(yyruleno==316);
      /* (317) anylist ::= anylist LP anylist RP */ yytestcase(yyruleno==317);
      /* (318) anylist ::= anylist ANY */ yytestcase(yyruleno==318);
        break;
/********** End reduce actions ************************************************/
  };
  assert( yyruleno<sizeof(yyRuleInfo)/sizeof(yyRuleInfo[0]) );
  yygoto = yyRuleInfo[yyruleno].lhs;
  yysize = yyRuleInfo[yyruleno].nrhs;
  yyact = yy_find_reduce_action(yymsp[-yysize].stateno,(YYCODETYPE)yygoto);
  if( yyact <= YY_MAX_SHIFTREDUCE ){
    if( yyact>YY_MAX_SHIFT ){
      yyact += YY_MIN_REDUCE - YY_MIN_SHIFTREDUCE;
    }
    yymsp -= yysize-1;
    yypParser->yytos = yymsp;
    yymsp->stateno = (YYACTIONTYPE)yyact;
    yymsp->major = (YYCODETYPE)yygoto;
    yyTraceShift(yypParser, yyact);
  }else{
    assert( yyact == YY_ACCEPT_ACTION );
    yypParser->yytos -= yysize;
    yy_accept(yypParser);
  }
}

/*
** The following code executes when the parse fails
*/
#ifndef YYNOERRORRECOVERY
static void yy_parse_failed(
  yyParser *yypParser           /* The parser */
){
  sqlite3ParserARG_FETCH;
#ifndef NDEBUG
  if( yyTraceFILE ){
    fprintf(yyTraceFILE,"%sFail!\n",yyTracePrompt);
  }
#endif
  while( yypParser->yytos>yypParser->yystack ) yy_pop_parser_stack(yypParser);
  /* Here code is inserted which will be executed whenever the
  ** parser fails */
/************ Begin %parse_failure code ***************************************/
/************ End %parse_failure code *****************************************/
  sqlite3ParserARG_STORE; /* Suppress warning about unused %extra_argument variable */
}
#endif /* YYNOERRORRECOVERY */

/*
** The following code executes when a syntax error first occurs.
*/
static void yy_syntax_error(
  yyParser *yypParser,           /* The parser */
  int yymajor,                   /* The major type of the error token */
  sqlite3ParserTOKENTYPE yyminor         /* The minor type of the error token */
){
  sqlite3ParserARG_FETCH;
#define TOKEN yyminor
/************ Begin %syntax_error code ****************************************/
#line 32 "parse.y"

  UNUSED_PARAMETER(yymajor);  /* Silence some compiler warnings */
  assert( TOKEN.z[0] );  /* The tokenizer always gives us a token */
  sqlite3ErrorMsg(pParse, "near \"%T\": syntax error", &TOKEN);
#line 3950 "parse.c"
/************ End %syntax_error code ******************************************/
  sqlite3ParserARG_STORE; /* Suppress warning about unused %extra_argument variable */
}

/*
** The following is executed when the parser accepts
*/
static void yy_accept(
  yyParser *yypParser           /* The parser */
){
  sqlite3ParserARG_FETCH;
#ifndef NDEBUG
  if( yyTraceFILE ){
    fprintf(yyTraceFILE,"%sAccept!\n",yyTracePrompt);
  }
#endif
#ifndef YYNOERRORRECOVERY
  yypParser->yyerrcnt = -1;
#endif
  assert( yypParser->yytos==yypParser->yystack );
  /* Here code is inserted which will be executed whenever the
  ** parser accepts */
/*********** Begin %parse_accept code *****************************************/
/*********** End %parse_accept code *******************************************/
  sqlite3ParserARG_STORE; /* Suppress warning about unused %extra_argument variable */
}

/* The main parser program.
** The first argument is a pointer to a structure obtained from
** "sqlite3ParserAlloc" which describes the current state of the parser.
** The second argument is the major token number.  The third is
** the minor token.  The fourth optional argument is whatever the
** user wants (and specified in the grammar) and is available for
** use by the action routines.
**
** Inputs:
** <ul>
** <li> A pointer to the parser (an opaque structure.)
** <li> The major token number.
** <li> The minor token number.
** <li> An option argument of a grammar-specified type.
** </ul>
**
** Outputs:
** None.
*/
void sqlite3Parser(
  void *yyp,                   /* The parser */
  int yymajor,                 /* The major token code number */
  sqlite3ParserTOKENTYPE yyminor       /* The value for the token */
  sqlite3ParserARG_PDECL               /* Optional %extra_argument parameter */
){
  YYMINORTYPE yyminorunion;
  unsigned int yyact;   /* The parser action. */
#if !defined(YYERRORSYMBOL) && !defined(YYNOERRORRECOVERY)
  int yyendofinput;     /* True if we are at the end of input */
#endif
#ifdef YYERRORSYMBOL
  int yyerrorhit = 0;   /* True if yymajor has invoked an error */
#endif
  yyParser *yypParser;  /* The parser */

  yypParser = (yyParser*)yyp;
  assert( yypParser->yytos!=0 );
#if !defined(YYERRORSYMBOL) && !defined(YYNOERRORRECOVERY)
  yyendofinput = (yymajor==0);
#endif
  sqlite3ParserARG_STORE;

#ifndef NDEBUG
  if( yyTraceFILE ){
    fprintf(yyTraceFILE,"%sInput '%s'\n",yyTracePrompt,yyTokenName[yymajor]);
  }
#endif

  do{
    yyact = yy_find_shift_action(yypParser,(YYCODETYPE)yymajor);
    if( yyact <= YY_MAX_SHIFTREDUCE ){
      yy_shift(yypParser,yyact,yymajor,yyminor);
#ifndef YYNOERRORRECOVERY
      yypParser->yyerrcnt--;
#endif
      yymajor = YYNOCODE;
    }else if( yyact <= YY_MAX_REDUCE ){
      yy_reduce(yypParser,yyact-YY_MIN_REDUCE);
    }else{
      assert( yyact == YY_ERROR_ACTION );
      yyminorunion.yy0 = yyminor;
#ifdef YYERRORSYMBOL
      int yymx;
#endif
#ifndef NDEBUG
      if( yyTraceFILE ){
        fprintf(yyTraceFILE,"%sSyntax Error!\n",yyTracePrompt);
      }
#endif
#ifdef YYERRORSYMBOL
      /* A syntax error has occurred.
      ** The response to an error depends upon whether or not the
      ** grammar defines an error token "ERROR".  
      **
      ** This is what we do if the grammar does define ERROR:
      **
      **  * Call the %syntax_error function.
      **
      **  * Begin popping the stack until we enter a state where
      **    it is legal to shift the error symbol, then shift
      **    the error symbol.
      **
      **  * Set the error count to three.
      **
      **  * Begin accepting and shifting new tokens.  No new error
      **    processing will occur until three tokens have been
      **    shifted successfully.
      **
      */
      if( yypParser->yyerrcnt<0 ){
        yy_syntax_error(yypParser,yymajor,yyminor);
      }
      yymx = yypParser->yytos->major;
      if( yymx==YYERRORSYMBOL || yyerrorhit ){
#ifndef NDEBUG
        if( yyTraceFILE ){
          fprintf(yyTraceFILE,"%sDiscard input token %s\n",
             yyTracePrompt,yyTokenName[yymajor]);
        }
#endif
        yy_destructor(yypParser, (YYCODETYPE)yymajor, &yyminorunion);
        yymajor = YYNOCODE;
      }else{
        while( yypParser->yytos >= yypParser->yystack
            && yymx != YYERRORSYMBOL
            && (yyact = yy_find_reduce_action(
                        yypParser->yytos->stateno,
                        YYERRORSYMBOL)) >= YY_MIN_REDUCE
        ){
          yy_pop_parser_stack(yypParser);
        }
        if( yypParser->yytos < yypParser->yystack || yymajor==0 ){
          yy_destructor(yypParser,(YYCODETYPE)yymajor,&yyminorunion);
          yy_parse_failed(yypParser);
#ifndef YYNOERRORRECOVERY
          yypParser->yyerrcnt = -1;
#endif
          yymajor = YYNOCODE;
        }else if( yymx!=YYERRORSYMBOL ){
          yy_shift(yypParser,yyact,YYERRORSYMBOL,yyminor);
        }
      }
      yypParser->yyerrcnt = 3;
      yyerrorhit = 1;
#elif defined(YYNOERRORRECOVERY)
      /* If the YYNOERRORRECOVERY macro is defined, then do not attempt to
      ** do any kind of error recovery.  Instead, simply invoke the syntax
      ** error routine and continue going as if nothing had happened.
      **
      ** Applications can set this macro (for example inside %include) if
      ** they intend to abandon the parse upon the first syntax error seen.
      */
      yy_syntax_error(yypParser,yymajor, yyminor);
      yy_destructor(yypParser,(YYCODETYPE)yymajor,&yyminorunion);
      yymajor = YYNOCODE;
      
#else  /* YYERRORSYMBOL is not defined */
      /* This is what we do if the grammar does not define ERROR:
      **
      **  * Report an error message, and throw away the input token.
      **
      **  * If the input token is $, then fail the parse.
      **
      ** As before, subsequent error messages are suppressed until
      ** three input tokens have been successfully shifted.
      */
      if( yypParser->yyerrcnt<=0 ){
        yy_syntax_error(yypParser,yymajor, yyminor);
      }
      yypParser->yyerrcnt = 3;
      yy_destructor(yypParser,(YYCODETYPE)yymajor,&yyminorunion);
      if( yyendofinput ){
        yy_parse_failed(yypParser);
#ifndef YYNOERRORRECOVERY
        yypParser->yyerrcnt = -1;
#endif
      }
      yymajor = YYNOCODE;
#endif
    }
  }while( yymajor!=YYNOCODE && yypParser->yytos>yypParser->yystack );
#ifndef NDEBUG
  if( yyTraceFILE ){
    yyStackEntry *i;
    char cDiv = '[';
    fprintf(yyTraceFILE,"%sReturn. Stack=",yyTracePrompt);
    for(i=&yypParser->yystack[1]; i<=yypParser->yytos; i++){
      fprintf(yyTraceFILE,"%c%s", cDiv, yyTokenName[i->major]);
      cDiv = ' ';
    }
    fprintf(yyTraceFILE,"]\n");
  }
#endif
  return;
}

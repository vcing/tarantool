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
#include <stdbool.h>
/************ Begin %include sections from the grammar ************************/
#line 52 "parse.y"

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

#line 392 "parse.y"

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
#line 831 "parse.y"

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
      if (op != TK_VARIABLE){
        sqlite3NormalizeName(p->u.zToken);
      }
#if SQLITE_MAX_EXPR_DEPTH>0
      p->nHeight = 1;
#endif  
    }
    pOut->pExpr = p;
    pOut->zStart = t.z;
    pOut->zEnd = &t.z[t.n];
  }
#line 939 "parse.y"

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
#line 1013 "parse.y"

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
#line 1030 "parse.y"

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
#line 1058 "parse.y"

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
#line 1263 "parse.y"

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
#define YYNOCODE 231
#define YYACTIONTYPE unsigned short int
#define YYWILDCARD 74
#define sqlite3ParserTOKENTYPE Token
typedef union {
  int yyinit;
  sqlite3ParserTOKENTYPE yy0;
  struct TrigEvent yy10;
  IdList* yy40;
  int yy52;
  struct {int value; int mask;} yy107;
  With* yy151;
  ExprSpan yy162;
  Select* yy279;
  Expr* yy362;
  ExprList* yy382;
  struct LimitVal yy384;
  SrcList* yy387;
  TriggerStep* yy427;
} YYMINORTYPE;
#ifndef YYSTACKDEPTH
#define YYSTACKDEPTH 100
#endif
#define sqlite3ParserARG_SDECL Parse *pParse;
#define sqlite3ParserARG_PDECL ,Parse *pParse
#define sqlite3ParserARG_FETCH Parse *pParse = yypParser->pParse
#define sqlite3ParserARG_STORE yypParser->pParse = pParse
#define YYFALLBACK 1
#define YYNSTATE             413
#define YYNRULE              300
#define YY_MAX_SHIFT         412
#define YY_MIN_SHIFTREDUCE   609
#define YY_MAX_SHIFTREDUCE   908
#define YY_MIN_REDUCE        909
#define YY_MAX_REDUCE        1208
#define YY_ERROR_ACTION      1209
#define YY_ACCEPT_ACTION     1210
#define YY_NO_ACTION         1211
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
#define YY_ACTTAB_COUNT (1407)
static const YYACTIONTYPE yy_action[] = {
 /*     0 */    91,   92,  287,   82,  776,  776,  788,  791,  780,  780,
 /*    10 */    89,   89,   90,   90,   90,   90,  309,   88,   88,   88,
 /*    20 */    88,   87,   87,   86,   86,   86,   85,  309,   90,   90,
 /*    30 */    90,   90,   83,   88,   88,   88,   88,   87,   87,   86,
 /*    40 */    86,   86,   85,  309,  210,  122,  893,   90,   90,   90,
 /*    50 */    90,  635,   88,   88,   88,   88,   87,   87,   86,   86,
 /*    60 */    86,   85,  309,   87,   87,   86,   86,   86,   85,  309,
 /*    70 */   893,   86,   86,   86,   85,  309,   91,   92,  287,   82,
 /*    80 */   776,  776,  788,  791,  780,  780,   89,   89,   90,   90,
 /*    90 */    90,   90,  638,   88,   88,   88,   88,   87,   87,   86,
 /*   100 */    86,   86,   85,  309,   91,   92,  287,   82,  776,  776,
 /*   110 */   788,  791,  780,  780,   89,   89,   90,   90,   90,   90,
 /*   120 */   725,   88,   88,   88,   88,   87,   87,   86,   86,   86,
 /*   130 */    85,  309,  637,   91,   92,  287,   82,  776,  776,  788,
 /*   140 */   791,  780,  780,   89,   89,   90,   90,   90,   90,   67,
 /*   150 */    88,   88,   88,   88,   87,   87,   86,   86,   86,   85,
 /*   160 */   309,  777,  777,  789,  792,  321,   93,   84,   81,  176,
 /*   170 */   307,  306, 1210,  412,    3,  724,  245,  610,  312,  726,
 /*   180 */   727,  377,   91,   92,  287,   82,  776,  776,  788,  791,
 /*   190 */   780,  780,   89,   89,   90,   90,   90,   90,  885,   88,
 /*   200 */    88,   88,   88,   87,   87,   86,   86,   86,   85,  309,
 /*   210 */    88,   88,   88,   88,   87,   87,   86,   86,   86,   85,
 /*   220 */   309,  122,   84,   81,  176,  643,  378, 1161, 1161,  829,
 /*   230 */   781,   91,   92,  287,   82,  776,  776,  788,  791,  780,
 /*   240 */   780,   89,   89,   90,   90,   90,   90,  365,   88,   88,
 /*   250 */    88,   88,   87,   87,   86,   86,   86,   85,  309,  904,
 /*   260 */   748,  904,  122,  411,  411,  172,  654,  711,  766,  220,
 /*   270 */   759,  119,  878,  754,  636,  684,  238,  334,  237,  653,
 /*   280 */    91,   92,  287,   82,  776,  776,  788,  791,  780,  780,
 /*   290 */    89,   89,   90,   90,   90,   90,  878,   88,   88,   88,
 /*   300 */    88,   87,   87,   86,   86,   86,   85,  309,   22,  748,
 /*   310 */   758,  758,  760,  201,  694,  652,  360,  357,  356,  693,
 /*   320 */   165,  711,  704,  767,  122,  238,  334,  237,  355,   91,
 /*   330 */    92,  287,   82,  776,  776,  788,  791,  780,  780,   89,
 /*   340 */    89,   90,   90,   90,   90,  748,   88,   88,   88,   88,
 /*   350 */    87,   87,   86,   86,   86,   85,  309,  697,   84,   81,
 /*   360 */   176,  238,  324,  226,  406,  406,  406,  671,  650,   84,
 /*   370 */    81,  176,  753,  122,  218,  370,  671,  341,   91,   92,
 /*   380 */   287,   82,  776,  776,  788,  791,  780,  780,   89,   89,
 /*   390 */    90,   90,   90,   90,  209,   88,   88,   88,   88,   87,
 /*   400 */    87,   86,   86,   86,   85,  309,   91,   92,  287,   82,
 /*   410 */   776,  776,  788,  791,  780,  780,   89,   89,   90,   90,
 /*   420 */    90,   90,  342,   88,   88,   88,   88,   87,   87,   86,
 /*   430 */    86,   86,   85,  309,   91,   92,  287,   82,  776,  776,
 /*   440 */   788,  791,  780,  780,   89,   89,   90,   90,   90,   90,
 /*   450 */   380,   88,   88,   88,   88,   87,   87,   86,   86,   86,
 /*   460 */    85,  309,   91,   92,  287,   82,  776,  776,  788,  791,
 /*   470 */   780,  780,   89,   89,   90,   90,   90,   90,  145,   88,
 /*   480 */    88,   88,   88,   87,   87,   86,   86,   86,   85,  309,
 /*   490 */   308,  308,  308,   85,  309,   70,   92,  287,   82,  776,
 /*   500 */   776,  788,  791,  780,  780,   89,   89,   90,   90,   90,
 /*   510 */    90,  164,   88,   88,   88,   88,   87,   87,   86,   86,
 /*   520 */    86,   85,  309,   73,  629,  629,  835,  835,  329,   91,
 /*   530 */    80,  287,   82,  776,  776,  788,  791,  780,  780,   89,
 /*   540 */    89,   90,   90,   90,   90,  391,   88,   88,   88,   88,
 /*   550 */    87,   87,   86,   86,   86,   85,  309,  287,   82,  776,
 /*   560 */   776,  788,  791,  780,  780,   89,   89,   90,   90,   90,
 /*   570 */    90,   78,   88,   88,   88,   88,   87,   87,   86,   86,
 /*   580 */    86,   85,  309,  218,  370,  699,  141,  375,  301,  141,
 /*   590 */    75,   76,  275,  629,  629,  283,  282,   77,  286,  280,
 /*   600 */   279,  278,  222,  276,  851,   78,  623,  143,  629,  629,
 /*   610 */   404,    2, 1105,  298,  318,  310,  310,  202,  852,  202,
 /*   620 */   109,  343,  881,  408,   75,   76,  853,  677,  320,  319,
 /*   630 */   645,   77,  393,  182,  678,  162,  174,  766,  337,  759,
 /*   640 */    48,   48,  754,  348,  404,    2,  345,  408,  137,  310,
 /*   650 */   310,  408,  629,  629,  288,  387,  266,  219,  155,  255,
 /*   660 */   363,  250,  362,  205,   48,   48,  393,  756,   48,   48,
 /*   670 */   248,  766,  710,  759,  408,  302,  754,  676,  676,  758,
 /*   680 */   758,  760,  761,  407,   18,  674,  674,  184,  109,  848,
 /*   690 */   320,   48,   48,  180,  315,  122,  337,  122,  752,  386,
 /*   700 */   388,  756,  185,  386,  371,  190,  374,  307,  306,   78,
 /*   710 */   314,  629,  629,  758,  758,  760,  761,  407,   18,  210,
 /*   720 */   408,  893,  109,    9,    9,  332,  386,  376,   75,   76,
 /*   730 */   698,  122,  629,  629,  167,   77,  201,   48,   48,  360,
 /*   740 */   357,  356,  402,   78,  686,  893,  335,  267,  404,    2,
 /*   750 */    20,  355,  266,  310,  310,  373,  899,  745,  903,   23,
 /*   760 */   191,  328,   75,   76,  333,  901,  343,  902,  408,   77,
 /*   770 */   393,  267,  386,  385,  217,  766,  408,  759,  851,  296,
 /*   780 */   754,   19,  404,    2,   54,   10,   10,  310,  310,  408,
 /*   790 */   109,  339,  852,   48,   48,  408,  904,  367,  904,  295,
 /*   800 */   853,  392,  710,  305,  393,  756,   30,   30,  832,  766,
 /*   810 */   831,  759,   10,   10,  754,  408,  327,  758,  758,  760,
 /*   820 */   761,  407,   18,  177,  177,  408,  297,  408,  386,  366,
 /*   830 */   109,  408,   10,   10,  710,  373,  157,  156,  398,  756,
 /*   840 */   225,  368,   48,   48,   10,   10,  200,   68,   47,   47,
 /*   850 */   236,  758,  758,  760,  761,  407,   18,   95,  383,  231,
 /*   860 */   821,  639,  639,  848,  243,  657,   75,   76,  352,  757,
 /*   870 */   203,  361,  186,   77,  821,  823,  658,  381,  177,  177,
 /*   880 */   894,  894,  146,  766,  710,  759,  404,    2,  754,  203,
 /*   890 */   373,  310,  310,    5,  299,  210,  109,  893,  257,  894,
 /*   900 */   894,  811,  265,  710,  322,   74,  408,   72,  393,  230,
 /*   910 */   828,  408,  828,  766,  242,  759,  710,  408,  754,  254,
 /*   920 */   335,  893,  187,   34,   34,  758,  758,  760,   35,   35,
 /*   930 */   253,  408,  895,  713,   36,   36,  821,  110,  344,  149,
 /*   940 */   229,  854,  228,  756,  408,  289,  710,  234,   37,   37,
 /*   950 */   248,  895,  712,  259,  325,  758,  758,  760,  761,  407,
 /*   960 */    18,   38,   38,  408,  289,  408,  161,  160,  159,  408,
 /*   970 */   710,  408,    7,  408,  138,  408,  261,  408,  629,  629,
 /*   980 */    26,   26,   27,   27,  683,  408,   29,   29,   39,   39,
 /*   990 */    40,   40,   41,   41,   11,   11,  408,  710,  408,  694,
 /*  1000 */   408,  163,   42,   42,  693,  408,  343,  408,  679,  408,
 /*  1010 */   264,  408,  711,   97,   97,   43,   43,   44,   44,  408,
 /*  1020 */   752,  408,   31,   31,   45,   45,   46,   46,   32,   32,
 /*  1030 */   408, 1184,  408,  666,  408,  233,  112,  112,  113,  113,
 /*  1040 */   408,  752,  860,  408,  752,  408,  846,  114,  114,   52,
 /*  1050 */    52,   33,   33,  408,  859,  408,  682,   98,   98,  408,
 /*  1060 */    49,   49,   99,   99,  408,  165,  711,  408,  752,  408,
 /*  1070 */   100,  100,   96,   96,  169,  408,  111,  111,  408,  109,
 /*  1080 */   408,  108,  108,  292,  104,  104,  103,  103,  408,  109,
 /*  1090 */   193,  408,  101,  101,  408,  102,  102,   51,   51,  408,
 /*  1100 */   369,  627,  689,  689,  293,   53,   53,  294,   50,   50,
 /*  1110 */    24,   25,   25,  663,  629,  629,   28,   28,    1,  395,
 /*  1120 */   107,  399,  629,  629,  633,  628,  290,  403,  405,  290,
 /*  1130 */    66,  303,  175,  174,  109,  726,  727,   64,  892,  750,
 /*  1140 */   336,  208,  208,  338,  239,  208,   66,  353,  633,  214,
 /*  1150 */   857,  246,  109,   66,  109,  646,  646,  178,  656,  655,
 /*  1160 */   109,  316,  691,  648,   69,  317,  720,  664,  208,  291,
 /*  1170 */   818,  818,  814,  827,  214,  827,  631,  740,  106,  323,
 /*  1180 */   762,  762,  227,  819,  168,  235,  845,  843,  340,  842,
 /*  1190 */   153,  346,  347,  241,  622,  244,  358,  667,  651,  650,
 /*  1200 */   158,  252,  249,  718,  751,  263,  394,  700,  816,  268,
 /*  1210 */   815,  929,  269,  274,  874,  154,  135,  634,  620,  619,
 /*  1220 */   124,  621,  871,  117,   64,  737,  326,   55,  331,  830,
 /*  1230 */   126,  351,  232,  189,  196,  144,  128,  129,  197,  147,
 /*  1240 */   364,  198,  130,  300,  648,  131,  670,  139,  349,  747,
 /*  1250 */   379,  669,  284,    6,   63,  796,  847,   71,  211,  668,
 /*  1260 */   304,   94,  384,  285,  661,  641,   21,  642,   65,  251,
 /*  1270 */   640,  810,  660,  382,  883,  872,  224,  612,  615,  221,
 /*  1280 */   311,  397,  223,  409,  410,  617,  179,  616,  613,  281,
 /*  1290 */   115,  313,  401,  123,  181,  825,  183,  824,  746,  822,
 /*  1300 */   116,  125,  120,  127,  188,  680,  105,  204,  208,  833,
 /*  1310 */   132,  133,  841,  708,  906,  134,  256,  136,  330,  709,
 /*  1320 */   258,  273,  271,  690,  707,  260,  270,  706,  262,  272,
 /*  1330 */    56,   57,   58,   59,  121,  844,  192,  194,  840,  212,
 /*  1340 */     8,   12,  148,  625,  213,  195,  240,  350,  199,  253,
 /*  1350 */   140,  354,   60,  359,   13,  659,  247,   14,  206,   61,
 /*  1360 */   118,  765,  688,  170,  389,  216,  764,  794,   15,    4,
 /*  1370 */    62,  692,  714,  171,  372,  173,  207,  142,  719,   69,
 /*  1380 */    16,   66,  809,   17,  795,  390,  793,  850,  798,  849,
 /*  1390 */   166,  396,  864,  150,  151,  215,  865,  609,  400,  797,
 /*  1400 */   152,  277,  763,  632,   79, 1166,  626,
};
static const YYCODETYPE yy_lookahead[] = {
 /*     0 */     5,    6,    7,    8,    9,   10,   11,   12,   13,   14,
 /*    10 */    15,   16,   17,   18,   19,   20,   32,   22,   23,   24,
 /*    20 */    25,   26,   27,   28,   29,   30,   31,   32,   17,   18,
 /*    30 */    19,   20,   21,   22,   23,   24,   25,   26,   27,   28,
 /*    40 */    29,   30,   31,   32,   49,  133,   51,   17,   18,   19,
 /*    50 */    20,  160,   22,   23,   24,   25,   26,   27,   28,   29,
 /*    60 */    30,   31,   32,   26,   27,   28,   29,   30,   31,   32,
 /*    70 */    75,   28,   29,   30,   31,   32,    5,    6,    7,    8,
 /*    80 */     9,   10,   11,   12,   13,   14,   15,   16,   17,   18,
 /*    90 */    19,   20,  160,   22,   23,   24,   25,   26,   27,   28,
 /*   100 */    29,   30,   31,   32,    5,    6,    7,    8,    9,   10,
 /*   110 */    11,   12,   13,   14,   15,   16,   17,   18,   19,   20,
 /*   120 */   163,   22,   23,   24,   25,   26,   27,   28,   29,   30,
 /*   130 */    31,   32,  160,    5,    6,    7,    8,    9,   10,   11,
 /*   140 */    12,   13,   14,   15,   16,   17,   18,   19,   20,   50,
 /*   150 */    22,   23,   24,   25,   26,   27,   28,   29,   30,   31,
 /*   160 */    32,    9,   10,   11,   12,   77,   67,  210,  211,  212,
 /*   170 */    26,   27,  136,  137,  138,  163,   48,    1,    2,  108,
 /*   180 */   109,    7,    5,    6,    7,    8,    9,   10,   11,   12,
 /*   190 */    13,   14,   15,   16,   17,   18,   19,   20,  173,   22,
 /*   200 */    23,   24,   25,   26,   27,   28,   29,   30,   31,   32,
 /*   210 */    22,   23,   24,   25,   26,   27,   28,   29,   30,   31,
 /*   220 */    32,  133,  210,  211,  212,   48,   52,   98,   99,   38,
 /*   230 */    78,    5,    6,    7,    8,    9,   10,   11,   12,   13,
 /*   240 */    14,   15,   16,   17,   18,   19,   20,   28,   22,   23,
 /*   250 */    24,   25,   26,   27,   28,   29,   30,   31,   32,  115,
 /*   260 */    69,  117,  133,  139,  140,   48,  169,   50,   73,  145,
 /*   270 */    75,  147,   51,   78,   48,  151,   85,   86,   87,  169,
 /*   280 */     5,    6,    7,    8,    9,   10,   11,   12,   13,   14,
 /*   290 */    15,   16,   17,   18,   19,   20,   75,   22,   23,   24,
 /*   300 */    25,   26,   27,   28,   29,   30,   31,   32,  184,   69,
 /*   310 */   115,  116,  117,   76,   95,  169,   79,   80,   81,  100,
 /*   320 */   103,  104,  201,   48,  133,   85,   86,   87,   91,    5,
 /*   330 */     6,    7,    8,    9,   10,   11,   12,   13,   14,   15,
 /*   340 */    16,   17,   18,   19,   20,   69,   22,   23,   24,   25,
 /*   350 */    26,   27,   28,   29,   30,   31,   32,  198,  210,  211,
 /*   360 */   212,   85,   86,   87,  156,  157,  158,  167,  168,  210,
 /*   370 */   211,  212,   48,  133,   98,   99,  176,    7,    5,    6,
 /*   380 */     7,    8,    9,   10,   11,   12,   13,   14,   15,   16,
 /*   390 */    17,   18,   19,   20,  198,   22,   23,   24,   25,   26,
 /*   400 */    27,   28,   29,   30,   31,   32,    5,    6,    7,    8,
 /*   410 */     9,   10,   11,   12,   13,   14,   15,   16,   17,   18,
 /*   420 */    19,   20,   52,   22,   23,   24,   25,   26,   27,   28,
 /*   430 */    29,   30,   31,   32,    5,    6,    7,    8,    9,   10,
 /*   440 */    11,   12,   13,   14,   15,   16,   17,   18,   19,   20,
 /*   450 */   205,   22,   23,   24,   25,   26,   27,   28,   29,   30,
 /*   460 */    31,   32,    5,    6,    7,    8,    9,   10,   11,   12,
 /*   470 */    13,   14,   15,   16,   17,   18,   19,   20,   49,   22,
 /*   480 */    23,   24,   25,   26,   27,   28,   29,   30,   31,   32,
 /*   490 */   156,  157,  158,   31,   32,  122,    6,    7,    8,    9,
 /*   500 */    10,   11,   12,   13,   14,   15,   16,   17,   18,   19,
 /*   510 */    20,  205,   22,   23,   24,   25,   26,   27,   28,   29,
 /*   520 */    30,   31,   32,  122,   51,   52,   85,   86,   87,    5,
 /*   530 */     6,    7,    8,    9,   10,   11,   12,   13,   14,   15,
 /*   540 */    16,   17,   18,   19,   20,  179,   22,   23,   24,   25,
 /*   550 */    26,   27,   28,   29,   30,   31,   32,    7,    8,    9,
 /*   560 */    10,   11,   12,   13,   14,   15,   16,   17,   18,   19,
 /*   570 */    20,    7,   22,   23,   24,   25,   26,   27,   28,   29,
 /*   580 */    30,   31,   32,   98,   99,   28,  143,  151,    7,  143,
 /*   590 */    26,   27,  149,   51,   52,  152,   34,   33,  152,   37,
 /*   600 */    38,   39,   40,   41,   39,    7,   44,  134,   51,   52,
 /*   610 */    46,   47,   48,   32,  143,   51,   52,  171,   53,  173,
 /*   620 */   184,  143,  159,  143,   26,   27,   61,   62,  157,  158,
 /*   630 */   167,   33,   68,   71,   69,  199,  200,   73,  143,   75,
 /*   640 */   160,  161,   78,  217,   46,   47,  220,  143,   47,   51,
 /*   650 */    52,  143,   51,   52,   92,  151,  143,   76,   77,   78,
 /*   660 */    79,   80,   81,   82,  160,  161,   68,  103,  160,  161,
 /*   670 */    89,   73,  143,   75,  143,  195,   78,  178,  179,  115,
 /*   680 */   116,  117,  118,  119,  120,  178,  179,  125,  184,  151,
 /*   690 */   219,  160,  161,  131,  132,  133,  143,  133,  143,  195,
 /*   700 */   196,  103,  207,  195,  196,  227,  143,   26,   27,    7,
 /*   710 */   181,   51,   52,  115,  116,  117,  118,  119,  120,   49,
 /*   720 */   143,   51,  184,  160,  161,  223,  195,  196,   26,   27,
 /*   730 */    28,  133,   51,   52,  222,   33,   76,  160,  161,   79,
 /*   740 */    80,   81,  229,    7,  183,   75,  208,  143,   46,   47,
 /*   750 */    16,   91,  143,   51,   52,  194,   75,  151,   77,  221,
 /*   760 */   207,  206,   26,   27,  226,   84,  143,   86,  143,   33,
 /*   770 */    68,  143,  195,  196,  187,   73,  143,   75,   39,  175,
 /*   780 */    78,   47,   46,   47,  197,  160,  161,   51,   52,  143,
 /*   790 */   184,  143,   53,  160,  161,  143,  115,  151,  117,  174,
 /*   800 */    61,   62,  143,  175,   68,  103,  160,  161,   56,   73,
 /*   810 */    58,   75,  160,  161,   78,  143,   64,  115,  116,  117,
 /*   820 */   118,  119,  120,  182,  183,  143,  174,  143,  195,  196,
 /*   830 */   184,  143,  160,  161,  143,  194,   26,   27,  229,  103,
 /*   840 */   181,  195,  160,  161,  160,  161,  174,    7,  160,  161,
 /*   850 */   227,  115,  116,  117,  118,  119,  120,   47,  174,   43,
 /*   860 */   143,   51,   52,  151,   43,   59,   26,   27,    7,  143,
 /*   870 */     9,   65,  181,   33,  157,  158,   70,  195,  182,  183,
 /*   880 */    51,   52,  185,   73,  143,   75,   46,   47,   78,    9,
 /*   890 */   194,   51,   52,   47,   88,   49,  184,   51,  198,   51,
 /*   900 */    52,   80,  214,  143,  208,  121,  143,  123,   68,   93,
 /*   910 */   115,  143,  117,   73,   93,   75,  143,  143,   78,   78,
 /*   920 */   208,   75,  181,  160,  161,  115,  116,  117,  160,  161,
 /*   930 */    89,  143,  103,  104,  160,  161,  219,   47,  226,   49,
 /*   940 */   124,  181,  126,  103,  143,   84,  143,  126,  160,  161,
 /*   950 */    89,  103,  104,  198,  181,  115,  116,  117,  118,  119,
 /*   960 */   120,  160,  161,  143,   84,  143,   85,   86,   87,  143,
 /*   970 */   143,  143,  186,  143,   47,  143,  198,  143,   51,   52,
 /*   980 */   160,  161,  160,  161,  181,  143,  160,  161,  160,  161,
 /*   990 */   160,  161,  160,  161,  160,  161,  143,  143,  143,   95,
 /*  1000 */   143,  143,  160,  161,  100,  143,  143,  143,  181,  143,
 /*  1010 */   143,  143,   50,  160,  161,  160,  161,  160,  161,  143,
 /*  1020 */   143,  143,  160,  161,  160,  161,  160,  161,  160,  161,
 /*  1030 */   143,   48,  143,   50,  143,  181,  160,  161,  160,  161,
 /*  1040 */   143,  143,  143,  143,  143,  143,  151,  160,  161,  160,
 /*  1050 */   161,  160,  161,  143,  143,  143,  151,  160,  161,  143,
 /*  1060 */   160,  161,  160,  161,  143,  103,  104,  143,  143,  143,
 /*  1070 */   160,  161,  160,  161,   50,  143,  160,  161,  143,  184,
 /*  1080 */   143,  160,  161,  206,  160,  161,  160,  161,  143,  184,
 /*  1090 */   227,  143,  160,  161,  143,  160,  161,  160,  161,  143,
 /*  1100 */    94,  151,   96,   97,  206,  160,  161,  206,  160,  161,
 /*  1110 */    47,  160,  161,   36,   51,   52,  160,  161,   47,  151,
 /*  1120 */    47,  151,   51,   52,   51,  154,  155,  151,  154,  155,
 /*  1130 */    50,  206,  199,  200,  184,  108,  109,  113,   50,   48,
 /*  1140 */    48,   50,   50,   48,   48,   50,   50,   48,   75,   50,
 /*  1150 */   143,   48,  184,   50,  184,   51,   52,   47,   77,   78,
 /*  1160 */   184,  143,   48,   83,   50,  143,   48,   90,   50,  143,
 /*  1170 */    51,   52,   48,  115,   50,  117,   48,  189,   50,  202,
 /*  1180 */    51,   52,  202,  143,  143,  228,  189,  143,  228,  143,
 /*  1190 */   102,  143,  143,  143,  143,  143,  164,  143,  143,  168,
 /*  1200 */   172,  163,  143,  143,  143,  202,  216,  143,  163,  143,
 /*  1210 */   163,  101,  143,  188,  146,  186,   47,  143,  143,  143,
 /*  1220 */   209,  143,  143,    5,  113,  189,   45,  121,  128,  225,
 /*  1230 */   177,   45,  224,  148,  148,   47,  180,  180,  148,  209,
 /*  1240 */    84,  148,  180,   63,   83,  180,  162,  177,  165,  177,
 /*  1250 */   106,  162,  165,   47,   84,  213,  189,  121,  215,  162,
 /*  1260 */    32,  112,  107,  165,  170,  164,   50,  162,  111,  162,
 /*  1270 */   162,  189,  170,  110,  162,   40,   35,    4,   36,  144,
 /*  1280 */     3,  165,  144,  150,  142,  142,   42,  142,  142,  141,
 /*  1290 */   153,   72,  165,   43,   84,   48,  101,   48,   99,   48,
 /*  1300 */   153,  114,   88,  102,   84,   46,  166,  166,   50,  127,
 /*  1310 */   127,   84,    1,  204,  130,  102,  203,  114,  129,  204,
 /*  1320 */   203,  189,  191,  193,  204,  203,  192,  204,  203,  190,
 /*  1330 */    16,   16,   16,   16,   88,   52,  105,  101,    1,  218,
 /*  1340 */    34,   47,   49,   46,  218,   84,  124,    7,   82,   89,
 /*  1350 */    47,   66,   47,   66,   47,   54,   48,   47,   66,   47,
 /*  1360 */    60,   48,   95,  101,   75,  101,   48,   48,   47,   47,
 /*  1370 */    50,   48,  104,   48,   50,   48,  105,   47,   52,   50,
 /*  1380 */   105,   50,   48,  105,   48,   50,   48,   48,   38,   48,
 /*  1390 */    47,   49,   48,   47,   47,   50,   48,    1,   49,   48,
 /*  1400 */    47,   42,   48,   48,   47,    0,   48,
};
#define YY_SHIFT_USE_DFLT (1407)
#define YY_SHIFT_COUNT    (412)
#define YY_SHIFT_MIN      (-88)
#define YY_SHIFT_MAX      (1405)
static const short yy_shift_ofst[] = {
 /*     0 */   176,  564,  598,  562,  736,  736,  736,  736,  240,   -5,
 /*    10 */    71,   71,  736,  736,  736,  736,  736,  736,  736,  681,
 /*    20 */   681,  660,  276,  191,  129,   99,  128,  177,  226,  275,
 /*    30 */   324,  373,  401,  429,  457,  457,  457,  457,  457,  457,
 /*    40 */   457,  457,  457,  457,  457,  457,  457,  457,  457,  524,
 /*    50 */   457,  490,  550,  550,  702,  736,  736,  736,  736,  736,
 /*    60 */   736,  736,  736,  736,  736,  736,  736,  736,  736,  736,
 /*    70 */   736,  736,  736,  736,  736,  736,  736,  736,  736,  736,
 /*    80 */   736,  736,  840,  736,  736,  736,  736,  736,  736,  736,
 /*    90 */   736,  736,  736,  736,  736,  736,   11,   30,   30,   30,
 /*   100 */    30,   30,  188,   37,   43,  861,  144,  144,  462,  485,
 /*   110 */   542,  -16, 1407, 1407, 1407,  581,  581,  565,  565,  821,
 /*   120 */   601,  601,  473,  542,   88,  542,  542,  542,  542,  542,
 /*   130 */   542,  542,  542,  542,  542,  542,  542,  542,  542,  542,
 /*   140 */   542,  221,  542,  542,  542,  221,  485,  -88,  -88,  -88,
 /*   150 */   -88,  -88,  -88, 1407, 1407,  810,  195,  195,  237,  806,
 /*   160 */   806,  806,  217,  846,  829,  848,  739,  441,  752,  927,
 /*   170 */   557,  670,  670,  670, 1063,  962, 1071, 1006,  219,  542,
 /*   180 */   542,  542,  542,  542,  542, 1024,  174,  174,  542,  542,
 /*   190 */   370, 1024,  542,  370,  542,  542,  542,  542,  542,  542,
 /*   200 */  1080,  542,  983,  542,  880,  542, 1027,  542,  542,  174,
 /*   210 */   542,  784, 1027, 1027,  542,  542,  542, 1088,  904,  542,
 /*   220 */   890,  542,  542,  542,  542, 1169, 1218, 1111, 1181, 1181,
 /*   230 */  1181, 1181, 1106, 1100, 1186, 1111, 1169, 1218, 1218, 1111,
 /*   240 */  1186, 1188, 1186, 1186, 1188, 1156, 1156, 1156, 1180, 1188,
 /*   250 */  1156, 1161, 1156, 1180, 1156, 1156, 1144, 1170, 1144, 1170,
 /*   260 */  1144, 1170, 1144, 1170, 1206, 1136, 1188, 1228, 1228, 1188,
 /*   270 */  1149, 1155, 1157, 1163, 1111, 1216, 1235, 1235, 1241, 1241,
 /*   280 */  1241, 1241, 1242, 1407, 1407, 1407, 1407,  152,  816,  881,
 /*   290 */  1073,  734, 1091, 1092, 1095, 1096, 1099, 1103, 1104, 1081,
 /*   300 */  1077,  841, 1114, 1118, 1119, 1124,  795, 1058, 1128, 1129,
 /*   310 */  1110, 1273, 1277, 1244, 1219, 1250, 1210, 1247, 1195, 1249,
 /*   320 */  1251, 1199, 1187, 1214, 1201, 1220, 1259, 1182, 1258, 1183,
 /*   330 */  1184, 1189, 1227, 1311, 1213, 1203, 1314, 1315, 1316, 1317,
 /*   340 */  1246, 1283, 1231, 1236, 1337, 1306, 1294, 1261, 1222, 1293,
 /*   350 */  1297, 1340, 1260, 1266, 1303, 1285, 1305, 1307, 1308, 1310,
 /*   360 */  1287, 1301, 1312, 1292, 1300, 1313, 1318, 1319, 1320, 1267,
 /*   370 */  1321, 1323, 1322, 1324, 1262, 1325, 1327, 1326, 1271, 1330,
 /*   380 */  1268, 1329, 1275, 1331, 1278, 1334, 1329, 1336, 1338, 1339,
 /*   390 */  1289, 1335, 1341, 1343, 1350, 1344, 1346, 1342, 1345, 1348,
 /*   400 */  1347, 1349, 1345, 1351, 1353, 1354, 1355, 1357, 1264, 1358,
 /*   410 */  1359, 1396, 1405,
};
#define YY_REDUCE_USE_DFLT (-110)
#define YY_REDUCE_COUNT (286)
#define YY_REDUCE_MIN   (-109)
#define YY_REDUCE_MAX   (1148)
static const short yy_reduce_ofst[] = {
 /*     0 */    36,  504,  646,  124,  508,  531,  577,  633,  538,  159,
 /*    10 */   -43,   12,  625,  652,  672,  480,  682,  684,  688,  471,
 /*    20 */   717,  446,  696,  712,  436,  148,  148,  148,  148,  148,
 /*    30 */   148,  148,  148,  148,  148,  148,  148,  148,  148,  148,
 /*    40 */   148,  148,  148,  148,  148,  148,  148,  148,  148,  148,
 /*    50 */   148,  148,  148,  148,  563,  763,  768,  774,  788,  801,
 /*    60 */   820,  822,  826,  828,  830,  832,  834,  842,  853,  855,
 /*    70 */   857,  862,  864,  866,  868,  876,  878,  887,  889,  891,
 /*    80 */   897,  900,  902,  910,  912,  916,  921,  924,  926,  932,
 /*    90 */   935,  937,  945,  948,  951,  956,  148,  148,  148,  148,
 /*   100 */   148,  148,  148,  148,  148,  200,  208,  334,  148,  641,
 /*   110 */   443,  148,  148,  148,  148,  463,  463,  499,  507,  426,
 /*   120 */   495,  553,  513,  529,  606,  659,  691,  741,  760,  773,
 /*   130 */   803,  827,  555,  854,  478,  877,  623,  898,  901,  863,
 /*   140 */   604,  971,  925,  609,  628,  974,  561,  895,  905,  950,
 /*   150 */   968,  970,  976,  933,  587, -109,  -68,  -28,   25,   97,
 /*   160 */   110,  146,  121,  196,  245,  306,  366,  502,  512,  648,
 /*   170 */   726,  700,  755,  778,  858,  121,  867,  697,  786,  899,
 /*   180 */   911, 1007, 1018, 1022, 1026,  988,  977,  980, 1040, 1041,
 /*   190 */   957,  997, 1044,  960, 1046, 1048, 1049, 1050, 1051, 1052,
 /*   200 */  1032, 1054, 1028, 1055, 1031, 1059, 1038, 1060, 1061, 1003,
 /*   210 */  1064,  990, 1045, 1047, 1066, 1069,  726, 1025, 1029, 1074,
 /*   220 */  1068, 1075, 1076, 1078, 1079, 1011, 1053, 1036, 1056, 1057,
 /*   230 */  1062, 1065, 1004, 1008, 1085, 1067, 1030, 1070, 1072, 1082,
 /*   240 */  1086, 1083, 1090, 1093, 1087, 1084, 1089, 1097, 1094, 1098,
 /*   250 */  1105, 1101, 1107, 1102, 1108, 1112, 1109, 1113, 1115, 1117,
 /*   260 */  1120, 1122, 1123, 1125, 1042, 1043, 1116, 1121, 1126, 1127,
 /*   270 */  1130, 1134, 1131, 1139, 1132, 1133, 1135, 1138, 1142, 1143,
 /*   280 */  1145, 1146, 1148, 1137, 1140, 1141, 1147,
};
static const YYACTIONTYPE yy_default[] = {
 /*     0 */  1167, 1161, 1161, 1161, 1105, 1105, 1105, 1105, 1161, 1001,
 /*    10 */  1028, 1028, 1209, 1209, 1209, 1209, 1209, 1209, 1104, 1209,
 /*    20 */  1209, 1209, 1209, 1161, 1005, 1034, 1209, 1209, 1209, 1106,
 /*    30 */  1107, 1209, 1209, 1209, 1139, 1044, 1043, 1042, 1041, 1015,
 /*    40 */  1039, 1032, 1036, 1106, 1100, 1101, 1099, 1103, 1107, 1209,
 /*    50 */  1035, 1069, 1084, 1068, 1209, 1209, 1209, 1209, 1209, 1209,
 /*    60 */  1209, 1209, 1209, 1209, 1209, 1209, 1209, 1209, 1209, 1209,
 /*    70 */  1209, 1209, 1209, 1209, 1209, 1209, 1209, 1209, 1209, 1209,
 /*    80 */  1209, 1209, 1209, 1209, 1209, 1209, 1209, 1209, 1209, 1209,
 /*    90 */  1209, 1209, 1209, 1209, 1209, 1209, 1078, 1083, 1090, 1082,
 /*   100 */  1079, 1071, 1070, 1072, 1073,  972, 1209, 1209, 1074, 1209,
 /*   110 */  1209, 1075, 1087, 1086, 1085, 1176, 1175, 1209, 1209, 1112,
 /*   120 */  1209, 1209, 1209, 1209, 1161, 1209, 1209, 1209, 1209, 1209,
 /*   130 */  1209, 1209, 1209, 1209, 1209, 1209, 1209, 1209, 1209, 1209,
 /*   140 */  1209,  930, 1209, 1209, 1209,  930, 1209, 1161, 1161, 1161,
 /*   150 */  1161, 1161, 1161, 1005,  996, 1209, 1209, 1209, 1209, 1209,
 /*   160 */  1209, 1209, 1209, 1001, 1209, 1209, 1209, 1209, 1134, 1209,
 /*   170 */  1209, 1001, 1001, 1001, 1209, 1003, 1209,  985,  995, 1209,
 /*   180 */  1158, 1209, 1155, 1209, 1126, 1038, 1017, 1017, 1209, 1209,
 /*   190 */  1208, 1038, 1209, 1208, 1209, 1209, 1209, 1209, 1209, 1209,
 /*   200 */   947, 1209, 1187, 1209,  944, 1209, 1028, 1209, 1209, 1017,
 /*   210 */  1209, 1102, 1028, 1028, 1209, 1209, 1209, 1002,  995, 1209,
 /*   220 */  1209, 1209, 1209, 1209, 1170, 1049,  975, 1038,  981,  981,
 /*   230 */   981,  981, 1138, 1205,  924, 1038, 1049,  975,  975, 1038,
 /*   240 */   924, 1113,  924,  924, 1113,  973,  973,  973,  962, 1113,
 /*   250 */   973,  947,  973,  962,  973,  973, 1021, 1016, 1021, 1016,
 /*   260 */  1021, 1016, 1021, 1016, 1108, 1209, 1113, 1117, 1117, 1113,
 /*   270 */  1033, 1022, 1031, 1029, 1038,  965, 1173, 1173, 1169, 1169,
 /*   280 */  1169, 1169,  914, 1182,  949,  949, 1182, 1209, 1209, 1209,
 /*   290 */  1177, 1120, 1209, 1209, 1209, 1209, 1209, 1209, 1209, 1209,
 /*   300 */  1209, 1209, 1209, 1209, 1209, 1209, 1209, 1209, 1209, 1209,
 /*   310 */  1055, 1209,  911, 1209, 1209, 1209, 1156, 1209, 1200, 1209,
 /*   320 */  1209, 1209, 1209, 1209, 1209, 1209, 1209, 1209, 1137, 1136,
 /*   330 */  1209, 1209, 1209, 1209, 1209, 1209, 1209, 1209, 1209, 1209,
 /*   340 */  1209, 1209, 1209, 1207, 1209, 1209, 1209, 1209, 1209, 1209,
 /*   350 */  1209, 1209, 1209, 1209, 1209, 1209, 1209, 1209, 1209, 1209,
 /*   360 */  1209, 1209, 1209, 1209, 1209, 1209, 1209, 1209, 1209,  987,
 /*   370 */  1209, 1209, 1209, 1191, 1209, 1209, 1209, 1209, 1209, 1209,
 /*   380 */  1209, 1030, 1209, 1023, 1209, 1209, 1197, 1209, 1209, 1209,
 /*   390 */  1209, 1209, 1209, 1209, 1209, 1209, 1209, 1209, 1163, 1209,
 /*   400 */  1209, 1209, 1162, 1209, 1209, 1209, 1209, 1209, 1209, 1209,
 /*   410 */   918, 1209, 1209,
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
    0,  /*    EXPLAIN => nothing */
   51,  /*      QUERY => ID */
   51,  /*       PLAN => ID */
    0,  /*         OR => nothing */
    0,  /*        AND => nothing */
    0,  /*        NOT => nothing */
    0,  /*         IS => nothing */
   51,  /*      MATCH => ID */
    0,  /*    LIKE_KW => nothing */
    0,  /*    BETWEEN => nothing */
    0,  /*         IN => nothing */
   51,  /*     ISNULL => ID */
   51,  /*    NOTNULL => ID */
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
    0,  /*      BEGIN => nothing */
    0,  /* TRANSACTION => nothing */
   51,  /*   DEFERRED => ID */
    0,  /*     COMMIT => nothing */
   51,  /*        END => ID */
    0,  /*   ROLLBACK => nothing */
    0,  /*  SAVEPOINT => nothing */
   51,  /*    RELEASE => ID */
    0,  /*         TO => nothing */
    0,  /*      TABLE => nothing */
    0,  /*     CREATE => nothing */
   51,  /*         IF => ID */
    0,  /*     EXISTS => nothing */
    0,  /*         LP => nothing */
    0,  /*         RP => nothing */
    0,  /*         AS => nothing */
    0,  /*      COMMA => nothing */
    0,  /*         ID => nothing */
   51,  /*    INDEXED => ID */
   51,  /*      ABORT => ID */
   51,  /*     ACTION => ID */
   51,  /*        ADD => ID */
   51,  /*      AFTER => ID */
   51,  /* AUTOINCREMENT => ID */
   51,  /*     BEFORE => ID */
   51,  /*    CASCADE => ID */
   51,  /*   CONFLICT => ID */
   51,  /*       FAIL => ID */
   51,  /*     IGNORE => ID */
   51,  /*  INITIALLY => ID */
   51,  /*    INSTEAD => ID */
   51,  /*         NO => ID */
   51,  /*        KEY => ID */
   51,  /*     OFFSET => ID */
   51,  /*      RAISE => ID */
   51,  /*    REPLACE => ID */
   51,  /*   RESTRICT => ID */
   51,  /*    REINDEX => ID */
   51,  /*     RENAME => ID */
   51,  /*   CTIME_KW => ID */
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
  bool is_fallback_failed;      /* Shows if fallback failed or not */
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
  "DEFERRED",      "COMMIT",        "END",           "ROLLBACK",    
  "SAVEPOINT",     "RELEASE",       "TO",            "TABLE",       
  "CREATE",        "IF",            "EXISTS",        "LP",          
  "RP",            "AS",            "COMMA",         "ID",          
  "INDEXED",       "ABORT",         "ACTION",        "ADD",         
  "AFTER",         "AUTOINCREMENT",  "BEFORE",        "CASCADE",     
  "CONFLICT",      "FAIL",          "IGNORE",        "INITIALLY",   
  "INSTEAD",       "NO",            "KEY",           "OFFSET",      
  "RAISE",         "REPLACE",       "RESTRICT",      "REINDEX",     
  "RENAME",        "CTIME_KW",      "ANY",           "STRING",      
  "CONSTRAINT",    "DEFAULT",       "NULL",          "PRIMARY",     
  "UNIQUE",        "CHECK",         "REFERENCES",    "AUTOINCR",    
  "ON",            "INSERT",        "DELETE",        "UPDATE",      
  "SET",           "DEFERRABLE",    "IMMEDIATE",     "FOREIGN",     
  "DROP",          "VIEW",          "UNION",         "ALL",         
  "EXCEPT",        "INTERSECT",     "SELECT",        "VALUES",      
  "DISTINCT",      "DOT",           "FROM",          "JOIN_KW",     
  "JOIN",          "BY",            "USING",         "ORDER",       
  "ASC",           "DESC",          "GROUP",         "HAVING",      
  "LIMIT",         "WHERE",         "INTO",          "FLOAT",       
  "BLOB",          "INTEGER",       "VARIABLE",      "CAST",        
  "CASE",          "WHEN",          "THEN",          "ELSE",        
  "INDEX",         "PRAGMA",        "TRIGGER",       "OF",          
  "FOR",           "EACH",          "ROW",           "ANALYZE",     
  "ALTER",         "WITH",          "RECURSIVE",     "error",       
  "input",         "ecmd",          "explain",       "cmdx",        
  "cmd",           "transtype",     "trans_opt",     "nm",          
  "savepoint_opt",  "create_table",  "create_table_args",  "createkw",    
  "ifnotexists",   "columnlist",    "conslist_opt",  "select",      
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
  "using_opt",     "join_nm",       "idlist",        "setlist",     
  "insert_cmd",    "idlist_opt",    "likeop",        "between_op",  
  "in_op",         "paren_exprlist",  "case_operand",  "case_exprlist",
  "case_else",     "uniqueflag",    "collate",       "nmnum",       
  "trigger_decl",  "trigger_cmd_list",  "trigger_time",  "trigger_event",
  "foreach_clause",  "when_clause",   "trigger_cmd",   "trnm",        
  "tridxby",       "wqlist",      
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
 /*   7 */ "cmd ::= COMMIT trans_opt",
 /*   8 */ "cmd ::= END trans_opt",
 /*   9 */ "cmd ::= ROLLBACK trans_opt",
 /*  10 */ "cmd ::= SAVEPOINT nm",
 /*  11 */ "cmd ::= RELEASE savepoint_opt nm",
 /*  12 */ "cmd ::= ROLLBACK trans_opt TO savepoint_opt nm",
 /*  13 */ "create_table ::= createkw TABLE ifnotexists nm",
 /*  14 */ "createkw ::= CREATE",
 /*  15 */ "ifnotexists ::=",
 /*  16 */ "ifnotexists ::= IF NOT EXISTS",
 /*  17 */ "create_table_args ::= LP columnlist conslist_opt RP",
 /*  18 */ "create_table_args ::= AS select",
 /*  19 */ "columnname ::= nm typetoken",
 /*  20 */ "nm ::= ID|INDEXED",
 /*  21 */ "typetoken ::=",
 /*  22 */ "typetoken ::= typename LP signed RP",
 /*  23 */ "typetoken ::= typename LP signed COMMA signed RP",
 /*  24 */ "typename ::= typename ID|STRING",
 /*  25 */ "ccons ::= CONSTRAINT nm",
 /*  26 */ "ccons ::= DEFAULT term",
 /*  27 */ "ccons ::= DEFAULT LP expr RP",
 /*  28 */ "ccons ::= DEFAULT PLUS term",
 /*  29 */ "ccons ::= DEFAULT MINUS term",
 /*  30 */ "ccons ::= DEFAULT ID|INDEXED",
 /*  31 */ "ccons ::= NOT NULL onconf",
 /*  32 */ "ccons ::= PRIMARY KEY sortorder onconf autoinc",
 /*  33 */ "ccons ::= UNIQUE onconf",
 /*  34 */ "ccons ::= CHECK LP expr RP",
 /*  35 */ "ccons ::= REFERENCES nm eidlist_opt refargs",
 /*  36 */ "ccons ::= defer_subclause",
 /*  37 */ "ccons ::= COLLATE ID|INDEXED",
 /*  38 */ "autoinc ::=",
 /*  39 */ "autoinc ::= AUTOINCR",
 /*  40 */ "refargs ::=",
 /*  41 */ "refargs ::= refargs refarg",
 /*  42 */ "refarg ::= MATCH nm",
 /*  43 */ "refarg ::= ON INSERT refact",
 /*  44 */ "refarg ::= ON DELETE refact",
 /*  45 */ "refarg ::= ON UPDATE refact",
 /*  46 */ "refact ::= SET NULL",
 /*  47 */ "refact ::= SET DEFAULT",
 /*  48 */ "refact ::= CASCADE",
 /*  49 */ "refact ::= RESTRICT",
 /*  50 */ "refact ::= NO ACTION",
 /*  51 */ "defer_subclause ::= NOT DEFERRABLE init_deferred_pred_opt",
 /*  52 */ "defer_subclause ::= DEFERRABLE init_deferred_pred_opt",
 /*  53 */ "init_deferred_pred_opt ::=",
 /*  54 */ "init_deferred_pred_opt ::= INITIALLY DEFERRED",
 /*  55 */ "init_deferred_pred_opt ::= INITIALLY IMMEDIATE",
 /*  56 */ "conslist_opt ::=",
 /*  57 */ "tconscomma ::= COMMA",
 /*  58 */ "tcons ::= CONSTRAINT nm",
 /*  59 */ "tcons ::= PRIMARY KEY LP sortlist autoinc RP onconf",
 /*  60 */ "tcons ::= UNIQUE LP sortlist RP onconf",
 /*  61 */ "tcons ::= CHECK LP expr RP onconf",
 /*  62 */ "tcons ::= FOREIGN KEY LP eidlist RP REFERENCES nm eidlist_opt refargs defer_subclause_opt",
 /*  63 */ "defer_subclause_opt ::=",
 /*  64 */ "onconf ::=",
 /*  65 */ "onconf ::= ON CONFLICT resolvetype",
 /*  66 */ "orconf ::=",
 /*  67 */ "orconf ::= OR resolvetype",
 /*  68 */ "resolvetype ::= IGNORE",
 /*  69 */ "resolvetype ::= REPLACE",
 /*  70 */ "cmd ::= DROP TABLE ifexists fullname",
 /*  71 */ "ifexists ::= IF EXISTS",
 /*  72 */ "ifexists ::=",
 /*  73 */ "cmd ::= createkw VIEW ifnotexists nm eidlist_opt AS select",
 /*  74 */ "cmd ::= DROP VIEW ifexists fullname",
 /*  75 */ "cmd ::= select",
 /*  76 */ "select ::= with selectnowith",
 /*  77 */ "selectnowith ::= selectnowith multiselect_op oneselect",
 /*  78 */ "multiselect_op ::= UNION",
 /*  79 */ "multiselect_op ::= UNION ALL",
 /*  80 */ "multiselect_op ::= EXCEPT|INTERSECT",
 /*  81 */ "oneselect ::= SELECT distinct selcollist from where_opt groupby_opt having_opt orderby_opt limit_opt",
 /*  82 */ "values ::= VALUES LP nexprlist RP",
 /*  83 */ "values ::= values COMMA LP exprlist RP",
 /*  84 */ "distinct ::= DISTINCT",
 /*  85 */ "distinct ::= ALL",
 /*  86 */ "distinct ::=",
 /*  87 */ "sclp ::=",
 /*  88 */ "selcollist ::= sclp expr as",
 /*  89 */ "selcollist ::= sclp STAR",
 /*  90 */ "selcollist ::= sclp nm DOT STAR",
 /*  91 */ "as ::= AS nm",
 /*  92 */ "as ::=",
 /*  93 */ "from ::=",
 /*  94 */ "from ::= FROM seltablist",
 /*  95 */ "stl_prefix ::= seltablist joinop",
 /*  96 */ "stl_prefix ::=",
 /*  97 */ "seltablist ::= stl_prefix nm as indexed_opt on_opt using_opt",
 /*  98 */ "seltablist ::= stl_prefix nm LP exprlist RP as on_opt using_opt",
 /*  99 */ "seltablist ::= stl_prefix LP select RP as on_opt using_opt",
 /* 100 */ "seltablist ::= stl_prefix LP seltablist RP as on_opt using_opt",
 /* 101 */ "fullname ::= nm",
 /* 102 */ "joinop ::= COMMA|JOIN",
 /* 103 */ "joinop ::= JOIN_KW JOIN",
 /* 104 */ "joinop ::= JOIN_KW join_nm JOIN",
 /* 105 */ "joinop ::= JOIN_KW join_nm join_nm JOIN",
 /* 106 */ "on_opt ::= ON expr",
 /* 107 */ "on_opt ::=",
 /* 108 */ "indexed_opt ::=",
 /* 109 */ "indexed_opt ::= INDEXED BY nm",
 /* 110 */ "indexed_opt ::= NOT INDEXED",
 /* 111 */ "using_opt ::= USING LP idlist RP",
 /* 112 */ "using_opt ::=",
 /* 113 */ "orderby_opt ::=",
 /* 114 */ "orderby_opt ::= ORDER BY sortlist",
 /* 115 */ "sortlist ::= sortlist COMMA expr sortorder",
 /* 116 */ "sortlist ::= expr sortorder",
 /* 117 */ "sortorder ::= ASC",
 /* 118 */ "sortorder ::= DESC",
 /* 119 */ "sortorder ::=",
 /* 120 */ "groupby_opt ::=",
 /* 121 */ "groupby_opt ::= GROUP BY nexprlist",
 /* 122 */ "having_opt ::=",
 /* 123 */ "having_opt ::= HAVING expr",
 /* 124 */ "limit_opt ::=",
 /* 125 */ "limit_opt ::= LIMIT expr",
 /* 126 */ "limit_opt ::= LIMIT expr OFFSET expr",
 /* 127 */ "limit_opt ::= LIMIT expr COMMA expr",
 /* 128 */ "cmd ::= with DELETE FROM fullname indexed_opt where_opt",
 /* 129 */ "where_opt ::=",
 /* 130 */ "where_opt ::= WHERE expr",
 /* 131 */ "cmd ::= with UPDATE orconf fullname indexed_opt SET setlist where_opt",
 /* 132 */ "setlist ::= setlist COMMA nm EQ expr",
 /* 133 */ "setlist ::= setlist COMMA LP idlist RP EQ expr",
 /* 134 */ "setlist ::= nm EQ expr",
 /* 135 */ "setlist ::= LP idlist RP EQ expr",
 /* 136 */ "cmd ::= with insert_cmd INTO fullname idlist_opt select",
 /* 137 */ "cmd ::= with insert_cmd INTO fullname idlist_opt DEFAULT VALUES",
 /* 138 */ "insert_cmd ::= INSERT orconf",
 /* 139 */ "insert_cmd ::= REPLACE",
 /* 140 */ "idlist_opt ::=",
 /* 141 */ "idlist_opt ::= LP idlist RP",
 /* 142 */ "idlist ::= idlist COMMA nm",
 /* 143 */ "idlist ::= nm",
 /* 144 */ "expr ::= LP expr RP",
 /* 145 */ "term ::= NULL",
 /* 146 */ "expr ::= ID|INDEXED",
 /* 147 */ "expr ::= JOIN_KW",
 /* 148 */ "expr ::= nm DOT nm",
 /* 149 */ "term ::= FLOAT|BLOB",
 /* 150 */ "term ::= STRING",
 /* 151 */ "term ::= INTEGER",
 /* 152 */ "expr ::= VARIABLE",
 /* 153 */ "expr ::= expr COLLATE ID|INDEXED",
 /* 154 */ "expr ::= CAST LP expr AS typetoken RP",
 /* 155 */ "expr ::= ID|INDEXED LP distinct exprlist RP",
 /* 156 */ "expr ::= ID|INDEXED LP STAR RP",
 /* 157 */ "term ::= CTIME_KW",
 /* 158 */ "expr ::= LP nexprlist COMMA expr RP",
 /* 159 */ "expr ::= expr AND expr",
 /* 160 */ "expr ::= expr OR expr",
 /* 161 */ "expr ::= expr LT|GT|GE|LE expr",
 /* 162 */ "expr ::= expr EQ|NE expr",
 /* 163 */ "expr ::= expr BITAND|BITOR|LSHIFT|RSHIFT expr",
 /* 164 */ "expr ::= expr PLUS|MINUS expr",
 /* 165 */ "expr ::= expr STAR|SLASH|REM expr",
 /* 166 */ "expr ::= expr CONCAT expr",
 /* 167 */ "likeop ::= LIKE_KW|MATCH",
 /* 168 */ "likeop ::= NOT LIKE_KW|MATCH",
 /* 169 */ "expr ::= expr likeop expr",
 /* 170 */ "expr ::= expr likeop expr ESCAPE expr",
 /* 171 */ "expr ::= expr ISNULL|NOTNULL",
 /* 172 */ "expr ::= expr NOT NULL",
 /* 173 */ "expr ::= expr IS expr",
 /* 174 */ "expr ::= expr IS NOT expr",
 /* 175 */ "expr ::= NOT expr",
 /* 176 */ "expr ::= BITNOT expr",
 /* 177 */ "expr ::= MINUS expr",
 /* 178 */ "expr ::= PLUS expr",
 /* 179 */ "between_op ::= BETWEEN",
 /* 180 */ "between_op ::= NOT BETWEEN",
 /* 181 */ "expr ::= expr between_op expr AND expr",
 /* 182 */ "in_op ::= IN",
 /* 183 */ "in_op ::= NOT IN",
 /* 184 */ "expr ::= expr in_op LP exprlist RP",
 /* 185 */ "expr ::= LP select RP",
 /* 186 */ "expr ::= expr in_op LP select RP",
 /* 187 */ "expr ::= expr in_op nm paren_exprlist",
 /* 188 */ "expr ::= EXISTS LP select RP",
 /* 189 */ "expr ::= CASE case_operand case_exprlist case_else END",
 /* 190 */ "case_exprlist ::= case_exprlist WHEN expr THEN expr",
 /* 191 */ "case_exprlist ::= WHEN expr THEN expr",
 /* 192 */ "case_else ::= ELSE expr",
 /* 193 */ "case_else ::=",
 /* 194 */ "case_operand ::= expr",
 /* 195 */ "case_operand ::=",
 /* 196 */ "exprlist ::=",
 /* 197 */ "nexprlist ::= nexprlist COMMA expr",
 /* 198 */ "nexprlist ::= expr",
 /* 199 */ "paren_exprlist ::=",
 /* 200 */ "paren_exprlist ::= LP exprlist RP",
 /* 201 */ "cmd ::= createkw uniqueflag INDEX ifnotexists nm ON nm LP sortlist RP where_opt",
 /* 202 */ "uniqueflag ::= UNIQUE",
 /* 203 */ "uniqueflag ::=",
 /* 204 */ "eidlist_opt ::=",
 /* 205 */ "eidlist_opt ::= LP eidlist RP",
 /* 206 */ "eidlist ::= eidlist COMMA nm collate sortorder",
 /* 207 */ "eidlist ::= nm collate sortorder",
 /* 208 */ "collate ::=",
 /* 209 */ "collate ::= COLLATE ID|INDEXED",
 /* 210 */ "cmd ::= DROP INDEX ifexists fullname ON nm",
 /* 211 */ "cmd ::= PRAGMA nm",
 /* 212 */ "cmd ::= PRAGMA nm EQ nmnum",
 /* 213 */ "cmd ::= PRAGMA nm LP nmnum RP",
 /* 214 */ "cmd ::= PRAGMA nm EQ minus_num",
 /* 215 */ "cmd ::= PRAGMA nm LP minus_num RP",
 /* 216 */ "cmd ::= PRAGMA nm LP nm DOT nm RP",
 /* 217 */ "cmd ::= PRAGMA",
 /* 218 */ "plus_num ::= PLUS INTEGER|FLOAT",
 /* 219 */ "minus_num ::= MINUS INTEGER|FLOAT",
 /* 220 */ "cmd ::= createkw trigger_decl BEGIN trigger_cmd_list END",
 /* 221 */ "trigger_decl ::= TRIGGER ifnotexists nm trigger_time trigger_event ON fullname foreach_clause when_clause",
 /* 222 */ "trigger_time ::= BEFORE",
 /* 223 */ "trigger_time ::= AFTER",
 /* 224 */ "trigger_time ::= INSTEAD OF",
 /* 225 */ "trigger_time ::=",
 /* 226 */ "trigger_event ::= DELETE|INSERT",
 /* 227 */ "trigger_event ::= UPDATE",
 /* 228 */ "trigger_event ::= UPDATE OF idlist",
 /* 229 */ "when_clause ::=",
 /* 230 */ "when_clause ::= WHEN expr",
 /* 231 */ "trigger_cmd_list ::= trigger_cmd_list trigger_cmd SEMI",
 /* 232 */ "trigger_cmd_list ::= trigger_cmd SEMI",
 /* 233 */ "trnm ::= nm DOT nm",
 /* 234 */ "tridxby ::= INDEXED BY nm",
 /* 235 */ "tridxby ::= NOT INDEXED",
 /* 236 */ "trigger_cmd ::= UPDATE orconf trnm tridxby SET setlist where_opt",
 /* 237 */ "trigger_cmd ::= insert_cmd INTO trnm idlist_opt select",
 /* 238 */ "trigger_cmd ::= DELETE FROM trnm tridxby where_opt",
 /* 239 */ "trigger_cmd ::= select",
 /* 240 */ "expr ::= RAISE LP IGNORE RP",
 /* 241 */ "expr ::= RAISE LP raisetype COMMA STRING RP",
 /* 242 */ "raisetype ::= ROLLBACK",
 /* 243 */ "raisetype ::= ABORT",
 /* 244 */ "raisetype ::= FAIL",
 /* 245 */ "cmd ::= DROP TRIGGER ifexists fullname",
 /* 246 */ "cmd ::= REINDEX",
 /* 247 */ "cmd ::= REINDEX nm",
 /* 248 */ "cmd ::= REINDEX nm ON nm",
 /* 249 */ "cmd ::= ANALYZE",
 /* 250 */ "cmd ::= ANALYZE nm",
 /* 251 */ "cmd ::= ALTER TABLE fullname RENAME TO nm",
 /* 252 */ "with ::=",
 /* 253 */ "with ::= WITH wqlist",
 /* 254 */ "with ::= WITH RECURSIVE wqlist",
 /* 255 */ "wqlist ::= nm eidlist_opt AS LP select RP",
 /* 256 */ "wqlist ::= wqlist COMMA nm eidlist_opt AS LP select RP",
 /* 257 */ "input ::= ecmd",
 /* 258 */ "explain ::=",
 /* 259 */ "cmdx ::= cmd",
 /* 260 */ "trans_opt ::=",
 /* 261 */ "trans_opt ::= TRANSACTION",
 /* 262 */ "trans_opt ::= TRANSACTION nm",
 /* 263 */ "savepoint_opt ::= SAVEPOINT",
 /* 264 */ "savepoint_opt ::=",
 /* 265 */ "cmd ::= create_table create_table_args",
 /* 266 */ "columnlist ::= columnlist COMMA columnname carglist",
 /* 267 */ "columnlist ::= columnname carglist",
 /* 268 */ "typetoken ::= typename",
 /* 269 */ "typename ::= ID|STRING",
 /* 270 */ "signed ::= plus_num",
 /* 271 */ "signed ::= minus_num",
 /* 272 */ "carglist ::= carglist ccons",
 /* 273 */ "carglist ::=",
 /* 274 */ "ccons ::= NULL onconf",
 /* 275 */ "conslist_opt ::= COMMA conslist",
 /* 276 */ "conslist ::= conslist tconscomma tcons",
 /* 277 */ "conslist ::= tcons",
 /* 278 */ "tconscomma ::=",
 /* 279 */ "defer_subclause_opt ::= defer_subclause",
 /* 280 */ "resolvetype ::= raisetype",
 /* 281 */ "selectnowith ::= oneselect",
 /* 282 */ "oneselect ::= values",
 /* 283 */ "sclp ::= selcollist COMMA",
 /* 284 */ "as ::= ID|STRING",
 /* 285 */ "join_nm ::= ID|INDEXED",
 /* 286 */ "join_nm ::= JOIN_KW",
 /* 287 */ "expr ::= term",
 /* 288 */ "exprlist ::= nexprlist",
 /* 289 */ "nmnum ::= plus_num",
 /* 290 */ "nmnum ::= STRING",
 /* 291 */ "nmnum ::= nm",
 /* 292 */ "nmnum ::= ON",
 /* 293 */ "nmnum ::= DELETE",
 /* 294 */ "nmnum ::= DEFAULT",
 /* 295 */ "plus_num ::= INTEGER|FLOAT",
 /* 296 */ "foreach_clause ::=",
 /* 297 */ "foreach_clause ::= FOR EACH ROW",
 /* 298 */ "trnm ::= nm",
 /* 299 */ "tridxby ::=",
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
    pParser->is_fallback_failed = false;
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
    case 151: /* select */
    case 182: /* selectnowith */
    case 183: /* oneselect */
    case 194: /* values */
{
#line 386 "parse.y"
sqlite3SelectDelete(pParse->db, (yypminor->yy279));
#line 1474 "parse.c"
}
      break;
    case 160: /* term */
    case 161: /* expr */
{
#line 829 "parse.y"
sqlite3ExprDelete(pParse->db, (yypminor->yy162).pExpr);
#line 1482 "parse.c"
}
      break;
    case 165: /* eidlist_opt */
    case 174: /* sortlist */
    case 175: /* eidlist */
    case 187: /* selcollist */
    case 190: /* groupby_opt */
    case 192: /* orderby_opt */
    case 195: /* nexprlist */
    case 196: /* exprlist */
    case 197: /* sclp */
    case 207: /* setlist */
    case 213: /* paren_exprlist */
    case 215: /* case_exprlist */
{
#line 1261 "parse.y"
sqlite3ExprListDelete(pParse->db, (yypminor->yy382));
#line 1500 "parse.c"
}
      break;
    case 181: /* fullname */
    case 188: /* from */
    case 199: /* seltablist */
    case 200: /* stl_prefix */
{
#line 613 "parse.y"
sqlite3SrcListDelete(pParse->db, (yypminor->yy387));
#line 1510 "parse.c"
}
      break;
    case 184: /* with */
    case 229: /* wqlist */
{
#line 1510 "parse.y"
sqlite3WithDelete(pParse->db, (yypminor->yy151));
#line 1518 "parse.c"
}
      break;
    case 189: /* where_opt */
    case 191: /* having_opt */
    case 203: /* on_opt */
    case 214: /* case_operand */
    case 216: /* case_else */
    case 225: /* when_clause */
{
#line 738 "parse.y"
sqlite3ExprDelete(pParse->db, (yypminor->yy362));
#line 1530 "parse.c"
}
      break;
    case 204: /* using_opt */
    case 206: /* idlist */
    case 209: /* idlist_opt */
{
#line 650 "parse.y"
sqlite3IdListDelete(pParse->db, (yypminor->yy40));
#line 1539 "parse.c"
}
      break;
    case 221: /* trigger_cmd_list */
    case 226: /* trigger_cmd */
{
#line 1384 "parse.y"
sqlite3DeleteTriggerStep(pParse->db, (yypminor->yy427));
#line 1547 "parse.c"
}
      break;
    case 223: /* trigger_event */
{
#line 1370 "parse.y"
sqlite3IdListDelete(pParse->db, (yypminor->yy10).b);
#line 1554 "parse.c"
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
      YYCODETYPE iFallback = -1;            /* Fallback token */
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
      } else if ( iFallback==0 ) {
        pParser->is_fallback_failed = true;
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
#line 41 "parse.y"

  sqlite3ErrorMsg(pParse, "parser stack overflow");
#line 1729 "parse.c"
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
  { 137, 3 },
  { 137, 1 },
  { 138, 1 },
  { 138, 3 },
  { 140, 3 },
  { 141, 0 },
  { 141, 1 },
  { 140, 2 },
  { 140, 2 },
  { 140, 2 },
  { 140, 2 },
  { 140, 3 },
  { 140, 5 },
  { 145, 4 },
  { 147, 1 },
  { 148, 0 },
  { 148, 3 },
  { 146, 4 },
  { 146, 2 },
  { 152, 2 },
  { 143, 1 },
  { 154, 0 },
  { 154, 4 },
  { 154, 6 },
  { 155, 2 },
  { 159, 2 },
  { 159, 2 },
  { 159, 4 },
  { 159, 3 },
  { 159, 3 },
  { 159, 2 },
  { 159, 3 },
  { 159, 5 },
  { 159, 2 },
  { 159, 4 },
  { 159, 4 },
  { 159, 1 },
  { 159, 2 },
  { 164, 0 },
  { 164, 1 },
  { 166, 0 },
  { 166, 2 },
  { 168, 2 },
  { 168, 3 },
  { 168, 3 },
  { 168, 3 },
  { 169, 2 },
  { 169, 2 },
  { 169, 1 },
  { 169, 1 },
  { 169, 2 },
  { 167, 3 },
  { 167, 2 },
  { 170, 0 },
  { 170, 2 },
  { 170, 2 },
  { 150, 0 },
  { 172, 1 },
  { 173, 2 },
  { 173, 7 },
  { 173, 5 },
  { 173, 5 },
  { 173, 10 },
  { 176, 0 },
  { 162, 0 },
  { 162, 3 },
  { 177, 0 },
  { 177, 2 },
  { 178, 1 },
  { 178, 1 },
  { 140, 4 },
  { 180, 2 },
  { 180, 0 },
  { 140, 7 },
  { 140, 4 },
  { 140, 1 },
  { 151, 2 },
  { 182, 3 },
  { 185, 1 },
  { 185, 2 },
  { 185, 1 },
  { 183, 9 },
  { 194, 4 },
  { 194, 5 },
  { 186, 1 },
  { 186, 1 },
  { 186, 0 },
  { 197, 0 },
  { 187, 3 },
  { 187, 2 },
  { 187, 4 },
  { 198, 2 },
  { 198, 0 },
  { 188, 0 },
  { 188, 2 },
  { 200, 2 },
  { 200, 0 },
  { 199, 6 },
  { 199, 8 },
  { 199, 7 },
  { 199, 7 },
  { 181, 1 },
  { 201, 1 },
  { 201, 2 },
  { 201, 3 },
  { 201, 4 },
  { 203, 2 },
  { 203, 0 },
  { 202, 0 },
  { 202, 3 },
  { 202, 2 },
  { 204, 4 },
  { 204, 0 },
  { 192, 0 },
  { 192, 3 },
  { 174, 4 },
  { 174, 2 },
  { 163, 1 },
  { 163, 1 },
  { 163, 0 },
  { 190, 0 },
  { 190, 3 },
  { 191, 0 },
  { 191, 2 },
  { 193, 0 },
  { 193, 2 },
  { 193, 4 },
  { 193, 4 },
  { 140, 6 },
  { 189, 0 },
  { 189, 2 },
  { 140, 8 },
  { 207, 5 },
  { 207, 7 },
  { 207, 3 },
  { 207, 5 },
  { 140, 6 },
  { 140, 7 },
  { 208, 2 },
  { 208, 1 },
  { 209, 0 },
  { 209, 3 },
  { 206, 3 },
  { 206, 1 },
  { 161, 3 },
  { 160, 1 },
  { 161, 1 },
  { 161, 1 },
  { 161, 3 },
  { 160, 1 },
  { 160, 1 },
  { 160, 1 },
  { 161, 1 },
  { 161, 3 },
  { 161, 6 },
  { 161, 5 },
  { 161, 4 },
  { 160, 1 },
  { 161, 5 },
  { 161, 3 },
  { 161, 3 },
  { 161, 3 },
  { 161, 3 },
  { 161, 3 },
  { 161, 3 },
  { 161, 3 },
  { 161, 3 },
  { 210, 1 },
  { 210, 2 },
  { 161, 3 },
  { 161, 5 },
  { 161, 2 },
  { 161, 3 },
  { 161, 3 },
  { 161, 4 },
  { 161, 2 },
  { 161, 2 },
  { 161, 2 },
  { 161, 2 },
  { 211, 1 },
  { 211, 2 },
  { 161, 5 },
  { 212, 1 },
  { 212, 2 },
  { 161, 5 },
  { 161, 3 },
  { 161, 5 },
  { 161, 4 },
  { 161, 4 },
  { 161, 5 },
  { 215, 5 },
  { 215, 4 },
  { 216, 2 },
  { 216, 0 },
  { 214, 1 },
  { 214, 0 },
  { 196, 0 },
  { 195, 3 },
  { 195, 1 },
  { 213, 0 },
  { 213, 3 },
  { 140, 11 },
  { 217, 1 },
  { 217, 0 },
  { 165, 0 },
  { 165, 3 },
  { 175, 5 },
  { 175, 3 },
  { 218, 0 },
  { 218, 2 },
  { 140, 6 },
  { 140, 2 },
  { 140, 4 },
  { 140, 5 },
  { 140, 4 },
  { 140, 5 },
  { 140, 7 },
  { 140, 1 },
  { 157, 2 },
  { 158, 2 },
  { 140, 5 },
  { 220, 9 },
  { 222, 1 },
  { 222, 1 },
  { 222, 2 },
  { 222, 0 },
  { 223, 1 },
  { 223, 1 },
  { 223, 3 },
  { 225, 0 },
  { 225, 2 },
  { 221, 3 },
  { 221, 2 },
  { 227, 3 },
  { 228, 3 },
  { 228, 2 },
  { 226, 7 },
  { 226, 5 },
  { 226, 5 },
  { 226, 1 },
  { 161, 4 },
  { 161, 6 },
  { 179, 1 },
  { 179, 1 },
  { 179, 1 },
  { 140, 4 },
  { 140, 1 },
  { 140, 2 },
  { 140, 4 },
  { 140, 1 },
  { 140, 2 },
  { 140, 6 },
  { 184, 0 },
  { 184, 2 },
  { 184, 3 },
  { 229, 6 },
  { 229, 8 },
  { 136, 1 },
  { 138, 0 },
  { 139, 1 },
  { 142, 0 },
  { 142, 1 },
  { 142, 2 },
  { 144, 1 },
  { 144, 0 },
  { 140, 2 },
  { 149, 4 },
  { 149, 2 },
  { 154, 1 },
  { 155, 1 },
  { 156, 1 },
  { 156, 1 },
  { 153, 2 },
  { 153, 0 },
  { 159, 2 },
  { 150, 2 },
  { 171, 3 },
  { 171, 1 },
  { 172, 0 },
  { 176, 1 },
  { 178, 1 },
  { 182, 1 },
  { 183, 1 },
  { 197, 2 },
  { 198, 1 },
  { 205, 1 },
  { 205, 1 },
  { 161, 1 },
  { 196, 1 },
  { 219, 1 },
  { 219, 1 },
  { 219, 1 },
  { 219, 1 },
  { 219, 1 },
  { 219, 1 },
  { 157, 1 },
  { 224, 0 },
  { 224, 3 },
  { 227, 1 },
  { 228, 0 },
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
#line 111 "parse.y"
{ sqlite3FinishCoding(pParse); }
#line 2169 "parse.c"
        break;
      case 1: /* ecmd ::= SEMI */
#line 112 "parse.y"
{
  sqlite3ErrorMsg(pParse, "syntax error: empty request");
}
#line 2176 "parse.c"
        break;
      case 2: /* explain ::= EXPLAIN */
#line 117 "parse.y"
{ pParse->explain = 1; }
#line 2181 "parse.c"
        break;
      case 3: /* explain ::= EXPLAIN QUERY PLAN */
#line 118 "parse.y"
{ pParse->explain = 2; }
#line 2186 "parse.c"
        break;
      case 4: /* cmd ::= BEGIN transtype trans_opt */
#line 150 "parse.y"
{sqlite3BeginTransaction(pParse, yymsp[-1].minor.yy52);}
#line 2191 "parse.c"
        break;
      case 5: /* transtype ::= */
#line 155 "parse.y"
{yymsp[1].minor.yy52 = TK_DEFERRED;}
#line 2196 "parse.c"
        break;
      case 6: /* transtype ::= DEFERRED */
#line 156 "parse.y"
{yymsp[0].minor.yy52 = yymsp[0].major; /*A-overwrites-X*/}
#line 2201 "parse.c"
        break;
      case 7: /* cmd ::= COMMIT trans_opt */
      case 8: /* cmd ::= END trans_opt */ yytestcase(yyruleno==8);
#line 157 "parse.y"
{sqlite3CommitTransaction(pParse);}
#line 2207 "parse.c"
        break;
      case 9: /* cmd ::= ROLLBACK trans_opt */
#line 159 "parse.y"
{sqlite3RollbackTransaction(pParse);}
#line 2212 "parse.c"
        break;
      case 10: /* cmd ::= SAVEPOINT nm */
#line 163 "parse.y"
{
  sqlite3Savepoint(pParse, SAVEPOINT_BEGIN, &yymsp[0].minor.yy0);
}
#line 2219 "parse.c"
        break;
      case 11: /* cmd ::= RELEASE savepoint_opt nm */
#line 166 "parse.y"
{
  sqlite3Savepoint(pParse, SAVEPOINT_RELEASE, &yymsp[0].minor.yy0);
}
#line 2226 "parse.c"
        break;
      case 12: /* cmd ::= ROLLBACK trans_opt TO savepoint_opt nm */
#line 169 "parse.y"
{
  sqlite3Savepoint(pParse, SAVEPOINT_ROLLBACK, &yymsp[0].minor.yy0);
}
#line 2233 "parse.c"
        break;
      case 13: /* create_table ::= createkw TABLE ifnotexists nm */
#line 176 "parse.y"
{
   sqlite3StartTable(pParse,&yymsp[0].minor.yy0,yymsp[-1].minor.yy52);
}
#line 2240 "parse.c"
        break;
      case 14: /* createkw ::= CREATE */
#line 179 "parse.y"
{disableLookaside(pParse);}
#line 2245 "parse.c"
        break;
      case 15: /* ifnotexists ::= */
      case 38: /* autoinc ::= */ yytestcase(yyruleno==38);
      case 53: /* init_deferred_pred_opt ::= */ yytestcase(yyruleno==53);
      case 63: /* defer_subclause_opt ::= */ yytestcase(yyruleno==63);
      case 72: /* ifexists ::= */ yytestcase(yyruleno==72);
      case 86: /* distinct ::= */ yytestcase(yyruleno==86);
      case 208: /* collate ::= */ yytestcase(yyruleno==208);
#line 182 "parse.y"
{yymsp[1].minor.yy52 = 0;}
#line 2256 "parse.c"
        break;
      case 16: /* ifnotexists ::= IF NOT EXISTS */
#line 183 "parse.y"
{yymsp[-2].minor.yy52 = 1;}
#line 2261 "parse.c"
        break;
      case 17: /* create_table_args ::= LP columnlist conslist_opt RP */
#line 185 "parse.y"
{
  sqlite3EndTable(pParse,&yymsp[-1].minor.yy0,&yymsp[0].minor.yy0,0,0);
}
#line 2268 "parse.c"
        break;
      case 18: /* create_table_args ::= AS select */
#line 188 "parse.y"
{
  sqlite3EndTable(pParse,0,0,0,yymsp[0].minor.yy279);
  sqlite3SelectDelete(pParse->db, yymsp[0].minor.yy279);
}
#line 2276 "parse.c"
        break;
      case 19: /* columnname ::= nm typetoken */
#line 194 "parse.y"
{sqlite3AddColumn(pParse,&yymsp[-1].minor.yy0,&yymsp[0].minor.yy0);}
#line 2281 "parse.c"
        break;
      case 20: /* nm ::= ID|INDEXED */
#line 225 "parse.y"
{
  if(yymsp[0].minor.yy0.isReserved) {
    sqlite3ErrorMsg(pParse, "keyword \"%T\" is reserved", &yymsp[0].minor.yy0);
  }
}
#line 2290 "parse.c"
        break;
      case 21: /* typetoken ::= */
      case 56: /* conslist_opt ::= */ yytestcase(yyruleno==56);
      case 92: /* as ::= */ yytestcase(yyruleno==92);
#line 236 "parse.y"
{yymsp[1].minor.yy0.n = 0; yymsp[1].minor.yy0.z = 0;}
#line 2297 "parse.c"
        break;
      case 22: /* typetoken ::= typename LP signed RP */
#line 238 "parse.y"
{
  yymsp[-3].minor.yy0.n = (int)(&yymsp[0].minor.yy0.z[yymsp[0].minor.yy0.n] - yymsp[-3].minor.yy0.z);
}
#line 2304 "parse.c"
        break;
      case 23: /* typetoken ::= typename LP signed COMMA signed RP */
#line 241 "parse.y"
{
  yymsp[-5].minor.yy0.n = (int)(&yymsp[0].minor.yy0.z[yymsp[0].minor.yy0.n] - yymsp[-5].minor.yy0.z);
}
#line 2311 "parse.c"
        break;
      case 24: /* typename ::= typename ID|STRING */
#line 246 "parse.y"
{yymsp[-1].minor.yy0.n=yymsp[0].minor.yy0.n+(int)(yymsp[0].minor.yy0.z-yymsp[-1].minor.yy0.z);}
#line 2316 "parse.c"
        break;
      case 25: /* ccons ::= CONSTRAINT nm */
      case 58: /* tcons ::= CONSTRAINT nm */ yytestcase(yyruleno==58);
#line 255 "parse.y"
{pParse->constraintName = yymsp[0].minor.yy0;}
#line 2322 "parse.c"
        break;
      case 26: /* ccons ::= DEFAULT term */
      case 28: /* ccons ::= DEFAULT PLUS term */ yytestcase(yyruleno==28);
#line 256 "parse.y"
{sqlite3AddDefaultValue(pParse,&yymsp[0].minor.yy162);}
#line 2328 "parse.c"
        break;
      case 27: /* ccons ::= DEFAULT LP expr RP */
#line 257 "parse.y"
{sqlite3AddDefaultValue(pParse,&yymsp[-1].minor.yy162);}
#line 2333 "parse.c"
        break;
      case 29: /* ccons ::= DEFAULT MINUS term */
#line 259 "parse.y"
{
  ExprSpan v;
  v.pExpr = sqlite3PExpr(pParse, TK_UMINUS, yymsp[0].minor.yy162.pExpr, 0);
  v.zStart = yymsp[-1].minor.yy0.z;
  v.zEnd = yymsp[0].minor.yy162.zEnd;
  sqlite3AddDefaultValue(pParse,&v);
}
#line 2344 "parse.c"
        break;
      case 30: /* ccons ::= DEFAULT ID|INDEXED */
#line 266 "parse.y"
{
  ExprSpan v;
  spanExpr(&v, pParse, TK_STRING, yymsp[0].minor.yy0);
  sqlite3AddDefaultValue(pParse,&v);
}
#line 2353 "parse.c"
        break;
      case 31: /* ccons ::= NOT NULL onconf */
#line 276 "parse.y"
{sqlite3AddNotNull(pParse, yymsp[0].minor.yy52);}
#line 2358 "parse.c"
        break;
      case 32: /* ccons ::= PRIMARY KEY sortorder onconf autoinc */
#line 278 "parse.y"
{sqlite3AddPrimaryKey(pParse,0,yymsp[-1].minor.yy52,yymsp[0].minor.yy52,yymsp[-2].minor.yy52);}
#line 2363 "parse.c"
        break;
      case 33: /* ccons ::= UNIQUE onconf */
#line 279 "parse.y"
{sqlite3CreateIndex(pParse,0,0,0,yymsp[0].minor.yy52,0,0,0,0,
                                   SQLITE_IDXTYPE_UNIQUE);}
#line 2369 "parse.c"
        break;
      case 34: /* ccons ::= CHECK LP expr RP */
#line 281 "parse.y"
{sqlite3AddCheckConstraint(pParse,yymsp[-1].minor.yy162.pExpr);}
#line 2374 "parse.c"
        break;
      case 35: /* ccons ::= REFERENCES nm eidlist_opt refargs */
#line 283 "parse.y"
{sqlite3CreateForeignKey(pParse,0,&yymsp[-2].minor.yy0,yymsp[-1].minor.yy382,yymsp[0].minor.yy52);}
#line 2379 "parse.c"
        break;
      case 36: /* ccons ::= defer_subclause */
#line 284 "parse.y"
{sqlite3DeferForeignKey(pParse,yymsp[0].minor.yy52);}
#line 2384 "parse.c"
        break;
      case 37: /* ccons ::= COLLATE ID|INDEXED */
#line 285 "parse.y"
{sqlite3AddCollateType(pParse, &yymsp[0].minor.yy0);}
#line 2389 "parse.c"
        break;
      case 39: /* autoinc ::= AUTOINCR */
#line 290 "parse.y"
{yymsp[0].minor.yy52 = 1;}
#line 2394 "parse.c"
        break;
      case 40: /* refargs ::= */
#line 298 "parse.y"
{ yymsp[1].minor.yy52 = ON_CONFLICT_ACTION_NONE*0x0101; /* EV: R-19803-45884 */}
#line 2399 "parse.c"
        break;
      case 41: /* refargs ::= refargs refarg */
#line 299 "parse.y"
{ yymsp[-1].minor.yy52 = (yymsp[-1].minor.yy52 & ~yymsp[0].minor.yy107.mask) | yymsp[0].minor.yy107.value; }
#line 2404 "parse.c"
        break;
      case 42: /* refarg ::= MATCH nm */
#line 301 "parse.y"
{ yymsp[-1].minor.yy107.value = 0;     yymsp[-1].minor.yy107.mask = 0x000000; }
#line 2409 "parse.c"
        break;
      case 43: /* refarg ::= ON INSERT refact */
#line 302 "parse.y"
{ yymsp[-2].minor.yy107.value = 0;     yymsp[-2].minor.yy107.mask = 0x000000; }
#line 2414 "parse.c"
        break;
      case 44: /* refarg ::= ON DELETE refact */
#line 303 "parse.y"
{ yymsp[-2].minor.yy107.value = yymsp[0].minor.yy52;     yymsp[-2].minor.yy107.mask = 0x0000ff; }
#line 2419 "parse.c"
        break;
      case 45: /* refarg ::= ON UPDATE refact */
#line 304 "parse.y"
{ yymsp[-2].minor.yy107.value = yymsp[0].minor.yy52<<8;  yymsp[-2].minor.yy107.mask = 0x00ff00; }
#line 2424 "parse.c"
        break;
      case 46: /* refact ::= SET NULL */
#line 306 "parse.y"
{ yymsp[-1].minor.yy52 = OE_SetNull;  /* EV: R-33326-45252 */}
#line 2429 "parse.c"
        break;
      case 47: /* refact ::= SET DEFAULT */
#line 307 "parse.y"
{ yymsp[-1].minor.yy52 = OE_SetDflt;  /* EV: R-33326-45252 */}
#line 2434 "parse.c"
        break;
      case 48: /* refact ::= CASCADE */
#line 308 "parse.y"
{ yymsp[0].minor.yy52 = OE_Cascade;  /* EV: R-33326-45252 */}
#line 2439 "parse.c"
        break;
      case 49: /* refact ::= RESTRICT */
#line 309 "parse.y"
{ yymsp[0].minor.yy52 = OE_Restrict; /* EV: R-33326-45252 */}
#line 2444 "parse.c"
        break;
      case 50: /* refact ::= NO ACTION */
#line 310 "parse.y"
{ yymsp[-1].minor.yy52 = ON_CONFLICT_ACTION_NONE;     /* EV: R-33326-45252 */}
#line 2449 "parse.c"
        break;
      case 51: /* defer_subclause ::= NOT DEFERRABLE init_deferred_pred_opt */
#line 312 "parse.y"
{yymsp[-2].minor.yy52 = 0;}
#line 2454 "parse.c"
        break;
      case 52: /* defer_subclause ::= DEFERRABLE init_deferred_pred_opt */
      case 67: /* orconf ::= OR resolvetype */ yytestcase(yyruleno==67);
      case 138: /* insert_cmd ::= INSERT orconf */ yytestcase(yyruleno==138);
#line 313 "parse.y"
{yymsp[-1].minor.yy52 = yymsp[0].minor.yy52;}
#line 2461 "parse.c"
        break;
      case 54: /* init_deferred_pred_opt ::= INITIALLY DEFERRED */
      case 71: /* ifexists ::= IF EXISTS */ yytestcase(yyruleno==71);
      case 180: /* between_op ::= NOT BETWEEN */ yytestcase(yyruleno==180);
      case 183: /* in_op ::= NOT IN */ yytestcase(yyruleno==183);
      case 209: /* collate ::= COLLATE ID|INDEXED */ yytestcase(yyruleno==209);
#line 316 "parse.y"
{yymsp[-1].minor.yy52 = 1;}
#line 2470 "parse.c"
        break;
      case 55: /* init_deferred_pred_opt ::= INITIALLY IMMEDIATE */
#line 317 "parse.y"
{yymsp[-1].minor.yy52 = 0;}
#line 2475 "parse.c"
        break;
      case 57: /* tconscomma ::= COMMA */
#line 323 "parse.y"
{pParse->constraintName.n = 0;}
#line 2480 "parse.c"
        break;
      case 59: /* tcons ::= PRIMARY KEY LP sortlist autoinc RP onconf */
#line 327 "parse.y"
{sqlite3AddPrimaryKey(pParse,yymsp[-3].minor.yy382,yymsp[0].minor.yy52,yymsp[-2].minor.yy52,0);}
#line 2485 "parse.c"
        break;
      case 60: /* tcons ::= UNIQUE LP sortlist RP onconf */
#line 329 "parse.y"
{sqlite3CreateIndex(pParse,0,0,yymsp[-2].minor.yy382,yymsp[0].minor.yy52,0,0,0,0,
                                       SQLITE_IDXTYPE_UNIQUE);}
#line 2491 "parse.c"
        break;
      case 61: /* tcons ::= CHECK LP expr RP onconf */
#line 332 "parse.y"
{sqlite3AddCheckConstraint(pParse,yymsp[-2].minor.yy162.pExpr);}
#line 2496 "parse.c"
        break;
      case 62: /* tcons ::= FOREIGN KEY LP eidlist RP REFERENCES nm eidlist_opt refargs defer_subclause_opt */
#line 334 "parse.y"
{
    sqlite3CreateForeignKey(pParse, yymsp[-6].minor.yy382, &yymsp[-3].minor.yy0, yymsp[-2].minor.yy382, yymsp[-1].minor.yy52);
    sqlite3DeferForeignKey(pParse, yymsp[0].minor.yy52);
}
#line 2504 "parse.c"
        break;
      case 64: /* onconf ::= */
      case 66: /* orconf ::= */ yytestcase(yyruleno==66);
#line 348 "parse.y"
{yymsp[1].minor.yy52 = ON_CONFLICT_ACTION_DEFAULT;}
#line 2510 "parse.c"
        break;
      case 65: /* onconf ::= ON CONFLICT resolvetype */
#line 349 "parse.y"
{yymsp[-2].minor.yy52 = yymsp[0].minor.yy52;}
#line 2515 "parse.c"
        break;
      case 68: /* resolvetype ::= IGNORE */
#line 353 "parse.y"
{yymsp[0].minor.yy52 = ON_CONFLICT_ACTION_IGNORE;}
#line 2520 "parse.c"
        break;
      case 69: /* resolvetype ::= REPLACE */
      case 139: /* insert_cmd ::= REPLACE */ yytestcase(yyruleno==139);
#line 354 "parse.y"
{yymsp[0].minor.yy52 = ON_CONFLICT_ACTION_REPLACE;}
#line 2526 "parse.c"
        break;
      case 70: /* cmd ::= DROP TABLE ifexists fullname */
#line 358 "parse.y"
{
  sqlite3DropTable(pParse, yymsp[0].minor.yy387, 0, yymsp[-1].minor.yy52);
}
#line 2533 "parse.c"
        break;
      case 73: /* cmd ::= createkw VIEW ifnotexists nm eidlist_opt AS select */
#line 369 "parse.y"
{
  sqlite3CreateView(pParse, &yymsp[-6].minor.yy0, &yymsp[-3].minor.yy0, yymsp[-2].minor.yy382, yymsp[0].minor.yy279, yymsp[-4].minor.yy52);
}
#line 2540 "parse.c"
        break;
      case 74: /* cmd ::= DROP VIEW ifexists fullname */
#line 372 "parse.y"
{
  sqlite3DropTable(pParse, yymsp[0].minor.yy387, 1, yymsp[-1].minor.yy52);
}
#line 2547 "parse.c"
        break;
      case 75: /* cmd ::= select */
#line 379 "parse.y"
{
  SelectDest dest = {SRT_Output, 0, 0, 0, 0, 0};
  sqlite3Select(pParse, yymsp[0].minor.yy279, &dest);
  sqlite3SelectDelete(pParse->db, yymsp[0].minor.yy279);
}
#line 2556 "parse.c"
        break;
      case 76: /* select ::= with selectnowith */
#line 416 "parse.y"
{
  Select *p = yymsp[0].minor.yy279;
  if( p ){
    p->pWith = yymsp[-1].minor.yy151;
    parserDoubleLinkSelect(pParse, p);
  }else{
    sqlite3WithDelete(pParse->db, yymsp[-1].minor.yy151);
  }
  yymsp[-1].minor.yy279 = p; /*A-overwrites-W*/
}
#line 2570 "parse.c"
        break;
      case 77: /* selectnowith ::= selectnowith multiselect_op oneselect */
#line 429 "parse.y"
{
  Select *pRhs = yymsp[0].minor.yy279;
  Select *pLhs = yymsp[-2].minor.yy279;
  if( pRhs && pRhs->pPrior ){
    SrcList *pFrom;
    Token x;
    x.n = 0;
    parserDoubleLinkSelect(pParse, pRhs);
    pFrom = sqlite3SrcListAppendFromTerm(pParse,0,0,&x,pRhs,0,0);
    pRhs = sqlite3SelectNew(pParse,0,pFrom,0,0,0,0,0,0,0);
  }
  if( pRhs ){
    pRhs->op = (u8)yymsp[-1].minor.yy52;
    pRhs->pPrior = pLhs;
    if( ALWAYS(pLhs) ) pLhs->selFlags &= ~SF_MultiValue;
    pRhs->selFlags &= ~SF_MultiValue;
    if( yymsp[-1].minor.yy52!=TK_ALL ) pParse->hasCompound = 1;
  }else{
    sqlite3SelectDelete(pParse->db, pLhs);
  }
  yymsp[-2].minor.yy279 = pRhs;
}
#line 2596 "parse.c"
        break;
      case 78: /* multiselect_op ::= UNION */
      case 80: /* multiselect_op ::= EXCEPT|INTERSECT */ yytestcase(yyruleno==80);
#line 452 "parse.y"
{yymsp[0].minor.yy52 = yymsp[0].major; /*A-overwrites-OP*/}
#line 2602 "parse.c"
        break;
      case 79: /* multiselect_op ::= UNION ALL */
#line 453 "parse.y"
{yymsp[-1].minor.yy52 = TK_ALL;}
#line 2607 "parse.c"
        break;
      case 81: /* oneselect ::= SELECT distinct selcollist from where_opt groupby_opt having_opt orderby_opt limit_opt */
#line 457 "parse.y"
{
#ifdef SELECTTRACE_ENABLED
  Token s = yymsp[-8].minor.yy0; /*A-overwrites-S*/
#endif
  yymsp[-8].minor.yy279 = sqlite3SelectNew(pParse,yymsp[-6].minor.yy382,yymsp[-5].minor.yy387,yymsp[-4].minor.yy362,yymsp[-3].minor.yy382,yymsp[-2].minor.yy362,yymsp[-1].minor.yy382,yymsp[-7].minor.yy52,yymsp[0].minor.yy384.pLimit,yymsp[0].minor.yy384.pOffset);
#ifdef SELECTTRACE_ENABLED
  /* Populate the Select.zSelName[] string that is used to help with
  ** query planner debugging, to differentiate between multiple Select
  ** objects in a complex query.
  **
  ** If the SELECT keyword is immediately followed by a C-style comment
  ** then extract the first few alphanumeric characters from within that
  ** comment to be the zSelName value.  Otherwise, the label is #N where
  ** is an integer that is incremented with each SELECT statement seen.
  */
  if( yymsp[-8].minor.yy279!=0 ){
    const char *z = s.z+6;
    int i;
    sqlite3_snprintf(sizeof(yymsp[-8].minor.yy279->zSelName), yymsp[-8].minor.yy279->zSelName, "#%d",
                     ++pParse->nSelect);
    while( z[0]==' ' ) z++;
    if( z[0]=='/' && z[1]=='*' ){
      z += 2;
      while( z[0]==' ' ) z++;
      for(i=0; sqlite3Isalnum(z[i]); i++){}
      sqlite3_snprintf(sizeof(yymsp[-8].minor.yy279->zSelName), yymsp[-8].minor.yy279->zSelName, "%.*s", i, z);
    }
  }
#endif /* SELECTRACE_ENABLED */
}
#line 2641 "parse.c"
        break;
      case 82: /* values ::= VALUES LP nexprlist RP */
#line 491 "parse.y"
{
  yymsp[-3].minor.yy279 = sqlite3SelectNew(pParse,yymsp[-1].minor.yy382,0,0,0,0,0,SF_Values,0,0);
}
#line 2648 "parse.c"
        break;
      case 83: /* values ::= values COMMA LP exprlist RP */
#line 494 "parse.y"
{
  Select *pRight, *pLeft = yymsp[-4].minor.yy279;
  pRight = sqlite3SelectNew(pParse,yymsp[-1].minor.yy382,0,0,0,0,0,SF_Values|SF_MultiValue,0,0);
  if( ALWAYS(pLeft) ) pLeft->selFlags &= ~SF_MultiValue;
  if( pRight ){
    pRight->op = TK_ALL;
    pRight->pPrior = pLeft;
    yymsp[-4].minor.yy279 = pRight;
  }else{
    yymsp[-4].minor.yy279 = pLeft;
  }
}
#line 2664 "parse.c"
        break;
      case 84: /* distinct ::= DISTINCT */
#line 511 "parse.y"
{yymsp[0].minor.yy52 = SF_Distinct;}
#line 2669 "parse.c"
        break;
      case 85: /* distinct ::= ALL */
#line 512 "parse.y"
{yymsp[0].minor.yy52 = SF_All;}
#line 2674 "parse.c"
        break;
      case 87: /* sclp ::= */
      case 113: /* orderby_opt ::= */ yytestcase(yyruleno==113);
      case 120: /* groupby_opt ::= */ yytestcase(yyruleno==120);
      case 196: /* exprlist ::= */ yytestcase(yyruleno==196);
      case 199: /* paren_exprlist ::= */ yytestcase(yyruleno==199);
      case 204: /* eidlist_opt ::= */ yytestcase(yyruleno==204);
#line 525 "parse.y"
{yymsp[1].minor.yy382 = 0;}
#line 2684 "parse.c"
        break;
      case 88: /* selcollist ::= sclp expr as */
#line 526 "parse.y"
{
   yymsp[-2].minor.yy382 = sqlite3ExprListAppend(pParse, yymsp[-2].minor.yy382, yymsp[-1].minor.yy162.pExpr);
   if( yymsp[0].minor.yy0.n>0 ) sqlite3ExprListSetName(pParse, yymsp[-2].minor.yy382, &yymsp[0].minor.yy0, 1);
   sqlite3ExprListSetSpan(pParse,yymsp[-2].minor.yy382,&yymsp[-1].minor.yy162);
}
#line 2693 "parse.c"
        break;
      case 89: /* selcollist ::= sclp STAR */
#line 531 "parse.y"
{
  Expr *p = sqlite3Expr(pParse->db, TK_ASTERISK, 0);
  yymsp[-1].minor.yy382 = sqlite3ExprListAppend(pParse, yymsp[-1].minor.yy382, p);
}
#line 2701 "parse.c"
        break;
      case 90: /* selcollist ::= sclp nm DOT STAR */
#line 535 "parse.y"
{
  Expr *pRight = sqlite3PExpr(pParse, TK_ASTERISK, 0, 0);
  Expr *pLeft = sqlite3ExprAlloc(pParse->db, TK_ID, &yymsp[-2].minor.yy0, 1);
  Expr *pDot = sqlite3PExpr(pParse, TK_DOT, pLeft, pRight);
  yymsp[-3].minor.yy382 = sqlite3ExprListAppend(pParse,yymsp[-3].minor.yy382, pDot);
}
#line 2711 "parse.c"
        break;
      case 91: /* as ::= AS nm */
      case 218: /* plus_num ::= PLUS INTEGER|FLOAT */ yytestcase(yyruleno==218);
      case 219: /* minus_num ::= MINUS INTEGER|FLOAT */ yytestcase(yyruleno==219);
#line 546 "parse.y"
{yymsp[-1].minor.yy0 = yymsp[0].minor.yy0;}
#line 2718 "parse.c"
        break;
      case 93: /* from ::= */
#line 560 "parse.y"
{yymsp[1].minor.yy387 = sqlite3DbMallocZero(pParse->db, sizeof(*yymsp[1].minor.yy387));}
#line 2723 "parse.c"
        break;
      case 94: /* from ::= FROM seltablist */
#line 561 "parse.y"
{
  yymsp[-1].minor.yy387 = yymsp[0].minor.yy387;
  sqlite3SrcListShiftJoinType(yymsp[-1].minor.yy387);
}
#line 2731 "parse.c"
        break;
      case 95: /* stl_prefix ::= seltablist joinop */
#line 569 "parse.y"
{
   if( ALWAYS(yymsp[-1].minor.yy387 && yymsp[-1].minor.yy387->nSrc>0) ) yymsp[-1].minor.yy387->a[yymsp[-1].minor.yy387->nSrc-1].fg.jointype = (u8)yymsp[0].minor.yy52;
}
#line 2738 "parse.c"
        break;
      case 96: /* stl_prefix ::= */
#line 572 "parse.y"
{yymsp[1].minor.yy387 = 0;}
#line 2743 "parse.c"
        break;
      case 97: /* seltablist ::= stl_prefix nm as indexed_opt on_opt using_opt */
#line 574 "parse.y"
{
  yymsp[-5].minor.yy387 = sqlite3SrcListAppendFromTerm(pParse,yymsp[-5].minor.yy387,&yymsp[-4].minor.yy0,&yymsp[-3].minor.yy0,0,yymsp[-1].minor.yy362,yymsp[0].minor.yy40);
  sqlite3SrcListIndexedBy(pParse, yymsp[-5].minor.yy387, &yymsp[-2].minor.yy0);
}
#line 2751 "parse.c"
        break;
      case 98: /* seltablist ::= stl_prefix nm LP exprlist RP as on_opt using_opt */
#line 579 "parse.y"
{
  yymsp[-7].minor.yy387 = sqlite3SrcListAppendFromTerm(pParse,yymsp[-7].minor.yy387,&yymsp[-6].minor.yy0,&yymsp[-2].minor.yy0,0,yymsp[-1].minor.yy362,yymsp[0].minor.yy40);
  sqlite3SrcListFuncArgs(pParse, yymsp[-7].minor.yy387, yymsp[-4].minor.yy382);
}
#line 2759 "parse.c"
        break;
      case 99: /* seltablist ::= stl_prefix LP select RP as on_opt using_opt */
#line 585 "parse.y"
{
    yymsp[-6].minor.yy387 = sqlite3SrcListAppendFromTerm(pParse,yymsp[-6].minor.yy387,0,&yymsp[-2].minor.yy0,yymsp[-4].minor.yy279,yymsp[-1].minor.yy362,yymsp[0].minor.yy40);
  }
#line 2766 "parse.c"
        break;
      case 100: /* seltablist ::= stl_prefix LP seltablist RP as on_opt using_opt */
#line 589 "parse.y"
{
    if( yymsp[-6].minor.yy387==0 && yymsp[-2].minor.yy0.n==0 && yymsp[-1].minor.yy362==0 && yymsp[0].minor.yy40==0 ){
      yymsp[-6].minor.yy387 = yymsp[-4].minor.yy387;
    }else if( yymsp[-4].minor.yy387->nSrc==1 ){
      yymsp[-6].minor.yy387 = sqlite3SrcListAppendFromTerm(pParse,yymsp[-6].minor.yy387,0,&yymsp[-2].minor.yy0,0,yymsp[-1].minor.yy362,yymsp[0].minor.yy40);
      if( yymsp[-6].minor.yy387 ){
        struct SrcList_item *pNew = &yymsp[-6].minor.yy387->a[yymsp[-6].minor.yy387->nSrc-1];
        struct SrcList_item *pOld = yymsp[-4].minor.yy387->a;
        pNew->zName = pOld->zName;
        pNew->pSelect = pOld->pSelect;
        pOld->zName =  0;
        pOld->pSelect = 0;
      }
      sqlite3SrcListDelete(pParse->db, yymsp[-4].minor.yy387);
    }else{
      Select *pSubquery;
      sqlite3SrcListShiftJoinType(yymsp[-4].minor.yy387);
      pSubquery = sqlite3SelectNew(pParse,0,yymsp[-4].minor.yy387,0,0,0,0,SF_NestedFrom,0,0);
      yymsp[-6].minor.yy387 = sqlite3SrcListAppendFromTerm(pParse,yymsp[-6].minor.yy387,0,&yymsp[-2].minor.yy0,pSubquery,yymsp[-1].minor.yy362,yymsp[0].minor.yy40);
    }
  }
#line 2791 "parse.c"
        break;
      case 101: /* fullname ::= nm */
#line 615 "parse.y"
{yymsp[0].minor.yy387 = sqlite3SrcListAppend(pParse->db,0,&yymsp[0].minor.yy0); /*A-overwrites-X*/}
#line 2796 "parse.c"
        break;
      case 102: /* joinop ::= COMMA|JOIN */
#line 621 "parse.y"
{ yymsp[0].minor.yy52 = JT_INNER; }
#line 2801 "parse.c"
        break;
      case 103: /* joinop ::= JOIN_KW JOIN */
#line 623 "parse.y"
{yymsp[-1].minor.yy52 = sqlite3JoinType(pParse,&yymsp[-1].minor.yy0,0,0);  /*X-overwrites-A*/}
#line 2806 "parse.c"
        break;
      case 104: /* joinop ::= JOIN_KW join_nm JOIN */
#line 625 "parse.y"
{yymsp[-2].minor.yy52 = sqlite3JoinType(pParse,&yymsp[-2].minor.yy0,&yymsp[-1].minor.yy0,0); /*X-overwrites-A*/}
#line 2811 "parse.c"
        break;
      case 105: /* joinop ::= JOIN_KW join_nm join_nm JOIN */
#line 627 "parse.y"
{yymsp[-3].minor.yy52 = sqlite3JoinType(pParse,&yymsp[-3].minor.yy0,&yymsp[-2].minor.yy0,&yymsp[-1].minor.yy0);/*X-overwrites-A*/}
#line 2816 "parse.c"
        break;
      case 106: /* on_opt ::= ON expr */
      case 123: /* having_opt ::= HAVING expr */ yytestcase(yyruleno==123);
      case 130: /* where_opt ::= WHERE expr */ yytestcase(yyruleno==130);
      case 192: /* case_else ::= ELSE expr */ yytestcase(yyruleno==192);
#line 631 "parse.y"
{yymsp[-1].minor.yy362 = yymsp[0].minor.yy162.pExpr;}
#line 2824 "parse.c"
        break;
      case 107: /* on_opt ::= */
      case 122: /* having_opt ::= */ yytestcase(yyruleno==122);
      case 129: /* where_opt ::= */ yytestcase(yyruleno==129);
      case 193: /* case_else ::= */ yytestcase(yyruleno==193);
      case 195: /* case_operand ::= */ yytestcase(yyruleno==195);
#line 632 "parse.y"
{yymsp[1].minor.yy362 = 0;}
#line 2833 "parse.c"
        break;
      case 108: /* indexed_opt ::= */
#line 645 "parse.y"
{yymsp[1].minor.yy0.z=0; yymsp[1].minor.yy0.n=0;}
#line 2838 "parse.c"
        break;
      case 109: /* indexed_opt ::= INDEXED BY nm */
#line 646 "parse.y"
{yymsp[-2].minor.yy0 = yymsp[0].minor.yy0;}
#line 2843 "parse.c"
        break;
      case 110: /* indexed_opt ::= NOT INDEXED */
#line 647 "parse.y"
{yymsp[-1].minor.yy0.z=0; yymsp[-1].minor.yy0.n=1;}
#line 2848 "parse.c"
        break;
      case 111: /* using_opt ::= USING LP idlist RP */
#line 651 "parse.y"
{yymsp[-3].minor.yy40 = yymsp[-1].minor.yy40;}
#line 2853 "parse.c"
        break;
      case 112: /* using_opt ::= */
      case 140: /* idlist_opt ::= */ yytestcase(yyruleno==140);
#line 652 "parse.y"
{yymsp[1].minor.yy40 = 0;}
#line 2859 "parse.c"
        break;
      case 114: /* orderby_opt ::= ORDER BY sortlist */
      case 121: /* groupby_opt ::= GROUP BY nexprlist */ yytestcase(yyruleno==121);
#line 666 "parse.y"
{yymsp[-2].minor.yy382 = yymsp[0].minor.yy382;}
#line 2865 "parse.c"
        break;
      case 115: /* sortlist ::= sortlist COMMA expr sortorder */
#line 667 "parse.y"
{
  yymsp[-3].minor.yy382 = sqlite3ExprListAppend(pParse,yymsp[-3].minor.yy382,yymsp[-1].minor.yy162.pExpr);
  sqlite3ExprListSetSortOrder(yymsp[-3].minor.yy382,yymsp[0].minor.yy52);
}
#line 2873 "parse.c"
        break;
      case 116: /* sortlist ::= expr sortorder */
#line 671 "parse.y"
{
  yymsp[-1].minor.yy382 = sqlite3ExprListAppend(pParse,0,yymsp[-1].minor.yy162.pExpr); /*A-overwrites-Y*/
  sqlite3ExprListSetSortOrder(yymsp[-1].minor.yy382,yymsp[0].minor.yy52);
}
#line 2881 "parse.c"
        break;
      case 117: /* sortorder ::= ASC */
#line 678 "parse.y"
{yymsp[0].minor.yy52 = SQLITE_SO_ASC;}
#line 2886 "parse.c"
        break;
      case 118: /* sortorder ::= DESC */
#line 679 "parse.y"
{yymsp[0].minor.yy52 = SQLITE_SO_DESC;}
#line 2891 "parse.c"
        break;
      case 119: /* sortorder ::= */
#line 680 "parse.y"
{yymsp[1].minor.yy52 = SQLITE_SO_UNDEFINED;}
#line 2896 "parse.c"
        break;
      case 124: /* limit_opt ::= */
#line 705 "parse.y"
{yymsp[1].minor.yy384.pLimit = 0; yymsp[1].minor.yy384.pOffset = 0;}
#line 2901 "parse.c"
        break;
      case 125: /* limit_opt ::= LIMIT expr */
#line 706 "parse.y"
{yymsp[-1].minor.yy384.pLimit = yymsp[0].minor.yy162.pExpr; yymsp[-1].minor.yy384.pOffset = 0;}
#line 2906 "parse.c"
        break;
      case 126: /* limit_opt ::= LIMIT expr OFFSET expr */
#line 708 "parse.y"
{yymsp[-3].minor.yy384.pLimit = yymsp[-2].minor.yy162.pExpr; yymsp[-3].minor.yy384.pOffset = yymsp[0].minor.yy162.pExpr;}
#line 2911 "parse.c"
        break;
      case 127: /* limit_opt ::= LIMIT expr COMMA expr */
#line 710 "parse.y"
{yymsp[-3].minor.yy384.pOffset = yymsp[-2].minor.yy162.pExpr; yymsp[-3].minor.yy384.pLimit = yymsp[0].minor.yy162.pExpr;}
#line 2916 "parse.c"
        break;
      case 128: /* cmd ::= with DELETE FROM fullname indexed_opt where_opt */
#line 727 "parse.y"
{
  sqlite3WithPush(pParse, yymsp[-5].minor.yy151, 1);
  sqlite3SrcListIndexedBy(pParse, yymsp[-2].minor.yy387, &yymsp[-1].minor.yy0);
  sqlSubProgramsRemaining = SQL_MAX_COMPILING_TRIGGERS;
  /* Instruct SQL to initate Tarantool's transaction.  */
  pParse->initiateTTrans = true;
  sqlite3DeleteFrom(pParse,yymsp[-2].minor.yy387,yymsp[0].minor.yy362);
}
#line 2928 "parse.c"
        break;
      case 131: /* cmd ::= with UPDATE orconf fullname indexed_opt SET setlist where_opt */
#line 760 "parse.y"
{
  sqlite3WithPush(pParse, yymsp[-7].minor.yy151, 1);
  sqlite3SrcListIndexedBy(pParse, yymsp[-4].minor.yy387, &yymsp[-3].minor.yy0);
  sqlite3ExprListCheckLength(pParse,yymsp[-1].minor.yy382,"set list"); 
  sqlSubProgramsRemaining = SQL_MAX_COMPILING_TRIGGERS;
  /* Instruct SQL to initate Tarantool's transaction.  */
  pParse->initiateTTrans = true;
  sqlite3Update(pParse,yymsp[-4].minor.yy387,yymsp[-1].minor.yy382,yymsp[0].minor.yy362,yymsp[-5].minor.yy52);
}
#line 2941 "parse.c"
        break;
      case 132: /* setlist ::= setlist COMMA nm EQ expr */
#line 774 "parse.y"
{
  yymsp[-4].minor.yy382 = sqlite3ExprListAppend(pParse, yymsp[-4].minor.yy382, yymsp[0].minor.yy162.pExpr);
  sqlite3ExprListSetName(pParse, yymsp[-4].minor.yy382, &yymsp[-2].minor.yy0, 1);
}
#line 2949 "parse.c"
        break;
      case 133: /* setlist ::= setlist COMMA LP idlist RP EQ expr */
#line 778 "parse.y"
{
  yymsp[-6].minor.yy382 = sqlite3ExprListAppendVector(pParse, yymsp[-6].minor.yy382, yymsp[-3].minor.yy40, yymsp[0].minor.yy162.pExpr);
}
#line 2956 "parse.c"
        break;
      case 134: /* setlist ::= nm EQ expr */
#line 781 "parse.y"
{
  yylhsminor.yy382 = sqlite3ExprListAppend(pParse, 0, yymsp[0].minor.yy162.pExpr);
  sqlite3ExprListSetName(pParse, yylhsminor.yy382, &yymsp[-2].minor.yy0, 1);
}
#line 2964 "parse.c"
  yymsp[-2].minor.yy382 = yylhsminor.yy382;
        break;
      case 135: /* setlist ::= LP idlist RP EQ expr */
#line 785 "parse.y"
{
  yymsp[-4].minor.yy382 = sqlite3ExprListAppendVector(pParse, 0, yymsp[-3].minor.yy40, yymsp[0].minor.yy162.pExpr);
}
#line 2972 "parse.c"
        break;
      case 136: /* cmd ::= with insert_cmd INTO fullname idlist_opt select */
#line 791 "parse.y"
{
  sqlite3WithPush(pParse, yymsp[-5].minor.yy151, 1);
  sqlSubProgramsRemaining = SQL_MAX_COMPILING_TRIGGERS;
  /* Instruct SQL to initate Tarantool's transaction.  */
  pParse->initiateTTrans = true;
  sqlite3Insert(pParse, yymsp[-2].minor.yy387, yymsp[0].minor.yy279, yymsp[-1].minor.yy40, yymsp[-4].minor.yy52);
}
#line 2983 "parse.c"
        break;
      case 137: /* cmd ::= with insert_cmd INTO fullname idlist_opt DEFAULT VALUES */
#line 799 "parse.y"
{
  sqlite3WithPush(pParse, yymsp[-6].minor.yy151, 1);
  sqlSubProgramsRemaining = SQL_MAX_COMPILING_TRIGGERS;
  /* Instruct SQL to initate Tarantool's transaction.  */
  pParse->initiateTTrans = true;
  sqlite3Insert(pParse, yymsp[-3].minor.yy387, 0, yymsp[-2].minor.yy40, yymsp[-5].minor.yy52);
}
#line 2994 "parse.c"
        break;
      case 141: /* idlist_opt ::= LP idlist RP */
#line 817 "parse.y"
{yymsp[-2].minor.yy40 = yymsp[-1].minor.yy40;}
#line 2999 "parse.c"
        break;
      case 142: /* idlist ::= idlist COMMA nm */
#line 819 "parse.y"
{yymsp[-2].minor.yy40 = sqlite3IdListAppend(pParse->db,yymsp[-2].minor.yy40,&yymsp[0].minor.yy0);}
#line 3004 "parse.c"
        break;
      case 143: /* idlist ::= nm */
#line 821 "parse.y"
{yymsp[0].minor.yy40 = sqlite3IdListAppend(pParse->db,0,&yymsp[0].minor.yy0); /*A-overwrites-Y*/}
#line 3009 "parse.c"
        break;
      case 144: /* expr ::= LP expr RP */
#line 870 "parse.y"
{spanSet(&yymsp[-2].minor.yy162,&yymsp[-2].minor.yy0,&yymsp[0].minor.yy0); /*A-overwrites-B*/  yymsp[-2].minor.yy162.pExpr = yymsp[-1].minor.yy162.pExpr;}
#line 3014 "parse.c"
        break;
      case 145: /* term ::= NULL */
      case 149: /* term ::= FLOAT|BLOB */ yytestcase(yyruleno==149);
      case 150: /* term ::= STRING */ yytestcase(yyruleno==150);
#line 871 "parse.y"
{spanExpr(&yymsp[0].minor.yy162,pParse,yymsp[0].major,yymsp[0].minor.yy0);/*A-overwrites-X*/}
#line 3021 "parse.c"
        break;
      case 146: /* expr ::= ID|INDEXED */
      case 147: /* expr ::= JOIN_KW */ yytestcase(yyruleno==147);
#line 872 "parse.y"
{spanExpr(&yymsp[0].minor.yy162,pParse,TK_ID,yymsp[0].minor.yy0); /*A-overwrites-X*/}
#line 3027 "parse.c"
        break;
      case 148: /* expr ::= nm DOT nm */
#line 874 "parse.y"
{
  Expr *temp1 = sqlite3ExprAlloc(pParse->db, TK_ID, &yymsp[-2].minor.yy0, 1);
  Expr *temp2 = sqlite3ExprAlloc(pParse->db, TK_ID, &yymsp[0].minor.yy0, 1);
  spanSet(&yymsp[-2].minor.yy162,&yymsp[-2].minor.yy0,&yymsp[0].minor.yy0); /*A-overwrites-X*/
  yymsp[-2].minor.yy162.pExpr = sqlite3PExpr(pParse, TK_DOT, temp1, temp2);
}
#line 3037 "parse.c"
        break;
      case 151: /* term ::= INTEGER */
#line 882 "parse.y"
{
  yylhsminor.yy162.pExpr = sqlite3ExprAlloc(pParse->db, TK_INTEGER, &yymsp[0].minor.yy0, 1);
  yylhsminor.yy162.zStart = yymsp[0].minor.yy0.z;
  yylhsminor.yy162.zEnd = yymsp[0].minor.yy0.z + yymsp[0].minor.yy0.n;
  if( yylhsminor.yy162.pExpr ) yylhsminor.yy162.pExpr->flags |= EP_Leaf;
}
#line 3047 "parse.c"
  yymsp[0].minor.yy162 = yylhsminor.yy162;
        break;
      case 152: /* expr ::= VARIABLE */
#line 888 "parse.y"
{
  if( !(yymsp[0].minor.yy0.z[0]=='#' && sqlite3Isdigit(yymsp[0].minor.yy0.z[1])) ){
    u32 n = yymsp[0].minor.yy0.n;
    spanExpr(&yymsp[0].minor.yy162, pParse, TK_VARIABLE, yymsp[0].minor.yy0);
    sqlite3ExprAssignVarNumber(pParse, yymsp[0].minor.yy162.pExpr, n);
  }else{
    /* When doing a nested parse, one can include terms in an expression
    ** that look like this:   #1 #2 ...  These terms refer to registers
    ** in the virtual machine.  #N is the N-th register. */
    Token t = yymsp[0].minor.yy0; /*A-overwrites-X*/
    assert( t.n>=2 );
    spanSet(&yymsp[0].minor.yy162, &t, &t);
    if( pParse->nested==0 ){
      sqlite3ErrorMsg(pParse, "near \"%T\": syntax error", &t);
      yymsp[0].minor.yy162.pExpr = 0;
    }else{
      yymsp[0].minor.yy162.pExpr = sqlite3PExpr(pParse, TK_REGISTER, 0, 0);
      if( yymsp[0].minor.yy162.pExpr ) sqlite3GetInt32(&t.z[1], &yymsp[0].minor.yy162.pExpr->iTable);
    }
  }
}
#line 3073 "parse.c"
        break;
      case 153: /* expr ::= expr COLLATE ID|INDEXED */
#line 909 "parse.y"
{
  yymsp[-2].minor.yy162.pExpr = sqlite3ExprAddCollateToken(pParse, yymsp[-2].minor.yy162.pExpr, &yymsp[0].minor.yy0, 1);
  yymsp[-2].minor.yy162.zEnd = &yymsp[0].minor.yy0.z[yymsp[0].minor.yy0.n];
}
#line 3081 "parse.c"
        break;
      case 154: /* expr ::= CAST LP expr AS typetoken RP */
#line 914 "parse.y"
{
  spanSet(&yymsp[-5].minor.yy162,&yymsp[-5].minor.yy0,&yymsp[0].minor.yy0); /*A-overwrites-X*/
  yymsp[-5].minor.yy162.pExpr = sqlite3ExprAlloc(pParse->db, TK_CAST, &yymsp[-1].minor.yy0, 1);
  sqlite3ExprAttachSubtrees(pParse->db, yymsp[-5].minor.yy162.pExpr, yymsp[-3].minor.yy162.pExpr, 0);
}
#line 3090 "parse.c"
        break;
      case 155: /* expr ::= ID|INDEXED LP distinct exprlist RP */
#line 920 "parse.y"
{
  if( yymsp[-1].minor.yy382 && yymsp[-1].minor.yy382->nExpr>pParse->db->aLimit[SQLITE_LIMIT_FUNCTION_ARG] ){
    sqlite3ErrorMsg(pParse, "too many arguments on function %T", &yymsp[-4].minor.yy0);
  }
  yylhsminor.yy162.pExpr = sqlite3ExprFunction(pParse, yymsp[-1].minor.yy382, &yymsp[-4].minor.yy0);
  spanSet(&yylhsminor.yy162,&yymsp[-4].minor.yy0,&yymsp[0].minor.yy0);
  if( yymsp[-2].minor.yy52==SF_Distinct && yylhsminor.yy162.pExpr ){
    yylhsminor.yy162.pExpr->flags |= EP_Distinct;
  }
}
#line 3104 "parse.c"
  yymsp[-4].minor.yy162 = yylhsminor.yy162;
        break;
      case 156: /* expr ::= ID|INDEXED LP STAR RP */
#line 930 "parse.y"
{
  yylhsminor.yy162.pExpr = sqlite3ExprFunction(pParse, 0, &yymsp[-3].minor.yy0);
  spanSet(&yylhsminor.yy162,&yymsp[-3].minor.yy0,&yymsp[0].minor.yy0);
}
#line 3113 "parse.c"
  yymsp[-3].minor.yy162 = yylhsminor.yy162;
        break;
      case 157: /* term ::= CTIME_KW */
#line 934 "parse.y"
{
  yylhsminor.yy162.pExpr = sqlite3ExprFunction(pParse, 0, &yymsp[0].minor.yy0);
  spanSet(&yylhsminor.yy162, &yymsp[0].minor.yy0, &yymsp[0].minor.yy0);
}
#line 3122 "parse.c"
  yymsp[0].minor.yy162 = yylhsminor.yy162;
        break;
      case 158: /* expr ::= LP nexprlist COMMA expr RP */
#line 963 "parse.y"
{
  ExprList *pList = sqlite3ExprListAppend(pParse, yymsp[-3].minor.yy382, yymsp[-1].minor.yy162.pExpr);
  yylhsminor.yy162.pExpr = sqlite3PExpr(pParse, TK_VECTOR, 0, 0);
  if( yylhsminor.yy162.pExpr ){
    yylhsminor.yy162.pExpr->x.pList = pList;
    spanSet(&yylhsminor.yy162, &yymsp[-4].minor.yy0, &yymsp[0].minor.yy0);
  }else{
    sqlite3ExprListDelete(pParse->db, pList);
  }
}
#line 3137 "parse.c"
  yymsp[-4].minor.yy162 = yylhsminor.yy162;
        break;
      case 159: /* expr ::= expr AND expr */
      case 160: /* expr ::= expr OR expr */ yytestcase(yyruleno==160);
      case 161: /* expr ::= expr LT|GT|GE|LE expr */ yytestcase(yyruleno==161);
      case 162: /* expr ::= expr EQ|NE expr */ yytestcase(yyruleno==162);
      case 163: /* expr ::= expr BITAND|BITOR|LSHIFT|RSHIFT expr */ yytestcase(yyruleno==163);
      case 164: /* expr ::= expr PLUS|MINUS expr */ yytestcase(yyruleno==164);
      case 165: /* expr ::= expr STAR|SLASH|REM expr */ yytestcase(yyruleno==165);
      case 166: /* expr ::= expr CONCAT expr */ yytestcase(yyruleno==166);
#line 974 "parse.y"
{spanBinaryExpr(pParse,yymsp[-1].major,&yymsp[-2].minor.yy162,&yymsp[0].minor.yy162);}
#line 3150 "parse.c"
        break;
      case 167: /* likeop ::= LIKE_KW|MATCH */
#line 987 "parse.y"
{yymsp[0].minor.yy0=yymsp[0].minor.yy0;/*A-overwrites-X*/}
#line 3155 "parse.c"
        break;
      case 168: /* likeop ::= NOT LIKE_KW|MATCH */
#line 988 "parse.y"
{yymsp[-1].minor.yy0=yymsp[0].minor.yy0; yymsp[-1].minor.yy0.n|=0x80000000; /*yymsp[-1].minor.yy0-overwrite-yymsp[0].minor.yy0*/}
#line 3160 "parse.c"
        break;
      case 169: /* expr ::= expr likeop expr */
#line 989 "parse.y"
{
  ExprList *pList;
  int bNot = yymsp[-1].minor.yy0.n & 0x80000000;
  yymsp[-1].minor.yy0.n &= 0x7fffffff;
  pList = sqlite3ExprListAppend(pParse,0, yymsp[0].minor.yy162.pExpr);
  pList = sqlite3ExprListAppend(pParse,pList, yymsp[-2].minor.yy162.pExpr);
  yymsp[-2].minor.yy162.pExpr = sqlite3ExprFunction(pParse, pList, &yymsp[-1].minor.yy0);
  exprNot(pParse, bNot, &yymsp[-2].minor.yy162);
  yymsp[-2].minor.yy162.zEnd = yymsp[0].minor.yy162.zEnd;
  if( yymsp[-2].minor.yy162.pExpr ) yymsp[-2].minor.yy162.pExpr->flags |= EP_InfixFunc;
}
#line 3175 "parse.c"
        break;
      case 170: /* expr ::= expr likeop expr ESCAPE expr */
#line 1000 "parse.y"
{
  ExprList *pList;
  int bNot = yymsp[-3].minor.yy0.n & 0x80000000;
  yymsp[-3].minor.yy0.n &= 0x7fffffff;
  pList = sqlite3ExprListAppend(pParse,0, yymsp[-2].minor.yy162.pExpr);
  pList = sqlite3ExprListAppend(pParse,pList, yymsp[-4].minor.yy162.pExpr);
  pList = sqlite3ExprListAppend(pParse,pList, yymsp[0].minor.yy162.pExpr);
  yymsp[-4].minor.yy162.pExpr = sqlite3ExprFunction(pParse, pList, &yymsp[-3].minor.yy0);
  exprNot(pParse, bNot, &yymsp[-4].minor.yy162);
  yymsp[-4].minor.yy162.zEnd = yymsp[0].minor.yy162.zEnd;
  if( yymsp[-4].minor.yy162.pExpr ) yymsp[-4].minor.yy162.pExpr->flags |= EP_InfixFunc;
}
#line 3191 "parse.c"
        break;
      case 171: /* expr ::= expr ISNULL|NOTNULL */
#line 1027 "parse.y"
{spanUnaryPostfix(pParse,yymsp[0].major,&yymsp[-1].minor.yy162,&yymsp[0].minor.yy0);}
#line 3196 "parse.c"
        break;
      case 172: /* expr ::= expr NOT NULL */
#line 1028 "parse.y"
{spanUnaryPostfix(pParse,TK_NOTNULL,&yymsp[-2].minor.yy162,&yymsp[0].minor.yy0);}
#line 3201 "parse.c"
        break;
      case 173: /* expr ::= expr IS expr */
#line 1049 "parse.y"
{
  spanBinaryExpr(pParse,TK_IS,&yymsp[-2].minor.yy162,&yymsp[0].minor.yy162);
  binaryToUnaryIfNull(pParse, yymsp[0].minor.yy162.pExpr, yymsp[-2].minor.yy162.pExpr, TK_ISNULL);
}
#line 3209 "parse.c"
        break;
      case 174: /* expr ::= expr IS NOT expr */
#line 1053 "parse.y"
{
  spanBinaryExpr(pParse,TK_ISNOT,&yymsp[-3].minor.yy162,&yymsp[0].minor.yy162);
  binaryToUnaryIfNull(pParse, yymsp[0].minor.yy162.pExpr, yymsp[-3].minor.yy162.pExpr, TK_NOTNULL);
}
#line 3217 "parse.c"
        break;
      case 175: /* expr ::= NOT expr */
      case 176: /* expr ::= BITNOT expr */ yytestcase(yyruleno==176);
#line 1077 "parse.y"
{spanUnaryPrefix(&yymsp[-1].minor.yy162,pParse,yymsp[-1].major,&yymsp[0].minor.yy162,&yymsp[-1].minor.yy0);/*A-overwrites-B*/}
#line 3223 "parse.c"
        break;
      case 177: /* expr ::= MINUS expr */
#line 1081 "parse.y"
{spanUnaryPrefix(&yymsp[-1].minor.yy162,pParse,TK_UMINUS,&yymsp[0].minor.yy162,&yymsp[-1].minor.yy0);/*A-overwrites-B*/}
#line 3228 "parse.c"
        break;
      case 178: /* expr ::= PLUS expr */
#line 1083 "parse.y"
{spanUnaryPrefix(&yymsp[-1].minor.yy162,pParse,TK_UPLUS,&yymsp[0].minor.yy162,&yymsp[-1].minor.yy0);/*A-overwrites-B*/}
#line 3233 "parse.c"
        break;
      case 179: /* between_op ::= BETWEEN */
      case 182: /* in_op ::= IN */ yytestcase(yyruleno==182);
#line 1086 "parse.y"
{yymsp[0].minor.yy52 = 0;}
#line 3239 "parse.c"
        break;
      case 181: /* expr ::= expr between_op expr AND expr */
#line 1088 "parse.y"
{
  ExprList *pList = sqlite3ExprListAppend(pParse,0, yymsp[-2].minor.yy162.pExpr);
  pList = sqlite3ExprListAppend(pParse,pList, yymsp[0].minor.yy162.pExpr);
  yymsp[-4].minor.yy162.pExpr = sqlite3PExpr(pParse, TK_BETWEEN, yymsp[-4].minor.yy162.pExpr, 0);
  if( yymsp[-4].minor.yy162.pExpr ){
    yymsp[-4].minor.yy162.pExpr->x.pList = pList;
  }else{
    sqlite3ExprListDelete(pParse->db, pList);
  } 
  exprNot(pParse, yymsp[-3].minor.yy52, &yymsp[-4].minor.yy162);
  yymsp[-4].minor.yy162.zEnd = yymsp[0].minor.yy162.zEnd;
}
#line 3255 "parse.c"
        break;
      case 184: /* expr ::= expr in_op LP exprlist RP */
#line 1104 "parse.y"
{
    if( yymsp[-1].minor.yy382==0 ){
      /* Expressions of the form
      **
      **      expr1 IN ()
      **      expr1 NOT IN ()
      **
      ** simplify to constants 0 (false) and 1 (true), respectively,
      ** regardless of the value of expr1.
      */
      sqlite3ExprDelete(pParse->db, yymsp[-4].minor.yy162.pExpr);
      yymsp[-4].minor.yy162.pExpr = sqlite3ExprAlloc(pParse->db, TK_INTEGER,&sqlite3IntTokens[yymsp[-3].minor.yy52],1);
    }else if( yymsp[-1].minor.yy382->nExpr==1 ){
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
      Expr *pRHS = yymsp[-1].minor.yy382->a[0].pExpr;
      yymsp[-1].minor.yy382->a[0].pExpr = 0;
      sqlite3ExprListDelete(pParse->db, yymsp[-1].minor.yy382);
      /* pRHS cannot be NULL because a malloc error would have been detected
      ** before now and control would have never reached this point */
      if( ALWAYS(pRHS) ){
        pRHS->flags &= ~EP_Collate;
        pRHS->flags |= EP_Generic;
      }
      yymsp[-4].minor.yy162.pExpr = sqlite3PExpr(pParse, yymsp[-3].minor.yy52 ? TK_NE : TK_EQ, yymsp[-4].minor.yy162.pExpr, pRHS);
    }else{
      yymsp[-4].minor.yy162.pExpr = sqlite3PExpr(pParse, TK_IN, yymsp[-4].minor.yy162.pExpr, 0);
      if( yymsp[-4].minor.yy162.pExpr ){
        yymsp[-4].minor.yy162.pExpr->x.pList = yymsp[-1].minor.yy382;
        sqlite3ExprSetHeightAndFlags(pParse, yymsp[-4].minor.yy162.pExpr);
      }else{
        sqlite3ExprListDelete(pParse->db, yymsp[-1].minor.yy382);
      }
      exprNot(pParse, yymsp[-3].minor.yy52, &yymsp[-4].minor.yy162);
    }
    yymsp[-4].minor.yy162.zEnd = &yymsp[0].minor.yy0.z[yymsp[0].minor.yy0.n];
  }
#line 3310 "parse.c"
        break;
      case 185: /* expr ::= LP select RP */
#line 1155 "parse.y"
{
    spanSet(&yymsp[-2].minor.yy162,&yymsp[-2].minor.yy0,&yymsp[0].minor.yy0); /*A-overwrites-B*/
    yymsp[-2].minor.yy162.pExpr = sqlite3PExpr(pParse, TK_SELECT, 0, 0);
    sqlite3PExprAddSelect(pParse, yymsp[-2].minor.yy162.pExpr, yymsp[-1].minor.yy279);
  }
#line 3319 "parse.c"
        break;
      case 186: /* expr ::= expr in_op LP select RP */
#line 1160 "parse.y"
{
    yymsp[-4].minor.yy162.pExpr = sqlite3PExpr(pParse, TK_IN, yymsp[-4].minor.yy162.pExpr, 0);
    sqlite3PExprAddSelect(pParse, yymsp[-4].minor.yy162.pExpr, yymsp[-1].minor.yy279);
    exprNot(pParse, yymsp[-3].minor.yy52, &yymsp[-4].minor.yy162);
    yymsp[-4].minor.yy162.zEnd = &yymsp[0].minor.yy0.z[yymsp[0].minor.yy0.n];
  }
#line 3329 "parse.c"
        break;
      case 187: /* expr ::= expr in_op nm paren_exprlist */
#line 1166 "parse.y"
{
    SrcList *pSrc = sqlite3SrcListAppend(pParse->db, 0,&yymsp[-1].minor.yy0);
    Select *pSelect = sqlite3SelectNew(pParse, 0,pSrc,0,0,0,0,0,0,0);
    if( yymsp[0].minor.yy382 )  sqlite3SrcListFuncArgs(pParse, pSelect ? pSrc : 0, yymsp[0].minor.yy382);
    yymsp[-3].minor.yy162.pExpr = sqlite3PExpr(pParse, TK_IN, yymsp[-3].minor.yy162.pExpr, 0);
    sqlite3PExprAddSelect(pParse, yymsp[-3].minor.yy162.pExpr, pSelect);
    exprNot(pParse, yymsp[-2].minor.yy52, &yymsp[-3].minor.yy162);
    yymsp[-3].minor.yy162.zEnd = &yymsp[-1].minor.yy0.z[yymsp[-1].minor.yy0.n];
  }
#line 3342 "parse.c"
        break;
      case 188: /* expr ::= EXISTS LP select RP */
#line 1175 "parse.y"
{
    Expr *p;
    spanSet(&yymsp[-3].minor.yy162,&yymsp[-3].minor.yy0,&yymsp[0].minor.yy0); /*A-overwrites-B*/
    p = yymsp[-3].minor.yy162.pExpr = sqlite3PExpr(pParse, TK_EXISTS, 0, 0);
    sqlite3PExprAddSelect(pParse, p, yymsp[-1].minor.yy279);
  }
#line 3352 "parse.c"
        break;
      case 189: /* expr ::= CASE case_operand case_exprlist case_else END */
#line 1184 "parse.y"
{
  spanSet(&yymsp[-4].minor.yy162,&yymsp[-4].minor.yy0,&yymsp[0].minor.yy0);  /*A-overwrites-C*/
  yymsp[-4].minor.yy162.pExpr = sqlite3PExpr(pParse, TK_CASE, yymsp[-3].minor.yy362, 0);
  if( yymsp[-4].minor.yy162.pExpr ){
    yymsp[-4].minor.yy162.pExpr->x.pList = yymsp[-1].minor.yy362 ? sqlite3ExprListAppend(pParse,yymsp[-2].minor.yy382,yymsp[-1].minor.yy362) : yymsp[-2].minor.yy382;
    sqlite3ExprSetHeightAndFlags(pParse, yymsp[-4].minor.yy162.pExpr);
  }else{
    sqlite3ExprListDelete(pParse->db, yymsp[-2].minor.yy382);
    sqlite3ExprDelete(pParse->db, yymsp[-1].minor.yy362);
  }
}
#line 3367 "parse.c"
        break;
      case 190: /* case_exprlist ::= case_exprlist WHEN expr THEN expr */
#line 1197 "parse.y"
{
  yymsp[-4].minor.yy382 = sqlite3ExprListAppend(pParse,yymsp[-4].minor.yy382, yymsp[-2].minor.yy162.pExpr);
  yymsp[-4].minor.yy382 = sqlite3ExprListAppend(pParse,yymsp[-4].minor.yy382, yymsp[0].minor.yy162.pExpr);
}
#line 3375 "parse.c"
        break;
      case 191: /* case_exprlist ::= WHEN expr THEN expr */
#line 1201 "parse.y"
{
  yymsp[-3].minor.yy382 = sqlite3ExprListAppend(pParse,0, yymsp[-2].minor.yy162.pExpr);
  yymsp[-3].minor.yy382 = sqlite3ExprListAppend(pParse,yymsp[-3].minor.yy382, yymsp[0].minor.yy162.pExpr);
}
#line 3383 "parse.c"
        break;
      case 194: /* case_operand ::= expr */
#line 1211 "parse.y"
{yymsp[0].minor.yy362 = yymsp[0].minor.yy162.pExpr; /*A-overwrites-X*/}
#line 3388 "parse.c"
        break;
      case 197: /* nexprlist ::= nexprlist COMMA expr */
#line 1222 "parse.y"
{yymsp[-2].minor.yy382 = sqlite3ExprListAppend(pParse,yymsp[-2].minor.yy382,yymsp[0].minor.yy162.pExpr);}
#line 3393 "parse.c"
        break;
      case 198: /* nexprlist ::= expr */
#line 1224 "parse.y"
{yymsp[0].minor.yy382 = sqlite3ExprListAppend(pParse,0,yymsp[0].minor.yy162.pExpr); /*A-overwrites-Y*/}
#line 3398 "parse.c"
        break;
      case 200: /* paren_exprlist ::= LP exprlist RP */
      case 205: /* eidlist_opt ::= LP eidlist RP */ yytestcase(yyruleno==205);
#line 1232 "parse.y"
{yymsp[-2].minor.yy382 = yymsp[-1].minor.yy382;}
#line 3404 "parse.c"
        break;
      case 201: /* cmd ::= createkw uniqueflag INDEX ifnotexists nm ON nm LP sortlist RP where_opt */
#line 1239 "parse.y"
{
  sqlite3CreateIndex(pParse, &yymsp[-6].minor.yy0, 
                     sqlite3SrcListAppend(pParse->db,0,&yymsp[-4].minor.yy0), yymsp[-2].minor.yy382, yymsp[-9].minor.yy52,
                      &yymsp[-10].minor.yy0, yymsp[0].minor.yy362, SQLITE_SO_ASC, yymsp[-7].minor.yy52, SQLITE_IDXTYPE_APPDEF);
}
#line 3413 "parse.c"
        break;
      case 202: /* uniqueflag ::= UNIQUE */
      case 243: /* raisetype ::= ABORT */ yytestcase(yyruleno==243);
#line 1246 "parse.y"
{yymsp[0].minor.yy52 = ON_CONFLICT_ACTION_ABORT;}
#line 3419 "parse.c"
        break;
      case 203: /* uniqueflag ::= */
#line 1247 "parse.y"
{yymsp[1].minor.yy52 = ON_CONFLICT_ACTION_NONE;}
#line 3424 "parse.c"
        break;
      case 206: /* eidlist ::= eidlist COMMA nm collate sortorder */
#line 1290 "parse.y"
{
  yymsp[-4].minor.yy382 = parserAddExprIdListTerm(pParse, yymsp[-4].minor.yy382, &yymsp[-2].minor.yy0, yymsp[-1].minor.yy52, yymsp[0].minor.yy52);
}
#line 3431 "parse.c"
        break;
      case 207: /* eidlist ::= nm collate sortorder */
#line 1293 "parse.y"
{
  yymsp[-2].minor.yy382 = parserAddExprIdListTerm(pParse, 0, &yymsp[-2].minor.yy0, yymsp[-1].minor.yy52, yymsp[0].minor.yy52); /*A-overwrites-Y*/
}
#line 3438 "parse.c"
        break;
      case 210: /* cmd ::= DROP INDEX ifexists fullname ON nm */
#line 1304 "parse.y"
{
    sqlite3DropIndex(pParse, yymsp[-2].minor.yy387, &yymsp[0].minor.yy0, yymsp[-3].minor.yy52);
}
#line 3445 "parse.c"
        break;
      case 211: /* cmd ::= PRAGMA nm */
#line 1311 "parse.y"
{
    sqlite3Pragma(pParse,&yymsp[0].minor.yy0,0,0,0,0);
}
#line 3452 "parse.c"
        break;
      case 212: /* cmd ::= PRAGMA nm EQ nmnum */
#line 1314 "parse.y"
{
    sqlite3Pragma(pParse,&yymsp[-2].minor.yy0,0,&yymsp[0].minor.yy0,0,0);
}
#line 3459 "parse.c"
        break;
      case 213: /* cmd ::= PRAGMA nm LP nmnum RP */
#line 1317 "parse.y"
{
    sqlite3Pragma(pParse,&yymsp[-3].minor.yy0,0,&yymsp[-1].minor.yy0,0,0);
}
#line 3466 "parse.c"
        break;
      case 214: /* cmd ::= PRAGMA nm EQ minus_num */
#line 1320 "parse.y"
{
    sqlite3Pragma(pParse,&yymsp[-2].minor.yy0,0,&yymsp[0].minor.yy0,0,1);
}
#line 3473 "parse.c"
        break;
      case 215: /* cmd ::= PRAGMA nm LP minus_num RP */
#line 1323 "parse.y"
{
    sqlite3Pragma(pParse,&yymsp[-3].minor.yy0,0,&yymsp[-1].minor.yy0,0,1);
}
#line 3480 "parse.c"
        break;
      case 216: /* cmd ::= PRAGMA nm LP nm DOT nm RP */
#line 1326 "parse.y"
{
    sqlite3Pragma(pParse,&yymsp[-5].minor.yy0,0,&yymsp[-1].minor.yy0,&yymsp[-3].minor.yy0,0);
}
#line 3487 "parse.c"
        break;
      case 217: /* cmd ::= PRAGMA */
#line 1329 "parse.y"
{
    sqlite3Pragma(pParse, 0,0,0,0,0);
}
#line 3494 "parse.c"
        break;
      case 220: /* cmd ::= createkw trigger_decl BEGIN trigger_cmd_list END */
#line 1349 "parse.y"
{
  Token all;
  all.z = yymsp[-3].minor.yy0.z;
  all.n = (int)(yymsp[0].minor.yy0.z - yymsp[-3].minor.yy0.z) + yymsp[0].minor.yy0.n;
  sqlite3FinishTrigger(pParse, yymsp[-1].minor.yy427, &all);
}
#line 3504 "parse.c"
        break;
      case 221: /* trigger_decl ::= TRIGGER ifnotexists nm trigger_time trigger_event ON fullname foreach_clause when_clause */
#line 1358 "parse.y"
{
  sqlite3BeginTrigger(pParse, &yymsp[-6].minor.yy0, yymsp[-5].minor.yy52, yymsp[-4].minor.yy10.a, yymsp[-4].minor.yy10.b, yymsp[-2].minor.yy387, yymsp[0].minor.yy362, yymsp[-7].minor.yy52);
  yymsp[-8].minor.yy0 = yymsp[-6].minor.yy0; /*yymsp[-8].minor.yy0-overwrites-T*/
}
#line 3512 "parse.c"
        break;
      case 222: /* trigger_time ::= BEFORE */
#line 1364 "parse.y"
{ yymsp[0].minor.yy52 = TK_BEFORE; }
#line 3517 "parse.c"
        break;
      case 223: /* trigger_time ::= AFTER */
#line 1365 "parse.y"
{ yymsp[0].minor.yy52 = TK_AFTER;  }
#line 3522 "parse.c"
        break;
      case 224: /* trigger_time ::= INSTEAD OF */
#line 1366 "parse.y"
{ yymsp[-1].minor.yy52 = TK_INSTEAD;}
#line 3527 "parse.c"
        break;
      case 225: /* trigger_time ::= */
#line 1367 "parse.y"
{ yymsp[1].minor.yy52 = TK_BEFORE; }
#line 3532 "parse.c"
        break;
      case 226: /* trigger_event ::= DELETE|INSERT */
      case 227: /* trigger_event ::= UPDATE */ yytestcase(yyruleno==227);
#line 1371 "parse.y"
{yymsp[0].minor.yy10.a = yymsp[0].major; /*A-overwrites-X*/ yymsp[0].minor.yy10.b = 0;}
#line 3538 "parse.c"
        break;
      case 228: /* trigger_event ::= UPDATE OF idlist */
#line 1373 "parse.y"
{yymsp[-2].minor.yy10.a = TK_UPDATE; yymsp[-2].minor.yy10.b = yymsp[0].minor.yy40;}
#line 3543 "parse.c"
        break;
      case 229: /* when_clause ::= */
#line 1380 "parse.y"
{ yymsp[1].minor.yy362 = 0; }
#line 3548 "parse.c"
        break;
      case 230: /* when_clause ::= WHEN expr */
#line 1381 "parse.y"
{ yymsp[-1].minor.yy362 = yymsp[0].minor.yy162.pExpr; }
#line 3553 "parse.c"
        break;
      case 231: /* trigger_cmd_list ::= trigger_cmd_list trigger_cmd SEMI */
#line 1385 "parse.y"
{
  assert( yymsp[-2].minor.yy427!=0 );
  yymsp[-2].minor.yy427->pLast->pNext = yymsp[-1].minor.yy427;
  yymsp[-2].minor.yy427->pLast = yymsp[-1].minor.yy427;
}
#line 3562 "parse.c"
        break;
      case 232: /* trigger_cmd_list ::= trigger_cmd SEMI */
#line 1390 "parse.y"
{ 
  assert( yymsp[-1].minor.yy427!=0 );
  yymsp[-1].minor.yy427->pLast = yymsp[-1].minor.yy427;
}
#line 3570 "parse.c"
        break;
      case 233: /* trnm ::= nm DOT nm */
#line 1401 "parse.y"
{
  yymsp[-2].minor.yy0 = yymsp[0].minor.yy0;
  sqlite3ErrorMsg(pParse, 
        "qualified table names are not allowed on INSERT, UPDATE, and DELETE "
        "statements within triggers");
}
#line 3580 "parse.c"
        break;
      case 234: /* tridxby ::= INDEXED BY nm */
#line 1413 "parse.y"
{
  sqlite3ErrorMsg(pParse,
        "the INDEXED BY clause is not allowed on UPDATE or DELETE statements "
        "within triggers");
}
#line 3589 "parse.c"
        break;
      case 235: /* tridxby ::= NOT INDEXED */
#line 1418 "parse.y"
{
  sqlite3ErrorMsg(pParse,
        "the NOT INDEXED clause is not allowed on UPDATE or DELETE statements "
        "within triggers");
}
#line 3598 "parse.c"
        break;
      case 236: /* trigger_cmd ::= UPDATE orconf trnm tridxby SET setlist where_opt */
#line 1431 "parse.y"
{yymsp[-6].minor.yy427 = sqlite3TriggerUpdateStep(pParse->db, &yymsp[-4].minor.yy0, yymsp[-1].minor.yy382, yymsp[0].minor.yy362, yymsp[-5].minor.yy52);}
#line 3603 "parse.c"
        break;
      case 237: /* trigger_cmd ::= insert_cmd INTO trnm idlist_opt select */
#line 1435 "parse.y"
{yymsp[-4].minor.yy427 = sqlite3TriggerInsertStep(pParse->db, &yymsp[-2].minor.yy0, yymsp[-1].minor.yy40, yymsp[0].minor.yy279, yymsp[-4].minor.yy52);/*A-overwrites-R*/}
#line 3608 "parse.c"
        break;
      case 238: /* trigger_cmd ::= DELETE FROM trnm tridxby where_opt */
#line 1439 "parse.y"
{yymsp[-4].minor.yy427 = sqlite3TriggerDeleteStep(pParse->db, &yymsp[-2].minor.yy0, yymsp[0].minor.yy362);}
#line 3613 "parse.c"
        break;
      case 239: /* trigger_cmd ::= select */
#line 1443 "parse.y"
{yymsp[0].minor.yy427 = sqlite3TriggerSelectStep(pParse->db, yymsp[0].minor.yy279); /*A-overwrites-X*/}
#line 3618 "parse.c"
        break;
      case 240: /* expr ::= RAISE LP IGNORE RP */
#line 1446 "parse.y"
{
  spanSet(&yymsp[-3].minor.yy162,&yymsp[-3].minor.yy0,&yymsp[0].minor.yy0);  /*A-overwrites-X*/
  yymsp[-3].minor.yy162.pExpr = sqlite3PExpr(pParse, TK_RAISE, 0, 0); 
  if( yymsp[-3].minor.yy162.pExpr ){
    yymsp[-3].minor.yy162.pExpr->affinity = ON_CONFLICT_ACTION_IGNORE;
  }
}
#line 3629 "parse.c"
        break;
      case 241: /* expr ::= RAISE LP raisetype COMMA STRING RP */
#line 1453 "parse.y"
{
  spanSet(&yymsp[-5].minor.yy162,&yymsp[-5].minor.yy0,&yymsp[0].minor.yy0);  /*A-overwrites-X*/
  yymsp[-5].minor.yy162.pExpr = sqlite3ExprAlloc(pParse->db, TK_RAISE, &yymsp[-1].minor.yy0, 1); 
  if( yymsp[-5].minor.yy162.pExpr ) {
    yymsp[-5].minor.yy162.pExpr->affinity = (char)yymsp[-3].minor.yy52;
  }
}
#line 3640 "parse.c"
        break;
      case 242: /* raisetype ::= ROLLBACK */
#line 1463 "parse.y"
{yymsp[0].minor.yy52 = ON_CONFLICT_ACTION_ROLLBACK;}
#line 3645 "parse.c"
        break;
      case 244: /* raisetype ::= FAIL */
#line 1465 "parse.y"
{yymsp[0].minor.yy52 = ON_CONFLICT_ACTION_FAIL;}
#line 3650 "parse.c"
        break;
      case 245: /* cmd ::= DROP TRIGGER ifexists fullname */
#line 1470 "parse.y"
{
  sqlite3DropTrigger(pParse,yymsp[0].minor.yy387,yymsp[-1].minor.yy52);
}
#line 3657 "parse.c"
        break;
      case 246: /* cmd ::= REINDEX */
#line 1477 "parse.y"
{sqlite3Reindex(pParse, 0, 0);}
#line 3662 "parse.c"
        break;
      case 247: /* cmd ::= REINDEX nm */
#line 1478 "parse.y"
{sqlite3Reindex(pParse, &yymsp[0].minor.yy0, 0);}
#line 3667 "parse.c"
        break;
      case 248: /* cmd ::= REINDEX nm ON nm */
#line 1479 "parse.y"
{sqlite3Reindex(pParse, &yymsp[-2].minor.yy0, &yymsp[0].minor.yy0);}
#line 3672 "parse.c"
        break;
      case 249: /* cmd ::= ANALYZE */
#line 1484 "parse.y"
{sqlite3Analyze(pParse, 0);}
#line 3677 "parse.c"
        break;
      case 250: /* cmd ::= ANALYZE nm */
#line 1485 "parse.y"
{sqlite3Analyze(pParse, &yymsp[0].minor.yy0);}
#line 3682 "parse.c"
        break;
      case 251: /* cmd ::= ALTER TABLE fullname RENAME TO nm */
#line 1490 "parse.y"
{
  sqlite3AlterRenameTable(pParse,yymsp[-3].minor.yy387,&yymsp[0].minor.yy0);
}
#line 3689 "parse.c"
        break;
      case 252: /* with ::= */
#line 1513 "parse.y"
{yymsp[1].minor.yy151 = 0;}
#line 3694 "parse.c"
        break;
      case 253: /* with ::= WITH wqlist */
#line 1515 "parse.y"
{ yymsp[-1].minor.yy151 = yymsp[0].minor.yy151; }
#line 3699 "parse.c"
        break;
      case 254: /* with ::= WITH RECURSIVE wqlist */
#line 1516 "parse.y"
{ yymsp[-2].minor.yy151 = yymsp[0].minor.yy151; }
#line 3704 "parse.c"
        break;
      case 255: /* wqlist ::= nm eidlist_opt AS LP select RP */
#line 1518 "parse.y"
{
  yymsp[-5].minor.yy151 = sqlite3WithAdd(pParse, 0, &yymsp[-5].minor.yy0, yymsp[-4].minor.yy382, yymsp[-1].minor.yy279); /*A-overwrites-X*/
}
#line 3711 "parse.c"
        break;
      case 256: /* wqlist ::= wqlist COMMA nm eidlist_opt AS LP select RP */
#line 1521 "parse.y"
{
  yymsp[-7].minor.yy151 = sqlite3WithAdd(pParse, yymsp[-7].minor.yy151, &yymsp[-5].minor.yy0, yymsp[-4].minor.yy382, yymsp[-1].minor.yy279);
}
#line 3718 "parse.c"
        break;
      default:
      /* (257) input ::= ecmd */ yytestcase(yyruleno==257);
      /* (258) explain ::= */ yytestcase(yyruleno==258);
      /* (259) cmdx ::= cmd (OPTIMIZED OUT) */ assert(yyruleno!=259);
      /* (260) trans_opt ::= */ yytestcase(yyruleno==260);
      /* (261) trans_opt ::= TRANSACTION */ yytestcase(yyruleno==261);
      /* (262) trans_opt ::= TRANSACTION nm */ yytestcase(yyruleno==262);
      /* (263) savepoint_opt ::= SAVEPOINT */ yytestcase(yyruleno==263);
      /* (264) savepoint_opt ::= */ yytestcase(yyruleno==264);
      /* (265) cmd ::= create_table create_table_args */ yytestcase(yyruleno==265);
      /* (266) columnlist ::= columnlist COMMA columnname carglist */ yytestcase(yyruleno==266);
      /* (267) columnlist ::= columnname carglist */ yytestcase(yyruleno==267);
      /* (268) typetoken ::= typename */ yytestcase(yyruleno==268);
      /* (269) typename ::= ID|STRING */ yytestcase(yyruleno==269);
      /* (270) signed ::= plus_num (OPTIMIZED OUT) */ assert(yyruleno!=270);
      /* (271) signed ::= minus_num (OPTIMIZED OUT) */ assert(yyruleno!=271);
      /* (272) carglist ::= carglist ccons */ yytestcase(yyruleno==272);
      /* (273) carglist ::= */ yytestcase(yyruleno==273);
      /* (274) ccons ::= NULL onconf */ yytestcase(yyruleno==274);
      /* (275) conslist_opt ::= COMMA conslist */ yytestcase(yyruleno==275);
      /* (276) conslist ::= conslist tconscomma tcons */ yytestcase(yyruleno==276);
      /* (277) conslist ::= tcons (OPTIMIZED OUT) */ assert(yyruleno!=277);
      /* (278) tconscomma ::= */ yytestcase(yyruleno==278);
      /* (279) defer_subclause_opt ::= defer_subclause (OPTIMIZED OUT) */ assert(yyruleno!=279);
      /* (280) resolvetype ::= raisetype (OPTIMIZED OUT) */ assert(yyruleno!=280);
      /* (281) selectnowith ::= oneselect (OPTIMIZED OUT) */ assert(yyruleno!=281);
      /* (282) oneselect ::= values */ yytestcase(yyruleno==282);
      /* (283) sclp ::= selcollist COMMA */ yytestcase(yyruleno==283);
      /* (284) as ::= ID|STRING */ yytestcase(yyruleno==284);
      /* (285) join_nm ::= ID|INDEXED */ yytestcase(yyruleno==285);
      /* (286) join_nm ::= JOIN_KW */ yytestcase(yyruleno==286);
      /* (287) expr ::= term (OPTIMIZED OUT) */ assert(yyruleno!=287);
      /* (288) exprlist ::= nexprlist */ yytestcase(yyruleno==288);
      /* (289) nmnum ::= plus_num (OPTIMIZED OUT) */ assert(yyruleno!=289);
      /* (290) nmnum ::= STRING */ yytestcase(yyruleno==290);
      /* (291) nmnum ::= nm */ yytestcase(yyruleno==291);
      /* (292) nmnum ::= ON */ yytestcase(yyruleno==292);
      /* (293) nmnum ::= DELETE */ yytestcase(yyruleno==293);
      /* (294) nmnum ::= DEFAULT */ yytestcase(yyruleno==294);
      /* (295) plus_num ::= INTEGER|FLOAT */ yytestcase(yyruleno==295);
      /* (296) foreach_clause ::= */ yytestcase(yyruleno==296);
      /* (297) foreach_clause ::= FOR EACH ROW */ yytestcase(yyruleno==297);
      /* (298) trnm ::= nm */ yytestcase(yyruleno==298);
      /* (299) tridxby ::= */ yytestcase(yyruleno==299);
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
  if (yypParser->is_fallback_failed && TOKEN.isReserved) {
    sqlite3ErrorMsg(pParse, "keyword \"%T\" is reserved", &TOKEN);
  } else {
    sqlite3ErrorMsg(pParse, "near \"%T\": syntax error", &TOKEN);
  }
#line 3829 "parse.c"
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

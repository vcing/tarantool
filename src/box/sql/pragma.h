/* DO NOT EDIT!
 * This file is automatically generated by the script at
 * ../tool/mkpragmatab.tcl.  To update the set of pragmas, edit
 * that script and rerun it.
 */

/* The various pragma types */
#define PragTyp_HEADER_VALUE                   0
#define PragTyp_BUSY_TIMEOUT                   1
#define PragTyp_CASE_SENSITIVE_LIKE            2
#define PragTyp_COLLATION_LIST                 3
#define PragTyp_FLAG                           5
#define PragTyp_FOREIGN_KEY_CHECK              8
#define PragTyp_FOREIGN_KEY_LIST               9
#define PragTyp_INDEX_INFO                    10
#define PragTyp_INDEX_LIST                    11
#define PragTyp_STATS                         15
#define PragTyp_TABLE_INFO                    17
#define PragTyp_PARSER_TRACE                  24

/* Property flags associated with various pragma. */
#define PragFlg_NeedSchema 0x01	/* Force schema load before running */
#define PragFlg_NoColumns  0x02	/* OP_ResultRow called with zero columns */
#define PragFlg_NoColumns1 0x04	/* zero columns if RHS argument is present */
#define PragFlg_ReadOnly   0x08	/* Read-only HEADER_VALUE */
#define PragFlg_Result0    0x10	/* Acts as query when no argument */
#define PragFlg_Result1    0x20	/* Acts as query when has one argument */
#define PragFlg_SchemaOpt  0x40	/* Schema restricts name search if present */
#define PragFlg_SchemaReq  0x80	/* Schema required - "main" is default */

/* Names of columns for pragmas that return multi-column result
 * or that return single-column results where the name of the
 * result column is different from the name of the pragma
 */
static const char *const pragCName[] = {
				/*   0 */ "cid",
				/* Used by: table_info */
	/*   1 */ "name",
	/*   2 */ "type",
	/*   3 */ "notnull",
	/*   4 */ "dflt_value",
	/*   5 */ "pk",
				/*   6 */ "table",
				/* Used by: stats */
	/*   7 */ "index",
	/*   8 */ "width",
	/*   9 */ "height",
				/*  10 */ "seqno",
				/* Used by: index_info */
	/*  11 */ "cid",
	/*  12 */ "name",
				/*  13 */ "seqno",
				/* Used by: index_xinfo */
	/*  14 */ "cid",
	/*  15 */ "name",
	/*  16 */ "desc",
	/*  17 */ "coll",
	/*  18 */ "key",
				/*  19 */ "seq",
				/* Used by: index_list */
	/*  20 */ "name",
	/*  21 */ "unique",
	/*  22 */ "origin",
	/*  23 */ "partial",
				/*  24 */ "seq",
				/* Used by: database_list */
	/*  25 */ "name",
	/*  26 */ "file",
				/*  27 */ "seq",
				/* Used by: collation_list */
	/*  28 */ "name",
				/*  29 */ "id",
				/* Used by: foreign_key_list */
	/*  30 */ "seq",
	/*  31 */ "table",
	/*  32 */ "from",
	/*  33 */ "to",
	/*  34 */ "on_update",
	/*  35 */ "on_delete",
	/*  36 */ "match",
				/*  37 */ "table",
				/* Used by: foreign_key_check */
	/*  38 */ "rowid",
	/*  39 */ "parent",
	/*  40 */ "fkid",
				/*  41 */ "busy",
				/* Used by: wal_checkpoint */
	/*  42 */ "log",
	/*  43 */ "checkpointed",
				/*  44 */ "timeout",
				/* Used by: busy_timeout */
};

/* Definitions of all built-in pragmas */
typedef struct PragmaName {
	const char *const zName;	/* Name of pragma */
	u8 ePragTyp;		/* PragTyp_XXX value */
	u8 mPragFlg;		/* Zero or more PragFlg_XXX values */
	u8 iPragCName;		/* Start of column names in pragCName[] */
	u8 nPragCName;		/* Num of col names. 0 means use pragma name */
	u32 iArg;		/* Extra argument */
} PragmaName;
static const PragmaName aPragmaName[] = {
	{ /* zName:     */ "busy_timeout",
	 /* ePragTyp:  */ PragTyp_BUSY_TIMEOUT,
	 /* ePragFlg:  */ PragFlg_Result0,
	 /* ColNames:  */ 44, 1,
	 /* iArg:      */ 0},
	{ /* zName:     */ "case_sensitive_like",
	 /* ePragTyp:  */ PragTyp_CASE_SENSITIVE_LIKE,
	 /* ePragFlg:  */ PragFlg_NoColumns,
	 /* ColNames:  */ 0, 0,
	 /* iArg:      */ 0},
#if !defined(SQLITE_OMIT_SCHEMA_PRAGMAS)
	{ /* zName:     */ "collation_list",
	 /* ePragTyp:  */ PragTyp_COLLATION_LIST,
	 /* ePragFlg:  */ PragFlg_Result0,
	 /* ColNames:  */ 27, 2,
	 /* iArg:      */ 0},
#endif
#if !defined(SQLITE_OMIT_FLAG_PRAGMAS)
	{ /* zName:     */ "count_changes",
	 /* ePragTyp:  */ PragTyp_FLAG,
	 /* ePragFlg:  */ PragFlg_Result0 | PragFlg_NoColumns1,
	 /* ColNames:  */ 0, 0,
	 /* iArg:      */ SQLITE_CountRows},
#endif
#if !defined(SQLITE_OMIT_FLAG_PRAGMAS)
#if !defined(SQLITE_OMIT_FOREIGN_KEY) && !defined(SQLITE_OMIT_TRIGGER)
	{ /* zName:     */ "defer_foreign_keys",
	 /* ePragTyp:  */ PragTyp_FLAG,
	 /* ePragFlg:  */ PragFlg_Result0 | PragFlg_NoColumns1,
	 /* ColNames:  */ 0, 0,
	 /* iArg:      */ SQLITE_DeferFKs},
#endif
#endif
#if !defined(SQLITE_OMIT_FOREIGN_KEY) && !defined(SQLITE_OMIT_TRIGGER)
	{ /* zName:     */ "foreign_key_check",
	 /* ePragTyp:  */ PragTyp_FOREIGN_KEY_CHECK,
	 /* ePragFlg:  */ PragFlg_NeedSchema,
	 /* ColNames:  */ 37, 4,
	 /* iArg:      */ 0},
#endif
#if !defined(SQLITE_OMIT_FOREIGN_KEY)
	{ /* zName:     */ "foreign_key_list",
	 /* ePragTyp:  */ PragTyp_FOREIGN_KEY_LIST,
	 /* ePragFlg:  */
	 PragFlg_NeedSchema | PragFlg_Result1 | PragFlg_SchemaOpt,
	 /* ColNames:  */ 29, 8,
	 /* iArg:      */ 0},
#endif
#if !defined(SQLITE_OMIT_FLAG_PRAGMAS)
#if !defined(SQLITE_OMIT_FOREIGN_KEY) && !defined(SQLITE_OMIT_TRIGGER)
	{ /* zName:     */ "foreign_keys",
	 /* ePragTyp:  */ PragTyp_FLAG,
	 /* ePragFlg:  */ PragFlg_Result0 | PragFlg_NoColumns1,
	 /* ColNames:  */ 0, 0,
	 /* iArg:      */ SQLITE_ForeignKeys},
#endif
#endif
#if !defined(SQLITE_OMIT_FLAG_PRAGMAS)
	{ /* zName:     */ "full_column_names",
	 /* ePragTyp:  */ PragTyp_FLAG,
	 /* ePragFlg:  */ PragFlg_Result0 | PragFlg_NoColumns1,
	 /* ColNames:  */ 0, 0,
	 /* iArg:      */ SQLITE_FullColNames},
#endif
#if !defined(SQLITE_OMIT_FLAG_PRAGMAS)
#if !defined(SQLITE_OMIT_CHECK)
	{ /* zName:     */ "ignore_check_constraints",
	 /* ePragTyp:  */ PragTyp_FLAG,
	 /* ePragFlg:  */ PragFlg_Result0 | PragFlg_NoColumns1,
	 /* ColNames:  */ 0, 0,
	 /* iArg:      */ SQLITE_IgnoreChecks},
#endif
#endif
#if !defined(SQLITE_OMIT_SCHEMA_PRAGMAS)
	{ /* zName:     */ "index_info",
	 /* ePragTyp:  */ PragTyp_INDEX_INFO,
	 /* ePragFlg:  */
	 PragFlg_NeedSchema | PragFlg_Result1 | PragFlg_SchemaOpt,
	 /* ColNames:  */ 10, 3,
	 /* iArg:      */ 0},
	{ /* zName:     */ "index_list",
	 /* ePragTyp:  */ PragTyp_INDEX_LIST,
	 /* ePragFlg:  */
	 PragFlg_NeedSchema | PragFlg_Result1 | PragFlg_SchemaOpt,
	 /* ColNames:  */ 19, 5,
	 /* iArg:      */ 0},
	{ /* zName:     */ "index_xinfo",
	 /* ePragTyp:  */ PragTyp_INDEX_INFO,
	 /* ePragFlg:  */
	 PragFlg_NeedSchema | PragFlg_Result1 | PragFlg_SchemaOpt,
	 /* ColNames:  */ 13, 6,
	 /* iArg:      */ 1},
#endif
#if defined(SQLITE_DEBUG) && !defined(SQLITE_OMIT_PARSER_TRACE)
	{ /* zName:     */ "parser_trace",
	 /* ePragTyp:  */ PragTyp_PARSER_TRACE,
	 /* ePragFlg:  */ 0,
	 /* ColNames:  */ 0, 0,
	 /* iArg:      */ 0},
#endif
#if !defined(SQLITE_OMIT_FLAG_PRAGMAS)
	{ /* zName:     */ "query_only",
	 /* ePragTyp:  */ PragTyp_FLAG,
	 /* ePragFlg:  */ PragFlg_Result0 | PragFlg_NoColumns1,
	 /* ColNames:  */ 0, 0,
	 /* iArg:      */ SQLITE_QueryOnly},
	{ /* zName:     */ "read_uncommitted",
	 /* ePragTyp:  */ PragTyp_FLAG,
	 /* ePragFlg:  */ PragFlg_Result0 | PragFlg_NoColumns1,
	 /* ColNames:  */ 0, 0,
	 /* iArg:      */ SQLITE_ReadUncommitted},
	{ /* zName:     */ "recursive_triggers",
	 /* ePragTyp:  */ PragTyp_FLAG,
	 /* ePragFlg:  */ PragFlg_Result0 | PragFlg_NoColumns1,
	 /* ColNames:  */ 0, 0,
	 /* iArg:      */ SQLITE_RecTriggers},
#endif
#if !defined(SQLITE_OMIT_FLAG_PRAGMAS)
	{ /* zName:     */ "reverse_unordered_selects",
	 /* ePragTyp:  */ PragTyp_FLAG,
	 /* ePragFlg:  */ PragFlg_Result0 | PragFlg_NoColumns1,
	 /* ColNames:  */ 0, 0,
	 /* iArg:      */ SQLITE_ReverseOrder},
#endif
#if !defined(SQLITE_OMIT_SCHEMA_VERSION_PRAGMAS)
	{ /* zName:     */ "schema_version",
	 /* ePragTyp:  */ PragTyp_HEADER_VALUE,
	 /* ePragFlg:  */ PragFlg_NoColumns1 | PragFlg_Result0,
	 /* ColNames:  */ 0, 0,
	  /* Tarantool: need to take schema version from
	   * backend.
	   */
	 /* iArg:      */ 0},
#endif
#if !defined(SQLITE_OMIT_FLAG_PRAGMAS) && defined(SQLITE_ENABLE_SELECTTRACE)
	{ /* zName:     */ "select_trace",
	/* ePragTyp:  */ PragTyp_FLAG,
	/* ePragFlg:  */ PragFlg_Result0 | PragFlg_NoColumns1,
	/* ColNames:  */ 0, 0,
	/* iArg:      */ SQLITE_SelectTrace},
#endif
#if !defined(SQLITE_OMIT_FLAG_PRAGMAS)
	{ /* zName:     */ "short_column_names",
	 /* ePragTyp:  */ PragTyp_FLAG,
	 /* ePragFlg:  */ PragFlg_Result0 | PragFlg_NoColumns1,
	 /* ColNames:  */ 0, 0,
	 /* iArg:      */ SQLITE_ShortColNames},
#endif
#if !defined(SQLITE_OMIT_FLAG_PRAGMAS)
#if defined(SQLITE_DEBUG)
	{ /* zName:     */ "sql_trace",
	 /* ePragTyp:  */ PragTyp_FLAG,
	 /* ePragFlg:  */ PragFlg_Result0 | PragFlg_NoColumns1,
	 /* ColNames:  */ 0, 0,
	 /* iArg:      */ SQLITE_SqlTrace},
#endif
#endif
#if !defined(SQLITE_OMIT_SCHEMA_PRAGMAS)
	{ /* zName:     */ "stats",
	 /* ePragTyp:  */ PragTyp_STATS,
	 /* ePragFlg:  */
	 PragFlg_NeedSchema | PragFlg_Result0 | PragFlg_SchemaReq,
	 /* ColNames:  */ 6, 4,
	 /* iArg:      */ 0},
#endif
#if !defined(SQLITE_OMIT_SCHEMA_PRAGMAS)
	{ /* zName:     */ "table_info",
	 /* ePragTyp:  */ PragTyp_TABLE_INFO,
	 /* ePragFlg:  */
	 PragFlg_NeedSchema | PragFlg_Result1 | PragFlg_SchemaOpt,
	 /* ColNames:  */ 0, 6,
	 /* iArg:      */ 0},
#endif
#if !defined(SQLITE_OMIT_FLAG_PRAGMAS)
#if defined(SQLITE_DEBUG)
	{ /* zName:     */ "vdbe_addoptrace",
	 /* ePragTyp:  */ PragTyp_FLAG,
	 /* ePragFlg:  */ PragFlg_Result0 | PragFlg_NoColumns1,
	 /* ColNames:  */ 0, 0,
	 /* iArg:      */ SQLITE_VdbeAddopTrace},
	{ /* zName:     */ "vdbe_debug",
	 /* ePragTyp:  */ PragTyp_FLAG,
	 /* ePragFlg:  */ PragFlg_Result0 | PragFlg_NoColumns1,
	 /* ColNames:  */ 0, 0,
	 /* iArg:      */
	 SQLITE_SqlTrace | SQLITE_VdbeListing | SQLITE_VdbeTrace},
	{ /* zName:     */ "vdbe_eqp",
	 /* ePragTyp:  */ PragTyp_FLAG,
	 /* ePragFlg:  */ PragFlg_Result0 | PragFlg_NoColumns1,
	 /* ColNames:  */ 0, 0,
	 /* iArg:      */ SQLITE_VdbeEQP},
	{ /* zName:     */ "vdbe_listing",
	 /* ePragTyp:  */ PragTyp_FLAG,
	 /* ePragFlg:  */ PragFlg_Result0 | PragFlg_NoColumns1,
	 /* ColNames:  */ 0, 0,
	 /* iArg:      */ SQLITE_VdbeListing},
	{ /* zName:     */ "vdbe_trace",
	 /* ePragTyp:  */ PragTyp_FLAG,
	 /* ePragFlg:  */ PragFlg_Result0 | PragFlg_NoColumns1,
	 /* ColNames:  */ 0, 0,
	 /* iArg:      */ SQLITE_VdbeTrace},
#endif
#endif
#if !defined(SQLITE_OMIT_FLAG_PRAGMAS) && defined(SQLITE_ENABLE_WHERETRACE)

	{ /* zName:     */ "where_trace",
	/* ePragTyp:  */ PragTyp_FLAG,
	/* ePragFlg:  */ PragFlg_Result0 | PragFlg_NoColumns1,
	/* ColNames:  */ 0, 0,
	/* iArg:      */ SQLITE_WhereTrace},
#endif
};
/* Number of pragmas: 36 on by default, 47 total. */

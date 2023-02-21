/*-------------------------------------------------------------------------
 *
 * mongo_fdw.h
 *
 *-------------------------------------------------------------------------
 */

#ifndef MONGO_FDW_H
#define MONGO_FDW_H

#include "mongo_wrapper.h"
#include "bson.h"
#include "mongoc.h"
#include "fmgr.h"
#include "catalog/pg_foreign_server.h"
#include "catalog/pg_foreign_table.h"
#include "utils/datetime.h"
#include "nodes/pg_list.h"
#include "nodes/relation.h"
#include "utils/timestamp.h"
#include "access/reloptions.h"
#include "catalog/pg_type.h"
#include "commands/defrem.h"
#include "commands/explain.h"
#include "commands/vacuum.h"
#include "foreign/fdwapi.h"
#include "foreign/foreign.h"
#include "nodes/makefuncs.h"
#include "optimizer/cost.h"
#include "optimizer/pathnode.h"
#include "optimizer/plancat.h"
#include "optimizer/planmain.h"
#include "optimizer/restrictinfo.h"
#include "utils/array.h"
#include "utils/builtins.h"
#include "utils/date.h"
#include "utils/hsearch.h"
#include "utils/lsyscache.h"
#include "utils/rel.h"
#include "utils/memutils.h"
#include "catalog/pg_user_mapping.h"

/* Defines for valid type names */
#define BSON bson_t
#define BSON_TYPE bson_type_t
#define BSON_ITERATOR bson_iter_t
#define MONGO_CONN mongoc_client_t
#define MONGO_CURSOR mongoc_cursor_t
#define BSON_TYPE_DOCUMENT BSON_TYPE_DOCUMENT
#define BSON_TYPE_NULL BSON_TYPE_NULL
#define BSON_TYPE_ARRAY BSON_TYPE_ARRAY
#define BSON_TYPE_INT32 BSON_TYPE_INT32
#define BSON_TYPE_INT64 BSON_TYPE_INT64
#define BSON_TYPE_DOUBLE BSON_TYPE_DOUBLE
#define BSON_TYPE_BINDATA BSON_TYPE_BINARY
#define BSON_TYPE_BOOL BSON_TYPE_BOOL
#define BSON_TYPE_UTF8 BSON_TYPE_UTF8
#define BSON_TYPE_OID BSON_TYPE_OID
#define BSON_TYPE_DATE_TIME BSON_TYPE_DATE_TIME
#define BSON_TYPE_SYMBOL BSON_TYPE_SYMBOL
#define BSON_TYPE_UNDEFINED BSON_TYPE_UNDEFINED
#define BSON_TYPE_REGEX BSON_TYPE_REGEX
#define BSON_TYPE_CODE BSON_TYPE_CODE
#define BSON_TYPE_CODEWSCOPE BSON_TYPE_CODEWSCOPE
#define BSON_TYPE_TIMESTAMP BSON_TYPE_TIMESTAMP

#define PREF_READ_PRIMARY_NAME "readPrimary"
#define PREF_READ_SECONDARY_NAME "readSecondary"
#define PREF_READ_PRIMARY_PREFERRED_NAME "readPrimaryPreferred"
#define PREF_READ_SECONDARY_PREFERRED_NAME "readSecondaryPreferred"
#define PREF_READ_NEAREST_NAME "readNearest"

#define BSON_ITER_BOOL bson_iter_bool
#define BSON_ITER_DOUBLE bson_iter_double
#define BSON_ITER_INT32 bson_iter_int32
#define BSON_ITER_INT64 bson_iter_int64
#define BSON_ITER_OID bson_iter_oid
#define BSON_ITER_UTF8 bson_iter_utf8
#define BSON_ITER_REGEX bson_iter_regex
#define BSON_ITER_DATE_TIME bson_iter_date_time
#define BSON_ITER_CODE bson_iter_code
#define BSON_ITER_VALUE bson_iter_value
#define BSON_ITER_KEY bson_iter_key
#define BSON_ITER_NEXT bson_iter_next
#define BSON_ITER_TYPE bson_iter_type
#define BSON_ITER_BINARY bson_iter_binary


/* Defines for valid option names */
#define OPTION_NAME_ADDRESS "address"
#define OPTION_NAME_PORT "port"
#define OPTION_NAME_DATABASE "database"
#define OPTION_NAME_COLLECTION "collection"
#define OPTION_NAME_USERNAME "username"
#define OPTION_NAME_PASSWORD "password"
#define OPTION_NAME_READ_PREFERENCE "read_preference"

/* Default values for option parameters */
#define DEFAULT_IP_ADDRESS "127.0.0.1"
#define DEFAULT_PORT_NUMBER 27017
#define DEFAULT_DATABASE_NAME "test"

/* Defines for sending queries and converting types */
#define EQUALITY_OPERATOR_NAME "="
#define INITIAL_ARRAY_CAPACITY 8
#define MONGO_TUPLE_COST_MULTIPLIER 5
#define MONGO_CONNECTION_COST_MULTIPLIER 5
#define POSTGRES_TO_UNIX_EPOCH_DAYS (POSTGRES_EPOCH_JDATE - UNIX_EPOCH_JDATE)
#define POSTGRES_TO_UNIX_EPOCH_USECS (POSTGRES_TO_UNIX_EPOCH_DAYS * USECS_PER_DAY)


/*
 * MongoValidOption keeps an option name and a context. When an option is passed
 * into mongo_fdw objects (server and foreign table), we compare this option's
 * name and context against those of valid options.
 */
typedef struct MongoValidOption
{
    const char *optionName;
    Oid optionContextId;

} MongoValidOption;


/* Array of options that are valid for mongo_fdw */
static const uint32 ValidOptionCount = 7;
static const MongoValidOption ValidOptionArray[] =
        {
                /* foreign server options */
                {OPTION_NAME_ADDRESS, ForeignServerRelationId},
                {OPTION_NAME_PORT, ForeignServerRelationId},
                {OPTION_NAME_READ_PREFERENCE, ForeignServerRelationId},

                /* foreign table options */
                {OPTION_NAME_DATABASE, ForeignTableRelationId},
                {OPTION_NAME_COLLECTION, ForeignTableRelationId},

                /* User mapping options */
                {OPTION_NAME_USERNAME, UserMappingRelationId},
                {OPTION_NAME_PASSWORD, UserMappingRelationId}

        };


/*
 * MongoFdwOptions holds the option values to be used when connecting to Mongo.
 * To resolve these values, we first check foreign table's options, and if not
 * present, we then fall back to the default values specified above.
 */
typedef struct MongoFdwOptions
{
    char *svr_address;
    int32 svr_port;
    char *svr_database;
    char *collectionName;
    char *svr_username;
    char *svr_password;
    char *readPreference;
} MongoFdwOptions;


/*
 * MongoFdwExecState keeps foreign data wrapper specific execution state that we
 * create and hold onto when executing the query.
 */
/*
 * Execution state of a foreign insert/update/delete operation.
 */
typedef struct MongoFdwModifyState
{
    Relation rel;                /* relcache entry for the foreign table */
    List *target_attrs;        /* list of target attribute numbers */

    /* info about parameters for prepared statement */
    int p_nums;                /* number of parameters to transmit */
    FmgrInfo *p_flinfo;            /* output conversion functions for them */

    struct HTAB *columnMappingHash;

    MONGO_CONN *mongoConnection;    /* MongoDB connection */
    MONGO_CURSOR *mongoCursor;        /* MongoDB cursor */
    BSON *queryDocument;        /* Bson Document */

    MongoFdwOptions *options;

    /* working memory context */
    MemoryContext temp_cxt;            /* context for per-tuple temporary data */
} MongoFdwModifyState;


/*
 * ColumnMapping reprents a hash table entry that maps a column name to column
 * related information. We construct these hash table entries to speed up the
 * conversion from BSON documents to PostgreSQL tuples; and each hash entry maps
 * the column name to the column's tuple index and its type-related information.
 */
typedef struct ColumnMapping
{
    char columnName[NAMEDATALEN];
    uint32 columnIndex;
    Oid columnTypeId;
    int32 columnTypeMod;
    Oid columnArrayTypeId;
} ColumnMapping;

/* options.c */
extern MongoFdwOptions *mongo_get_options(Oid foreignTableId);

extern void mongo_free_options(MongoFdwOptions *options);

extern StringInfo mongo_option_names_string(Oid currentContextId);

/* connection.c */
MONGO_CONN *mongo_get_connection(ForeignServer *server, UserMapping *user, MongoFdwOptions *opt);

extern void mongo_cleanup_connection(void);

extern void mongo_release_connection(MONGO_CONN *conn);

/* Function declarations related to creating the mongo query */
extern List *ApplicableOpExpressionList(RelOptInfo *baserel);

extern BSON *QueryDocument(Oid relationId, List *opExpressionList);

extern List *ColumnList(RelOptInfo *baserel);

/* Function declarations for foreign data wrapper */
extern Datum mongo_fdw_handler(PG_FUNCTION_ARGS);

extern Datum mongo_fdw_validator(PG_FUNCTION_ARGS);


#endif   /* MONGO_FDW_H */

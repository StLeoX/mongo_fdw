/*-------------------------------------------------------------------------
 *
 * mongo_query.h
 *
 *-------------------------------------------------------------------------
 */

#ifndef MONGO_QUERY_H
#define MONGO_QUERY_H


#define NUMERICARRAY_OID 1231

bool AppenMongoValue(BSON *queryDocument, const char *keyName, Datum value, bool isnull, Oid id);

#endif /* MONGO_QUERY_H */

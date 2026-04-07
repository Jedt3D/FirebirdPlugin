// FirebirdLoader.h - Dynamic loading of Firebird client library for cross-architecture support

#ifndef FIREBIRD_LOADER_H
#define FIREBIRD_LOADER_H

#include <windows.h>
#include <ibase.h>

// Firebird function pointers
extern decltype(isc_attach_database)* ptr_isc_attach_database;
extern decltype(isc_dsql_execute)* ptr_isc_dsql_execute;
extern decltype(isc_dsql_fetch)* ptr_isc_dsql_fetch;
extern decltype(isc_dsql_prepare)* ptr_isc_dsql_prepare;
extern decltype(isc_start_transaction)* ptr_isc_start_transaction;
extern decltype(isc_commit_transaction)* ptr_isc_commit_transaction;
extern decltype(isc_rollback_transaction)* ptr_isc_rollback_transaction;
extern decltype(isc_detach_database)* ptr_isc_detach_database;
extern decltype(isc_open_blob2)* ptr_isc_open_blob2;
extern decltype(isc_close_blob)* ptr_isc_close_blob;
extern decltype(isc_blob_info)* ptr_isc_blob_info;
extern decltype(isc_seek_blob)* ptr_isc_seek_blob;
extern decltype(isc_get_segment)* ptr_isc_get_segment;
extern decltype(isc_put_segment)* ptr_isc_put_segment;
extern decltype(isc_create_blob2)* ptr_isc_create_blob2;
extern decltype(isc_dsql_allocate_statement)* ptr_isc_dsql_allocate_statement;
extern decltype(isc_dsql_describe)* ptr_isc_dsql_describe;
extern decltype(isc_dsql_describe_bind)* ptr_isc_dsql_describe_bind;
extern decltype(isc_dsql_execute2)* ptr_isc_dsql_execute2;
extern decltype(isc_dsql_execute_immediate)* ptr_isc_dsql_execute_immediate;
extern decltype(isc_dsql_free_statement)* ptr_isc_dsql_free_statement;
extern decltype(isc_dsql_sql_info)* ptr_isc_dsql_sql_info;
extern decltype(isc_database_info)* ptr_isc_database_info;
extern decltype(isc_transaction_info)* ptr_isc_transaction_info;
extern decltype(isc_service_attach)* ptr_isc_service_attach;
extern decltype(isc_service_detach)* ptr_isc_service_detach;
extern decltype(isc_service_query)* ptr_isc_service_query;
extern decltype(isc_service_start)* ptr_isc_service_start;
extern decltype(isc_cancel_events)* ptr_isc_cancel_events;
extern decltype(isc_que_events)* ptr_isc_que_events;
extern decltype(isc_event_block)* ptr_isc_event_block;
extern decltype(isc_event_counts)* ptr_isc_event_counts;
extern decltype(isc_free)* ptr_isc_free;
extern decltype(isc_sqlcode)* ptr_isc_sqlcode;
extern decltype(fb_interpret)* ptr_fb_interpret;
extern decltype(isc_vax_integer)* ptr_isc_vax_integer;
extern decltype(isc_portable_integer)* ptr_isc_portable_integer;
extern decltype(isc_decode_sql_date)* ptr_isc_decode_sql_date;
extern decltype(isc_decode_sql_time)* ptr_isc_decode_sql_time;
extern decltype(isc_decode_timestamp)* ptr_isc_decode_timestamp;
extern decltype(isc_encode_sql_date)* ptr_isc_encode_sql_date;
extern decltype(isc_encode_sql_time)* ptr_isc_encode_sql_time;
extern decltype(isc_encode_timestamp)* ptr_isc_encode_timestamp;

// Load Firebird DLL and resolve function pointers
bool LoadFirebirdClient(const char* fbClientPath = nullptr);
void UnloadFirebirdClient();

#endif // FIREBIRD_LOADER_H

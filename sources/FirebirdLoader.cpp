// FirebirdLoader.cpp - Dynamic loading implementation

#include "FirebirdLoader.h"
#include <stdexcept>
#include <string>

// Function pointers
decltype(isc_attach_database)* ptr_isc_attach_database = nullptr;
decltype(isc_dsql_execute)* ptr_isc_dsql_execute = nullptr;
decltype(isc_dsql_fetch)* ptr_isc_dsql_fetch = nullptr;
decltype(isc_dsql_prepare)* ptr_isc_dsql_prepare = nullptr;
decltype(isc_start_transaction)* ptr_isc_start_transaction = nullptr;
decltype(isc_commit_transaction)* ptr_isc_commit_transaction = nullptr;
decltype(isc_rollback_transaction)* ptr_isc_rollback_transaction = nullptr;
decltype(isc_detach_database)* ptr_isc_detach_database = nullptr;
decltype(isc_open_blob2)* ptr_isc_open_blob2 = nullptr;
decltype(isc_close_blob)* ptr_isc_close_blob = nullptr;
decltype(isc_get_segment)* ptr_isc_get_segment = nullptr;
decltype(isc_put_segment)* ptr_isc_put_segment = nullptr;
decltype(isc_create_blob2)* ptr_isc_create_blob2 = nullptr;
decltype(isc_dsql_allocate_statement)* ptr_isc_dsql_allocate_statement = nullptr;
decltype(isc_dsql_describe)* ptr_isc_dsql_describe = nullptr;
decltype(isc_dsql_describe_bind)* ptr_isc_dsql_describe_bind = nullptr;
decltype(isc_dsql_execute2)* ptr_isc_dsql_execute2 = nullptr;
decltype(isc_dsql_execute_immediate)* ptr_isc_dsql_execute_immediate = nullptr;
decltype(isc_dsql_free_statement)* ptr_isc_dsql_free_statement = nullptr;
decltype(isc_dsql_sql_info)* ptr_isc_dsql_sql_info = nullptr;
decltype(isc_database_info)* ptr_isc_database_info = nullptr;
decltype(isc_transaction_info)* ptr_isc_transaction_info = nullptr;
decltype(isc_service_attach)* ptr_isc_service_attach = nullptr;
decltype(isc_service_detach)* ptr_isc_service_detach = nullptr;
decltype(isc_service_query)* ptr_isc_service_query = nullptr;
decltype(isc_service_start)* ptr_isc_service_start = nullptr;
decltype(isc_cancel_events)* ptr_isc_cancel_events = nullptr;
decltype(isc_que_events)* ptr_isc_que_events = nullptr;
decltype(isc_event_block)* ptr_isc_event_block = nullptr;
decltype(isc_event_counts)* ptr_isc_event_counts = nullptr;
decltype(isc_free)* ptr_isc_free = nullptr;
decltype(isc_sqlcode)* ptr_isc_sqlcode = nullptr;
decltype(fb_interpret)* ptr_fb_interpret = nullptr;
decltype(isc_vax_integer)* ptr_isc_vax_integer = nullptr;
decltype(isc_portable_integer)* ptr_isc_portable_integer = nullptr;
decltype(isc_decode_sql_date)* ptr_isc_decode_sql_date = nullptr;
decltype(isc_decode_sql_time)* ptr_isc_decode_sql_time = nullptr;
decltype(isc_decode_timestamp)* ptr_isc_decode_timestamp = nullptr;
decltype(isc_encode_sql_date)* ptr_isc_encode_sql_date = nullptr;
decltype(isc_encode_sql_time)* ptr_isc_encode_sql_time = nullptr;
decltype(isc_encode_timestamp)* ptr_isc_encode_timestamp = nullptr;

static HMODULE firebirdDllHandle = nullptr;

bool LoadFirebirdClient(const char* fbClientPath) {
    if (firebirdDllHandle) {
        return true; // Already loaded
    }

    std::string dllPath = fbClientPath ? fbClientPath : "fbclient.dll";

    firebirdDllHandle = LoadLibraryA(dllPath.c_str());
    if (!firebirdDllHandle) {
        return false;
    }

    // Resolve all function pointers
    #define RESOLVE_FUNCTION(name) \
        ptr_##name = (decltype(name)*)GetProcAddress(firebirdDllHandle, #name); \
        if (!ptr_##name) { \
            FreeLibrary(firebirdDllHandle); \
            firebirdDllHandle = nullptr; \
            return false; \
        }

    RESOLVE_FUNCTION(isc_attach_database);
    RESOLVE_FUNCTION(isc_dsql_execute);
    RESOLVE_FUNCTION(isc_dsql_fetch);
    RESOLVE_FUNCTION(isc_dsql_prepare);
    RESOLVE_FUNCTION(isc_start_transaction);
    RESOLVE_FUNCTION(isc_commit_transaction);
    RESOLVE_FUNCTION(isc_rollback_transaction);
    RESOLVE_FUNCTION(isc_detach_database);
    RESOLVE_FUNCTION(isc_open_blob2);
    RESOLVE_FUNCTION(isc_close_blob);
    RESOLVE_FUNCTION(isc_get_segment);
    RESOLVE_FUNCTION(isc_put_segment);
    RESOLVE_FUNCTION(isc_create_blob2);
    RESOLVE_FUNCTION(isc_dsql_allocate_statement);
    RESOLVE_FUNCTION(isc_dsql_describe);
    RESOLVE_FUNCTION(isc_dsql_describe_bind);
    RESOLVE_FUNCTION(isc_dsql_execute2);
    RESOLVE_FUNCTION(isc_dsql_execute_immediate);
    RESOLVE_FUNCTION(isc_dsql_free_statement);
    RESOLVE_FUNCTION(isc_dsql_sql_info);
    RESOLVE_FUNCTION(isc_database_info);
    RESOLVE_FUNCTION(isc_transaction_info);
    RESOLVE_FUNCTION(isc_service_attach);
    RESOLVE_FUNCTION(isc_service_detach);
    RESOLVE_FUNCTION(isc_service_query);
    RESOLVE_FUNCTION(isc_service_start);
    RESOLVE_FUNCTION(isc_cancel_events);
    RESOLVE_FUNCTION(isc_que_events);
    RESOLVE_FUNCTION(isc_event_block);
    RESOLVE_FUNCTION(isc_event_counts);
    RESOLVE_FUNCTION(isc_free);
    RESOLVE_FUNCTION(isc_sqlcode);
    RESOLVE_FUNCTION(fb_interpret);
    RESOLVE_FUNCTION(isc_vax_integer);
    RESOLVE_FUNCTION(isc_portable_integer);
    RESOLVE_FUNCTION(isc_decode_sql_date);
    RESOLVE_FUNCTION(isc_decode_sql_time);
    RESOLVE_FUNCTION(isc_decode_timestamp);
    RESOLVE_FUNCTION(isc_encode_sql_date);
    RESOLVE_FUNCTION(isc_encode_sql_time);
    RESOLVE_FUNCTION(isc_encode_timestamp);

    #undef RESOLVE_FUNCTION

    return true;
}

void UnloadFirebirdClient() {
    if (firebirdDllHandle) {
        FreeLibrary(firebirdDllHandle);
        firebirdDllHandle = nullptr;
    }
}

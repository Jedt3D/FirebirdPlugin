// FirebirdDB.cpp — Implementation of thin C++ wrapper over libfbclient

#include "FirebirdDB.h"

#if defined(_WIN64) && defined(__aarch64__)
// ARM64 builds use dynamic loading for cross-architecture support
#include "FirebirdLoader.h"

// Macros to redirect function calls to pointers for ARM64 builds
#define isc_attach_database ptr_isc_attach_database
#define isc_dsql_execute ptr_isc_dsql_execute
#define isc_dsql_fetch ptr_isc_dsql_fetch
#define isc_dsql_prepare ptr_isc_dsql_prepare
#define isc_start_transaction ptr_isc_start_transaction
#define isc_commit_transaction ptr_isc_commit_transaction
#define isc_rollback_transaction ptr_isc_rollback_transaction
#define isc_detach_database ptr_isc_detach_database
#define isc_open_blob2 ptr_isc_open_blob2
#define isc_close_blob ptr_isc_close_blob
#define isc_blob_info ptr_isc_blob_info
#define isc_seek_blob ptr_isc_seek_blob
#define isc_get_segment ptr_isc_get_segment
#define isc_put_segment ptr_isc_put_segment
#define isc_create_blob2 ptr_isc_create_blob2
#define isc_dsql_allocate_statement ptr_isc_dsql_allocate_statement
#define isc_dsql_describe ptr_isc_dsql_describe
#define isc_dsql_describe_bind ptr_isc_dsql_describe_bind
#define isc_dsql_execute2 ptr_isc_dsql_execute2
#define isc_dsql_execute_immediate ptr_isc_dsql_execute_immediate
#define isc_dsql_free_statement ptr_isc_dsql_free_statement
#define isc_dsql_sql_info ptr_isc_dsql_sql_info
#define isc_database_info ptr_isc_database_info
#define isc_transaction_info ptr_isc_transaction_info
#define isc_service_attach ptr_isc_service_attach
#define isc_service_detach ptr_isc_service_detach
#define isc_service_query ptr_isc_service_query
#define isc_service_start ptr_isc_service_start
#define isc_sqlcode ptr_isc_sqlcode
#define fb_interpret ptr_fb_interpret
#define isc_vax_integer ptr_isc_vax_integer
#define isc_portable_integer ptr_isc_portable_integer
#define isc_decode_sql_date ptr_isc_decode_sql_date
#define isc_decode_sql_time ptr_isc_decode_sql_time
#define isc_decode_timestamp ptr_isc_decode_timestamp
#define isc_encode_sql_date ptr_isc_encode_sql_date
#define isc_encode_sql_time ptr_isc_encode_sql_time
#define isc_encode_timestamp ptr_isc_encode_timestamp
#endif

#if __has_include(<firebird/fb_c_api.h>) && !(defined(_WIN64) && defined(__aarch64__))
#define FB_HAS_MODERN_API 1
#else
#define FB_HAS_MODERN_API 0
#endif

#include <algorithm>
#include <array>
#include <cctype>
#include <iomanip>
#include <cmath>
#include <limits>
#include <sstream>

extern "C" {
#if FB_HAS_MODERN_API
#include <firebird/fb_c_api.h>
#else
#include <ibase.h>
#endif
}

namespace {

#if FB_HAS_MODERN_API

struct FirebirdUtilityInterfaces {
    IMaster *master = nullptr;
    IUtil *util = nullptr;
    IDecFloat16 *dec16 = nullptr;
    IDecFloat34 *dec34 = nullptr;
    IInt128 *int128 = nullptr;

    FirebirdUtilityInterfaces() {
        master = fb_get_master_interface();
        if (!master) return;

        util = IMaster_getUtilInterface(master);
        if (!util) return;

        IStatus *status = IMaster_getStatus(master);
        if (!status) return;

        IStatus_init(status);
        dec16 = IUtil_getDecFloat16(util, status);
        IStatus_init(status);
        dec34 = IUtil_getDecFloat34(util, status);
        IStatus_init(status);
        int128 = IUtil_getInt128(util, status);
        IStatus_dispose(status);
    }
};

FirebirdUtilityInterfaces &GetFirebirdUtilities() {
    static FirebirdUtilityInterfaces utilities;
    return utilities;
}

class FirebirdStatusScope {
public:
    FirebirdStatusScope() {
        auto &utilities = GetFirebirdUtilities();
        if (utilities.master) {
            mStatus = IMaster_getStatus(utilities.master);
            if (mStatus) IStatus_init(mStatus);
        }
    }

    ~FirebirdStatusScope() {
        if (mStatus) IStatus_dispose(mStatus);
    }

    IStatus *get() const { return mStatus; }

    bool ok() const {
        return mStatus && !(IStatus_getState(mStatus) & IStatus_STATE_ERRORS);
    }

private:
    IStatus *mStatus = nullptr;
};

#else

// Forward declarations for Firebird modern interfaces (not supported in this build)
struct IStatus;
struct IUtil;
struct IXpbBuilder;

// Modern interface constants
const unsigned IXpbBuilder_SPB_START = 0;

// Forward declarations for stub functions
extern "C" {
    // IUtil interface stubs
    void IUtil_decodeTimeStampTz(IUtil* util, void* status, const void* value, unsigned* year, unsigned* month, unsigned* day,
                                  unsigned* hour, unsigned* minute, unsigned* second, unsigned* fractions,
                                  unsigned timeZoneSize, char* timeZone);
    void IUtil_encodeTimeTz(IUtil* util, void* status, void* value, unsigned hour, unsigned minute, unsigned second,
                            unsigned fractions, const char* timeZone);
    void IUtil_encodeTimeStampTz(IUtil* util, void* status, void* value, unsigned year, unsigned month, unsigned day,
                                  unsigned hour, unsigned minute, unsigned second, unsigned fractions, const char* timeZone);

    // IXpbBuilder interface stubs
    IXpbBuilder* IUtil_getXpbBuilder(IUtil* util, void* status, unsigned kind, void* opts, unsigned optsSize);
    void IXpbBuilder_dispose(IXpbBuilder* builder);
    void IXpbBuilder_insertTag(IXpbBuilder* builder, void* status, unsigned tag);
    void IXpbBuilder_insertString(IXpbBuilder* builder, void* status, unsigned tag, const char* value);
    void IXpbBuilder_insertInt(IXpbBuilder* builder, void* status, unsigned tag, int value);
    void IXpbBuilder_insertBigInt(IXpbBuilder* builder, void* status, unsigned tag, long long value);
    void IXpbBuilder_insertBytes(IXpbBuilder* builder, void* status, unsigned tag, const void* data, unsigned length);
    unsigned IXpbBuilder_getBufferLength(IXpbBuilder* builder, void* status);
    const unsigned char* IXpbBuilder_getBuffer(IXpbBuilder* builder, void* status);
}

// Simplified stub for Firebird modern interfaces
// These features (INT128, DECFLOAT, TIMEZONE types) are not supported in this build
struct FirebirdUtilityInterfaces {
    bool available = false;
    IUtil* util = nullptr;

    FirebirdUtilityInterfaces() {
        available = false; // Modern interfaces not available
        util = nullptr;
    }
};

FirebirdUtilityInterfaces &GetFirebirdUtilities() {
    static FirebirdUtilityInterfaces utilities;
    return utilities;
}

// Stub status class for modern interface compatibility
class FirebirdStatusScope {
public:
    FirebirdStatusScope() {}
    ~FirebirdStatusScope() {}

    void* get() const { return nullptr; }
    bool ok() const { return false; }
};

#endif

bool ParseUnsignedComponent(const std::string &text, unsigned maxValue, unsigned &out) {
    if (text.empty()) return false;
    char *endPtr = nullptr;
    unsigned long value = strtoul(text.c_str(), &endPtr, 10);
    if (!endPtr || *endPtr != '\0' || value > maxValue) return false;
    out = (unsigned)value;
    return true;
}

bool ParseFractionComponent(const std::string &text, unsigned &out) {
    if (text.empty()) {
        out = 0;
        return true;
    }
    if (text.size() > 4) return false;

    std::string padded = text;
    while (padded.size() < 4) padded.push_back('0');
    return ParseUnsignedComponent(padded, 9999, out);
}

bool ParseTimeLiteral(const std::string &text,
                      unsigned &hour,
                      unsigned &minute,
                      unsigned &second,
                      unsigned &fractions)
{
    size_t firstColon = text.find(':');
    size_t secondColon = text.find(':', firstColon == std::string::npos ? firstColon : firstColon + 1);
    if (firstColon == std::string::npos || secondColon == std::string::npos) return false;

    std::string hourPart = text.substr(0, firstColon);
    std::string minutePart = text.substr(firstColon + 1, secondColon - firstColon - 1);
    std::string secondPart = text.substr(secondColon + 1);

    size_t dot = secondPart.find('.');
    std::string secondOnly = dot == std::string::npos ? secondPart : secondPart.substr(0, dot);
    std::string fractionPart = dot == std::string::npos ? "" : secondPart.substr(dot + 1);

    return ParseUnsignedComponent(hourPart, 23, hour) &&
           ParseUnsignedComponent(minutePart, 59, minute) &&
           ParseUnsignedComponent(secondOnly, 59, second) &&
           ParseFractionComponent(fractionPart, fractions);
}

bool ParseDateLiteral(const std::string &text, unsigned &year, unsigned &month, unsigned &day) {
    size_t firstDash = text.find('-');
    size_t secondDash = text.find('-', firstDash == std::string::npos ? firstDash : firstDash + 1);
    if (firstDash == std::string::npos || secondDash == std::string::npos) return false;

    std::string yearPart = text.substr(0, firstDash);
    std::string monthPart = text.substr(firstDash + 1, secondDash - firstDash - 1);
    std::string dayPart = text.substr(secondDash + 1);

    return ParseUnsignedComponent(yearPart, 9999, year) &&
           ParseUnsignedComponent(monthPart, 12, month) &&
           ParseUnsignedComponent(dayPart, 31, day);
}

#if FB_HAS_MODERN_API

bool FormatInt128Value(const FB_I128 &value, short scale, std::string &out) {
    auto &utilities = GetFirebirdUtilities();
    if (!utilities.int128) return false;

    FirebirdStatusScope status;
    if (!status.get()) return false;

    std::array<char, IInt128_STRING_SIZE + 1> buffer = {};
    IInt128_toString(utilities.int128, status.get(), &value, scale, (unsigned)buffer.size(), buffer.data());
    if (!status.ok()) return false;

    out.assign(buffer.data());
    return true;
}

bool ParseInt128Value(const std::string &text, short scale, FB_I128 &out) {
    auto &utilities = GetFirebirdUtilities();
    if (!utilities.int128) return false;

    FirebirdStatusScope status;
    if (!status.get()) return false;

    IInt128_fromString(utilities.int128, status.get(), scale, text.c_str(), &out);
    return status.ok();
}

bool FormatDecFloat16Value(const FB_DEC16 &value, std::string &out) {
    auto &utilities = GetFirebirdUtilities();
    if (!utilities.dec16) return false;

    FirebirdStatusScope status;
    if (!status.get()) return false;

    std::array<char, IDecFloat16_STRING_SIZE + 1> buffer = {};
    IDecFloat16_toString(utilities.dec16, status.get(), &value, (unsigned)buffer.size(), buffer.data());
    if (!status.ok()) return false;

    out.assign(buffer.data());
    return true;
}

bool ParseDecFloat16Value(const std::string &text, FB_DEC16 &out) {
    auto &utilities = GetFirebirdUtilities();
    if (!utilities.dec16) return false;

    FirebirdStatusScope status;
    if (!status.get()) return false;

    IDecFloat16_fromString(utilities.dec16, status.get(), text.c_str(), &out);
    return status.ok();
}

bool FormatDecFloat34Value(const FB_DEC34 &value, std::string &out) {
    auto &utilities = GetFirebirdUtilities();
    if (!utilities.dec34) return false;

    FirebirdStatusScope status;
    if (!status.get()) return false;

    std::array<char, IDecFloat34_STRING_SIZE + 1> buffer = {};
    IDecFloat34_toString(utilities.dec34, status.get(), &value, (unsigned)buffer.size(), buffer.data());
    if (!status.ok()) return false;

    out.assign(buffer.data());
    return true;
}

bool ParseDecFloat34Value(const std::string &text, FB_DEC34 &out) {
    auto &utilities = GetFirebirdUtilities();
    if (!utilities.dec34) return false;

    FirebirdStatusScope status;
    if (!status.get()) return false;

    IDecFloat34_fromString(utilities.dec34, status.get(), text.c_str(), &out);
    return status.ok();
}

#else

bool FormatInt128Value(const FB_I128 &value, short scale, std::string &out) {
    // Modern Firebird INT128 support not available
    return false;
}

bool ParseInt128Value(const std::string &text, short scale, FB_I128 &out) {
    // Modern Firebird INT128 support not available
    return false;
}

bool FormatDecFloat16Value(const FB_DEC16 &value, std::string &out) {
    // Modern Firebird DECFLOAT support not available
    return false;
}

bool ParseDecFloat16Value(const std::string &text, FB_DEC16 &out) {
    // Modern Firebird DECFLOAT support not available
    return false;
}

bool FormatDecFloat34Value(const FB_DEC34 &value, std::string &out) {
    // Modern Firebird DECFLOAT support not available
    return false;
}

bool ParseDecFloat34Value(const std::string &text, FB_DEC34 &out) {
    // Modern Firebird DECFLOAT support not available
    return false;
}

#endif

std::string FormatTimeZoneText(unsigned hour,
                               unsigned minute,
                               unsigned second,
                               unsigned fractions,
                               const char *timeZone)
{
    std::ostringstream out;
    out << std::setfill('0')
        << std::setw(2) << hour << ':'
        << std::setw(2) << minute << ':'
        << std::setw(2) << second
        << '.'
        << std::setw(4) << fractions;

    if (timeZone && *timeZone) out << ' ' << timeZone;
    return out.str();
}

std::string FormatTimestampZoneText(unsigned year,
                                    unsigned month,
                                    unsigned day,
                                    unsigned hour,
                                    unsigned minute,
                                    unsigned second,
                                    unsigned fractions,
                                    const char *timeZone)
{
    std::ostringstream out;
    out << std::setfill('0')
        << std::setw(4) << year << '-'
        << std::setw(2) << month << '-'
        << std::setw(2) << day << ' '
        << std::setw(2) << hour << ':'
        << std::setw(2) << minute << ':'
        << std::setw(2) << second
        << '.'
        << std::setw(4) << fractions;

    if (timeZone && *timeZone) out << ' ' << timeZone;
    return out.str();
}

#if FB_HAS_MODERN_API

bool FormatTimeTzValue(const ISC_TIME_TZ &value, std::string &out) {
    auto &utilities = GetFirebirdUtilities();
    if (!utilities.util) return false;

    FirebirdStatusScope status;
    if (!status.get()) return false;

    unsigned hour = 0;
    unsigned minute = 0;
    unsigned second = 0;
    unsigned fractions = 0;
    std::array<char, 128> timeZone = {};

    IUtil_decodeTimeTz(utilities.util, status.get(), &value,
                       &hour, &minute, &second, &fractions,
                       (unsigned)timeZone.size(), timeZone.data());
    if (!status.ok()) return false;

    out = FormatTimeZoneText(hour, minute, second, fractions, timeZone.data());
    return true;
}

bool FormatTimeTzExValue(const ISC_TIME_TZ_EX &value, std::string &out) {
    auto &utilities = GetFirebirdUtilities();
    if (!utilities.util) return false;

    FirebirdStatusScope status;
    if (!status.get()) return false;

    unsigned hour = 0;
    unsigned minute = 0;
    unsigned second = 0;
    unsigned fractions = 0;
    std::array<char, 128> timeZone = {};

    IUtil_decodeTimeTzEx(utilities.util, status.get(), &value,
                         &hour, &minute, &second, &fractions,
                         (unsigned)timeZone.size(), timeZone.data());
    if (!status.ok()) return false;

    out = FormatTimeZoneText(hour, minute, second, fractions, timeZone.data());
    return true;
}

bool ParseTimeTzValue(const std::string &text, ISC_TIME_TZ &out) {
    auto &utilities = GetFirebirdUtilities();
    if (!utilities.util) return false;

    size_t split = text.find_last_of(' ');
    if (split == std::string::npos) return false;

    std::string timePart = text.substr(0, split);
    std::string zonePart = text.substr(split + 1);

    unsigned hour = 0;
    unsigned minute = 0;
    unsigned second = 0;
    unsigned fractions = 0;
    if (!ParseTimeLiteral(timePart, hour, minute, second, fractions)) return false;

    FirebirdStatusScope status;
    if (!status.get()) return false;

    IUtil_encodeTimeTz(utilities.util, status.get(), &out, hour, minute, second, fractions, zonePart.c_str());
    return status.ok();
}

bool FormatTimestampTzValue(const ISC_TIMESTAMP_TZ &value, std::string &out) {
    auto &utilities = GetFirebirdUtilities();
    if (!utilities.util) return false;

    FirebirdStatusScope status;
    if (!status.get()) return false;

    unsigned year = 0;
    unsigned month = 0;
    unsigned day = 0;
    unsigned hour = 0;
    unsigned minute = 0;
    unsigned second = 0;
    unsigned fractions = 0;
    std::array<char, 128> timeZone = {};

    IUtil_decodeTimeStampTz(utilities.util, status.get(), &value,
                            &year, &month, &day, &hour, &minute, &second, &fractions,
                            (unsigned)timeZone.size(), timeZone.data());
    if (!status.ok()) return false;

    out = FormatTimestampZoneText(year, month, day, hour, minute, second, fractions, timeZone.data());
    return true;
}

bool FormatTimestampTzExValue(const ISC_TIMESTAMP_TZ_EX &value, std::string &out) {
    auto &utilities = GetFirebirdUtilities();
    if (!utilities.util) return false;

    FirebirdStatusScope status;
    if (!status.get()) return false;

    unsigned year = 0;
    unsigned month = 0;
    unsigned day = 0;
    unsigned hour = 0;
    unsigned minute = 0;
    unsigned second = 0;
    unsigned fractions = 0;
    std::array<char, 128> timeZone = {};

    IUtil_decodeTimeStampTzEx(utilities.util, status.get(), &value,
                              &year, &month, &day, &hour, &minute, &second, &fractions,
                              (unsigned)timeZone.size(), timeZone.data());
    if (!status.ok()) return false;

    out = FormatTimestampZoneText(year, month, day, hour, minute, second, fractions, timeZone.data());
    return true;
}

bool ParseTimestampTzValue(const std::string &text, ISC_TIMESTAMP_TZ &out) {
    auto &utilities = GetFirebirdUtilities();
    if (!utilities.util) return false;

    size_t split = text.find_last_of(' ');
    if (split == std::string::npos) return false;

    std::string timestampPart = text.substr(0, split);
    std::string zonePart = text.substr(split + 1);

    size_t spacePos = timestampPart.find(' ');
    if (spacePos == std::string::npos) return false;

    std::string datePart = timestampPart.substr(0, spacePos);
    std::string timePart = timestampPart.substr(spacePos + 1);

    unsigned year = 0;
    unsigned month = 0;
    unsigned day = 0;
    unsigned hour = 0;
    unsigned minute = 0;
    unsigned second = 0;
    unsigned fractions = 0;

    if (!ParseDateLiteral(datePart, year, month, day)) return false;
    if (!ParseTimeLiteral(timePart, hour, minute, second, fractions)) return false;

    FirebirdStatusScope status;
    if (!status.get()) return false;

    IUtil_encodeTimeStampTz(utilities.util, status.get(), &out,
                            year, month, day, hour, minute, second, fractions, zonePart.c_str());
    return status.ok();
}

#else

bool FormatTimeTzValue(const ISC_TIME_TZ &value, std::string &out) {
    // Modern Firebird TIMEZONE support not available
    return false;
}

// Modern TIMEZONE_EX support stub - Firebird 5.0.3 compatibility
bool FormatTimeTzExValue(const ISC_TIME_TZ_EX &value, std::string &out) {
    // Modern Firebird TIMEZONE_EX support not available
    return false;
}

// Modern TIMEZONE support stub - Firebird 5.0.3 compatibility
bool ParseTimeTzValue(const std::string &text, ISC_TIME_TZ &out) {
    // Modern Firebird TIMEZONE support not available
    return false;
}

bool FormatTimestampTzValue(const ISC_TIMESTAMP_TZ &value, std::string &out) {
    auto &utilities = GetFirebirdUtilities();
    if (!utilities.util) return false;

    FirebirdStatusScope status;
    if (!status.get()) return false;

    unsigned year = 0;
    unsigned month = 0;
    unsigned day = 0;
    unsigned hour = 0;
    unsigned minute = 0;
    unsigned second = 0;
    unsigned fractions = 0;
    std::array<char, 128> timeZone = {};

    IUtil_decodeTimeStampTz(utilities.util, status.get(), &value,
                            &year, &month, &day, &hour, &minute, &second, &fractions,
                            (unsigned)timeZone.size(), timeZone.data());
    if (!status.ok()) return false;

    out = FormatTimestampZoneText(year, month, day, hour, minute, second, fractions, timeZone.data());
    return true;
}

// Modern TIMESTAMP_TZ_EX support stub - Firebird 5.0.3 compatibility
bool FormatTimestampTzExValue(const ISC_TIMESTAMP_TZ_EX &value, std::string &out) {
    // Modern Firebird TIMESTAMP_TZ_EX support not available
    return false;
}

// Modern TIMESTAMP_TZ support stub - Firebird 5.0.3 compatibility
bool ParseTimestampTzValue(const std::string &text, ISC_TIMESTAMP_TZ &out) {
    // Modern Firebird TIMESTAMP_TZ support not available
    return false;
}

#endif

std::string NormalizeTextToken(const std::string &text) {
    std::string normalized;
    normalized.reserve(text.size());

    bool inWhitespace = false;
    for (unsigned char ch : text) {
        if (std::isspace(ch)) {
            if (!normalized.empty()) inWhitespace = true;
            continue;
        }

        if (inWhitespace) {
            normalized.push_back(' ');
            inWhitespace = false;
        }

        normalized.push_back((char)std::tolower(ch));
    }

    return normalized;
}

std::string TrimText(const std::string &text) {
    size_t start = 0;
    while (start < text.size() && std::isspace((unsigned char)text[start])) {
        start += 1;
    }

    size_t end = text.size();
    while (end > start && std::isspace((unsigned char)text[end - 1])) {
        end -= 1;
    }

    return text.substr(start, end - start);
}

bool NormalizeWireCryptOption(const std::string &text, std::string &out) {
    const std::string normalized = NormalizeTextToken(text);
    if (normalized.empty()) {
        out.clear();
        return true;
    }

    if (normalized == "disabled") {
        out = "Disabled";
        return true;
    }

    if (normalized == "enabled") {
        out = "Enabled";
        return true;
    }

    if (normalized == "required") {
        out = "Required";
        return true;
    }

    return false;
}

std::string BuildWireCryptConfig(const std::string &wireCrypt) {
    if (wireCrypt.empty()) return "";
    return "WireCrypt = " + wireCrypt;
}

struct TransactionOptionMapping {
    std::string normalizedIsolation;
    std::vector<unsigned char> isolationItems;
};

bool ParseTransactionIsolation(const std::string &text, TransactionOptionMapping &out) {
    const std::string normalized = NormalizeTextToken(text);
    if (normalized.empty()) return false;

    if (normalized == "consistency") {
        out.normalizedIsolation = "consistency";
        out.isolationItems = { isc_tpb_consistency };
        return true;
    }

    if (normalized == "concurrency" || normalized == "snapshot") {
        out.normalizedIsolation = "concurrency";
        out.isolationItems = { isc_tpb_concurrency };
        return true;
    }

    if (normalized == "read committed") {
        out.normalizedIsolation = "read committed";
        out.isolationItems = { isc_tpb_read_committed };
        return true;
    }

    if (normalized == "read committed record version") {
        out.normalizedIsolation = "read committed record version";
        out.isolationItems = { isc_tpb_read_committed, isc_tpb_rec_version };
        return true;
    }

    if (normalized == "read committed no record version") {
        out.normalizedIsolation = "read committed no record version";
        out.isolationItems = { isc_tpb_read_committed, isc_tpb_no_rec_version };
        return true;
    }

#ifdef isc_tpb_read_consistency
    if (normalized == "read committed read consistency") {
        out.normalizedIsolation = "read committed read consistency";
        out.isolationItems = { isc_tpb_read_committed, isc_tpb_read_consistency };
        return true;
    }
#endif

    return false;
}

void AppendInt32LE(std::vector<unsigned char> &bytes, long value) {
    bytes.push_back(4);
    bytes.push_back((unsigned char)(value & 0xFF));
    bytes.push_back((unsigned char)((value >> 8) & 0xFF));
    bytes.push_back((unsigned char)((value >> 16) & 0xFF));
    bytes.push_back((unsigned char)((value >> 24) & 0xFF));
}

void AppendUInt16LE(std::vector<char> &bytes, uint16_t value) {
    bytes.push_back((char)(value & 0xFF));
    bytes.push_back((char)((value >> 8) & 0xFF));
}

void AppendUInt32LE(std::vector<char> &bytes, uint32_t value) {
    bytes.push_back((char)(value & 0xFF));
    bytes.push_back((char)((value >> 8) & 0xFF));
    bytes.push_back((char)((value >> 16) & 0xFF));
    bytes.push_back((char)((value >> 24) & 0xFF));
}

void AppendAttachStringClumplet(std::vector<char> &bytes, unsigned char item, const std::string &value) {
    bytes.push_back((char)item);
    bytes.push_back((char)value.size());
    bytes.insert(bytes.end(), value.begin(), value.end());
}

void AppendServiceStringClumplet(std::vector<char> &bytes, unsigned char item, const std::string &value) {
    bytes.push_back((char)item);
    AppendUInt16LE(bytes, (uint16_t)value.size());
    bytes.insert(bytes.end(), value.begin(), value.end());
}

void AppendServiceIntClumplet(std::vector<char> &bytes, unsigned char item, uint32_t value) {
    bytes.push_back((char)item);
    AppendUInt16LE(bytes, 4);
    AppendUInt32LE(bytes, value);
}

std::string BuildServiceManagerName(const std::string &host, int port) {
    if (host.empty()) return "service_mgr";

    std::string result = host;
    if (port > 0 && port != 3050) {
        result += "/" + std::to_string(port);
    }
    result += ":service_mgr";
    return result;
}

bool CollectServiceQueryResult(const std::vector<char> &result, std::string &output, bool &keepPolling) {
    keepPolling = false;
    size_t pos = 0;

    while (pos < result.size()) {
        unsigned char item = (unsigned char)result[pos++];
        switch (item) {
            case isc_info_end:
                return true;

            case isc_info_truncated:
            case isc_info_svc_timeout:
            case isc_info_data_not_ready:
                keepPolling = true;
                break;

            case isc_info_svc_line:
            case isc_info_svc_to_eof: {
                if (pos + 2 > result.size()) return false;
                uint16_t len = (uint16_t)isc_vax_integer(&result[pos], 2);
                pos += 2;
                if (pos + len > result.size()) return false;
                if (len > 0) {
                    for (size_t i = pos; i < pos + len; ++i) {
                        unsigned char ch = (unsigned char)result[i];
                        if (ch >= 32 || ch == '\t' || ch == '\r' || ch == '\n') {
                            output.push_back((char)ch);
                        }
                    }
                    output.push_back('\n');
                    keepPolling = true;
                }
                pos += len;
                break;
            }

            default:
                return false;
        }
    }

    return true;
}

#if FB_HAS_MODERN_API
std::string FormatInterfaceStatus(IStatus *status) {
    auto &utilities = GetFirebirdUtilities();
    if (!status || !utilities.util) return "Firebird interface error";

    std::array<char, 1024> buffer = {};
    IUtil_formatStatus(utilities.util, buffer.data(), (unsigned)buffer.size(), status);
    return std::string(buffer.data());
}
#else
// Modern interface status formatting stub - Firebird 5.0.3 compatibility
std::string FormatInterfaceStatus(void *status) {
    return "Firebird interface error";
}

// Stub implementations for Firebird modern interface functions
// These are required for linking but modern interfaces are not supported in this build
extern "C" {
    // IUtil interface stubs
    void IUtil_decodeTimeStampTz(IUtil* util, void* status, const void* value, unsigned* year, unsigned* month, unsigned* day,
                                  unsigned* hour, unsigned* minute, unsigned* second, unsigned* fractions,
                                  unsigned timeZoneSize, char* timeZone) {}
    void IUtil_encodeTimeTz(IUtil* util, void* status, void* value, unsigned hour, unsigned minute, unsigned second,
                            unsigned fractions, const char* timeZone) {}
    void IUtil_encodeTimeStampTz(IUtil* util, void* status, void* value, unsigned year, unsigned month, unsigned day,
                                  unsigned hour, unsigned minute, unsigned second, unsigned fractions, const char* timeZone) {}

    // IXpbBuilder interface stubs
    IXpbBuilder* IUtil_getXpbBuilder(IUtil* util, void* status, unsigned kind, void* opts, unsigned optsSize) { return nullptr; }
    void IXpbBuilder_dispose(IXpbBuilder* builder) {}
    void IXpbBuilder_insertTag(IXpbBuilder* builder, void* status, unsigned tag) {}
    void IXpbBuilder_insertString(IXpbBuilder* builder, void* status, unsigned tag, const char* value) {}
    void IXpbBuilder_insertInt(IXpbBuilder* builder, void* status, unsigned tag, int value) {}
    void IXpbBuilder_insertBigInt(IXpbBuilder* builder, void* status, unsigned tag, long long value) {}
    void IXpbBuilder_insertBytes(IXpbBuilder* builder, void* status, unsigned tag, const void* data, unsigned length) {}
    unsigned IXpbBuilder_getBufferLength(IXpbBuilder* builder, void* status) { return 0; }
    const unsigned char* IXpbBuilder_getBuffer(IXpbBuilder* builder, void* status) { return nullptr; }
}
#endif

int QueryStatementTypeHandle(isc_stmt_handle stmtHandle) {
    if (!stmtHandle) return 0;

    char typeItem[] = { isc_info_sql_stmt_type };
    char infoBuffer[20];
    memset(infoBuffer, 0, sizeof(infoBuffer));

    ISC_STATUS_ARRAY status = {};
    if (isc_dsql_sql_info(status, &stmtHandle, sizeof(typeItem), typeItem,
                          sizeof(infoBuffer), infoBuffer)) {
        return 0;
    }

    if ((unsigned char)infoBuffer[0] == isc_info_sql_stmt_type) {
        short len = (short)isc_vax_integer(&infoBuffer[1], 2);
        return (int)isc_vax_integer(&infoBuffer[3], len);
    }
    return 0;
}

int64_t QueryAffectedRowCountHandle(isc_stmt_handle stmtHandle) {
    if (!stmtHandle) return 0;

    char items[] = { isc_info_sql_records };
    char infoBuffer[128];
    memset(infoBuffer, 0, sizeof(infoBuffer));

    ISC_STATUS_ARRAY status = {};
    if (isc_dsql_sql_info(status, &stmtHandle, sizeof(items), items,
                          sizeof(infoBuffer), infoBuffer)) {
        return 0;
    }

    if ((unsigned char)infoBuffer[0] != isc_info_sql_records) {
        return 0;
    }

    short blockLen = (short)isc_vax_integer(&infoBuffer[1], 2);
    int pos = 3;
    int end = pos + blockLen;
    int64_t affectedRows = 0;

    while (pos < end) {
        unsigned char item = (unsigned char)infoBuffer[pos++];
        if (item == isc_info_end) break;
        if (pos + 2 > end) break;

        short valueLen = (short)isc_vax_integer(&infoBuffer[pos], 2);
        pos += 2;
        if (valueLen < 0 || pos + valueLen > end) break;

        int64_t value = isc_vax_integer(&infoBuffer[pos], valueLen);
        pos += valueLen;

        switch (item) {
            case isc_info_req_insert_count:
            case isc_info_req_update_count:
            case isc_info_req_delete_count:
                affectedRows += value;
                break;
            default:
                break;
        }
    }

    return affectedRows;
}

} // namespace

// ============================================================================
// FBDatabase
// ============================================================================

FBDatabase::FBDatabase() {}

FBDatabase::~FBDatabase() {
    disconnect();
}

void FBDatabase::registerBlob(FBBlob *blob) {
    if (!blob) return;
    if (std::find(mTrackedBlobs.begin(), mTrackedBlobs.end(), blob) == mTrackedBlobs.end()) {
        mTrackedBlobs.push_back(blob);
    }
}

void FBDatabase::unregisterBlob(FBBlob *blob) {
    if (!blob) return;
    mTrackedBlobs.erase(
        std::remove(mTrackedBlobs.begin(), mTrackedBlobs.end(), blob),
        mTrackedBlobs.end()
    );
}

bool FBDatabase::connect(const std::string &database,
                         const std::string &user,
                         const std::string &password,
                         const std::string &charset,
                         const std::string &role,
                         int dialect,
                         const std::string &host,
                         int port,
                         const std::string &databasePath,
                         const std::string &wireCrypt,
                         const std::string &authClientPlugins)
{
    if (mConnected) disconnect();
    clearError();

    std::string normalizedWireCrypt;
    if (!NormalizeWireCryptOption(wireCrypt, normalizedWireCrypt)) {
        setError(-200150, "WireCrypt must be Disabled, Enabled, or Required");
        return false;
    }

    const std::string trimmedAuthClientPlugins = TrimText(authClientPlugins);
    if (trimmedAuthClientPlugins.size() > 255) {
        setError(-200151, "AuthClientPlugins exceeds Firebird DPB length limit");
        return false;
    }

    const std::string wireCryptConfig = BuildWireCryptConfig(normalizedWireCrypt);
    if (wireCryptConfig.size() > 255) {
        setError(-200152, "WireCrypt configuration exceeds Firebird DPB length limit");
        return false;
    }

    mDialect = dialect;
    mCharset = charset.empty() ? "UTF8" : charset;
    mHost = host;
    mPort = port > 0 ? port : 3050;
    mDatabasePath = databasePath.empty() ? database : databasePath;
    mUser = user;
    mPassword = password;
    mRole = role;
    mWireCrypt = normalizedWireCrypt;
    mAuthClientPlugins = trimmedAuthClientPlugins;
    mServiceOutput.clear();
    mAffectedRowCount = 0;

    // Build DPB
    std::vector<char> dpb;
    dpb.reserve(512);
    dpb.push_back((char)isc_dpb_version1);

    if (!user.empty()) {
        AppendAttachStringClumplet(dpb, isc_dpb_user_name, user);
    }

    if (!password.empty()) {
        AppendAttachStringClumplet(dpb, isc_dpb_password, password);
    }

    if (!charset.empty()) {
        AppendAttachStringClumplet(dpb, isc_dpb_lc_ctype, charset);
    }

    if (!role.empty()) {
        AppendAttachStringClumplet(dpb, isc_dpb_sql_role_name, role);
    }

    if (!trimmedAuthClientPlugins.empty()) {
        AppendAttachStringClumplet(dpb, isc_dpb_auth_plugin_list, trimmedAuthClientPlugins);
    }

    if (!wireCryptConfig.empty()) {
        AppendAttachStringClumplet(dpb, isc_dpb_config, wireCryptConfig);
    }

    // SQL dialect
    dpb.push_back((char)isc_dpb_sql_dialect);
    dpb.push_back(1);
    dpb.push_back((char)dialect);

    mDB = 0;
    if (isc_attach_database(mStatus, 0, database.c_str(), &mDB, (short)dpb.size(), dpb.data())) {
        captureError();
        return false;
    }

    mConnected = true;
    return true;
}

void FBDatabase::disconnect() {
    for (auto *blob : mTrackedBlobs) {
        if (blob) blob->invalidateFromDatabase();
    }
    mTrackedBlobs.clear();

    if (!mConnected) return;

    if (mTrans) {
        isc_rollback_transaction(mStatus, &mTrans);
        mTrans = 0;
    }

    isc_detach_database(mStatus, &mDB);
    mDB = 0;
    mConnected = false;
    mAffectedRowCount = 0;
}

bool FBDatabase::beginTransaction() {
    if (!mConnected) return false;
    if (mTrans) return true; // already active
    clearError();

    mTrans = 0;
    if (isc_start_transaction(mStatus, &mTrans, 1, &mDB, 0, nullptr)) {
        captureError();
        return false;
    }
    return true;
}

bool FBDatabase::beginTransactionWithOptions(const std::string &isolation, bool readOnly, long lockTimeout) {
    if (!mConnected) {
        setError(-200002, "Database is not connected");
        return false;
    }
    if (mTrans) {
        setError(-200001, "A transaction is already active");
        return false;
    }

    TransactionOptionMapping mapping;
    if (!ParseTransactionIsolation(isolation, mapping)) {
        setError(-200003, "Unsupported transaction isolation: " + isolation);
        return false;
    }

    if (lockTimeout < -1) {
        setError(-200004, "Lock timeout must be -1, 0, or a positive number of seconds");
        return false;
    }

    if (lockTimeout > (std::numeric_limits<int32_t>::max)()) {
        setError(-200005, "Lock timeout exceeds Firebird TPB range");
        return false;
    }

    clearError();

    std::vector<unsigned char> tpb;
    tpb.push_back(isc_tpb_version3);
    tpb.insert(tpb.end(), mapping.isolationItems.begin(), mapping.isolationItems.end());
    tpb.push_back(readOnly ? isc_tpb_read : isc_tpb_write);

    if (lockTimeout == -1) {
        tpb.push_back(isc_tpb_wait);
    } else if (lockTimeout == 0) {
        tpb.push_back(isc_tpb_nowait);
    } else {
        tpb.push_back(isc_tpb_wait);
        tpb.push_back(isc_tpb_lock_timeout);
        AppendInt32LE(tpb, lockTimeout);
    }

    mTrans = 0;
    if (isc_start_transaction(mStatus, &mTrans, 1, &mDB, (unsigned short)tpb.size(),
                              reinterpret_cast<char *>(tpb.data()))) {
        captureError();
        return false;
    }
    return true;
}

bool FBDatabase::commit() {
    if (!mTrans) return true;
    clearError();

    if (isc_commit_transaction(mStatus, &mTrans)) {
        captureError();
        return false;
    }
    mTrans = 0;
    return true;
}

bool FBDatabase::rollback() {
    if (!mTrans) return true;
    clearError();

    if (isc_rollback_transaction(mStatus, &mTrans)) {
        captureError();
        return false;
    }
    mTrans = 0;
    return true;
}

bool FBDatabase::ensureTransaction() {
    if (mTrans) return true;
    return beginTransaction();
}

bool FBDatabase::executeImmediate(const std::string &sql) {
    if (!mConnected) return false;
    clearError();
    mAffectedRowCount = 0;

    if (!ensureTransaction()) return false;

    FBStatement stmt;
    if (!stmt.prepare(*this, sql)) return false;
    return stmt.execute(*this, true);
}

bool FBDatabase::readBlob(ISC_QUAD blobId, std::string &out, bool isText) {
    if (!mConnected || !mTrans) return false;
    clearError();

    // For text blobs, build a BPB requesting UTF-8 transliteration
    // so Firebird converts from the blob's native charset to UTF-8
    ISC_UCHAR bpb[] = {
        isc_bpb_version1,
        isc_bpb_target_type, 1, 1,     // target type = text
        isc_bpb_target_interp, 1, 4    // target charset = UTF8 (charset id 4)
    };
    ISC_USHORT bpb_len = isText ? (ISC_USHORT)sizeof(bpb) : 0;
    const ISC_UCHAR *bpb_ptr = isText ? bpb : nullptr;

    isc_blob_handle blob = 0;
    if (isc_open_blob2(mStatus, &mDB, &mTrans, &blob, &blobId, bpb_len, bpb_ptr)) {
        captureError();
        return false;
    }

    out.clear();
    char buf[4096];
    unsigned short actual = 0;
    ISC_STATUS stat;

    for (;;) {
        stat = isc_get_segment(mStatus, &blob, &actual, sizeof(buf), buf);
        if (stat == 0 || mStatus[1] == isc_segment) {
            out.append(buf, actual);
        } else {
            break;
        }
    }

    isc_close_blob(mStatus, &blob);
    return true;
}

bool FBDatabase::writeBlob(const void *data, size_t len, ISC_QUAD &outId) {
    if (!mConnected || !mTrans) return false;
    clearError();

    isc_blob_handle blob = 0;
    if (isc_create_blob2(mStatus, &mDB, &mTrans, &blob, &outId, 0, nullptr)) {
        captureError();
        return false;
    }

    const char *ptr = (const char *)data;
    size_t remaining = len;
    while (remaining > 0) {
        unsigned short chunk = (remaining > 32767) ? 32767 : (unsigned short)remaining;
        if (isc_put_segment(mStatus, &blob, chunk, ptr)) {
            captureError();
            isc_close_blob(mStatus, &blob);
            return false;
        }
        ptr += chunk;
        remaining -= chunk;
    }

    isc_close_blob(mStatus, &blob);
    return true;
}

FBBlob::FBBlob() {}

FBBlob::~FBBlob() {
    close();
    releaseAssociation();
}

void FBBlob::releaseAssociation() {
    if (mDB) {
        mDB->unregisterBlob(this);
        mDB = nullptr;
    }
}

void FBBlob::invalidateFromDatabase() {
    mBlob = 0;
    mOpen = false;
    mWritable = false;
    mPosition = 0;
    mDB = nullptr;
}

bool FBBlob::create(FBDatabase &db) {
    close();
    releaseAssociation();

    if (!db.isConnected()) {
        db.setError(-200180, "Database is not connected");
        return false;
    }
    if (!db.ensureTransaction()) return false;

    db.clearError();
    isc_blob_handle blob = 0;
    ISC_QUAD blobId = {};
    if (isc_create_blob2(db.mStatus, &db.mDB, &db.mTrans, &blob, &blobId, 0, nullptr)) {
        db.captureError();
        return false;
    }

    mDB = &db;
    mDB->registerBlob(this);
    mBlob = blob;
    mBlobId = blobId;
    mOpen = true;
    mWritable = true;
    mLengthKnown = true;
    mPosition = 0;
    mLength = 0;
    return true;
}

bool FBBlob::open(FBDatabase &db, ISC_QUAD blobId) {
    close();
    releaseAssociation();

    if (!db.isConnected()) {
        db.setError(-200180, "Database is not connected");
        return false;
    }
    if (!db.ensureTransaction()) return false;

    db.clearError();
    isc_blob_handle blob = 0;
    if (isc_open_blob2(db.mStatus, &db.mDB, &db.mTrans, &blob, &blobId, 0, nullptr)) {
        db.captureError();
        return false;
    }

    mDB = &db;
    mDB->registerBlob(this);
    mBlob = blob;
    mBlobId = blobId;
    mOpen = true;
    mWritable = false;
    mLengthKnown = false;
    mPosition = 0;
    mLength = 0;
    return refreshInfo();
}

bool FBBlob::close() {
    if (!mOpen) return true;

    if (!mDB) {
        mBlob = 0;
        mOpen = false;
        mWritable = false;
        mPosition = 0;
        return true;
    }

    mDB->clearError();
    if (isc_close_blob(mDB->mStatus, &mBlob)) {
        mDB->captureError();
        return false;
    }

    mBlob = 0;
    mOpen = false;
    mWritable = false;
    mPosition = 0;
    return true;
}

bool FBBlob::refreshInfo() {
    if (!mDB || !mOpen) return false;

    mDB->clearError();
    const char items[] = {
        (char)isc_info_blob_total_length
    };
    char info[32] = {};
    if (isc_blob_info(mDB->mStatus, &mBlob, (short)sizeof(items), items, (short)sizeof(info), info)) {
        mDB->captureError();
        return false;
    }

    size_t pos = 0;
    while (pos < sizeof(info)) {
        unsigned char item = (unsigned char)info[pos];
        if (item == isc_info_end || item == isc_info_truncated) break;
        if (pos + 3 > sizeof(info)) break;

        short len = (short)isc_vax_integer(&info[pos + 1], 2);
        if (pos + 3 + (size_t)len > sizeof(info)) break;

        if (item == isc_info_blob_total_length) {
            mLength = (int64_t)isc_vax_integer(&info[pos + 3], len);
            mLengthKnown = true;
            return true;
        }

        pos += 3 + (size_t)len;
    }

    mDB->setError(-200181, "Unexpected blob info response");
    return false;
}

bool FBBlob::read(size_t count, std::string &out) {
    out.clear();
    if (!mDB || !mOpen) {
        if (mDB) mDB->setError(-200182, "Blob is not open");
        return false;
    }
    if (mWritable) {
        mDB->setError(-200183, "Blob was opened for writing");
        return false;
    }
    if (count == 0) return true;

    mDB->clearError();
    char buffer[4096];

    while (out.size() < count) {
        const size_t remaining = count - out.size();
        const unsigned short request = (unsigned short)std::min<size_t>(remaining, sizeof(buffer));
        unsigned short actual = 0;
        ISC_STATUS stat = isc_get_segment(mDB->mStatus, &mBlob, &actual, request, buffer);
        if (stat == 0 || mDB->mStatus[1] == isc_segment) {
            out.append(buffer, actual);
            mPosition += actual;
            continue;
        }
        if (mDB->mStatus[1] == isc_segstr_eof) {
            break;
        }

        mDB->captureError();
        return false;
    }

    return true;
}

bool FBBlob::write(const void *data, size_t len) {
    if (!mDB || !mOpen) {
        if (mDB) mDB->setError(-200182, "Blob is not open");
        return false;
    }
    if (!mWritable) {
        mDB->setError(-200184, "Blob was not created for writing");
        return false;
    }

    mDB->clearError();
    const char *ptr = static_cast<const char *>(data);
    size_t remaining = len;
    while (remaining > 0) {
        const unsigned short chunk = (unsigned short)std::min<size_t>(remaining, 32767);
        if (isc_put_segment(mDB->mStatus, &mBlob, chunk, ptr)) {
            mDB->captureError();
            return false;
        }
        ptr += chunk;
        remaining -= chunk;
        mPosition += chunk;
    }

    if (mPosition > mLength) mLength = mPosition;
    mLengthKnown = true;
    return true;
}

int64_t FBBlob::seek(int64_t offset, int mode) {
    if (!mDB || !mOpen) {
        if (mDB) mDB->setError(-200182, "Blob is not open");
        return -1;
    }
    if (mWritable) {
        mDB->setError(-200186, "Seek is only supported on opened read blobs");
        return -1;
    }
    if (mode < 0 || mode > 2) {
        mDB->setError(-200185, "Blob seek mode must be 0, 1, or 2");
        return -1;
    }
    if (!mLengthKnown && !refreshInfo()) {
        return -1;
    }

    int64_t target = 0;
    switch (mode) {
        case 0: {
            target = offset;
            break;
        }
        case 1: {
            target = mPosition + offset;
            break;
        }
        case 2: {
            target = mLength + offset;
            break;
        }
    }

    if (target < 0) {
        mDB->setError(-200187, "Blob seek target cannot be negative");
        return -1;
    }
    if (target > mLength) {
        target = mLength;
    }
    if (target == mPosition) {
        return mPosition;
    }

    mDB->clearError();
    if (isc_close_blob(mDB->mStatus, &mBlob)) {
        mDB->captureError();
        return -1;
    }

    mBlob = 0;
    if (isc_open_blob2(mDB->mStatus, &mDB->mDB, &mDB->mTrans, &mBlob, &mBlobId, 0, nullptr)) {
        mDB->captureError();
        mOpen = false;
        return -1;
    }

    mOpen = true;
    mWritable = false;
    mPosition = 0;

    std::string discard;
    while (mPosition < target) {
        const size_t remaining = (size_t)(target - mPosition);
        const size_t chunk = std::min<size_t>(remaining, 4096);
        if (!read(chunk, discard)) {
            return -1;
        }
        if (discard.empty()) {
            break;
        }
    }

    return mPosition;
}

int64_t FBBlob::length() {
    if (!mLengthKnown && mOpen && mDB) {
        refreshInfo();
    }
    return mLength;
}

bool FBDatabase::databaseInfo(const unsigned char *items, short itemLen, std::vector<char> &out) {
    if (!mConnected) return false;
    clearError();

    out.assign(512, 0);
    if (isc_database_info(mStatus, &mDB, itemLen, reinterpret_cast<const char *>(items),
                          (short)out.size(), out.data())) {
        captureError();
        return false;
    }
    return true;
}

bool FBDatabase::transactionInfo(const unsigned char *items, short itemLen, std::vector<char> &out) {
    if (!mConnected || !mTrans) return false;
    clearError();

    out.assign(512, 0);
    if (isc_transaction_info(mStatus, &mTrans, itemLen, reinterpret_cast<const char *>(items),
                             (short)out.size(), out.data())) {
        captureError();
        return false;
    }
    return true;
}

bool FBDatabase::runServiceRequest(const std::vector<char> &request, std::string &output) {
    if (mUser.empty()) {
        setError(-200102, "Service manager requires a user name");
        return false;
    }

    clearError();
    output.clear();

    std::string normalizedWireCrypt;
    if (!NormalizeWireCryptOption(mWireCrypt, normalizedWireCrypt)) {
        setError(-200150, "WireCrypt must be Disabled, Enabled, or Required");
        return false;
    }

    const std::string trimmedAuthClientPlugins = TrimText(mAuthClientPlugins);

    isc_svc_handle service = 0;
    std::vector<char> attachSpb;
    attachSpb.push_back((char)isc_spb_version1);
    AppendAttachStringClumplet(attachSpb, isc_spb_user_name, mUser);
    if (!mPassword.empty()) {
        AppendAttachStringClumplet(attachSpb, isc_spb_password, mPassword);
    }
    if (!trimmedAuthClientPlugins.empty()) {
        AppendServiceStringClumplet(attachSpb, isc_spb_auth_plugin_list, trimmedAuthClientPlugins);
    }

    const std::string wireCryptConfig = BuildWireCryptConfig(normalizedWireCrypt);
    if (!wireCryptConfig.empty()) {
        AppendServiceStringClumplet(attachSpb, isc_spb_config, wireCryptConfig);
    }

    const std::string serviceName = BuildServiceManagerName(mHost, mPort);
    if (isc_service_attach(mStatus, 0, serviceName.c_str(), &service,
                           (unsigned short)attachSpb.size(), attachSpb.data())) {
        captureError();
        return false;
    }

    bool ok = true;
    if (isc_service_start(mStatus, &service, nullptr, (unsigned short)request.size(),
                          const_cast<char *>(request.data()))) {
        captureError();
        ok = false;
    } else {
        const unsigned char receiveItems[] = { isc_info_svc_line };
        std::vector<char> result(8192, 0);

        for (;;) {
            if (isc_service_query(mStatus, &service, nullptr, 0, nullptr,
                                  (unsigned short)sizeof(receiveItems),
                                  reinterpret_cast<const char *>(receiveItems),
                                  (unsigned short)result.size(), result.data())) {
                captureError();
                ok = false;
                break;
            }

            bool keepPolling = false;
            if (!CollectServiceQueryResult(result, output, keepPolling)) {
                setError(-200103, "Unexpected service manager response");
                ok = false;
                break;
            }
            if (!keepPolling) break;
        }
    }

    ISC_STATUS_ARRAY detachStatus = {};
    if (service) {
        isc_service_detach(detachStatus, &service);
    }

    return ok;
}

const char *FBDatabase::findInfoItem(const std::vector<char> &info, unsigned char item, short &len) const {
    len = 0;
    size_t pos = 0;

    while (pos < info.size()) {
        unsigned char code = (unsigned char)info[pos];
        if (code == isc_info_end || code == isc_info_truncated) {
            return nullptr;
        }
        if (code == isc_info_error || pos + 3 > info.size()) {
            return nullptr;
        }

        short itemLen = (short)isc_vax_integer(&info[pos + 1], 2);
        if (pos + 3 + itemLen > info.size()) {
            return nullptr;
        }

        if (code == item) {
            len = itemLen;
            return &info[pos + 3];
        }

        pos += 3 + itemLen;
    }

    return nullptr;
}

bool FBDatabase::serverVersion(std::string &out) {
    const unsigned char items[] = { isc_info_firebird_version };
    std::vector<char> info;
    short len = 0;

    if (!databaseInfo(items, sizeof(items), info)) return false;

    const char *value = findInfoItem(info, isc_info_firebird_version, len);
    if (!value || len <= 0) return false;

    out.assign(value, len);
    size_t nul = out.find('\0');
    if (nul != std::string::npos) out.resize(nul);
    return true;
}

bool FBDatabase::pageSize(long &out) {
    const unsigned char items[] = { isc_info_page_size };
    std::vector<char> info;
    short len = 0;

    if (!databaseInfo(items, sizeof(items), info)) return false;

    const char *value = findInfoItem(info, isc_info_page_size, len);
    if (!value || len <= 0) return false;

    out = isc_vax_integer(value, len);
    return true;
}

bool FBDatabase::databaseSQLDialect(long &out) {
    const unsigned char items[] = { isc_info_db_sql_dialect };
    std::vector<char> info;
    short len = 0;

    if (!databaseInfo(items, sizeof(items), info)) return false;

    const char *value = findInfoItem(info, isc_info_db_sql_dialect, len);
    if (!value || len <= 0) return false;

    out = isc_vax_integer(value, len);
    return true;
}

bool FBDatabase::odsVersion(std::string &out) {
    const unsigned char items[] = { isc_info_ods_version, isc_info_ods_minor_version };
    std::vector<char> info;
    short majorLen = 0;
    short minorLen = 0;

    if (!databaseInfo(items, sizeof(items), info)) return false;

    const char *majorValue = findInfoItem(info, isc_info_ods_version, majorLen);
    const char *minorValue = findInfoItem(info, isc_info_ods_minor_version, minorLen);
    if (!majorValue || !minorValue || majorLen <= 0 || minorLen <= 0) return false;

    long major = isc_vax_integer(majorValue, majorLen);
    long minor = isc_vax_integer(minorValue, minorLen);

    out = std::to_string(major) + "." + std::to_string(minor);
    return true;
}

bool FBDatabase::isReadOnly(bool &out) {
    const unsigned char items[] = { isc_info_db_read_only };
    std::vector<char> info;
    short len = 0;

    if (!databaseInfo(items, sizeof(items), info)) return false;

    const char *value = findInfoItem(info, isc_info_db_read_only, len);
    if (!value || len <= 0) return false;

    out = isc_vax_integer(value, len) != 0;
    return true;
}

bool FBDatabase::transactionID(int64_t &out) {
    const unsigned char items[] = { isc_info_tra_id };
    std::vector<char> info;
    short len = 0;

    if (!transactionInfo(items, sizeof(items), info)) return false;

    const char *value = findInfoItem(info, isc_info_tra_id, len);
    if (!value || len <= 0) return false;

    out = (int64_t)isc_portable_integer(reinterpret_cast<const ISC_UCHAR *>(value), len);
    return true;
}

bool FBDatabase::transactionIsolation(std::string &out) {
    const unsigned char items[] = { isc_info_tra_isolation };
    std::vector<char> info;
    short len = 0;

    if (!transactionInfo(items, sizeof(items), info)) return false;

    const char *value = findInfoItem(info, isc_info_tra_isolation, len);
    if (!value || len <= 0) return false;

    unsigned char isolation = (unsigned char)value[0];
    switch (isolation) {
        case isc_info_tra_consistency:
            out = "consistency";
            return true;

        case isc_info_tra_concurrency:
            out = "concurrency";
            return true;

        case isc_info_tra_read_committed:
            if (len > 1) {
                unsigned char option = (unsigned char)value[1];
                switch (option) {
                    case isc_info_tra_no_rec_version:
                        out = "read committed no record version";
                        return true;

                    case isc_info_tra_rec_version:
                        out = "read committed record version";
                        return true;

                    case isc_info_tra_read_consistency:
                        out = "read committed read consistency";
                        return true;
                }
            }

            out = "read committed";
            return true;
    }

    out.clear();
    return false;
}

bool FBDatabase::transactionAccessMode(std::string &out) {
    const unsigned char items[] = { isc_info_tra_access };
    std::vector<char> info;
    short len = 0;

    if (!transactionInfo(items, sizeof(items), info)) return false;

    const char *value = findInfoItem(info, isc_info_tra_access, len);
    if (!value || len <= 0) return false;

    unsigned char access = (unsigned char)value[0];
    switch (access) {
        case isc_info_tra_readonly:
            out = "read only";
            return true;

        case isc_info_tra_readwrite:
            out = "read write";
            return true;
    }

    out.clear();
    return false;
}

bool FBDatabase::transactionLockTimeout(long &out) {
    const unsigned char items[] = { isc_info_tra_lock_timeout };
    std::vector<char> info;
    short len = 0;

    if (!transactionInfo(items, sizeof(items), info)) return false;

    const char *value = findInfoItem(info, isc_info_tra_lock_timeout, len);
    if (!value || len <= 0) return false;

    out = (long)isc_portable_integer(reinterpret_cast<const ISC_UCHAR *>(value), len);
    return true;
}

bool FBDatabase::backupDatabase(const std::string &backupFile) {
    if (backupFile.empty()) {
        setError(-200104, "Backup file path is required");
        return false;
    }
    if (mDatabasePath.empty()) {
        setError(-200105, "Database path is unavailable for backup");
        return false;
    }

    auto &utilities = GetFirebirdUtilities();
    FirebirdStatusScope status;
    if (!status.get() || !utilities.util) {
        setError(-200108, "Firebird utility interface is unavailable");
        return false;
    }

    IXpbBuilder *builder = IUtil_getXpbBuilder(utilities.util, status.get(), IXpbBuilder_SPB_START, nullptr, 0);
    if (!builder || !status.ok()) {
        setError(-200109, FormatInterfaceStatus(status.get()));
        return false;
    }

    IXpbBuilder_insertTag(builder, status.get(), isc_action_svc_backup);
    IXpbBuilder_insertString(builder, status.get(), isc_spb_dbname, mDatabasePath.c_str());
    IXpbBuilder_insertString(builder, status.get(), isc_spb_bkp_file, backupFile.c_str());
    if (!mRole.empty()) {
        IXpbBuilder_insertString(builder, status.get(), isc_spb_sql_role_name, mRole.c_str());
    }
    IXpbBuilder_insertTag(builder, status.get(), isc_spb_verbose);

    std::vector<char> request;
    if (status.ok()) {
        unsigned length = IXpbBuilder_getBufferLength(builder, status.get());
        const unsigned char *buffer = IXpbBuilder_getBuffer(builder, status.get());
        if (status.ok() && buffer && length > 0) {
            request.assign(reinterpret_cast<const char *>(buffer), reinterpret_cast<const char *>(buffer) + length);
        }
    }
    IXpbBuilder_dispose(builder);

    if (!status.ok() || request.empty()) {
        setError(-200110, FormatInterfaceStatus(status.get()));
        return false;
    }

    std::string output;
    if (!runServiceRequest(request, output)) return false;

    mServiceOutput = output;
    return true;
}

bool FBDatabase::restoreDatabase(const std::string &backupFile, const std::string &targetDatabase, bool replaceExisting) {
    if (backupFile.empty()) {
        setError(-200106, "Backup file path is required");
        return false;
    }
    if (targetDatabase.empty()) {
        setError(-200107, "Target database path is required");
        return false;
    }

    auto &utilities = GetFirebirdUtilities();
    FirebirdStatusScope status;
    if (!status.get() || !utilities.util) {
        setError(-200111, "Firebird utility interface is unavailable");
        return false;
    }

    IXpbBuilder *builder = IUtil_getXpbBuilder(utilities.util, status.get(), IXpbBuilder_SPB_START, nullptr, 0);
    if (!builder || !status.ok()) {
        setError(-200112, FormatInterfaceStatus(status.get()));
        return false;
    }

    IXpbBuilder_insertTag(builder, status.get(), isc_action_svc_restore);
    IXpbBuilder_insertString(builder, status.get(), isc_spb_bkp_file, backupFile.c_str());
    IXpbBuilder_insertString(builder, status.get(), isc_spb_dbname, targetDatabase.c_str());
    if (!mRole.empty()) {
        IXpbBuilder_insertString(builder, status.get(), isc_spb_sql_role_name, mRole.c_str());
    }
    IXpbBuilder_insertInt(builder, status.get(), isc_spb_options,
                          replaceExisting ? isc_spb_res_replace : isc_spb_res_create);
    IXpbBuilder_insertTag(builder, status.get(), isc_spb_verbose);

    std::vector<char> request;
    if (status.ok()) {
        unsigned length = IXpbBuilder_getBufferLength(builder, status.get());
        const unsigned char *buffer = IXpbBuilder_getBuffer(builder, status.get());
        if (status.ok() && buffer && length > 0) {
            request.assign(reinterpret_cast<const char *>(buffer), reinterpret_cast<const char *>(buffer) + length);
        }
    }
    IXpbBuilder_dispose(builder);

    if (!status.ok() || request.empty()) {
        setError(-200113, FormatInterfaceStatus(status.get()));
        return false;
    }

    std::string output;
    if (!runServiceRequest(request, output)) return false;

    mServiceOutput = output;
    return true;
}

bool FBDatabase::databaseStatistics() {
    if (mDatabasePath.empty()) {
        setError(-200114, "Database path is unavailable for statistics");
        return false;
    }

    auto &utilities = GetFirebirdUtilities();
    FirebirdStatusScope status;
    if (!status.get() || !utilities.util) {
        setError(-200115, "Firebird utility interface is unavailable");
        return false;
    }

    IXpbBuilder *builder = IUtil_getXpbBuilder(utilities.util, status.get(), IXpbBuilder_SPB_START, nullptr, 0);
    if (!builder || !status.ok()) {
        setError(-200116, FormatInterfaceStatus(status.get()));
        return false;
    }

    IXpbBuilder_insertTag(builder, status.get(), isc_action_svc_db_stats);
    IXpbBuilder_insertString(builder, status.get(), isc_spb_dbname, mDatabasePath.c_str());
    if (!mRole.empty()) {
        IXpbBuilder_insertString(builder, status.get(), isc_spb_sql_role_name, mRole.c_str());
    }
    IXpbBuilder_insertInt(builder, status.get(), isc_spb_options,
                          isc_spb_sts_data_pages | isc_spb_sts_idx_pages);

    std::vector<char> request;
    if (status.ok()) {
        unsigned length = IXpbBuilder_getBufferLength(builder, status.get());
        const unsigned char *buffer = IXpbBuilder_getBuffer(builder, status.get());
        if (status.ok() && buffer && length > 0) {
            request.assign(reinterpret_cast<const char *>(buffer), reinterpret_cast<const char *>(buffer) + length);
        }
    }
    IXpbBuilder_dispose(builder);

    if (!status.ok() || request.empty()) {
        setError(-200117, FormatInterfaceStatus(status.get()));
        return false;
    }

    std::string output;
    if (!runServiceRequest(request, output)) return false;

    mServiceOutput = output;
    return true;
}

bool FBDatabase::validateDatabase() {
    if (mDatabasePath.empty()) {
        setError(-200118, "Database path is unavailable for validation");
        return false;
    }

    auto &utilities = GetFirebirdUtilities();
    FirebirdStatusScope status;
    if (!status.get() || !utilities.util) {
        setError(-200119, "Firebird utility interface is unavailable");
        return false;
    }

    IXpbBuilder *builder = IUtil_getXpbBuilder(utilities.util, status.get(), IXpbBuilder_SPB_START, nullptr, 0);
    if (!builder || !status.ok()) {
        setError(-200120, FormatInterfaceStatus(status.get()));
        return false;
    }

    IXpbBuilder_insertTag(builder, status.get(), isc_action_svc_validate);
    IXpbBuilder_insertString(builder, status.get(), isc_spb_dbname, mDatabasePath.c_str());
    if (!mRole.empty()) {
        IXpbBuilder_insertString(builder, status.get(), isc_spb_sql_role_name, mRole.c_str());
    }

    std::vector<char> request;
    if (status.ok()) {
        unsigned length = IXpbBuilder_getBufferLength(builder, status.get());
        const unsigned char *buffer = IXpbBuilder_getBuffer(builder, status.get());
        if (status.ok() && buffer && length > 0) {
            request.assign(reinterpret_cast<const char *>(buffer), reinterpret_cast<const char *>(buffer) + length);
        }
    }
    IXpbBuilder_dispose(builder);

    if (!status.ok() || request.empty()) {
        setError(-200121, FormatInterfaceStatus(status.get()));
        return false;
    }

    std::string output;
    if (!runServiceRequest(request, output)) return false;

    mServiceOutput = output;
    return true;
}

bool FBDatabase::sweepDatabase() {
    if (mDatabasePath.empty()) {
        setError(-200148, "Database path is unavailable for sweep");
        return false;
    }

    auto &utilities = GetFirebirdUtilities();
    FirebirdStatusScope status;
    if (!status.get() || !utilities.util) {
        setError(-200149, "Firebird utility interface is unavailable");
        return false;
    }

    IXpbBuilder *builder = IUtil_getXpbBuilder(utilities.util, status.get(), IXpbBuilder_SPB_START, nullptr, 0);
    if (!builder || !status.ok()) {
        setError(-200150, FormatInterfaceStatus(status.get()));
        return false;
    }

    IXpbBuilder_insertTag(builder, status.get(), isc_action_svc_repair);
    IXpbBuilder_insertString(builder, status.get(), isc_spb_dbname, mDatabasePath.c_str());
    if (!mRole.empty()) {
        IXpbBuilder_insertString(builder, status.get(), isc_spb_sql_role_name, mRole.c_str());
    }
    IXpbBuilder_insertInt(builder, status.get(), isc_spb_options, isc_spb_rpr_sweep_db);

    std::vector<char> request;
    if (status.ok()) {
        unsigned length = IXpbBuilder_getBufferLength(builder, status.get());
        const unsigned char *buffer = IXpbBuilder_getBuffer(builder, status.get());
        if (status.ok() && buffer && length > 0) {
            request.assign(reinterpret_cast<const char *>(buffer), reinterpret_cast<const char *>(buffer) + length);
        }
    }
    IXpbBuilder_dispose(builder);

    if (!status.ok() || request.empty()) {
        setError(-200151, FormatInterfaceStatus(status.get()));
        return false;
    }

    std::string output;
    if (!runServiceRequest(request, output)) return false;

    mServiceOutput = output;
    return true;
}

bool FBDatabase::listLimboTransactions() {
    if (mDatabasePath.empty()) {
        setError(-200152, "Database path is unavailable for limbo transaction listing");
        return false;
    }

    auto &utilities = GetFirebirdUtilities();
    FirebirdStatusScope status;
    if (!status.get() || !utilities.util) {
        setError(-200153, "Firebird utility interface is unavailable");
        return false;
    }

    IXpbBuilder *builder = IUtil_getXpbBuilder(utilities.util, status.get(), IXpbBuilder_SPB_START, nullptr, 0);
    if (!builder || !status.ok()) {
        setError(-200154, FormatInterfaceStatus(status.get()));
        return false;
    }

    IXpbBuilder_insertTag(builder, status.get(), isc_action_svc_repair);
    IXpbBuilder_insertString(builder, status.get(), isc_spb_dbname, mDatabasePath.c_str());
    if (!mRole.empty()) {
        IXpbBuilder_insertString(builder, status.get(), isc_spb_sql_role_name, mRole.c_str());
    }
    IXpbBuilder_insertInt(builder, status.get(), isc_spb_options, isc_spb_rpr_list_limbo_trans);

    std::vector<char> request;
    if (status.ok()) {
        unsigned length = IXpbBuilder_getBufferLength(builder, status.get());
        const unsigned char *buffer = IXpbBuilder_getBuffer(builder, status.get());
        if (status.ok() && buffer && length > 0) {
            request.assign(reinterpret_cast<const char *>(buffer), reinterpret_cast<const char *>(buffer) + length);
        }
    }
    IXpbBuilder_dispose(builder);

    if (!status.ok() || request.empty()) {
        setError(-200155, FormatInterfaceStatus(status.get()));
        return false;
    }

    std::string output;
    if (!runServiceRequest(request, output)) return false;

    mServiceOutput = output;
    return true;
}

bool FBDatabase::commitLimboTransaction(int64_t transactionId) {
    if (mDatabasePath.empty()) {
        setError(-200161, "Database path is unavailable for limbo transaction commit");
        return false;
    }
    if (transactionId <= 0) {
        setError(-200162, "Limbo transaction id must be greater than zero");
        return false;
    }

    auto &utilities = GetFirebirdUtilities();
    FirebirdStatusScope status;
    if (!status.get() || !utilities.util) {
        setError(-200163, "Firebird utility interface is unavailable");
        return false;
    }

    IXpbBuilder *builder = IUtil_getXpbBuilder(utilities.util, status.get(), IXpbBuilder_SPB_START, nullptr, 0);
    if (!builder || !status.ok()) {
        setError(-200164, FormatInterfaceStatus(status.get()));
        return false;
    }

    IXpbBuilder_insertTag(builder, status.get(), isc_action_svc_repair);
    IXpbBuilder_insertString(builder, status.get(), isc_spb_dbname, mDatabasePath.c_str());
    if (!mRole.empty()) {
        IXpbBuilder_insertString(builder, status.get(), isc_spb_sql_role_name, mRole.c_str());
    }
    IXpbBuilder_insertBigInt(builder, status.get(), isc_spb_rpr_commit_trans_64, transactionId);

    std::vector<char> request;
    if (status.ok()) {
        unsigned length = IXpbBuilder_getBufferLength(builder, status.get());
        const unsigned char *buffer = IXpbBuilder_getBuffer(builder, status.get());
        if (status.ok() && buffer && length > 0) {
            request.assign(reinterpret_cast<const char *>(buffer), reinterpret_cast<const char *>(buffer) + length);
        }
    }
    IXpbBuilder_dispose(builder);

    if (!status.ok() || request.empty()) {
        setError(-200165, FormatInterfaceStatus(status.get()));
        return false;
    }

    std::string output;
    if (!runServiceRequest(request, output)) return false;

    mServiceOutput = output;
    return true;
}

bool FBDatabase::rollbackLimboTransaction(int64_t transactionId) {
    if (mDatabasePath.empty()) {
        setError(-200166, "Database path is unavailable for limbo transaction rollback");
        return false;
    }
    if (transactionId <= 0) {
        setError(-200167, "Limbo transaction id must be greater than zero");
        return false;
    }

    auto &utilities = GetFirebirdUtilities();
    FirebirdStatusScope status;
    if (!status.get() || !utilities.util) {
        setError(-200168, "Firebird utility interface is unavailable");
        return false;
    }

    IXpbBuilder *builder = IUtil_getXpbBuilder(utilities.util, status.get(), IXpbBuilder_SPB_START, nullptr, 0);
    if (!builder || !status.ok()) {
        setError(-200169, FormatInterfaceStatus(status.get()));
        return false;
    }

    IXpbBuilder_insertTag(builder, status.get(), isc_action_svc_repair);
    IXpbBuilder_insertString(builder, status.get(), isc_spb_dbname, mDatabasePath.c_str());
    if (!mRole.empty()) {
        IXpbBuilder_insertString(builder, status.get(), isc_spb_sql_role_name, mRole.c_str());
    }
    IXpbBuilder_insertBigInt(builder, status.get(), isc_spb_rpr_rollback_trans_64, transactionId);

    std::vector<char> request;
    if (status.ok()) {
        unsigned length = IXpbBuilder_getBufferLength(builder, status.get());
        const unsigned char *buffer = IXpbBuilder_getBuffer(builder, status.get());
        if (status.ok() && buffer && length > 0) {
            request.assign(reinterpret_cast<const char *>(buffer), reinterpret_cast<const char *>(buffer) + length);
        }
    }
    IXpbBuilder_dispose(builder);

    if (!status.ok() || request.empty()) {
        setError(-200170, FormatInterfaceStatus(status.get()));
        return false;
    }

    std::string output;
    if (!runServiceRequest(request, output)) return false;

    mServiceOutput = output;
    return true;
}

bool FBDatabase::setSweepInterval(long interval) {
    if (mDatabasePath.empty()) {
        setError(-200156, "Database path is unavailable for sweep interval update");
        return false;
    }
    if (interval < 0) {
        setError(-200157, "Sweep interval must be zero or greater");
        return false;
    }

    auto &utilities = GetFirebirdUtilities();
    FirebirdStatusScope status;
    if (!status.get() || !utilities.util) {
        setError(-200158, "Firebird utility interface is unavailable");
        return false;
    }

    IXpbBuilder *builder = IUtil_getXpbBuilder(utilities.util, status.get(), IXpbBuilder_SPB_START, nullptr, 0);
    if (!builder || !status.ok()) {
        setError(-200159, FormatInterfaceStatus(status.get()));
        return false;
    }

    IXpbBuilder_insertTag(builder, status.get(), isc_action_svc_properties);
    IXpbBuilder_insertString(builder, status.get(), isc_spb_dbname, mDatabasePath.c_str());
    if (!mRole.empty()) {
        IXpbBuilder_insertString(builder, status.get(), isc_spb_sql_role_name, mRole.c_str());
    }
    IXpbBuilder_insertInt(builder, status.get(), isc_spb_prp_sweep_interval, interval);

    std::vector<char> request;
    if (status.ok()) {
        unsigned length = IXpbBuilder_getBufferLength(builder, status.get());
        const unsigned char *buffer = IXpbBuilder_getBuffer(builder, status.get());
        if (status.ok() && buffer && length > 0) {
            request.assign(reinterpret_cast<const char *>(buffer), reinterpret_cast<const char *>(buffer) + length);
        }
    }
    IXpbBuilder_dispose(builder);

    if (!status.ok() || request.empty()) {
        setError(-200160, FormatInterfaceStatus(status.get()));
        return false;
    }

    std::string output;
    if (!runServiceRequest(request, output)) return false;

    mServiceOutput = output;
    return true;
}

bool FBDatabase::shutdownDenyNewAttachments(long timeoutSeconds) {
    if (mDatabasePath.empty()) {
        setError(-200171, "Database path is unavailable for shutdown");
        return false;
    }
    if (timeoutSeconds < 0) {
        setError(-200172, "Shutdown timeout must be zero or greater");
        return false;
    }

    auto &utilities = GetFirebirdUtilities();
    FirebirdStatusScope status;
    if (!status.get() || !utilities.util) {
        setError(-200173, "Firebird utility interface is unavailable");
        return false;
    }

    IXpbBuilder *builder = IUtil_getXpbBuilder(utilities.util, status.get(), IXpbBuilder_SPB_START, nullptr, 0);
    if (!builder || !status.ok()) {
        setError(-200174, FormatInterfaceStatus(status.get()));
        return false;
    }

    const unsigned char shutdownMode = isc_spb_prp_sm_multi;

    IXpbBuilder_insertTag(builder, status.get(), isc_action_svc_properties);
    IXpbBuilder_insertString(builder, status.get(), isc_spb_dbname, mDatabasePath.c_str());
    if (!mRole.empty()) {
        IXpbBuilder_insertString(builder, status.get(), isc_spb_sql_role_name, mRole.c_str());
    }
    IXpbBuilder_insertBytes(builder, status.get(), isc_spb_prp_shutdown_mode, &shutdownMode, 1);
    IXpbBuilder_insertInt(builder, status.get(), isc_spb_prp_attachments_shutdown, timeoutSeconds);

    std::vector<char> request;
    if (status.ok()) {
        unsigned length = IXpbBuilder_getBufferLength(builder, status.get());
        const unsigned char *buffer = IXpbBuilder_getBuffer(builder, status.get());
        if (status.ok() && buffer && length > 0) {
            request.assign(reinterpret_cast<const char *>(buffer), reinterpret_cast<const char *>(buffer) + length);
        }
    }
    IXpbBuilder_dispose(builder);

    if (!status.ok() || request.empty()) {
        setError(-200175, FormatInterfaceStatus(status.get()));
        return false;
    }

    std::string output;
    if (!runServiceRequest(request, output)) return false;

    mServiceOutput = output;
    return true;
}

bool FBDatabase::bringDatabaseOnline() {
    if (mDatabasePath.empty()) {
        setError(-200176, "Database path is unavailable for online transition");
        return false;
    }

    auto &utilities = GetFirebirdUtilities();
    FirebirdStatusScope status;
    if (!status.get() || !utilities.util) {
        setError(-200177, "Firebird utility interface is unavailable");
        return false;
    }

    IXpbBuilder *builder = IUtil_getXpbBuilder(utilities.util, status.get(), IXpbBuilder_SPB_START, nullptr, 0);
    if (!builder || !status.ok()) {
        setError(-200178, FormatInterfaceStatus(status.get()));
        return false;
    }

    const unsigned char onlineMode = isc_spb_prp_sm_normal;

    IXpbBuilder_insertTag(builder, status.get(), isc_action_svc_properties);
    IXpbBuilder_insertString(builder, status.get(), isc_spb_dbname, mDatabasePath.c_str());
    if (!mRole.empty()) {
        IXpbBuilder_insertString(builder, status.get(), isc_spb_sql_role_name, mRole.c_str());
    }
    IXpbBuilder_insertBytes(builder, status.get(), isc_spb_prp_online_mode, &onlineMode, 1);

    std::vector<char> request;
    if (status.ok()) {
        unsigned length = IXpbBuilder_getBufferLength(builder, status.get());
        const unsigned char *buffer = IXpbBuilder_getBuffer(builder, status.get());
        if (status.ok() && buffer && length > 0) {
            request.assign(reinterpret_cast<const char *>(buffer), reinterpret_cast<const char *>(buffer) + length);
        }
    }
    IXpbBuilder_dispose(builder);

    if (!status.ok() || request.empty()) {
        setError(-200179, FormatInterfaceStatus(status.get()));
        return false;
    }

    std::string output;
    if (!runServiceRequest(request, output)) return false;

    mServiceOutput = output;
    return true;
}

bool FBDatabase::displayUsers() {
    auto &utilities = GetFirebirdUtilities();
    FirebirdStatusScope status;
    if (!status.get() || !utilities.util) {
        setError(-200122, "Firebird utility interface is unavailable");
        return false;
    }

    IXpbBuilder *builder = IUtil_getXpbBuilder(utilities.util, status.get(), IXpbBuilder_SPB_START, nullptr, 0);
    if (!builder || !status.ok()) {
        setError(-200123, FormatInterfaceStatus(status.get()));
        return false;
    }

    IXpbBuilder_insertTag(builder, status.get(), isc_action_svc_display_user);

    std::vector<char> request;
    if (status.ok()) {
        unsigned length = IXpbBuilder_getBufferLength(builder, status.get());
        const unsigned char *buffer = IXpbBuilder_getBuffer(builder, status.get());
        if (status.ok() && buffer && length > 0) {
            request.assign(reinterpret_cast<const char *>(buffer), reinterpret_cast<const char *>(buffer) + length);
        }
    }
    IXpbBuilder_dispose(builder);

    if (!status.ok() || request.empty()) {
        setError(-200124, FormatInterfaceStatus(status.get()));
        return false;
    }

    std::string output;
    if (!runServiceRequest(request, output)) return false;
    if (output.empty()) {
        setError(-200125, "User display returned no output");
        return false;
    }

    mServiceOutput = output;
    return true;
}

bool FBDatabase::addUser(const std::string &userName, const std::string &password) {
    if (userName.empty()) {
        setError(-200126, "User name is required");
        return false;
    }
    if (password.empty()) {
        setError(-200127, "Password is required");
        return false;
    }

    auto &utilities = GetFirebirdUtilities();
    FirebirdStatusScope status;
    if (!status.get() || !utilities.util) {
        setError(-200128, "Firebird utility interface is unavailable");
        return false;
    }

    IXpbBuilder *builder = IUtil_getXpbBuilder(utilities.util, status.get(), IXpbBuilder_SPB_START, nullptr, 0);
    if (!builder || !status.ok()) {
        setError(-200129, FormatInterfaceStatus(status.get()));
        return false;
    }

    IXpbBuilder_insertTag(builder, status.get(), isc_action_svc_add_user);
    IXpbBuilder_insertString(builder, status.get(), isc_spb_sec_username, userName.c_str());
    IXpbBuilder_insertString(builder, status.get(), isc_spb_sec_password, password.c_str());

    std::vector<char> request;
    if (status.ok()) {
        unsigned length = IXpbBuilder_getBufferLength(builder, status.get());
        const unsigned char *buffer = IXpbBuilder_getBuffer(builder, status.get());
        if (status.ok() && buffer && length > 0) {
            request.assign(reinterpret_cast<const char *>(buffer), reinterpret_cast<const char *>(buffer) + length);
        }
    }
    IXpbBuilder_dispose(builder);

    if (!status.ok() || request.empty()) {
        setError(-200130, FormatInterfaceStatus(status.get()));
        return false;
    }

    std::string output;
    if (!runServiceRequest(request, output)) return false;

    mServiceOutput = output;
    return true;
}

bool FBDatabase::changeUserPassword(const std::string &userName, const std::string &password) {
    if (userName.empty()) {
        setError(-200135, "User name is required");
        return false;
    }
    if (password.empty()) {
        setError(-200136, "Password is required");
        return false;
    }

    auto &utilities = GetFirebirdUtilities();
    FirebirdStatusScope status;
    if (!status.get() || !utilities.util) {
        setError(-200137, "Firebird utility interface is unavailable");
        return false;
    }

    IXpbBuilder *builder = IUtil_getXpbBuilder(utilities.util, status.get(), IXpbBuilder_SPB_START, nullptr, 0);
    if (!builder || !status.ok()) {
        setError(-200138, FormatInterfaceStatus(status.get()));
        return false;
    }

    IXpbBuilder_insertTag(builder, status.get(), isc_action_svc_modify_user);
    IXpbBuilder_insertString(builder, status.get(), isc_spb_sec_username, userName.c_str());
    IXpbBuilder_insertString(builder, status.get(), isc_spb_sec_password, password.c_str());

    std::vector<char> request;
    if (status.ok()) {
        unsigned length = IXpbBuilder_getBufferLength(builder, status.get());
        const unsigned char *buffer = IXpbBuilder_getBuffer(builder, status.get());
        if (status.ok() && buffer && length > 0) {
            request.assign(reinterpret_cast<const char *>(buffer), reinterpret_cast<const char *>(buffer) + length);
        }
    }
    IXpbBuilder_dispose(builder);

    if (!status.ok() || request.empty()) {
        setError(-200139, FormatInterfaceStatus(status.get()));
        return false;
    }

    std::string output;
    if (!runServiceRequest(request, output)) return false;

    mServiceOutput = output;
    return true;
}

bool FBDatabase::setUserAdmin(const std::string &userName, bool isAdmin) {
    if (userName.empty()) {
        setError(-200140, "User name is required");
        return false;
    }

    auto &utilities = GetFirebirdUtilities();
    FirebirdStatusScope status;
    if (!status.get() || !utilities.util) {
        setError(-200141, "Firebird utility interface is unavailable");
        return false;
    }

    IXpbBuilder *builder = IUtil_getXpbBuilder(utilities.util, status.get(), IXpbBuilder_SPB_START, nullptr, 0);
    if (!builder || !status.ok()) {
        setError(-200142, FormatInterfaceStatus(status.get()));
        return false;
    }

    IXpbBuilder_insertTag(builder, status.get(), isc_action_svc_modify_user);
    IXpbBuilder_insertString(builder, status.get(), isc_spb_sec_username, userName.c_str());
    IXpbBuilder_insertInt(builder, status.get(), isc_spb_sec_admin, isAdmin ? 1 : 0);

    std::vector<char> request;
    if (status.ok()) {
        unsigned length = IXpbBuilder_getBufferLength(builder, status.get());
        const unsigned char *buffer = IXpbBuilder_getBuffer(builder, status.get());
        if (status.ok() && buffer && length > 0) {
            request.assign(reinterpret_cast<const char *>(buffer), reinterpret_cast<const char *>(buffer) + length);
        }
    }
    IXpbBuilder_dispose(builder);

    if (!status.ok() || request.empty()) {
        setError(-200143, FormatInterfaceStatus(status.get()));
        return false;
    }

    std::string output;
    if (!runServiceRequest(request, output)) return false;

    mServiceOutput = output;
    return true;
}

bool FBDatabase::updateUserNames(const std::string &userName, const std::string &firstName, const std::string &middleName, const std::string &lastName) {
    if (userName.empty()) {
        setError(-200144, "User name is required");
        return false;
    }

    auto &utilities = GetFirebirdUtilities();
    FirebirdStatusScope status;
    if (!status.get() || !utilities.util) {
        setError(-200145, "Firebird utility interface is unavailable");
        return false;
    }

    IXpbBuilder *builder = IUtil_getXpbBuilder(utilities.util, status.get(), IXpbBuilder_SPB_START, nullptr, 0);
    if (!builder || !status.ok()) {
        setError(-200146, FormatInterfaceStatus(status.get()));
        return false;
    }

    IXpbBuilder_insertTag(builder, status.get(), isc_action_svc_modify_user);
    IXpbBuilder_insertString(builder, status.get(), isc_spb_sec_username, userName.c_str());
    IXpbBuilder_insertString(builder, status.get(), isc_spb_sec_firstname, firstName.c_str());
    IXpbBuilder_insertString(builder, status.get(), isc_spb_sec_middlename, middleName.c_str());
    IXpbBuilder_insertString(builder, status.get(), isc_spb_sec_lastname, lastName.c_str());

    std::vector<char> request;
    if (status.ok()) {
        unsigned length = IXpbBuilder_getBufferLength(builder, status.get());
        const unsigned char *buffer = IXpbBuilder_getBuffer(builder, status.get());
        if (status.ok() && buffer && length > 0) {
            request.assign(reinterpret_cast<const char *>(buffer), reinterpret_cast<const char *>(buffer) + length);
        }
    }
    IXpbBuilder_dispose(builder);

    if (!status.ok() || request.empty()) {
        setError(-200147, FormatInterfaceStatus(status.get()));
        return false;
    }

    std::string output;
    if (!runServiceRequest(request, output)) return false;

    mServiceOutput = output;
    return true;
}

bool FBDatabase::deleteUser(const std::string &userName) {
    if (userName.empty()) {
        setError(-200131, "User name is required");
        return false;
    }

    auto &utilities = GetFirebirdUtilities();
    FirebirdStatusScope status;
    if (!status.get() || !utilities.util) {
        setError(-200132, "Firebird utility interface is unavailable");
        return false;
    }

    IXpbBuilder *builder = IUtil_getXpbBuilder(utilities.util, status.get(), IXpbBuilder_SPB_START, nullptr, 0);
    if (!builder || !status.ok()) {
        setError(-200133, FormatInterfaceStatus(status.get()));
        return false;
    }

    IXpbBuilder_insertTag(builder, status.get(), isc_action_svc_delete_user);
    IXpbBuilder_insertString(builder, status.get(), isc_spb_sec_username, userName.c_str());

    std::vector<char> request;
    if (status.ok()) {
        unsigned length = IXpbBuilder_getBufferLength(builder, status.get());
        const unsigned char *buffer = IXpbBuilder_getBuffer(builder, status.get());
        if (status.ok() && buffer && length > 0) {
            request.assign(reinterpret_cast<const char *>(buffer), reinterpret_cast<const char *>(buffer) + length);
        }
    }
    IXpbBuilder_dispose(builder);

    if (!status.ok() || request.empty()) {
        setError(-200134, FormatInterfaceStatus(status.get()));
        return false;
    }

    std::string output;
    if (!runServiceRequest(request, output)) return false;

    mServiceOutput = output;
    return true;
}

void FBDatabase::captureError() {
    char buf[512];
    std::ostringstream oss;
    const ISC_STATUS *pvector = mStatus;

    mErrorCode = isc_sqlcode(mStatus);

    while (fb_interpret(buf, sizeof(buf), &pvector)) {
        if (!oss.str().empty()) oss << "\n";
        oss << buf;
    }
    mErrorMsg = oss.str();
}

void FBDatabase::clearError() {
    mErrorCode = 0;
    mErrorMsg.clear();
    memset(mStatus, 0, sizeof(mStatus));
}

void FBDatabase::setError(long code, const std::string &message) {
    clearError();
    mErrorCode = code;
    mErrorMsg = message;
}

void FBDatabase::configureServiceContext(const std::string &databasePath,
                                         const std::string &user,
                                         const std::string &password,
                                         const std::string &role,
                                         const std::string &host,
                                         int port,
                                         const std::string &wireCrypt,
                                         const std::string &authClientPlugins) {
    mHost = host;
    mPort = port > 0 ? port : 3050;
    mDatabasePath = databasePath;
    mUser = user.empty() ? "SYSDBA" : user;
    mPassword = password;
    mRole = role;
    mWireCrypt = wireCrypt;
    mAuthClientPlugins = authClientPlugins;
    mServiceOutput.clear();
    clearError();
}

void FBDatabase::copyServiceStateFrom(const FBDatabase &other) {
    mServiceOutput = other.mServiceOutput;
    mErrorCode = other.mErrorCode;
    mErrorMsg = other.mErrorMsg;
    memcpy(mStatus, other.mStatus, sizeof(mStatus));
}

// Schema SQL — queries Firebird system tables
const char *FBDatabase::tableListSQL() {
    return "SELECT TRIM(RDB$RELATION_NAME) AS TABLE_NAME "
           "FROM RDB$RELATIONS "
           "WHERE RDB$SYSTEM_FLAG = 0 AND RDB$VIEW_BLR IS NULL "
           "ORDER BY RDB$RELATION_NAME";
}

const char *FBDatabase::columnListSQL() {
    return "SELECT TRIM(RF.RDB$FIELD_NAME) AS COLUMN_NAME, "
           "F.RDB$FIELD_TYPE AS FIELD_TYPE, "
           "F.RDB$FIELD_SUB_TYPE AS FIELD_SUB_TYPE, "
           "F.RDB$FIELD_LENGTH AS FIELD_LENGTH, "
           "F.RDB$FIELD_PRECISION AS FIELD_PRECISION, "
           "F.RDB$FIELD_SCALE AS FIELD_SCALE, "
           "RF.RDB$NULL_FLAG AS NOT_NULL_FLAG, "
           "RF.RDB$DEFAULT_SOURCE AS DEFAULT_SOURCE, "
           "F.RDB$CHARACTER_LENGTH AS CHAR_LEN "
           "FROM RDB$RELATION_FIELDS RF "
           "JOIN RDB$FIELDS F ON RF.RDB$FIELD_SOURCE = F.RDB$FIELD_NAME "
           "WHERE RF.RDB$RELATION_NAME = ? "
           "ORDER BY RF.RDB$FIELD_POSITION";
}

const char *FBDatabase::indexListSQL() {
    return "SELECT TRIM(I.RDB$INDEX_NAME) AS INDEX_NAME, "
           "TRIM(SEG.RDB$FIELD_NAME) AS COLUMN_NAME, "
           "I.RDB$UNIQUE_FLAG AS IS_UNIQUE, "
           "RC.RDB$CONSTRAINT_TYPE AS CONSTRAINT_TYPE "
           "FROM RDB$INDICES I "
           "LEFT JOIN RDB$INDEX_SEGMENTS SEG ON I.RDB$INDEX_NAME = SEG.RDB$INDEX_NAME "
           "LEFT JOIN RDB$RELATION_CONSTRAINTS RC ON I.RDB$INDEX_NAME = RC.RDB$INDEX_NAME "
           "WHERE I.RDB$RELATION_NAME = ? AND I.RDB$SYSTEM_FLAG = 0 "
           "ORDER BY I.RDB$INDEX_NAME, SEG.RDB$FIELD_POSITION";
}

const char *FBDatabase::primaryKeyColumnSQL() {
    return "SELECT TRIM(SEG.RDB$FIELD_NAME) AS COLUMN_NAME "
           "FROM RDB$RELATION_CONSTRAINTS RC "
           "JOIN RDB$INDEX_SEGMENTS SEG ON SEG.RDB$INDEX_NAME = RC.RDB$INDEX_NAME "
           "WHERE RC.RDB$CONSTRAINT_TYPE = 'PRIMARY KEY' "
           "AND RC.RDB$RELATION_NAME = ? "
           "ORDER BY SEG.RDB$FIELD_POSITION";
}

// ============================================================================
// FBStatement
// ============================================================================

FBStatement::FBStatement() {}

FBStatement::~FBStatement() {
    close();
}

void FBStatement::allocateXSQLDA(XSQLDA *&sqlda, int n) {
    if (n < 1) n = 1;
    sqlda = (XSQLDA *)malloc(XSQLDA_LENGTH(n));
    memset(sqlda, 0, XSQLDA_LENGTH(n));
    sqlda->version = SQLDA_CURRENT_VERSION;
    sqlda->sqln = n;
}

void FBStatement::allocateBuffers(XSQLDA *sqlda) {
    if (!sqlda) return;
    for (int i = 0; i < sqlda->sqld; i++) {
        XSQLVAR *var = &sqlda->sqlvar[i];
        short dtype = var->sqltype & ~1;

        switch (dtype) {
            case SQL_VARYING:
                var->sqldata = (char *)calloc(1, var->sqllen + 2);
                break;
            case SQL_TEXT:
                var->sqldata = (char *)calloc(1, var->sqllen + 1);
                break;
            case SQL_SHORT:
                var->sqldata = (char *)calloc(1, sizeof(short));
                break;
            case SQL_LONG:
                var->sqldata = (char *)calloc(1, sizeof(ISC_LONG));
                break;
            case SQL_INT64:
                var->sqldata = (char *)calloc(1, sizeof(ISC_INT64));
                break;
#ifdef SQL_INT128
            case SQL_INT128:
                var->sqldata = (char *)calloc(1, sizeof(FB_I128));
                break;
#endif
            case SQL_FLOAT:
                var->sqldata = (char *)calloc(1, sizeof(float));
                break;
            case SQL_DOUBLE:
                var->sqldata = (char *)calloc(1, sizeof(double));
                break;
#ifdef SQL_DEC16
            case SQL_DEC16:
                var->sqldata = (char *)calloc(1, sizeof(FB_DEC16));
                break;
#endif
#ifdef SQL_DEC34
            case SQL_DEC34:
                var->sqldata = (char *)calloc(1, sizeof(FB_DEC34));
                break;
#endif
            case SQL_TIMESTAMP:
                var->sqldata = (char *)calloc(1, sizeof(ISC_TIMESTAMP));
                break;
            case SQL_TYPE_DATE:
                var->sqldata = (char *)calloc(1, sizeof(ISC_DATE));
                break;
            case SQL_TYPE_TIME:
                var->sqldata = (char *)calloc(1, sizeof(ISC_TIME));
                break;
#ifdef SQL_TIME_TZ
            case SQL_TIME_TZ:
                var->sqldata = (char *)calloc(1, sizeof(ISC_TIME_TZ));
                break;
#endif
#ifdef SQL_TIME_TZ_EX
            case SQL_TIME_TZ_EX:
                var->sqldata = (char *)calloc(1, sizeof(ISC_TIME_TZ_EX));
                break;
#endif
#ifdef SQL_TIMESTAMP_TZ
            case SQL_TIMESTAMP_TZ:
                var->sqldata = (char *)calloc(1, sizeof(ISC_TIMESTAMP_TZ));
                break;
#endif
#ifdef SQL_TIMESTAMP_TZ_EX
            case SQL_TIMESTAMP_TZ_EX:
                var->sqldata = (char *)calloc(1, sizeof(ISC_TIMESTAMP_TZ_EX));
                break;
#endif
            case SQL_BLOB:
                var->sqldata = (char *)calloc(1, sizeof(ISC_QUAD));
                break;
#ifdef SQL_BOOLEAN
            case SQL_BOOLEAN:
                var->sqldata = (char *)calloc(1, sizeof(unsigned char));
                break;
#endif
            default:
                var->sqldata = (char *)calloc(1, var->sqllen > 0 ? var->sqllen : 8);
                break;
        }

        if (var->sqltype & 1) {
            var->sqlind = (short *)calloc(1, sizeof(short));
        }
    }
}

void FBStatement::freeXSQLDA(XSQLDA *&sqlda) {
    if (!sqlda) return;
    for (int i = 0; i < sqlda->sqln; i++) {
        if (sqlda->sqlvar[i].sqldata) free(sqlda->sqlvar[i].sqldata);
        if (sqlda->sqlvar[i].sqlind) free(sqlda->sqlvar[i].sqlind);
    }
    free(sqlda);
    sqlda = nullptr;
}

bool FBStatement::prepare(FBDatabase &db, const std::string &sql) {
    close();
    db.clearError();

    if (!db.ensureTransaction()) return false;

    mStmt = 0;
    if (isc_dsql_allocate_statement(db.mStatus, &db.mDB, &mStmt)) {
        db.captureError();
        return false;
    }

    // Allocate initial output XSQLDA
    allocateXSQLDA(mOutSqlda, 20);

    if (isc_dsql_prepare(db.mStatus, &db.mTrans, &mStmt, 0, sql.c_str(),
                         db.dialect(), mOutSqlda)) {
        db.captureError();
        close();
        return false;
    }

    // Resize if needed
    if (mOutSqlda->sqld > mOutSqlda->sqln) {
        int n = mOutSqlda->sqld;
        freeXSQLDA(mOutSqlda);
        allocateXSQLDA(mOutSqlda, n);
        if (isc_dsql_describe(db.mStatus, &mStmt, 1, mOutSqlda)) {
            db.captureError();
            close();
            return false;
        }
    }

    allocateBuffers(mOutSqlda);
    buildColumnInfo();

    // Describe input parameters
    allocateXSQLDA(mInSqlda, 10);
    if (isc_dsql_describe_bind(db.mStatus, &mStmt, 1, mInSqlda)) {
        db.captureError();
        close();
        return false;
    }

    if (mInSqlda->sqld > mInSqlda->sqln) {
        int n = mInSqlda->sqld;
        freeXSQLDA(mInSqlda);
        allocateXSQLDA(mInSqlda, n);
        if (isc_dsql_describe_bind(db.mStatus, &mStmt, 1, mInSqlda)) {
            db.captureError();
            close();
            return false;
        }
    }

    allocateBuffers(mInSqlda);

    mStmtType = queryStatementType();
    mPrepared = true;
    mExecuted = false;
    return true;
}

int FBStatement::queryStatementType() {
    return QueryStatementTypeHandle(mStmt);
}

bool FBStatement::execute(FBDatabase &db, bool trackAffectedRows) {
    if (!mPrepared) return false;
    db.clearError();
    if (trackAffectedRows) {
        db.mAffectedRowCount = 0;
    }

    if (!db.ensureTransaction()) return false;

    XSQLDA *inParam = (mInSqlda && mInSqlda->sqld > 0) ? mInSqlda : nullptr;

    if (mStmtType == isc_info_sql_stmt_exec_procedure) {
        if (isc_dsql_execute2(db.mStatus, &db.mTrans, &mStmt, 1, inParam, mOutSqlda)) {
            db.captureError();
            return false;
        }
        mExecProcRowPending = true;
    } else {
        if (isc_dsql_execute(db.mStatus, &db.mTrans, &mStmt, 1, inParam)) {
            db.captureError();
            return false;
        }
        mExecProcRowPending = false;
    }

    mExecuted = true;
    if (trackAffectedRows) {
        db.mAffectedRowCount = QueryAffectedRowCountHandle(mStmt);
    }
    return true;
}

bool FBStatement::fetch() {
    if (!mExecuted || !mOutSqlda) return false;

    if (mStmtType == isc_info_sql_stmt_exec_procedure) {
        if (mExecProcRowPending) {
            mExecProcRowPending = false;
            return true;
        }
        return false;
    }

    ISC_STATUS_ARRAY status;
    long stat = isc_dsql_fetch(status, &mStmt, 1, mOutSqlda);
    return (stat == 0);
}

void FBStatement::close() {
    if (mStmt) {
        ISC_STATUS_ARRAY status;
        isc_dsql_free_statement(status, &mStmt, DSQL_drop);
        mStmt = 0;
    }
    freeXSQLDA(mOutSqlda);
    freeXSQLDA(mInSqlda);
    mColumns.clear();
    mPrepared = false;
    mExecuted = false;
    mStmtType = 0;
    mExecProcRowPending = false;
}

int FBStatement::columnCount() const {
    return mOutSqlda ? mOutSqlda->sqld : 0;
}

const FBColumnInfo &FBStatement::columnInfo(int index) const {
    return mColumns[index];
}

int FBStatement::paramCount() const {
    return mInSqlda ? mInSqlda->sqld : 0;
}

short FBStatement::paramBaseType(int index) const {
    if (!mInSqlda || index < 0 || index >= mInSqlda->sqld) return 0;
    return mInSqlda->sqlvar[index].sqltype & ~1;
}

short FBStatement::paramScale(int index) const {
    if (!mInSqlda || index < 0 || index >= mInSqlda->sqld) return 0;
    return mInSqlda->sqlvar[index].sqlscale;
}

int FBStatement::statementType() const {
    return mStmtType;
}

void FBStatement::buildColumnInfo() {
    mColumns.clear();
    if (!mOutSqlda) return;

    for (int i = 0; i < mOutSqlda->sqld; i++) {
        XSQLVAR *var = &mOutSqlda->sqlvar[i];
        FBColumnInfo ci;

        // Column name: prefer alias, fall back to name
        if (var->aliasname_length > 0) {
            ci.alias.assign(var->aliasname, var->aliasname_length);
            // Trim trailing spaces
            size_t end = ci.alias.find_last_not_of(' ');
            if (end != std::string::npos) ci.alias.resize(end + 1);
        }
        if (var->sqlname_length > 0) {
            ci.name.assign(var->sqlname, var->sqlname_length);
            size_t end = ci.name.find_last_not_of(' ');
            if (end != std::string::npos) ci.name.resize(end + 1);
        }
        if (var->relname_length > 0) {
            ci.relation.assign(var->relname, var->relname_length);
            size_t end = ci.relation.find_last_not_of(' ');
            if (end != std::string::npos) ci.relation.resize(end + 1);
        }

        ci.sqltype = var->sqltype;
        ci.sqlsubtype = var->sqlsubtype;
        ci.sqlscale = var->sqlscale;
        ci.sqllen = var->sqllen;

        mColumns.push_back(ci);
    }
}

FBValue FBStatement::columnValue(int index) const {
    FBValue val;
    if (!mOutSqlda || index < 0 || index >= mOutSqlda->sqld) return val;

    XSQLVAR *var = &mOutSqlda->sqlvar[index];

    // Check NULL
    if ((var->sqltype & 1) && var->sqlind && (*var->sqlind < 0)) {
        val.isNull = true;
        return val;
    }
    val.isNull = false;

    short dtype = var->sqltype & ~1;
    val.sqltype = dtype;
    val.sqlscale = var->sqlscale;
    val.sqlsubtype = var->sqlsubtype;

    switch (dtype) {
        case SQL_TEXT: {
            val.strVal.assign(var->sqldata, var->sqllen);
            // Trim trailing spaces for CHAR fields
            size_t end = val.strVal.find_last_not_of(' ');
            if (end != std::string::npos) val.strVal.resize(end + 1);
            else val.strVal.clear();
            break;
        }
        case SQL_VARYING: {
            short len = *(short *)var->sqldata;
            val.strVal.assign(var->sqldata + sizeof(short), len);
            break;
        }
        case SQL_SHORT: {
            short raw = *(short *)var->sqldata;
            if (var->sqlscale < 0) {
                val.dblVal = (double)raw;
                for (int s = 0; s > var->sqlscale; s--) val.dblVal /= 10.0;
            } else {
                val.intVal = raw;
            }
            break;
        }
        case SQL_LONG: {
            ISC_LONG raw = *(ISC_LONG *)var->sqldata;
            if (var->sqlscale < 0) {
                val.dblVal = (double)raw;
                for (int s = 0; s > var->sqlscale; s--) val.dblVal /= 10.0;
            } else {
                val.intVal = raw;
            }
            break;
        }
        case SQL_INT64: {
            ISC_INT64 raw = *(ISC_INT64 *)var->sqldata;
            if (var->sqlscale < 0) {
                val.dblVal = (double)raw;
                for (int s = 0; s > var->sqlscale; s--) val.dblVal /= 10.0;
            } else {
                val.intVal = (int64_t)raw;
            }
            break;
        }
#ifdef SQL_INT128
        case SQL_INT128: {
            if (!FormatInt128Value(*(FB_I128 *)var->sqldata, var->sqlscale, val.strVal)) {
                val.strVal.assign(var->sqldata, sizeof(FB_I128));
            }
            break;
        }
#endif
        case SQL_FLOAT:
            val.dblVal = *(float *)var->sqldata;
            break;
        case SQL_DOUBLE:
            val.dblVal = *(double *)var->sqldata;
            break;
#ifdef SQL_DEC16
        case SQL_DEC16: {
            if (!FormatDecFloat16Value(*(FB_DEC16 *)var->sqldata, val.strVal)) {
                val.strVal.assign(var->sqldata, sizeof(FB_DEC16));
            }
            break;
        }
#endif
#ifdef SQL_DEC34
        case SQL_DEC34: {
            if (!FormatDecFloat34Value(*(FB_DEC34 *)var->sqldata, val.strVal)) {
                val.strVal.assign(var->sqldata, sizeof(FB_DEC34));
            }
            break;
        }
#endif
        case SQL_TYPE_DATE:
            val.dateVal = *(ISC_DATE *)var->sqldata;
            break;
        case SQL_TYPE_TIME:
            val.timeVal = *(ISC_TIME *)var->sqldata;
            break;
        case SQL_TIMESTAMP:
            val.tsVal = *(ISC_TIMESTAMP *)var->sqldata;
            break;
#ifdef SQL_TIME_TZ
        case SQL_TIME_TZ: {
            if (!FormatTimeTzValue(*(ISC_TIME_TZ *)var->sqldata, val.strVal)) {
                val.strVal.assign(var->sqldata, sizeof(ISC_TIME_TZ));
            }
            break;
        }
#endif
#ifdef SQL_TIME_TZ_EX
        case SQL_TIME_TZ_EX: {
            if (!FormatTimeTzExValue(*(ISC_TIME_TZ_EX *)var->sqldata, val.strVal)) {
                val.strVal.assign(var->sqldata, sizeof(ISC_TIME_TZ_EX));
            }
            break;
        }
#endif
#ifdef SQL_TIMESTAMP_TZ
        case SQL_TIMESTAMP_TZ: {
            if (!FormatTimestampTzValue(*(ISC_TIMESTAMP_TZ *)var->sqldata, val.strVal)) {
                val.strVal.assign(var->sqldata, sizeof(ISC_TIMESTAMP_TZ));
            }
            break;
        }
#endif
#ifdef SQL_TIMESTAMP_TZ_EX
        case SQL_TIMESTAMP_TZ_EX: {
            if (!FormatTimestampTzExValue(*(ISC_TIMESTAMP_TZ_EX *)var->sqldata, val.strVal)) {
                val.strVal.assign(var->sqldata, sizeof(ISC_TIMESTAMP_TZ_EX));
            }
            break;
        }
#endif
        case SQL_BLOB:
            val.blobId = *(ISC_QUAD *)var->sqldata;
            break;
#ifdef SQL_BOOLEAN
        case SQL_BOOLEAN:
            val.intVal = *(unsigned char *)var->sqldata ? 1 : 0;
            break;
#endif
        default: {
            val.strVal.assign(var->sqldata, var->sqllen);
            break;
        }
    }

    return val;
}

// ---------------------------------------------------------------------------
// Bind helpers
// ---------------------------------------------------------------------------

void FBStatement::bindNull(int index) {
    if (!mInSqlda || index >= mInSqlda->sqld) return;
    XSQLVAR *var = &mInSqlda->sqlvar[index];
    if (var->sqlind) *var->sqlind = -1;
}

void FBStatement::bindString(int index, const std::string &val) {
    if (!mInSqlda || index >= mInSqlda->sqld) return;
    XSQLVAR *var = &mInSqlda->sqlvar[index];

    short dtype = var->sqltype & ~1;
#ifdef SQL_INT128
    if (dtype == SQL_INT128) {
        FB_I128 parsed = {};
        if (ParseInt128Value(val, var->sqlscale, parsed)) {
            if (var->sqldata) free(var->sqldata);
            var->sqldata = (char *)calloc(1, sizeof(FB_I128));
            memcpy(var->sqldata, &parsed, sizeof(FB_I128));
            var->sqltype = SQL_INT128 | (var->sqltype & 1);
            if (var->sqlind) *var->sqlind = 0;
            return;
        }
    }
#endif
#ifdef SQL_DEC16
    if (dtype == SQL_DEC16) {
        FB_DEC16 parsed = {};
        if (ParseDecFloat16Value(val, parsed)) {
            if (var->sqldata) free(var->sqldata);
            var->sqldata = (char *)calloc(1, sizeof(FB_DEC16));
            memcpy(var->sqldata, &parsed, sizeof(FB_DEC16));
            var->sqltype = SQL_DEC16 | (var->sqltype & 1);
            if (var->sqlind) *var->sqlind = 0;
            return;
        }
    }
#endif
#ifdef SQL_DEC34
    if (dtype == SQL_DEC34) {
        FB_DEC34 parsed = {};
        if (ParseDecFloat34Value(val, parsed)) {
            if (var->sqldata) free(var->sqldata);
            var->sqldata = (char *)calloc(1, sizeof(FB_DEC34));
            memcpy(var->sqldata, &parsed, sizeof(FB_DEC34));
            var->sqltype = SQL_DEC34 | (var->sqltype & 1);
            if (var->sqlind) *var->sqlind = 0;
            return;
        }
    }
#endif
#ifdef SQL_TIME_TZ
    if (dtype == SQL_TIME_TZ) {
        ISC_TIME_TZ parsed = {};
        if (ParseTimeTzValue(val, parsed)) {
            if (var->sqldata) free(var->sqldata);
            var->sqldata = (char *)calloc(1, sizeof(ISC_TIME_TZ));
            memcpy(var->sqldata, &parsed, sizeof(ISC_TIME_TZ));
            var->sqltype = SQL_TIME_TZ | (var->sqltype & 1);
            if (var->sqlind) *var->sqlind = 0;
            return;
        }
    }
#endif
#ifdef SQL_TIMESTAMP_TZ
    if (dtype == SQL_TIMESTAMP_TZ) {
        ISC_TIMESTAMP_TZ parsed = {};
        if (ParseTimestampTzValue(val, parsed)) {
            if (var->sqldata) free(var->sqldata);
            var->sqldata = (char *)calloc(1, sizeof(ISC_TIMESTAMP_TZ));
            memcpy(var->sqldata, &parsed, sizeof(ISC_TIMESTAMP_TZ));
            var->sqltype = SQL_TIMESTAMP_TZ | (var->sqltype & 1);
            if (var->sqlind) *var->sqlind = 0;
            return;
        }
    }
#endif

    // Reallocate buffer for the string
    if (dtype == SQL_VARYING || dtype == SQL_TEXT) {
        if (var->sqldata) free(var->sqldata);
        if (dtype == SQL_VARYING) {
            var->sqllen = (short)val.size();
            var->sqldata = (char *)calloc(1, val.size() + 2);
            short len = (short)val.size();
            memcpy(var->sqldata, &len, sizeof(short));
            memcpy(var->sqldata + sizeof(short), val.c_str(), val.size());
        } else {
            var->sqldata = (char *)calloc(1, val.size() + 1);
            var->sqllen = (short)val.size();
            memcpy(var->sqldata, val.c_str(), val.size());
        }
    } else {
        // Force VARYING type for text binding to non-text params
        var->sqltype = SQL_VARYING | (var->sqltype & 1);
        if (var->sqldata) free(var->sqldata);
        var->sqllen = (short)val.size();
        var->sqldata = (char *)calloc(1, val.size() + 2);
        short len = (short)val.size();
        memcpy(var->sqldata, &len, sizeof(short));
        memcpy(var->sqldata + sizeof(short), val.c_str(), val.size());
    }

    if (var->sqlind) *var->sqlind = 0;
}

void FBStatement::bindInt(int index, int64_t val) {
    if (!mInSqlda || index >= mInSqlda->sqld) return;
    XSQLVAR *var = &mInSqlda->sqlvar[index];
    short dtype = var->sqltype & ~1;

    switch (dtype) {
        case SQL_SHORT:
            *(short *)var->sqldata = (short)val;
            break;
        case SQL_LONG:
            *(ISC_LONG *)var->sqldata = (ISC_LONG)val;
            break;
        case SQL_INT64:
            *(ISC_INT64 *)var->sqldata = (ISC_INT64)val;
            break;
#ifdef SQL_INT128
        case SQL_INT128: {
            FB_I128 parsed = {};
            if (ParseInt128Value(std::to_string(val), var->sqlscale, parsed)) {
                if (var->sqldata) free(var->sqldata);
                var->sqldata = (char *)calloc(1, sizeof(FB_I128));
                memcpy(var->sqldata, &parsed, sizeof(FB_I128));
                break;
            }
            var->sqltype = SQL_INT64 | (var->sqltype & 1);
            if (var->sqldata) free(var->sqldata);
            var->sqldata = (char *)calloc(1, sizeof(ISC_INT64));
            *(ISC_INT64 *)var->sqldata = (ISC_INT64)val;
            break;
        }
#endif
        default:
            // Force INT64
            var->sqltype = SQL_INT64 | (var->sqltype & 1);
            if (var->sqldata) free(var->sqldata);
            var->sqldata = (char *)calloc(1, sizeof(ISC_INT64));
            *(ISC_INT64 *)var->sqldata = (ISC_INT64)val;
            break;
    }
    if (var->sqlind) *var->sqlind = 0;
}

void FBStatement::bindDouble(int index, double val) {
    if (!mInSqlda || index >= mInSqlda->sqld) return;
    XSQLVAR *var = &mInSqlda->sqlvar[index];
    short dtype = var->sqltype & ~1;

#ifdef SQL_INT128
    if (dtype == SQL_INT128) {
        std::ostringstream scaled;
        int digits = var->sqlscale < 0 ? -var->sqlscale : 0;
        scaled << std::fixed << std::setprecision(digits) << val;

        FB_I128 parsed = {};
        if (ParseInt128Value(scaled.str(), var->sqlscale, parsed)) {
            if (var->sqldata) free(var->sqldata);
            var->sqldata = (char *)calloc(1, sizeof(FB_I128));
            memcpy(var->sqldata, &parsed, sizeof(FB_I128));
            if (var->sqlind) *var->sqlind = 0;
            return;
        }
    }
#endif

    if (dtype == SQL_FLOAT) {
        *(float *)var->sqldata = (float)val;
    } else if (dtype == SQL_DOUBLE) {
        *(double *)var->sqldata = val;
    } else if (dtype == SQL_SHORT && var->sqlscale < 0) {
        double scaled = val;
        for (int s = 0; s > var->sqlscale; s--) scaled *= 10.0;
        *(short *)var->sqldata = (short)std::llround(scaled);
    } else if (dtype == SQL_LONG && var->sqlscale < 0) {
        double scaled = val;
        for (int s = 0; s > var->sqlscale; s--) scaled *= 10.0;
        *(ISC_LONG *)var->sqldata = (ISC_LONG)std::llround(scaled);
    } else if (dtype == SQL_INT64 && var->sqlscale < 0) {
        double scaled = val;
        for (int s = 0; s > var->sqlscale; s--) scaled *= 10.0;
        *(ISC_INT64 *)var->sqldata = (ISC_INT64)std::llround(scaled);
    } else {
        var->sqltype = SQL_DOUBLE | (var->sqltype & 1);
        if (var->sqldata) free(var->sqldata);
        var->sqldata = (char *)calloc(1, sizeof(double));
        *(double *)var->sqldata = val;
    }
    if (var->sqlind) *var->sqlind = 0;
}

void FBStatement::bindBlob(int index, FBDatabase &db, const void *data, size_t len) {
    if (!mInSqlda || index >= mInSqlda->sqld) return;
    XSQLVAR *var = &mInSqlda->sqlvar[index];

    ISC_QUAD blobId;
    if (!db.writeBlob(data, len, blobId)) return;

    var->sqltype = SQL_BLOB | (var->sqltype & 1);
    if (var->sqldata) free(var->sqldata);
    var->sqldata = (char *)calloc(1, sizeof(ISC_QUAD));
    *(ISC_QUAD *)var->sqldata = blobId;
    if (var->sqlind) *var->sqlind = 0;
}

void FBStatement::bindExistingBlob(int index, ISC_QUAD blobId) {
    if (!mInSqlda || index >= mInSqlda->sqld) return;
    XSQLVAR *var = &mInSqlda->sqlvar[index];

    var->sqltype = SQL_BLOB | (var->sqltype & 1);
    if (var->sqldata) free(var->sqldata);
    var->sqldata = (char *)calloc(1, sizeof(ISC_QUAD));
    *(ISC_QUAD *)var->sqldata = blobId;
    if (var->sqlind) *var->sqlind = 0;
}

void FBStatement::bindDate(int index, ISC_DATE val) {
    if (!mInSqlda || index >= mInSqlda->sqld) return;
    XSQLVAR *var = &mInSqlda->sqlvar[index];
    var->sqltype = SQL_TYPE_DATE | (var->sqltype & 1);
    if (var->sqldata) free(var->sqldata);
    var->sqldata = (char *)calloc(1, sizeof(ISC_DATE));
    *(ISC_DATE *)var->sqldata = val;
    if (var->sqlind) *var->sqlind = 0;
}

void FBStatement::bindTime(int index, ISC_TIME val) {
    if (!mInSqlda || index >= mInSqlda->sqld) return;
    XSQLVAR *var = &mInSqlda->sqlvar[index];
    var->sqltype = SQL_TYPE_TIME | (var->sqltype & 1);
    if (var->sqldata) free(var->sqldata);
    var->sqldata = (char *)calloc(1, sizeof(ISC_TIME));
    *(ISC_TIME *)var->sqldata = val;
    if (var->sqlind) *var->sqlind = 0;
}

void FBStatement::bindTimestamp(int index, ISC_TIMESTAMP val) {
    if (!mInSqlda || index >= mInSqlda->sqld) return;
    XSQLVAR *var = &mInSqlda->sqlvar[index];
    var->sqltype = SQL_TIMESTAMP | (var->sqltype & 1);
    if (var->sqldata) free(var->sqldata);
    var->sqldata = (char *)calloc(1, sizeof(ISC_TIMESTAMP));
    *(ISC_TIMESTAMP *)var->sqldata = val;
    if (var->sqlind) *var->sqlind = 0;
}

void FBStatement::bindBoolean(int index, bool val) {
    if (!mInSqlda || index >= mInSqlda->sqld) return;
    XSQLVAR *var = &mInSqlda->sqlvar[index];
    short dtype = var->sqltype & ~1;

    // Respect the parameter's original type from describe_bind
#ifdef SQL_BOOLEAN
    if (dtype == SQL_BOOLEAN) {
        *(unsigned char *)var->sqldata = val ? 1 : 0;
    } else
#endif
    if (dtype == SQL_SHORT) {
        *(short *)var->sqldata = val ? 1 : 0;
    } else if (dtype == SQL_LONG) {
        *(ISC_LONG *)var->sqldata = val ? 1 : 0;
    } else if (dtype == SQL_INT64) {
        *(ISC_INT64 *)var->sqldata = val ? 1 : 0;
    } else {
        // Unknown type — use SMALLINT
        var->sqltype = SQL_SHORT | (var->sqltype & 1);
        if (var->sqldata) free(var->sqldata);
        var->sqldata = (char *)calloc(1, sizeof(short));
        *(short *)var->sqldata = val ? 1 : 0;
    }
    if (var->sqlind) *var->sqlind = 0;
}

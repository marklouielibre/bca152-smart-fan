#ifndef app_wifi_h_
#define app_wifi_h_

#ifndef WIFI_SSID
#define WIFI_SSID
#endif

#ifndef WIFI_PASS
#define WIFI_PASS
#endif

#define APP_WIFI_ARG3(_0, _1, _2, ...) _2
#define APP_WIFI_HAS_COMMA(...) APP_WIFI_ARG3(__VA_ARGS__, 1, 0)
#define APP_WIFI_TRIGGER_PARENTHESIS_(...) ,
#define APP_WIFI_CAT5(_0, _1, _2, _3, _4) APP_WIFI_CAT5_I(_0, _1, _2, _3, _4)
#define APP_WIFI_CAT5_I(_0, _1, _2, _3, _4) _0##_1##_2##_3##_4
#define APP_WIFI_IS_EMPTY_CASE_0000 0
#define APP_WIFI_IS_EMPTY_CASE_0001 1
#define APP_WIFI_IS_EMPTY_IMPL(_0, _1, _2, _3) APP_WIFI_CAT5(APP_WIFI_IS_EMPTY_CASE_, _0, _1, _2, _3)
#define APP_WIFI_IS_EMPTY(_0) APP_WIFI_IS_EMPTY_IMPL( \
	APP_WIFI_HAS_COMMA(_0), \
	APP_WIFI_HAS_COMMA(APP_WIFI_TRIGGER_PARENTHESIS_ _0), \
	APP_WIFI_HAS_COMMA(_0 (/*empty*/)), \
	APP_WIFI_HAS_COMMA(APP_WIFI_TRIGGER_PARENTHESIS_ _0 (/*empty*/)))

#if APP_WIFI_IS_EMPTY(WIFI_SSID)
#undef WIFI_SSID
#define WIFI_SSID "louie"
#endif

#if APP_WIFI_IS_EMPTY(WIFI_PASS)
#undef WIFI_PASS
#define WIFI_PASS "marklouie"
#endif

#undef APP_WIFI_ARG3
#undef APP_WIFI_HAS_COMMA
#undef APP_WIFI_TRIGGER_PARENTHESIS_
#undef APP_WIFI_CAT5
#undef APP_WIFI_CAT5_I
#undef APP_WIFI_IS_EMPTY_CASE_0000
#undef APP_WIFI_IS_EMPTY_CASE_0001
#undef APP_WIFI_IS_EMPTY_IMPL
#undef APP_WIFI_IS_EMPTY

typedef void (*on_connected_f)(void);
typedef void (*on_failed_f)(void);

typedef struct {
    on_connected_f on_connected;
    on_failed_f on_failed;
} connect_wifi_params_t;

void appwifi_connect(connect_wifi_params_t);

#endif

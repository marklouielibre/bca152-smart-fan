
#ifndef app_aws_h_
#define app_aws_h_

#include <stdint.h>

#ifndef AWS_THING_NAME
#define AWS_THING_NAME "myhome_fan1"
#endif

#ifndef AWS_ENDPOINT
#define AWS_ENDPOINT
#endif

#define APP_AWS_ARG3(_0, _1, _2, ...) _2
#define APP_AWS_HAS_COMMA(...) APP_AWS_ARG3(__VA_ARGS__, 1, 0)
#define APP_AWS_TRIGGER_PARENTHESIS_(...) ,
#define APP_AWS_CAT5(_0, _1, _2, _3, _4) APP_AWS_CAT5_I(_0, _1, _2, _3, _4)
#define APP_AWS_CAT5_I(_0, _1, _2, _3, _4) _0##_1##_2##_3##_4
#define APP_AWS_IS_EMPTY_CASE_0000 0
#define APP_AWS_IS_EMPTY_CASE_0001 1
#define APP_AWS_IS_EMPTY_IMPL(_0, _1, _2, _3) APP_AWS_CAT5(APP_AWS_IS_EMPTY_CASE_, _0, _1, _2, _3)
#define APP_AWS_IS_EMPTY(_0) APP_AWS_IS_EMPTY_IMPL( \
	APP_AWS_HAS_COMMA(_0), \
	APP_AWS_HAS_COMMA(APP_AWS_TRIGGER_PARENTHESIS_ _0), \
	APP_AWS_HAS_COMMA(_0 (/*empty*/)), \
	APP_AWS_HAS_COMMA(APP_AWS_TRIGGER_PARENTHESIS_ _0 (/*empty*/)))

#if APP_AWS_IS_EMPTY(AWS_ENDPOINT)
#undef AWS_ENDPOINT
#define AWS_ENDPOINT "placeholder.iot.us-east-1.amazonaws.com"
#endif

#undef APP_AWS_ARG3
#undef APP_AWS_HAS_COMMA
#undef APP_AWS_TRIGGER_PARENTHESIS_
#undef APP_AWS_CAT5
#undef APP_AWS_CAT5_I
#undef APP_AWS_IS_EMPTY_CASE_0000
#undef APP_AWS_IS_EMPTY_CASE_0001
#undef APP_AWS_IS_EMPTY_IMPL
#undef APP_AWS_IS_EMPTY

typedef void (*fan_state_changed_f)(uint8_t);

void appaws_init(fan_state_changed_f);
void appaws_connect(void *);
void appaws_publish(uint8_t);


#endif

#ifndef AT_COMMAND_PARSER_H
#define AT_COMMAND_PARSER_H
#include "app_error.h"
#include "nrf.h"

ret_code_t at_command_parse(char * buffer, int len);

#endif //AT_COMMAND_PARSER_H
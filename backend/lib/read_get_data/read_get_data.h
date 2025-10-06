#ifndef READ_GET_DATA_H_
#define READ_GET_DATA_H_

#include "/app/backend/lib/result/result.h"

#define ERR_GET_NULL_INPUT 3001
result_t* read_get_data(char** out_query);

#endif// READ_GET_DATA_H_

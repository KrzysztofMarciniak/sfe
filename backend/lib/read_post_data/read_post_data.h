#ifndef READ_POST_DATA_H_
#define READ_POST_DATA_H_
#include "/app/backend/lib/result/result.h"
result_t read_post_data(char** out_body);
/**
 * @brief Error codes for read_post_data operations
 */
#define ERR_INVALID_CONTENT_LENGTH                              \
        2001               /* CONTENT_LENGTH not set or invalid \
                            */
#define ERR_READ_FAIL 2002 /* Failed to read POST data */

#endif// READ_POST_DATA_H_

#ifndef DATA_MARSH_H
#define DATA_MARSH_H

#include <stdint.h>
#include "../str_util/str_util.h"

#define CPD_VERSION "0.0.1"

#define CPD_FLAG_ERR_NULLPTR    0x01
#define CPD_FLAG_ERR_ALLOC      0x02
#define CPD_FLAG_ERR_TYPE       0x03
#define CPD_FLAG_ERR_EOF        0x04
#define CPD_FLAG_SUCCESS        0x00

typedef struct dm_encode_st dm_encode;
typedef struct dm_decode_st dm_decode;

typedef void *(dm_obj_new)();
typedef int32_t(*dm_encode_func)(void *, dm_encode *) __THROW __nonnull ((1, 2));
typedef int32_t(*dm_decode_func)(void *, dm_decode *) __THROW __nonnull ((1, 2));



void dm_encode_clear(dm_encode *ctx) __THROW __nonnull ((1));
void dm_encode_init (dm_encode *ctx) __THROW __nonnull ((1));
void dm_encode_free (dm_encode *ctx) __THROW __nonnull ((1));

uint64_t dm_encode_size(const dm_encode *ctx) __THROW __nonnull ((1));
uint64_t dm_encode_data(const dm_encode *ctx, uint8_t *data, uint64_t size, uint64_t offset) __THROW __nonnull ((1, 2));

int32_t dm_encode_struct(dm_encode *ctx, void *_obj, dm_encode_func func) __THROW __nonnull ((1, 2));
int32_t dm_encode_buff(dm_encode *ctx, const uint8_t *str, int32_t size) __THROW __nonnull ((1, 2));
int32_t dm_encode_str(dm_encode *ctx, const string_t *str) __THROW __nonnull ((1, 2));

int32_t dm_encode_double(dm_encode *ctx, double _double) __THROW __nonnull ((1));
int32_t dm_encode_float (dm_encode *ctx, float  _float ) __THROW __nonnull ((1));

int32_t dm_encode_uint64(dm_encode *ctx, uint64_t _uint64) __THROW __nonnull ((1));
int32_t dm_encode_uint32(dm_encode *ctx, uint32_t _uint32) __THROW __nonnull ((1));
int32_t dm_encode_uint16(dm_encode *ctx, uint16_t _uint16) __THROW __nonnull ((1));
int32_t dm_encode_uint8 (dm_encode *ctx, uint8_t  _uint8 ) __THROW __nonnull ((1));

int32_t dm_encode_int64(dm_encode *ctx, int64_t _int64) __THROW __nonnull ((1));
int32_t dm_encode_int32(dm_encode *ctx, int32_t _int32) __THROW __nonnull ((1));
int32_t dm_encode_int16(dm_encode *ctx, int16_t _int16) __THROW __nonnull ((1));
int32_t dm_encode_int8 (dm_encode *ctx, int8_t  _int8 ) __THROW __nonnull ((1));



void dm_decode_clear(dm_decode *ctx) __THROW __nonnull ((1));
void dm_decode_init (dm_decode *ctx) __THROW __nonnull ((1));
void dm_decode_free (dm_decode *ctx) __THROW __nonnull ((1));

uint64_t dm_decode_count(const dm_decode *ctx) __THROW __nonnull ((1));
int32_t dm_decode_data  (      dm_decode *ctx, const uint8_t *data, uint64_t size) __THROW __nonnull ((1, 2));

int32_t dm_decode_struct(dm_decode *ctx, void **_obj, dm_decode_func func, dm_obj_new func_new) __THROW __nonnull ((1, 2));
int32_t dm_decode_buff(dm_decode *ctx, uint8_t *str, uint64_t size, uint64_t *res_size) __THROW __nonnull ((1, 2, 4));
int32_t dm_decode_str(dm_decode *ctx, string_t *str) __THROW __nonnull ((1, 2));

int32_t dm_decode_double(dm_decode *ctx, double *_double) __THROW __nonnull ((1, 2));
int32_t dm_decode_float (dm_decode *ctx, float  *_float ) __THROW __nonnull ((1, 2));

int32_t dm_decode_uint64(dm_decode *ctx, uint64_t *_uint64) __THROW __nonnull ((1, 2));
int32_t dm_decode_uint32(dm_decode *ctx, uint32_t *_uint32) __THROW __nonnull ((1, 2));
int32_t dm_decode_uint16(dm_decode *ctx, uint16_t *_uint16) __THROW __nonnull ((1, 2));
int32_t dm_decode_uint8 (dm_decode *ctx, uint8_t  *_uint8 ) __THROW __nonnull ((1, 2));

int32_t dm_decode_int64(dm_decode *ctx, int64_t *_int64) __THROW __nonnull ((1, 2));
int32_t dm_decode_int32(dm_decode *ctx, int32_t *_int32) __THROW __nonnull ((1, 2));
int32_t dm_decode_int16(dm_decode *ctx, int16_t *_int16) __THROW __nonnull ((1, 2));
int32_t dm_decode_int8 (dm_decode *ctx, int8_t  *_int8 ) __THROW __nonnull ((1, 2));






#endif //DATA_MARSH_H

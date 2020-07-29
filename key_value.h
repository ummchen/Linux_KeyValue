/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright 2020, Josh Chen
 */

#ifndef __KEY_VALUE_H__
#define __KEY_VALUE_H__

#include <stdbool.h>

/***** Key Value Define *****/
enum KV_RETURN {
	KV_OK_SUCCESS		= 0,
	KV_OUT_OF_MEM_ERR	= -1,
	KV_ARGUMENT_ERR		= -2,
	KV_FILE_ERR			= -3,
	KV_NOT_FOUND		= -4,
};

bool kv_is_decimal(const char *string);

bool kv_is_hexadecimal(const char *string);

void kv_trim_string(char *string);

int kv_get_value(
		const char *filename,
		const char *key,
		char *value,
		size_t size);

int kv_set_value(
		const char *filename,
		const char *key,
		const char *value);

#endif //__KEY_VALUE_H__

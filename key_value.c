// SPDX-License-Identifier: BSD-3-Clause
// Copyright 2020, Josh Chen

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include "key_value.h"

#define KV_LINE_SIZE	512
#define KV_DELIMITERS	" =,\t\r\n"

#define KV_DBG			printf
#define KV_ZALLOC(t, n)	((t *) calloc((n), sizeof(t)))
#define KV_FREE(p)		{ free((void *)(p)); (p) = NULL; }
#define KV_FCLOSE(f)	{ fclose((f)); (f) = NULL; }

static int kv_free_buf(char **buf)
{
	int rtn = KV_OK_SUCCESS;

	if (buf == NULL) {
		KV_DBG("buf = %p\n", buf);
		rtn = KV_ARGUMENT_ERR;
		goto exit_kv_free_buf;
	}

	if (*buf != NULL)
		KV_FREE(*buf);

exit_kv_free_buf:
	return rtn;
}

static int kv_alloc_buf(
		char **buf,
		size_t size)
{
	int rtn = KV_OK_SUCCESS;

	if ((buf == NULL) || (size == 0)) {
		KV_DBG("buf = %p, size = %lu\n", buf, size);
		rtn = KV_ARGUMENT_ERR;
		goto exit_kv_alloc_buf;
	}

	*buf = KV_ZALLOC(char, size);
	if (*buf == NULL) {
		rtn = KV_OUT_OF_MEM_ERR;
		goto exit_kv_alloc_buf;
	}

exit_kv_alloc_buf:
	return rtn;
}

static size_t kv_get_value_pos(
		const char *string,
		const char *key)
{
	char *line = NULL, *pch = NULL;
	size_t key_len = 0, token_len = 0;
	size_t rtn = 0;

	if ((string == NULL) || (key == NULL)) {
		KV_DBG("string = %p, key = %p\n", string, key);
		goto exit_kv_get_value_pos;
	}

	key_len = strlen(key);
	if ((strlen(string) == 0) || (key_len == 0)) {
		KV_DBG("string = %s, key = %s\n", string, key);
		goto exit_kv_get_value_pos;
	}

	if (kv_alloc_buf(&line, KV_LINE_SIZE) != KV_OK_SUCCESS)
		goto exit_kv_get_value_pos;

	strncpy(line, string, KV_LINE_SIZE);

	pch = strtok(line, KV_DELIMITERS);
	if (pch == NULL)
		goto exit_kv_get_value_pos;

	token_len = strlen(pch);
	if ((key_len != token_len) || (strncmp(pch, key, key_len) != 0))
		goto exit_kv_get_value_pos;

	pch = strtok(NULL, KV_DELIMITERS);
	if (pch == NULL)
		goto exit_kv_get_value_pos;

	if (pch < line) {
		KV_DBG("pch = %p, line = %p\n", pch, line);
		goto exit_kv_get_value_pos;
	}

	rtn = pch - line;

exit_kv_get_value_pos:
	kv_free_buf(&line);
	return rtn;
}

bool kv_is_decimal(const char *string)
{
	size_t len = 0;

	if (string == NULL)
		return false;

	len = strlen(string);
	while (len--) {
		if (isdigit(string[len]) == 0)
			return false;
	}

	return true;
}

bool kv_is_hexadecimal(const char *string)
{
	size_t len = 0;

	if (string == NULL)
		return false;

	len = strlen(string);
	while (len--) {
		if ((isxdigit(string[len] == 0)) &&
			(toupper(string[len]) != 'X'))
			return false;
	}

	return true;
}

void kv_trim_string(char *string)
{
	char *pch = string;
	size_t len = 0;

	if (string == NULL)
		return;

	//remove end of line
	strtok(string, "\r\n");

	//remove leading spaces
	while ((*pch != '\0') && (isspace(*pch)))
		++pch;

	len = strlen(pch) + 1;
	memmove(string, pch, len);

	//remove trailling spaces
	pch = string + len;
	while ((string < pch) && (isspace(*pch)))
		--pch;

	*pch = '\0';
}

int kv_get_value(
		const char *filename,
		const char *key,
		char *value,
		size_t size)
{
	FILE *fp = NULL;
	char *line = NULL;
	size_t value_pos = 0, key_len = 0, token_len = 0;
	int rtn = KV_OK_SUCCESS;

	if ((filename == NULL) || (key == NULL) || (value == NULL) ||
			(size == 0)) {
		KV_DBG("filename = %p, key = %p, value = %p, size = %lu\n",
				filename, key, value, size);
		rtn = KV_ARGUMENT_ERR;
		goto exit_kv_get_value;
	}

	key_len = strlen(key);
	if ((strlen(filename) == 0) || (key_len == 0)) {
		KV_DBG("filename = %s, key = %s\n", filename, key);
		rtn = KV_ARGUMENT_ERR;
		goto exit_kv_get_value;
	}

	fp = fopen(filename, "r");
	if (fp == NULL) {
		KV_DBG("fopen %s errno = %d\n", filename, errno);
		rtn = KV_ARGUMENT_ERR;
		goto exit_kv_get_value;
	}

	rtn = kv_alloc_buf(&line, KV_LINE_SIZE);
	if (rtn != KV_OK_SUCCESS)
		goto exit_kv_get_value;

	if (fseek(fp, 0, SEEK_SET) != 0) {
		rtn = KV_FILE_ERR;
		goto exit_kv_get_value;
	}

	rtn = KV_NOT_FOUND;
	while (fgets(line, KV_LINE_SIZE, fp) != NULL) {
		kv_trim_string(line);

		value_pos = (strlen(line) == 0) ?
			0 : kv_get_value_pos(line, key);
		if (value_pos == 0) {
			memset(line, 0, KV_LINE_SIZE);
			continue;
		}

		token_len = strlen(line + value_pos);
		if (token_len > size) {
			rtn = KV_OUT_OF_MEM_ERR;
			goto exit_kv_get_value;
		}

		strncpy(value, line + value_pos, token_len);
		value[token_len] = '\0';
		rtn = KV_OK_SUCCESS;
		break;
	}

exit_kv_get_value:
	kv_free_buf(&line);

	if (fp != NULL)
		KV_FCLOSE(fp);

	return rtn;
}

int kv_set_value(
		const char *filename,
		const char *key,
		const char *value)
{
	FILE *fp = NULL, *tmp_fp = NULL;
	char *tmp_buf = NULL, *line = NULL;
	size_t value_pos = 0, key_len = 0, tmp_len = 0;
	bool key_found = false;
	int rtn = KV_OK_SUCCESS;

	if ((filename == NULL) || (key == NULL) || (value == NULL)) {
		KV_DBG("filename = %p, key = %p, value = %p\n",
				filename, key, value);
		rtn = KV_ARGUMENT_ERR;
		goto exit_kv_set_value;
	}

	key_len = strlen(key);
	if ((strlen(filename) == 0) || (key_len == 0) || (strlen(value) == 0)) {
		KV_DBG("filename = %s, key = %s, value = %s\n",
				filename, key, value);
		rtn = KV_ARGUMENT_ERR;
		goto exit_kv_set_value;
	}

	rtn = kv_alloc_buf(&line, KV_LINE_SIZE);
	if (rtn != KV_OK_SUCCESS)
		goto exit_kv_set_value;

	tmp_fp = open_memstream(&tmp_buf, &tmp_len);
	if (tmp_fp == NULL) {
		KV_DBG("open_memstream errno = %d\n", errno);
		rtn = KV_OUT_OF_MEM_ERR;
		goto exit_kv_set_value;
	}

	fp = fopen(filename, "r");
	if (fp == NULL) {
		KV_DBG("%s not exists, create new one\n", filename);
		goto new_kv_set_value;
	}

	while (fgets(line, KV_LINE_SIZE, fp) != NULL) {
		kv_trim_string(line);

		if (strlen(line) == 0)
			continue;

		value_pos = kv_get_value_pos(line, key);
		if (value_pos == 0)
			fprintf(tmp_fp, "%s\n", line);
		else {
			fprintf(tmp_fp, "%s=%s\n", key, value);
			key_found = true;
		}

		memset(line, 0, KV_LINE_SIZE);
	}
	KV_FCLOSE(fp);

new_kv_set_value:
	if (key_found == false)
		fprintf(tmp_fp, "%s=%s\n", key, value);

	fflush(tmp_fp);
	if (fseek(tmp_fp, 0, SEEK_SET) != 0) {
		KV_DBG("fseek tmp_fp set fail\n");
		rtn = KV_FILE_ERR;
		goto exit_kv_set_value;
	}

	fp = fopen(filename, "w");
	if (fp == NULL) {
		KV_DBG("fopen %s errno = %d\n", filename, errno);
		rtn = KV_ARGUMENT_ERR;
		goto exit_kv_set_value;
	}

	while (fgets(line, KV_LINE_SIZE, tmp_fp) != NULL) {
		if (fputs(line, fp) == EOF) {
			KV_DBG("fputs to fp failed\n");
			rtn = KV_OUT_OF_MEM_ERR;
			goto exit_kv_set_value;
		}

		memset(line, 0, KV_LINE_SIZE);
	}

exit_kv_set_value:
	kv_free_buf(&line);

	if (fp != NULL)
		KV_FCLOSE(fp);

	if (tmp_fp != NULL)
		KV_FCLOSE(tmp_fp);

	KV_FREE(tmp_buf);

	return rtn;
}


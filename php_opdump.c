/* {{{ Includes */
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php_opdump.h"
#include "php_ini.h"
#include "ext/standard/info.h"
#include "zend.h"

#include "php.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#if HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef HAVE_SYS_UIO_H
# include <sys/uio.h>
#endif

#ifdef HAVE_SYS_FILE_H
# include <sys/file.h>
#endif

#include <ext/opcache/ZendAccelerator.h>
#include <ext/opcache/zend_file_cache.h>
#include <ext/opcache/zend_accelerator_util_funcs.h>
/* }}} */
/* {{{ Global functions */
#ifndef O_BINARY
#  define O_BINARY 0
#endif
typedef struct _zend_file_cache_metainfo {
	char         magic[8];
	char         system_id[32];
	size_t       mem_size;
	size_t       str_size;
	size_t       script_offset;
	accel_time_t timestamp;
	uint32_t     checksum;
} zend_file_cache_metainfo;

static int zend_file_cache_mkdir(char *filename, size_t start);
static void zend_file_cache_serialize(zend_persistent_script *script, zend_file_cache_metainfo *info, void *buf);
static int zend_file_cache_flock(int fd, int op);
static zend_persistent_script *opcache_compile_file(zend_file_handle *file_handle, int type, char *key, unsigned int key_length, zend_op_array **op_array_p);
/**
 * @brief Function copied from zend_file_cache.c with pointing file where to store OPcache 
 * @param script Script object to store
 * @param filename File path to store script
 * @return bool
 */
int zend_file_cache_script_store_file(zend_persistent_script *script, char *filename, size_t filename_len)
{
	int fd;
	zend_file_cache_metainfo info;
#ifdef HAVE_SYS_UIO_H
	struct iovec vec[3];
#endif
	void *mem, *buf;

	if (zend_file_cache_mkdir(filename, filename_len) != SUCCESS) {
		zend_accel_error(ACCEL_LOG_WARNING, "opdump cannot create directory for file '%s'\n", filename);
		efree(filename);
		return FAILURE;
	}

#ifndef ZEND_WIN32
	fd = open(filename, O_CREAT | O_EXCL | O_RDWR | O_BINARY, S_IRUSR | S_IWUSR);
#else
	fd = open(filename, O_CREAT | O_EXCL | O_RDWR | O_BINARY, _S_IREAD | _S_IWRITE);
#endif
	if (fd < 0) {
		if (errno != EEXIST) {
			zend_accel_error(ACCEL_LOG_WARNING, "opdump cannot create file '%s'\n", filename);
		}
		efree(filename);
		return FAILURE;
	}
	if (zend_file_cache_flock(fd, LOCK_EX) != 0) {
		close(fd);
		efree(filename);
		return FAILURE;
	}
#ifdef __SSE2__
	/* Align to 64-byte boundary */
	mem = emalloc(script->size + 64);
	buf = (void*)(((zend_uintptr_t)mem + 63L) & ~63L);
#else
	mem = buf = emalloc(script->size);
#endif

	ZCG(mem) = zend_string_alloc(4096 - (_ZSTR_HEADER_SIZE + 1), 0);

	zend_file_cache_serialize(script, &info, buf);

	info.checksum = zend_adler32(ADLER32_INIT, buf, script->size);
	info.checksum = zend_adler32(info.checksum, (signed char*)ZSTR_VAL((zend_string*)ZCG(mem)), info.str_size);

#ifdef HAVE_SYS_UIO_H
	vec[0].iov_base = &info;
	vec[0].iov_len = sizeof(info);
	vec[1].iov_base = buf;
	vec[1].iov_len = script->size;
	vec[2].iov_base = ZSTR_VAL((zend_string*)ZCG(mem));
	vec[2].iov_len = info.str_size;

	if (writev(fd, vec, 3) != (ssize_t)(sizeof(info) + script->size + info.str_size)) {
		zend_accel_error(ACCEL_LOG_WARNING, "opdump cannot write to file '%s'\n", filename);
		zend_string_release((zend_string*)ZCG(mem));
		efree(mem);
		unlink(filename);
		efree(filename);
		return FAILURE;
	}
#else
	if (ZEND_LONG_MAX < (zend_long)(sizeof(info) + script->size + info.str_size) ||
		write(fd, &info, sizeof(info)) != sizeof(info) ||
		write(fd, buf, script->size) != script->size ||
		write(fd, ((zend_string*)ZCG(mem))->val, info.str_size) != info.str_size
		) {
		zend_accel_error(ACCEL_LOG_WARNING, "opdump cannot write to file '%s'\n", filename);
		zend_string_release((zend_string*)ZCG(mem));
		efree(mem);
		unlink(filename);
		efree(filename);
		return FAILURE;
	}
#endif

	zend_string_release((zend_string*)ZCG(mem));
	efree(mem);
	if (zend_file_cache_flock(fd, LOCK_UN) != 0) {
		zend_accel_error(ACCEL_LOG_WARNING, "opdump cannot unlock file '%s'\n", filename);
	}
	close(fd);
	efree(filename);

	return SUCCESS;
}

ZEND_FUNCTION(opdump_file)
{
	int cached;
	char *filename;
	char *script_name;
	size_t script_name_len;
	char *opbin_name;
	size_t opbin_name_len;
	zend_file_handle file_handle;
	zend_persistent_script *persistent_script;
	zend_op_array *op_array = NULL;
	zend_execute_data *orig_execute_data = NULL;

	if (zend_parse_parameters(ZEND_NUM_ARGS(), "ss", &script_name, &script_name_len, &opbin_name, &opbin_name_len) == FAILURE) {
		return;
	}

	if (!ZCG(enabled) || !accel_startup_ok || !ZCSG(accelerator_enabled)) {
		zend_error(E_NOTICE, ACCELERATOR_PRODUCT_NAME " seems to be disabled, can't compile file");
		RETURN_FALSE;
	}

	file_handle.filename = script_name;
	file_handle.free_filename = 0;
	file_handle.opened_path = NULL;
	file_handle.type = ZEND_HANDLE_FILENAME;

	zend_try {
		persistent_script = opcache_compile_file(&file_handle, ZEND_INCLUDE, NULL, 0, &op_array);
		if (persistent_script) {
			cached = zend_file_cache_script_store(persistent_script, 0);
			zend_file_cache_script_store_file(persistent_script, opbin_name, opbin_name_len);
		}
	} zend_catch {
		zend_error(E_WARNING, ACCELERATOR_PRODUCT_NAME " could not compile file %s", file_handle.filename);
	} zend_end_try();

	if(op_array != NULL) {
		destroy_op_array(op_array);
		efree(op_array);
		RETVAL_TRUE;
	} else {
		RETVAL_FALSE;
	}
	zend_destroy_file_handle(&file_handle);
	
  RETURN_FALSE;
}
/* }}} */
/* {{{ Function table */
ZEND_BEGIN_ARG_INFO_EX(ai_opdump_file, 0, 0, 2)
  ZEND_ARG_INFO(0, source)
  ZEND_ARG_INFO(0, destination)
ZEND_END_ARG_INFO()

static zend_function_entry opdump_functions[] = {
  ZEND_FE(opdump_file, ai_opdump_file)
  {NULL, NULL, NULL}
};
/* }}} */
/* {{{ Init */
PHP_MINIT_FUNCTION(opdump)
{  
  return SUCCESS;
}
/* }}} */
/* {{{ Info */
PHP_MINFO_FUNCTION(opdump)
{
  php_info_print_table_start();
  php_info_print_table_header(2, "opdump", "enabled");
  php_info_print_table_row(2, "Version", "1.0");
  php_info_print_table_end();
}
/* }}} */
/* {{{ Module entry */
zend_module_entry opdump_module_entry = {
  STANDARD_MODULE_HEADER,
  "opdump",
  opdump_functions,
  PHP_MINIT(opdump),
  NULL,
  NULL,
  NULL,
  PHP_MINFO(opdump),
  "1.0",
  STANDARD_MODULE_PROPERTIES
};
#ifdef COMPILE_DL_opdump
ZEND_GET_MODULE(opdump)
#endif
/* }}} */
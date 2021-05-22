/*
  +----------------------------------------------------------------------+
  | PHP Version 7                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) 1997-2018 The PHP Group                                |
  +----------------------------------------------------------------------+
  | This source file is subject to version 3.01 of the PHP license,      |
  | that is bundled with this package in the file LICENSE, and is        |
  | available through the world-wide-web at the following url:           |
  | http://www.php.net/license/3_01.txt                                  |
  | If you did not receive a copy of the PHP license and are unable to   |
  | obtain it through the world-wide-web, please send a note to          |
  | license@php.net so we can mail you a copy immediately.               |
  +----------------------------------------------------------------------+
  | Author:                                                              |
  +----------------------------------------------------------------------+
*/

/* $Id$ */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "php_ini.h"
#include "ext/standard/info.h"
#include "php_sentinel.h"
#include "Zend/zend_exceptions.h"

ZEND_DECLARE_MODULE_GLOBALS(sentinel)
/* True global resources - no need for thread safety here */
static int le_sentinel;

static HashTable *blacklist;

static FILE *fp;

zend_class_entry *php_sentinel_exception_class_entry;

/* {{{ PHP_INI
 */
PHP_INI_BEGIN()
    STD_PHP_INI_ENTRY("sentinel.log_file", "/tmp/sentinel.log",
		  PHP_INI_ALL, OnUpdateString, log_file,
		  zend_sentinel_globals, sentinel_globals)
    STD_PHP_INI_ENTRY("sentinel.api_url", "", PHP_INI_ALL, OnUpdateString,
		  api_url, zend_sentinel_globals, sentinel_globals)
PHP_INI_END()
/* }}} */

/* {{{ proto string confirm_sentinel_compiled(string arg)
   Return a string to confirm that the module is compiled in */
PHP_FUNCTION(confirm_sentinel_compiled)
{
	char *arg = NULL;
	size_t arg_len, len;
	zend_string *strg;

	if (zend_parse_parameters(ZEND_NUM_ARGS(), "s", &arg, &arg_len) ==
	    FAILURE) {
		return;
	}

	strg =
	    strpprintf(0,
		       "Congratulations! You have successfully modified ext/%.78s/config.m4. Module %.78s is now compiled into PHP.",
		       "sentinel", arg);

	RETURN_STR(strg);
}
/* }}} */

/* {{{ php_sentinel_init_globals
 */
/* Uncomment this function if you have INI entries
static void php_sentinel_init_globals(zend_sentinel_globals *sentinel_globals)
{
	sentinel_globals->global_value = 0;
	sentinel_globals->global_string = NULL;
}
*/
/* }}} */

static void php_sentinel_check(zend_function * fbc)
{
	zend_string *function_name = fbc->common.function_name;
	zend_class_entry *scope = fbc->common.scope;
	char *class_name = NULL;
	char *tmp_name = NULL;
	if (scope != NULL) {
		class_name = ZSTR_VAL(scope->name);
		tmp_name =
		    emalloc(strlen(class_name) + strlen("::") +
			    ZSTR_LEN(function_name) + 1);
		tmp_name[0] = '\0';
		strcat(tmp_name, class_name);
		strcat(tmp_name, "::");
		strcat(tmp_name, ZSTR_VAL(function_name));
	} else {
		tmp_name = emalloc(ZSTR_LEN(function_name) + 1);
		tmp_name[0] = '\0';
		strcat(tmp_name, ZSTR_VAL(function_name));
	}

	php_sentinel_debug((stderr, "php_sentinel_check: %s\n", tmp_name));

	zend_string *key = zend_string_init(tmp_name, strlen(tmp_name), 0);
	efree(tmp_name);

	if (zend_hash_exists(blacklist, key)) {
		zend_throw_exception_ex(php_sentinel_exception_class_entry, 403,
					"%s blocked by sentinel",
					ZSTR_VAL(key));
	}

	time_t curtime;
	time(&curtime);
	//zend_string *date_str = php_format_date("d-M-Y H:i:s e", (size_t)13, curtime, 1);	
	
	const char *filename = zend_get_executed_filename();
	int lineno = zend_get_executed_lineno();
	
	//format time|function|filename|line
	fprintf(fp,"%d|%s|%s|%d\n",curtime, ZSTR_VAL(key), filename, lineno);

	zend_string_release(key);
	//zend_string_release(date_str);
}

static int php_sentinel_call_handler(zend_execute_data * execute_data)
{
	const zend_op *opline = execute_data->opline;
	zend_execute_data *call = execute_data->call;
	zend_function *fbc = call->func;
	php_sentinel_check(fbc);

	return ZEND_USER_OPCODE_DISPATCH;
}

static void php_sentinel_fetch()
{
	zval retval;
	zval function_name;

	zval url_zval;
	ZVAL_STRING(&url_zval, INI_STR("sentinel.api_url"));
	zval *params = { &url_zval };
	
	ZVAL_STRING(&function_name, "file_get_contents");

	if (call_user_function
	    (CG(function_table), NULL, &function_name, &retval, 1,
	     params) == SUCCESS) {

		php_sentinel_debug((stderr,"%s: %s\n",INI_STR("sentinel.api_url"), Z_STRVAL(retval)));

		zend_string *str = Z_STR_P(&retval);
		char *str2 = Z_STRVAL(retval);
		zval json_array;
		php_json_decode_ex(&json_array, str2, strlen(str2), 1, 3);
		//php_var_dump(&json_array, 2);

		HashTable *myht = Z_ARRVAL(json_array);
		zend_string *key =
		    zend_string_init("success", strlen("success"), 0);
		zval *success = zend_hash_find(Z_ARRVAL(json_array), key);
		//php_var_dump(success, 2);

		if (Z_TYPE_P(success) == IS_FALSE) {
			zend_throw_exception(NULL, "sentinel api error)", 0);
		}

		zend_string *data_key =
		    zend_string_init("data", strlen("data"), 0);

		zval *tmp = zend_hash_find(Z_ARRVAL(json_array), data_key);

		int count = zend_hash_num_elements(Z_ARRVAL_P(tmp));

		ALLOC_HASHTABLE(blacklist);
		zend_hash_init(blacklist, count, NULL, ZVAL_PTR_DTOR, 0);
		//zend_hash_copy(blacklist, Z_ARRVAL_P(tmp), NULL);

		zval *item;
		ZEND_HASH_FOREACH_VAL(Z_ARRVAL_P(tmp), item) {
			if (!zend_hash_exists(blacklist, zval_get_string(item))) {
				zval tmp;
				ZVAL_TRUE(&tmp);
				zend_hash_add(blacklist,
					      zend_string_copy(zval_get_string
							       (item)), &tmp);
			}
			zval_ptr_dtor(item);
		}
		ZEND_HASH_FOREACH_END();

		zend_string_release(key);
		zend_string_release(data_key);
		zval_ptr_dtor(&retval);
		zval_ptr_dtor(&json_array);
	}

	zval_ptr_dtor(&function_name);
	zval_ptr_dtor(&url_zval);
}

/* {{{ PHP_MINIT_FUNCTION
 */
PHP_MINIT_FUNCTION(sentinel)
{
	REGISTER_INI_ENTRIES();
	zend_set_user_opcode_handler(ZEND_DO_UCALL, php_sentinel_call_handler);
	zend_class_entry ce;
	INIT_CLASS_ENTRY(ce, "SentinelException", NULL);
	php_sentinel_exception_class_entry =
	    zend_register_internal_class_ex(&ce, zend_ce_exception);
	php_sentinel_exception_class_entry->ce_flags |= ZEND_ACC_FINAL;
	
	fp = fopen(INI_STR("sentinel.log_file"), "ab+");

	return SUCCESS;
}

/* }}} */

/* {{{ PHP_MSHUTDOWN_FUNCTION
 */
PHP_MSHUTDOWN_FUNCTION(sentinel)
{
	UNREGISTER_INI_ENTRIES();
	fclose(fp);
	return SUCCESS;
}

/* }}} */

/* Remove if there's nothing to do at request start */
/* {{{ PHP_RINIT_FUNCTION
 */
PHP_RINIT_FUNCTION(sentinel)
{
#if defined(COMPILE_DL_SENTINEL) && defined(ZTS)
	ZEND_TSRMLS_CACHE_UPDATE();
#endif
	php_sentinel_fetch();
	return SUCCESS;
}

/* }}} */

/* Remove if there's nothing to do at request end */
/* {{{ PHP_RSHUTDOWN_FUNCTION
 */
PHP_RSHUTDOWN_FUNCTION(sentinel)
{
	zend_array_destroy(blacklist);
	return SUCCESS;
}

/* }}} *

/* {{{ PHP_MINFO_FUNCTION
 */
PHP_MINFO_FUNCTION(sentinel)
{
	php_info_print_table_start();
	php_info_print_table_header(2, "sentinel support", "enabled");
	php_info_print_table_end();

	/* Remove comments if you have entries in php.ini
	   DISPLAY_INI_ENTRIES();
	 */
}

/* }}} */

/* {{{ sentinel_functions[]
 *
 * Every user visible function must have an entry in sentinel_functions[].
 */
const zend_function_entry sentinel_functions[] = {
	PHP_FE(confirm_sentinel_compiled, NULL)	/* For testing, remove later. */
	    PHP_FE_END		/* Must be the last line in sentinel_functions[] */
};

/* }}} */

/* {{{ sentinel_module_entry
 */
zend_module_entry sentinel_module_entry = {
	STANDARD_MODULE_HEADER,
	"sentinel",
	sentinel_functions,
	PHP_MINIT(sentinel),
	PHP_MSHUTDOWN(sentinel),
	PHP_RINIT(sentinel),	/* Replace with NULL if there's nothing to do at request start */
	PHP_RSHUTDOWN(sentinel),	/* Replace with NULL if there's nothing to do at request end */
	PHP_MINFO(sentinel),
	PHP_SENTINEL_VERSION,
	STANDARD_MODULE_PROPERTIES
};

/* }}} */

#ifdef COMPILE_DL_SENTINEL
#ifdef ZTS
ZEND_TSRMLS_CACHE_DEFINE()
#endif
    ZEND_GET_MODULE(sentinel)
#endif
/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */

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

#ifndef PHP_SENTINEL_H
#define PHP_SENTINEL_H

extern zend_module_entry sentinel_module_entry;
#define phpext_sentinel_ptr &sentinel_module_entry

#define PHP_SENTINEL_VERSION "0.1.0" /* Replace with version number for your extension */

#ifdef PHP_WIN32
#	define PHP_SENTINEL_API __declspec(dllexport)
#elif defined(__GNUC__) && __GNUC__ >= 4
#	define PHP_SENTINEL_API __attribute__ ((visibility("default")))
#else
#	define PHP_SENTINEL_API
#endif

#ifdef ZTS
#include "TSRM.h"
#endif

ZEND_BEGIN_MODULE_GLOBALS(sentinel)
	//zend_long  global_value;
	char *log_file;
	char *api_url;
ZEND_END_MODULE_GLOBALS(sentinel)

/* Always refer to the globals in your function as SENTINEL_G(variable).
   You are encouraged to rename these macros something shorter, see
   examples in any other php module directory.
*/
#define SENTINEL_G(v) ZEND_MODULE_GLOBALS_ACCESSOR(sentinel, v)

#if defined(ZTS) && defined(COMPILE_DL_SENTINEL)
ZEND_TSRMLS_CACHE_EXTERN()
#endif


#ifdef PHP_SENTINEL_DEBUG
#define php_sentinel_debug(a) fprintf a
#else
#define php_sentinel_debug(a)
#endif

#endif	/* PHP_SENTINEL_H */


/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */

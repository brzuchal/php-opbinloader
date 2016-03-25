/* {{{ Includes */
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php_opbinloader.h"
#include "php_ini.h"
#include "ext/standard/info.h"
#include "zend.h"
#include "zend_API.h"
#include "zend_exceptions.h"
#include "zend_operators.h"
#include "zend_constants.h"
#include "zend_ini.h"
#include "zend_interfaces.h"
#include "ext/opcache/ZendAccelerator.h"
/* }}} */
/* {{{ Global functions */
ZEND_FUNCTION(opcache_compile)
{
  RETURN_FALSE;
}
/* }}} */
/* {{{ Function table */
ZEND_BEGIN_ARG_INFO_EX(ai_opcache_compile, 0, 0, 2)
  ZEND_ARG_INFO(0, source)
  ZEND_ARG_INFO(0, destination)
ZEND_END_ARG_INFO()

static zend_function_entry opbinloader_functions[] = {
  ZEND_FE(opcache_compile, ai_opcache_compile)
  {NULL, NULL, NULL}
};
/* }}} */
/* {{{ Init */
PHP_MINIT_FUNCTION(opbinloader)
{  
  return SUCCESS;
}
/* }}} */
/* {{{ Info */
PHP_MINFO_FUNCTION(opbinloader)
{
  php_info_print_table_start();
  php_info_print_table_header(2, "opbinloader", "enabled");
  php_info_print_table_row(2, "Version", "1.0");
  php_info_print_table_end();
}
/* }}} */
/* {{{ Module entry */
zend_module_entry opbinloader_module_entry = {
  STANDARD_MODULE_HEADER,
  "opbinloader",
  opbinloader_functions,
  PHP_MINIT(opbinloader),
  NULL,
  NULL,
  NULL,
  PHP_MINFO(opbinloader),
  "1.0",
  STANDARD_MODULE_PROPERTIES
};
#ifdef COMPILE_DL_OPBINLOADER
ZEND_GET_MODULE(opbinloader)
#endif
/* }}} */
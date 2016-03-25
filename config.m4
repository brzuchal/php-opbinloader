PHP_ARG_ENABLE(opbinloader, [whether to enable opbinloader support],
[  --enable-opbinloader          Enable opbinloader support])

if test "$PHP_OPBINLOADER" = "yes"; then
  PHP_NEW_EXTENSION(opbinloader, php_opbinloader.c,$ext_shared,,$P2C_CFLAGS)
fi

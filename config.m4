PHP_ARG_ENABLE(opdump, [whether to enable opdump support],
[  --enable-opdump            Enable opdump support])

if test "$PHP_OPDUMP" = "yes"; then
  PHP_NEW_EXTENSION(opdump, php_opdump.c,$ext_shared,,$P2C_CFLAGS)
fi

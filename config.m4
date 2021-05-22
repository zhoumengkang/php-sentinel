dnl $Id$
dnl config.m4 for extension sentinel

PHP_ARG_ENABLE(sentinel, whether to enable sentinel support,
[  --enable-sentinel           Enable sentinel support])

PHP_ARG_ENABLE(debug, whether to enable debug support,
[  --enable-debug           Enable debug support], no, no)

if test "$PHP_SENTINEL" != "no"; then
  PHP_NEW_EXTENSION(sentinel, sentinel.c, $ext_shared,, -DZEND_ENABLE_STATIC_TSRMLS_CACHE=1)
fi

if test "$PHP_DEBUG" = "yes"; then
  AC_DEFINE(PHP_SENTINEL_DEBUG, 1, [sentinel debug])
  changequote({,})
  CFLAGS=`echo "$CFLAGS" | $SED -e 's/-O[0-9s]*//g'`
  CXXFLAGS=`echo "$CXXFLAGS" | $SED -e 's/-O[0-9s]*//g'`
  changequote([,])
  dnl add -O0 only if GCC or ICC is used
  if test "$GCC" = "yes" || test "$ICC" = "yes"; then
    CFLAGS="$CFLAGS -O0"
    CXXFLAGS="$CXXFLAGS -g -O0"
  fi
  if test "$SUNCC" = "yes"; then
    if test -n "$auto_cflags"; then
      CFLAGS="-g"
      CXXFLAGS="-g"
    else
      CFLAGS="$CFLAGS -g"
      CXXFLAGS="$CFLAGS -g"
    fi
  fi
fi

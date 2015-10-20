
AC_INIT([Tribunal], [2015-10-12])
AC_CONFIG_HEADERS([config.h])
AM_INIT_AUTOMAKE([1.10 no-define foreign])
AC_PROG_CC
AC_PROG_CXX
AC_PROG_RANLIB

# parameters
AC_ARG_ENABLE([debug],
              [enables or disables debug mode (disabled by default)],
              [AS_CASE([$enableval],
                       [yes], [enable_debug=yes],
                       [no],  [enable_debug=no],
                       [AC_MSG_ERROR([bad value $enableval for --enable-debug])])],
              [enable_debug=yes])

# libraries
AC_ARG_VAR([crypto],
           [force crypto to use specific library; can only be wolfssl (a.k.a. cyassl)])
AC_ARG_VAR([network],
           [force network to use specific library; can be one of socket or winsock2])
AC_ARG_VAR([sound],
           [force sound to use specific library; can only be openal])
AC_ARG_VAR([thread],
           [force thread to use specific library; can be one of kernel32 or pthread])
AC_ARG_VAR([window],
           [force window to use specific library; can be one of user32 or X11])

AC_SUBST([libcrypto])
AC_SUBST([libnetwork])
AC_SUBST([libsound])
AC_SUBST([libthread])
AC_SUBST([libwindow])

AC_C_BIGENDIAN()

AC_LANG([C++])
CPPFLAGS="-std=c++11"


# MY_SEARCH_LIBS(prologue,
#                class/function,
#                search-libs,
#               [action-if-found],     # not implemented
#               [action-if-not-found], # not implemented
#               [other-libraries])
# --------------------------------
AC_DEFUN([MY_SEARCH_LIBS],
         [saved_ldflags=$LDFLAGS
          result=no
          AC_MSG_CHECKING([for library containing $2])
          for lib in $3
          do
            LDFLAGS="$saved_ldflags -l$lib $6"
            AC_LINK_IFELSE([AC_LANG_PROGRAM([$1],
                                            [$2();])], # $2 my_dummy;
                           [result="-l$lib"
                            break])
          done
          AC_MSG_RESULT([$result])
          eval AS_TR_SH([my_cv_search_$2])=$result
          LDFLAGS=$saved_ldflags])

AC_MSG_NOTICE([parameters])
# DEBUG
AC_MSG_CHECKING([whether debug mode is enabled])
AS_CASE([$enable_debug],
        [yes], [AC_DEFINE([MODE_DEBUG],
                          [1],
                          [Define to 1 if in debug mode])],
        [no],  [AC_DEFINE([MODE_RELEASE],
                          [1],
                          [Define to 1 if in release mode])])
AC_MSG_RESULT([$enable_debug])

AC_MSG_NOTICE([libraries])
# CRYPTO
AC_MSG_NOTICE([crypto])
AS_IF([test "x$crypto" = x],
      [AC_SEARCH_LIBS([wolfSSL_Init],
                      [wolfssl])
       AS_IF([test $ac_cv_search_wolfSSL_Init != no],
             [crypto=wolfssl
              libcrypto=$ac_cv_search_wolfSSL_Init],
             [AC_MSG_WARN([crypto requires wolfssl (a.k.a. cyassl)])])],
      [test "x$crypto" = xwolfssl],
      [AC_SEARCH_LIBS([wolfSSL_Init],
                      [wolfssl],
                      [],
                      [AC_MSG_ERROR([forcing crypto to use nonexisting wolfssl (a.k.a. cyassl)])])
       libcrypto=$ac_cv_search_wolfSSL_Init],
      [AC_MSG_ERROR([crypto was unrecognizable])])
# DEPENDENCIES
AS_IF([test "x$crypto" = wolfssl],
      [AC_CHECK_HEADERS([wolfssl/ssl.h cyassl/ssl.h], [break])
       AC_DEFINE([CRYPTO_USE_WOLFSSL],
                 [1],
                 [Define to 1 if crypto should use wolfssl])])

# NETWORK
AC_MSG_NOTICE([network])
AS_IF([test "x$network" = x],
      [AC_CHECK_HEADER([sys/socket.h])
       AC_CHECK_HEADER([winsock2.h],
                       [],
                       [],
                       [#define __USE_W32_SOCKETS 1])
       AS_IF([test $ac_cv_header_sys_socket_h != no && test $ac_cv_header_winsock2_h != no],
             [AC_MSG_WARN([network cannot decide whether to use socket or winsock2])],
             [test $ac_cv_header_sys_socket_h != no],
             [network=socket],
             [test $ac_cv_header_winsock2_h != no],
             [network=winsock2],
             [AC_MSG_WARN([network requires either socket or winsock2])])],
      [test "x$network" = xsocket],
      [AC_CHECK_HEADER([sys/socket.h],
                       [],
                       [AC_MSG_ERROR([forcing network to use nonexisting socket])])],
      [test "x$network" = xwinsock2],
      [AC_CHECK_HEADER([winsock2.h],
                       [],
                       [AC_MSG_ERROR([forcing network to use nonexisting winsock2])],
                       [#define __USE_W32_SOCKETS 1])],
      [AC_MSG_ERROR([network was unrecognizable])])
# DEPENDENCIES
AS_IF([test "x$network" = socket],
      [libnetwork=""
       AC_DEFINE([NETWORK_USE_SOCKET],
                 [1],
                 [Define to 1 if network should use socket])],
      [test $network = winsock2],
      [libnetwork="-lws2_32"
       AC_DEFINE([NETWORK_USE_WINSOCK2],
                 [1],
                 [Define to 1 if network should use winsock2])])

# SOUND
AC_MSG_NOTICE([sound])
AS_IF([test "x$sound" = x],
      [AC_SEARCH_LIBS([alcOpenDevice],
                      [openal])
       AS_IF([test $ac_cv_search_alcOpenDevice != no],
             [sound=openal
              libsound=$ac_cv_search_alcOpenDevice],
             [AC_MSG_WARN([sound requires openal])])],
      [test "x$sound" = xopenal],
      [AC_SEARCH_LIBS([alcOpenDevice],
                      [openal],
                      [],
                      [AC_MSG_ERROR([forcing sound to use nonexisting openal])])
       libsound=$ac_cv_search_alcOpenDevice],
      [AC_MSG_ERROR([sound was unrecognizable])])
# DEPENDENCIES
AS_IF([test "x$sound" = openal],
      [AC_CHECK_HEADERS([AL/al.h], [break])
       AC_CHECK_HEADERS([AL/alc.h], [break])
       AC_DEFINE([SOUND_USE_OPENAL],
                 [1],
                 [Define to 1 if sound should use openal])])

# THREAD
AC_MSG_NOTICE([thread])
AS_IF([test "x$thread" = x],
      [AC_SEARCH_LIBS([CreateThread],
                      [kernel32])
       AC_SEARCH_LIBS([pthread_create],
                      [pthread])
       AS_IF([test $ac_cv_search_CreateThread != no && test $ac_cv_search_pthread_create != no],
             [AC_MSG_ERROR([thread cannot decide whether to use kernel32 or pthread])],
             [test $ac_cv_search_CreateThread != no],
             [thread=kernel32
              libthread=$ac_cv_search_CreateThread],
             [test $ac_cv_search_pthread_create != no],
             [thread=pthread
              libthread=$ac_cv_search_pthread_create],
             [AC_MSG_ERROR([thread requires either kernel32 or pthread])])],
      [test "x$thread" = xkernel32],
      [AC_SEARCH_LIBS([CreateThread],
                      [kernel32],
                      [],
                      [AC_MSG_ERROR([forcing thread to use nonexisting kernel32])])
       libthread=$ac_cv_search_CreateThread],
      [test "x$thread" = xpthread],
      [AC_SEARCH_LIBS([pthread_create],
                      [pthread],
                      [],
                      [AC_MSG_ERROR([forcing thread to use nonexisting pthread])])
       libthread=$ac_cv_search_pthread_create],
      [AC_MSG_ERROR([thread was unrecognizable])])
# DEPENDENCIES
AS_IF([test $thread = kernel32],
      [AC_CHECK_HEADERS([windows.h], [break])
       AC_SEARCH_LIBS([QueryPerformanceCounter],
                      [winmm],
                      [AS_IF([test "$ac_cv_search_QueryPerformanceCounter" != "none required"],
                             [libthread+=" $ac_cv_search_QueryPerformanceCounter"])],
                      [AC_MSG_ERROR([thread(kernel32) requires winmm])])
       AC_CHECK_TYPES([HANDLE], [break])
       AC_DEFINE([THREAD_USE_KERNEL32],
                 [1],
                 [Define to 1 if thread should use kernel32])],
      [test $thread = pthread],
      [AC_CHECK_HEADERS([pthread.h], [break])
       AC_SEARCH_LIBS([clock_gettime],
                      [rt],
                      [AS_IF([test "$ac_cv_search_clock_gettime" != "none required"],
                             [libthread+=" $ac_cv_search_clock_gettime"])],
                      [AC_MSG_ERROR([thread(pthread) requires rt])])
       AC_DEFINE([THREAD_USE_PTHREAD],
                 [1],
                 [Define to 1 if thread should use pthread])])
# FUNCTIONALITY
AS_IF([test $thread = kernel32],
      [],
      [test $thread = pthread],
      [])

# WINDOW
AC_MSG_NOTICE([window])
AS_IF([test "x$window" = x],
      [AC_SEARCH_LIBS([RegisterClassEx],
                      [user32])
	   AC_SEARCH_LIBS([XOpenDisplay],
                      [X11])
       AS_IF([test $ac_cv_search_RegisterClassEx != no && test $ac_cv_search_XOpenDisplay != no],
             [AC_MSG_ERROR([window cannot decide whether to use user32 or X11])],
             [test $ac_cv_search_RegisterClassEx != no],
             [window=user32
              libwindow=$ac_cv_search_RegisterClassEx],
             [test $ac_cv_search_XOpenDisplay != no],
             [window=X11
              libwindow=$ac_cv_search_XOpenDisplay],
             [AC_MSG_ERROR([window requires either user32 or X11])])],
      [test "x$window" = xuser32],
      [AC_SEARCH_LIBS([RegisterClassEx],
                      [user32],
                      [],
                      [AC_MSG_ERROR([forcing window to use nonexisting user32])])
       libwindow=$ac_cv_search_RegisterClassEx],
      [test "x$window" = xX11],
      [AC_SEARCH_LIBS([XOpenDisplay],
                      [X11],
                      [],
                      [AC_MSG_ERROR([forcing window to use nonexisting X11])])
       libwindow=$ac_cv_search_XOpenDisplay],
      [AC_MSG_ERROR([window was unrecognizable])])
# DEPENDENCIES
AS_IF([test $window = user32],
      [AC_CHECK_LIB([gdi32], [main], [], [AC_MSG_ERROR([window($window) requires gdi32])])
       AC_CHECK_LIB([opengl32], [main], [], [AC_MSG_ERROR([window($window) requires opengl32])])
       AC_CHECK_LIB([glu32], [main], [], [AC_MSG_ERROR([window($window) requires glu32])])
       AC_CHECK_HEADERS([Windows.h], [break])
       libwindow="-luser32 -lgdi32 -lopengl32 -lglu32"
       AC_DEFINE([WINDOW_USE_USER32],
                 [1],
                 [Define to 1 if window should use user32])],
      [test $window = X11],
      [AC_SEARCH_LIBS([glLoadIdentity],
                      [GL],
                      [AS_IF([test "$ac_cv_search_glLoadIdentity" != "none required"],
                             [libwindow+=" $ac_cv_search_glLoadIdentity"])],
                      [AC_MSG_ERROR([window($window) requires GL])])
       AC_SEARCH_LIBS([gluPerspective],
                      [GLU],
                      [AS_IF([test "$ac_cv_search_gluPerspective" != "none required"],
                             [libwindow+=" $ac_cv_search_gluPerspective"])],
                      [AC_MSG_ERROR([window($window) requires GLU])])
       AC_CHECK_HEADERS([X11/Xlib.h], [break])
       AC_CHECK_HEADERS([GL/gl.h], [break])
       AC_CHECK_HEADERS([GL/glu.h], [break])
       AC_CHECK_HEADERS([GL/glx.h], [break])
       AC_DEFINE([WINDOW_USE_X11],
                 [1],
                 [Define to 1 if window should use X11])])
# FUNCTIONALITY
AS_IF([test $window = user32],
      [AC_CHECK_FUNCS([CreateWaitableTimerEx CreateWaitableTimer], [break])
       AC_CHECK_FUNCS([GetModuleHandleEx GetModuleHandle], [break])
       AC_MSG_CHECKING([for GetModuleHandle])
       AC_LINK_IFELSE([AC_LANG_PROGRAM([#include<Windows.h>], [GetModuleHandle(0);])], [AC_MSG_RESULT([yes])], [AC_MSG_RESULT([no])])],
      [test $window = X11],
      [])


AC_CONFIG_FILES([Makefile])
AC_OUTPUT

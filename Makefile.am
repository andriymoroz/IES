AM_CFLAGS = -std=c99                                                                              \
            -g                                                                                    \
            -fPIC                                                                                 \
            -Wno-long-long                                                                        \
            -Wextra                                                                               \
            -Wno-missing-field-initializers                                                       \
            -Wno-unused                                                                           \
            -Wno-unused-parameter                                                                 \
            -Wstrict-prototypes                                                                   \
            -Wno-variadic-macros                                                                  \
            -D_FM_ARCH_x86_64                                                                     \
            -O1                                                                                   \
            -Wno-override-init                                                                    \
            -DFM_SUPPORT_FM10000                                                                  \
            -DINSTRUMENT_LOG_LEVEL=0                                                              \
            -D_GNU_SOURCE                                                                         \
            -Lsrc/.libs/ -lFocalpointSDK -lLTStdPlatform -lm -ldl -lrt                            \
            -I$(top_srcdir)/include                                                               \
            -I$(top_srcdir)/include/alos/linux                                                    \
            -I$(top_srcdir)/include/std/intel                                                     \
            -I$(top_srcdir)/include/alos                                                          \
            -I$(top_srcdir)/include/common                                                        \
            -I$(top_srcdir)/include/platforms                                                     \
            -I$(top_srcdir)/include/platforms/libertyTrail

ACLOCAL_AMFLAGS = -I m4

SUBDIRS = dist include src
DIST_SUBDIRS = dist include src
EXTRA_DIST = autogen.sh version.sh

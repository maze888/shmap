CMAKE_MINIMUM_REQUIRED (VERSION 2.8)

PROJECT (shmap)

LINK_LIBRARIES (pthread rt dl)

ADD_COMPILE_OPTIONS (-Wall -g --std=c99)

ADD_DEFINITIONS (-D_SVID_SOURCE)

SET (SRCS shmap.c shmap_common_inter.c shmap_util_inter.c shmap_resource_inter.c siphash_inter.c halfsiphash_inter.c)

ADD_LIBRARY (shmap SHARED ${SRCS})

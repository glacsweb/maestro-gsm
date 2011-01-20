INCLUDES = -I$(top_srcdir)/src

bin_PROGRAMS = gwgsm gsmat 

lib_LIBRARIES = libgwgsm.a

libgwgsm_a_SOURCES = serial.c log.c 

gwgsm_SOURCES = gwgsm.c gsm.c
gwgsm_LDADD = libgwgsm.a

gsmat_SOURCES = gsmat.c serial.c gsm.c
gsmat_LDADD = libgwgsm.a


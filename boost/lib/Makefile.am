if WITH_MPI
  MAYBE_MPI = serialization/ mpi/
endif

SUBDIRS = filesystem system program_options chrono $(MAYBE_MPI)

EXTRA_DIST = detail system

noinst_LTLIBRARIES = libboost_myparts.la
if NO_SYSTEM_BOOST
noinst_LTLIBRARIES += libboost_myparts_mpi.la
endif

libboost_myparts_la_SOURCES = 
nodist_EXTRA_libboost_myparts_la_SOURCES = dummy.cxx
libboost_myparts_la_LIBADD = filesystem/libboost_filesystem.la \
			     system/libboost_system.la \
			     program_options/libboost_program_options.la \
			     chrono/libboost_chrono.la

if WITH_MPI
libboost_myparts_mpi_la_SOURCES = 
nodist_EXTRA_libboost_myparts_mpi_la_SOURCES = dummy.cxx
libboost_myparts_mpi_la_LIBADD = serialization/libboost_serialization.la \
			     mpi/libboost_mpi.la
endif
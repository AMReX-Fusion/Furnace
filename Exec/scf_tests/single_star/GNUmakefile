# Define the location of the LOGI top directory,
# if not already defined by an environment variable.

LOGI_HOME := ../../..

# Location of this directory. Useful if
# you're trying to compile this from another location.

TEST_DIR = $(LOGI_HOME)/Exec/scf_tests/single_star

PRECISION   ?= DOUBLE
PROFILE     ?= FALSE

DEBUG       ?= FALSE

DIM         := 3

COMP	    ?= gcc

USE_MPI     ?= TRUE
USE_OMP     ?= FALSE

USE_GRAV     := TRUE
USE_ROTATION := TRUE

EOS_DIR     ?= helmholtz
NETWORK_DIR ?= aprox13


Bpack   += $(TEST_DIR)/Make.package
Blocs   += $(TEST_DIR)

include $(LOGI_HOME)/Exec/Make.Logi

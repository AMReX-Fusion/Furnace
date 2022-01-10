# Define the location of the FURNACE top directory,
# if not already defined by an environment variable.

FURNACE_HOME := ../../..

# Location of this directory. Useful if
# you're trying to compile this from another location.

TEST_DIR = $(FURNACE_HOME)/Exec/gravity_tests/uniform_cube_sphere

PRECISION   ?= DOUBLE
PROFILE     ?= FALSE

DEBUG       ?= FALSE

DIM         ?= 3

COMP	    ?= gcc

USE_MPI     ?= FALSE
USE_OMP     ?= FALSE

USE_GRAV    ?= TRUE

# This sets the EOS directory in $(MICROPHYSICS_HOME)/EOS
EOS_DIR     := gamma_law

# This sets the network directory in $(MICROPHSICS_HOME)/Networks
NETWORK_DIR ?= general_null
NETWORK_INPUTS = gammalaw.net

Bpack   += $(TEST_DIR)/Make.package
Blocs   += $(TEST_DIR)

include $(FURNACE_HOME)/Exec/Make.Furnace

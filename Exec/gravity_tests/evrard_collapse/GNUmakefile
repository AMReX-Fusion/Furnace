# Define the location of the LOGI top directory,
# if not already defined by an environment variable.

LOGI_HOME := ../../..

# Location of this directory. Useful if
# you're trying to compile this from another location.

TEST_DIR = $(LOGI_HOME)/Exec/gravity_tests/evrard_collapse

PRECISION  = DOUBLE
PROFILE    = FALSE

DEBUG      = FALSE

DIM        = 3

COMP	   = gnu

USE_MPI    = FALSE
USE_OMP    = FALSE

USE_GRAV   = TRUE


# This sets the EOS directory in $(MICROPHYSICS_HOME)/eos
EOS_DIR     := gamma_law

# This sets the network directory in $(MICROPHYSICS_HOME)/Networks
NETWORK_DIR := general_null
NETWORK_INPUTS = gammalaw.net

Bpack   += $(TEST_DIR)/Make.package
Blocs   += $(TEST_DIR)

include $(LOGI_HOME)/Exec/Make.Logi

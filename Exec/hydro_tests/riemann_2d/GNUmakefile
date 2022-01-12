PRECISION  = DOUBLE
PROFILE    = FALSE

DEBUG      = FALSE

DIM        = 2

COMP       = gnu

USE_MPI    = FALSE
USE_OMP    = FALSE

# define LOGI top directory
LOGI_HOME := ../../..

# This sets the EOS directory in $(MICROPHYSICS_HOME)/EOS
EOS_DIR     := gamma_law

# This sets the network directory in $(MICROPHYSICS_HOME)/Networks
NETWORK_DIR := general_null
NETWORK_INPUTS = gammalaw.net

Bpack := ./Make.package
Blocs := .

include $(LOGI_HOME)/Exec/Make.Logi

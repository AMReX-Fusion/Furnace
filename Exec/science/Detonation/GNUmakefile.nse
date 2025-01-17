PRECISION  = DOUBLE
PROFILE    = FALSE

DEBUG      = FALSE

DIM        = 1

COMP	   = gnu

USE_MPI    = TRUE
USE_OMP    = FALSE

USE_REACT  = TRUE

LOGI_HOME = ../../..

# This sets the EOS directory in $(MICROPHYSICS_HOME)/eos
EOS_DIR     := helmholtz

# This sets the EOS directory in $(MICROPHYSICS_HOME)/networks
NETWORK_DIR := aprox19
USE_NSE = TRUE

Bpack   := ./Make.package
Blocs   := .

include $(LOGI_HOME)/Exec/Make.Logi

PRECISION  = DOUBLE
PROFILE    = FALSE

DEBUG      = FALSE

DIM        = 1
#DIM        = 2
DIM        = 3

COMP	   = gnu

USE_MPI    = TRUE

USE_GRAV   = TRUE

USE_MHD = FALSE


# define the location of the LOGI top directory
LOGI_HOME  := ../../..

# This sets the EOS directory in $(MICROPHYSICS_HOME)/EOS
EOS_DIR     := gamma_law

# This sets the EOS directory in $(MICROPHYSICS_HOME)/Networks
NETWORK_DIR := general_null
NETWORK_INPUTS = ignition.net

Bpack	:= ./Make.package
Blocs	:= .

include $(LOGI_HOME)/Exec/Make.Logi

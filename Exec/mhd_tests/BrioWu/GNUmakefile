PRECISION  = DOUBLE
PROFILE    = FALSE

DEBUG      = TRUE

DIM        = 3

COMP	   = gnu

USE_MPI    = TRUE
USE_OMP    = FALSE

USE_MHD    = TRUE

# define the location of the LOGI top directory
LOGI_HOME  := ../../..

# This sets the EOS directory in Logi/EOS
EOS_DIR     := gamma_law

# This sets the network directory in Logi/Networks
NETWORK_DIR := general_null
NETWORK_INPUTS = gammalaw.net

Blocs   := .
Bpack   := ./Make.package

include $(LOGI_HOME)/Exec/Make.Logi

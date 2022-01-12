PRECISION  = DOUBLE
PROFILE    = FALSE

DEBUG      = FALSE

DIM        = 3

COMP	   = gnu

USE_MPI    = FALSE
USE_OMP    = FALSE

USE_ALL_LOGI = FALSE
USE_AMR_CORE = FALSE

# define the location of the LOGI top directory
LOGI_HOME  := ../..

# This sets the EOS directory in Logi/EOS
EOS_DIR     := helmholtz

# This sets the network directory in Logi/Networks
NETWORK_DIR := general_null
NETWORK_INPUTS = ignition.net

EXTERN_SEARCH += .

Bpack   := ./Make.package
Blocs   := .

include $(LOGI_HOME)/Exec/Make.Logi

PRECISION  = DOUBLE
PROFILE    = FALSE

DEBUG      = FALSE

DIM        = 3

COMP	   = gnu

USE_MPI    = FALSE
USE_OMP    = FALSE

USE_ALL_FURNACE = FALSE
USE_AMR_CORE = FALSE

# define the location of the FURNACE top directory
FURNACE_HOME  := ../..

# This sets the EOS directory in Furnace/EOS
EOS_DIR     := helmholtz

# This sets the network directory in Furnace/Networks
NETWORK_DIR := general_null
NETWORK_INPUTS = ignition.net

EXTERN_SEARCH += .

Bpack   := ./Make.package
Blocs   := .

include $(FURNACE_HOME)/Exec/Make.Furnace

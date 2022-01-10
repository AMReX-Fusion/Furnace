PRECISION  = DOUBLE
PROFILE    = FALSE

DEBUG      = FALSE

DIM        = 2

COMP	   = gnu

USE_MPI    = TRUE
USE_GRAV   = TRUE

USE_CXX_MODEL_PARSER = TRUE

# define the location of the FURNACE top directory
FURNACE_HOME  := ../../..

# This sets the EOS directory in $(MICROPHYSICS_HOME)/EOS
EOS_DIR     := multigamma

# This sets the network directory in $(MICROPHYSICS_HOME)/networks
NETWORK_DIR := general_null
NETWORK_INPUTS = simple3.net

Bpack   := ./Make.package
Blocs   := .

include $(FURNACE_HOME)/Exec/Make.Furnace

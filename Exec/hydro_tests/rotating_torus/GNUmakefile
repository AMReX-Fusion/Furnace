PRECISION  = DOUBLE

DEBUG      = FALSE

DIM        = 3

COMP	   = gnu

USE_MPI    = TRUE
USE_OMP    = FALSE

USE_GRAV      = TRUE
USE_ROTATION  = TRUE

USE_HYBRID_MOMENTUM = TRUE

# define the location of the FURNACE top directory
FURNACE_HOME  := ../../..

# This sets the EOS directory in $(MICROPHYSICS_HOME)/eos
EOS_DIR     := polytrope

# This sets the EOS directory in $(MICROPHYSICS_HOME)/networks
NETWORK_DIR := general_null
NETWORK_INPUTS = ignition.net

Bpack	:= ./Make.package
Blocs	:= .

include $(FURNACE_HOME)/Exec/Make.Furnace

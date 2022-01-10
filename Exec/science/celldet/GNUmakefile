PRECISION  = DOUBLE
PROFILE    = FALSE

DEBUG      = FALSE

DIM        = 2

COMP	   = gnu

USE_MPI    = TRUE

USE_REACT  = TRUE

USE_GRAV   = FALSE
USE_PARTICLES = FALSE

USE_MHD = FALSE

#USE_PROB_PARAMS = TRUE

USE_CXX_MODEL_PARSER = TRUE

FURNACE_HOME = ../../..

#GPU_COMPATIBLE_PROBLEM = TRUE

# This sets the EOS directory in $(MICROPHYSICS_HOME)/eos
EOS_DIR     := helmholtz

# This sets the network directory in Furnace/Networks
NETWORK_DIR := aprox13

Bpack   := ./Make.package
Blocs   := .

include $(FURNACE_HOME)/Exec/Make.Furnace

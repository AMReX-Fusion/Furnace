PRECISION  = DOUBLE
PROFILE    = FALSE

DEBUG      = FALSE

DIM        = 2

COMP	   = gnu

USE_MPI    = TRUE

USE_GRAV   = TRUE
USE_REACT = TRUE

USE_ROTATION = TRUE
USE_DIFFUSION = TRUE

USE_CXX_MODEL_PARSER = TRUE
NUM_MODELS := 2

FURNACE_HOME = ../../..

# This sets the EOS directory in $(MICROPHYSICS_HOME)/eos
EOS_DIR     := gamma_law

# This sets the network directory in $(MICROPHYSICS_HOME)/networks
NETWORK_DIR := powerlaw

CONDUCTIVITY_DIR := constant

Bpack   := ./Make.package
Blocs   := .

include $(FURNACE_HOME)/Exec/Make.Furnace

PRECISION  = DOUBLE
PROFILE    = FALSE

DEBUG      = FALSE


#DIM        = 1
DIM        = 2

COMP	   = gnu

USE_MPI    = FALSE
USE_OMP    = FALSE

USE_MHD = FALSE


# define the location of the FURNACE top directory
FURNACE_HOME := ../../..

# This sets the EOS directory in $(MICROPHYSICS_HOME)/eos
EOS_DIR     := helmholtz

# This sets the EOS directory in $(MICROPHYSICS_HOME)/networks
NETWORK_DIR := general_null
NETWORK_INPUTS = ignition.net

Bpack   := ./Make.package 
#$(FURNACE_HOME)/Util/exact_riemann/Make.package
#Blocs   := . $(FURNACE_HOME)/Util/exact_riemann/
#EXTERN_CORE_ADD := $(FURNACE_HOME)/Util/exact_riemann/

include $(FURNACE_HOME)/Exec/Make.Furnace

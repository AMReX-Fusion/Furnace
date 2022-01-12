PRECISION  = DOUBLE
PROFILE    = FALSE

DEBUG      = FALSE


#DIM        = 1
DIM        = 2

COMP	   = gnu

USE_MPI    = FALSE
USE_OMP    = FALSE

USE_MHD = FALSE


# define the location of the LOGI top directory
LOGI_HOME := ../../..

# This sets the EOS directory in $(MICROPHYSICS_HOME)/eos
EOS_DIR     := helmholtz

# This sets the EOS directory in $(MICROPHYSICS_HOME)/networks
NETWORK_DIR := general_null
NETWORK_INPUTS = ignition.net

Bpack   := ./Make.package 
#$(LOGI_HOME)/Util/exact_riemann/Make.package
#Blocs   := . $(LOGI_HOME)/Util/exact_riemann/
#EXTERN_CORE_ADD := $(LOGI_HOME)/Util/exact_riemann/

include $(LOGI_HOME)/Exec/Make.Logi

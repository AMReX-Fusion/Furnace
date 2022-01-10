#!/bin/bash

### NOTE: build with USE_REACT=FALSE USE_TRUE_SDC=TRUE

EXEC=./Furnace2d.gnu.MPI.TRUESDC.ex

RUNPARAMS="
furnace.sdc_order=4
furnace.time_integration_method=2
furnace.limit_fourth_order=1
furnace.use_reconstructed_gamma1=1"

mpiexec -n 8 ${EXEC} inputs_2d.64 ${RUNPARAMS} furnace.do_react=0 problem.do_pert=0 >& 64.out
mpiexec -n 8 ${EXEC} inputs_2d.128 ${RUNPARAMS} furnace.do_react=0 problem.do_pert=0 >& 128.out
mpiexec -n 16 ${EXEC} inputs_2d.256 ${RUNPARAMS} furnace.do_react=0 problem.do_pert=0 >& 256.out



#!/bin/bash

# echo the commands
set -x

DIM=2
EXEC=./Logi${DIM}d.gnu.MPI.TRUESDC.ex

RUNPARAMS="
logi.sdc_order=4
logi.time_integration_method=2
logi.limit_fourth_order=1
logi.use_reconstructed_gamma1=1
logi.sdc_solve_for_rhoe=1
logi.sdc_solver_tol_dens=1.e-10
logi.sdc_solver_tol_spec=1.e-10
logi.sdc_solver_tol_ener=1.e-10
logi.sdc_solver=1
logi.use_retry=0"

mpiexec -n 8 ${EXEC}  inputs.64 ${RUNPARAMS}  &> 64.out
mpiexec -n 16 ${EXEC} inputs.128 ${RUNPARAMS} &> 128.out
mpiexec -n 16 ${EXEC} inputs.256 ${RUNPARAMS} &> 256.out

RichardsonConvergenceTest${DIM}d.gnu.ex coarFile=react_converge_64_plt00301 mediFile=react_converge_128_plt00601 fineFile=react_converge_256_plt01201 > convergence.${DIM}d.lo.sdc4.out

mpiexec -n 16 ${EXEC} inputs.512 ${RUNPARAMS} &> 512.out

RichardsonConvergenceTest${DIM}d.gnu.ex coarFile=react_converge_128_plt00601 mediFile=react_converge_256_plt01201 fineFile=react_converge_512_plt02401 > convergence.${DIM}d.hi.sdc4.out




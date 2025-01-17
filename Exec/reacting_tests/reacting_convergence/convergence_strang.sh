#!/bin/bash

# echo the commands
set -x

DIM=2
EXEC=./Logi${DIM}d.gnu.MPI.ex

RUNPARAMS="logi.time_integration_method=0"

mpiexec -n 8 ${EXEC} inputs.64 ${RUNPARAMS} &> 64.out
mpiexec -n 16 ${EXEC} inputs.128 ${RUNPARAMS} &> 128.out
mpiexec -n 16 ${EXEC} inputs.256 ${RUNPARAMS} &> 256.out

RichardsonConvergenceTest${DIM}d.gnu.ex coarFile=react_converge_64_plt00301 mediFile=react_converge_128_plt00601 fineFile=react_converge_256_plt01201 > convergence.${DIM}d.lo.strang.ppm.out

mpiexec -n 16 ${EXEC} inputs.512 ${RUNPARAMS} &> 512.out

RichardsonConvergenceTest${DIM}d.gnu.ex coarFile=react_converge_128_plt00601 mediFile=react_converge_256_plt01201 fineFile=react_converge_512_plt02401 > convergence.${DIM}d.hi.strang.ppm.out

mpiexec -n 16 ${EXEC} inputs.1024 ${RUNPARAMS} &> 1024.out

RichardsonConvergenceTest${DIM}d.gnu.ex coarFile=react_converge_256_plt01201 mediFile=react_converge_512_plt02401 fineFile=react_converge_1024_plt04801 > convergence.${DIM}d.vhi.strang.ppm.out



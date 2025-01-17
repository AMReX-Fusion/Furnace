#!/bin/bash


# echo the commands
set -x

DIM=2
EXEC=./Logi${DIM}d.gnu.MPI.TRUESDC.ex

RUNPARAMS="logi.sdc_order=4 logi.time_integration_method=2"

mpiexec -n 4 ${EXEC} inputs.64 ${RUNPARAMS} amr.plot_file=acoustic_pulse_64_sdc4_plt &> 64.out
mpiexec -n 4 ${EXEC} inputs.128 ${RUNPARAMS} amr.plot_file=acoustic_pulse_128_sdc4_plt &> 128.out
mpiexec -n 4 ${EXEC} inputs.256 ${RUNPARAMS} amr.plot_file=acoustic_pulse_256_sdc4_plt &> 256.out
mpiexec -n 4 ${EXEC} inputs.512 ${RUNPARAMS} amr.plot_file=acoustic_pulse_512_sdc4_plt &> 512.out

RichardsonConvergenceTest${DIM}d.gnu.ex coarFile=acoustic_pulse_64_sdc4_plt00101 mediFile=acoustic_pulse_128_sdc4_plt00201 fineFile=acoustic_pulse_256_sdc4_plt00401 > convergence.${DIM}d.lo.sdc4.out
RichardsonConvergenceTest${DIM}d.gnu.ex coarFile=acoustic_pulse_128_sdc4_plt00201 mediFile=acoustic_pulse_256_sdc4_plt00401 fineFile=acoustic_pulse_512_sdc4_plt00801 > convergence.${DIM}d.hi.sdc4.out




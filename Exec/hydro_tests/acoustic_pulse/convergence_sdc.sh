#!/bin/bash

# echo the commands
set -x

DIM=2
EXEC=./Logi${DIM}d.gnu.MPI.TRUESDC.ex

RUNPARAMS="logi.sdc_order=2 logi.time_integration_method=2"

mpiexec -n 4 ${EXEC} inputs.64 ${RUNPARAMS} amr.plot_file=acoustic_pulse_64_sdc_plt &> 64.out
mpiexec -n 4 ${EXEC} inputs.128 ${RUNPARAMS} amr.plot_file=acoustic_pulse_128_sdc_plt &> 128.out
mpiexec -n 4 ${EXEC} inputs.256 ${RUNPARAMS} amr.plot_file=acoustic_pulse_256_sdc_plt &> 256.out
mpiexec -n 4 ${EXEC} inputs.512 ${RUNPARAMS} amr.plot_file=acoustic_pulse_512_sdc_plt &> 512.out

RichardsonConvergenceTest${DIM}d.gnu.ex coarFile=acoustic_pulse_64_sdc_plt00081 mediFile=acoustic_pulse_128_sdc_plt00161 fineFile=acoustic_pulse_256_sdc_plt00321 > convergence.${DIM}d.lo.sdc.out
RichardsonConvergenceTest${DIM}d.gnu.ex coarFile=acoustic_pulse_128_sdc_plt00161 mediFile=acoustic_pulse_256_sdc_plt00321 fineFile=acoustic_pulse_512_sdc_plt00641 > convergence.${DIM}d.hi.sdc.out


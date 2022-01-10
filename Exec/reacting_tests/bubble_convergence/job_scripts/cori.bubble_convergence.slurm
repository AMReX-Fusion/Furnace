#!/bin/bash
#SBATCH --job-name=bubble_convergence
#SBATCH --account=m3018
#SBATCH --nodes=2
#SBATCH --tasks-per-node=32
#SBATCH --qos=regular
#SBATCH --time=06:00:00
#SBATCH --constraint=haswell

echo "starting..."

export OMP_NUM_THREADS=1

export FURNACE_EXEC=./Furnace2d.intel.haswell.MPI.TRUESDC.ex

# Cori Haswell has 2 Intel "Haswell" processors each with 16 cores.
# number of nodes = # of MPI * number of threads / 32 (cores / node)

RUNPARAMS="
furnace.sdc_order=4
furnace.time_integration_method=2
furnace.limit_fourth_order=1
furnace.use_reconstructed_gamma1=1
furnace.sdc_solve_for_rhoe=1
furnace.sdc_solver_tol_spec=1.e-10
furnace.sdc_solver_tol_dens=1.e-10
furnace.sdc_solver_tol_ener=1.e-5
furnace.sdc_solver_atol=1.e-10
furnace.sdc_solver=1"

srun ${FURNACE_EXEC} inputs_2d.64 ${RUNPARAMS} amr.plot_file=bubble_64_sdc4_plt &> 64.out
srun ${FURNACE_EXEC} inputs_2d.128 ${RUNPARAMS} amr.plot_file=bubble_128_sdc4_plt &> 128.out
srun ${FURNACE_EXEC} inputs_2d.256 ${RUNPARAMS} amr.plot_file=bubble_256_sdc4_plt &> 256.out
srun ${FURNACE_EXEC} inputs_2d.512 ${RUNPARAMS} amr.plot_file=bubble_512_sdc4_plt &> 512.out

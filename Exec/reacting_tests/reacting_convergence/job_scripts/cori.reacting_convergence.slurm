#!/bin/bash
#SBATCH --job-name=react_conv
#SBATCH --account=m3018
#SBATCH --nodes=1
#SBATCH --tasks-per-node=32
#SBATCH --qos=regular
#SBATCH --time=04:00:00
#SBATCH --constraint=haswell

echo "starting..."

export OMP_NUM_THREADS=1


slack_job_start.py "starting reacting_convergence problem @ NERSC" @michael

export FURNACE_EXEC=./Furnace2d.intel.haswell.MPI.ex

# Cori Haswell has 2 Intel "Haswell" processors each with 16 cores.
# number of nodes = # of MPI * number of threads / 32 (cores / node)


# SDC-4
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

srun ${FURNACE_EXEC} inputs.64 amr.plot_file=react_converge_64_sdc4_plt ${RUNPARAMS}
srun ${FURNACE_EXEC} inputs.128 amr.plot_file=react_converge_128_sdc4_plt ${RUNPARAMS}
srun ${FURNACE_EXEC} inputs.256 amr.plot_file=react_converge_256_sdc4_plt ${RUNPARAMS}
srun ${FURNACE_EXEC} inputs.512 amr.plot_file=react_converge_512_sdc4_plt ${RUNPARAMS}

# SDC
RUNPARAMS="
furnace.sdc_order=2
furnace.time_integration_method=2
furnace.ppm_type=0
furnace.sdc_solve_for_rhoe=1
furnace.sdc_solver_tol=1.e-10
furnace.sdc_solver=1"

srun ${FURNACE_EXEC} inputs.64 amr.plot_file=react_converge_64_sdc_plt ${RUNPARAMS}
srun ${FURNACE_EXEC} inputs.128 amr.plot_file=react_converge_128_sdc_plt ${RUNPARAMS}
srun ${FURNACE_EXEC} inputs.256 amr.plot_file=react_converge_256_sdc_plt ${RUNPARAMS}
srun ${FURNACE_EXEC} inputs.512 amr.plot_file=react_converge_512_sdc_plt ${RUNPARAMS}

# Strang
RUNPARAMS="furnace.time_integration_method=0"

srun ${FURNACE_EXEC} inputs.64 amr.plot_file=react_converge_64_strang_plt ${RUNPARAMS}
srun ${FURNACE_EXEC} inputs.128 amr.plot_file=react_converge_128_strang_plt ${RUNPARAMS}
srun ${FURNACE_EXEC} inputs.256 amr.plot_file=react_converge_256_strang_plt ${RUNPARAMS}
srun ${FURNACE_EXEC} inputs.512 amr.plot_file=react_converge_512_strang_plt ${RUNPARAMS}

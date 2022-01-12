#!/bin/bash


EXEC=./Logi1d.gnu.MPI.TRUESDC.ex


## sdc-2 + reflect

ofile=sdc2-reflect.converge.out

RUNPARAMS="
logi.time_integration_method=2
logi.sdc_order=2
logi.ppm_type=0
logi.limit_fourth_order=1
logi.use_reconstructed_gamma1=1
logi.lo_bc=3
logi.hi_bc=3
"""

${EXEC} inputs.ppm.64 ${RUNPARAMS} >& 64.out
pfile=`ls -t | grep -i hse_64_plt | head -1` 
fextrema.gnu.ex -v magvel ${pfile} | grep -i magvel > ${ofile}

${EXEC} inputs.ppm.128 ${RUNPARAMS} >& 128.out
pfile=`ls -t | grep -i hse_128_plt | head -1` 
fextrema.gnu.ex -v magvel ${pfile} | grep -i magvel >> ${ofile}

${EXEC} inputs.ppm.256 ${RUNPARAMS} >& 256.out
pfile=`ls -t | grep -i hse_256_plt | head -1` 
fextrema.gnu.ex -v magvel ${pfile} | grep -i magvel >> ${ofile}

${EXEC} inputs.ppm.512 ${RUNPARAMS} >& 512.out
pfile=`ls -t | grep -i hse_512_plt | head -1` 
fextrema.gnu.ex -v magvel ${pfile} | grep -i magvel >> ${ofile}


## sdc-2 + ppm + reflect

ofile=sdc2-ppm-reflect.converge.out

RUNPARAMS="
logi.time_integration_method=2
logi.sdc_order=2
logi.ppm_type=1
logi.limit_fourth_order=1
logi.use_reconstructed_gamma1=1
logi.lo_bc=3
logi.hi_bc=3
"""

${EXEC} inputs.ppm.64 ${RUNPARAMS} >& 64.out
pfile=`ls -t | grep -i hse_64_plt | head -1` 
fextrema.gnu.ex -v magvel ${pfile} | grep -i magvel > ${ofile}

${EXEC} inputs.ppm.128 ${RUNPARAMS} >& 128.out
pfile=`ls -t | grep -i hse_128_plt | head -1` 
fextrema.gnu.ex -v magvel ${pfile} | grep -i magvel >> ${ofile}

${EXEC} inputs.ppm.256 ${RUNPARAMS} >& 256.out
pfile=`ls -t | grep -i hse_256_plt | head -1` 
fextrema.gnu.ex -v magvel ${pfile} | grep -i magvel >> ${ofile}

${EXEC} inputs.ppm.512 ${RUNPARAMS} >& 512.out
pfile=`ls -t | grep -i hse_512_plt | head -1` 
fextrema.gnu.ex -v magvel ${pfile} | grep -i magvel >> ${ofile}


## sdc-2 + ppm

ofile=sdc2-ppm.converge.out

RUNPARAMS="
logi.time_integration_method=2
logi.sdc_order=2
logi.ppm_type=1
logi.limit_fourth_order=1
logi.use_reconstructed_gamma1=1
"""

${EXEC} inputs.ppm.64 ${RUNPARAMS} >& 64.out
pfile=`ls -t | grep -i hse_64_plt | head -1` 
fextrema.gnu.ex -v magvel ${pfile} | grep -i magvel > ${ofile}

${EXEC} inputs.ppm.128 ${RUNPARAMS} >& 128.out
pfile=`ls -t | grep -i hse_128_plt | head -1` 
fextrema.gnu.ex -v magvel ${pfile} | grep -i magvel >> ${ofile}

${EXEC} inputs.ppm.256 ${RUNPARAMS} >& 256.out
pfile=`ls -t | grep -i hse_256_plt | head -1` 
fextrema.gnu.ex -v magvel ${pfile} | grep -i magvel >> ${ofile}

${EXEC} inputs.ppm.512 ${RUNPARAMS} >& 512.out
pfile=`ls -t | grep -i hse_512_plt | head -1` 
fextrema.gnu.ex -v magvel ${pfile} | grep -i magvel >> ${ofile}


## sdc-2 + HSE

ofile=sdc2.converge.out

RUNPARAMS="
logi.time_integration_method=2
logi.sdc_order=2
logi.ppm_type=0
logi.limit_fourth_order=1
logi.use_reconstructed_gamma1=1
"""

${EXEC} inputs.ppm.64 ${RUNPARAMS} >& 64.out
pfile=`ls -t | grep -i hse_64_plt | head -1` 
fextrema.gnu.ex -v magvel ${pfile} | grep -i magvel > ${ofile}

${EXEC} inputs.ppm.128 ${RUNPARAMS} >& 128.out
pfile=`ls -t | grep -i hse_128_plt | head -1` 
fextrema.gnu.ex -v magvel ${pfile} | grep -i magvel >> ${ofile}

${EXEC} inputs.ppm.256 ${RUNPARAMS} >& 256.out
pfile=`ls -t | grep -i hse_256_plt | head -1` 
fextrema.gnu.ex -v magvel ${pfile} | grep -i magvel >> ${ofile}

${EXEC} inputs.ppm.512 ${RUNPARAMS} >& 512.out
pfile=`ls -t | grep -i hse_512_plt | head -1` 
fextrema.gnu.ex -v magvel ${pfile} | grep -i magvel >> ${ofile}



## sdc-4 + reflect

ofile=sdc4-reflect.converge.out

RUNPARAMS="
logi.time_integration_method=2
logi.sdc_order=4
logi.limit_fourth_order=1
logi.use_reconstructed_gamma1=1
logi.lo_bc=3
logi.hi_bc=3
"""

${EXEC} inputs.ppm.64 ${RUNPARAMS} >& 64.out
pfile=`ls -t | grep -i hse_64_plt | head -1` 
fextrema.gnu.ex -v magvel ${pfile} | grep -i magvel > ${ofile}

${EXEC} inputs.ppm.128 ${RUNPARAMS} >& 128.out
pfile=`ls -t | grep -i hse_128_plt | head -1` 
fextrema.gnu.ex -v magvel ${pfile} | grep -i magvel >> ${ofile}

${EXEC} inputs.ppm.256 ${RUNPARAMS} >& 256.out
pfile=`ls -t | grep -i hse_256_plt | head -1` 
fextrema.gnu.ex -v magvel ${pfile} | grep -i magvel >> ${ofile}

${EXEC} inputs.ppm.512 ${RUNPARAMS} >& 512.out
pfile=`ls -t | grep -i hse_512_plt | head -1` 
fextrema.gnu.ex -v magvel ${pfile} | grep -i magvel >> ${ofile}


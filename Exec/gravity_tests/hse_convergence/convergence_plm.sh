#!/bin/bash


EXEC=./Logi1d.gnu.MPI.ex


## plm

ofile=plm.converge.out

RUNPARAMS="
logi.ppm_type=0
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


## plm + hse reflect

ofile=plm-hsereflect.converge.out

RUNPARAMS="
logi.ppm_type=0
logi.hse_interp_temp=1
logi.hse_reflect_vels=1
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


## plm + hse reflect + no pslope

ofile=plm-hsereflect-nopslope.converge.out

RUNPARAMS="
logi.ppm_type=0
logi.use_pslope=0
logi.hse_interp_temp=1
logi.hse_reflect_vels=1
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


## plm + reflect

ofile=plm-reflect.converge.out

RUNPARAMS="
logi.ppm_type=0
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



## plm + reflect + nopslope

ofile=plm-reflect-nopslope.converge.out

RUNPARAMS="
logi.ppm_type=0
logi.lo_bc=3
logi.hi_bc=3
logi.use_pslope=0
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








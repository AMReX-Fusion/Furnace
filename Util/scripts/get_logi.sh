#!/bin/sh

# clone Logi and all associated repos, then output the necessary
# environment variables to build with this source.

branch=master

amrex_server=https://github.com/AMReX-Codes
astro_server=https://github.com/AMReX-Astro
micro_server=https://github.com/StarKiller-astro


#-----------------------------------------------------------------------------

pwd=`pwd`

# Logi
echo "cloning Logi"
git clone ${astro_server}/Logi.git
cd Logi
git checkout ${branch}
cd ..


# AMReX
echo " "
echo "cloning amrex"
git clone ${amrex_server}/amrex.git
cd amrex
git checkout ${branch}
cd ..


# Microphysics
echo " "
echo "cloning Microphysics"
git clone ${micro_server}/Microphysics.git
cd Microphysics
git checkout ${branch}
cd ..


# output the necessary environment changes
if [ -f logi_exports.sh ]; then
    echo "removing old logi_exports.sh"
    rm -f logi_exports.sh
fi

cat >> logi_exports.sh << EOF 
export LOGI_HOME="${pwd}/Logi"
export MICROPHYSICS_HOME="${pwd}/Microphysics"
export AMREX_HOME="${pwd}/amrex"
EOF

echo " "
echo "source logi_exports.sh to setup the environment for building"





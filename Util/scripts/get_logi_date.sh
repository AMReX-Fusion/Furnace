#!/bin/sh

# clone Logi and all associated repos and reset them to a particular
# date/time.  Then output the necessary environment variables to build
# with this source.


amrex_server=https://github.com/AMReX-Codes
astro_server=https://github.com/AMReX-Astro
micro_server=https://github.com/StarKiller-astro
branch=development

#-----------------------------------------------------------------------------

date=$1
echo date is \"$date\"

pwd=`pwd`

# Logi
echo "cloning Logi"
git clone ${astro_server}/Logi.git
cd Logi
git checkout ${branch}
cd ..

if [ -n "$date" ]
   then
       echo " "
       echo "resetting to before ${date}"
       cd Logi
       hash=`git rev-list -n 1 --before="$date" $branch`
       #git reset --hard ${hash}
       git checkout ${hash}
       cd ..
fi

# AMReX
echo " "
echo "cloning amrex"
git clone ${amrex_server}/amrex.git
cd amrex
git checkout ${branch}
cd ..

if [ -n "$date" ]
   then
       echo " "
       echo "resetting to before ${date}"
       cd amrex
       hash=`git rev-list -n 1 --before="$date" $branch`
       #git reset --hard ${hash}
       git checkout ${hash}
       cd ..
fi

# Microphysics
echo " "
echo "cloning Microphysics"
git clone ${micro_server}/Microphysics.git
cd Microphysics
git checkout ${branch}
cd ..

if [ -n "$date" ]
   then
       echo " "
       echo "resetting to before ${date}"
       cd Microphysics
       hash=`git rev-list -n 1 --before="$date" $branch`
       #git reset --hard ${hash}
       git checkout ${hash}
       cd ..
fi

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





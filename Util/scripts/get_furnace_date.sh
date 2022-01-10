#!/bin/sh

# clone Furnace and all associated repos and reset them to a particular
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

# Furnace
echo "cloning Furnace"
git clone ${astro_server}/Furnace.git
cd Furnace
git checkout ${branch}
cd ..

if [ -n "$date" ]
   then
       echo " "
       echo "resetting to before ${date}"
       cd Furnace
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
if [ -f furnace_exports.sh ]; then
    echo "removing old furnace_exports.sh"
    rm -f furnace_exports.sh
fi

cat >> furnace_exports.sh << EOF 
export FURNACE_HOME="${pwd}/Furnace"
export MICROPHYSICS_HOME="${pwd}/Microphysics"
export AMREX_HOME="${pwd}/amrex"
EOF

echo " "
echo "source furnace_exports.sh to setup the environment for building"





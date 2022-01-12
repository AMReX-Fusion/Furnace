#!/bin/bash

# In order to use this bash script do the folowing:
#
# 1) Change the refactoring string 
# 2) Run this script: . refactor.sh
# 3) Include ignored file extensions and re-run again

# List of file to look in
arr=("*.sh" "*.H" "*.cpp" "*.py" "*.rst*" "*.md" "*.txt" "*.submit"
     "*.tex" "*.slurm" "*.gp" "*.svg" "*.f90" "*.json" "*.yml" "*.ps"
     "inputs*" "Make*" "GNUmakefile*"
     "README" "Readme" "_cpp_parameters" 
     ".gitignore" "Doxyfile" "_variables"
    )

# Refactoring function
function refactor {

    echo "Renaming '$1' to '$2'"
    find $3 -name "*$1*" -exec rename "s/$1/$2/" {} +
    
    echo "Refactoring '$1' to '$2'"
    echo ""
    
    ## now loop through the above array
    for i in "${arr[@]}"
    do
	echo -e "\e[1A\e[KIn file '$i'"
	find $3 -type f \( -iname "$i" ! -iname "refactor.sh" \) -exec sed -i "s/$1/$2/g" {} \;    
    done

    echo -e "\e[1A\e[KDone"

    echo "Ignored files:"
    grep -r $1 $3
    echo ""
}

# Refactoring strings
refactor 'castro' 'logi' ./
refactor 'Castro' 'Logi' ./
refactor 'CASTRO' 'LOGI' ./

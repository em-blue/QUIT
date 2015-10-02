#!/bin/bash -e

# Tobias Wood 2015
# Simple test script for DESPOT programs

# Tests whether programs run successfully on toy data

# First, create input data

source ./test_common.sh
SILENCE_TESTS="0"

DATADIR="2C"
mkdir -p $DATADIR
cd $DATADIR
rm *

DIMS="5 5 3"

$QUITDIR/qinewimage -d "$DIMS" -f "1.0" PD.nii
$QUITDIR/qinewimage -d "$DIMS" -f "0.465" T1_m.nii
$QUITDIR/qinewimage -d "$DIMS" -f "0.026" T2_m.nii
$QUITDIR/qinewimage -d "$DIMS" -f "1.070" T1_ie.nii
$QUITDIR/qinewimage -d "$DIMS" -f "0.117" T2_ie.nii
$QUITDIR/qinewimage -d "$DIMS" -f "0.18" tau_m.nii
$QUITDIR/qinewimage -d "$DIMS" -g "0 -15. 15." f0.nii
$QUITDIR/qinewimage -d "$DIMS" -g "1 0.75 1.25" B1.nii
$QUITDIR/qinewimage -d "$DIMS" -g "2 0.05 0.25" f_m.nii

# Setup parameters
SPGR_FILE="spgr.nii"
SPGR_FLIP="3 4 5 6 7 9 13 18"
SPGR_TR="0.0065"
SSFP_FILE="ssfp.nii"
SSFP_FLIP="12 16 21 27 33 40 51 68 "
SSFP_PC="180 0"
SSFP_TR="0.005"

run_test "CREATE_SIGNALS" $QUITDIR/qisignal --2 -n -v << END_MCSIG
PD.nii
T1_m.nii
T2_m.nii
T1_ie.nii
T2_ie.nii
tau_m.nii
f_m.nii
f0.nii
B1.nii
SPGR
$SPGR_FLIP
$SPGR_TR
$SPGR_FILE
SSFP
$SSFP_FLIP
$SSFP_PC
$SSFP_TR
$SSFP_FILE
END
END_MCSIG

# Save these for mcd input to make debugging easy.
# Currently the sequence type/filename are the opposite to qisignal due to history/idiocy
# This needs to be changed for a later release
ALL_PARS="SPGR
$SPGR_FILE
$SPGR_FLIP
$SPGR_TR
SSFP
$SSFP_FILE
$SSFP_FLIP
$SSFP_PC
$SSFP_TR
END"

echo "$ALL_PARS" > mcd_input.txt

function run() {
PREFIX="$1"
OPTS="$2"
run_test $PREFIX $QUITDIR/qimcdespot $OPTS -M2 -bB1.nii -r -o $PREFIX -v -n < mcd_input.txt

compare_test $PREFIX f_m.nii ${PREFIX}2C_f_m.nii 0.05

#echo "       Mean     Std.     CoV"
#echo "T1_m:  " $( fslstats ${PREFIX}2C_T1_m.nii  -m -s | awk '{if(($1)>(0.)) {print $1, $2, $2/$1} else {print 0}}' )
#echo "T2_m:  " $( fslstats ${PREFIX}2C_T2_m.nii  -m -s | awk '{if(($1)>(0.)) {print $1, $2, $2/$1} else {print 0}}' )
#echo "T1_ie: " $( fslstats ${PREFIX}2C_T1_ie.nii -m -s | awk '{if(($1)>(0.)) {print $1, $2, $2/$1} else {print 0}}' )
#echo "T2_ie: " $( fslstats ${PREFIX}2C_T2_ie.nii -m -s | awk '{if(($1)>(0.)) {print $1, $2, $2/$1} else {print 0}}' )
#echo "MWF:   " $( fslstats ${PREFIX}2C_f_m.nii   -m -s | awk '{if(($1)>(0.)) {print $1, $2, $2/$1} else {print 0}}' )
#echo "Tau:   " $( fslstats ${PREFIX}2C_tau_m.nii -m -s | awk '{if(($1)>(0.)) {print $1, $2, $2/$1} else {print 0}}' )
}

run "BFGS" " -ff0.nii -ab"
run "GRC"  " -ff0.nii -aG"
run "SRC"  " -ff0.nii -aS"
run "SRC2" " -aS "
cd ..
SILENCE_TESTS="0"

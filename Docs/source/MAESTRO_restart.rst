***********************
Restarting from Maestro
***********************

Overview
========

We can now initialize a Furnace simulation using data from a Maestro plotfile. This should not be thought of as a restart mode, but rather
a new simulation with a special initialization. In order to use this
feature, you must make sure the Maestro plotfile has the proper
variables, add some new parameters to your inputs file, and add a few
subroutines to Prob_Xd.f90. You need to build a special executable
with “USE_MAESTRO_INIT=TRUE”, which will add “.MAESTRO” to the
executable string. For multilevel problems, there are a few extra
steps relating to the fact that you have to supply a grids file
consistent with the Maestro grid structure.

MAESTRO Plotfile Requirements
=============================

The Maestro plotfile needs to have the following variables:

-  “x_vel”, “y_vel”, (and “z_vel”, depending on
   dimensionality of the problem)

-  “density” (**furnace.MAESTRO_init_type** = 1 and 2 only)

-  Optional species (such as “X(C12)”) - there is an option to
   not read any species from the Maestro plotfile. In this case, you
   must make sure your code manually defines the species cell-by-cell
   in the initial Furnace data

-  “tfromp”

-  “pi” (**furnace.MAESTRO_init_type** = 2, 3, and 4 only)

-  “entropy” (**furnace.MAESTRO_init_type** = 4 only)

Also, model_cc_XXXXX needs to list variables in the following order,
which is the default order found in MAESTRO/Source/base_io.f90: r,
base_r, rho0, p0, gamma1bar, rhoh0, div_coeff, psi, tempbar,
etarho_cc, tempbar_init.

List of Parameters
==================

Here are the additional parameters you must add to your inputs file.

+-----------------------------------+------------------+-----------------+-----------------+
| Parameter                         | Definition       | Type            | Default         |
+===================================+==================+=================+=================+
| **furnace.MAESTRO_plotfile**       | name of the      | std::string     | must be set     |
|                                   | Maestro plotfile |                 |                 |
|                                   |                  |                 |                 |
+-----------------------------------+------------------+-----------------+-----------------+
| **furnace.MAESTRO_modelfile**      | name of the      | std::string     | must be set     |
|                                   | Maestro model_cc |                 |                 |
|                                   | file             |                 |                 |
+-----------------------------------+------------------+-----------------+-----------------+
| **furnace.MAESTRO_npts_model**     | number of        | int             | must be set     |
|                                   | points in the    |                 |                 |
|                                   | Maestro model_cc |                 |                 |
|                                   | file             |                 |                 |
+-----------------------------------+------------------+-----------------+-----------------+
| **furnace.MAESTRO_first_species**  | name of the      | std::string     | must be set or  |
|                                   | first species    |                 | else nothing    |
|                                   |                  |                 | will be read in |
+-----------------------------------+------------------+-----------------+-----------------+
| **furnace.MAESTRO_nspec**          | number of        | std::string     | NumSpec in      |
|                                   | species in the   |                 | Furnace          |
|                                   | Maestro plotfile |                 |                 |
+-----------------------------------+------------------+-----------------+-----------------+
| **furnace.MAESTRO_cutoff_density** | controls how we  | Real            | must be set     |
|                                   | overwrite data   |                 |                 |
|                                   | at the edge of   |                 |                 |
|                                   | the star         |                 |                 |
+-----------------------------------+------------------+-----------------+-----------------+
| **furnace.MAESTRO_init_type**      | determines how   | int             | must be set     |
|                                   | we initialize    |                 |                 |
|                                   | the              |                 |                 |
|                                   | Furnace state     |                 |                 |
+-----------------------------------+------------------+-----------------+-----------------+
| **furnace.MAESTRO_spherical**      | specifies        | int             | must be set     |
|                                   | planar or        |                 |                 |
|                                   | spherical        |                 |                 |
|                                   | problem          |                 |                 |
+-----------------------------------+------------------+-----------------+-----------------+

Examples of Usage
-----------------

-  **furnace.MAESTRO_plotfile** = "wd_384_6.25e8K_norotate_plt120578"

-  **furnace.MAESTRO_modelfile** = "./wd_384_6.25e8K_norotate_plt120578/model_cc_120578"

-  | **furnace.MAESTRO_npts_model** = 1663
   | This is the number of
     points in **furnace.MAESTRO_modelfile**. Note that this is not
     the same thing as “npts_model”, which is the number of points in
     the initial model file used for standard simulations where we do not
     initialize from a Maestro plotfile.

-  **furnace.MAESTRO_first_species** = “X(C12)” If you do not
   specify this, no species will be read in. You can always manually
   specify or overwrite the species cell-by-cell later.

-  | **furnace.MAESTRO_nspec** = 3
   | If you do not specify this, it
     will default to the number of species in the Furnace network,
     “NumSpec”. We have this here because sometimes Maestro and Furnace will use different networks with different number of species.

-  | **furnace.MAESTRO_cutoff_density** = 1.e6
   | The code will use
     this density to figure out the radial coordinate, r_model_start,
     which is the last radial coordinate before rho0 falls below
     **furnace.MAESTRO_cutoff_density**. It is possible to set
     **furnace.MAESTRO_cutoff_density** to a tiny value, such that rho0
     never falls below this value, in which case we set r_model_start
     to :math:`\infty`. In INITDATA_MAKEMODEL, we create a new 1D model
     integrating outward starting from r_model_start. Then, in
     INITDATA_OVERWRITE, we overwrite newly initialized Furnace data in
     any cell that maps into a radial coordinate greater than
     r_model_start by interpolating from the new 1D model.

-  | **furnace.MAESTRO_init_type** = 2
   | Furnace will read in data
     from the Maestro plotfile, and then call the EOS to make sure that
     :math:`\rho`, :math:`e`, :math:`T`, and :math:`X_k` are consistent. The inputs to the EOS
     are based on the value of **furnace.MAESTRO_init_type**:

   #. :math:`e = e(\rho,T,X_k)`

   #. :math:`e,T = e,T(\rho,p_0+\pi,X_k)`

   #. :math:`\rho,e = \rho,e(p_0+\pi,T,X_k)`

   #. :math:`\rho,T,e = \rho,T,e(p_0+\pi,s,X_k)`

-  | **furnace.MAESTRO_spherical** = 1
   | 0 = planar; 1 = spherical.

New Subroutines in Prob_Xd.f90
==============================

There are three routines that need to be added to your local copy of
Prob_Xd.f90. See Furnace/Exec/wdconvect/Prob_3d.f90 for
a standard spherical Maestro initialization.

#. | INITDATA_MAESTRO
   | This fills in the Furnace state by taking
     the Maestro data, calling the EOS, and making the proper variables
     conserved quantities. Specifically, we need a thermodynamically
     consistent :math:`\rho`, :math:`T`, :math:`e`, and :math:`X_k`, and then algebraically
     compute :math:`\rho{\bf u}`, :math:`\rho e`, :math:`\rho E`, and :math:`\rho X_k`,

#. | INITDATA_MAKEMODEL
   | This creates a user-defined 1D initial model starting from r_model_start.

#. | INITDATA_OVERWRITE
   | This overwrites the initialized Furnace data using the new 1D initial model for all cells that map into
     radial coordinates greater than r_model_start.

Additional Notes
================

Note that for both single-level and multilevel Maestro to Furnace initialization, the Furnace base grid structure does not have to match
the Maestro base grid structure, as long as the problem domain is the
same. For example, if the coarsest level in a Maestro plotfile
contains :math:`64^3` cells divided into 8-\ :math:`32^3` grids, it is ok to use a
Furnace base grid structure with 1-\ :math:`64^3` grid, 64-\ :math:`16^3` grids, or
anything else you can imagine - the grids don’t even have to be the
same size. As is normally the case, the Furnace base grid structure is
created based on the parameters in the Furnace inputs file, such as
**amr.max_grid_size**, **amr.blocking_factor**, etc.

Multilevel Restart
------------------

When initialing from a multilevel Maestro plotfile, there are some
extra steps. First, you need to create a Furnace-compatible grids file
from the Maestro plotfile. This can be done with the
BoxLib/Tools/Postprocessing/F_Src/fboxinfo.f90 utility. Compile
and run this using the “``–``\ furnace” option, e.g.,
“fboxinfo.Linux.gfortran.exe ``–``\ furnace pltxxxxx ``|``
tee gr0.maestro”, to generate the Furnace-compatible grids file. Note
that the base grid structure is still controlled by
``amr.max_grid_size``, ``amr.blocking_factor``, etc., since in C++ AMReX, the grids file only indicates the refined grid structure,
whereas in Fortran BoxLib the grids file contains the base grid and
refined grid structures.

Now, when you initialize the Furnace simulation, you need to specify
the grid file using **amr.regrid_file = "gr0_3d.128_2levels"**,
for example. You can happily run this now, but note that the
regridding algorithm will never be called (since Furnace thinks it’s
started a new simulation from scratch with a grids file, thus
disabling the regridding). If you wish for the grid structure to be
changed, you must do a traditional Furnace restart from the
Furnace-generated checkpoint file (you can still use the same
“.MAESTRO” executable or an executable built with
USE_MAESTRO_INIT=FALSE), making sure that you **do not** specity
**amr.regrid_file** (or else the grids will stay fixed). You are
free to specify **amr.regrid_on_restart**,
**amr.compute_new_dt_on_regrid**, and
**amr.plotfile_on_restart**.

Sometimes a Maestro plotfile will only have 1 or 2 total levels, but
you ultimately want to run a Furnace simulation with many more levels
of refinement. My recommended strategy is the following:

#. Initialize a Furnace simulation from the Maestro plotfile
   while preserving the exact same grid structure and run for 10 time
   steps.

#. Do a traditional Furnace restart from chk00010, but do not
   increase **amr.max_level**, and run for 10 more time steps. This
   allows a new grid structure with the same effective resolution as
   before settle in using the C BoxLib regridding algorithm.

#. Do a traditional Furnace restart from chk00020, but increase
   **amr.max_level** by 1, and run for 10 time steps.

#. Repeat the procedure from the previous step (using the most
   updated checkpoint of course) as many times as desired.

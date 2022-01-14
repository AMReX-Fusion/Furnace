#ifndef WIN32
#include <unistd.h>
#endif

#include <iomanip>

#include <algorithm>
#include <cstdio>
#include <vector>
#include <iostream>
#include <string>
#include <ctime>

#include <AMReX_Utility.H>
#include <AMReX_CONSTANTS.H>
#include <Logi.H>
#include <Logi_F.H>
#include <runtime_parameters.H>
#include <AMReX_VisMF.H>
#include <AMReX_TagBox.H>
#include <AMReX_FillPatchUtil.H>
#include <AMReX_ParmParse.H>
#ifdef MICROPHYSICS_FORT
#include <extern_parameters_F.H>
#endif

#ifdef RADIATION
#include <Radiation.H>
#include <RAD_F.H>
#endif

#ifdef AMREX_PARTICLES
#include <AMReX_Particles.H>
#endif

#ifdef DIFFUSION
#include <Diffusion.H>
#endif

#ifdef _OPENMP
#include <omp.h>
#endif

#include <extern_parameters.H>
#include <prob_parameters.H>

#ifdef MICROPHYSICS_FORT
#include <microphysics_F.H>
#endif

#include <problem_initialize.H>
#include <problem_initialize_state_data.H>
#ifdef MHD
#include <problem_initialize_mhd_data.H>
#endif
#ifdef RADIATION
#include <problem_initialize_rad_data.H>
#endif
#include <problem_tagging.H>

#include <ambient.H>

using namespace amrex;

bool         Logi::signalStopJob = false;

#ifdef REACTIONS
std::vector<std::string> Logi::burn_weight_names;
#endif

std::vector<std::string> Logi::err_list_names;
std::vector<int> Logi::err_list_ng;
int          Logi::num_err_list_default = 0;
int          Logi::radius_grow   = 1;
BCRec        Logi::phys_bc;
int          Logi::NUM_GROW      = -1;
int          Logi::NUM_GROW_SRC  = -1;

int          Logi::lastDtPlotLimited = 0;
Real         Logi::lastDtBeforePlotLimiting = 0.0;

Real         Logi::num_zones_advanced = 0.0;

Vector<std::string> Logi::source_names;

Vector<AMRErrorTag> Logi::error_tags;

Vector<std::unique_ptr<std::fstream>> Logi::data_logs;
Vector<std::unique_ptr<std::fstream>> Logi::problem_data_logs;

#ifdef TRUE_SDC
int          Logi::SDC_NODES;
Vector<Real> Logi::dt_sdc;
Vector<Real> Logi::node_weights;
#endif

#ifdef DIFFUSION
// the diffusion object
Diffusion*    Logi::diffusion  = 0;
#endif

#ifdef RADIATION

// the radiation object
Radiation*   Logi::radiation = 0;
#endif


std::string  Logi::probin_file = "probin";


#if AMREX_SPACEDIM == 1
#ifndef AMREX_USE_GPU
IntVect      Logi::hydro_tile_size(1024);
#else
IntVect      Logi::hydro_tile_size(1048576);
#endif
IntVect      Logi::no_tile_size(1024);
#elif AMREX_SPACEDIM == 2
#ifndef AMREX_USE_GPU
IntVect      Logi::hydro_tile_size(1024,16);
#else
IntVect      Logi::hydro_tile_size(1048576,1048576);
#endif
IntVect      Logi::no_tile_size(1024,1024);
#else
#ifndef AMREX_USE_GPU
IntVect      Logi::hydro_tile_size(1024,16,16);
#else
IntVect      Logi::hydro_tile_size(1048576,1048576,1048576);
#endif
IntVect      Logi::no_tile_size(1024,1024,1024);
#endif

// this will be reset upon restart
Real         Logi::previousCPUTimeUsed = 0.0;

Real         Logi::startCPUTime = 0.0;

int          Logi::SDC_Source_Type = -1;
int          Logi::num_state_type = 0;

int          Logi::do_cxx_prob_initialize = 0;


// Logi::variableSetUp is in Logi_setup.cpp
// variableCleanUp is called once at the end of a simulation
void
Logi::variableCleanUp ()
{

#ifdef DIFFUSION
  if (diffusion != 0) {
    if (verbose > 1 && ParallelDescriptor::IOProcessor()) {
      std::cout << "Deleting diffusion in variableCleanUp..." << '\n';
    }
    delete diffusion;
    diffusion = 0;
  }
#endif

#ifdef RADIATION
  if (radiation != 0) { int report = (verbose || radiation->verbose);
    if (report && ParallelDescriptor::IOProcessor()) {
      std::cout << "Deleting radiation in variableCleanUp..." << '\n';
    }
    delete radiation;
    radiation = 0;
    if (report && ParallelDescriptor::IOProcessor()) {
      std::cout << "                                        done" << std::endl;
    }
  }
#endif

#ifdef AMREX_PARTICLES
  delete TracerPC;
  TracerPC = 0;
#endif

    desc_lst.clear();

#if !defined(NETWORK_HAS_CXX_IMPLEMENTATION)
    // Fortran cleaning
#ifdef MICROPHYSICS_FORT
    microphysics_finalize();
#endif
#endif

    // C++ cleaning
    eos_finalize();

}

void
Logi::read_params ()
{
    static bool done = false;

    if (done) return;

    done = true;

    // this gets all of the parameters defined in _cpp_params, regardless of
    // namespace
    initialize_cpp_runparams();

    ParmParse pp("logi");

    using namespace logi;


    // Get boundary conditions
    Vector<int> lo_bc(AMREX_SPACEDIM), hi_bc(AMREX_SPACEDIM);
    pp.getarr("lo_bc",lo_bc,0,AMREX_SPACEDIM);
    pp.getarr("hi_bc",hi_bc,0,AMREX_SPACEDIM);
    for (int i = 0; i < AMREX_SPACEDIM; i++)
    {
        phys_bc.setLo(i,lo_bc[i]);
        phys_bc.setHi(i,hi_bc[i]);
    }

    const Geometry& dgeom = DefaultGeometry();

    //
    // Check phys_bc against possible periodic geometry
    // if periodic, must have internal BC marked.
    //
    if (dgeom.isAnyPeriodic())
    {
        //
        // Do idiot check.  Periodic means interior in those directions.
        //
        for (int dir = 0; dir<AMREX_SPACEDIM; dir++)
        {
            if (dgeom.isPeriodic(dir))
            {
                if (lo_bc[dir] != Interior)
                {
                    std::cerr << "Logi::read_params:periodic in direction "
                              << dir
                              << " but low BC is not Interior\n";
                    amrex::Error();
                }
                if (hi_bc[dir] != Interior)
                {
                    std::cerr << "Logi::read_params:periodic in direction "
                              << dir
                              << " but high BC is not Interior\n";
                    amrex::Error();
                }
            }
        }
    }
    else
    {
        //
        // Do idiot check.  If not periodic, should be no interior.
        //
        for (int dir=0; dir<AMREX_SPACEDIM; dir++)
        {
            if (lo_bc[dir] == Interior)
            {
                std::cerr << "Logi::read_params:interior bc in direction "
                          << dir
                          << " but not periodic\n";
                amrex::Error();
            }
            if (hi_bc[dir] == Interior)
            {
                std::cerr << "Logi::read_params:interior bc in direction "
                          << dir
                          << " but not periodic\n";
                amrex::Error();
            }
        }
    }

    if ( dgeom.IsRZ() && (lo_bc[0] != Symmetry) ) {
        std::cerr << "ERROR:Logi::read_params: must set r=0 boundary condition to Symmetry for r-z\n";
        amrex::Error();
    }

#if (AMREX_SPACEDIM == 1)
    if ( dgeom.IsSPHERICAL() )
    {
      if ( (lo_bc[0] != Symmetry) && (dgeom.ProbLo(0) == 0.0) )
      {
        std::cerr << "ERROR:Logi::read_params: must set r=0 boundary condition to Symmetry for spherical\n";
        amrex::Error();
      }
    }
#elif (AMREX_SPACEDIM == 2)
    if ( dgeom.IsSPHERICAL() )
      {
        amrex::Abort("We don't support spherical coordinate systems in 2D");
      }
#elif (AMREX_SPACEDIM == 3)
    if ( dgeom.IsRZ() )
      {
        amrex::Abort("We don't support cylindrical coordinate systems in 3D");
      }
    else if ( dgeom.IsSPHERICAL() )
      {
        amrex::Abort("We don't support spherical coordinate systems in 3D");
      }
#endif

#ifdef HYBRID_MOMENTUM
    // We do not support hybrid advection when using the HLLC solver.

    if (riemann_solver == 2) {
        amrex::Abort("HLLC Riemann solver unsupported when using hybrid momentum.");
    }
#endif

#ifdef REACTIONS
#ifdef SIMPLIFIED_SDC
    if (jacobian == 1) {
      amrex::Abort("Simplified SDC requires the numerical Jacobian now (jacobian = 2)");
    }
#endif
#endif
    // sanity checks

    if (grown_factor < 1) {
      amrex::Error("grown_factor must be integer >= 1");
    }

    if (cfl <= 0.0 || cfl > 1.0) {
      amrex::Error("Invalid CFL factor; must be between zero and one.");
    }

    // SDC does not support GPUs yet
#ifdef AMREX_USE_GPU
    if (time_integration_method == SpectralDeferredCorrections) {
        amrex::Error("SDC is currently not enabled on GPUs.");
    }
#endif


    // Simplified SDC currently requires USE_SIMPLIFIED_SDC to be defined.
    // Also, if we have USE_SIMPLIFIED_SDC defined, we can't use the other
    // time integration_methods, because only the SDC burner
    // interface is available in Microphysics in this case.
#ifndef SIMPLIFIED_SDC
    if (time_integration_method == SimplifiedSpectralDeferredCorrections) {
        amrex::Error("Simplified SDC currently requires USE_SIMPLIFIED_SDC=TRUE when compiling.");
    }
#else
    if (time_integration_method != SimplifiedSpectralDeferredCorrections) {
        amrex::Error("When building with USE_SIMPLIFIED_SDC=TRUE, only simplified SDC can be used.");
    }
#endif

#ifndef TRUE_SDC
    if (time_integration_method == SpectralDeferredCorrections) {
        amrex::Error("True SDC currently requires USE_TRUE_SDC=TRUE when compiling.");
    }
#else
    if (time_integration_method != SpectralDeferredCorrections) {
        amrex::Error("When building with USE_TRUE_SDC=TRUE, only true SDC can be used.");
    }
#endif

#ifndef AMREX_USE_GPU

#ifdef RADIATION
    if (hybrid_riemann == 1) {
        amrex::Error("ERROR: hybrid Riemann not supported for radiation");
    }

    if (riemann_solver > 0) {
        amrex::Error("ERROR: only the CGF Riemann solver is supported for radiation");
    }
#endif

#if AMREX_SPACEDIM == 1
    if (riemann_solver > 1) {
        amrex::Error("ERROR: HLLC not implemented for 1-d");
    }
#endif

    if (riemann_solver == 1) {
        if (cg_maxiter > HISTORY_SIZE) {
            amrex::Error("error in riemanncg: cg_maxiter > HISTORY_SIZE");
        }

        if (cg_blend == 2 && cg_maxiter < 5) {
            amrex::Error("Error: need cg_maxiter >= 5 to do a bisection search on secant iteration failure.");
        }
    }
#endif

    if (hybrid_riemann == 1 && AMREX_SPACEDIM == 1)
      {
        std::cerr << "hybrid_riemann only implemented in 2- and 3-d\n";
        amrex::Error();
      }

    if (hybrid_riemann == 1 && (dgeom.IsSPHERICAL() || dgeom.IsRZ() ))
      {
        std::cerr << "hybrid_riemann should only be used for Cartesian coordinates\n";
        amrex::Error();
      }

#ifdef ROTATION
    if (dgeom.IsRZ() && state_in_rotating_frame == 0 && use_axisymmetric_geom_source)
    {
        std::cerr << "use_axisymmetric_geom_source is not compatible with state_in_rotating_frame=0\n";
        amrex::Error();
    }
#endif

    // Make sure not to call refluxing if we're not actually doing any hydro.
    if (do_hydro == 0) {
      do_reflux = 0;
    }

    if (max_dt < fixed_dt)
      {
        std::cerr << "cannot have max_dt < fixed_dt\n";
        amrex::Error();
      }

    if (change_max <= 1.0)
    {
        std::cerr << "change_max must be greater than 1.0\n";
        amrex::Error();
    }

#ifdef AMREX_PARTICLES
    read_particle_params();
#endif

#ifdef RADIATION
    // Some radiation parameters are initialized here because they
    // may be used in variableSetUp, well before the call to the
    // Radiation constructor,

    if (do_radiation) {
      Radiation::read_static_params();
    }

    // radiation is only supported with CTU
    if (do_radiation && time_integration_method != CornerTransportUpwind) {
        amrex::Error("Radiation is currently only supported for CTU time advancement.");
    }
#endif

#ifdef ROTATION
    if (do_rotation) {
      if (rotational_period <= 0.0) {
        std::cerr << "Error:Logi::Rotation enabled but rotation period less than zero\n";
        amrex::Error();
      }
    }
    if (dgeom.IsRZ())
      rot_axis = 2;
#if (AMREX_SPACEDIM == 1)
      if (do_rotation) {
        std::cerr << "ERROR:Logi::Rotation not implemented in 1d\n";
        amrex::Error();
      }
#endif
#endif

#ifdef SPONGE
    if (do_sponge) {
        if (sponge_timescale <= 0.0) {
            amrex::Error("If using the sponge, the sponge_timescale must be positive.");
        }
        if (amrex::max(sponge_upper_radius, sponge_upper_density, sponge_upper_pressure) < 0.0) {
            amrex::Error("If using the sponge, at least one of the upper radius, density, "
                         "or pressure must be non-negative.");
        }
        if (amrex::max(sponge_lower_radius, sponge_lower_density, sponge_lower_pressure) < 0.0) {
            amrex::Error("If using the sponge, at least one of the lower radius, density, "
                         "or pressure must be non-negative.");
        }
    }
#endif

   StateDescriptor::setBndryFuncThreadSafety(bndry_func_thread_safe);

   // Open up Logi data logs
   // Note that this functionality also exists in the Amr class
   // but we implement it on our own to have a little more control.
   // Some of these will only be filled for certain ifdefs, but
   // we should use consistent indexing regardless of ifdefs (so some
   // logs may be unused in a given run).

   if (sum_interval > 0 && ParallelDescriptor::IOProcessor()) {

       data_logs.resize(4);

       data_logs[0].reset(new std::fstream);
       data_logs[0]->open("grid_diag.out", std::ios::out | std::ios::app);
       if (!data_logs[0]->good()) {
           amrex::FileOpenFailed("grid_diag.out");
       }

       data_logs[1].reset(new std::fstream);

       data_logs[2].reset(new std::fstream);
       data_logs[2]->open("species_diag.out", std::ios::out | std::ios::app);
       if (!data_logs[2]->good()) {
           amrex::FileOpenFailed("species_diag.out");
       }

       data_logs[3].reset(new std::fstream);
       data_logs[3]->open("amr_diag.out", std::ios::out | std::ios::app);
       if (!data_logs[3]->good()) {
           amrex::FileOpenFailed("amr_diag.out");
       }

   }

   ParmParse ppa("amr");
   ppa.query("probin_file",probin_file);

    Vector<int> tilesize(AMREX_SPACEDIM);
    if (pp.queryarr("hydro_tile_size", tilesize, 0, AMREX_SPACEDIM))
    {
        for (int i=0; i<AMREX_SPACEDIM; i++) {
          hydro_tile_size[i] = tilesize[i];
        }
    }

    // Override Amr defaults. Note: this function is called after Amr::Initialize()
    // in Amr::InitAmr(), right before the ParmParse checks, so if the user opts to
    // override our overriding, they can do so.

    Amr::setComputeNewDtOnRegrid(1);

    // Read in custom refinement scheme.

    Vector<std::string> refinement_indicators;
    ppa.queryarr("refinement_indicators", refinement_indicators, 0, ppa.countval("refinement_indicators"));

    for (int i = 0; i < refinement_indicators.size(); ++i)
    {
        std::string ref_prefix = "amr.refine." + refinement_indicators[i];

        ParmParse ppr(ref_prefix);

        AMRErrorTagInfo info;

        if (ppr.countval("start_time") > 0) {
            Real min_time;
            ppr.get("start_time", min_time);
            info.SetMinTime(min_time);
        }
        if (ppr.countval("end_time") > 0) {
            Real max_time;
            ppr.get("end_time", max_time);
            info.SetMaxTime(max_time);
        }
        if (ppr.countval("max_level") > 0) {
            int max_level;
            ppr.get("max_level", max_level);
            info.SetMaxLevel(max_level);
        }

        if (int nval = ppr.countval("value_greater")) {
            Vector<Real> value;
            ppr.getarr("value_greater", value, 0, nval);
            std::string field;
            ppr.get("field_name", field);
            error_tags.push_back(AMRErrorTag(value, AMRErrorTag::GREATER, field, info));
        }
        else if (int nval = ppr.countval("value_less")) {
            Vector<Real> value;
            ppr.getarr("value_less", value, 0, nval);
            std::string field;
            ppr.get("field_name", field);
            error_tags.push_back(AMRErrorTag(value, AMRErrorTag::LESS, field, info));
        }
        else if (int nval = ppr.countval("gradient")) {
            Vector<Real> value;
            ppr.getarr("gradient", value, 0, nval);
            std::string field;
            ppr.get("field_name", field);
            error_tags.push_back(AMRErrorTag(value, AMRErrorTag::GRAD, field, info));
        }
        else if (int nval = ppr.countval("relative_gradient")) {
            Vector<Real> value;
            ppr.getarr("relative_gradient", value, 0, nval);
            std::string field;
            ppr.get("field_name", field);
            error_tags.push_back(AMRErrorTag(value, AMRErrorTag::RELGRAD, field, info));
        }
        else {
            amrex::Abort("Unrecognized refinement indicator for " + refinement_indicators[i]);
        }
    }

}

Logi::Logi ()
    :
    prev_state(num_state_type)
{
}

Logi::Logi (Amr&            papa,
                int             lev,
                const Geometry& level_geom,
                const BoxArray& bl,
                const DistributionMapping& dm,
                Real            time)
    :
    AmrLevel(papa,lev,level_geom,bl,dm,time),
    prev_state(num_state_type)
{
    MultiFab::RegionTag amrlevel_tag("AmrLevel_Level_" + std::to_string(lev));

    buildMetrics();

    initMFs();

    // Coterminous AMR boundaries are not supported in Logi if we're doing refluxing.

    if (do_hydro && do_reflux) {
        for (int ilev = 0; ilev <= parent->maxLevel(); ++ilev) {
            if (parent->nErrorBuf(ilev) == 0) {
                amrex::Error("n_error_buf = 0 is unsupported when using hydro.");
            }
        }
    }

    // initialize all the new time level data to zero
    for (int k = 0; k < num_state_type; k++) {
      MultiFab& data = get_new_data(k);
      data.setVal(0.0, data.nGrow());
    }

#ifdef DIFFUSION
      // diffusion is a static object, only alloc if not already there
      if (diffusion == 0) {
        diffusion = new Diffusion(parent,&phys_bc);
      }

      diffusion->install_level(level,this,volume,area.data());
#endif

#ifdef RADIATION
    if (do_radiation) {
      if (radiation == 0) {
        // radiation is a static object, only alloc if not already there
        radiation = new Radiation(parent, this);
      }
      radiation->regrid(level, grids, dmap);

      rad_solver.reset(new RadSolve(parent, level, grids, dmap));
    }
#endif

    // now check the runtime parameters to warn / abort if the user set
    // anything that isn't known to Logi

    validate_runparams();

}

Logi::~Logi ()
{
#ifdef RADIATION
    if (radiation != 0) {
      //radiation->cleanup(level);
      radiation->close(level);
    }
#endif
}

void
Logi::buildMetrics ()
{
    const int ngrd = grids.size();

    radius.resize(ngrd);

    const Real* dx = geom.CellSize();

    for (int i = 0; i < ngrd; i++)
    {
        const Box& b = grids[i];
        int ilo      = b.smallEnd(0)-radius_grow;
        int ihi      = b.bigEnd(0)+radius_grow;
        int len      = ihi - ilo + 1;

        radius[i].resize(len);

        Real* rad = radius[i].dataPtr();

        if (Geom().IsCartesian())
        {
            for (int j = 0; j < len; j++)
            {
                rad[j] = 1.0;
            }
        }
        else
        {
            RealBox gridloc = RealBox(grids[i],geom.CellSize(),geom.ProbLo());

            const Real xlo = gridloc.lo(0) + (0.5 - radius_grow)*dx[0];

            for (int j = 0; j < len; j++)
            {
                rad[j] = xlo + j*dx[0];
            }
        }
    }

    volume.clear();
    volume.define(grids,dmap,1,NUM_GROW);
    geom.GetVolume(volume);

    for (int dir = 0; dir < AMREX_SPACEDIM; dir++)
    {
        area[dir].clear();
        area[dir].define(getEdgeBoxArray(dir),dmap,1,NUM_GROW);
        geom.GetFaceArea(area[dir],dir);
    }
    for (int dir = AMREX_SPACEDIM; dir < 3; dir++)
    {
        area[dir].clear();
        area[dir].define(grids, dmap, 1, 0);
        area[dir].setVal(0.0);
    }

    dLogArea[0].clear();
#if (AMREX_SPACEDIM <= 2)
    geom.GetDLogA(dLogArea[0],grids,dmap,0,NUM_GROW);
#endif

    wall_time_start = 0.0;
}

// Initialize the MultiFabs and flux registers that live as class members.

void
Logi::initMFs()
{
    fluxes.resize(3);

    for (int dir = 0; dir < AMREX_SPACEDIM; ++dir) {
      fluxes[dir].reset(new MultiFab(getEdgeBoxArray(dir), dmap, NUM_STATE, 0));
    }

    for (int dir = AMREX_SPACEDIM; dir < 3; ++dir) {
      fluxes[dir].reset(new MultiFab(get_new_data(State_Type).boxArray(), dmap, NUM_STATE, 0));
    }

    mass_fluxes.resize(3);

    for (int dir = 0; dir < AMREX_SPACEDIM; ++dir) {
      mass_fluxes[dir].reset(new MultiFab(getEdgeBoxArray(dir), dmap, 1, 0));
    }

    for (int dir = AMREX_SPACEDIM; dir < 3; ++dir) {
      mass_fluxes[dir].reset(new MultiFab(get_new_data(State_Type).boxArray(), dmap, 1, 0));
    }

#if (AMREX_SPACEDIM <= 2)
    if (!Geom().IsCartesian()) {
      P_radial.define(getEdgeBoxArray(0), dmap, 1, 0);
    }
#endif

#ifdef RADIATION
    if (Radiation::rad_hydro_combined) {
        rad_fluxes.resize(AMREX_SPACEDIM);
        for (int dir = 0; dir < AMREX_SPACEDIM; ++dir) {
            rad_fluxes[dir].reset(new MultiFab(getEdgeBoxArray(dir), dmap, Radiation::nGroups, 0));
        }
    }
#endif

    if (do_reflux && level > 0) {

        flux_reg.define(grids, dmap, crse_ratio, level, NUM_STATE);
        flux_reg.setVal(0.0);

#if (AMREX_SPACEDIM < 3)
        if (!Geom().IsCartesian()) {
            pres_reg.define(grids, dmap, crse_ratio, level, 1);
            pres_reg.setVal(0.0);
        }
#endif

#ifdef RADIATION
        if (Radiation::rad_hydro_combined) {
            rad_flux_reg.define(grids, dmap, crse_ratio, level, Radiation::nGroups);
            rad_flux_reg.setVal(0.0);
        }
#endif

    }


#ifdef REACTIONS
    if (store_burn_weights) {
#ifdef STRANG
        // we have 2 components: first half and second half
        burn_weights.define(grids, dmap, 2, 0);
#endif
#ifdef SIMPLIFIED_SDC
        // we have a component for each sdc iteration + 1 extra for retries
        burn_weights.define(grids, dmap, sdc_iters+1, 0);
#endif
        burn_weights.setVal(0.0);
    }
#endif

    // Set the flux register scalings.

    if (do_reflux) {

        flux_crse_scale = -1.0;
        flux_fine_scale = 1.0;

        // The fine pressure scaling depends on dimensionality,
        // as the dimensionality determines the number of
        // adjacent zones. In 1D the face is a point so
        // there's only one fine neighbor for a given coarse
        // face; in 2D there's crse_ratio[1] faces adjacent
        // to a face perpendicular to the radial dimension;
        // and in 3D there would be crse_ratio**2, though
        // we do not separate the pressure out in 3D. Note
        // that the scaling by dt has already been handled
        // in the construction of the P_radial array.

        // The coarse pressure scaling is the same as for the
        // fluxes, we want the total refluxing contribution
        // over the full set of fine timesteps to equal P_radial.

#if (AMREX_SPACEDIM == 1)
        pres_crse_scale = -1.0;
        pres_fine_scale = 1.0;
#elif (AMREX_SPACEDIM == 2)
        pres_crse_scale = -1.0;
        pres_fine_scale = 1.0 / crse_ratio[1];
#endif

    }

    post_step_regrid = 0;

    lastDt = 1.e200;

    if (do_cxx_prob_initialize == 0) {

      // sync up some C++ values of the runtime parameters

      do_cxx_prob_initialize = 1;

      // If we're doing C++ problem initialization, do it here. We have to make
      // sure it's done after the above call to init_prob_parameters() in case
      // any changes are made to the problem parameters.

      problem_initialize();

    }

}

void
Logi::setTimeLevel (Real time,
                      Real dt_old,
                      Real dt_new)
{
    AmrLevel::setTimeLevel(time,dt_old,dt_new);
}


void
Logi::initData ()
{
    BL_PROFILE("Logi::initData()");

    //
    // Loop over grids, call FORTRAN function to init with data.
    //
    const Real* dx  = geom.CellSize();
    const Real* prob_lo = geom.ProbLo();
    MultiFab& S_new = get_new_data(State_Type);
    Real cur_time   = state[State_Type].curTime();

    S_new.setVal(0.);

    // make sure dx = dy = dz -- that's all we guarantee to support
#if (AMREX_SPACEDIM == 2)
    const Real SMALL = 1.e-13;
    if (fabs(dx[0] - dx[1]) > SMALL*dx[0])
      {
        amrex::Abort("We don't support dx != dy");
      }
#elif (AMREX_SPACEDIM == 3)
    const Real SMALL = 1.e-13;
    if ( (fabs(dx[0] - dx[1]) > SMALL*dx[0]) || (fabs(dx[0] - dx[2]) > SMALL*dx[0]) )
      {
        amrex::Abort("We don't support dx != dy != dz");
      }
#endif

    if (verbose && ParallelDescriptor::IOProcessor()) {
      std::cout << "Initializing the data at level " << level << std::endl;
    }

#ifdef MHD
   MultiFab& Bx_new   = get_new_data(Mag_Type_x);
   Bx_new.setVal(0.0);

   MultiFab& By_new   = get_new_data(Mag_Type_y);
   By_new.setVal(0.0);

   MultiFab& Bz_new  =  get_new_data(Mag_Type_z);
   Bz_new.setVal(0.0);

#endif

    // Don't profile for this code, since there will be a lot of host
    // activity and GPU page faults that we're uninterested in.

    Gpu::Device::profilerStop();

#ifdef RADIATION
    // rad quantities are in the state even if (do_radiation == 0)
    MultiFab &Rad_new = get_new_data(Rad_Type);
    Rad_new.setVal(0.);
#endif

#ifdef REACTIONS
    MultiFab &React_new = get_new_data(Reactions_Type);
    React_new.setVal(0.);
#endif

#ifdef SIMPLIFIED_SDC
#ifdef REACTIONS
   if (time_integration_method == SimplifiedSpectralDeferredCorrections) {
       MultiFab& react_src_new = get_new_data(Simplified_SDC_React_Type);
       react_src_new.setVal(0.0, react_src_new.nGrow());
   }
#endif
#endif

#ifdef MAESTRO_INIT
    MAESTRO_init();
#else
    {

#ifdef MHD
       int nbx = Bx_new.nComp();
       int nby = By_new.nComp();
       int nbz = Bz_new.nComp();

       Bx_new.setVal(0.0);
       By_new.setVal(0.0);
       Bz_new.setVal(0.0);

       for (MFIter mfi(S_new); mfi.isValid(); ++mfi) {


          auto geomdata = geom.data();

          auto Bx_arr = Bx_new.array(mfi);
          auto By_arr = By_new.array(mfi);
          auto Bz_arr = Bz_new.array(mfi);

          const Box& box_x = mfi.nodaltilebox(0);

          amrex::ParallelFor(box_x,
          [=] AMREX_GPU_HOST_DEVICE (int i, int j, int k)
          {
              // C++ MHD problem initialization; has no effect if not
              // implemented by a problem setup (defaults to an empty
              // routine).
              problem_initialize_mhd_data(i, j, k, Bx_arr, 0, geomdata);
          });

          const Box& box_y = mfi.nodaltilebox(1);

          amrex::ParallelFor(box_y,
          [=] AMREX_GPU_HOST_DEVICE (int i, int j, int k)
          {
              // C++ MHD problem initialization; has no effect if not
              // implemented by a problem setup (defaults to an empty
              // routine).
              problem_initialize_mhd_data(i, j, k, By_arr, 1, geomdata);
          });

          const Box& box_z = mfi.nodaltilebox(2);

          amrex::ParallelFor(box_z,
          [=] AMREX_GPU_HOST_DEVICE (int i, int j, int k)
          {
              // C++ MHD problem initialization; has no effect if not
              // implemented by a problem setup (defaults to an empty
              // routine).
              problem_initialize_mhd_data(i, j, k, Bz_arr, 2, geomdata);
          });

       }

#endif //MHD

       for (MFIter mfi(S_new); mfi.isValid(); ++mfi)
       {
          const Box& box     = mfi.validbox();
          const int* lo      = box.loVect();
          const int* hi      = box.hiVect();

          auto s = S_new[mfi].array();
          auto geomdata = geom.data();

          amrex::ParallelFor(box,
          [=] AMREX_GPU_HOST_DEVICE (int i, int j, int k)
          {
              // problem initialization
              problem_initialize_state_data(i, j, k, s, geomdata);
          });
       }


#ifdef MHD
      //correct energy density with the magnetic field contribution 
      add_magnetic_e(Bx_new, By_new, Bz_new, S_new);
      
      //check divB
      check_div_B(Bx_new, By_new, Bz_new, S_new);    

#endif

       // it is not a requirement that the problem setup defines the
       // temperature, so we do that here _and_ ensure that we are
       // within any small limits
       computeTemp(
#ifdef MHD
                   Bx_new, By_new, Bz_new,
#endif
                   S_new, cur_time, 0);

       ReduceOps<ReduceOpSum, ReduceOpSum> reduce_op;
       ReduceData<int, int> reduce_data(reduce_op);
       using ReduceTuple = typename decltype(reduce_data)::Type;

#ifdef _OPENMP
#pragma omp parallel
#endif
       for (MFIter mfi(S_new, TilingIfNotGPU()); mfi.isValid(); ++mfi)
       {
           const Box& bx = mfi.tilebox();

           auto S_arr = S_new.array(mfi);

           Real lsmall_temp = small_temp;
           Real lsmall_dens = small_dens;

           reduce_op.eval(bx, reduce_data,
           [=] AMREX_GPU_HOST_DEVICE (int i, int j, int k) -> ReduceTuple
           {
               // if the problem tried to initialize a thermodynamic
               // state that is at or below small_temp, then we abort.
               // This is dangerous and we should recommend a smaller
               // small_temp
               int T_failed = 0;
               if (S_arr(i,j,k,UTEMP) < lsmall_temp * 1.001) {
                   T_failed = 1;
               }

               int rho_failed = 0;
               if (S_arr(i,j,k,URHO) < lsmall_dens * 1.001) {
                   rho_failed = 1;
               }

               return {T_failed, rho_failed};
           });

       }

       ReduceTuple hv = reduce_data.value();
       int init_failed_T   = amrex::get<0>(hv);
       int init_failed_rho = amrex::get<1>(hv);

       if (init_failed_rho != 0) {
         amrex::Error("Error: initial data has rho <~ small_dens");
       }

       if (init_failed_T != 0) {
         amrex::Error("Error: initial data has T <~ small_temp");
       }

#ifdef HYBRID_MOMENTUM
       // Generate the initial hybrid momenta based on this user data.

       linear_to_hybrid_momentum(S_new, 0);
#endif

       // Verify that the sum of (rho X)_i = rho at every cell

       for (MFIter mfi(S_new); mfi.isValid(); ++mfi) {
         const Box& bx = mfi.validbox();

         auto S_arr = S_new.array(mfi);

         amrex::ParallelFor(bx,
         [=] AMREX_GPU_HOST_DEVICE (int i, int j, int k)
         {
           Real spec_sum = 0.0_rt;
           for (int n = 0; n < NumSpec; n++) {
             spec_sum += S_arr(i,j,k,UFS+n);
           }
           if (std::abs(S_arr(i,j,k,URHO) - spec_sum) > 1.e-8_rt * S_arr(i,j,k,URHO)) {
#ifndef AMREX_USE_GPU
             std::cout << "Sum of (rho X)_i vs rho at (i,j,k): " 
                       << i << " " << j << " " << k << " " 
                       << spec_sum << " " << S_arr(i,j,k,URHO) << std::endl;
#endif
             amrex::Error("Error: failed check of initial species summing to 1");
           }
         });
       }

#ifdef TRUE_SDC
       if (initialization_is_cell_average == 0) {
         // we are assuming that the initialization was done to cell-centers

         // Enforce that the total and internal energies are consistent.
         enforce_consistent_e(
#ifdef MHD
                            Bx_new, By_new,Bz_new,
#endif
                            S_new);

         // For fourth-order, we need to convert to cell-averages now.
         // (to second-order, these are cell-averages, so we're done in that case).

#ifndef AMREX_USE_GPU
         if (sdc_order == 4) {
           Sborder.define(grids, dmap, NUM_STATE, NUM_GROW);
           AmrLevel::FillPatch(*this, Sborder, NUM_GROW, cur_time, State_Type, 0, NUM_STATE);

           // note: this cannot be tiled
           auto domain_lo = geom.Domain().loVect3d();
           auto domain_hi = geom.Domain().hiVect3d();

           FArrayBox tmp;

           for (MFIter mfi(S_new); mfi.isValid(); ++mfi)
             {
               const Box& box = mfi.validbox();

               tmp.resize(box, 1);
               Elixir elix_tmp = tmp.elixir();
               auto tmp_arr = tmp.array();

               make_fourth_in_place(box, Sborder.array(mfi), tmp_arr, domain_lo, domain_hi);
             }

           // now copy back the averages
           MultiFab::Copy(S_new, Sborder, 0, 0, NUM_STATE, 0);
           Sborder.clear();
         }
#endif
       } else {

         Sborder.define(grids, dmap, NUM_STATE, NUM_GROW);
         AmrLevel::FillPatch(*this, Sborder, NUM_GROW, cur_time, State_Type, 0, NUM_STATE);

         // convert to centers -- not tile safe
         auto domain_lo = geom.Domain().loVect3d();
         auto domain_hi = geom.Domain().hiVect3d();

         FArrayBox tmp;

         for (MFIter mfi(S_new); mfi.isValid(); ++mfi)
           {
             const Box& box = mfi.growntilebox(2);

             tmp.resize(box, 1);
             Elixir elix_tmp = tmp.elixir();
             auto tmp_arr = tmp.array();

             make_cell_center_in_place(box, Sborder.array(mfi), tmp_arr, domain_lo, domain_hi);
           }

         // reset the energy -- do this in one ghost cell so we can average in place below
         for (MFIter mfi(S_new); mfi.isValid(); ++mfi)
           {
             const Box& box = mfi.growntilebox(1);

             auto S_arr = Sborder.array(mfi);

             amrex::ParallelFor(box,
             [=] AMREX_GPU_HOST_DEVICE (int i, int j, int k)
             {

               Real rhoInv = 1.0_rt / S_arr(i,j,k,URHO);
               Real u = S_arr(i,j,k,UMX) * rhoInv;
               Real v = S_arr(i,j,k,UMY) * rhoInv;
               Real w = S_arr(i,j,k,UMZ) * rhoInv;

               eos_re_t eos_state;
               eos_state.rho = S_arr(i,j,k,URHO);
               eos_state.T = S_arr(i,j,k,UTEMP);
               eos_state.e = S_arr(i,j,k,UEINT) * rhoInv - 0.5_rt * (u*u + v*v + w*w);
               for (int n = 0; n < NumSpec; n++) {
                 eos_state.xn[n] = S_arr(i,j,k,UFS+n) * rhoInv;
               }
#if NAUX_NET > 0
               for (int n = 0; n < NumAux; n++) {
                 eos_state.aux[n] = S_arr(i,j,k,UFX+n) * rhoInv;
               }
#endif

               eos(eos_input_re, eos_state);

               S_arr(i,j,k,UTEMP) = eos_state.T;

               S_arr(i,j,k,UEINT) = eos_state.rho * eos_state.e;
             });
           }

         // convert back to averages -- not tile safe
         for (MFIter mfi(S_new); mfi.isValid(); ++mfi)
           {
             const Box& box = mfi.validbox();

             tmp.resize(box, 1);
             Elixir elix_tmp = tmp.elixir();
             auto tmp_arr = tmp.array();

             make_fourth_in_place(box, Sborder.array(mfi), tmp_arr, domain_lo, domain_hi);
           }

         // now copy back the averages for UEINT and UTEMP only
         MultiFab::Copy(S_new, Sborder, UEINT, UEINT, 1, 0);
         MultiFab::Copy(S_new, Sborder, UTEMP, UTEMP, 1, 0);
         Sborder.clear();

       }
#else
       // Enforce that the total and internal energies are consistent.
       enforce_consistent_e(
#ifdef MHD
                            Bx_new, By_new,Bz_new,
#endif
                            S_new);
#endif

       // Do a FillPatch so that we can get the ghost zones filled.

       int ng = S_new.nGrow();

       if (ng > 0) {
         AmrLevel::FillPatch(*this, S_new, ng, cur_time, State_Type, 0, S_new.nComp());
       }
    }

    clean_state(
#ifdef MHD
                    Bx_new, By_new, Bz_new,
#endif
                    S_new, cur_time, S_new.nGrow());


#ifdef RADIATION
    if (do_radiation) {
      for (MFIter mfi(S_new); mfi.isValid(); ++mfi) {
          int i = mfi.index();

          if (radiation->verbose > 2) {
            std::cout << "Calling RADINIT at level " << level << ", grid "
                 << i << std::endl;
          }

          const Box& box = mfi.validbox();
          const int* lo  = box.loVect();
          const int* hi  = box.hiVect();

          auto r = Rad_new[mfi].array();
          auto geomdata = geom.data();

          GpuArray<Real, NGROUPS+1> xnu_pass = {0.0};
          GpuArray<Real, NGROUPS> nugroup_pass = {0.0};
          GpuArray<Real, NGROUPS> dnugroup_pass = {0.0};
#if NGROUPS > 1
          for (int g = 0; g <= NGROUPS; g++) {
              xnu_pass[g] = radiation->xnu[g];
          }
          for (int g = 0; g < NGROUPS; g++) {
              nugroup_pass[g] = radiation->nugroup[g];
              dnugroup_pass[g] = radiation->dnugroup[g];
          }
#endif

          amrex::ParallelFor(box,
          [=] AMREX_GPU_HOST_DEVICE (int i, int j, int k)
          {
              // C++ problem initialization; has no effect if not implemented
              // by a problem setup (defaults to an empty routine).

              problem_initialize_rad_data(i, j, k, r, xnu_pass, nugroup_pass, dnugroup_pass, geomdata);

          });

      }
    }
#endif // RADIATION

#endif // MAESTRO_INIT

    MultiFab& source_new = get_new_data(Source_Type);
    source_new.setVal(0., source_new.nGrow());

#ifdef AMREX_PARTICLES
    if (level == 0) {
      init_particles();
    }
#endif

    Gpu::Device::profilerStart();

    if (verbose && ParallelDescriptor::IOProcessor()) {
      std::cout << "Done initializing the level " << level << " data " << std::endl;
    }
}

void
Logi::init (AmrLevel &old)
{
    BL_PROFILE("Logi::init(old)");

    Logi* oldlev = (Logi*) &old;

    //
    // Create new grid data by fillpatching from old.
    //
    Real dt_new    = parent->dtLevel(level);
    Real cur_time  = oldlev->state[State_Type].curTime();
    Real prev_time = oldlev->state[State_Type].prevTime();
    Real dt_old    = cur_time - prev_time;
    setTimeLevel(cur_time,dt_old,dt_new);

    for (int s = 0; s < num_state_type; ++s) {
        MultiFab& state_MF = get_new_data(s);
        FillPatch(old, state_MF, state_MF.nGrow(), cur_time, s, 0, state_MF.nComp());
        if (oldlev->state[s].hasOldData()) {
            if (!state[s].hasOldData()) {
                state[s].allocOldData();
            }
            MultiFab& old_state_MF = get_old_data(s);
            FillPatch(old, old_state_MF, old_state_MF.nGrow(), prev_time, s, 0, old_state_MF.nComp());
        }
    }

    // Copy some other data we need from the old class.
    // One reason this is necessary is if we are doing
    // a post-timestep regrid -- then we're going to need
    // to save information about whether there was a retry
    // during the timestep.

    iteration = oldlev->iteration;
    sub_iteration = oldlev->sub_iteration;

    sub_ncycle = oldlev->sub_ncycle;
    dt_subcycle = oldlev->dt_subcycle;
    dt_advance = oldlev->dt_advance;

    keep_prev_state = oldlev->keep_prev_state;

    in_retry = oldlev->in_retry;

}

//
// This version inits the data on a new level that did not
// exist before regridding.
//
void
Logi::init ()
{
    BL_PROFILE("Logi::init()");

    Real dt        = parent->dtLevel(level);
    Real cur_time  = getLevel(level-1).state[State_Type].curTime();
    Real prev_time = getLevel(level-1).state[State_Type].prevTime();

    Real dt_old = (cur_time - prev_time)/(Real)parent->MaxRefRatio(level-1);

    Real time = cur_time;

    // If we just triggered a regrid, we need to account for the fact that
    // the data on the coarse level has already been advanced.

    if (getLevel(level-1).post_step_regrid) {
      time = prev_time;
    }

    setTimeLevel(time,dt_old,dt);

    for (int s = 0; s < num_state_type; ++s) {
        MultiFab& state_MF = get_new_data(s);
        FillCoarsePatch(state_MF, 0, time, s, 0, state_MF.nComp(), state_MF.nGrow());
    }
}

Real
Logi::initialTimeStep ()
{
    Real init_dt  = 0.0;

    if (initial_dt > 0.0)
    {
       init_dt = initial_dt;
    }
    else
    {
       init_dt = init_shrink*estTimeStep();
    }

    return init_dt;
}

Real
Logi::estTimeStep ()
{
    BL_PROFILE("Logi::estTimeStep()");

    if (fixed_dt > 0.0) {
      return fixed_dt;
    }

    Real estdt = max_dt;

    Real time = state[State_Type].curTime();

    std::string limiter = "logi.max_dt";

    // Start the hydro with the max_dt value, but divide by CFL
    // to account for the fact that we multiply by it at the end.
    // This ensures that if max_dt is more restrictive than the hydro
    // criterion, we will get exactly max_dt for a timestep.

    Real estdt_hydro = max_dt / cfl;

    if (do_hydro)
    {

#ifdef RADIATION
        if (Radiation::rad_hydro_combined) {

            estdt_hydro = estdt_rad();

        }
        else
        {
#endif

#ifdef MHD
          estdt_hydro = estdt_mhd();
#else
          estdt_hydro = estdt_cfl(time);
#endif

#ifdef RADIATION
        }
#endif

        ParallelDescriptor::ReduceRealMin(estdt_hydro);
        estdt_hydro *= cfl;
        if (verbose) {
            amrex::Print() << "...estimated hydro-limited timestep at level " << level << ": " << estdt_hydro << std::endl;
        }

        // Determine if this is more restrictive than the maximum timestep limiting

        if (estdt_hydro < estdt) {
            limiter = "hydro";
            estdt = estdt_hydro;
        }

    }

#ifdef DIFFUSION
    // Diffusion-limited timestep
    // Note that the diffusion uses the same CFL safety factor
    // as the main hydrodynamics timestep limiter.

    Real estdt_diffusion = max_dt / cfl;

    if (diffuse_temp)
    {
      estdt_diffusion = estdt_temp_diffusion();
    }

    ParallelDescriptor::ReduceRealMin(estdt_diffusion);
    estdt_diffusion *= cfl;
    if (verbose) {
        amrex::Print() << "...estimated diffusion-limited timestep at level " << level << ": " << estdt_diffusion << std::endl;
    }

    // Determine if this is more restrictive than the hydro limiting

    if (estdt_diffusion < estdt) {
        limiter = "diffusion";
        estdt = estdt_diffusion;
    }
#endif  // diffusion

#ifdef REACTIONS
    // Dummy value to start with
    Real estdt_burn = max_dt;

    if (do_react) {

        // Compute burning-limited timestep.

        estdt_burn = estdt_burning();

        ParallelDescriptor::ReduceRealMin(estdt_burn);

        if (verbose && estdt_burn < max_dt) {
            amrex::Print() << "...estimated burning-limited timestep at level " << level << ": " << estdt_burn << std::endl;
        }

        // Determine if this is more restrictive than the hydro limiting

        if (estdt_burn < estdt) {
            limiter = "burning";
            estdt = estdt_burn;
        }
    }
#endif

#ifdef RADIATION
    if (do_radiation) radiation->EstTimeStep(estdt, level);
#endif

    if (verbose) {
        amrex::Print() << "Logi::estTimeStep (" << limiter << "-limited) at level " << level << ":  estdt = " << estdt << '\n' << std::endl;
    }

    return estdt;
}

void
Logi::computeNewDt (int                    finest_level,
                      int                    /*sub_cycle*/,
                      Vector<int>&           n_cycle,
                      const Vector<IntVect>& /*ref_ratio*/,
                      Vector<Real>&          dt_min,
                      Vector<Real>&          dt_level,
                      Real                   stop_time,
                      int                    post_regrid_flag)
{
    BL_PROFILE("Logi::computeNewDt()");

    //
    // We are at the start of a coarse grid timecycle.
    // Compute the timesteps for the next iteration.
    //
    if (level > 0)
        return;

    Real dt_0 = 1.0e+100;
    int n_factor = 1;
    for (int i = 0; i <= finest_level; i++)
    {
        Logi& adv_level = getLevel(i);
        dt_min[i] = adv_level.estTimeStep();
    }

    if (fixed_dt <= 0.0)
    {
       if (post_regrid_flag == 1)
       {
          //
          // Limit dt's by pre-regrid dt
          //
          for (int i = 0; i <= finest_level; i++)
          {
              dt_min[i] = std::min(dt_min[i],dt_level[i]);
          }
       }
       else
       {
          //
          // Limit dt's by change_max * old dt,
          // if we didn't limit the last timestep
          // to hit a plotfile interval.
          //
          if (lastDtPlotLimited) {

              dt_min[0] = std::min(dt_min[0], lastDtBeforePlotLimiting);

              lastDtPlotLimited = 0;
              lastDtBeforePlotLimiting = 0.0;

          }
          else {

              for (int i = 0; i <= finest_level; i++)
              {
                  if (verbose && ParallelDescriptor::IOProcessor()) {
                    if (dt_min[i] > change_max*dt_level[i])
                      {
                          std::cout << "Logi::compute_new_dt : limiting dt at level "
                                    << i << '\n';
                          std::cout << " ... new dt computed: " << dt_min[i]
                                    << '\n';
                          std::cout << " ... but limiting to: "
                                    << change_max * dt_level[i] << " = " << change_max
                                    << " * " << dt_level[i] << '\n';
                      }
                  }
                  dt_min[i] = std::min(dt_min[i],change_max*dt_level[i]);
              }

          }
       }
    }

    //
    // Find the minimum over all levels
    //
    for (int i = 0; i <= finest_level; i++)
    {
        n_factor *= n_cycle[i];
        dt_0 = std::min(dt_0,n_factor*dt_min[i]);
    }

    //
    // Optionally, limit dt's by the value of
    // plot_per or small_plot_per.
    //
    if (plot_per_is_exact) {

        const Real plot_per = parent->plotPer();

        if (plot_per > 0.0) {

            const Real cur_time = state[State_Type].curTime();

            // Calculate the new dt by comparing to the dt needed to get
            // to the next multiple of plot_per.

            const Real dtMod = std::fmod(cur_time, plot_per);

            Real newPlotDt;

            // Note that if we are just about exactly on a multiple of plot_per,
            // then we need to be careful to avoid floating point issues.

            if (std::abs(dtMod - plot_per) <= std::numeric_limits<Real>::epsilon() * cur_time) {
                newPlotDt = plot_per + (plot_per - dtMod);
            }
            else {
                newPlotDt = plot_per - dtMod;
            }

            if (newPlotDt < dt_0) {
                lastDtPlotLimited = 1;
                lastDtBeforePlotLimiting = dt_0;
                dt_0 = newPlotDt;

                // Avoid taking timesteps that are so small that
                // they may cause problems in the hydrodynamics.

                const Real epsDt = 1.e-4 * lastDtBeforePlotLimiting;
                dt_0 = std::max(dt_0, epsDt);

                if (verbose) {
                  amrex::Print() << " ... limiting dt to " << dt_0 << " to hit the next plot interval.\n";
                }
            }

        }

    }

    if (small_plot_per_is_exact) {

        const Real small_plot_per = parent->smallplotPer();

        if (small_plot_per > 0.0) {

            const Real cur_time = state[State_Type].curTime();

            // Same logic as for plot_per_is_exact.

            const Real dtMod = std::fmod(cur_time, small_plot_per);

            Real newSmallPlotDt;

            if (std::abs(dtMod - small_plot_per) <= std::numeric_limits<Real>::epsilon() * cur_time) {
                newSmallPlotDt = small_plot_per + (small_plot_per - dtMod);
            }
            else {
                newSmallPlotDt = small_plot_per - dtMod;
            }

            if (newSmallPlotDt < dt_0) {
                lastDtPlotLimited = 1;
                lastDtBeforePlotLimiting = dt_0;
                dt_0 = newSmallPlotDt;

                const Real epsDt = 1.e-4 * lastDtBeforePlotLimiting;
                dt_0 = std::max(dt_0, epsDt);

                if (verbose)
                    amrex::Print() << " ... limiting dt to " << dt_0 << " to hit the next smallplot interval.\n";
            }

        }

    }

    //
    // Limit dt's by the value of stop_time.
    //
    const Real eps = std::numeric_limits<Real>::epsilon();
    Real cur_time = state[State_Type].curTime();
    if (stop_time >= 0.0) {
        if ((cur_time + dt_0) >= (stop_time - eps)) {
            dt_0 = stop_time - cur_time;
            if (verbose)
                amrex::Print() << " ... limiting dt to " << dt_0 << " to hit the stop_time.\n";
        }
    }

    n_factor = 1;
    for (int i = 0; i <= finest_level; i++)
    {
        n_factor *= n_cycle[i];
        dt_level[i] = dt_0/n_factor;
    }
}

void
Logi::computeInitialDt (int                   finest_level,
                          int                   /*subcycle*/,
                          Vector<int>&           n_cycle,
                          const Vector<IntVect>& /*ref_ratio*/,
                          Vector<Real>&          dt_level,
                          Real                  stop_time)
{
    BL_PROFILE("Logi::computeInitialDt()");

    //
    // Grids have been constructed, compute dt for all levels.
    //
    if (level > 0) {
      return;
    }

    int i;

    Real dt_0 = 1.0e+100;
    int n_factor = 1;
    // TODO/DEBUG: This will need to change for optimal subcycling.
    for (i = 0; i <= finest_level; i++)
    {
        dt_level[i] = getLevel(i).initialTimeStep();
        n_factor   *= n_cycle[i];
        dt_0 = std::min(dt_0,n_factor*dt_level[i]);
    }

    //
    // Limit dt's by the value of stop_time.
    //
    const Real eps = 0.001*dt_0;
    Real cur_time  = state[State_Type].curTime();
    if (stop_time >= 0.0) {
        if ((cur_time + dt_0) > (stop_time - eps)) {
          dt_0 = stop_time - cur_time;
        }
    }

    n_factor = 1;
    for (i = 0; i <= finest_level; i++)
    {
        n_factor *= n_cycle[i];
        dt_level[i] = dt_0/n_factor;
    }
}

void
Logi::post_timestep (int iteration_local)
{
    BL_PROFILE("Logi::post_timestep()");

    //
    // Integration cycle on fine level grids is complete
    // do post_timestep stuff here.
    //
    int finest_level = parent->finestLevel();

#ifdef RADIATION
    if (do_radiation && (level < finest_level)) {
        // computeTemp is not needed before or after this call because
        // setup for deferred sync does not touch state, only flux registers.
        radiation->deferred_sync_setup(level);

        if (do_reflux) {
            radiation->reflux(level);
            // Since radiation->reflux does not touch the fluid state,
            // we do need to recompute Temp here.
        }
    }
#endif

    // Now do the refluxing. If we're using gravity it
    // will also do the sync solve associated with the reflux.

    if (do_reflux && level < parent->finestLevel()) {
      reflux(level, level+1);
    }

    // Ensure consistency with finer grids.

    if (level < finest_level)
        avgDown();


#ifdef MHD
    MultiFab& Bx_new = get_new_data(Mag_Type_x);
    MultiFab& By_new = get_new_data(Mag_Type_y);
    MultiFab& Bz_new = get_new_data(Mag_Type_z);
#endif

    // Clean up any aberrant state data generated by the reflux and average-down,
    // and then update quantities like temperature to be consistent.
    MultiFab& S_new = get_new_data(State_Type);
    clean_state(
#ifdef MHD
                Bx_new, By_new, Bz_new,
#endif
                S_new, state[State_Type].curTime(), S_new.nGrow());


#ifdef DO_PROBLEM_POST_TIMESTEP

    // Provide a hook for the user to do things after all of
    // the normal updates have been applied. The user is
    // responsible for any actions after this point, like
    // doing a computeTemp call if they change the state data.

    problem_post_timestep();

#endif

    if (level == 0)
    {
        int nstep = parent->levelSteps(0);
        Real dtlev = parent->dtLevel(0);
        Real cumtime = parent->cumTime() + dtlev;

        bool sum_int_test = false;

        if (sum_interval > 0) {

          if (nstep%sum_interval == 0) {
            sum_int_test = true;
          }

        }

        bool sum_per_test = false;

        if (sum_per > 0.0) {

          const int num_per_old = static_cast<int>(std::floor((cumtime - dtlev) / sum_per));
          const int num_per_new = static_cast<int>(std::floor((cumtime        ) / sum_per));

          if (num_per_old != num_per_new) {
            sum_per_test = true;
          }

        }

        if (sum_int_test || sum_per_test) {
          sum_integrated_quantities();
        }

    }

#ifdef RADIATION
    // diagnostic stuff

    if (level == 0) {
      do_energy_diagnostics();
    }
#endif

#ifdef AMREX_PARTICLES
    if (TracerPC)
    {
        const int ncycle = parent->nCycle(level);
        //
        // Don't redistribute/timestamp on the final subiteration except on the coarsest grid.
        //
        if (iteration_local < ncycle || level == 0)
        {
            int ngrow = (level == 0) ? 0 : iteration_local;

            TracerPC->Redistribute(level, parent->finestLevel(), ngrow);

            TimestampParticles(ngrow+1);
        }
    }
#endif
}

void
Logi::post_restart ()
{
   BL_PROFILE("Logi::post_restart()");

#ifdef AMREX_PARTICLES
   ParticlePostRestart(parent->theRestartFile());
#endif

#ifdef DIFFUSION
      // diffusion is a static object, only alloc if not already there
      if (diffusion == 0)
        diffusion = new Diffusion(parent,&phys_bc);

      if (level == 0)
         for (int lev = 0; lev <= parent->finestLevel(); lev++) {
            AmrLevel& this_level = getLevel(lev);
                Logi& cs_level = getLevel(lev);
            diffusion->install_level(lev,&this_level,
                                     cs_level.Volume(),cs_level.Area());
         }
#endif

#ifdef DO_PROBLEM_POST_RESTART
    problem_post_restart();
#endif

}

void
Logi::postCoarseTimeStep (Real cumtime)
{
    // postCoarseTimeStep() is only called by level 0.
    BL_ASSERT(level == 0);
    AmrLevel::postCoarseTimeStep(cumtime);
}

void
Logi::post_regrid (int lbase,
                     int new_finest)
{

    BL_PROFILE("Logi::post_regrid()");

    fine_mask.clear();

#ifdef AMREX_PARTICLES
    if (TracerPC && level == lbase) {
        TracerPC->Redistribute(lbase);
    }
#endif

    // Ensure regridded data is valid. This addresses the fact that data
    // on this level that was interpolated from a coarser level may not
    // respect the consistency between certain state variables
    // (e.g. UEINT and UEDEN) that we demand in every zone.

    if (state[State_Type].hasOldData()) {

        MultiFab& S_old = get_old_data(State_Type);

        clean_state(
#ifdef MHD
                    get_old_data(Mag_Type_x),
                    get_old_data(Mag_Type_y),
                    get_old_data(Mag_Type_z),
#endif
                    S_old, state[State_Type].prevTime(), S_old.nGrow());

    }

    if (state[State_Type].hasNewData()) {

        MultiFab& S_new = get_new_data(State_Type);

        clean_state(
#ifdef MHD
                    get_new_data(Mag_Type_x),
                    get_new_data(Mag_Type_y),
                    get_new_data(Mag_Type_z),
#endif
                    S_new, state[State_Type].curTime(), S_new.nGrow());

    }
}

void
Logi::post_init (Real /*stop_time*/)
{
    BL_PROFILE("Logi::post_init()");

    if (level > 0) {
        return;
    }

    //
    // Average data down from finer levels
    // so that conserved data is consistent between levels.
    //
    int finest_level = parent->finestLevel();
    for (int k = finest_level-1; k>= 0; k--) {
      getLevel(k).avgDown();
    }

#ifdef RADIATION
    if (do_radiation) {
      // The option of whether to do a multilevel initialization is
      // controlled within the radiation class.

      radiation->post_init(level);

      for (int k = finest_level-1; k>= 0; k--) {
        getLevel(k).avgDown(Rad_Type);
      }

      do_energy_diagnostics();

      // re-estimate the initial timestep using the initialized
      // fine-level data and radiation field.
      //if (level == 0)
      //post_init_estDT();
    }
#endif

// Allow the user to define their own post_init functions.

#ifdef DO_PROBLEM_POST_INIT

    problem_post_init();

#endif

    // If we're doing SCF initialization, do it here.

        int nstep = parent->levelSteps(0);
        Real dtlev = parent->dtLevel(0);
        Real cumtime = parent->cumTime();
        if (cumtime != 0.0) cumtime += dtlev;

        bool sum_int_test = false;

        if (sum_interval > 0) {

          if (nstep%sum_interval == 0) {
            sum_int_test = true;
          }
        }

        bool sum_per_test = false;

        if (sum_per > 0.0) {

          const int num_per_old = static_cast<int>(std::floor((cumtime - dtlev) / sum_per));
          const int num_per_new = static_cast<int>(std::floor((cumtime        ) / sum_per));

          if (num_per_old != num_per_new) {
            sum_per_test = true;
          }

        }

        if (sum_int_test || sum_per_test) {
          sum_integrated_quantities();
        }

}

void
Logi::post_grown_restart ()
{

    BL_PROFILE("Logi::post_grown_restart()");

    if (level > 0)
        return;

#ifdef RADIATION
    if (do_radiation) {
      // The option of whether to do a multilevel initialization is
      // controlled within the radiation class.

      radiation->post_init(level);

      int finest_level = parent->finestLevel();

      for (int k = finest_level-1; k>= 0; k--) {
        getLevel(k).avgDown(Rad_Type);
      }

      do_energy_diagnostics();

      // re-estimate the initial timestep using the initialized
      // fine-level data and radiation field.
      //if (level == 0)
      //post_init_estDT();
    }
#endif
}

int
Logi::okToContinue ()
{
    if (level > 0) {
      return 1;
    }

    int test = 1;

    if (signalStopJob) {
      test = 0;
      if (ParallelDescriptor::IOProcessor()) {
        std::cout << " Signalling a stop of the run due to signalStopJob = true." << std::endl;
      }
    }
    else if (parent->dtLevel(level) < dt_cutoff * parent->cumTime()) {
      test = 0;
      if (ParallelDescriptor::IOProcessor()) {
        std::cout << " Signalling a stop of the run because dt < dt_cutoff * time." << std::endl;
      }
    }

    return test;
}

#ifdef AUX_UPDATE
void
Logi::advance_aux(Real time, Real dt)
{
    BL_PROFILE("Logi::advance_aux()");

    if (verbose && ParallelDescriptor::IOProcessor()) {
      std::cout << "... special update for auxiliary variables \n";
    }

    MultiFab&  S_old = get_old_data(State_Type);
    MultiFab&  S_new = get_new_data(State_Type);

#ifdef _OPENMP
#pragma omp parallel
#endif
    for (MFIter mfi(S_old, TilingIfNotGPU()); mfi.isValid(); ++mfi)
    {
        const Box& box = mfi.tilebox();
        FArrayBox& old_fab = S_old[mfi];
        FArrayBox& new_fab = S_new[mfi];
        void ca_auxupdate(BL_TO_FORTRAN(old_fab),
                          BL_TO_FORTRAN(new_fab),
                          box.loVect(), box.hiVect(),
                          &dt);
    }
}
#endif


void
Logi::FluxRegCrseInit() {

    if (level == parent->finestLevel()) {
      return;
    }

    Logi& fine_level = getLevel(level+1);

    for (int i = 0; i < AMREX_SPACEDIM; ++i) {
      fine_level.flux_reg.CrseInit(*fluxes[i], i, 0, 0, NUM_STATE, flux_crse_scale);
    }

#if (AMREX_SPACEDIM <= 2)
    if (!Geom().IsCartesian()) {
      fine_level.pres_reg.CrseInit(P_radial, 0, 0, 0, 1, pres_crse_scale);
    }
#endif

#ifdef RADIATION
    if (Radiation::rad_hydro_combined) {
      for (int i = 0; i < AMREX_SPACEDIM; ++i) {
        fine_level.rad_flux_reg.CrseInit(*rad_fluxes[i], i, 0, 0, Radiation::nGroups, flux_crse_scale);
      }
    }
#endif

}


void
Logi::FluxRegFineAdd() {

    BL_PROFILE("Logi::FluxRegFineAdd()");

    if (level == 0) {
      return;
    }

    for (int i = 0; i < AMREX_SPACEDIM; ++i) {
      flux_reg.FineAdd(*fluxes[i], i, 0, 0, NUM_STATE, flux_fine_scale);
    }

#if (AMREX_SPACEDIM <= 2)
    if (!Geom().IsCartesian()) {
      getLevel(level).pres_reg.FineAdd(P_radial, 0, 0, 0, 1, pres_fine_scale);
    }
#endif

#ifdef RADIATION
    if (Radiation::rad_hydro_combined) {
      for (int i = 0; i < AMREX_SPACEDIM; ++i) {
        getLevel(level).rad_flux_reg.FineAdd(*rad_fluxes[i], i, 0, 0, Radiation::nGroups, flux_fine_scale);
      }
    }
#endif

}


void
Logi::reflux(int crse_level, int fine_level)
{
    BL_PROFILE("Logi::reflux()");

    BL_ASSERT(fine_level > crse_level);

    const Real strt = ParallelDescriptor::second();

    FluxRegister* reg;

    for (int lev = fine_level; lev > crse_level; --lev) {

        reg = &getLevel(lev).flux_reg;

        Logi& crse_lev = getLevel(lev-1);

        MultiFab& crse_state = crse_lev.get_new_data(State_Type);

        // Clear out the data that's not on coarse-fine boundaries so that this register only
        // modifies the fluxes on coarse-fine interfaces.

        reg->ClearInternalBorders(crse_lev.geom);

        // The reflux operation can cause a small or negative density (for the same reason this
        // can happen during the level advance). We want to avoid this scenario because our
        // only recourse will be a density reset, which is disruptive. So before we apply the reflux,
        // we need to clean up the flux register to limit any fluxes that would do this.
        // This is nonconservative, since we do not go back and retroactively apply any
        // correction on the fine grid. (Of course, a density reset would also be nonconservative.)
        // We assume that the amount of fluid material lost this way is small since refluxes
        // causing a small density should only happen around ambient material.

        // The simplest way to do this is make a copy of the flux register to a MultiFab,
        // then loop through the data and calculate what the reflux operation would be,
        // zeroing out the flux if it would result in a negative density. (An alternate
        // approach might be to use limit_hydro_fluxes_on_small_dens().) Then we overwrite
        // the flux register with the updated data. This is more straightforward than operating
        // on the flux register data directly, and we will anyway need this copy of the flux
        // data in MultiFab form later.

        for (int idir = 0; idir < AMREX_SPACEDIM; ++idir) {

            MultiFab temp_fluxes(crse_lev.fluxes[idir]->boxArray(),
                                 crse_lev.fluxes[idir]->DistributionMap(),
                                 crse_lev.fluxes[idir]->nComp(), crse_lev.fluxes[idir]->nGrow());

            temp_fluxes.setVal(0.0);

            // Start with a MultiFab version of the flux register.

            for (OrientationIter fi; fi; ++fi) {
                const FabSet& fs = (*reg)[fi()];
                if (fi().coordDir() == idir) {
                    fs.copyTo(temp_fluxes, 0, 0, 0, temp_fluxes.nComp());
                }
            }

            // Now zero out any problematic flux corrections.

            for (MFIter mfi(crse_state, TilingIfNotGPU()); mfi.isValid(); ++mfi) {
                const Box& bx = mfi.tilebox();
                const Box& nbx = amrex::surroundingNodes(bx, idir);

                auto U = crse_state[mfi].array();
                auto V = crse_lev.volume[mfi].array();
                auto F = temp_fluxes[mfi].array();
                Real dt = parent->dtLevel(lev);

                amrex::ParallelFor(nbx,
                [=] AMREX_GPU_DEVICE (int i, int j, int k)
                {
                    // Note in this check we will zero out the flux if either
                    // the left or right zone would go negative due to this update.
                    // Only one of the two zones would actually be updated (whichever
                    // is the one not covered by the fine grid) but it is easier to
                    // ignore that and just check both sides here instead of figuring
                    // out which zone is the coarse-only zone.

                    bool zero_flux = false;

                    if (bx.contains(i,j,k)) {
                        if (U(i,j,k,URHO) + dt * F(i,j,k,URHO) / V(i,j,k) < logi::small_dens) {
                            zero_flux = true;
                        }
                    }

                    if (idir == 0) {
                        if (bx.contains(i-1,j,k)) {
                            if (U(i-1,j,k,URHO) - dt * F(i,j,k,URHO) / V(i-1,j,k) < logi::small_dens) {
                                zero_flux = true;
                            }
                        }
                    }
                    else if (idir == 1) {
                        if (bx.contains(i,j-1,k)) {
                            if (U(i,j-1,k,URHO) - dt * F(i,j,k,URHO) / V(i,j-1,k) < logi::small_dens) {
                                zero_flux = true;
                            }
                        }
                    }
                    else if (idir == 2) {
                        if (bx.contains(i,j,k-1)) {
                            if (U(i,j,k-1,URHO) - dt * F(i,j,k,URHO) / V(i,j,k-1) < logi::small_dens) {
                                zero_flux = true;
                            }
                        }
                    }

                    if (zero_flux) {
                        for (int n = 0; n < NUM_STATE; ++n) {
                            F(i,j,k,n) = 0.0;
                        }
                    }
                });
            }

            // Update the flux register now that we may have modified some of the flux corrections.

            for (OrientationIter fi; fi; ++fi) {
                FabSet& fs = (*reg)[fi()];
                if (fi().coordDir() == idir) {
                    fs.copyFrom(temp_fluxes, 0, 0, 0, temp_fluxes.nComp());
                }
            }

            // Update the coarse fluxes MultiFabs using the reflux data. This should only make
            // a difference if we re-evaluate the source terms later.

            if (update_sources_after_reflux) {

                MultiFab::Add(*crse_lev.fluxes[idir], temp_fluxes, 0, 0, crse_lev.fluxes[idir]->nComp(), 0);

                // The gravity and rotation source terms depend on the mass fluxes.
                // These should be the same as the URHO component of the fluxes.
                // This update must be a copy from the fluxes rather than an add
                // from the flux register because the mass fluxes only represent
                // the last subcycle of the previous timestep.

                MultiFab::Copy(*crse_lev.mass_fluxes[idir], *crse_lev.fluxes[idir], URHO, 0, 1, 0);
            }

        }

        // Trigger the actual reflux on the coarse level now.

        reg->Reflux(crse_state, crse_lev.volume, 1.0, 0, 0, NUM_STATE, crse_lev.geom);

        // We no longer need the flux register data, so clear it out.

        reg->setVal(0.0);

#if (AMREX_SPACEDIM <= 2)
        if (!Geom().IsCartesian()) {

            reg = &getLevel(lev).pres_reg;

            MultiFab dr(crse_lev.grids, crse_lev.dmap, 1, 0);
            dr.setVal(crse_lev.geom.CellSize(0));

            reg->ClearInternalBorders(crse_lev.geom);

            reg->Reflux(crse_state, dr, 1.0, 0, UMX, 1, crse_lev.geom);

            if (update_sources_after_reflux) {

                MultiFab temp_fluxes(crse_lev.P_radial.boxArray(),
                                     crse_lev.P_radial.DistributionMap(),
                                     crse_lev.P_radial.nComp(), crse_lev.P_radial.nGrow());

                temp_fluxes.setVal(0.0);

                for (OrientationIter fi; fi; ++fi)
                {
                    const FabSet& fs = (*reg)[fi()];
                    if (fi().coordDir() == 0) {
                        fs.copyTo(temp_fluxes, 0, 0, 0, temp_fluxes.nComp());
                    }
                }

                MultiFab::Add(crse_lev.P_radial, temp_fluxes, 0, 0, crse_lev.P_radial.nComp(), 0);

            }

            reg->setVal(0.0);

        }
#endif

#ifdef RADIATION

        // This follows the same logic as the pure hydro fluxes; see above for details.

        if (Radiation::rad_hydro_combined) {

            reg = &getLevel(lev).rad_flux_reg;

            reg->ClearInternalBorders(crse_lev.geom);

            reg->Reflux(crse_lev.get_new_data(Rad_Type), crse_lev.volume, 1.0, 0, 0, Radiation::nGroups, crse_lev.geom);

            if (update_sources_after_reflux) {

                for (int idir = 0; idir < AMREX_SPACEDIM; ++idir) {

                    MultiFab temp_fluxes(crse_lev.rad_fluxes[idir]->boxArray(),
                                         crse_lev.rad_fluxes[idir]->DistributionMap(),
                                         crse_lev.rad_fluxes[idir]->nComp(), crse_lev.rad_fluxes[idir]->nGrow());

                    temp_fluxes.setVal(0.0);

                    for (OrientationIter fi; fi; ++fi) {
                        const FabSet& fs = (*reg)[fi()];
                        if (fi().coordDir() == idir) {
                            fs.copyTo(temp_fluxes, 0, 0, 0, temp_fluxes.nComp());
                        }
                    }

                    MultiFab::Add(*crse_lev.rad_fluxes[idir], temp_fluxes, 0, 0, crse_lev.rad_fluxes[idir]->nComp(), 0);

                }

            }

            reg->setVal(0.0);

        }

#endif

    }

    // Now subtract the new-time updates to the state data,
    // recompute it, and add it back. This corrects for the fact
    // that the new-time data was calculated the first time around
    // using a state that hadn't yet been refluxed. Note that this
    // needs to come after the gravity sync solve because the gravity
    // source depends on an up-to-date value of phi. We'll do this
    // on the fine level in addition to the coarser levels, because
    // global sources like gravity or source terms that rely on
    // ghost zone fills like diffusion depend on the data in the
    // coarser levels.

    if (update_sources_after_reflux &&
        (time_integration_method == CornerTransportUpwind ||
         time_integration_method == SimplifiedSpectralDeferredCorrections)) {

        for (int lev = fine_level; lev >= crse_level; --lev) {

            MultiFab& S_old = getLevel(lev).get_old_data(State_Type);

            MultiFab& S_new = getLevel(lev).get_new_data(State_Type);
#ifdef MHD
            MultiFab& Bx_new = getLevel(lev).get_new_data(Mag_Type_x);
            MultiFab& By_new = getLevel(lev).get_new_data(Mag_Type_y);
            MultiFab& Bz_new = getLevel(lev).get_new_data(Mag_Type_z);
#endif
            MultiFab& source = getLevel(lev).get_new_data(Source_Type);
            Real time = getLevel(lev).state[State_Type].curTime();
            Real dt_advance_local = getLevel(lev).dt_advance; // Note that this may be shorter than the full timestep due to subcycling.
            Real dt_amr = parent->dtLevel(lev); // The full timestep expected by the Amr class.

            if (getLevel(lev).apply_sources()) {

                getLevel(lev).apply_source_to_state(S_new, source, -dt_advance_local, 0);
                getLevel(lev).clean_state(
#ifdef MHD
                                          Bx_new, By_new, Bz_new,
#endif
                                          S_new, time, 0);

            }

            // Temporarily restore the last iteration's old data for the purposes of recalculating the corrector.
            // This is only necessary if we've done subcycles on that level.

            if (use_retry && dt_advance_local < dt_amr && getLevel(lev).keep_prev_state) {

                for (int k = 0; k < num_state_type; k++) {

                    if (getLevel(lev).prev_state[k]->hasOldData()) {

                        // Use the new-time data as a temporary buffer. Ideally this would be done
                        // as a pointer swap, but we cannot assume that the distribution mapping
                        // is the same between the current state and the original state, due to
                        // possible regrids that have occurred in between.

                        MultiFab& old = getLevel(lev).get_old_data(k);
                        MultiFab::Copy(getLevel(lev).prev_state[k]->newData(), old, 0, 0, old.nComp(), old.nGrow());
                        MultiFab::Copy(old, getLevel(lev).prev_state[k]->oldData(), 0, 0, old.nComp(), old.nGrow());

                        getLevel(lev).state[k].setTimeLevel(time, dt_advance_local, 0.0);
                        getLevel(lev).prev_state[k]->setTimeLevel(time, dt_amr, 0.0);

                    }

                }

            }

            if (getLevel(lev).apply_sources()) {
                bool apply_sources_to_state = true;
                getLevel(lev).do_new_sources(
#ifdef MHD
                                Bx_new, By_new, Bz_new,
#endif
                                source, S_old, S_new, time, dt_advance_local, apply_sources_to_state);
            }

            if (use_retry && dt_advance_local < dt_amr && getLevel(lev).keep_prev_state) {

                for (int k = 0; k < num_state_type; k++) {

                    if (getLevel(lev).prev_state[k]->hasOldData()) {

                        // Now retrieve the original old time data.

                        MultiFab& old = getLevel(lev).get_old_data(k);
                        MultiFab::Copy(old, getLevel(lev).prev_state[k]->newData(), 0, 0, old.nComp(), old.nGrow());

                        getLevel(lev).state[k].setTimeLevel(time, dt_amr, 0.0);
                        getLevel(lev).prev_state[k]->setTimeLevel(time, dt_advance_local, 0.0);

                    }

                }

                // Now deallocate the old data, it is no longer needed.

                if (lev == 0 || lev > level) {
                    amrex::FillNull(getLevel(lev).prev_state);
                    getLevel(lev).keep_prev_state = false;
                }

            }

        }

    }

    if (verbose)
    {
        const int IOProc = ParallelDescriptor::IOProcessorNumber();
        Real      end    = ParallelDescriptor::second() - strt;

#ifdef BL_LAZY
        Lazy::QueueReduction( [=] () mutable {
#endif
        ParallelDescriptor::ReduceRealMax(end,IOProc);
        if (ParallelDescriptor::IOProcessor()) {
          std::cout << "Logi::reflux() at level " << level << " : time = " << end << std::endl;
        }
#ifdef BL_LAZY
        });
#endif
    }
}

void
Logi::avgDown ()
{
  BL_PROFILE("Logi::avgDown()");

  if (level == parent->finestLevel()) return;

  for (int k = 0; k < num_state_type; k++) {
      avgDown(k);
  }

}

void
Logi::normalize_species (MultiFab& S_new, int ng)
{
    BL_PROFILE("Logi::normalize_species()");

    Real lsmall_x = small_x;

#ifdef _OPENMP
#pragma omp parallel
#endif
    for (MFIter mfi(S_new, TilingIfNotGPU()); mfi.isValid(); ++mfi)
    {
        const Box& bx = mfi.growntilebox(ng);

        auto u = S_new.array(mfi);

        // Ensure the species mass fractions are between small_x and 1,
        // then normalize them so that they sum to 1.

        amrex::ParallelFor(bx,
        [=] AMREX_GPU_HOST_DEVICE (int i, int j, int k)
        {
            Real rhoX_sum = 0.0_rt;
            Real rhoInv = 1.0_rt / u(i,j,k,URHO);

            for (int n = 0; n < NumSpec; ++n) {
#ifndef AMREX_USE_GPU
                const Real X_failure_tolerance = 1.e-2_rt;

                // Abort if X is unphysically large.
                Real X = u(i,j,k,UFS+n) * rhoInv;

                if (X < -X_failure_tolerance || X > 1.0_rt + X_failure_tolerance) {
                    std::cout << "(i, j, k) = " << i << " " << j << " " << k << " " << ", X[" << n << "] = " << X << std::endl;
                    amrex::Error("Invalid mass fraction in Logi::normalize_species()");
                }
#endif
                u(i,j,k,UFS+n) = amrex::max(lsmall_x * u(i,j,k,URHO), amrex::min(u(i,j,k,URHO), u(i,j,k,UFS+n)));
                rhoX_sum += u(i,j,k,UFS+n);
            }

            Real fac = u(i,j,k,URHO) / rhoX_sum;

            for (int n = 0; n < NumSpec; ++n) {
                u(i,j,k,UFS+n) *= fac;
            }
        });
    }
}

void
Logi::enforce_consistent_e (
#ifdef MHD
                              MultiFab& Bx,
                              MultiFab& By,
                              MultiFab& Bz,
#endif
                              MultiFab& S)
{

    BL_PROFILE("Logi::enforce_consistent_e()");

#ifdef _OPENMP
#pragma omp parallel
#endif
    for (MFIter mfi(S, TilingIfNotGPU()); mfi.isValid(); ++mfi)
    {
        const Box& box     = mfi.tilebox();

        auto S_arr = S.array(mfi);

#ifdef MHD
        auto Bx_arr = Bx.array(mfi);
        auto By_arr = By.array(mfi);
        auto Bz_arr = Bz.array(mfi);
#endif

        ParallelFor(box,
        [=] AMREX_GPU_HOST_DEVICE (int i, int j, int k)
        {
          Real rhoInv = 1.0_rt / S_arr(i,j,k,URHO);
          Real u = S_arr(i,j,k,UMX) * rhoInv;
          Real v = S_arr(i,j,k,UMY) * rhoInv;
          Real w = S_arr(i,j,k,UMZ) * rhoInv;

          S_arr(i,j,k,UEDEN) = S_arr(i,j,k,UEINT) +
            0.5_rt * S_arr(i,j,k,URHO) * (u*u + v*v + w*w);

#ifdef MHD
          Real bx_cell_c = 0.5_rt * (Bx_arr(i,j,k) + Bx_arr(i+1,j,k));
          Real by_cell_c = 0.5_rt * (By_arr(i,j,k) + By_arr(i,j+1,k));
          Real bz_cell_c = 0.5_rt * (Bz_arr(i,j,k) + Bz_arr(i,j,k+1));

          S_arr(i,j,k,UEDEN) += 0.5_rt * (bx_cell_c * bx_cell_c +
                                          by_cell_c * by_cell_c +
                                          bz_cell_c * bz_cell_c);
#endif

        });

    }
}

void
Logi::enforce_min_density (MultiFab& state_in, int ng)
{

    BL_PROFILE("Logi::enforce_min_density()");

    // This routine sets the density in state_in to be larger than the
    // density floor.  Note that it will operate everywhere on state_in,
    // including ghost zones.

    MultiFab reset_source;

    if (print_update_diagnostics)
    {

        // Before we do anything, make a copy of the state.

        reset_source.define(state_in.boxArray(), state_in.DistributionMap(), state_in.nComp(), 0);

        MultiFab::Copy(reset_source, state_in, 0, 0, state_in.nComp(), 0);

    }

#ifdef _OPENMP
#pragma omp parallel
#endif
    for (MFIter mfi(state_in, TilingIfNotGPU()); mfi.isValid(); ++mfi) {

        const Box& bx = mfi.growntilebox(ng);

        do_enforce_minimum_density(bx, state_in.array(mfi), verbose);

    }

    if (print_update_diagnostics)
    {
        // Evaluate what the effective reset source was.

        MultiFab::Subtract(reset_source, state_in, 0, 0, state_in.nComp(), 0);

        evaluate_and_print_source_change(reset_source, 1.0, "negative density resets");
    }

}

void
Logi::enforce_speed_limit (MultiFab& state_in, int ng)
{
    BL_PROFILE("Logi::enforce_speed_limit()");

    // This routine sets the velocity in state_in to be no larger than the
    // speed limit, if one has been applied.

    if (logi::speed_limit <= 0.0_rt) return;

#ifdef _OPENMP
#pragma omp parallel
#endif
    for (MFIter mfi(state_in, TilingIfNotGPU()); mfi.isValid(); ++mfi) {

        const Box& bx = mfi.growntilebox(ng);

        auto u = state_in[mfi].array();

        amrex::ParallelFor(bx,
        [=] AMREX_GPU_HOST_DEVICE (int i, int j, int k)
        {
            Real rho = u(i,j,k,URHO);
            Real rhoInv = 1.0_rt / rho;

            Real vx = u(i,j,k,UMX) * rhoInv;
            Real vy = u(i,j,k,UMY) * rhoInv;
            Real vz = u(i,j,k,UMZ) * rhoInv;

            Real v = std::sqrt(vx * vx + vy * vy + vz * vz);

            if (v > logi::speed_limit) {
                Real reduce_factor = logi::speed_limit / v;

                u(i,j,k,UMX) *= reduce_factor;
                u(i,j,k,UMY) *= reduce_factor;
                u(i,j,k,UMZ) *= reduce_factor;

                u(i,j,k,UEDEN) -= 0.5_rt * rhoInv * (rho * vx * rho * vx - u(i,j,k,UMX) * u(i,j,k,UMX) +
                                                     rho * vy * rho * vy - u(i,j,k,UMY) * u(i,j,k,UMY) +
                                                     rho * vz * rho * vz - u(i,j,k,UMZ) * u(i,j,k,UMZ));
            }
        });
    }
}

void
Logi::avgDown (int state_indx)
{
    BL_PROFILE("Logi::avgDown(state_indx)");

    if (level == parent->finestLevel()) return;

    Logi& fine_lev = getLevel(level+1);

    const Geometry& fgeom = fine_lev.geom;
    const Geometry& cgeom =          geom;

    MultiFab&  S_crse   = get_new_data(state_indx);
    MultiFab&  S_fine   = fine_lev.get_new_data(state_indx);

    amrex::average_down(S_fine, S_crse,
                         fgeom, cgeom,
                         0, S_fine.nComp(), fine_ratio);
}

void
Logi::allocOldData ()
{
    MultiFab::RegionTag amrlevel_tag("AmrLevel_Level_" + std::to_string(level));
    MultiFab::RegionTag statedata_tag("StateData_Level_" + std::to_string(level));
    for (int k = 0; k < num_state_type; k++)
        state[k].allocOldData();
}

void
Logi::removeOldData()
{
    AmrLevel::removeOldData();
}

void
Logi::errorEst (TagBoxArray& tags,
                  int          /*clearval*/,
                  int          /*tagval*/,
                  Real         time,
                  int          /*n_error_buf*/,
                  int          /*ngrow*/)
{
    BL_PROFILE("Logi::errorEst()");

    Real ltime = time;

    // If we are forcing a post-timestep regrid,
    // note that we need to use the new time here,
    // not the old time.

    if (post_step_regrid) {
      ltime = get_state_data(State_Type).curTime();
    }

    // Apply each of the tagging criteria defined in the inputs.

    for (int j = 0; j < error_tags.size(); j++) {
        std::unique_ptr<MultiFab> mf;
        if (error_tags[j].Field() != std::string()) {
            mf = derive(error_tags[j].Field(), time, error_tags[j].NGrow());
        }
        error_tags[j](tags, mf.get(), TagBox::CLEAR, TagBox::SET, time, level, geom);
    }

    // Now we'll tag any user-specified zones using the full state array.

    apply_problem_tags(tags, ltime);

    // Finally we'll apply any tagging restrictions which must be obeyed by any setup.

    apply_tagging_restrictions(tags, ltime);

}



void
Logi::apply_problem_tags (TagBoxArray& tags, Real time)
{

    BL_PROFILE("Logi::apply_problem_tags()");

    MultiFab& S_new = get_new_data(State_Type);

    int lev = level;

#ifdef _OPENMP
#pragma omp parallel
#endif
    {
        for (MFIter mfi(tags); mfi.isValid(); ++mfi)
        {
            const Box& bx = mfi.validbox();

            TagBox& tagfab = tags[mfi];

            auto tag_arr = tagfab.array();
            const auto state_arr = S_new[mfi].array();
            const GeometryData& geomdata = geom.data();

            amrex::ParallelFor(bx,
            [=] AMREX_GPU_HOST_DEVICE (int i, int j, int k)
            {
                problem_tagging(i, j, k, tag_arr, state_arr, lev, geomdata);
            });
        }
    }

}



void
Logi::apply_tagging_restrictions(TagBoxArray& tags, Real time)
{
    BL_PROFILE("Logi::apply_tagging_restrictions()");
}



std::unique_ptr<MultiFab>
Logi::derive (const std::string& name,
                Real           time,
                int            ngrow)
{

    BL_PROFILE("Logi::derive()");

#ifdef AMREX_PARTICLES
  return ParticleDerive(name,time,ngrow);
#else
   return AmrLevel::derive(name,time,ngrow);
#endif
}

void
Logi::derive (const std::string& name,
                Real           time,
                MultiFab&      mf,
                int            dcomp)
{

    BL_PROFILE("Logi::derive()");

    AmrLevel::derive(name,time,mf,dcomp);
}

void
Logi::extern_init ()
{
  // initialize the external runtime parameters -- these will
  // live in the probin

  if (ParallelDescriptor::IOProcessor()) {
    std::cout << "reading extern runtime parameters ..." << std::endl;
  }

#ifdef MICROPHYSICS_FORT
  const int probin_file_length = probin_file.length();
  Vector<int> probin_file_name(probin_file_length);

  for (int i = 0; i < probin_file_length; i++) {
    probin_file_name[i] = probin_file[i];
  }

  // read them in in Fortran from the probin file
  runtime_init(probin_file_name.dataPtr(),&probin_file_length);
#endif

  // grab them from Fortran to C++; then read any C++ parameters directly
  // from inputs (via ParmParse)
  init_extern_parameters();

#ifdef MICROPHYSICS_FORT
  // finally, update the Fortran side via ParmParse to override the
  // values of any parameters that were set in inputs
  update_fortran_extern_after_cxx();
#endif

}

void
Logi::reset_internal_energy(const Box& bx,
#ifdef MHD
                              Array4<Real> const Bx, Array4<Real> const By, Array4<Real> const Bz,
#endif
                              Array4<Real> const u)
{
    Real lsmall_temp = small_temp;
    Real ldual_energy_eta2 = dual_energy_eta2;

    amrex::ParallelFor(bx,
    [=] AMREX_GPU_HOST_DEVICE (int i, int j, int k)
    {
        Real rhoInv = 1.0_rt / u(i,j,k,URHO);
        Real Up = u(i,j,k,UMX) * rhoInv;
        Real Vp = u(i,j,k,UMY) * rhoInv;
        Real Wp = u(i,j,k,UMZ) * rhoInv;
        Real ke = 0.5_rt * (Up * Up + Vp * Vp + Wp * Wp);

        eos_re_t eos_state;

        eos_state.rho = u(i,j,k,URHO);
        eos_state.T   = lsmall_temp;
        for (int n = 0; n < NumSpec; ++n) {
            eos_state.xn[n] = u(i,j,k,UFS+n) * rhoInv;
        }
#if NAUX_NET > 0
        for (int n = 0; n < NumAux; ++n) {
            eos_state.aux[n] = u(i,j,k,UFX+n) * rhoInv;
        }
#endif

        eos(eos_input_rt, eos_state);

        Real small_e = eos_state.e;

#ifdef MHD
        Real bx_cell_c = 0.5_rt * (Bx(i,j,k) + Bx(i+1,j,k));
        Real by_cell_c = 0.5_rt * (By(i,j,k) + By(i,j+1,k));
        Real bz_cell_c = 0.5_rt * (Bz(i,j,k) + Bz(i,j,k+1));

        Real B_ener = 0.5_rt * (bx_cell_c*bx_cell_c +
                                by_cell_c*by_cell_c +
                                bz_cell_c*bz_cell_c);
#else
        Real B_ener = 0.0_rt;
#endif

        // Ensure the internal energy is at least as large as this minimum
        // from the EOS; the same holds true for the total energy.

        u(i,j,k,UEINT) = amrex::max(u(i,j,k,UEINT), u(i,j,k,URHO) * small_e);
        u(i,j,k,UEDEN) = amrex::max(u(i,j,k,UEDEN), u(i,j,k,URHO) * (small_e + ke) + B_ener);

        // Apply the dual energy criterion: get e from E if (E - K) > eta * E.

        Real rho_eint = u(i,j,k,UEDEN) - u(i,j,k,URHO) * ke - B_ener;

        if (rho_eint > ldual_energy_eta2 * u(i,j,k,UEDEN)) {
            u(i,j,k,UEINT) = rho_eint;
        }
    });
}

void
Logi::reset_internal_energy(
#ifdef MHD
                              MultiFab& Bx,
                              MultiFab& By,
                              MultiFab& Bz,
#endif
                              MultiFab& S_new, int ng)

{

    BL_PROFILE("Logi::reset_internal_energy()");

    MultiFab old_state;

    // Make a copy of the state so we can evaluate how much changed.

    if (print_update_diagnostics)
    {
        old_state.define(S_new.boxArray(), S_new.DistributionMap(), S_new.nComp(), 0);
        MultiFab::Copy(old_state, S_new, 0, 0, S_new.nComp(), 0);
    }

    // Ensure (rho e) isn't too small or negative
#ifdef _OPENMP
#pragma omp parallel
#endif
    for (MFIter mfi(S_new, TilingIfNotGPU()); mfi.isValid(); ++mfi)
    {
        const Box& bx = mfi.growntilebox(ng);

        reset_internal_energy(bx,
#ifdef MHD
                              Bx.array(mfi), By.array(mfi), Bz.array(mfi),
#endif
                              S_new.array(mfi));
    }

    if (print_update_diagnostics)
    {
        // Evaluate what the effective reset source was.

        MultiFab reset_source(S_new.boxArray(), S_new.DistributionMap(), S_new.nComp(), 0);

        MultiFab::Copy(reset_source, S_new, 0, 0, S_new.nComp(), 0);

        MultiFab::Subtract(reset_source, old_state, 0, 0, old_state.nComp(), 0);

        evaluate_and_print_source_change(reset_source, 1.0, "negative energy resets");
    }
}


#ifdef MHD
void
Logi::add_magnetic_e( MultiFab& Bx,
                        MultiFab& By, 
                        MultiFab& Bz,
                        MultiFab& State)
{
           
#ifdef _OPENMP
#pragma omp parallel
#endif
  for (MFIter mfi(State, TilingIfNotGPU()); mfi.isValid(); ++mfi)
  {
      const Box& box     = mfi.tilebox();
      auto S_arr = State.array(mfi);
      auto Bx_arr = Bx.array(mfi);
      auto By_arr = By.array(mfi);
      auto Bz_arr = Bz.array(mfi);


      ParallelFor(box,
      [=] AMREX_GPU_HOST_DEVICE (int i, int j, int k)
      {

          Real bx_cell_c = 0.5_rt * (Bx_arr(i,j,k) + Bx_arr(i+1,j,k));
          Real by_cell_c = 0.5_rt * (By_arr(i,j,k) + By_arr(i,j+1,k));
          Real bz_cell_c = 0.5_rt * (Bz_arr(i,j,k) + Bz_arr(i,j,k+1));

          S_arr(i,j,k,UEDEN) += 0.5_rt * (bx_cell_c * bx_cell_c +
                                          by_cell_c * by_cell_c +
                                          bz_cell_c * bz_cell_c);

      });

  }


}

void
Logi::check_div_B( MultiFab& Bx,
                     MultiFab& By, 
                     MultiFab& Bz,
                     MultiFab& State)
{

 

  ReduceOps<ReduceOpSum> reduce_op;
  ReduceData<int> reduce_data(reduce_op);
  using ReduceTuple = typename decltype(reduce_data)::Type;

           
#ifdef _OPENMP
#pragma omp parallel
#endif
  for (MFIter mfi(State, TilingIfNotGPU()); mfi.isValid(); ++mfi)
  {
      const Box& box     = mfi.tilebox();
      auto Bx_arr = Bx.array(mfi);
      auto By_arr = By.array(mfi);
      auto Bz_arr = Bz.array(mfi);

      const auto dx = geom.CellSizeArray();

      reduce_op.eval(box, reduce_data,
      [=] AMREX_GPU_HOST_DEVICE (int i, int j, int k) -> ReduceTuple
      {
          
          Real divB = (Bx_arr(i+1,j,k) - Bx_arr(i,j,k))/dx[0] +
                      (By_arr(i,j+1,k) - By_arr(i,j,k))/dx[1] + 
                      (Bz_arr(i,j,k+1) - Bz_arr(i,j,k))/dx[2];
        
          Real bx_cell_c = 0.5_rt * (Bx_arr(i,j,k) + Bx_arr(i+1,j,k));
          Real by_cell_c = 0.5_rt * (By_arr(i,j,k) + By_arr(i,j+1,k));
          Real bz_cell_c = 0.5_rt * (Bz_arr(i,j,k) + Bz_arr(i,j,k+1));

          Real magB = std::sqrt(bx_cell_c * bx_cell_c + 
                                by_cell_c * by_cell_c +
                                bz_cell_c * bz_cell_c);
                  
  
          int fail_divB = 0;

          if (std::abs(divB) > 1.0e-10*magB){
             fail_divB = 1; 
          }
          

          return {fail_divB}; 
      });

  }

  ReduceTuple hv = reduce_data.value();
  int init_fail_divB = amrex::get<0>(hv);

  if (init_fail_divB != 0) {
     amrex::Error("Error: initial data has divergence of B not zero");  
  } 


}
#endif

void
Logi::computeTemp(
#ifdef MHD
                    MultiFab& Bx,
                    MultiFab& By,
                    MultiFab& Bz,
#endif

                    MultiFab& State, Real time, int ng)

{

  BL_PROFILE("Logi::computeTemp()");

  MultiFab Stemp;

  // for 4th order, the only variables that may change here are Temp
  // and Eint.  To ensure that we don't modify values unless there is
  // a reset for Eint, we want to store the Laplacian that we use for
  // the cell-average -> cell-center conversion so we can use the same
  // Laplacian to convert back, resulting only in roundoff changes if
  // Eint is not modified.  We have to store it here, since we
  // overwrite the grown state as we work.
  MultiFab Eint_lap;

#ifdef TRUE_SDC
  if (sdc_order == 4) {

    // we need to make the data live at cell-centers first

    // fill Stemp with S_new.
    // we only need 2 ghost cells here, then the make_cell_center
    // makes 1 ghost cell a valid center, we compute its temp, and
    // then the final average results only in interior temps valid
    Stemp.define(State.boxArray(), State.DistributionMap(), NUM_STATE, 2);
    expand_state(Stemp, time, Stemp.nGrow());

    // store the Laplacian term for the internal energy
    Eint_lap.define(State.boxArray(), State.DistributionMap(), 1, 0);

    // convert to cell centers -- this will result in Stemp being
    // cell centered only on 1 ghost cells
    auto domain_lo = geom.Domain().loVect3d();
    auto domain_hi = geom.Domain().hiVect3d();

    FArrayBox tmp;

    for (MFIter mfi(Stemp); mfi.isValid(); ++mfi) {
      const Box& bx = mfi.growntilebox(1);
      const Box& bx0 = mfi.tilebox();
      const int idx = mfi.tileIndex();

      compute_lap_term(bx0, Stemp.array(mfi), Eint_lap.array(mfi), UEINT,
                       domain_lo, domain_hi);

      tmp.resize(bx, 1);
      Elixir elix_tmp = tmp.elixir();
      auto tmp_arr = tmp.array();

      make_cell_center_in_place(bx, Stemp.array(mfi), tmp_arr, domain_lo, domain_hi);

    }

  }
#endif

#ifdef TRUE_SDC
  if (sdc_order == 4) {
    // we need to enforce minimum density here, since the conversion
    // from cell-average to centers could have made rho < 0 near steep
    // gradients
    enforce_min_density(Stemp, Stemp.nGrow());
    reset_internal_energy(Stemp, Stemp.nGrow());
  } else {
#endif    
    reset_internal_energy(
#ifdef MHD
                          Bx, By, Bz,
#endif
                          State, ng);
#ifdef TRUE_SDC
  }
#endif

#ifdef _OPENMP
#pragma omp parallel
#endif
  for (MFIter mfi(State, TilingIfNotGPU()); mfi.isValid(); ++mfi)
  {

      int num_ghost = ng;
#ifdef TRUE_SDC
      if (sdc_order == 4) {
          // only one ghost cell is at cell-centers
          num_ghost = 1;
      }
#endif

      const Box& bx = mfi.growntilebox(num_ghost);

#ifdef TRUE_SDC
      FArrayBox& u_fab = (sdc_order == 4) ? Stemp[mfi] : State[mfi];
#else
      FArrayBox& u_fab = State[mfi];
#endif

      Array4<Real> const u = u_fab.array();

      amrex::ParallelFor(bx,
      [=] AMREX_GPU_HOST_DEVICE (int i, int j, int k)
      {

          Real rhoInv = 1.0_rt / u(i,j,k,URHO);

          eos_re_t eos_state;

          eos_state.rho = u(i,j,k,URHO);
          eos_state.T   = u(i,j,k,UTEMP); // Initial guess for the EOS
          eos_state.e   = u(i,j,k,UEINT) * rhoInv;
          for (int n = 0; n < NumSpec; ++n) {
            eos_state.xn[n] = u(i,j,k,UFS+n) * rhoInv;
          }
#if NAUX_NET > 0
          for (int n = 0; n < NumAux; ++n) {
            eos_state.aux[n] = u(i,j,k,UFX+n) * rhoInv;
          }
#endif

          eos(eos_input_re, eos_state);

          u(i,j,k,UTEMP) = eos_state.T;

      });

      if (clamp_ambient_temp == 1) {
          amrex::ParallelFor(bx,
          [=] AMREX_GPU_DEVICE (int i, int j, int k)
          {
              Real rhoInv = 1.0_rt / u(i,j,k,URHO);

              if (u(i,j,k,URHO) <= logi::ambient_safety_factor * ambient::ambient_state[URHO]) {
                  u(i,j,k,UTEMP) = ambient::ambient_state[UTEMP];
                  u(i,j,k,UEINT) = ambient::ambient_state[UEINT] * (u(i,j,k,URHO) * rhoInv);
                  u(i,j,k,UEDEN) = u(i,j,k,UEINT) + 0.5_rt * rhoInv * (u(i,j,k,UMX) * u(i,j,k,UMX) +
                                                                       u(i,j,k,UMY) * u(i,j,k,UMY) +
                                                                       u(i,j,k,UMZ) * u(i,j,k,UMZ));
              }
          });
      }

  }

#ifdef TRUE_SDC
  if (sdc_order == 4) {

    // we need to copy back from Stemp into S_new, making it
    // cell-average in the process.  For temperature, we will
    // construct the Laplacian from the new state and use for the
    // correction, since the temperature was just updated.  For the
    // internal energy, most zones will not have been reset, so we
    // don't want to modify them from what they were before, therefore
    // we use the stored Laplacian computed above to convert back to
    // cell-averages -- this is 4th-order and will be a no-op for
    // those zones where e wasn't changed.

    auto domain_lo = geom.Domain().loVect3d();
    auto domain_hi = geom.Domain().hiVect3d();

    FArrayBox tmp;

    for (MFIter mfi(Stemp); mfi.isValid(); ++mfi) {

      const Box& bx = mfi.tilebox();
      const int idx = mfi.tileIndex();

      tmp.resize(bx, 1);
      Elixir elix_tmp = tmp.elixir();
      auto tmp_arr = tmp.array();

      // only temperature
      make_fourth_in_place_n(bx, Stemp.array(mfi), UTEMP, tmp_arr, domain_lo, domain_hi);
    }

    // correct UEINT
    MultiFab::Add(Stemp, Eint_lap, 0, UEINT, 1, 0);

    // copy back UTEMP and UEINT -- those are the only things that
    // should have changed.
    MultiFab::Copy(State, Stemp, UTEMP, UTEMP, 1, 0);
    MultiFab::Copy(State, Stemp, UEINT, UEINT, 1, 0);

    // now that we redid these, redo the ghost fill -- technically,
    // only need this for UTEMP and UEINT, and only if ng > 0
    if (ng > 0) {
      AmrLevel::FillPatch(*this, State, State.nGrow(), time, State_Type, 0, NUM_STATE);
    }

    Stemp.clear();
  }
#endif

}



void
Logi::create_source_corrector()
{

    BL_PROFILE("Logi::create_source_corrector()");

    if (time_integration_method == CornerTransportUpwind && source_term_predictor == 1) {

        // Optionally predict the source terms to t + dt/2,
        // which is the time-level n+1/2 value, To do this we use a
        // lagged predictor estimate: dS/dt_n = (S_n - S_{n-1}) / dt, so
        // S_{n+1/2} = S_n + (dt / 2) * dS/dt_n. We'll add the S_n
        // terms later; now we add the second term. We defer the
        // multiplication by dt / 2 until the actual advance, since
        // we may be subcycling and thus not know yet what the
        // advance timestep is.

        // Note that since for dS/dt we want (S^{n+1} - S^{n}) / dt,
        // we only need to take twice the new-time source term from the
        // last timestep, since in the predictor-corrector approach,
        // the new-time source term is 1/2 * S^{n+1} - 1/2 * S^{n}.
        // This is untrue in general for the non-momentum sources,
        // so for safety we'll only apply it to the momentum sources.

        // Even though we're calculating this predictor from the last
        // timestep, we've already done the swap, so the "new" data
        // from the last timestep is currently residing in the "old"
        // StateData. (As a corollary, this operation must be done
        // prior to updating any of the source StateData.) Since the
        // dt comes from the last timestep, which is no longer equal
        // to the difference between prevTime and curTime, we rely
        // on our recording of the last dt from the previous advance.

        const Real time = get_state_data(Source_Type).prevTime();

        AmrLevel::FillPatch(*this, source_corrector, NUM_GROW_SRC, time, Source_Type, UMX, 3, UMX);

        source_corrector.mult(2.0 / lastDt, NUM_GROW_SRC);

    }
    else if (time_integration_method == SimplifiedSpectralDeferredCorrections && source_term_predictor == 1) {

        // If we're doing simplified SDC, time-center the source term (using the
        // current iteration's old sources and the last iteration's new
        // sources). Since the "new-time" sources are just the corrector step
        // of the predictor-corrector formalism, we want to add the full
        // value of the "new-time" sources to the old-time sources to get a
        // time-centered value. Note that, as above, the "new" data from the
        // last step is currently residing in the "old" StateData since we
        // have already done the swap.

        const Real time = get_state_data(Source_Type).prevTime();

        AmrLevel::FillPatch(*this, source_corrector, NUM_GROW_SRC, time, Source_Type, 0, NSRC);

    }

}



void
Logi::swap_state_time_levels(const Real dt)
{

    BL_PROFILE("Logi::swap_state_time_levels()");

    MultiFab::RegionTag statedata_tag("StateData_Level_" + std::to_string(level));
    MultiFab::RegionTag amrlevel_tag("AmrLevel_Level_" + std::to_string(level));

    for (int k = 0; k < num_state_type; k++) {

        // The following is a hack to make sure that we only
        // ever have new data for certain state types that only
        // ever need new time data; by doing a swap now, we'll
        // guarantee that allocOldData() does nothing. We do
        // this because we never need the old data, so we
        // don't want to allocate memory for it.

#ifdef SIMPLIFIED_SDC
#ifdef REACTIONS
        if (time_integration_method == SimplifiedSpectralDeferredCorrections) {
            if (k == Simplified_SDC_React_Type) {
                state[k].swapTimeLevels(0.0);
            }
        }
#endif
#endif

#ifdef TRUE_SDC
#ifdef REACTIONS
        if (time_integration_method == SpectralDeferredCorrections &&
            sdc_order == 4 && k == SDC_Source_Type) {
          state[k].swapTimeLevels(0.0);
        }
#endif
#endif
        state[k].allocOldData();

        state[k].swapTimeLevels(dt);

    }

}

Real
Logi::getCPUTime()
{

  int numCores = ParallelDescriptor::NProcs();
#ifdef _OPENMP
  numCores = numCores*omp_get_max_threads();
#endif

  Real T = numCores*(ParallelDescriptor::second() - startCPUTime) +
    previousCPUTimeUsed;

  return T;
}


MultiFab&
Logi::build_fine_mask()
{
    BL_PROFILE("Logi::build_fine_mask()");

    BL_ASSERT(level > 0); // because we are building a mask for the coarser level

    if (fine_mask.empty()) {
        fine_mask = makeFineMask(parent->boxArray(level-1),
                                 parent->DistributionMap(level-1),
                                 parent->boxArray(level), crse_ratio,
                                 1.0,  // coarse
                                 0.0); // fine
    }

    return fine_mask;
}

iMultiFab&
Logi::build_interior_boundary_mask (int ng)
{
    BL_PROFILE("Logi::build_interior_boundary_mask()");

    for (int i = 0; i < ib_mask.size(); ++i)
    {
        if (ib_mask[i]->nGrow() == ng) {
            return *ib_mask[i];
        }
    }

    //  If we got here, we need to build a new one
    ib_mask.push_back(std::unique_ptr<iMultiFab>(new iMultiFab(grids, dmap, 1, ng)));

    iMultiFab& imf = *ib_mask.back();

    int ghost_covered_by_valid = 0;
    int other_cells = 1; // uncovered ghost, valid, and outside domain cells are set to 1

    imf.BuildMask(geom.Domain(), geom.periodicity(),
                  ghost_covered_by_valid, other_cells, other_cells, other_cells);

    return imf;
}

// Fill a version of the state with ng ghost zones from the state data.

void
Logi::expand_state(MultiFab& S, Real time, int ng)
{
  BL_PROFILE("Logi::expand_state()");

  BL_ASSERT(S.nGrow() >= ng);

  AmrLevel::FillPatch(*this, S, ng, time, State_Type, 0, NUM_STATE);
}


void
Logi::check_for_nan(MultiFab& state_in, int check_ghost)
{
  BL_PROFILE("Logi::check_for_nan()");

  int ng = 0;
  if (check_ghost == 1) {
    ng = state_in.nGrow();
  }

  if (state_in.contains_nan(URHO,state_in.nComp(),ng,true))
    {
      for (int i = 0; i < state_in.nComp(); i++)
        {
          if (state_in.contains_nan(URHO + i, 1, ng, true))
            {
              std::string abort_string = std::string("State has NaNs in the ") + desc_lst[State_Type].name(i) + std::string(" component::check_for_nan()");
              amrex::Abort(abort_string.c_str());
            }
        }
    }
}

// Given State_Type state data, perform a number of cleaning steps to make
// sure the data is sensible.

void
Logi::clean_state(
#ifdef MHD
                    MultiFab& bx,
                    MultiFab& by,
                    MultiFab& bz,
#endif
                    MultiFab& state_in, Real time, int ng) {

    BL_PROFILE("Logi::clean_state()");

    // Enforce a minimum density.

    enforce_min_density(state_in, ng);

    // Enforce the speed limit.

    enforce_speed_limit(state_in, ng);

    // Ensure all species are normalized.

    normalize_species(state_in, ng);

    // Sync the linear and hybrid momenta.

#ifdef HYBRID_MOMENTUM
    if (hybrid_hydro) {
        hybrid_to_linear_momentum(state_in, ng);
    }
#endif

    // Compute the temperature (note that this will also reset
    // the internal energy for consistency with the total energy).

    computeTemp(
#ifdef MHD
                bx, by, bz,
#endif
                state_in, time, ng);

}

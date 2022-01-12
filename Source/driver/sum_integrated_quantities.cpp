#include <iomanip>

#include <Logi.H>
#include <Logi_F.H>

#include <problem_diagnostics.H>

using namespace amrex;

void
Logi::sum_integrated_quantities ()
{

    if (verbose <= 0) return;

    bool local_flag = true;

    int finest_level = parent->finestLevel();
    Real time        = state[State_Type].curTime();
    Real dt          = parent->dtLevel(0);
    if (time == 0.0) {
        dt = 0.0; // dtLevel returns the next timestep for t = 0, so overwrite
    }
    int timestep     = parent->levelSteps(0);
    Real mass        = 0.0;
    Real mom[3]      = { 0.0 };
    Real ang_mom[3]  = { 0.0 };
#ifdef HYBRID_MOMENTUM
    Real hyb_mom[3]  = { 0.0 };
#endif
    Real com[3]      = { 0.0 };
    Real com_vel[3]  = { 0.0 };
    Real rho_e       = 0.0;
    Real rho_K       = 0.0;
    Real rho_E       = 0.0;

    int datprecision = 16;

    int datwidth     = 25; // Floating point data in scientific notation
    int fixwidth     = 25; // Floating point data not in scientific notation
    int intwidth     = 12; // Integer data

    for (int lev = 0; lev <= finest_level; lev++)
    {
        Logi& ca_lev = getLevel(lev);

        mass   += ca_lev.volWgtSum("density", time, local_flag);
        mom[0] += ca_lev.volWgtSum("xmom", time, local_flag);
        mom[1] += ca_lev.volWgtSum("ymom", time, local_flag);
        mom[2] += ca_lev.volWgtSum("zmom", time, local_flag);

        ang_mom[0] += ca_lev.volWgtSum("angular_momentum_x", time, local_flag);
        ang_mom[1] += ca_lev.volWgtSum("angular_momentum_y", time, local_flag);
        ang_mom[2] += ca_lev.volWgtSum("angular_momentum_z", time, local_flag);

#ifdef HYBRID_MOMENTUM
        hyb_mom[0] += ca_lev.volWgtSum("rmom", time, local_flag);
        hyb_mom[1] += ca_lev.volWgtSum("lmom", time, local_flag);
        hyb_mom[2] += ca_lev.volWgtSum("zmom", time, local_flag);
#endif

        if (show_center_of_mass) {
           com[0] += ca_lev.locWgtSum("density", time, 0, local_flag);
           com[1] += ca_lev.locWgtSum("density", time, 1, local_flag);
           com[2] += ca_lev.locWgtSum("density", time, 2, local_flag);
        }

       rho_e += ca_lev.volWgtSum("rho_e", time, local_flag);
       rho_K += ca_lev.volWgtSum("kineng", time, local_flag);
       rho_E += ca_lev.volWgtSum("rho_E", time, local_flag);

    }

    if (verbose > 0)
    {

#ifdef HYBRID_MOMENTUM
      const int nfoo = 13;
#else
      const int nfoo = 10;
#endif

        Real foo[nfoo] = {mass, mom[0], mom[1], mom[2], ang_mom[0], ang_mom[1], ang_mom[2],
#ifdef HYBRID_MOMENTUM
                          hyb_mom[0], hyb_mom[1], hyb_mom[2],
#endif
                          rho_e, rho_K, rho_E};

#ifdef BL_LAZY
        Lazy::QueueReduction( [=] () mutable {
#endif

        ParallelDescriptor::ReduceRealSum(foo, nfoo, ParallelDescriptor::IOProcessorNumber());

        if (show_center_of_mass)
            ParallelDescriptor::ReduceRealSum(com, 3, ParallelDescriptor::IOProcessorNumber());

        if (ParallelDescriptor::IOProcessor()) {

            int i = 0;
            mass       = foo[i++];
            mom[0]     = foo[i++];
            mom[1]     = foo[i++];
            mom[2]     = foo[i++];
            ang_mom[0] = foo[i++];
            ang_mom[1] = foo[i++];
            ang_mom[2] = foo[i++];
#ifdef HYBRID_MOMENTUM
            hyb_mom[0] = foo[i++];
            hyb_mom[1] = foo[i++];
            hyb_mom[2] = foo[i++];
#endif
            rho_e      = foo[i++];
            rho_K      = foo[i++];
            rho_E      = foo[i++];

            std::cout << '\n';
            std::cout << "TIME= " << time << " MASS        = "   << mass      << '\n';
            std::cout << "TIME= " << time << " XMOM        = "   << mom[0]    << '\n';
            std::cout << "TIME= " << time << " YMOM        = "   << mom[1]    << '\n';
            std::cout << "TIME= " << time << " ZMOM        = "   << mom[2]    << '\n';
            std::cout << "TIME= " << time << " ANG MOM X   = "   << ang_mom[0] << '\n';
            std::cout << "TIME= " << time << " ANG MOM Y   = "   << ang_mom[1] << '\n';
            std::cout << "TIME= " << time << " ANG MOM Z   = "   << ang_mom[2] << '\n';
#ifdef HYBRID_MOMENTUM
            std::cout << "TIME= " << time << " HYB MOM R   = "   << hyb_mom[0] << '\n';
            std::cout << "TIME= " << time << " HYB MOM L   = "   << hyb_mom[1] << '\n';
            std::cout << "TIME= " << time << " HYB MOM P   = "   << hyb_mom[2] << '\n';
#endif
            std::cout << "TIME= " << time << " RHO*e       = "   << rho_e     << '\n';
            std::cout << "TIME= " << time << " RHO*K       = "   << rho_K     << '\n';
            std::cout << "TIME= " << time << " RHO*E       = "   << rho_E     << '\n';

            std::ostream& data_log1 = *Logi::data_logs[0];

            if (data_log1.good()) {

               if (time == 0.0) {

                   std::ostringstream header;

                   int n = 0;

                   header << std::setw(intwidth) << "#   TIMESTEP";              ++n;
                   header << std::setw(fixwidth) << "                     TIME"; ++n;
                   header << std::setw(datwidth) << "                     MASS"; ++n;
                   header << std::setw(datwidth) << "                     XMOM"; ++n;
                   header << std::setw(datwidth) << "                     YMOM"; ++n;
                   header << std::setw(datwidth) << "                     ZMOM"; ++n;
                   header << std::setw(datwidth) << "              ANG. MOM. X"; ++n;
                   header << std::setw(datwidth) << "              ANG. MOM. Y"; ++n;
                   header << std::setw(datwidth) << "              ANG. MOM. Z"; ++n;
#if (AMREX_SPACEDIM == 3)
#ifdef HYBRID_MOMENTUM
                   header << std::setw(datwidth) << "              HYB. MOM. R"; ++n;
                   header << std::setw(datwidth) << "              HYB. MOM. L"; ++n;
                   header << std::setw(datwidth) << "              HYB. MOM. P"; ++n;
#endif
#endif
                   header << std::setw(datwidth) << "              KIN. ENERGY"; ++n;
                   header << std::setw(datwidth) << "              INT. ENERGY"; ++n;
                   header << std::setw(datwidth) << "               GAS ENERGY"; ++n;

                   header << std::endl;

                   data_log1 << std::setw(intwidth) << "#   COLUMN 1";
                   data_log1 << std::setw(fixwidth) << "                        2";

                   for (int i = 3; i <= n; ++i) {
                       data_log1 << std::setw(datwidth) << i;
                   }

                   data_log1 << std::endl;

                   data_log1 << header.str();
               }

               // Write the quantities at this time
               data_log1 << std::fixed;

               data_log1 << std::setw(intwidth) <<  timestep;

               if (time < 1.e-4_rt || time > 1.e4_rt) {
                   data_log1 << std::scientific;
               }
               else {
                   data_log1 << std::fixed;
               }

               data_log1 << std::setw(fixwidth) <<  std::setprecision(datprecision) << time;

               data_log1 << std::scientific;

               data_log1 << std::setw(datwidth) <<  std::setprecision(datprecision) << mass;
               data_log1 << std::setw(datwidth) <<  std::setprecision(datprecision) << mom[0];
               data_log1 << std::setw(datwidth) <<  std::setprecision(datprecision) << mom[1];
               data_log1 << std::setw(datwidth) <<  std::setprecision(datprecision) << mom[2];
               data_log1 << std::setw(datwidth) <<  std::setprecision(datprecision) << ang_mom[0];
               data_log1 << std::setw(datwidth) <<  std::setprecision(datprecision) << ang_mom[1];
               data_log1 << std::setw(datwidth) <<  std::setprecision(datprecision) << ang_mom[2];
#ifdef HYBRID_MOMENTUM
               data_log1 << std::setw(datwidth) <<  std::setprecision(datprecision) << hyb_mom[0];
               data_log1 << std::setw(datwidth) <<  std::setprecision(datprecision) << hyb_mom[1];
               data_log1 << std::setw(datwidth) <<  std::setprecision(datprecision) << hyb_mom[2];
#endif
               data_log1 << std::setw(datwidth) <<  std::setprecision(datprecision) << rho_K;
               data_log1 << std::setw(datwidth) <<  std::setprecision(datprecision) << rho_e;
               data_log1 << std::setw(datwidth) <<  std::setprecision(datprecision) << rho_E;
               data_log1 << std::endl;

            }

            if (show_center_of_mass) {
                for (int idir = 0; idir <= 2; idir++) {
                  com[idir]     = com[idir] / mass;
                  com_vel[idir] = mom[idir] / mass;
                }

                std::cout << "TIME= " << time << " CENTER OF MASS X-LOC = " << com[0]     << '\n';
                std::cout << "TIME= " << time << " CENTER OF MASS X-VEL = " << com_vel[0] << '\n';

                std::cout << "TIME= " << time << " CENTER OF MASS Y-LOC = " << com[1]     << '\n';
                std::cout << "TIME= " << time << " CENTER OF MASS Y-VEL = " << com_vel[1] << '\n';

                std::cout << "TIME= " << time << " CENTER OF MASS Z-LOC = " << com[2]     << '\n';
                std::cout << "TIME= " << time << " CENTER OF MASS Z-VEL = " << com_vel[2] << '\n';
            }
        }
#ifdef BL_LAZY
        });
#endif
    }

    // Species

    {
        std::vector<Real> species_mass(NumSpec);
        std::vector<std::string> species_names(NumSpec);

        // Species names

        for (int i = 0; i < NumSpec; i++) {
            species_names[i] = desc_lst[State_Type].name(UFS+i);
            species_names[i] = species_names[i].substr(4,std::string::npos);
            species_mass[i] = 0.0;
        }

        // Integrated mass of all species on the domain

        for (int lev = 0; lev <= finest_level; ++lev) {
            for (int i = 0; i < NumSpec; ++i) {
                species_mass[i] += getLevel(lev).volWgtSum("rho_" + species_names[i], time, local_flag) / C::M_solar;
            }
        }

        int nfoo_sum = NumSpec;

        amrex::Vector<Real> foo_sum(nfoo_sum);

        for (int i = 0; i < NumSpec; ++i) {
            foo_sum[i] = species_mass[i];
        }

        amrex::ParallelDescriptor::ReduceRealSum(foo_sum.dataPtr(), nfoo_sum);

        for (int i = 0; i < NumSpec; ++i) {
            species_mass[i] = foo_sum[i];
        }

        if (ParallelDescriptor::IOProcessor()) {

            std::ostream& log = *Logi::data_logs[2];

            if (time == 0.0) {

                int n = 0;

                std::ostringstream header;

                header << std::setw(intwidth) << "#   TIMESTEP";           ++n;
                header << std::setw(fixwidth) << "                  TIME"; ++n;

                // We need to be careful here since the species names have differing numbers of characters

                for (int i = 0; i < NumSpec; i++) {
                    std::string outString  = "";
                    std::string massString = "Mass ";
                    std::string specString = species_names[i];
                    while (outString.length() + specString.length() + massString.length() < datwidth) outString += " ";
                    outString += massString;
                    outString += specString;
                    header << std::setw(datwidth) << outString; ++n;
                }

                header << std::endl;

                log << std::setw(intwidth) << "#   COLUMN 1";
                log << std::setw(fixwidth) << "                        2";

                for (int i = 3; i <= n; ++i)
                    log << std::setw(datwidth) << i;

                log << std::endl;

                log << header.str();

            }

            log << std::fixed;

            log << std::setw(intwidth)                                    << timestep;

            if (time < 1.e-4_rt || time > 1.e4_rt) {
                log << std::scientific;
            }
            else {
                log << std::fixed;
            }

            log << std::setw(fixwidth) << std::setprecision(datprecision) << time;

            log << std::scientific;

            for (int i = 0; i < NumSpec; i++)
                log << std::setw(datwidth) << std::setprecision(datprecision) << species_mass[i];

            log << std::endl;

        }

    }

    // Information about the AMR driver.

    {
        // Calculate wall time for the step.

        Real wall_time = 0.0;

        if (time > 0.0) {
            wall_time = amrex::ParallelDescriptor::second() - wall_time_start;
        }

        // Calculate GPU memory consumption.

#ifdef AMREX_USE_GPU
        Long gpu_size_free_MB = Gpu::Device::freeMemAvailable() / (1024 * 1024);
        ParallelDescriptor::ReduceLongMin(gpu_size_free_MB, ParallelDescriptor::IOProcessorNumber());

        Long gpu_size_used_MB = (Gpu::Device::totalGlobalMem() - Gpu::Device::freeMemAvailable()) / (1024 * 1024);
        ParallelDescriptor::ReduceLongMax(gpu_size_used_MB, ParallelDescriptor::IOProcessorNumber());
#endif

        if (ParallelDescriptor::IOProcessor()) {

            std::ostream& log = *Logi::data_logs[3];

            if (time == 0.0) {

                int n = 0;

                std::ostringstream header;

                header << std::setw(intwidth) << "#   TIMESTEP";              ++n;
                header << std::setw(fixwidth) << "                     TIME"; ++n;
                header << std::setw(fixwidth) << "                       DT"; ++n;
                header << std::setw(intwidth) << "  FINEST LEV";              ++n;
                header << std::setw(fixwidth) << " COARSE TIMESTEP WALLTIME"; ++n;
#ifdef AMREX_USE_GPU
                header << std::setw(fixwidth) << "  MAXIMUM GPU MEMORY USED"; ++n;
                header << std::setw(fixwidth) << "  MINIMUM GPU MEMORY FREE"; ++n;
#endif

                header << std::endl;

                log << std::setw(intwidth) << "#   COLUMN 1";
                log << std::setw(fixwidth) << "                        2";

                for (int i = 3; i < 4; ++i) {
                    log << std::setw(datwidth) << i;
                }

                log << std::setw(intwidth) << 4; // Handle the finest lev column

                for (int i = 5; i <= n; ++i) {
                    log << std::setw(datwidth) << i;
                }

                log << std::endl;

                log << header.str();

            }

            log << std::fixed;

            log << std::setw(intwidth)                                    << timestep;

            if (time < 1.e-4_rt || time > 1.e4_rt) {
                log << std::scientific;
            }
            else {
                log << std::fixed;
            }

            log << std::setw(fixwidth) << std::setprecision(datprecision) << time;

            log << std::fixed;

            log << std::setw(fixwidth) << std::setprecision(datprecision) << dt;
            log << std::setw(intwidth)                                    << parent->finestLevel();
            log << std::setw(datwidth) << std::setprecision(datprecision) << wall_time;
#ifdef AMREX_USE_GPU
            log << std::setw(datwidth)                                    << gpu_size_used_MB;
            log << std::setw(datwidth)                                    << gpu_size_free_MB;
#endif

            log << std::endl;

        }

    }

    problem_diagnostics();
}

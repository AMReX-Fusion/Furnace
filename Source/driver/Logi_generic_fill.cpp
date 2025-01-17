
#include <AMReX_BLFort.H>
#include <Logi.H>
#include <Logi_generic_fill.H>
#include <AMReX_BCUtil.H>

using namespace amrex;

void ca_generic_fill(Box const& bx, FArrayBox& data,
                     const int dcomp, const int numcomp,
                     Geometry const& geom, const Real time,
                     const Vector<BCRec>& bcr, const int bcomp,
                     const int scomp)
{
    GpuBndryFuncFab<LogiGenericFill> gpu_bndry_func(LogiGenericFill{});
    gpu_bndry_func(bx, data, dcomp, numcomp, geom, time, bcr, bcomp, scomp);
}

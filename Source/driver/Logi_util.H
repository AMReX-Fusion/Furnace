#ifndef LOGI_UTIL_H
#define LOGI_UTIL_H

#include <AMReX_Geometry.H>
#include <network_properties.H>
#include <state_indices.H>

using namespace amrex;

#if AMREX_SPACEDIM == 1
constexpr int dg0 = 1;
constexpr int dg1 = 0;
constexpr int dg2 = 0;
#elif AMREX_SPACEDIM == 2
constexpr int dg0 = 1;
constexpr int dg1 = 1;
constexpr int dg2 = 0;
#else
constexpr int dg0 = 1;
constexpr int dg1 = 1;
constexpr int dg2 = 1;
#endif

AMREX_GPU_HOST_DEVICE AMREX_FORCE_INLINE
int upassmap (int ipassive)
{
    if (ipassive < NumAdv) {
        return UFA + ipassive;
    }
    else if (ipassive < NumAdv + NumSpec) {
        return UFS + ipassive - NumAdv;
    }
    else {
        return UFX + ipassive - NumAdv - NumSpec;
    }
}

AMREX_GPU_HOST_DEVICE AMREX_FORCE_INLINE
int qpassmap (int ipassive)
{
    if (ipassive < NumAdv) {
        return QFA + ipassive;
    }
    else if (ipassive < NumAdv + NumSpec) {
        return QFS + ipassive - NumAdv;
    }
    else {
        return QFX + ipassive - NumAdv - NumSpec;
    }
}

AMREX_GPU_HOST_DEVICE AMREX_FORCE_INLINE
bool mom_flux_has_p (const int mom_dir, const int flux_dir, const int coord)
{

    // Keep track of which components of the momentum flux have pressure

#if AMREX_SPACEDIM <= 2

    if (mom_dir == 0 && flux_dir == 0) {

        if (AMREX_SPACEDIM == 1 || (AMREX_SPACEDIM == 2 && coord == CoordSys::RZ)) {
            return false;
        }
        else {
            return true;
        }

    }
    else {

        return (mom_dir == flux_dir);

    }

#else // AMREX_SPACEDIM == 3; Cartesian

    return (mom_dir == flux_dir);

#endif

}



AMREX_GPU_HOST_DEVICE AMREX_FORCE_INLINE
void position(int i, int j, int k,
              GeometryData const& geomdata, GpuArray<Real, 3>& loc,
              bool ccx = true, bool ccy = true, bool ccz = true)
{
    // Given 3D indices (i,j,k), return the cell-centered spatial position.
    // Optionally we can also be edge-centered in any of the directions.

    int idx[3] = {i, j, k};

    bool cc[3] = {ccx, ccy, ccz};

    Real offset[AMREX_SPACEDIM];

    const Real* problo = geomdata.ProbLo();
    const Real* probhi = geomdata.ProbHi();
    const Real* dx     = geomdata.CellSize();
    const int*  domlo  = geomdata.domain.loVect();
    const int*  domhi  = geomdata.domain.hiVect();

    for (int dir = 0; dir < AMREX_SPACEDIM; ++dir) {
        if (cc[dir]) {
            // If we're cell-centered, we want to be in the middle of the zone.
            offset[dir] = problo[dir] + 0.5_rt * dx[dir];
        }
        else {
            // We are edge-centered.
            offset[dir] = problo[dir];
        }
    }

    // Be careful when using periodic boundary conditions. In that case,
    // we need to loop around to the other side of the domain.

    for (int dir = 0; dir < AMREX_SPACEDIM; ++dir) {

        if (geomdata.isPeriodic(dir)) {
            if (idx[dir] < domlo[dir]) {
                offset[dir] += probhi[dir] - problo[dir];
            }

            // For the hi end we need to deal correctly with
            // edge-centered indexing.
            if ((cc[dir] && idx[dir] > domhi[dir]) ||
                (!cc[dir] && idx[dir] > domhi[dir]+1)) {
                offset[dir] += problo[dir] - probhi[dir];
            }
        }

    }

    for (int dir = 0; dir < AMREX_SPACEDIM; ++dir) {
        loc[dir] = offset[dir] + idx[dir] * dx[dir];
    }

    for (int dir = AMREX_SPACEDIM; dir < 3; ++dir) {
        loc[dir] = 0.0_rt;
    }

}

namespace geometry_util
{

AMREX_GPU_HOST_DEVICE AMREX_INLINE
Real volume(const int& i, const int& j, const int& k,
            const GeometryData& geomdata)
{
    // Given 3D cell-centered indices (i,j,k), return the volume of the zone.
    // Note that Logi has no support for angular coordinates, so
    // this function only provides Cartesian in 1D/2D/3D, Cylindrical (R-Z)
    // in 2D, and Spherical in 1D.

    auto dx = geomdata.CellSize();

    Real vol;

#if AMREX_SPACEDIM == 1

    auto coord = geomdata.Coord();

    if (coord == 0) {

        // Cartesian

        vol = dx[0];

    }
    else {

        // Spherical

        Real rl = geomdata.ProbLo()[0] + static_cast<Real>(i) * dx[0];
        Real rr = rl + dx[0];

        vol = (4.0_rt / 3.0_rt) * M_PI * dx[0] * (rl * rl + rl * rr + rr * rr);

    }

#elif AMREX_SPACEDIM == 2

    auto coord = geomdata.Coord();

    if (coord == 0) {

        // Cartesian

        vol = dx[0] * dx[1];

    }
    else {

        // Cylindrical

        Real r_l = geomdata.ProbLo()[0] + static_cast<Real>(i) * dx[0];
        Real r_r = geomdata.ProbLo()[0] + static_cast<Real>(i+1) * dx[0];

        vol = M_PI * (r_l + r_r) * dx[0] * dx[1];

    }

#else

    // Cartesian

    vol = dx[0] * dx[1] * dx[2];

#endif

    return vol;
}

AMREX_GPU_HOST_DEVICE AMREX_INLINE
Real area(const int& i, const int& j, const int& k,
          const int& idir, const GeometryData& geomdata)
{
    auto dx = geomdata.CellSize();

    Real a;

#if AMREX_SPACEDIM == 1

    auto coord = geomdata.Coord();

    if (coord == 0) {

        // Cartesian

        a = 1.0_rt;

    }
    else {

        // Spherical

        Real r = geomdata.ProbLo()[0] + static_cast<Real>(i) * dx[0];

        a = 4.0_rt * M_PI * r * r;

    }

#elif AMREX_SPACEDIM == 2

    auto coord = geomdata.Coord();

    if (coord == 0) {

        // Cartesian

        if (idir == 0) {
            a = dx[1];
        }
        else {
            a = dx[0];
        }

    }
    else  {

        // Cylindrical

        if (idir == 0) {
            Real r = geomdata.ProbLo()[0] + static_cast<Real>(i) * dx[0];
            a = 2.0_rt * M_PI * r * dx[1];
        }
        else {
            Real r = geomdata.ProbLo()[0] + (static_cast<Real>(i) + 0.5_rt) * dx[0];
            a = 2.0_rt * M_PI * r * dx[0];
        }

    }

#else

    // Cartesian

    if (idir == 0) {
        a = dx[1] * dx[2];
    }
    else if (idir == 1) {
        a = dx[0] * dx[2];
    }
    else {
        a = dx[0] * dx[1];
    }

#endif

    return a;
}

}

#endif

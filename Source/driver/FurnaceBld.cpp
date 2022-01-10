
#include <AMReX_LevelBld.H>
#include <Furnace.H>

using namespace amrex;

class FurnaceBld
    :
    public LevelBld
{
    virtual void variableSetUp () override;
    virtual void variableCleanUp () override;
    virtual AmrLevel *operator() () override;
    virtual AmrLevel *operator() (Amr&            papa,
                                  int             lev,
                                  const Geometry& level_geom,
                                  const BoxArray& ba,
                                  const DistributionMapping& dm,
                                  Real            time) override;
};

FurnaceBld Furnace_bld;

LevelBld*
getLevelBld ()
{
    return &Furnace_bld;
}

void
FurnaceBld::variableSetUp ()
{
    Furnace::variableSetUp();
}

void
FurnaceBld::variableCleanUp ()
{
    Furnace::variableCleanUp();
}

AmrLevel*
FurnaceBld::operator() ()
{
    return new Furnace;
}

AmrLevel*
FurnaceBld::operator() (Amr&            papa,
                       int             lev,
                       const Geometry& level_geom,
                       const BoxArray& ba,
                       const DistributionMapping& dm,
                       Real            time)
{
    return new Furnace(papa, lev, level_geom, ba, dm, time);
}


#include <AMReX_LevelBld.H>
#include <Logi.H>

using namespace amrex;

class LogiBld
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

LogiBld Logi_bld;

LevelBld*
getLevelBld ()
{
    return &Logi_bld;
}

void
LogiBld::variableSetUp ()
{
    Logi::variableSetUp();
}

void
LogiBld::variableCleanUp ()
{
    Logi::variableCleanUp();
}

AmrLevel*
LogiBld::operator() ()
{
    return new Logi;
}

AmrLevel*
LogiBld::operator() (Amr&            papa,
                       int             lev,
                       const Geometry& level_geom,
                       const BoxArray& ba,
                       const DistributionMapping& dm,
                       Real            time)
{
    return new Logi(papa, lev, level_geom, ba, dm, time);
}

.. _ch:sdc:

*****************************
Spectral Deferred Corrections
*****************************

The Logi SDC solver couples the hydrodynamics tightly together,
iteratively improving the convergence of the solution.  This is the
basis of the 4th order accurate Logi solver.  The algorithm is described
in :cite:`logi-sdc`.

.. note::

   Here we are referring to the full SDC time integration scheme
   (``logi.time_integration_method = 2``), not the simplified-SDC solver.


The options that describe the quadrature and iterations are:

* ``logi.sdc_order`` : the desired spatial and temporal order.  2 and 4 are supported.

* ``logi.sdc_quadrature`` : the quadrature scheme used for the
  time-integration.  This determines the number and location of the
  temporal nodes.  Supported values are 0 for Gauss-Lobatto and 1 for
  Radau IIA.

* ``logi.sdc_extra`` : the number of extra iterations to take.  By
  default the number of iterations used is equal to the value of
  ``sdc_order``.


The options that affect the nonlinear solve are:

* ``sdc_solver`` : the method we use to do the nonlinear solution of
  the reaction system.  Values are:

  * 1 : pure Newton iteration (we subdivide the time interval if
    needed to get the Newton method to converge).

  * 2 : use VODE to solve the nonlinear system by expressing it as an ODE system.

  * 3 : use VODE for the first iteration and then Newton for the
    subsequent iterations.

* ``sdc_solver_tol_dens`` : the relative error on the density in solving the nonlinear system.

* ``sdc_solver_tol_spec`` : the relative error on the partial densities, :math:`(\rho X_k)`
  in the nonlinear solve.

* ``sdc_solver_tol_ener`` : the relative error of the energy in the nonlinear solve.

* ``sdc_solver_atol`` : the absolute error in the mass fractions during the nonlinear solve.

* ``sdc_solver_relax_factor`` : the factor by which to relax the
  tolerances (i.e. increase them) for earlier iterations.  We reach
  the desired tolerances on the final iteration.

* ``sdc_solve_for_rhoe`` : whether we solve the system in terms of :math:`(\rho e)` or :math:`(\rho E)`.

* ``sdc_newton_use_analytic_jac`` : whether we use the analytic Jacobian when doing Newton iterations for
  the reaction part of the system or compute it numerically.







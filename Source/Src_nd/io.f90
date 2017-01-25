module io_module

  use bl_fort_module, only : rt => c_real
  implicit none

  public

contains

  subroutine flush_output() bind(C,name='flush_output')

    use iso_fortran_env, only: output_unit

    use bl_fort_module, only : rt => c_real
    implicit none

    flush(output_unit)

  end subroutine flush_output

end module io_module

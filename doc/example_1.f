C Example 1:
C This example represents the simplest possible DrawP3D program.  A
C GOB is created, a sphere primitive GOB is placed in it, and the
C model is viewed with the default lights and camera.

C Subroutine to ease checking of return codes
      subroutine errchk( ival )
      if (ival.ne.1) write(6,*) 'ERROR!'
      return
      end

      program ex1
      integer pintrn, popen, psphr, pclose, psnap, pshtdn

      call errchk( pintrn('myrenderer','p3d','example_1.p3d',
     $     'This string will be ignored') )

      call errchk( popen('mygob') )
      call errchk( psphr() )
      call errchk( pclose() )

      call errchk( psnap('mygob','standard_lights','standard_camera') )

      call errchk( pshtdn() )

      stop
      end

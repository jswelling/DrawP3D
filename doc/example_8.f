C Example 8:
C This example uses the PRNISO function to generate an isosurface
C from data points at random 3D locations.  PBNDBX is used to 
C provide a bounding box for the surface.
C
C ***Note:  This example uses the 'ran()' function to generate a
C random number between 0.0 and 1.0.  Different Fortran compilers
C may require different functions.  For example, on Cray machines
C ran(iseed) needs to be replaced with ranf().
C

C     Subroutine to ease checking of return codes.
      subroutine errchk( ival )
      integer ival
      if (ival.ne.1) write(6,*) 'ERROR!'
      return
      end

C     This function actually constructs the isosurface and bounding 
C     box. 
      subroutine rniex()
      integer nrnipt
      parameter (nrnipt=1000)
      integer npts, i
      real coords(3,nrnipt)
      real vals(2,nrnipt)
      integer psdcmp, popen, pclose, prniso, pbndbx, pgbclr
      integer iseed

C     Corner points for the bounding box, and a color.
      real p1(3)
      data p1/ -0.5, -0.5, -0.5 /
      real p2(3)
      data p2/ 0.5, 0.5, 0.5 /
      real blue(4)
      data blue/ 0.0, 0.0, 1.0, 1.0 /

C     npts is the number of data points.  iseed is a random number seed.
C     value is the constant value of the isosurface to be generated.
      npts= nrnipt
      iseed= 1234567
      value= 0.48
      
C     Generate random coordinate and value data.  vals(1,n) provides
C     the set of values which will define the isosurface.  vals(2,n)
C     provides values which will be used to color the surface.
      do 10 i= 1, npts
         coords(1,i)= ran(iseed) - 0.5
         coords(2,i)= ran(iseed) - 0.5
         coords(3,i)= ran(iseed) - 0.5
         vals(1,i)= sqrt( coords(1,i)*coords(1,i) +
     $        coords(2,i)*coords(2,i) + coords(3,i)*coords(3,i) )
         vals(2,i)= coords(3,i)
 10      continue

C     This sets the color map which will be used to map the value
C     data to colors on the surface.
      call errchk( psdcmp(-0.5, 0.5, 2) )

C     Open a GOB to hold the isosurface and bounding box.
      call errchk( popen('mygob') )

C     Create the isosurface.  See the reference manual for information
C     on the calling parameters.  All the computing time spent in this
C     example takes place in this routine.
      call errchk( prniso( 6, 0, npts, coords, vals, 0, value, 1 ) )

C     Create a GOB containing a bounding box with an appropriate color.
      call errchk( popen(' ') )
      call errchk( pbndbx( p1, p2 ) )
      call errchk( pgbclr( 0, blue(1), blue(2), blue(3), blue(4) ) )
      call errchk( pclose() )

C     Close the GOB 'mygob'.
      call errchk( pclose() )

      return
      end

C     This is the main program.
      program ex8
      integer pintrn, pcamra, psnap, pshtdn

C     Camera positioning information follows.
      real up(3)
      data up/ 0.0, 1.0, 0.0 /
      real from(3)
      data from/ 0.0, 0.0, 8.0 /
      real at(3)
      data at/ 0.0, 0.0, 0.0 /

C     Initialize the renderer.
      call errchk( pintrn('myrenderer','p3d','example_8.p3d',
     $     'c') )

C     Create a camera, using the positioning information above.
      call errchk( pcamra('this_camera',from,at,up,10.0,-1.0,-10.0) )

C     Create a GOB named 'mygob' containing an isosurface from data at
C     random 3D locations and a bounding box.
      call rniex()

C     Cause the GOB 'mygob' to be rendered.
      call errchk( psnap('mygob','standard_lights','this_camera') )

C     Shut down the DrawP3D software.
      call errchk( pshtdn() )

      stop
      end


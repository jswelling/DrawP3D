C Example 7:
C This example uses the PRNZSF function to generate a height field 
C or z surface from data points at random 2D locations.  PBNDBX is
C used to provide bounds for the surface.
C

C     Subroutine to ease checking of return codes.
      subroutine errchk( ival )
      integer ival
      if (ival.ne.1) write(6,*) 'ERROR!'
      return
      end

C     This block data provides a set of 'random' data to form the 
C     z surface.  rnzdat is the 3D coordinate information;  rnzval
C     is the value data which will be used to color the surface.
      block data ftstbd
      integer rnzpts
      parameter (rnzpts=15)
      real rnzdat( 3, rnzpts )
      real rnzval( rnzpts )
      common /rnzcm/ rnzdat, rnzval
      data rnzdat/-10.0, 0.0, 0.0, 10.0, 0.0, 0.0,
     $     0.0, 10.0, 0.0, 0.0, 0.0, 10.0,
     $     -1.2, -2.3, -14.0, -30.4, 92.1, -10.0,
     $     35.45, 34.3, -19.9, -42.1, 65.3, -9.3,
     $     -20.6, 30.5, -14.3, 34.2, -40.1, -15.3,
     $     -3.0, 59.0, 3.0, -5.0, 59.0, 3.0,
     $     -1.0, 57.0, 3.0, -8.0, 61.0, 3.0,
     $     -6.0,     70.0,   3.0/
      data rnzval/ 0.0, 0.0, 0.0, 10.0, -14.0, -10.0, -19.9, -9.3,
     $     -14.3, -15.3, 3.0, 3.0, 3.0, 3.0, 3.0 /
      end

C     Those data values for which ixclde is set to 1 will be excluded
C     from the z surface, leaving holes.
      subroutine rnztfn( ixclde, x, y, z, i )
      integer ixclde, i
      real x, y, z
      if (i.eq.10) ixclde= 1
      return
      end

C     This function actually constructs the z surface and bounding box
      subroutine rnzex()
      integer rnzpts
      parameter (rnzpts=15)
      real rnzdat(3,rnzpts), rnzval(rnzpts)
      common /rnzcm/ rnzdat,rnzval
      integer npts
      integer psdcmp, popen, prnzsf, pgbclr, pclose, pbndbx
      external rnztfn

C     Corner points for the bounding box, and a color.
      real p1(3)
      data p1/ -42.1, -40.1, -19.9 /
      real p2(3)
      data p2/ 35.45, 92.1, 10.0 /
      real blue(4)
      data blue/ 0.0, 0.0, 1.0, 1.0 /

      npts= rnzpts

C     This sets the color map which will be used to map the value 
C     data to colors on the surface.
      call errchk( psdcmp(-15.3, 10.0, 1) )

C     Open a GOB to hold the z surface and bounding box.
      call errchk( popen('mygob') )

C     Create the z surface.  See the reference manual for information
C     on the calling parameters.
      call errchk( prnzsf( 4, 0, npts, rnzdat, rnzval, 0, 0, rnztfn ) )

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
      program ex7
      integer pintrn, pcamra, psnap, pshtdn

C     The following data provides camera viewpoint information.
      real up(3)
      data up/ 0.0, 0.0, 1.0 /
      real from(3)
      data from/ 200.0, 26.0, 19.9 /
      real at(3)
      data at/ 3.0, 26.0, 0.0 /

C     Initialize the renderer
      call errchk( pintrn('myrenderer','p3d','example_7.p3d',
     $     'c') )

C     Create an appropriate camera, using the viewpoint data above.
      call errchk( pcamra('this_camera',from,at,up,53.0,-50.0,-300.0) )

C     Create a GOB named 'mygob' containing a z surface from data at
C     random 2D locations and a bounding box.
      call rnzex()

C     Cause the GOB 'mygob' to be rendered.
      call errchk( psnap('mygob','standard_lights','this_camera') )

C     Shut down the DrawP3D software.
      call errchk( pshtdn() )

      stop
      end


C Example 6:
C This example uses the PZSURF function to produce a height field
C or Z surface from a 2D array of data.  The PAXIS and PBNDBX 
C routines are used to provide an axis and bounds for the surface.

C Subroutine to ease checking of return codes.
      subroutine errchk( ival )
      integer ival
      if (ival.ne.1) write(6,*) 'ERROR!'
      return
      end

      subroutine mkzsur( zdat )
C
C     This routine prepares data used to define the surface.
C
      real zdat( 29, 31 )
      integer i, j
      real r, tz
      real ir,jr
      do 10, i= -14, 14
         do 20, j = -15, 15
            ir = i
            jr = j
            r = sqrt( ir*ir + jr*jr )
            tz = cos(r)
            zdat(i+15,j+16) = 2.0*exp(-r/5.0)*tz
 20      continue
 10   continue
      return
      end
      

      subroutine tfun( bool, val, x, y )
C
C     Those data values for which bool is set to 1 will be excluded
C     from the z surface, leaving holes.
C
      integer bool
      real val
      integer x, y
      if (val .lt. 1.9) then 
         bool=0
      else
         bool=1
      endif
      return
      end

      subroutine zsfex()
C
C     This routine actually creates the height field or z surface.
C
      integer popen, pclose, pgbclr, paxis, pbndbx, psdcmp, pzsurf
      external tfun
      real zdat(29,31)

C     Definitions for some needed colors.
      real red(4)
      data red/1.0,0.0,0.0,1.0/
      real blue(4)
      data blue/0.0,0.0,1.0,1.0/

C     Specification of the corners of the bounding box and z surface
      real crna(3)
      data crna/-2.0, -2.0, -1.5/
      real crnb(3)
      data crnb/2.0, 2.0, 1.5/

C     This sets the location and orientation of the axis.
      real axspta(3)
      data axspta/-2.0,-2.1,-2.0 /
      real axsptb(3)
      data axsptb/2.0,-2.1,-2.0 /
      real axisup(3)
      data axisup/0.0,0.0,1.0 /

C     Calculate the data defining the surface.
      call mkzsur(zdat) 

C     This sets the color map which will be used to map the value data
C     to colors on the z surface.  We will be using the surface values
C     themselves, which give the height of the surface at any given
C     point, to color the surface.
      call errchk( psdcmp(-2.0,2.0,2) )

C     Open a GOB to contain the surface, bounding box, and axis.
      call errchk( popen('zsurf_gob') )

C     Insert a bounding box, with an associated color.
      call errchk( popen(' ') )
      call errchk( pbndbx( crna, crnb ) )
      call errchk( pgbclr(0,blue(1), blue(2), blue(3), blue(4) ) )
      call errchk( pclose() )

C     Insert an axis, with an associated color.  See the reference
C     manual for details on the calling parameters.
      call errchk( popen(' ') )
      call errchk( paxis( axspta, axsptb, axisup, -2.0, 2.0, 3,
     $      'BoundingBox',0.25, 2 ) ) 
      call errchk( pgbclr(0,red(1), red(2), red(3), red(4) ) )
      call errchk( pclose() )

C     Create the z surface, inserting it into the GOB 'zsurf_gob'.
C     The values in zdat are used both to provide the heights for
C     the surface and to provide values with which to color it.  
C     tfun provides a function which can be used to exclude points
C     from the surface;  it could be set to 0 and ignored if the
C     previous parameter were 1.
      call errchk( pzsurf( 4, zdat, zdat, 29, 31, crna, crnb, 
     $     0, tfun ) )

C     Close the GOB 'zsurf_gob'.
      call errchk( pclose() )

      return
      end

      program ex6
C
C     This is the main program
C
      integer pintrn, pcamra, psnap, pshtdn

C     The following information sets the camera position.
      real lookfm(3), lookat(3), up(3), fovea, hither, yon
      data lookfm/ 0.0, -10.0, 2.0 /
      data lookat/ 0.0, 0.0, 0.0 /
      data up/ 0.0, 0.0, 1.0 /
      data fovea, hither, yon/45.0, -5.0, -20.0/

C     Initialize the renderer.
      call errchk( pintrn('myrenderer','p3d','example_6.p3d',
     $     'c') )

C     Create a camera, according to the specs given above.
      call errchk( pcamra('mycamera',lookfm,lookat,up,fovea,
     $     hither,yon) )

C     Create a z surface, bounding box, and axis in a GOB called
C     'zsurf_gob'.
      call zsfex()

C     Cause the GOB to be rendered.
      call errchk( psnap('zsurf_gob','standard_lights', 'mycamera'))

C     Shut down the DrawP3D software.
      call errchk( pshtdn() )

      stop
      end


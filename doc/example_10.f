C Example 10: 
C This example uses the dp_irreg_zsurf function to produce a height 
C field or z surface from a 2D array of data in polar coordinates.
C The dp_axis and dp_boundbox routines are used to provide an axis
C and bounds for the surface.  Note that the bounds are not really
C accurate, since the data is regular in polar coordinates and
C the bounding box is drawn in Cartesian coordinates.
C
C Author: Silvia Olabarriaga (silvia@inf.ufrgs.br)
C March 24th 1993

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
      real zdat( nx, ny )
      integer i, j
      real rad, ang
      integer nx, ny
      real drad, minrad, maxrad, dang
      common /angcm/nx,ny,drad,minrad,maxrad,dang
      do 10, i= 1,nx
         rad= (i-1) * drad + minrad
         do 20, j = 1,ny
            ang = (j-1) * dang
            zdat(i,j)= rad * sin(ang) * cos(ang)
 20      continue
 10   continue
      return
      end
      

      subroutine xyfun( x, y, i, j )
C
C     This function returns cartesian (x,y) values from polar
C     coordinates on a regular grid.
C     i= row (radius) and j= column(theta)
C
      integer i, j
      real x, y
      integer nx, ny
      real drad, minrad, maxrad, dang
      common /angcm/nx, ny, drad, minrad, maxrad, dang
      real rad, theta
      rad= (i-1) * drad + minrad
      theta= (j-1) * dang
      x= rad * cos(theta)
      y= rad * sin(theta)
      return
      end

      subroutine tfun( bool, z, i, j )
C
C     Those data values for which bool is set to 1 will be excluded
C     from the z surface, leaving holes.  We exclude a band of 
C     constant angle.
C
      integer bool
      integer i, j
      real z
      integer nx, ny
      real drad, minrad, maxrad, dang
      common /angcm/nx, ny, drad, minrad, maxrad, dang
      if (j.eq.ny/2) then 
         bool=1
      else
         bool=0
      endif
      return
      end

      subroutine zsfex()
C
C     This routine actually creates the height field or z surface.
C
      integer popen, pclose, pgbclr, paxis, pbndbx, psdcmp, pirzsf
      external tfun, xyfun
      integer nxdim, nydim
      parameter (nxdim=31, nydim=29)
      real zdat(nxdim, nydim)

C     Common area for communication with callback routines
      integer nx, ny
      real drad, minrad, maxrad, dang
      common /angcm/nx, ny, drad,minrad,maxrad,dang

C     Definitions for some needed colors.
      real red(4)
      data red/1.0,0.0,0.0,1.0/
      real blue(4)
      data blue/0.0,0.0,1.0,1.0/

C     Specification of the corners of the bounding box and z surface
      real crna(3)
      data crna/-1.0, -1.0, -1.0/
      real crnb(3)
      data crnb/1.0, 1.0, 1.0/

C     This sets the location and orientation of the axis.
      real axspta(3)
      data axspta/-1.0,-1.0,-1.0 /
      real axsptb(3)
      data axsptb/1.0,-1.0,-1.0 /
      real axisup(3)
      data axisup/0.0,0.0,1.0 /

C     Initialize the common block
      nx= nxdim
      ny= nydim
      minrad= 0.01
      maxrad= 1.0
      drad= maxrad/(nx-1)
      dang= 2*3.14159265/(ny-1)

C     Calculate the data defining the surface.
      call mkzsur(zdat) 

C     This sets the color map which will be used to map the value data
C     to colors on the z surface.  We will be using the surface values
C     themselves, which give the height of the surface at any given
C     point, to color the surface.
      call errchk( psdcmp(-1.0,1.0,2) )

C     Open a GOB to contain the surface, bounding box, and axis.
      call errchk( popen('irreg_zsurf_gob') )

C     Insert a bounding box, with an associated color.
      call errchk( popen(' ') )
      call errchk( pbndbx( crna, crnb ) )
      call errchk( pgbclr(0,blue(1), blue(2), blue(3), blue(4) ) )
      call errchk( pclose() )

C     Insert an axis, with an associated color.  See the reference
C     manual for details on the calling parameters.
      call errchk( popen(' ') )
      call errchk( pgbclr(0,red(1), red(2), red(3), red(4) ) )
      call errchk( paxis( axspta, axsptb, axisup, -1.0, 1.0, 3,
     $      'Polar Coords', 0.15, 1 ) ) 
      call errchk( pclose() )

C     Create the z surface, inserting it into the GOB 'zsurf_gob'.
C     The values in zdat are used both to provide the heights for
C     the surface and to provide values with which to color it.  
C     tfun provides a function which can be used to exclude points
C     from the surface;  it could be set to 0 and ignored if the
C     previous parameter were 1.
      call errchk( pirzsf( 4, zdat, zdat, nx, ny, xyfun, 0, tfun ) )

C     Close the GOB 'irreg_zsurf_gob'.
      call errchk( pclose() )

      return
      end

      program ex10
C
C     This is the main program
C
      integer pintrn, pcamra, psnap, pshtdn

C     The following information sets the camera position.
      real lookfm(3), lookat(3), up(3), fovea, hither, yon
      data lookfm/ 0.0, -10.0, 3.0 /
      data lookat/ 0.0, 0.0, -0.5 /
      data up/ 0.0, 0.0, 1.0 /
      data fovea, hither, yon/20.0, -5.0, -20.0/

C     Initialize the renderer.
      call errchk( pintrn('myrenderer','p3d','example_10.p3d',
     $     'this string ignored') )

C     Create a camera, according to the specs given above.
      call errchk( pcamra('mycamera',lookfm,lookat,up,fovea,
     $     hither,yon) )

C     Create a z surface, bounding box, and axis in a GOB called
C     'zsurf_gob'.
      call zsfex()

C     Cause the GOB to be rendered.
      call errchk( psnap('irreg_zsurf_gob','standard_lights',
     $     'mycamera'))

C     Shut down the DrawP3D software.
      call errchk( pshtdn() )

      stop
      end


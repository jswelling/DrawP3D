C Example 5:
C This example shows how to generate isosurfaces from gridded 3D data.
C An isosurface is generated from function values on a 3D grid, and
C colored using a second set of values.

C Subroutine to ease checking of return codes.
      subroutine errchk( ival )
      integer ival
      if (ival.ne.1) write(6,*) 'ERROR!'
      return
      end

      subroutine isodat(data,nx,ny,nz)
C
C     This routine prepares data for the isosurface generator
C     routine.  A constant value surface will be extracted from
C     this data.
C
      integer nx, ny, nz
      real data(nx,ny,nz)
      integer i, j, k
      real x, y, z, phi, r, eps
      data eps/0.001/

      do 10 i= 1, nx
         x= real(i-1-(nx-1)/2)/real((nx-1)/2)
         do 20 j= 1,ny
            y= real(j-1-(ny-1)/2)/real((ny-1)/2)
            r= sqrt(x*x+y*y)
            if (r.lt.eps) then
               phi= 0.0
            else
               phi= acos( x/sqrt(x*x+y*y) )
               if (y.lt.0) phi= -phi
            endif
            do 30 k= 1,nz
               z= float(k-1-(nz-1)/2)/float((nz-1)/2)
               data(i,j,k)= (x*x + y*y + z*z)/
     $              (1.0+sin(3.0*phi+z)*sin(3.0*phi+z))
 30            continue
 20         continue
 10      continue

      return
      end

      subroutine isovdt(vdata,nx,ny,nz)
C
C     This routine prepares value data for the isosurface generator
C     test routine.  This data will be mapped into colors on the
C     surface of the generated isosurface.
C
      integer nx, ny, nz
      real vdata(nx,ny,nz)
      integer i, j, k
      real x, y, z

      do 10 i= 1, nx
         x= real(i-1-(nx-1)/2)/real((nx-1)/2)
         do 20 j= 1,ny
            y= real(j-1-(ny-1)/2)/real((ny-1)/2)
            do 30 k= 1,nz
               z= float(k-1-(nz-1)/2)/float((nz-1)/2)
               vdata(i,j,k)= 0.5*(x*x + y*y)
 30            continue
 20         continue
 10      continue

      return
      end

      subroutine isotst()
C
C     This routine tests the isosurface generator.
C
      integer psdcmp, popen, pisosf, pbndbx, pclose
      parameter (nxdim= 14, nydim= 15, nzdim= 16)
      real data(nxdim, nydim, nzdim), vdata(nxdim,nydim,nzdim)
      real val
      integer inside
      integer nx, ny, nz
      real crnr1(3), crnr2(3)

C This information specifies the corners of the region in which the
C isosurface and bounding box will be drawn.
      data crnr1/-2.6,-2.5,3.75/
      data crnr2/4.6,6.5,-9.75/

C val gives the constant value of the isosurface.  inside is used to
C show that the 'inside' surface is expected to be the visible 
C surface.
      data val,inside/0.5, 1/

C Generate the values to be contoured, and the values to use to color
C the isosurface.
      nx= nxdim
      ny= nydim
      nz= nzdim
      call isodat(data,nx,ny,nz)
      call isovdt(vdata,nx,ny,nz)
      
C Set a color map to be used in converting the vdata array into
C coloring information.
      call errchk( psdcmp(0.0, 1.0, 1) )

C Open a GOB to hold the isosurface and bounding box.
      call errchk( popen('mygob') )

C Generate the isosurface.  See the documentation for information on
C the calling parameters.
      call errchk( pisosf( 5, data, vdata, nx, ny, nz, val,
     $     crnr1, crnr2, inside ) )

C Draw a bounding box, so that the limits of the data grid will be 
C clear.
      call errchk( pbndbx( crnr1, crnr2 ) )

C Close the GOB containing the isosurface and bounding box.
      call errchk( pclose() )

      return
      end

C Main program follows.
      program ex5
      integer pintrn, popen, plight, pamblt, pclose, pcamra, psdcmp
      integer psnap, pshtdn

C Light specification informatin
      real ltloc(3), ltcolr(4), amcolr(4)
      data ltloc/ 0.0, 1.0, 10.0 /
      data ltcolr/ 0.8, 0.8, 0.8, 0.3 /
      data amcolr/ 0.3, 0.3, 0.3, 0.8 /

C Camera specification information
      real lookfm(3), lookat(3), up(3), fovea, hither, yon
      data lookfm/ 1.0, 2.0, 17.0 /
      data lookat/ 1.0, 2.0, -3.0 /
      data up/ 0.0, 1.0, 0.0 /
      data fovea, hither, yon/45.0, -5.0, -30.0/

C Create and initialize the renderer.
      call errchk( pintrn('myrenderer','p3d','example_5.p3d',
     $     'junk') )

C Generate lights and a camera.
      call errchk( popen('mylights') )
      call errchk( plight(ltloc,0,ltcolr) )
      call errchk( pamblt(0,amcolr) )
      call errchk( pclose() )
      call errchk( pcamra('mycamera',lookfm,lookat,up,fovea,
     $     hither,yon) )

C Generate the isosurface, and a bounding box to show the limits of
C the computational region.
      call isotst()

C Cause the isosurface and bounding box to be rendered.
      call errchk( psnap('mygob','mylights','mycamera') )

C Shut down DrawP3D
      call errchk( pshtdn() )

      stop
      end


C Example 11:
C This example shows how to generate isosurfaces from 3D data on
C irregular grids, for example spherical polar or cylindrical
C coordinate grids.  An isosurface is generated from function values
C on a 3D grid, and colored using a second set of values.

C Subroutine to ease checking of return codes.
      subroutine errchk( ival )
      integer ival
      if (ival.ne.1) write(6,*) 'ERROR!'
      return
      end

      subroutine irisod(data,nx,ny,nz)
C
C     This routine prepares data for the isosurface generator
C     routine.  A constant value surface will be extracted from
C     this data.
C
      integer nx, ny, nz
      real data(nx,ny,nz)
      integer i, j, k
      real r, theta, phi

      do 10 i= 1, nx
      r= 10.0 * float(i-1)/float(nx-1)
         do 20 j= 1,ny
            theta= float(j-1)/float(ny-1) * 3.14159265
            do 30 k= 1,nz
               phi= float(k-1)/float(nz-1) * 2.0 * 3.14159265
               data(i,j,k)= (r**2)*exp( -r ) * (3*cos(theta)**2 - 1.0)
 30            continue
 20         continue
 10      continue

      return
      end

      subroutine irisov(vdata,nx,ny,nz)
C
C     This routine prepares value data for the isosurface generator
C     test routine.  This data will be mapped into colors on the
C     surface of the generated isosurface.
C
      integer nx, ny, nz
      real vdata(nx,ny,nz)
      integer i, j, k
      real r, theta, phi

      do 10 i= 1, nx
      r= 10.0 * float(i-1)/float(nx-1)
         do 20 j= 1,ny
            theta= float(j-1)/float(ny-1) * 3.14159265
            do 30 k= 1,nz
               phi= float(k-1)/float(nz-1) * 2.0 * 3.14159265
               vdata(i,j,k)= sin(phi)
 30            continue
 20         continue
 10      continue

      return
      end

      subroutine firiso( x, y, z, i, j, k )
C
C     This routine is passed to the isosurface generator function
C     piriso, which calls it to translate grid addresses into x,y,z
C     coordinate values.
C
      integer i, j, k
      real x, y, z
      real r, theta, phi
      parameter (nxdim= 14, nydim= 15, nzdim= 16)

      r= 10.0 * float(i-1)/float(nxdim-1)
      theta= float(j-1)/float(nydim-1) * 3.14159265
      phi= float(k-1)/float(nzdim-1) * 2.0 * 3.14159265

      x= r*sin(theta)*cos(phi)
      y= r*sin(theta)*sin(phi)
      z= r*cos(theta)

      return
      end

      subroutine irisot()
C
C     This routine tests the isosurface generator.
C
      integer psdcmp, popen, piriso, pclose
      parameter (nxdim= 14, nydim= 15, nzdim= 16)
      real data(nxdim, nydim, nzdim), vdata(nxdim,nydim,nzdim)
      real val
      integer inside
      integer nx, ny, nz
      external firiso

C val gives the constant value of the isosurface.  inside is used to
C show that the 'inside' surface is expected to be the visible 
C surface.
      data val,inside/0.1, 0/

C Generate the values to be contoured, and the values to use to color
C the isosurface.
      nx= nxdim
      ny= nydim
      nz= nzdim
      call irisod(data,nx,ny,nz)
      call irisov(vdata,nx,ny,nz)
      
C Set a color map to be used in converting the vdata array into
C coloring information.
      call errchk( psdcmp(-1.0, 1.0, 1) )

C Open a GOB to hold the isosurface.
      call errchk( popen('mygob') )

C Generate the isosurface.  See the documentation for information on
C the calling parameters.
      call errchk( piriso( 4, data, vdata, nx, ny, nz, val,
     $     firiso, inside ) )

C Close the GOB containing the isosurface.
      call errchk( pclose() )

      return
      end

C Main program follows.
      program ex11
      integer pintrn, popen, plight, pamblt, pclose, pcamra
      integer psnap, pshtdn

C Light specification informatin
      real ltloc(3), ltcolr(4), amcolr(4)
      data ltloc/ 30.0, 0.0, 20.0 /
      data ltcolr/ 0.8, 0.8, 0.8, 0.3 /
      data amcolr/ 0.3, 0.3, 0.3, 0.8 /

C Camera specification information
      real lookfm(3), lookat(3), up(3), fovea, hither, yon
      data lookfm/ 30.0, 0.0, 0.0 /
      data lookat/ 0.0, 0.0, 0.0 /
      data up/ 0.0, 0.0, 1.0 /
      data fovea, hither, yon/45.0, -10.0, -60.0/

C Create and initialize the renderer.
      call errchk( pintrn('myrenderer','p3d','example_11.p3d',
     $     'junk') )

C Generate lights and a camera.
      call errchk( popen('mylights') )
      call errchk( plight(ltloc,0,ltcolr) )
      call errchk( pamblt(0,amcolr) )
      call errchk( pclose() )
      call errchk( pcamra('mycamera',lookfm,lookat,up,fovea,
     $     hither,yon) )

C Generate the isosurface
      call irisot()

C Cause the isosurface to be rendered.
      call errchk( psnap('mygob','mylights','mycamera') )

C Shut down DrawP3D
      call errchk( pshtdn() )

      stop
      end


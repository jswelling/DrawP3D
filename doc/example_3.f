C Example 3:
C In this example, we construct a ball-and-stick water molecule, using
C the camera and lights developped in Example 2. 

C Subroutine to ease checking of return codes
      subroutine errchk( ival )
      if (ival.ne.1) write(6,*) 'ERROR!'
      return
      end

C Subroutine to create the water molecule
      subroutine mkh2o()
      integer popen, pscale, psphr, pgbclr, pclose, ptrans, pchild,
     $     pplyln

C Atomic information- radii are 1/4 of Van der Waals
      real hrad, orad
      data hrad, orad/0.3, 0.35/
      real hclr(4), oclr(4)
      data hclr/1.0, 1.0, 1.0, 1.0/
      data oclr/1.0, 0.0, 0.0, 1.0/
      real h1loc(3), h2loc(3), oloc(3)
      data h1loc/0.854821, -.487790, 0.0/
      data h2loc/-0.854821, -.487790, 0.0/
      data oloc/0.0, 0.121948, 0.0/

C Polyline to join the atoms
      real lnclr(4)
      data lnclr/0.0, 0.0, 1.0, 1.0/
      real lncrd(3,3)
      data lncrd/
     $     0.854821, -.487790, 0.0,
     $     0.0, 0.121948, 0.0,
     $     -0.854821, -.487790, 0.0 /
      
C Make spheres of appropriate radius and color for elements.
C Note that the order in which attributes, transformations, and
C GOBs are inserted into a named GOB is irrelevant.
      call errchk( popen('hydrogen') )
      call errchk( pscale(hrad) )
      call errchk( psphr() )
      call errchk( pgbclr( 0, hclr(1), hclr(2), hclr(3), hclr(4) ) )
      call errchk( pclose() )

      call errchk( popen('oxygen') )
      call errchk( pscale(orad) )
      call errchk( psphr() )
      call errchk( pgbclr( 0, oclr(1), oclr(2), oclr(3), oclr(4) ) )
      call errchk( pclose() )

C Open the outer GOB, and insert in it three nameless subGOBs.  Each
C contains one atom and the transformation appropriate to put it in
C the proper place.  
      call errchk( popen('h2o') )

      call errchk( popen(' ') )
      call errchk( ptrans( h1loc(1), h1loc(2), h1loc(3) ) )
      call errchk( pchild('hydrogen') )
      call errchk( pclose() )

      call errchk( popen(' ') )
      call errchk( ptrans( h2loc(1), h2loc(2), h2loc(3) ) )
      call errchk( pchild('hydrogen') )
      call errchk( pclose() )

      call errchk( popen(' ') )
      call errchk( ptrans( oloc(1), oloc(2), oloc(3) ) )
      call errchk( pchild('oxygen') )
      call errchk( pclose() )

C Insert a polyline connecting the atoms.  The color would apply
C to the atoms as well, but it will be superceded by their own
C color attribute.
      call errchk( pgbclr(0, lnclr(1), lnclr(2), lnclr(3), lnclr(4) ) )
      call errchk( pplyln(0, 0, 3, lncrd, 0 ) )

C Close the outer GOB
      call errchk( pclose() )

      return
      end

      program ex3
      integer pintrn, popen, pclose, psnap, pshtdn, pamblt, plight,
     $     pcamra

C Light specification information
      real ltloc(3), ltclr(4), ambclr(4)
      data ltloc/0.0, 20.0, 20.0/
      data ltclr/0.7, 0.7, 0.7, 1.0/
      data ambclr/0.3, 0.3, 0.3, 1.0/

C Camera specification information
      real lookfm(3), lookat(3), up(3)
      data lookfm/0.0, 0.0, 10.0/
      data lookat/0.0, 0.0, 0.0/
      data up/0.0, 1.0, 0.0/
      real fovea, hither, yon
      data fovea, hither, yon/ 53.0, -5.0, -15.0 /

C Create and initialize the renderer
      call errchk( pintrn('myrenderer','p3d','example_3.p3d',
     $     'This string will be ignored') )

C Create the lights and camera
      call errchk( popen('mylights') )
      call errchk( pamblt( 0, ambclr ) )
      call errchk( plight( ltloc, 0, ltclr ) )
      call errchk( pclose() )
      call errchk( pcamra('mycamera', lookfm, lookat, up, fovea,
     $     hither, yon ) )

C Create the water molecule
      call mkh2o()

C Render the model
      call errchk( psnap('h2o','mylights','mycamera') )

C Shut down the renderer
      call errchk( pshtdn() )

      stop
      end

C Example 2:
C This example differs from example_1 in that we add explicit lights
C and camera.

C Subroutine to ease checking of return codes
      subroutine errchk( ival )
      if (ival.ne.1) write(6,*) 'ERROR!'
      return
      end

      program ex2
      integer pintrn, popen, psphr, pclose, psnap, pshtdn, pamblt,
     $     plight, pcamra

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

      call errchk( pintrn('myrenderer','p3d','example_2.p3d',
     $     'This string will be ignored') )

      call errchk( popen('mygob') )
      call errchk( psphr() )
      call errchk( pclose() )

      call errchk( popen('mylights') )
      call errchk( pamblt( 0, ambclr ) )
      call errchk( plight( ltloc, 0, ltclr ) )
      call errchk( pclose() )

      call errchk( pcamra('mycamera', lookfm, lookat, up, fovea,
     $     hither, yon ) )

      call errchk( psnap('mygob','mylights','mycamera') )

      call errchk( pshtdn() )

      stop
      end


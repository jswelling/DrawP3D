C Example 9: 
C This example produces a cylinder between two points.  It contains
C routines which can be used to draw a cylinder of any diameter
C between any two points.
C
C     Subroutine to ease checking of return codes
      subroutine errchk( ival)
      integer ival
      if (ival.ne.1) then
         write(6,*) 'ERROR',ival
         call exit(-1)
      end if
      return
      end
      

C     This routine produces a rotation which will reverse the given
C     vector (as opposed to a simple inversion, which is not a
C     rotation).
      subroutine make_flip(vx,vy,vz)
      real vx,vy,vz
      integer protat
      real px,py,pz,dot,cx,cy,cz,normsqr
      real epsilon
      parameter(epsilon=0.00001)
C     Initialize Variables
      px= 0.0
      py= 0.0
      pz= 1.0

C     Find a vector no parallel to the given vector
      dot= px*vx + py*vy + pz*vz
      cx= py*vz - pz*vy
      cy= pz*vx - px*vz
      cz= px*vy - py*vx
      if ( (cx*cx + cy*cy + cz*cz).lt. epsilon*dot*dot ) then
         px= 1.0
         py= 0.0
         pz= 0.0
         dot= px*vx + py*vy + pz*vz
      end if

C     Extract the normal component of that vector 
      normsqr= vx*vx + vy*vy + vz*vz
      px= px - vx*dot/normsqr
      py= py - vy*dot/normsqr
      pz= pz - vz*dot/normsqr

c     Generate a 180 degree rotation about that vector 
      call errchk( protat(px, py, pz, 180.0 ) )
      
      end


C
C     This routine returns a rotation which will rotate its first
C     parameter vector to align with its second parameter vector.
C
      subroutine make_aligning_rotation( v1x, v1y, v1z, v2x, v2y, v2z )
      real v1x, v1y, v1z, v2x, v2y, v2z
      real ax, ay, az, dot, theta
      real epsilon,radtodeg
      parameter (epsilon=0.00001)
      parameter (radtodeg= 180.0/3.14159265)

      integer ptrans,protat

C     Take the cross and dot products of the two vectors
      ax= v1y*v2z - v1z*v2y
      ay= v1z*v2x - v1x*v2z
      az= v1x*v2y - v1y*v2x
      dot= v1x*v2x + v1y*v2y + v1z*v2z

      if ( (ax*ax + ay*ay + az*az).lt.epsilon*dot*dot )then
         if (dot .ge. 0.0 ) then
            call errchk( ptrans( 0.0, 0.0, 0.0 ) )
         else 
C     anti-parallel 
            call make_flip( v1x, v1y, v1z )
         end if
      else 
C     Find the angle of rotation around the cross product axis */
         theta= acos( ( v1x*v2x+v1y*v2y+v1z*v2z ) / 
     $        ( sqrt( v1x*v1x+v1y*v1y+v1z*v1z )
     $        * sqrt( v2x*v2x+v2y*v2y+v2z*v2z ) ) )

C     Generate a theta degree rotation about that vector */
         call errchk( protat( ax,ay,az, RADTODEG*theta ) )
      end if
      
      end


C
C     This routine creates a cylinder between the two end points, and of
C     the given diameter.  The cylinder is added to the currently open GOB.
C
      subroutine make_connecting_cylinder( p1, p2, rad )

      real p1(3)
      real p2(3)
      real rad
      real length
      real dx, dy, dz
      integer popen,pascal,ptrans,pcyl,pclose

C     All of this goes inside a nameless GOB 
      call errchk( popen(" ") )
      
      
C     Find the needed orientation, and the length
      dx= p2(1) - p1(1)
      dy= p2(2) - p1(2)
      dz= p2(3) - p1(3)
      length= ( dx**2 + dy**2 + dz**2 )**0.5
      
C     Add a scaling operation to make it the right length and diameter
      call errchk( pascal( rad, rad, length ) )

C     Add a rotation to line the cylinder up with the vector between the
C     two points.

      call make_aligning_rotation( 0.0, 0.0, 1.0, dx, dy, dz )

C     Add a translation to move the end to point 1 
      call errchk( ptrans( p1(1), p1(2), p1(3) ) )

C     Add the cylinder 
      call errchk( pcyl() )
      
C     Close the outer GOB 
      call errchk( pclose() )

      end


      program ex9

      integer pintrn, popen, pclose, pshtrn, pamblt, plight,
     $     pcamra,ptrans,psphr,pgbclr,psnap,pshtdn

C     Camera specification information
      real lookfm(3), lookat(3), up(3)
      data lookfm/0.0, 0.0, 20.0/
      data lookat/0.0, 0.0, 0.0/
      data up/0.0, 1.0, 0.0/
      real fovea, hither, yon
      data fovea, hither, yon/ 53.0, -5.0, -15.0 /

      real pt1(3)
      data pt1/ 2.0, 0.0, 0.0/
      real pt2(3)
      data pt2/ 4.0, 5.0, 1.0/
      real blue(4)
      data blue/0.0, 0.0, 1.0, 1.0/

      
C     Initialize the renderer. 
      call  ERRCHK( pintrn("myrenderer","p3d","example_9.p3d",
     $     "ignored") )

C     Create a camera, using the information above. 
      call errchk( pcamra( "this_camera", lookfm, lookat, up,45.0,
     $     -5.0, -30.0 ) )
      
C     Open the overall model 
      call errchk( popen("mygob") )
      
      
C     Add spheres at points 1 and 2 
      call errchk( popen(" ") )
      call errchk( ptrans(pt1(1), pt1(2), pt1(3)))
      call errchk( psphr() )
      call errchk( pclose() )
      call errchk( popen(" ") )
      call errchk( ptrans(pt2(1), pt2(2), pt2(3)))
      call errchk( psphr() )
      call errchk( pclose() )

C     Add a blue cylinder between the two points 
      call errchk( popen(" ") )
      call errchk( pgbclr(0,blue(1),blue(2),blue(3),blue(4)) )

      call make_connecting_cylinder( pt1, pt2, 0.5 )

      call errchk( pclose() )
      
      
C     Close the overall model 
      call errchk( pclose() )
      

C     Cause the GOB "mygob" to be rendered. 
      call errchk( psnap("mygob","standard_lights","this_camera") )

C     Shut down the DrawP3D software. 
      call errchk( pshtdn() )

      end


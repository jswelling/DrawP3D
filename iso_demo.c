#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "p3dgen.h"
#include "drawp3d.h"

/* Macro to ease checking of error return codes */
#define ERRCHK( string ) if (!(string)) fprintf(stderr,"ERROR! %s:%d: %s\n", __FILE__ , __LINE__ , #string )

static char* progname= NULL;

/* Camera information */
static P_Point lookfrom= { 1.0, 2.0, 17.0 };
static P_Point lookat= { 0.0, 0.0, 0.0 };
static P_Vector up= { 0.0, -1.0, 0.0 };
static float fovea= 45.0;
static float hither= -5.0;
static float yon= -30.0;
static P_Point llf= {-5.0, -5.0, -5.0};
static P_Point trb= {5.0, 5.0, 5.0};

/* Light source information */
static P_Point light_loc= {0.0, -5.0, 10.0};
static P_Color light_color= { P3D_RGB, 0.7, 0.7, 0.7, 1.0 };
static P_Color ambient_color= { P3D_RGB, 0.3, 0.3, 0.3, 1.0 };

/* Some handy colors */
static P_Color white= { P3D_RGB, 1.0, 1.0, 1.0, 1.0 };
static P_Color red= { P3D_RGB, 0.8, 0.2, 0.2, 1.0 };
static P_Color green= { P3D_RGB, 0.2, 0.8, 0.2, 1.0 };
static P_Color blue= { P3D_RGB, 0.2, 0.2, 0.8, 1.0 };
static P_Color yellow= { P3D_RGB, 0.8, 0.8, 0.2, 1.0 };
static P_Color aqua= { P3D_RGB, 0.2, 0.8, 0.8, 1.0 };
static P_Color pink= { P3D_RGB, 0.8, 0.2, 0.8, 1.0 };
static P_Color gray= { P3D_RGB, 0.5, 0.5, 0.5, 1.0 };

/* Dimensions of the grid from which the isosurface is to be extracted */
#define ISO_NX 109
#define ISO_XDIM 10
#define ISO_NY 256
#define ISO_NZ 256
#ifdef never
#define ISO_NX 64
#define ISO_NY 64
#define ISO_NZ 64
#endif

static void load_iso_data( char* fname, float data[ISO_XDIM][ISO_NY][ISO_NZ],
			   const int iso_xoff )
/* This function provides the values of which an iso-valued surface is
 * found.
 */
{
  int i, j, k;
  long offset;

  FILE* infile= fopen(fname,"r");
  if (!infile) {
    fprintf(stderr,"Cannot open %s for reading!\n",fname);
    exit(-1);
  }

  offset= iso_xoff*ISO_NY*ISO_NZ*sizeof(char);

  fseek(infile,offset,SEEK_SET);

  for (i=0; i<ISO_XDIM; i++) 
    for (j=0; j<ISO_NY; j++) 
      for (k=0; k<ISO_NZ; k++) 
	data[i][j][k]= (float)fgetc(infile);

  fclose(infile);
}

static void create_isosurf( char* fname, float val, 
			    const int my_rank, const int iso_xoff_base )
/* This function generates two grids of data, and then produces an
 * isosurface of one colored by the other.
 */
{
  P_Point corner1, corner2;
  float data[ISO_XDIM][ISO_NY][ISO_NZ];
  int inner_surface= 0;
  int iso_xoff;

  /* Which chunk shall we draw? */
  /* Everyone draws the same chunk - cpg */
  iso_xoff= iso_xoff_base;
/*   iso_xoff= iso_xoff_base + my_rank*(ISO_XDIM-1); */

  /* Set the corners of the region in which to draw the isosurface 
   * and bounding box.
   */
  corner1.x= llf.x + ((float)(iso_xoff)/(float)(ISO_NX))*(trb.x-llf.x);
  corner1.y= llf.y;
  corner1.z= llf.z;
  corner2.x= llf.x + ((float)(iso_xoff+ISO_XDIM)/(float)(ISO_NX))*(trb.x-llf.x);
  corner2.y= trb.y;
  corner2.z= trb.z;

  /* Calculate the data to contour, and the values with which to color it */
  if ( my_rank == 0 )
    fprintf(stderr, "iso_demo:  loading dataset\n" );
  load_iso_data(fname, data, iso_xoff);

  /* This sets the color map which will be used to map the valdata values
   * to colors on the isosurface.
   */
  ERRCHK( dp_std_cmap(0.0, 1.0, 1) );

  /* Open a GOB into which to put the isosurface and bounding box */
  ERRCHK( dp_open("mygob") );

  /* Pick a color to indicate which process we are */
  switch (my_rank % 8) {
  case 0: ERRCHK( dp_gobcolor(&white) ); break;
  case 1: ERRCHK( dp_gobcolor(&red) ); break;
  case 2: ERRCHK( dp_gobcolor(&green) ); break;
  case 3: ERRCHK( dp_gobcolor(&blue) ); break;
  case 4: ERRCHK( dp_gobcolor(&yellow) ); break;
  case 5: ERRCHK( dp_gobcolor(&aqua) ); break;
  case 6: ERRCHK( dp_gobcolor(&pink) ); break;
  case 7: ERRCHK( dp_gobcolor(&gray) ); break;
  }

  /* This actually generates the isosurface GOB.  See the documentation
   * for an explaination of the parameters.
   */
  ERRCHK( dp_isosurface( P3D_CNVTX, (float *)data, NULL, 
			ISO_XDIM, ISO_NY, ISO_NZ, val, 
			&corner1, &corner2, inner_surface) );

  /* This adds a bounding box, to show the boundaries of the grid of
   * data from which the isosurface is extracted.
   */
  if (my_rank == 0) {
    ERRCHK( pg_boundbox( &llf, &trb ) );
  }

  /* Close the GOB which holds the new objects. */
  ERRCHK( dp_close() );
}

static void printUsage(const int iso_xoff_base, const int nprocs)
{
  fprintf(stderr,"Usage: %s nreps axis isoval fname\n",progname);
  fprintf(stderr,"\n");
  fprintf(stderr," where nreps is an integer for number of rotation steps\n");
  fprintf(stderr,"       axis is x, y, or z\n");
  fprintf(stderr,"       isoval is the constant isosurface value (0-255)\n");
  fprintf(stderr,"       fname is the name of a %d x %d x %d grid of bytes\n",
	  iso_xoff_base+(nprocs+1)*ISO_XDIM, ISO_NY, ISO_NZ);
}

int main(int argc, char* argv[])
{
  int i;
  float theta;
  float dtheta= 30.0; /* 30 degree rotation */
  int cl_error= 0;
  int my_rank = 0;
  int nprocs = 1;
  int nreps= 0;
  int axis;
  float isoval;
  char renDataString[256];
  int iso_xoff_base;

  progname= argv[0];

  /* Which chunk shall we draw? Well, let's do a band down the middle of the
   * data, just as wide as the number of processors can handle.
   */
  iso_xoff_base= ISO_NX - (ISO_XDIM-1);
  if ( iso_xoff_base < 0 ) {
    fprintf(stderr,"Too many processors for the data!\n");
    exit(-1);
  }
  iso_xoff_base /= 2;
  
  if (argc != 5) cl_error++;
  
  if (!cl_error && (nreps= atoi(argv[1])) == 0) cl_error++;
  if (!cl_error
      && strcmp(argv[2],"x") && strcmp(argv[2],"y") && strcmp(argv[2],"z"))
    cl_error++;
  else axis= *argv[2];
  if (!cl_error) isoval = atof(argv[3]);

  if (cl_error) {
    printUsage( iso_xoff_base, nprocs );
    exit(-1);
  }
  
  /* Initialize the renderer */
  fprintf(stderr,"iso_demo:  building environment\n");
  sprintf(renDataString,"title=\"%s\",nprocs=%d,rank=%d",progname,nprocs,my_rank);
  ERRCHK( dp_init_ren("myrenderer","gl","",renDataString) );

  /* Generate a light source and camera */
  fprintf(stderr,"iso_demo:  generating light source & camera\n");
  ERRCHK( dp_open("mylights") );
  ERRCHK( dp_light(&light_loc, &light_color) );
  ERRCHK( dp_ambient(&ambient_color) );
  ERRCHK( dp_close() );
  ERRCHK( dp_camera("mycamera",&lookfrom,&lookat,&up,fovea,hither,yon) );

  /* This function actually generates the isosurface GOB, and a bounding
   * box to show the limits of the grid from which the surface is extracted.
   */
  create_isosurf(argv[4],isoval,my_rank,iso_xoff_base);

  fprintf(stderr,"iso_demo:  rendering\n");

  /* Cause the isosurface and bounding box to be rendered */
  theta= 0.0;
  for (i=0; i<nreps; i++) {
    P_Vector x_axis= { 1.0, 0.0, 0.0 };
    P_Vector y_axis= { 0.0, 1.0, 0.0 };
    P_Vector z_axis= { 0.0, 0.0, 1.0 };

    ERRCHK( dp_open("scene") );

    ERRCHK( dp_open("") );
    switch (axis) {
    case 'x':
      ERRCHK( dp_rotate( &x_axis, theta ) );
      break;
      
    case 'y':
      ERRCHK( dp_rotate( &y_axis, theta ) );
      break;
    case 'z':
      ERRCHK( dp_rotate( &z_axis, theta ) );
      break;
    }
    ERRCHK( dp_child( "mygob" ) );
    ERRCHK( dp_close() );
    
    ERRCHK( dp_close() ); /* close "scene" */
    ERRCHK( dp_snap("scene","mylights","mycamera") );
    
    fprintf( stderr, "\riso_demo:  frame %d complete", i ); 
    theta += dtheta;
  }

  /* Close DrawP3D */
  ERRCHK( dp_shutdown() );

  return 0;
}


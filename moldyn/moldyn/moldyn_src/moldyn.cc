/* Moldyn - Molecular Dynamics Simulation */

#include <assert.h>
#include <math.h>
#include <immintrin.h>
#include <stdio.h>

#include "./molecule.h"

#include <mutex>
#include <thread>
#include <vector>

//using namespace std;

std::vector<std::thread> threads;
std::mutex transaction;

#define BEGIN_TRANSACTION() transaction.lock()

#define END_TRANSACTION() transaction.unlock()

#define CREATE_TM_THREADS(...)
#define DESTROY_TM_THREADS(...)
#define PARALLEL_EXECUTE(n, func, data) \
  do{ \
    for(int __i=0; __i < n; ++__i) { \
      auto __l = [=] () { func(data, __i); };\
      threads.push_back(std::thread(__l)); \
    } \
    for (auto &__t : threads) \
      __t.join(); \
    threads.clear(); \
  } while(0)

#define TM_INIT()

#define LOCAL       /* Such a function that changes no global vars */
#define INPARAMS    /* All parameters following this are 'in' params  */

#define SQRT(a)  sqrt(a)
#define POW(a,b) pow(a,b)
#define SQR(a)   ((a)*(a))
#define DRAND(a)  drand_x(a)

int NTHREADS;

#define NDIM 3

extern long random();
extern int srandom();

/*
!======================  DATA-SETS  ======================================
*/


# ifdef  SMALL
#      define BOXSIZE                 4    /* creates 256 molecules */
#      define NUMBER_TIMESTEPS       30     
#      define MAXINTERACT         32000    /* size of interaction array */
# elif defined(MEDIUM)
#      define BOXSIZE                 8 
#      define NUMBER_TIMESTEPS       30 
#      define MAXINTERACT        320000 
# elif defined(SUPER)
#      define BOXSIZE                20 
#      define NUMBER_TIMESTEPS       30 
#      define MAXINTERACT     5120000 
# else
#      define BOXSIZE                13 
#      define NUMBER_TIMESTEPS       30 
#      define MAXINTERACT       1600000 
# endif     

#define NUM_PARTICLES      (4*BOXSIZE*BOXSIZE*BOXSIZE)
#define DENSITY            0.83134 
#define TEMPERATURE        0.722
#define CUTOFF             3.5000
#define DEFAULT_TIMESTEP   0.064
#define SCALE_TIMESTEP     4
#define TOLERANCE          1.2  

#define DIMSIZE NUM_PARTICLES
#define DSIZE   2
#define INDX(aa,bb)  (((aa)*DSIZE) + (bb))    /* used to index inter  */ 
#define IND(aa,bb)   ((aa)*DIMSIZE + (bb))    /* used to index x,f,vh */ 
#define MIN(a,b)     (((a)<(b))?(a):(b))       

/*
!======================  GLOBAL ARRAYS ======================================
!
! Note : inter is usually the biggest array. If BOXSIZE = 13, it will
!        cause 1 million interactions to be generated. This will need
!        a minimum of 80 MBytes to hold 'inter'. The other
!        arrays will need about a sum of around 800 KBytes. Note
!        that MAXINTERACT may be defined to a more safe value causing
!        extra memory to be allocated. (~ 130 MBytes !)
!============================================================================
*/

//  Setup for eventual operator overloading
#define MOLDYN_DOUBLE double
#define MOLDYN_INT    int

//  Initialize 1 global lock for TSX fallback
TM_INIT();

//  Parameters
int MAX_RETRIES;
int TXN_SIZE;

// main data structures for tracking position, forces and velocities
Molecule *molecules;
MOLDYN_DOUBLE *vh;

// Replaced old data structure to track interactions
int *inter;

#ifdef MEASURE
int connect[NUM_PARTICLES];
#endif

/*
!======================  GLOBAL VARIABLES ===================================
*/

double      side;                  /*  length of side of box                 */ 
double      sideHalf;              /*  1/2 of side                           */
double      cutoffRadius;          /*  cuttoff distance for interactions     */
int         neighUpdate;           /*  timesteps between interaction updates */
double      perturb;               /*  perturbs initial coordinates          */ 

double      timeStep;              /*  length of each timestep   */
double      timeStepSq;            /*  square of timestep        */
double      timeStepSqHalf;        /*  1/2 of square of timestep */

int         numMoles;              /*  number of molecules                   */
MOLDYN_INT  ninter;                /*  number of interacting molecules pairs  */
double      vaver;                 /*                                        */

MOLDYN_DOUBLE   epot;              /*  The potential energy      */
MOLDYN_DOUBLE   vir;               /*  The virial  energy        */
MOLDYN_DOUBLE   count, vel ;
MOLDYN_DOUBLE   ekin;

int         n3;

/* ----------- UTILITY ROUTINES --------------------------- */

/*
!=============================================================
!  Function : drand_x()
!  Purpose  :
!    Used to calculate the distance between two molecules
!    given their coordinates.
!=============================================================
*/
LOCAL double drand_x(double x)
{
  double tmp = ( (double) random() ) * 4.6566128752458e-10;
#ifdef PRINT_RANDS
  printf("%lf\n", tmp);
#endif
  return tmp;
}

/*
!=============================================================
!  Function : Foo()
!  Purpose  :
!    Used to calculate the distance between two molecules
!    given their coordinates.
!=============================================================
*/
inline LOCAL double Foo(double xi, double yi, double zi, double xj, double yj, double zj)
{
  double xx, yy, zz, rd;

  xx = xi - xj;
  yy = yi - yj;
  zz = zi - zj;
  if ( xx < -sideHalf ) xx += side ;
  if ( yy < -sideHalf ) yy += side ;
  if ( zz < -sideHalf ) zz += side ;
  if ( xx >  sideHalf ) xx -= side ;
  if ( yy >  sideHalf ) yy -= side ;
  if ( zz >  sideHalf ) zz -= side ;
  rd = xx*xx + yy*yy + zz*zz ;
  return (rd);
}


/*---------- INITIALIZATION ROUTINES HERE               ------------------*/


/*
!============================================================================
!  Function :  InitSettings()
!  Purpose  : 
!     This routine sets up the global variables
!============================================================================
*/

int InitSettings()
{
    numMoles  = 4*BOXSIZE*BOXSIZE*BOXSIZE;

    vh = new MOLDYN_DOUBLE[numMoles * NDIM];

    // Dynamic memory allocation
    molecules = new Molecule[numMoles];
    inter = new int[MAXINTERACT * 2];

#ifdef MEASURE
   connect = new int[numMoles];
#endif

   side   = POW( ((double)(numMoles)/DENSITY), 0.3333333);
   sideHalf  = side * 0.5 ;

   cutoffRadius  = MIN(CUTOFF, sideHalf );

   timeStep      = DEFAULT_TIMESTEP/SCALE_TIMESTEP ;
   timeStepSq    = timeStep   * timeStep ;
   timeStepSqHalf= timeStepSq * 0.5 ;

   neighUpdate   = 10*(1+SCALE_TIMESTEP/4);
   perturb       = side/ (double)BOXSIZE;     /* used in InitCoordinates */
   vaver         = 1.13 * SQRT(TEMPERATURE/24.0);

   n3 = numMoles * 3;

#if (!defined(PRINT_COORDINATES) && !defined(PRINT_INTERACTION_LIST))
   fprintf(stdout,"----------------------------------------------------");
   fprintf(stdout,"\n MolDyn - A Simple Molecular Dynamics simulation \n");
   fprintf(stdout,"----------------------------------------------------");
   fprintf(stdout,"\n number of particles is ......... %6d", numMoles);
   fprintf(stdout,"\n side length of the box is ...... %13.6le",side);
   fprintf(stdout,"\n cut off radius is .............. %13.6le",cutoffRadius);
   fprintf(stdout,"\n temperature is ................. %13.6le",TEMPERATURE);
   fprintf(stdout,"\n time step is ................... %13.6le",timeStep);
   fprintf(stdout,"\n interaction-list updated every..   %d steps",neighUpdate);
   fprintf(stdout,"\n total no. of steps .............   %d ",NUMBER_TIMESTEPS);
   fprintf(stdout,
     "\n TimeStep   K.E.        P.E.        Energy    Temp.     Pres.    Vel.    rp ");
   fprintf(stdout,
     "\n -------- --------   ----------   ----------  -------  -------  ------  ------");
#endif
   
   return 0;
}


/*
!============================================================================
!  Function : InitCoordinates()
!  Purpose  :
!     Initialises the coordinates of the molecules by 
!     distribuuting them uniformly over the entire box
!     with slight perturbations.
!============================================================================
*/

void InitCoordinates(int numMoles, int siz, double perturb) {
  int i, j, k, n;
  
  double x_base, y_base, z_base;
  double offset;
  
  int npoints = siz * siz * siz ; 
  for ( n =0; n< npoints; n++) {
    k   = n % siz ;
    j   = (int)((n-k)/siz) % siz;
    i   = (int)((n - k - j*siz)/(siz*siz)) % siz ; 
    
    x_base = i * perturb;
    y_base = j * perturb;
    z_base = k * perturb;
    offset = perturb * 0.5;

    molecules[n].x = x_base ;
    molecules[n].y = y_base ;
    molecules[n].z = z_base ;
    
    molecules[n + npoints].x = x_base + offset;
    molecules[n + npoints].y = y_base + offset;
    molecules[n + npoints].z = z_base;

    molecules[n + npoints * 2].x = x_base + offset;
    molecules[n + npoints * 2].y = y_base;
    molecules[n + npoints * 2].z = z_base + offset;

    molecules[n + npoints * 3].x = x_base;
    molecules[n + npoints * 3].y = y_base + offset;
    molecules[n + npoints * 3].z = z_base + offset;
  }
} 

/*
!============================================================================
! Function  :  InitVelocities()
! Purpose   :
!    This routine initializes the velocities of the 
!    molecules according to a maxwellian distribution.
!============================================================================
*/

int  InitVelocities(double h)
{
   int i, nmoles1, nmoles2, iseed;
   double ts, sp, sc, r, s;
   double u1, u2, v1, v2, ujunk,tscale;
   double DRAND(double);

   iseed = 4711;
   ujunk = DRAND(iseed);
   (void)ujunk;       //  explicitly turn off unused variable warning
   iseed = 0;
   tscale = (16.0)/(1.0*numMoles - 1.0);
   
   for ( i =0; i< n3; i=i+2) {
     do {
       u1 = DRAND(iseed);
       u2 = DRAND(iseed);
       v1 = 2.0 * u1   - 1.0;
       v2 = 2.0 * u2   - 1.0;
       s  = v1*v1  + v2*v2 ;
     } while( s >= 1.0 );

     r = SQRT( -2.0*log(s)/s );

     vh[i]    = v1 * r;
     vh[i+1]  = v2 * r;
   }      

   // There are three parts - repeat for each part
   nmoles1 = n3/3 ;
   nmoles2 = nmoles1 * 2; 

   //  Find the average speed  for the 1st part
   sp   = 0.0 ;
   for ( i=0; i<nmoles1; i++) {
     sp = sp + vh[i];
   } 
   sp   = sp/nmoles1;


   //  Subtract average from all velocities of 1st part
   for ( i=0; i<nmoles1; i++) {
     vh[i] = vh[i] - sp;
   }

   //  Find the average speed for 2nd part
   sp   = 0.0 ;
   for ( i=nmoles1; i<nmoles2; i++) { 
     sp = sp + vh[i];
   }
   sp   = sp/(nmoles2-nmoles1);

   //  Subtract average from all velocities of 2nd part
   for ( i=nmoles1; i<nmoles2; i++) {
     vh[i] = vh[i] - sp;
   }

   //  Find the average speed for 2nd part
   sp   = 0.0 ;
   for ( i=nmoles2; i<n3; i++) { 
     sp = sp + vh[i];
   }
   sp   = sp/(n3-nmoles2);

   //  Subtract average from all velocities of 2nd part
   for ( i=nmoles2; i<n3; i++) {
     vh[i] = vh[i] - sp;
   }

   // Determine total kinetic energy
   ekin = 0.0 ;
   for ( i=0 ; i< n3; i++ ) {
     ekin  = ekin  + vh[i]*vh[i] ;
   }
   ts = tscale * ekin ;
   sc = h * SQRT(TEMPERATURE/ts);
   for ( i=0; i< n3; i++) {
     vh[i] = vh[i] * sc ;
   }

   return 0;
}

/*
!============================================================================
!  Function :  InitForces()
!  Purpose :
!    Initialize all the partial forces to 0.0
!============================================================================
*/

int InitForces() {
  int i;

  for ( i=0; i < numMoles; ++i ) {
    molecules[i].f_x = 0.0 ;
    molecules[i].f_y = 0.0 ;
    molecules[i].f_z = 0.0 ;
  }

  return 0;
}

int UpdateCoordinates( void* arg, int id ) {
  for (int i = (numMoles / NTHREADS ) * id; i < (numMoles / NTHREADS) * (id + 1); ++i) {
    vh[IND(0,i)] += molecules[i].f_x;
    molecules[i].x += vh[IND(0,i)];
    if ( molecules[i].x < 0.0 )    molecules[i].x += side ;
    if ( molecules[i].x > side   ) molecules[i].x -= side ;
    molecules[i].f_x = 0.0;

    vh[IND(1,i)] += molecules[i].f_y;
    molecules[i].y += vh[IND(1,i)];
    if ( molecules[i].y < 0.0 )    molecules[i].y += side ;
    if ( molecules[i].y > side   ) molecules[i].y -= side ;
    molecules[i].f_y = 0.0;

    vh[IND(2,i)] += molecules[i].f_z;
    molecules[i].z += vh[IND(2,i)];
    if ( molecules[i].z < 0.0 )    molecules[i].z += side ;
    if ( molecules[i].z > side   ) molecules[i].z -= side ;
    molecules[i].f_z = 0.0;
  }
  
  return 0;
}

/*---------- UPDATE ROUTINES HERE               ------------------*/

int PrintCoordinates(INPARAMS int numMoles) {
  int i = 0;
  for (i = 0; i < numMoles; i++) {
     printf("%d: %f:%f -- %f:%f -- %f:%f\n", i, (double)molecules[i].x, (double)vh[IND(0,i)], (double)molecules[i].y, (double)vh[IND(1,i)], (double)molecules[i].z, vh[IND(2,i)]);
  }

  return 0;
}

/*
!============================================================================
!  Function :  BuildNeigh()
!  Purpose  : 
!     This routine is called after every x timesteps
!     to  rebuild the list of interacting molecules
!     Note that molecules within cutoffRad+TOLERANCE
!     are included. This tolerance is in order to allow
!     for molecules that might move within range
!     during the computation.
!============================================================================
*/

//  TODO: Can use dynamic scheduling here
int BuildNeigh( void* arg, int id ) {
  double rd, cutoffSquare;
  int    i,j;
  
  cutoffSquare  = (cutoffRadius * TOLERANCE)*(cutoffRadius * TOLERANCE); 

  for (i = (numMoles / NTHREADS) * id; i < (numMoles / NTHREADS) * (id + 1); ++i) {
    for (j = i+1; j<numMoles; j++ ) {
      rd = Foo ( molecules[i].x, molecules[i].y, molecules[i].z, 
                 molecules[j].x, molecules[j].y, molecules[j].z); 

      if (rd > cutoffSquare) {
        continue;
      }
      
      BEGIN_TRANSACTION();

      inter[INDX(ninter,0)] = i;
      inter[INDX(ninter,1)] = j;
      ++ninter;   

      END_TRANSACTION();

      if ( ninter >= MAXINTERACT) { 
        perror("MAXINTERACT limit");
      }
    } 
  }
 
  return 0; 
}

void PrintInteractionList(INPARAMS int ninter)
{
  int i;
  printf("%d\n", ninter);
  for (i=0;i<ninter;i++) {
     printf("%d %d\n", inter[INDX(i,0)], inter[INDX(i,1)]);
  }
}

/*
!============================================================================
! Function :  ComputeForces
! Purpose  :
!   This is the most compute-intensive portion.
!   The routine iterates over all interacting  pairs
!   of molecules and checks if they are still within
!   inteacting range. If they are, the force on
!   each  molecule due to the other is calculated.
!   The net potential energy and the net virial
!   energy is also computed.
!============================================================================
*/

int ComputeForces( void* arg, int id ) {
  double cutoffSquare;
  double xx, yy, zz, rd, rrd, rrd2, rrd3, rrd4, rrd6, rrd7, r148; //  rrd5 never used, removed
  double forcex, forcey, forcez;
  int    i, k;
  double vir_tmp, epot_tmp;

  cutoffSquare = cutoffRadius*cutoffRadius ;

  vir_tmp  = 0.0;
  epot_tmp = 0.0;
  
  int finish_outer;
  
  if (id == NTHREADS - 1) {
    finish_outer = ninter;
  } else {
    finish_outer = (ninter / NTHREADS) * (id + 1);
  }

  for(int ii = (ninter / NTHREADS) * id; ii < finish_outer; ++ii) {
    i = inter[INDX(ii,0)];
    k = inter[INDX(ii,1)];

    xx = molecules[i].x - molecules[k].x;
    yy = molecules[i].y - molecules[k].y;
    zz = molecules[i].z - molecules[k].z;
 
    if (xx < -sideHalf) xx += side;
    if (yy < -sideHalf) yy += side;
    if (zz < -sideHalf) zz += side;
    if (xx > sideHalf) xx -= side;
    if (yy > sideHalf) yy -= side;
    if (zz > sideHalf) zz -= side;

    rd = (xx*xx + yy*yy + zz*zz);
    if ( rd < cutoffSquare ) {
      rrd   = 1.0/rd;          
      rrd2  = rrd*rrd ;       
      rrd3  = rrd2*rrd ;
      rrd4  = rrd2*rrd2 ;
      rrd6  = rrd2*rrd4;
      rrd7  = rrd6*rrd ;
      r148  = rrd7 - 0.5 * rrd4 ;

      forcex = xx*r148;
      forcey = yy*r148;
      forcez = zz*r148;

      BEGIN_TRANSACTION();
      
      molecules[i].f_x  += forcex ;
      molecules[i].f_y  += forcey ;
      molecules[i].f_z  += forcez ;

      molecules[k].f_x  -= forcex ;
      molecules[k].f_y  -= forcey ;
      molecules[k].f_z  -= forcez ;

      END_TRANSACTION();
 
      vir_tmp  -= rd*r148 ;
      epot_tmp += (rrd6 - rrd3);

    }
  }
  
  BEGIN_TRANSACTION();
 
  vir  += vir_tmp ;
  epot += epot_tmp;

  END_TRANSACTION();

  return 0;
}

/*
!============================================================================
!  Function : UpdateVelocities
!============================================================================
*/

int UpdateVelocities( void* arg, int id ) {
  int i;
  int finish;
    
  if (id == NTHREADS - 1) {
    finish = numMoles;
  } else {
    finish = (numMoles / NTHREADS) * (id + 1);
  }

  for (i = (numMoles / NTHREADS) * id; i < finish; ++i) {
    molecules[i].f_x  = molecules[i].f_x * timeStepSqHalf ;
    molecules[i].f_y  = molecules[i].f_y * timeStepSqHalf ;
    molecules[i].f_z  = molecules[i].f_z * timeStepSqHalf ;
   
    vh[ IND(0,i) ] += molecules[i].f_x;
    vh[ IND(1,i) ] += molecules[i].f_y;
    vh[ IND(2,i) ] += molecules[i].f_z;
  }

  return 0;
}

/*
!============================================================================
!  Function :  ComputeKE()
!============================================================================
*/

int ComputeKE( void* arg, int id ){
  double sum = 0.0;
  int    i;

  int finish;
  if (id == NTHREADS - 1) finish = numMoles;
  else finish = (numMoles / NTHREADS) * (id + 1);

  for ( i = (numMoles / NTHREADS ) * id; i < finish; i++) {
    sum = sum + vh[ IND(0,i) ] * vh[ IND(0,i) ]; 
    sum = sum + vh[ IND(1,i) ] * vh[ IND(1,i) ]; 
    sum = sum + vh[ IND(2,i) ] * vh[ IND(2,i) ]; 
  } 
    
  BEGIN_TRANSACTION();
  
  ekin += sum/timeStepSq ;

  END_TRANSACTION();

  return 0;
}


/*
!============================================================================
!  Function :  ComputeAvgVel()
!============================================================================
*/

int ComputeAvgVel( void* arg, int id ) {
  double vaverh, velocity, counter, sq;
  int    i;

  vaverh = vaver * timeStep ;
  velocity    = 0.0 ;
  counter     = 0.0 ;
    
  int finish;
  if (id == NTHREADS - 1) {
    finish = numMoles;
  }else {
    finish = (numMoles / NTHREADS) * (id + 1);
  }

  for (i = (numMoles / NTHREADS) * id; i < finish; i++) {
    sq = SQRT(SQR(vh[IND(0,i)]) + SQR(vh[IND(1,i)]) + SQR(vh[IND(2,i)]));

    if (sq > vaverh) {
      counter += 1.0;
    }
    
    velocity += sq ;
  }  

  BEGIN_TRANSACTION();

  vel += (velocity/timeStep);
  count += counter;

  END_TRANSACTION();

  return 0;
}

/*
!=============================================================
!  Function : PrintResults()
!  Purpose  :
!    Prints out the KE, the PE and other results
!=============================================================
*/

LOCAL void PrintResults(int move, double ekin, double epot,  double vir, double vel, double count, int numMoles, int ninteracts) {
  double ek, etot, temp, pres, rp, tscale ;

  ek   = 24.0 * ekin ;
  epot = 4.00 * epot ;
  etot = ek + epot ;
  tscale = (16.0)/((double)numMoles - 1.0);
  temp = tscale * ekin ;
  pres = DENSITY * 16.0 * (ekin-vir)/numMoles ;
  vel  = vel/numMoles;

  rp   = (count/(double)(numMoles)) * 100.0 ;

  fprintf(stdout,
          "\n %4d %12.4lf %12.4lf %12.4lf %8.4lf %8.4lf %8.4lf %5.1lf",
          move, ek,    epot,   etot,   temp,   pres,   vel,     rp);
#ifdef DEBUG
  fprintf(stdout,"\n\n In the final step there were %d interacting pairs\n", ninteracts);
#endif
}

void dump_values(char *s) {
  int i;
  printf("\n%s\n", s);
  for (i=0;i<n3/3;i++) {
    printf("%d: coord = (%lf, %lf, %lf), vel = (%lf, %lf, %lf), force = (%lf, %lf, %lf)\n", 
            i, (double)molecules[i].x, (double)molecules[i].y, (double)molecules[i].z, 
            (double)vh[IND(0,i)], (double)vh[IND(1,i)], (double)vh[IND(2,i)], 
            (double)molecules[i].f_x, (double)molecules[i].f_y, (double)molecules[i].f_z);      
  }
}

void do_libtm_work(void) {
  int tstep, i;
  for ( tstep=0; tstep< NUMBER_TIMESTEPS; tstep++) {
    PARALLEL_EXECUTE( NTHREADS, UpdateCoordinates, NULL );
 
    vir  = 0.0;
    epot = 0.0;
    ekin = 0.0;
    vel = 0.0;
    count = 0.0;

    if ( tstep % neighUpdate == 0) {
      ninter = 0;
      
      PARALLEL_EXECUTE( NTHREADS, BuildNeigh, NULL );
 
# ifdef PRINT_COORDINATES
      PrintCoordinates(numMoles); 
# endif

# ifdef PRINT_INTERACTION_LIST     
      PrintInteractionList(INPARAMS ninter);
# endif 

# ifdef MEASURE
      PrintConnectivity();
# endif
    }

    for (i = 0; i < numMoles; ++i) {
      molecules[i].f_x = 0;
      molecules[i].f_y = 0;
      molecules[i].f_z = 0;
    }  
 
    PARALLEL_EXECUTE( NTHREADS, ComputeForces, NULL );

    PARALLEL_EXECUTE( NTHREADS, UpdateVelocities, NULL );

    PARALLEL_EXECUTE( NTHREADS, ComputeKE, NULL );
  
    PARALLEL_EXECUTE( NTHREADS, ComputeAvgVel, NULL );

    PrintResults (INPARAMS tstep, (double)ekin, (double)epot, (double)vir,(double)vel,(double)count,numMoles,(int)ninter);

  } 
  
  return;
}


/*
!============================================================================
!  Function : main()
!  Purpose  :  
!      All the main computational structure  is here
!      Iterates for specified number of  timesteps.
!      In each time step, 
!        UpdateCoordinates() changes molecules coordinates based
!              on the velocity of that molecules
!              and that molecules
!        BuildNeigh() rebuilds the interaction-list on
!              certain time-steps
!        ComputeForces() - the time-consuming step, iterates
!              over all interacting pairs and computes forces
!        UpdateVelocities() - updates the velocities of
!              all molecules  based on the forces. 
!============================================================================
*/

int main( int argc, char *argv[]) {
  if (argc < 3) {
    printf("Usage: ./moldyn NTHREADS MAX_RETRIES\n");
    return 1;
  }

  NTHREADS = atoi(argv[1]);
  MAX_RETRIES = atoi(argv[2]);

  InitSettings   ();
  InitCoordinates(INPARAMS numMoles, BOXSIZE, perturb);

  InitVelocities (INPARAMS timeStep);
  InitForces     ();
 
  CREATE_TM_THREADS( NTHREADS );

  do_libtm_work();

  DESTROY_TM_THREADS( NTHREADS );

  printf("\n");

  delete [] molecules;
  delete [] vh;
  delete [] inter;

  return 0;
}

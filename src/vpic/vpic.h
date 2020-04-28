/*
 * Written by:
 *   Kevin J. Bowers, Ph.D.
 *   Plasma Physics Group (X-1)
 *   Applied Physics Division
 *   Los Alamos National Lab
 * March/April 2004 - Heavily revised and extended from earlier V4PIC versions
 *
 * snell - revised to add new dumps, 20080310
 *
 */

#ifndef vpic_h
#define vpic_h

#include <vector>
#include <cmath>

#include "../boundary/boundary.h"
#include "../collision/collision.h"
#include "../emitter/emitter.h"
// FIXME: INCLUDES ONCE ALL IS CLEANED UP
#include "../util/io/FileIO.h"
#include "../util/bitfield.h"
#include "../util/checksum.h"
#include "../util/system.h"
#include "../util/rng_policy.h"

#include "../collision/kokkos/takizuka_abe.h"

#ifndef USER_GLOBAL_SIZE
#define USER_GLOBAL_SIZE 16384
#endif

#ifndef NVARHISMX
#define NVARHISMX 250
#endif
//  #include "dumpvars.h"

typedef FileIO FILETYPE;

// TODO: this has previously conflicted with an internal definition somewhere, and should be renamed
const uint32_t all       (0xffffffff);
const uint32_t electric  (1<<0 | 1<<1 | 1<<2);
const uint32_t div_e_err (1<<3);
const uint32_t magnetic  (1<<4 | 1<<5 | 1<<6);
const uint32_t div_b_err (1<<7);
const uint32_t tca       (1<<8 | 1<<9 | 1<<10);
const uint32_t rhob      (1<<11);
const uint32_t current   (1<<12 | 1<<13 | 1<<14);
const uint32_t rhof      (1<<15);
const uint32_t emat      (1<<16 | 1<<17 | 1<<18);
const uint32_t nmat      (1<<19);
const uint32_t fmat      (1<<20 | 1<<21 | 1<<22);
const uint32_t cmat      (1<<23);

const size_t total_field_variables(24);
const size_t total_field_groups(12); // this counts vectors, tensors etc...
// These bits will be tested to determine which variables to output
const size_t field_indeces[12] = { 0, 3, 4, 7, 8, 11, 12, 15, 16, 19, 20, 23 };

struct FieldInfo {
  char name[128];
  char degree[128];
  char elements[128];
  char type[128];
  size_t size;
}; // struct FieldInfo

const uint32_t current_density  (1<<0 | 1<<1 | 1<<2);
const uint32_t charge_density (1<<3);
const uint32_t momentum_density (1<<4 | 1<<5 | 1<<6);
const uint32_t ke_density   (1<<7);
const uint32_t stress_tensor  (1<<8 | 1<<9 | 1<<10 | 1<<11 | 1<<12 | 1<<13);
/* May want to use these instead
const uint32_t stress_diagonal    (1<<8 | 1<<9 | 1<<10);
const uint32_t stress_offdiagonal (1<<11 | 1<<12 | 1<<13);
*/

const size_t total_hydro_variables(14);
const size_t total_hydro_groups(5); // this counts vectors, tensors etc...
// These bits will be tested to determine which variables to output
const size_t hydro_indeces[5] = { 0, 3, 4, 7, 8 };

struct HydroInfo {
  char name[128];
  char degree[128];
  char elements[128];
  char type[128];
  size_t size;
}; // struct FieldInfo

/*----------------------------------------------------------------------------
 * DumpFormat Enumeration
----------------------------------------------------------------------------*/
enum DumpFormat {
  band = 0,
  band_interleave = 1
}; // enum DumpFormat

/*----------------------------------------------------------------------------
 * DumpParameters Struct
----------------------------------------------------------------------------*/
struct DumpParameters {

  void output_variables(uint32_t mask) {
    output_vars.set(mask);
  } // output_variables

  BitField output_vars;

  size_t stride_x;
  size_t stride_y;
  size_t stride_z;

  DumpFormat format;

  char name[128];
  char baseDir[128];
  char baseFileName[128];

}; // struct DumpParameters

class vpic_simulation {
public:
  vpic_simulation();
  ~vpic_simulation();
  void initialize( int argc, char **argv );
  void modify( const char *fname );
  int advance( void );
  void finalize( void );

  // Set RNG policy
  // Use std::rng by default, optionally use Kokkos or "original"
  // TODO: turn this into a policy
#define USE_ORIGINAL_RNG
#ifndef USE_ORIGINAL_RNG
 #ifdef USE_KOKKOS_RNG
  _RNG::RandomNumberProvider<_RNG::KokkosRNG> rng_policy;
 #else
   // TODO: move to constructor
  _RNG::RandomNumberProvider<_RNG::CppRNG> rng_policy;
 #endif
#else
  _RNG::RandomNumberProvider<_RNG::OriginalRNG> rng_policy;
#endif

  // Directly initialized by user

  int verbose;              // Should system be verbose
  int num_step;             // Number of steps to take
  int num_comm_round;       // Num comm round
  int status_interval;      // How often to print status messages
  int clean_div_e_interval; // How often to clean div e
  int num_div_e_round;      // How many clean div e rounds per div e interval
  int clean_div_b_interval; // How often to clean div b
  int num_div_b_round;      // How many clean div b rounds per div b interval
  int sync_shared_interval; // How often to synchronize shared faces

  // Track whether injection functions necessary
  int field_injection_interval = -1;
  int current_injection_interval = -1;
  int particle_injection_interval = -1;
  // Track whether injection functions are ported to Kokkos
  bool kokkos_field_injection = false;
  bool kokkos_current_injection = false;
  bool kokkos_particle_injection = false;
  // Track how often the user wants us to copy data back from device
  int field_copy_interval = -1;
  int particle_copy_interval = -1;
  // Copy the last time-step on which we knowingly copied data back
  int field_copy_last;
  int particle_copy_last;

  // FIXME: THESE INTERVALS SHOULDN'T BE PART OF vpic_simulation
  // THE BIG LIST FOLLOWING IT SHOULD BE CLEANED UP TOO

  double quota;
  int checkpt_interval;
  int hydro_interval;
  int field_interval;
  int particle_interval;

  size_t nxout, nyout, nzout;
  size_t px, py, pz;
  float dxout, dyout, dzout;

  int ndfld;
  int ndhyd;
  int ndpar;
  int ndhis;
  int ndgrd;
  int head_option;
  int istride;
  int jstride;
  int kstride;
  int stride_option;
  int pstride;
  int nprobe;
  int ijkprobe[NVARHISMX][4];
  float xyzprobe[NVARHISMX][3];
  int block_dump;
  int stepdigit;
  int rankdigit;
  int ifenergies;

  // Helper initialized by user

  /* There are enough synchronous and local random number generators
     to permit the host thread plus all the pipeline threads for one
     dispatcher to simultaneously produce both synchronous and local
     random numbers.  Keeping the synchronous generators in sync is
     the generator users responsibility. */

  rng_pool_t           * entropy;            // Local entropy pool
  rng_pool_t           * sync_entropy;       // Synchronous entropy pool
  grid_t               * grid;               // define_*_grid et al
  material_t           * material_list;      // define_material
  field_array_t        * field_array;        // define_field_array
  interpolator_array_t * interpolator_array; // define_interpolator_array
  accumulator_array_t  * accumulator_array;  // define_accumulator_array
  hydro_array_t        * hydro_array;        // define_hydro_array
  species_t            * species_list;       // define_species /
                                             // species helpers
  particle_bc_t        * particle_bc_list;   // define_particle_bc /
                                             // boundary helpers
  emitter_t            * emitter_list;       // define_emitter /
                                             // emitter helpers
  collision_op_t       * collision_op_list;  // collision helpers

  // User defined checkpt preserved variables
  // Note: user_global is aliased with user_global_t (see deck_wrapper.cxx)

  char user_global[USER_GLOBAL_SIZE];

  /*----------------------------------------------------------------------------
   * Diagnostics
   ---------------------------------------------------------------------------*/
  double poynting_flux(double e0);

  /*----------------------------------------------------------------------------
   * Check Sums
   ---------------------------------------------------------------------------*/
#if defined(ENABLE_OPENSSL)
  void output_checksum_fields();
  void checksum_fields(CheckSum & cs);
  void output_checksum_species(const char * species);
  void checksum_species(const char * species, CheckSum & cs);
#endif // ENABLE_OPENSSL

  void print_available_ram() {
    SystemRAM::print_available();
  } // print_available_ram

  ///////////////
  // Dump helpers

  int dump_mkdir(const char * dname);
  int dump_cwd(char * dname, size_t size);

  // Text dumps
  void dump_energies( const char *fname, int append = 1 );
  void dump_materials( const char *fname );
  void dump_species( const char *fname );

  // Binary dumps
  void dump_grid( const char *fbase );
  void dump_fields( const char *fbase, int fname_tag = 1 );
  void dump_hydro( const char *sp_name, const char *fbase,
                   int fname_tag = 1 );
  void dump_particles( const char *sp_name, const char *fbase,
                       int fname_tag = 1 );

  // convenience functions for simlog output
  void create_field_list(char * strlist, DumpParameters & dumpParams);
  void create_hydro_list(char * strlist, DumpParameters & dumpParams);

  void print_hashed_comment(FileIO & fileIO, const char * comment);
  void global_header(const char * base,
    std::vector<DumpParameters *> dumpParams);

  void field_header(const char * fbase, DumpParameters & dumpParams);
  void hydro_header(const char * speciesname, const char * hbase,
    DumpParameters & dumpParams);

  void field_dump(DumpParameters & dumpParams);
  void hydro_dump(const char * speciesname, DumpParameters & dumpParams);

  ///////////////////
  // Useful accessors

  inline int
  rank() { return world_rank; }

  inline int
  nproc() { return world_size; }

  inline void
  barrier() { mp_barrier(); }

  inline double
  time() {
    return grid->t0 + (double)grid->dt*(double)grid->step;
  }

  inline int64_t &
  step() {
   return grid->step;
  }

  inline field_t &
  field( const int v ) {
    return field_array->f[ v ];
  }

  inline int
  voxel( const int ix, const int iy, const int iz ) {
    return ix + grid->sy*iy + grid->sz*iz;
  }

  inline int
  voxel( const int ix, const int iy, const int iz, const int sy, const int sz ) {
    return ix + sy*iy +sz*iz;
  }

  inline field_t &
  field( const int ix, const int iy, const int iz ) {
    return field_array->f[ voxel(ix,iy,iz) ];
  }

  inline k_field_t& get_field() {
      return field_array->k_f_d;
  }

  inline float& k_field(const int ix, const int iy, const int iz, field_var::f_v member) {
      return field_array->k_f_d(voxel(ix,iy,iz), member);
  }

  inline interpolator_t &
  interpolator( const int v ) {
    return interpolator_array->i[ v ];
  }

  inline interpolator_t &
  interpolator( const int ix, const int iy, const int iz ) {
    return interpolator_array->i[ voxel(ix,iy,iz) ];
  }

  inline hydro_t &
  hydro( const int v ) {
    return hydro_array->h[ v ];
  }

  inline hydro_t &
  hydro( const int ix, const int iy, const int iz ) {
    return hydro_array->h[ voxel(ix,iy,iz) ];
  }

  // TODO: do I need to update this ?
  inline rng_t *
  rng( const int n ) {
    return entropy->rng[n];
  }

  // TODO: do I need to update this ?
  inline rng_t *
  sync_rng( const int n ) {
    return sync_entropy->rng[n];
  }

  ///////////////
  // Grid helpers

  inline void
  define_units( float cvac,
                float eps0 ) {
    grid->cvac = cvac;
    grid->eps0 = eps0;
  }

  inline void
  define_timestep( float dt, double t0 = 0, int64_t step = 0 ) {
    grid->t0   = t0;
    grid->dt   = (float)dt;
    grid->step = step;
  }

  // The below functions automatically create partition simple grids with
  // simple boundary conditions on the edges.

  inline void
  define_periodic_grid( double xl,  double yl,  double zl,
                        double xh,  double yh,  double zh,
                        double gnx, double gny, double gnz,
                        double gpx, double gpy, double gpz ) {
  px = size_t(gpx); py = size_t(gpy); pz = size_t(gpz);
    partition_periodic_box( grid, xl, yl, zl, xh, yh, zh,
                            (int)gnx, (int)gny, (int)gnz,
                            (int)gpx, (int)gpy, (int)gpz );
  }

  inline void
  define_absorbing_grid( double xl,  double yl,  double zl,
                         double xh,  double yh,  double zh,
                         double gnx, double gny, double gnz,
                         double gpx, double gpy, double gpz, int pbc ) {
  px = size_t(gpx); py = size_t(gpy); pz = size_t(gpz);
    partition_absorbing_box( grid, xl, yl, zl, xh, yh, zh,
                             (int)gnx, (int)gny, (int)gnz,
                             (int)gpx, (int)gpy, (int)gpz,
                             pbc );
  }

  inline void
  define_reflecting_grid( double xl,  double yl,  double zl,
                          double xh,  double yh,  double zh,
                          double gnx, double gny, double gnz,
                          double gpx, double gpy, double gpz ) {
  px = size_t(gpx); py = size_t(gpy); pz = size_t(gpz);
    partition_metal_box( grid, xl, yl, zl, xh, yh, zh,
                         (int)gnx, (int)gny, (int)gnz,
                         (int)gpx, (int)gpy, (int)gpz );
  }

  // The below macros allow custom domains to be created

  // Creates a particle reflecting metal box in the local domain
  inline void
  size_domain( double lnx, double lny, double lnz ) {
    size_grid(grid,(int)lnx,(int)lny,(int)lnz);
  }

  // Attaches a local domain boundary to another domain
  inline void join_domain( int boundary, double rank ) {
    join_grid( grid, boundary, (int)rank );
  }

  // Sets the field boundary condition of a local domain boundary
  inline void set_domain_field_bc( int boundary, int fbc ) {
    set_fbc( grid, boundary, fbc );
  }

  // Sets the particle boundary condition of a local domain boundary
  inline void set_domain_particle_bc( int boundary, int pbc ) {
    set_pbc( grid, boundary, pbc );
  }

  ///////////////////
  // Material helpers

  inline material_t *
  define_material( const char * name,
                   double eps,
                   double mu = 1,
                   double sigma = 0,
                   double zeta = 0 ) {
    return append_material( material( name,
                                      eps,   eps,   eps,
                                      mu,    mu,    mu,
                                      sigma, sigma, sigma,
                                      zeta,  zeta,  zeta ), &material_list );
  }

  inline material_t *
  define_material( const char * name,
                   double epsx,        double epsy,       double epsz,
                   double mux,         double muy,        double muz,
                   double sigmax,      double sigmay,     double sigmaz,
       double zetax = 0 ,  double zetay = 0,  double zetaz = 0 ) {
    return append_material( material( name,
                                      epsx,   epsy,   epsz,
                                      mux,    muy,    muz,
                                      sigmax, sigmay, sigmaz,
                                      zetax,  zetay,  zetaz ), &material_list );
  }

  inline material_t *
  lookup_material( const char * name ) {
    return find_material_name( name, material_list );
  }

  inline material_t *
  lookup_material( material_id id ) {
    return find_material_id( id, material_list );
  }

  //////////////////////
  // Field array helpers

  // If fa is provided, define_field_advance will use it (and take ownership
  // of the it).  Otherwise the standard field array will be used with the
  // optionally provided radition damping parameter.

  inline void
  define_field_array( field_array_t * fa = NULL, double damp = 0 ) {
    int nx1 = grid->nx + 1, ny1 = grid->ny+1, nz1 = grid->nz+1;

    if( grid->nx<1 || grid->ny<1 || grid->nz<1 )
      ERROR(( "Define your grid before defining the field array" ));
    if( !material_list )
      ERROR(( "Define your materials before defining the field array" ));

    field_array        = fa ? fa :
                         new_standard_field_array( grid, material_list, damp );
    interpolator_array = new_interpolator_array( grid );
    accumulator_array  = new_accumulator_array( grid );
    hydro_array        = new_hydro_array( grid );

    // Pre-size communications buffers. This is done to get most memory
    // allocation over with before the simulation starts running

    mp_size_recv_buffer(grid->mp,BOUNDARY(-1, 0, 0),ny1*nz1*sizeof(hydro_t));
    mp_size_recv_buffer(grid->mp,BOUNDARY( 1, 0, 0),ny1*nz1*sizeof(hydro_t));
    mp_size_recv_buffer(grid->mp,BOUNDARY( 0,-1, 0),nz1*nx1*sizeof(hydro_t));
    mp_size_recv_buffer(grid->mp,BOUNDARY( 0, 1, 0),nz1*nx1*sizeof(hydro_t));
    mp_size_recv_buffer(grid->mp,BOUNDARY( 0, 0,-1),nx1*ny1*sizeof(hydro_t));
    mp_size_recv_buffer(grid->mp,BOUNDARY( 0, 0, 1),nx1*ny1*sizeof(hydro_t));

    mp_size_send_buffer(grid->mp,BOUNDARY(-1, 0, 0),ny1*nz1*sizeof(hydro_t));
    mp_size_send_buffer(grid->mp,BOUNDARY( 1, 0, 0),ny1*nz1*sizeof(hydro_t));
    mp_size_send_buffer(grid->mp,BOUNDARY( 0,-1, 0),nz1*nx1*sizeof(hydro_t));
    mp_size_send_buffer(grid->mp,BOUNDARY( 0, 1, 0),nz1*nx1*sizeof(hydro_t));
    mp_size_send_buffer(grid->mp,BOUNDARY( 0, 0,-1),nx1*ny1*sizeof(hydro_t));
    mp_size_send_buffer(grid->mp,BOUNDARY( 0, 0, 1),nx1*ny1*sizeof(hydro_t));
  }

  // Other field helpers are provided by macros in deck_wrapper.cxx

  //////////////////
  // Species helpers

  // FIXME: SILLY PROMOTIONS
  inline species_t *
  define_species( const char *name,
                  double q,
                  double m,
                  double max_local_np,
                  double max_local_nm,
                  double sort_interval,
                  double sort_out_of_place ) {
    // Compute a reasonble number of movers if user did not specify
    // Based on the twice the number of particles expected to hit the boundary
    // of a wpdt=0.2 / dx=lambda species in a 3x3x3 domain
    if( max_local_nm<0 ) {
      max_local_nm = 2*max_local_np/25;
      if( max_local_nm<16*(MAX_PIPELINE+1) )
        max_local_nm = 16*(MAX_PIPELINE+1);
    }
    return append_species( species( name, (float)q, (float)m,
                                    (int)max_local_np, (int)max_local_nm,
                                    (int)sort_interval, (int)sort_out_of_place,
                                    grid ), &species_list );
  }

  inline species_t *
  find_species( const char *name ) {
     return find_species_name( name, species_list );
  }

  inline species_t *
  find_species( int32_t id ) {
     return find_species_id( id, species_list );
  }

  ///////////////////
  // Particle helpers

  // Note: Don't use injection with aging during initialization

  // Defaults in the declaration below enable backwards compatibility.

  void
  inject_particle( species_t * sp,
                   double x,  double y,  double z,
                   double ux, double uy, double uz,
                   double w,  double age = 0, int update_rhob = 1 );

  // Inject particle raw is for power users!
  // No nannyism _at_ _all_:
  // - Availability of free stoarge is _not_ checked.
  // - Particle displacements and voxel index are _not_ for validity.
  // - The rhob field is _not_ updated.
  // - Injection with displacment may use up movers (i.e. don't use
  //   injection with displacement during initialization).
  // This injection is _ultra_ _fast_.

  inline void
  inject_particle_raw( species_t * RESTRICT sp,
                       float dx, float dy, float dz, int32_t i,
                       float ux, float uy, float uz, float w ) {
    particle_t * RESTRICT p = sp->p + (sp->np++);
    p->dx = dx; p->dy = dy; p->dz = dz; p->i = i;
    p->ux = ux; p->uy = uy; p->uz = uz; p->w = w;
  }

  // This variant does a raw inject and moves the particles

  inline void
  inject_particle_raw( species_t * RESTRICT sp,
                       float dx, float dy, float dz, int32_t i,
                       float ux, float uy, float uz, float w,
                       float dispx, float dispy, float dispz,
                       int update_rhob ) {
    particle_t       * RESTRICT p  = sp->p  + (sp->np++);
    particle_mover_t * RESTRICT pm = sp->pm + sp->nm;
    p->dx = dx; p->dy = dy; p->dz = dz; p->i = i;
    p->ux = ux; p->uy = uy; p->uz = uz; p->w = w;
    pm->dispx = dispx; pm->dispy = dispy; pm->dispz = dispz; pm->i = sp->np-1;
    if( update_rhob ) accumulate_rhob( field_array->f, p, grid, -sp->q );
    sp->nm += move_p( sp->p, pm, accumulator_array->a, grid, sp->q );
  }

  //////////////////////////////////
  // Random number generator helpers

  // seed_rand seed the all the random number generators.  The seed
  // used for the individual generators is based off the user provided
  // seed such each local generator in each process (rng[0:r-1]) gets
  // a unique seed.  Each synchronous generator (sync_rng[0:r-1]) gets a
  // unique seed that does not overlap with the local generators
  // (common across each process).  Lastly, all these seeds are such
  // that, no individual generator seeds are reused across different
  // user seeds.
  // FIXME: MTRAND DESPERATELY NEEDS A LARGER SEED SPACE!

  inline void seed_entropy( int base ) {
    rng_policy.seed( entropy, sync_entropy, base, 0 );
    //seed_rng_pool( entropy,      base, 0 );
    //seed_rng_pool( sync_entropy, base, 1 );
  }

  // Uniform random number on (low,high) (open interval)
  // FIXME: IS THE INTERVAL STILL OPEN IN FINITE PRECISION
  //        AND IS THE OPEN INTERVAL REALLY WHAT USERS WANT??
  inline double uniform( rng_t * rng, double low, double high ) {
    //double dx = drand( rng );
    //return low*(1-dx) + high*dx;
    return rng_policy.uniform(rng, low, high);
  }

  // Normal random number with mean mu and standard deviation sigma
  inline double normal( rng_t * rng, double mu, double sigma ) {
    //return mu + sigma*drandn( rng );
    return rng_policy.normal(rng, mu, sigma);
  }
  inline unsigned int random_uint( rng_t* rng, unsigned int max ) {
    return rng_policy.uint(rng, max);
  }

  /////////////////////////////////
  // Emitter and particle bc helpers

  // Note that append_emitter is hacked to silently returne if the
  // emitter is already in the list.  This allows things like:
  //
  // define_surface_emitter( my_emitter( ... ), rgn )
  // ... or ...
  // my_emit_t * e = my_emit( ... )
  // define_surface_emitter( e, rgn )
  // ... or ...
  // my_emit_t * e = define_emitter( my_emit( ... ) )
  // define_surface_emitter( e, rng )
  // ...
  // All to work.  (Nominally, would like define_surface_emitter
  // to evaluate to the value of e.  But, alas, the way
  // define_surface_emitter works and language limitations of
  // strict C++ prevent this.)

  inline emitter_t *
  define_emitter( emitter_t * e ) {
    return append_emitter( e, &emitter_list );
  }

  inline particle_bc_t *
  define_particle_bc( particle_bc_t * pbc ) {
    return append_particle_bc( pbc, &particle_bc_list );
  }

  inline collision_op_t *
  define_collision_op( collision_op_t * cop ) {
    return append_collision_op( cop, &collision_op_list );
  }

  ////////////////////////
  // Miscellaneous helpers

  inline void abort( double code ) {
    nanodelay(2000000000); mp_abort((((int)code)<<17)+1);
  }

  // Truncate "a" to the nearest integer multiple of "b"
  inline double trunc_granular( double a, double b ) { return b*int(a/b); }

  // Compute the remainder of a/b
  inline double remainder( double a, double b ) { return std::remainder(a,b); }
  // remainder(a,b);

  // Compute the Courant length on a regular mesh
  inline double courant_length( double lx, double ly, double lz,
        double nx, double ny, double nz ) {
    double w0, w1 = 0;
    if( nx>1 ) w0 = nx/lx, w1 += w0*w0;
    if( ny>1 ) w0 = ny/ly, w1 += w0*w0;
    if( nz>1 ) w0 = nz/lz, w1 += w0*w0;
    return sqrt(1/w1);
  }

  //////////////////////////////////////////////////////////
  // These friends are used by the checkpt / restore service

  friend void checkpt_vpic_simulation( const vpic_simulation * vpic );
  friend vpic_simulation * restore_vpic_simulation( void );
  friend void reanimate_vpic_simulation( vpic_simulation * vpic );

  ////////////////////////////////////////////////////////////
  // User input deck provided functions (see deck_wrapper.cxx)

  void user_initialization( int argc, char **argv );
  void user_particle_injection(void);
  void user_current_injection(void);
  void user_field_injection(void);
  void user_diagnostics(void);
  void user_particle_collisions(void);

  void KOKKOS_COPY_FIELD_MEM_TO_DEVICE(field_array_t* field_array)
  {
      int n_fields = field_array->g->nv;
      auto& k_field = field_array->k_f_h;
      auto& k_field_edge = field_array->k_fe_h;
      Kokkos::parallel_for("copy field to device", host_execution_policy(0, n_fields - 1) , KOKKOS_LAMBDA (int i) {
              k_field(i, field_var::ex) = field_array->f[i].ex;
              k_field(i, field_var::ey) = field_array->f[i].ey;
              k_field(i, field_var::ez) = field_array->f[i].ez;
              k_field(i, field_var::div_e_err) = field_array->f[i].div_e_err;

              k_field(i, field_var::cbx) = field_array->f[i].cbx;
              k_field(i, field_var::cby) = field_array->f[i].cby;
              k_field(i, field_var::cbz) = field_array->f[i].cbz;
              k_field(i, field_var::div_b_err) = field_array->f[i].div_b_err;

              k_field(i, field_var::tcax) = field_array->f[i].tcax;
              k_field(i, field_var::tcay) = field_array->f[i].tcay;
              k_field(i, field_var::tcaz) = field_array->f[i].tcaz;
              k_field(i, field_var::rhob) = field_array->f[i].rhob;

              k_field(i, field_var::jfx) = field_array->f[i].jfx;
              k_field(i, field_var::jfy) = field_array->f[i].jfy;
              k_field(i, field_var::jfz) = field_array->f[i].jfz;
              k_field(i, field_var::rhof) = field_array->f[i].rhof;

              k_field_edge(i, field_edge_var::ematx) = field_array->f[i].ematx;
              k_field_edge(i, field_edge_var::ematy) = field_array->f[i].ematy;
              k_field_edge(i, field_edge_var::ematz) = field_array->f[i].ematz;
              k_field_edge(i, field_edge_var::nmat) = field_array->f[i].nmat;

              k_field_edge(i, field_edge_var::fmatx) = field_array->f[i].fmatx;
              k_field_edge(i, field_edge_var::fmaty) = field_array->f[i].fmaty;
              k_field_edge(i, field_edge_var::fmatz) = field_array->f[i].fmatz;
              k_field_edge(i, field_edge_var::cmat) = field_array->f[i].cmat;
      });
      Kokkos::deep_copy(field_array->k_f_d, field_array->k_f_h);
      Kokkos::deep_copy(field_array->k_fe_d, field_array->k_fe_h);
  }

  void KOKKOS_COPY_FIELD_MEM_TO_HOST(field_array_t* field_array)
  {
      field_copy_last = step(); // track when we last moved this
      Kokkos::deep_copy(field_array->k_f_h, field_array->k_f_d);
      Kokkos::deep_copy(field_array->k_fe_h, field_array->k_fe_d);

      auto& k_field = field_array->k_f_h;
      auto& k_field_edge = field_array->k_fe_h;

      int n_fields = field_array->g->nv;

      Kokkos::parallel_for("copy field to host", host_execution_policy(0, n_fields - 1) , KOKKOS_LAMBDA (int i) {
              field_array->f[i].ex = k_field(i, field_var::ex);
              field_array->f[i].ey = k_field(i, field_var::ey);
              field_array->f[i].ez = k_field(i, field_var::ez);
              field_array->f[i].div_e_err = k_field(i, field_var::div_e_err);

              field_array->f[i].cbx = k_field(i, field_var::cbx);
              field_array->f[i].cby = k_field(i, field_var::cby);
              field_array->f[i].cbz = k_field(i, field_var::cbz);
              field_array->f[i].div_b_err = k_field(i, field_var::div_b_err);

              field_array->f[i].tcax = k_field(i, field_var::tcax);
              field_array->f[i].tcay = k_field(i, field_var::tcay);
              field_array->f[i].tcaz = k_field(i, field_var::tcaz);
              field_array->f[i].rhob = k_field(i, field_var::rhob);

              field_array->f[i].jfx = k_field(i, field_var::jfx);
              field_array->f[i].jfy = k_field(i, field_var::jfy);
              field_array->f[i].jfz = k_field(i, field_var::jfz);
              field_array->f[i].rhof = k_field(i, field_var::rhof);

              field_array->f[i].ematx = k_field_edge(i, field_edge_var::ematx);
              field_array->f[i].ematy = k_field_edge(i, field_edge_var::ematy);
              field_array->f[i].ematz = k_field_edge(i, field_edge_var::ematz);
              field_array->f[i].nmat = k_field_edge(i, field_edge_var::nmat);

              field_array->f[i].fmatx = k_field_edge(i, field_edge_var::fmatx);
              field_array->f[i].fmaty = k_field_edge(i, field_edge_var::fmaty);
              field_array->f[i].fmatz = k_field_edge(i, field_edge_var::fmatz);
              field_array->f[i].cmat = k_field_edge(i, field_edge_var::cmat);
      });
  }

  /**
   * @brief Copy all available particle memory from host to device, for a given
   * species list
   *
   * @param sp the species_t to copy
   */
  void KOKKOS_COPY_PARTICLE_MEM_TO_DEVICE_SP(species_t* sp)
  {
      auto n_particles = sp->np;
      auto max_pmovers = sp->max_nm;

      auto& k_particles_h = sp->k_p_h;
      auto& k_particles_i_h = sp->k_p_i_h;
      auto& k_particle_movers_h = sp->k_pm_h;
      auto& k_particle_movers_i_h = sp->k_pm_i_h;
      auto& k_nm_h = sp->k_nm_h;
      k_nm_h(0) = sp->nm;

      Kokkos::parallel_for("copy particles to device", host_execution_policy(0, n_particles) , KOKKOS_LAMBDA (int i) {
              k_particles_h(i, particle_var::dx) = sp->p[i].dx;
              k_particles_h(i, particle_var::dy) = sp->p[i].dy;
              k_particles_h(i, particle_var::dz) = sp->p[i].dz;
              k_particles_h(i, particle_var::ux) = sp->p[i].ux;
              k_particles_h(i, particle_var::uy) = sp->p[i].uy;
              k_particles_h(i, particle_var::uz) = sp->p[i].uz;
              k_particles_h(i, particle_var::w)  = sp->p[i].w;
              k_particles_i_h(i) = sp->p[i].i;
              });

      Kokkos::parallel_for("copy movers to device", host_execution_policy(0, max_pmovers) , KOKKOS_LAMBDA (int i) {
              k_particle_movers_h(i, particle_mover_var::dispx) = sp->pm[i].dispx;
              k_particle_movers_h(i, particle_mover_var::dispy) = sp->pm[i].dispy;
              k_particle_movers_h(i, particle_mover_var::dispz) = sp->pm[i].dispz;
              k_particle_movers_i_h(i) = sp->pm[i].i;
              });
      Kokkos::deep_copy(sp->k_p_d, sp->k_p_h);
      Kokkos::deep_copy(sp->k_p_i_d, sp->k_p_i_h);
      Kokkos::deep_copy(sp->k_pm_d, sp->k_pm_h);
      Kokkos::deep_copy(sp->k_pm_i_d, sp->k_pm_i_h);
      Kokkos::deep_copy(sp->k_nm_d, sp->k_nm_h);
  }

  /**
   * @brief Copy all available particle memory from host to device, for a given
   * list of species
   *
   * @param sp the species list to copy
   */
  void KOKKOS_COPY_PARTICLE_MEM_TO_DEVICE(species_t* species_list)
  {
      auto* sp = species_list;
      LIST_FOR_EACH( sp, species_list ) {
          KOKKOS_COPY_PARTICLE_MEM_TO_DEVICE_SP(sp);
      }
  }


  /**
   * @brief Copy all available particle memory from device to host, for a given species
   *
   * @param sp the species_t to copy
   */
  void KOKKOS_COPY_PARTICLE_MEM_TO_HOST_SP(species_t* sp)
  {
      Kokkos::deep_copy(sp->k_p_h, sp->k_p_d);
      Kokkos::deep_copy(sp->k_p_i_h, sp->k_p_i_d);
      Kokkos::deep_copy(sp->k_pm_h, sp->k_pm_d);
      Kokkos::deep_copy(sp->k_pm_i_h, sp->k_pm_i_d);
      Kokkos::deep_copy(sp->k_nm_h, sp->k_nm_d);

      auto n_particles = sp->np;
      auto max_pmovers = sp->max_nm;

      auto& k_particles_h = sp->k_p_h;
      auto& k_particles_i_h = sp->k_p_i_h;

      auto& k_particle_movers_h = sp->k_pm_h;
      auto& k_particle_movers_i_h = sp->k_pm_i_h;

      auto& k_nm_h = sp->k_nm_h;

      sp->nm = k_nm_h(0);

      Kokkos::parallel_for("copy particles to host", host_execution_policy(0, n_particles) , KOKKOS_LAMBDA (int i) {
              sp->p[i].dx = k_particles_h(i, particle_var::dx);
              sp->p[i].dy = k_particles_h(i, particle_var::dy);
              sp->p[i].dz = k_particles_h(i, particle_var::dz);
              sp->p[i].ux = k_particles_h(i, particle_var::ux);
              sp->p[i].uy = k_particles_h(i, particle_var::uy);
              sp->p[i].uz = k_particles_h(i, particle_var::uz);
              sp->p[i].w  = k_particles_h(i, particle_var::w);
              sp->p[i].i  = k_particles_i_h(i);
              });

      Kokkos::parallel_for("copy movers to host", host_execution_policy(0, max_pmovers) , KOKKOS_LAMBDA (int i) {
              sp->pm[i].dispx = k_particle_movers_h(i, particle_mover_var::dispx);
              sp->pm[i].dispy = k_particle_movers_h(i, particle_mover_var::dispy);
              sp->pm[i].dispz = k_particle_movers_h(i, particle_mover_var::dispz);
              sp->pm[i].i     = k_particle_movers_i_h(i);
              });
  }

  /**
   * @brief Copy all available particle memory from device to host, for a given species
   *
   * @param species_list The list of species to copy
   */
  void KOKKOS_COPY_PARTICLE_MEM_TO_HOST(species_t* species_list)
  {
      particle_copy_last = step();
      auto* sp = species_list;
      LIST_FOR_EACH( sp, species_list ) {
          KOKKOS_COPY_PARTICLE_MEM_TO_HOST_SP(sp);
      }
  }

  void KOKKOS_COPY_INTERPOLATOR_MEM_TO_DEVICE(interpolator_array_t* interpolator_array)
  {
      auto nv = interpolator_array->g->nv;

      auto& k_interpolator_h = interpolator_array->k_i_h;
      Kokkos::parallel_for("Copy interpolators to device", host_execution_policy(0, nv) , KOKKOS_LAMBDA (int i) {
              k_interpolator_h(i, interpolator_var::ex)       = interpolator_array->i[i].ex;
              k_interpolator_h(i, interpolator_var::ey)       = interpolator_array->i[i].ey;
              k_interpolator_h(i, interpolator_var::ez)       = interpolator_array->i[i].ez;
              k_interpolator_h(i, interpolator_var::dexdy)    = interpolator_array->i[i].dexdy;
              k_interpolator_h(i, interpolator_var::dexdz)    = interpolator_array->i[i].dexdz;
              k_interpolator_h(i, interpolator_var::d2exdydz) = interpolator_array->i[i].d2exdydz;
              k_interpolator_h(i, interpolator_var::deydz)    = interpolator_array->i[i].deydz;
              k_interpolator_h(i, interpolator_var::deydx)    = interpolator_array->i[i].deydx;
              k_interpolator_h(i, interpolator_var::d2eydzdx) = interpolator_array->i[i].d2eydzdx;
              k_interpolator_h(i, interpolator_var::dezdx)    = interpolator_array->i[i].dezdx;
              k_interpolator_h(i, interpolator_var::dezdy)    = interpolator_array->i[i].dezdy;
              k_interpolator_h(i, interpolator_var::d2ezdxdy) = interpolator_array->i[i].d2ezdxdy;
              k_interpolator_h(i, interpolator_var::cbx)      = interpolator_array->i[i].cbx;
              k_interpolator_h(i, interpolator_var::cby)      = interpolator_array->i[i].cby;
              k_interpolator_h(i, interpolator_var::cbz)      = interpolator_array->i[i].cbz;
              k_interpolator_h(i, interpolator_var::dcbxdx)   = interpolator_array->i[i].dcbxdx;
              k_interpolator_h(i, interpolator_var::dcbydy)   = interpolator_array->i[i].dcbydy;
              k_interpolator_h(i, interpolator_var::dcbzdz)   = interpolator_array->i[i].dcbzdz;
              });
      Kokkos::deep_copy(interpolator_array->k_i_d, interpolator_array->k_i_h);
  }

  void KOKKOS_COPY_INTERPOLATOR_MEM_TO_HOST(interpolator_array_t* interpolator_array)
  {

      Kokkos::deep_copy(interpolator_array->k_i_h, interpolator_array->k_i_d);

      auto nv = interpolator_array->g->nv;;
      auto& k_interpolator_h = interpolator_array->k_i_h;

      Kokkos::parallel_for("Copy interpolators to device", host_execution_policy(0, nv) , KOKKOS_LAMBDA (int i) {
              interpolator_array->i[i].ex       = k_interpolator_h(i, interpolator_var::ex);
              interpolator_array->i[i].ey       = k_interpolator_h(i, interpolator_var::ey);
              interpolator_array->i[i].ez       = k_interpolator_h(i, interpolator_var::ez);
              interpolator_array->i[i].dexdy    = k_interpolator_h(i, interpolator_var::dexdy);
              interpolator_array->i[i].dexdz    = k_interpolator_h(i, interpolator_var::dexdz);
              interpolator_array->i[i].d2exdydz = k_interpolator_h(i, interpolator_var::d2exdydz);
              interpolator_array->i[i].deydz    = k_interpolator_h(i, interpolator_var::deydz);
              interpolator_array->i[i].deydx    = k_interpolator_h(i, interpolator_var::deydx);
              interpolator_array->i[i].d2eydzdx = k_interpolator_h(i, interpolator_var::d2eydzdx);
              interpolator_array->i[i].dezdx    = k_interpolator_h(i, interpolator_var::dezdx);
              interpolator_array->i[i].dezdy    = k_interpolator_h(i, interpolator_var::dezdy);
              interpolator_array->i[i].d2ezdxdy = k_interpolator_h(i, interpolator_var::d2ezdxdy);
              interpolator_array->i[i].cbx      = k_interpolator_h(i, interpolator_var::cbx);
              interpolator_array->i[i].cby      = k_interpolator_h(i, interpolator_var::cby);
              interpolator_array->i[i].cbz      = k_interpolator_h(i, interpolator_var::cbz);
              interpolator_array->i[i].dcbxdx   = k_interpolator_h(i, interpolator_var::dcbxdx);
              interpolator_array->i[i].dcbydy   = k_interpolator_h(i, interpolator_var::dcbydy);
              interpolator_array->i[i].dcbzdz   = k_interpolator_h(i, interpolator_var::dcbzdz);
              });
  }

  void KOKKOS_COPY_ACCUMULATOR_MEM_TO_DEVICE(accumulator_array_t* accumulator_array)
  {
      auto na = accumulator_array->na;

      auto& k_accumulators_h = accumulator_array->k_a_h;
      Kokkos::parallel_for("copy accumulator to device", KOKKOS_TEAM_POLICY_HOST
              (na, Kokkos::AUTO),
              KOKKOS_LAMBDA
              (const KOKKOS_TEAM_POLICY_HOST::member_type &team_member) {
              const unsigned int i = team_member.league_rank();
              /* TODO: Do we really need a 2d loop here*/
              Kokkos::parallel_for(Kokkos::TeamThreadRange(team_member, ACCUMULATOR_ARRAY_LENGTH), [=] (int j) {
                      k_accumulators_h(i, accumulator_var::jx, j)       = accumulator_array->a[i].jx[j];
                      k_accumulators_h(i, accumulator_var::jy, j)       = accumulator_array->a[i].jy[j];
                      k_accumulators_h(i, accumulator_var::jz, j)       = accumulator_array->a[i].jz[j];
                      });
              });
      Kokkos::deep_copy(accumulator_array->k_a_d, accumulator_array->k_a_h);
  }

  void KOKKOS_COPY_ACCUMULATOR_MEM_TO_HOST(accumulator_array_t* accumulator_array)
  {
      auto& na = accumulator_array->na;
      auto& k_accumulators_h = accumulator_array->k_a_h;

      Kokkos::deep_copy(accumulator_array->k_a_h, accumulator_array->k_a_d);
      Kokkos::parallel_for("copy accumulator to host", KOKKOS_TEAM_POLICY_HOST
              (na, Kokkos::AUTO),
              KOKKOS_LAMBDA
              (const KOKKOS_TEAM_POLICY_HOST::member_type &team_member) {
              const unsigned int i = team_member.league_rank();

              Kokkos::parallel_for(Kokkos::TeamThreadRange(team_member, ACCUMULATOR_ARRAY_LENGTH), [=] (int j) {
                      accumulator_array->a[i].jx[j] = k_accumulators_h(i, accumulator_var::jx, j);
                      accumulator_array->a[i].jy[j] = k_accumulators_h(i, accumulator_var::jy, j);
                      accumulator_array->a[i].jz[j] = k_accumulators_h(i, accumulator_var::jz, j);
                      });
              });
  }

};

#endif // vpic_h

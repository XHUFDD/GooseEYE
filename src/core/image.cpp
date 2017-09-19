
#include "image.h"

template class cppmat::matrix<int>;
template class cppmat::matrix<double>;

namespace GooseEYE {
namespace Image {

// abbreviate data types to enhance readability below
template <class T> using Mat = cppmat::matrix<T>;
using MatD = cppmat::matrix<double>;
using MatI = cppmat::matrix<int>;
using VecS = std::vector<size_t>;
using VecI = std::vector<int>;
using str  = std::string;

// =================================================================================================
// pixel/voxel path between two points "xa" and "xb"
// =================================================================================================

MatI path (
  VecI xa, VecI xb, str mode )
{
  int ndim = static_cast<int>(xa.size());

  if ( xa.size()!=xb.size() )
    throw std::length_error("'xa' and 'xb' must have the same dimension");

  if ( ndim<1 || ndim>3 )
    throw std::length_error("Only allowed in 1, 2, or 3 dimensions");

  std::transform(mode.begin(), mode.end(), mode.begin(), ::tolower);

  VecI ret;

  if ( mode=="bresenham" )
  {
    // see http://www.luberth.com/plotter/line3d.c.txt.html
    int a[3],s[3],x[3],d[3],in[2],j,i,iin,nnz=0;

    // set defaults
    for ( i=0; i<3; i++ ) {
      a[i] = 1;
      s[i] = 0;
      x[i] = 0;
      d[i] = 0;
    }

    // calculate:
    // absolute distance, set to "1" if the distance is zero
    // sign of the distance (can be -1/+1 or 0)
    // current position (temporary value)
    for ( i=0; i<ndim; i++) {
      a[i] = std::abs(xb[i]-xa[i]) << 1;
      s[i] = SIGN    (xb[i]-xa[i]);
      x[i] = xa[i];
    }

    // determine which direction is dominant
    for ( j=0; j<3; j++ ) {
      // set the remaining directions
      iin = 0;
      for ( i=0; i<3; i++ ) {
        if ( i!=j ) {
          in[iin] = i;
          iin    += 1;
        }
      }
      // determine if the current direction is dominant
      if ( a[j] >= std::max(a[in[0]],a[in[1]]) )
        break;
    }

    // set increment in non-dominant directions
    for ( i=0; i<2; i++)
      d[in[i]] = a[in[i]]-(a[j]>>1);
    // loop until "x" coincides with "xb"
    while ( 1 ) {
      // add current voxel to path
      for ( i=0; i<ndim; i++)
        ret.push_back(x[i]);
      // update voxel counter
      nnz += 1;
      // check convergence
      if ( x[j]==xb[j] ) {
        MatI retmat({(size_t)nnz,(size_t)ndim},&ret[0]);
        return retmat;
      }
      // check increment in other directions
      for ( i=0; i<2; i++ ) {
        if ( d[in[i]]>=0 ) {
          x[in[i]] += s[in[i]];
          d[in[i]] -= a[j];
        }
      }
      // increment
      x[j] += s[j];
      for ( i=0; i<2; i++ )
        d[in[i]] += a[in[i]];
    }
  }

  if ( mode == "actual" || mode == "full" )
  {
    // position, slope, (length to) next intersection
    double x[3],v[3],t[3],next[3],sign[3];
    int isign[3];
    // active dimensions (for in-plane paths dimension have to be skipped
    // to avoid dividing by zero)
    int in[3],iin,nin;
    // path of the current voxel
    int cindex[3];
    int nnz = 1;
    // counters
    int i,imin,n;

    // set the direction coefficient in all dimensions; if it is zero this
    // dimension is excluded from further analysis (i.e. in-plane growth)
    nin = 0;
    for ( i=0 ; i<ndim ; i++ ) {
      // set origin, store to output array; initiate the position
      cindex[i] = xa[i];
      ret.push_back(cindex[i]);
      // initiate position, set slope
      x[i] = (double)(xa[i]);
      v[i] = (double)(xb[i]-xa[i]);
      // non-zero slope: calculate the sign and the next intersection
      // with a voxel's edges, and update the list to include this dimension
      // in the further analysis
      if ( v[i] ) {
        sign[i]  = v[i]/fabs(v[i]);
        isign[i] = (int)sign[i];
        next[i]  = sign[i]*0.5;
        in[nin]  = i;
        nin++;
      }
    }

    // starting from "xa" loop to "xb"
    while ( 1 ) {

      // find translation coefficient "t" for each next intersection
      // (only include dimensions with non-zero slope)
      for ( iin=0 ; iin<nin ; iin++ ) {
        i      = in[iin];
        t[iin] = (next[i]-x[i])/v[i];
      }
      // find the minimum "t": the intersection which is closet along the line
      // from the current position -> proceed in this direction
      imin = 0;
      for ( iin=1 ; iin<nin ; iin++ )
        if ( t[iin]<t[imin] )
          imin = iin;

      // update path: proceed in dimension of minimum "t"
      // note: if dimensions have equal "t" -> proceed in each dimension
      for ( iin=0 ; iin<nin ; iin++ ) {
        if ( fabs(t[iin]-t[imin])<1.e-6 ) {
          cindex[in[iin]] += isign[in[iin]];
          next[in[iin]]   += sign[in[iin]];
          // store all the face crossings ("mode")
          if ( mode=="full" ) {
            for ( i=0 ; i<ndim ; i++ )
              ret.push_back(cindex[i]);
            nnz++;
          }
        }
      }
      // store only the next voxel ("actual")
      if ( mode=="actual" ) {
        for ( i=0 ; i<ndim ; i++ )
          ret.push_back(cindex[i]);
        nnz++;
      }
      // update position, and current path
      for ( i=0 ; i<ndim ; i++ )
        x[i] = xa[i]+v[i]*t[imin];

      // check convergence: stop when "xb" is reached
      n = 0;
      for ( i=0 ; i<ndim; i++ )
        if ( cindex[i]==xb[i] )
          n++;
      if ( n==ndim )
        break;

    }

    MatI retmat({(size_t)nnz,(size_t)ndim},&ret[0]);
    return retmat;
  }

  throw std::out_of_range("Unknown 'mode'");
}

// =================================================================================================
// list of end-points of ROI-stamp used in path-based correlations
// =================================================================================================

MatI stamp_points ( VecS N )
{
  if ( N.size()<1 || N.size()>3 )
    throw std::length_error("'shape' must be 1-, 2-, or 3-D");

  for ( auto &i : N )
    if ( i%2 == 0 )
      throw std::length_error("'shape' must be odd shaped");

  int n,i,j,H,I,J,dH,dI,dJ;
  int idx = 0;
  int nd  = N.size();

  std::tie( H, I, J) = unpack3d(N,1);
  std::tie(dH,dI,dJ) = unpack3d(midpoint(N),0);

  // number of points
  if ( nd==1 ) n =           2;
  if ( nd==2 ) n =           2*H+2*POS(I-2);
  if ( nd==3 ) n = POS(J-2)*(2*H+2*POS(I-2))+2*H*I;

  // allocate
  MatI ret({(size_t)n,(size_t)nd});

  // 1-D
  // ---

  if ( nd==1 )
  {
    ret(idx,0) = -dH; idx++;
    ret(idx,0) = +dH; idx++;
    return ret;
  }

  // 2-D
  // ---

  if ( nd==2 )
  {
    for ( i=0 ; i<H   ; i++ ) {
      ret(idx,0) = i-dH; ret(idx,1) =  -dI; idx++;
      ret(idx,0) = i-dH; ret(idx,1) =  +dI; idx++;
    }
    for ( i=1 ; i<I-1 ; i++ ) {
      ret(idx,0) =  -dH; ret(idx,1) = i-dI; idx++;
      ret(idx,0) =  +dH; ret(idx,1) = i-dI; idx++;
    }
    return ret;
  }

  // 3-D
  // ---

  for ( j=1 ; j<J-1 ; j++ ) {
    for ( i=0 ; i<H   ; i++ ) {
      ret(idx,0) = i-dH; ret(idx,1) =  -dI; ret(idx,2) = j-dJ; idx++;
      ret(idx,0) = i-dH; ret(idx,1) =  +dI; ret(idx,2) = j-dJ; idx++;
    }
    for ( i=1 ; i<I-1 ; i++ ) {
      ret(idx,0) =  -dH; ret(idx,1) = i-dI; ret(idx,2) = j-dJ; idx++;
      ret(idx,0) =  +dH; ret(idx,1) = i-dI; ret(idx,2) = j-dJ; idx++;
    }
  }
  for ( i=0 ; i<H   ; i++ ) {
    for ( j=0 ; j<I   ; j++ ) {
      ret(idx,0) = i-dH; ret(idx,1) = j-dI; ret(idx,2) =  -dJ; idx++;
      ret(idx,0) = i-dH; ret(idx,1) = j-dI; ret(idx,2) =  +dJ; idx++;
    }
  }
  return ret;
}

// =================================================================================================
// return vector as "(h,i,j)", using a default "value" if "shape.size()<3"
// =================================================================================================

std::tuple<int,int,int> unpack3d ( VecS shape, int value )
{
  int h,i,j;

  h = value;
  i = value;
  j = value;

  if ( shape.size()>=1 ) h = (int)shape[0];
  if ( shape.size()>=2 ) i = (int)shape[1];
  if ( shape.size()>=3 ) j = (int)shape[2];

  return std::make_tuple(h,i,j);
}

// =================================================================================================
// compute midpoint from "shape"-vector
// =================================================================================================

VecS midpoint ( VecS shape )
{
  VecS ret(shape.size());

  for ( auto i : shape )
    if ( !(i%2) )
      throw std::domain_error("Only allowed for odd-shaped matrices");

  for ( size_t i=0 ; i<shape.size() ; i++ )
    ret[i] = (shape[i]-1)/2;

  return ret;

}

// =================================================================================================
// pad "pad_shape" entries on each side of "src" with a certain "value"
// =================================================================================================

template <class T>
Mat<T> pad ( Mat<T> &src, VecS pad_shape, T value )
{
  VecS shape = src.shape();

  for ( size_t i=0 ; i<pad_shape.size() ; i++ )
    shape[i] += 2*pad_shape[i];

  // allocate to supplied value
  Mat<T> ret(shape);
  ret.ones();
  ret *= value;

  int h,i,j,H,I,J,dH,dI,dJ;

  std::tie( H, I, J) = unpack3d(src.shape(),1);
  std::tie(dH,dI,dJ) = unpack3d(pad_shape  ,0);

  src.atleast_3d();
  ret.atleast_3d();

  // copy input matrix to inner region
  for ( h=0 ; h<H ; h++ )
    for ( i=0 ; i<I ; i++ )
      for ( j=0 ; j<J ; j++ )
        ret(h+dH,i+dI,j+dJ) = src(h,i,j);

  return ret;
}

template MatI pad<int>    (MatI &, VecS, int   );
template MatD pad<double> (MatD &, VecS, double);

// =================================================================================================
// dilate image
// =================================================================================================

MatI dilate ( MatI &src, MatI &kern,
  VecI &iterations, bool periodic )
{
  if ( (int)iterations.size()!=src.max()+1 )
    throw std::length_error("Iteration must be specified for each label");

  int h,i,j,dh,di,dj,H,I,J,dH,dI,dJ,nlab,ilab,iter;
  int max_iter = 0;

  MatI lab = src;

  std::tie( H, I, J) = unpack3d(src.shape(),1);
  std::tie(dH,dI,dJ) = unpack3d(midpoint(kern.shape()),0);

  src .atleast_3d();
  kern.atleast_3d();
  lab .atleast_3d();

  // number of labels
  nlab = lab.max();

  // find maximum number of iterations
  max_iter = *std::max_element(iterations.begin(),iterations.end());

  // loop over iterations
  for ( iter=0 ; iter<max_iter ; iter++ ) {

    // loop over all voxel
    for ( h=0 ; h<H ; h++ ) {
      for ( i=0 ; i<I ; i++ ) {
        for ( j=0 ; j<J ; j++ ) {
          // label over the current voxel
          ilab = lab(h,i,j);
          // proceed:
          // - for non-zero label
          // - if the number of iterations for this label has not been exceeded
          if ( ilab>0 && iterations[ilab]>iter ) {
            // loop over the kernel
            for ( dh=-dH ; dh<=dH ; dh++ ) {
              for ( di=-dI ; di<=dI ; di++ ) {
                for ( dj=-dJ ; dj<=dJ ; dj++ ) {
                  // check to dilate for non-zero kernel value
                  if ( kern(dh+dH,di+dI,dj+dJ) && !(dh==0 && di==0 && dj==0) ) {
                    if ( periodic ) {
                      if ( !lab(P(h+dh,H),P(i+di,I),P(j+dj,J)) )
                        lab(P(h+dh,H),P(i+di,I),P(j+dj,J)) = -1*ilab;
                    }
                    else {
                      if ( BND(h+dh,H) && BND(i+di,I) && BND(j+dj,J) )
                        if ( !lab(h+dh,i+di,j+dj) )
                          lab(h+dh,i+di,j+dj) = -1*ilab;
                    }
                  }
                }
              }
            }
          }
        }
      }
    }
    // accept all new labels (which were stored as negative)
    for ( auto &i : lab )
      i = std::abs(i);
  }

  return lab;
}

// =================================================================================================

MatI dilate ( MatI &src, int iterations, bool periodic )
{
  MatI kern = kernel(src.ndim());

  VecI iter(src.max()+1);
  for ( auto &i : iter )
    i = iterations;

  return dilate(src,kern,iter,periodic);
}

// =================================================================================================

MatI dilate ( MatI &src, MatI &kernel, int iterations,
  bool periodic )
{
  VecI iter(src.max()+1);
  for ( auto &i : iter )
    i = iterations;

  return dilate(src,kernel,iter,periodic);
}

// =================================================================================================

MatI dilate ( MatI &src, VecI &iterations,
  bool periodic )
{
  MatI kern = kernel(src.ndim());
  return dilate(src,kern,iterations,periodic);
}

// =================================================================================================
// create a dummy image with circles at position "row","col" with radius "r"
// =================================================================================================

MatI dummy_circles ( VecS shape, VecI &row,
  VecI &col, VecI &r, bool periodic )
{
  if ( row.size()!=col.size() || row.size()!=r.size() )
    throw std::length_error("'row', 'col', and 'r' are inconsistent");
  if ( shape.size()!=2 )
    throw std::length_error("Only allowed in 2 dimensions");

  int    di,dj,I,J;
  size_t i;
  MatI ret(shape); ret.zeros();

  I = shape[0];
  J = shape[1];

  for ( i=0 ; i<row.size() ; i++ )
    for ( di=-r[i] ; di<=r[i] ; di++ )
      for ( dj=-r[i] ; dj<=r[i] ; dj++ )
        if ( periodic || ( BND(row[i]+di,I) && BND(col[i]+dj,J) ) )
          if ( (int)(ceil(pow((double)(pow(di,2)+pow(dj,2)),0.5))) < r[i] )
            ret(P(row[i]+di,I),P(col[i]+dj,J)) = 1;

  return ret;
}

// =================================================================================================
// create a dummy image with a default number of circles
// at random positions and random radii
// =================================================================================================

MatI dummy_circles ( VecS shape, bool periodic )
{
  if ( shape.size()!=2 )
    throw std::length_error("Only allowed in 2 dimensions");

  std::srand(std::time(0));

  const double PI = std::atan(1.0)*4;

  // set default: number of circles in both directions and (constant) radius
  int N = (int)(.05*(double)shape[0]);
  int M = (int)(.05*(double)shape[1]);
  int R = (int)(pow((.3*(double)(shape[0]*shape[1]))/(PI*(double)(N*M)),.5));

  VecI row(N*M),col(N*M),r(N*M);

  // define regular grid of circles
  for ( int i=0 ; i<N ; i++ ) {
    for ( int j=0 ; j<M ; j++ ) {
      row[i*M+j] = (int)((double)i*(double)shape[0]/(double)N);
      col[i*M+j] = (int)((double)j*(double)shape[1]/(double)M);
      r  [i*M+j] = R;
    }
  }

  // distance between circles
  int dN = (int)(.5*(double)shape[0]/(double)N);
  int dM = (int)(.5*(double)shape[1]/(double)M);

  // randomly perturb circles (move in any direction, enlarge/shrink)
  for ( int i=0 ; i<N*M ; i++ ) {
    row[i] += (int)(((double)(std::rand()%2)-.5)*2.)*std::rand()%dN;
    col[i] += (int)(((double)(std::rand()%2)-.5)*2.)*std::rand()%dM;
    r  [i]  = (int)(((double)(std::rand()%100)/100.*2.+.1)*(double)(r[i]));
  }

  // convert to image
  return dummy_circles(shape,row,col,r,periodic);
}

// =================================================================================================
// create dummy image with default shape
// =================================================================================================

MatI dummy_circles ( bool periodic )
{
  return dummy_circles({100,100},periodic);
}

// =================================================================================================
// define kernel
// =================================================================================================

MatI kernel ( int ndim , str mode )
{
  std::transform(mode.begin(), mode.end(), mode.begin(), ::tolower);

  if ( mode=="default" )
  {
    VecS shape(ndim);
    for ( int i=0 ; i<ndim ; i++ )
      shape[i] = 3;

    MatI kern(shape); kern.zeros();

    if      ( ndim==1 ) {
      kern(0) = 1; kern(1) = 1; kern(2) = 1;
    }
    else if ( ndim==2 ) {
      kern(1,0) = 1; kern(1,1) = 1; kern(1,2) = 1;
      kern(0,1) = 1; kern(2,1) = 1;
    }
    else if ( ndim==3 ) {
      kern(1,1,0) = 1; kern(1,1,1) = 1; kern(1,1,2) = 1;
      kern(1,0,1) = 1; kern(1,2,1) = 1;
      kern(0,1,1) = 1; kern(2,1,1) = 1;
    }
    else
      throw std::length_error("Only defined in 1-, 2-, or 3-D");

    return kern;
  }

  throw std::length_error("Unknown mode");
}

// =================================================================================================
// determine clusters in image
// =================================================================================================

void _link ( VecI &linked, int a, int b )
{

  if ( a==b )
    return;

  int i,c;

  if ( a>b ) {
    c = a;
    a = b;
    b = c;
  }

  // both unlinked
  // -------------

  if ( linked[a]==a && linked[b]==b ) {
    linked[a] = b;
    linked[b] = a;
    return;
  }

  // a linked / b unlinked
  // ---------------------

  if ( linked[a]!=a && linked[b]==b ) {
    linked[b] = linked[a];
    linked[a] = b;
    return;
  }

  // a unlinked / b linked
  // ---------------------

  if ( linked[a]==a && linked[b]!=b ) {
    linked[a] = linked[b];
    linked[b] = a;
    return;
  }

  // both linked -> to each other
  // ----------------------------

  i = a;
  while ( 1 ) {
    if ( linked[i]==b ) { return; }
    i = linked[i];
    if ( i==a ) { break; }
  }

  // both linked -> to different groups
  // ----------------------------------

  c = linked[a];
  linked[a] = b;

  i = a;
  while ( 1 ) {
    i = linked[i];
    if ( linked[i]==b ) { break; }
  }
  linked[i] = c;

  return;

}

// -------------------------------------------------------------------------------------------------

std::tuple<MatI,MatI> clusters (
  MatI &f, MatI &kern, int min_size, bool periodic )
{
  int h,i,j,di,dj,dh,H,I,J,lH,lI,lJ,uH,uI,uJ,dI,dJ,dH,ii,jj,ilab,nlab;
  // cluster links (e.g. 4 coupled to 2: lnk[4]=2)
  VecI lnk(f.size());
  // saved clusters: 1=saved, 0=not-saved
  VecI inc(f.size());

  MatI l(f.shape()); l.zeros();
  MatI c(f.shape()); c.zeros();

  std::tie( H, I, J) = unpack3d(f.shape(),1);
  std::tie(dH,dI,dJ) = unpack3d(midpoint(kern.shape()),0);

  f   .atleast_3d();
  kern.atleast_3d();
  l   .atleast_3d();
  c   .atleast_3d();

  // --------------
  // basic labeling
  // --------------

  // current label
  ilab = 0;

  // initialize cluster-links (only coupled to self)
  for ( i=0 ; i<(int)f.size() ; i++ )
    lnk[i] = i;
  // initialize: no saved clusters, except background (inc[0]=1)
  inc[0] = 1;
  for ( size_t i=1 ; i<f.size() ; i++ )
    inc[i] = 0;

  // periodic: lower/upper bound of the kernel always == (shape[i]-1)/2
  if ( periodic ) {
    lH = -dH; uH = +dH;
    lI = -dI; uI = +dI;
    lJ = -dJ; uJ = +dJ;
  }

  // loop over voxels (in all directions)
  for ( h=0 ; h<H ; h++ ) {
    for ( i=0 ; i<I ; i++ ) {
      for ( j=0 ; j<J ; j++ ) {

        // only continue for non-zero voxels
        if ( f(h,i,j) ) {

          // set lower/upper bound of the kernel near edges
          // -> avoids reading out-of-bounds
          if ( !periodic ) {
            if ( h <    dH ) lH=0; else lH=-dH;
            if ( i <    dI ) lI=0; else lI=-dI;
            if ( j <    dJ ) lJ=0; else lJ=-dJ;
            if ( h >= H-dH ) uH=0; else uH=+dH;
            if ( i >= I-dI ) uJ=0; else uJ=+dI;
            if ( j >= J-dJ ) uI=0; else uI=+dJ;
          }

          // cluster not yet labeled: try to couple to labeled neighbors
          if ( l(h,i,j)==0 ) {
            for ( dh=lH ; dh<=uH ; dh++ ) {
              for ( di=lI ; di<=uJ ; di++ ) {
                for ( dj=lJ ; dj<=uI ; dj++ ) {
                  if ( kern(dh+dH,di+dI,dj+dJ) ) {
                    if ( l(P(h+dh,H),P(i+di,I),P(j+dj,J)) ) {
                      l(h,i,j) = l(P(h+dh,H),P(i+di,I),P(j+dj,J));
                      goto end;
                    }
          }}}}}
          end: ;

          // cluster not yet labeled: create new label
          if ( l(h,i,j)==0 ) {
            ilab++;
            l(h,i,j)  = ilab;
            inc[ilab] = 1;
          }

          // try to couple neighbors to current label
          // - not yet labeled -> label neighbor
          // - labeled         -> link labels
          for ( dh=lH ; dh<=uH ; dh++ ) {
            for ( di=lI ; di<=uJ ; di++ ) {
              for ( dj=lJ ; dj<=uI ; dj++ ) {
                if ( kern(dh+dH,di+dI,dj+dJ) ) {
                  if ( f(P(h+dh,H),P(i+di,I),P(j+dj,J)) ) {
                    if ( l(P(h+dh,H),P(i+di,I),P(j+dj,J))==0 )
                     l(P(h+dh,H),P(i+di,I),P(j+dj,J)) = l(h,i,j);
                    else
                     _link(lnk,l(h,i,j),l(P(h+dh,H),P(i+di,I),P(j+dj,J)));
          }}}}}

        }

      }
    }
  }

  // ---------------------------------------
  // renumber labels: all links to one label
  // ---------------------------------------

  nlab = 0;

  // loop over assigned labels -> loop over all coupled links
  // lnk[i] will contain the new label number
  for ( i=0 ; i<=ilab ; i++ ) {
    if ( inc[i] ) {
      ii = i;
      while ( 1 ) {
        jj      = lnk[ii];
        lnk[ii] = nlab;
        inc[ii] = 0;
        if ( jj==i ) { break; }
        ii      = jj;
      }
      nlab++;
    }
  }
  // apply renumbering
  for ( size_t i=0 ; i<f.size() ; i++ )
    l[i] = lnk[l[i]];

  // --------------------------
  // threshold for cluster size
  // --------------------------

  if ( min_size>0 ) {

    // find the size of all clusters
    for ( i=0 ; i<nlab ; i++ ) {
      lnk[i] = 0;    // now: size of the cluster with the label
      inc[i] = 0;    // now: included label (true/false)
    }
    for ( size_t i=0 ; i<l.size() ; i++ ) {
      lnk[l[i]]++;
      inc[l[i]] = 1;
    }
    // remove clusters with too small size
    for ( i=0 ; i<nlab ; i++ )
      if ( lnk[i]<min_size )
        inc[i] = 0;

    // remove clusters with too small size
    for ( size_t i=0 ; i<l.size() ; i++ )
      if ( !inc[l[i]] )
        l[i] = 0;

    // new numbering for the included labels
    // inc[i] will contain the new cluster numbers,
    // j-1 will be the number of clusters
    j = 0;
    for ( i=0 ; i<nlab ; i++ ) {
      if ( inc[i] ) {
        inc[i] = j;
        j++;
      }
    }
    nlab = j;

    // renumber the labels
    for ( size_t i=0 ; i<l.size() ; i++ )
      l[i] = inc[l[i]];

  }

  // cluster centers: not periodic
  // -----------------------------

  if ( !periodic )
  {
    // number of labels
    nlab = l.max()+1;

    // allocate matrix to contain the average position and total size:
    // [ [ h,i,j , size_feature ] , ... ]
    MatI x({(size_t)nlab,4}); x.zeros();

    // loop over the image to update the position and size of each label
    for ( h=0 ; h<H ; h++ ) {
      for ( i=0 ; i<I ; i++ ) {
        for ( j=0 ; j<J ; j++ ) {
          ilab = l(h,i,j);
          if ( ilab>0 ) {
            x(ilab,0) += h;
            x(ilab,1) += i;
            x(ilab,2) += j;
            x(ilab,3) += 1;
          }
        }
      }
    }

    // fill the centers of gravity
    for ( ilab=1 ; ilab<nlab ; ilab++ ) {
      if ( x(ilab,3)>0 ) {

        h = (int)round( (float)x(ilab,0) / (float)x(ilab,3) );
        i = (int)round( (float)x(ilab,1) / (float)x(ilab,3) );
        j = (int)round( (float)x(ilab,2) / (float)x(ilab,3) );

        if ( h <  0 ) { h = 0; }
        if ( i <  0 ) { i = 0; }
        if ( j <  0 ) { j = 0; }
        if ( h >= H ) { h = H; }
        if ( i >= I ) { i = I; }
        if ( j >= J ) { j = J; }

        c(h,i,j) = ilab;
      }
    }
  }

  // cluster centers: periodic
  // -------------------------

  if ( periodic )
  {
    int h,i,j,dh,di,dj,ilab,nlab,nlab_;

    // "l_": labels for the non-periodic version image "f"
    MatI l_,c_;
    std::tie(l_,c_) = clusters(f,kern,min_size,false);

    l_.atleast_3d();
    c_.atleast_3d();

    // delete clusters that are also not present in "l"
    for ( size_t i=0 ; i<f.size() ; i++ )
      if ( f[i] )
        if ( !l[i] )
          l_[i] = 0;

    // number of labels
    nlab  = l .max()+1;
    nlab_ = l_.max()+1;

    // matrix to contain the average position and total size:
    // [ [ h,i,j , size_feature ] , ... ]
    MatI  x({(size_t)nlab ,4});  x.zeros();
    // if dx(ilab,ix)==1 than N[ix] is subtracted from the position in averaging
    MatI dx({(size_t)nlab_,3}); dx.zeros();

    // label dependency: which labels in "l_" correspond to which labels in "l"
    VecI lnk(nlab_);

    for ( size_t i=0 ; i<l.size() ; i++ )
      lnk[l_[i]] = l[i];

    // check periodicity: to which sides the clusters are connected
    // ------------------------------------------------------------

    // i-j plane
    for ( i=0 ; i<I ; i++ )
      for ( j=0 ; j<J ; j++ )
        for ( dh=1 ; dh<=dH ; dh++ )
          for ( di=0 ; di<=dI ; di++ )
            for ( dj=0 ; dj<=dJ ; dj++ )
              if ( i+di<I && j+dj<J )
                if ( l_(H-1,i,j) )
                  if ( l(H-1,i,j)==l(P(H-1+dh,H),P(i+di,I),P(j+dj,J)) )
                    dx(l_(H-1,i,j),0) = 1;

    // h-j plane
    for ( h=0 ; h<H ; h++ )
      for ( j=0 ; j<J ; j++ )
        for ( dh=0 ; dh<=dH ; dh++ )
          for ( di=1 ; di<=dI ; di++ )
            for ( dj=0 ; dj<=dJ ; dj++ )
              if ( h+dh<H && j+dj<J )
                if ( l_(h,I-1,j) )
                  if ( l(h,I-1,j)==l(P(h+dh,H),P(I-1+di,I),P(j+dj,J)) )
                    dx(l_(h,I-1,j),1) = 1;

    // h-i plane
    for ( h=0 ; h<H ; h++ )
      for ( i=0 ; j<I ; j++ )
        for ( dh=0 ; dh<=dH ; dh++ )
          for ( di=0 ; di<=dI ; di++ )
            for ( dj=1 ; dj<=dJ ; dj++ )
              if ( h+dh<H && i+di<I )
                if ( l_(h,i,J-1) )
                  if ( l(h,i,J-1)==l(P(h+dh,H),P(i+di,I),P(J-1+dj,J)) )
                    dx(l_(h,i,J-1),2) = 1;

    // calculate centers
    // -----------------

    // loop over the image to update the position and size of each label
    for ( h=0 ; h<H ; h++ ) {
      for ( i=0 ; i<I ; i++ ) {
        for ( j=0 ; j<J ; j++ ) {
          ilab = l_(h,i,j);
          if ( ilab>0 ) {
            if ( dx(ilab,0) ) dh = -H; else dh = 0;
            if ( dx(ilab,1) ) di = -I; else di = 0;
            if ( dx(ilab,2) ) dj = -J; else dj = 0;

            x(lnk[ilab],0) += h+dh;
            x(lnk[ilab],1) += i+di;
            x(lnk[ilab],2) += j+dj;
            x(lnk[ilab],3) += 1;
          }
        }
      }
    }

    // fill the centers of gravity
    for ( ilab=1 ; ilab<nlab ; ilab++ ) {
      if ( x(ilab,3)>0 ) {

        h = (int)round( (float)x(ilab,0) / (float)x(ilab,3) );
        i = (int)round( (float)x(ilab,1) / (float)x(ilab,3) );
        j = (int)round( (float)x(ilab,2) / (float)x(ilab,3) );

        while ( h <  0 ) { h += H; }
        while ( i <  0 ) { i += I; }
        while ( j <  0 ) { j += J; }
        while ( h >= H ) { h -= H; }
        while ( i >= I ) { i -= I; }
        while ( j >= J ) { j -= J; }

        c(h,i,j) = ilab;
      }
    }
  }

  return std::make_tuple(l,c);
}

// -------------------------------------------------------------------------------------------------

std::tuple<MatI,MatI> clusters (
  MatI &f, int min_size, bool periodic )
{
  MatI kern = kernel(f.ndim());

  return clusters(f,kern,min_size,periodic);
}

// -------------------------------------------------------------------------------------------------

std::tuple<MatI,MatI> clusters (
  MatI &f, bool periodic )
{
  MatI kern = kernel(f.ndim());

  return clusters(f,kern,0,periodic);
}

// =================================================================================================
// mean
// =================================================================================================

template <class T>
std::tuple<double,double> mean ( Mat<T> &src )
{
  return std::make_tuple(src.mean(),static_cast<double>(src.size()));
}

// -------------------------------------------------------------------------------------------------

template <class T>
std::tuple<double,double> mean ( Mat<T> &src , MatI &mask )
{
  T      out = static_cast<T>(0);
  size_t n   = 0;

  for ( size_t i=0 ; i<src.size() ; i++ ) {
    if ( !mask[i] ) {
      out += src[i];
      n++;
    }
  }

  return std::make_tuple(static_cast<double>(out)/static_cast<double>(n),static_cast<double>(n));
}

// -------------------------------------------------------------------------------------------------

template std::tuple<double,double> mean<int   >(MatI &);
template std::tuple<double,double> mean<double>(MatD &);
template std::tuple<double,double> mean<int   >(MatI &, MatI &);
template std::tuple<double,double> mean<double>(MatD &, MatI &);

// =================================================================================================
// TODO include in header
// comparison functions to allow int/double overload in "S2_core"
// =================================================================================================

inline double compare ( int    f, int    g ) { return (f==g)? 1. : 0.; }
inline double compare ( int    f, double g ) { return (f   )? g  : 0.; }
inline double compare ( double f, int    g ) { return (g   )? f  : 0.; }
inline double compare ( double f, double g ) { return f*g            ; }
inline double compare ( int    f           ) { return (f   )? 1. : 0.; }
inline double compare ( double f           ) { return f              ; }
inline double unity   ( int    f           ) { return 1.             ; }
inline double unity   ( double f           ) { return 1.             ; }

// =================================================================================================
// core functions for "S2" and "W2"
// the function "func" will determine the normalization
// =================================================================================================

template <class T, class U>
std::tuple<MatD,double> S2_core (\
  Mat<T> &f, Mat<U> &g, VecS roi,\
  double (*func)(U) )
{
  if ( f.shape()!=g.shape() )
    throw std::length_error("Shape of input images inconsistent");

  for ( auto i : roi )
    if ( i%2==0 )
      throw std::length_error("'roi' must be odd shaped");

  int h,i,j,dh,di,dj,H,I,J,dH,dI,dJ;

  MatD ret(roi); ret.zeros();

  VecS mid = midpoint(roi);

  std::tie( H, I, J) = unpack3d(f.shape(),1);
  std::tie(dH,dI,dJ) = unpack3d(mid      ,0);

  f  .atleast_3d();
  g  .atleast_3d();
  ret.atleast_3d();

  // compute correlation
  for ( h=0 ; h<H ; h++ )
    for ( i=0 ; i<I ; i++ )
      for ( j=0 ; j<J ; j++ )
        if ( f(h,i,j) )
          for ( dh=-dH ; dh<=dH ; dh++ )
            for ( di=-dI ; di<=dI ; di++ )
              for ( dj=-dJ ; dj<=dJ ; dj++ )
                ret(dh+dH,di+dI,dj+dJ) +=\
                  compare(f(h,i,j),g(P(h+dh,H),P(i+di,I),P(j+dj,J)));

  // compute normalization
  double norm = 0.;
  for ( size_t i=0 ; i<f.size() ; i++ )
    norm += func(f[i]);

  return std::make_tuple(ret/norm,norm);
}

// -------------------------------------------------------------------------------------------------

template <class T, class U>
std::tuple<MatD,MatD> S2_core (\
  Mat<T> &f, Mat<U> &g, VecS roi,\
  MatI &fmsk, MatI &gmsk, bool zeropad, bool periodic,\
  double (*func)(U) )
{
  if ( f.shape()!=g.shape() )
    throw std::length_error("Shape of input images inconsistent");
  if ( f.shape()!=fmsk.shape() || f.shape()!=gmsk.shape() )
    throw std::length_error("Shape of input images is inconsistent with mask(s)");

  for ( auto i : roi )
    if ( i%2==0 )
      throw std::length_error("'roi' must be odd shaped");

  int h,i,j,dh,di,dj,H,I,J,dH,dI,dJ,bH=0,bI=0,bJ=0;

  MatD ret (roi); ret .zeros();
  MatD norm(roi); norm.zeros();

  f   .atleast_3d();
  g   .atleast_3d();
  fmsk.atleast_3d();
  gmsk.atleast_3d();
  ret .atleast_3d();
  norm.atleast_3d();

  VecS mid = midpoint(roi);

  if ( zeropad ) {
    f    = pad(f   ,mid  );
    g    = pad(g   ,mid  );
    fmsk = pad(fmsk,mid,1);
    gmsk = pad(gmsk,mid,1);
  }

  std::tie( H, I, J) = unpack3d(f.shape(),1);
  std::tie(dH,dI,dJ) = unpack3d(mid      ,0);

  // define boundary region to skip
  if ( !periodic )
    std::tie(bH,bI,bJ) = unpack3d(mid,0);

  // compute correlation
  for ( h=bH ; h<H-bH ; h++ )
    for ( i=bI ; i<I-bI ; i++ )
      for ( j=bJ ; j<J-bJ ; j++ )
        if ( f(h,i,j) && !fmsk(h,i,j) )
          for ( dh=-dH ; dh<=dH ; dh++ )
            for ( di=-dI ; di<=dI ; di++ )
              for ( dj=-dJ ; dj<=dJ ; dj++ )
                if ( !gmsk(P(h+dh,H),P(i+di,I),P(j+dj,J)) )
                  ret(dh+dH,di+dI,dj+dJ) +=\
                    compare(f(h,i,j),g(P(h+dh,H),P(i+di,I),P(j+dj,J)));

  // compute normalization (account for masked voxels)
  for ( h=bH ; h<H-bH ; h++ )
    for ( i=bI ; i<I-bI ; i++ )
      for ( j=bJ ; j<J-bJ ; j++ )
        if ( func(f(h,i,j)) && !fmsk(h,i,j) )
          for ( dh=-dH ; dh<=dH ; dh++ )
            for ( di=-dI ; di<=dI ; di++ )
              for ( dj=-dJ ; dj<=dJ ; dj++ )
                if ( !gmsk(P(h+dh,H),P(i+di,I),P(j+dj,J)) )
                  norm(dh+dH,di+dI,dj+dJ) += func(f(h,i,j));

  // apply normalization & return
  return std::make_tuple(ret/norm,norm);
}

// =================================================================================================
// 2-point probability/cluster (binary/int) / correlation (double)
// =================================================================================================

template <class T>
std::tuple<MatD,double> S2 (\
  Mat<T> &f, Mat<T> &g, VecS roi )
{
  return S2_core(f,g,roi,&unity); // (norm == f.size())
}

// -------------------------------------------------------------------------------------------------

template <class T>
std::tuple<MatD,MatD> S2 (\
  Mat<T> &f, Mat<T> &g, VecS roi,\
  MatI &fmsk, MatI &gmsk, bool zeropad, bool periodic )
{
  return S2_core(f,g,roi,fmsk,gmsk,zeropad,periodic,&unity);
}

// -------------------------------------------------------------------------------------------------

template <class T>
std::tuple<MatD,MatD> S2 (\
  Mat<T> &f, Mat<T> &g, VecS roi,\
  MatI &fmsk, bool zeropad, bool periodic )
{
  MatI gmsk(g.shape()); gmsk.zeros();
  return S2_core(f,g,roi,fmsk,gmsk,zeropad,periodic,&unity);
}

// -------------------------------------------------------------------------------------------------

template <class T>
std::tuple<MatD,MatD> S2 (\
  Mat<T> &f, Mat<T> &g, VecS roi,\
  bool zeropad, bool periodic )
{
  MatI fmsk(f.shape()); fmsk.zeros();
  MatI gmsk(g.shape()); gmsk.zeros();
  return S2_core(f,g,roi,fmsk,gmsk,zeropad,periodic,&unity);
}

// -------------------------------------------------------------------------------------------------

template std::tuple<MatD,double> S2<int   >( MatI &, MatI &, VecS);

template std::tuple<MatD,double> S2<double>( MatD &, MatD &, VecS);

template std::tuple<MatD,MatD>   S2<int   >( MatI &, MatI &, VecS, MatI &, MatI &, bool, bool );

template std::tuple<MatD,MatD>   S2<double>( MatD &, MatD &, VecS, MatI &, MatI &, bool, bool );

template std::tuple<MatD,MatD>   S2<int   >( MatI &, MatI &, VecS, MatI &, bool, bool );

template std::tuple<MatD,MatD>   S2<double>( MatD &, MatD &, VecS, MatI &, bool, bool );

template std::tuple<MatD,MatD>   S2<int   >( MatI &, MatI &, VecS, bool, bool );

template std::tuple<MatD,MatD>   S2<double>( MatD &, MatD &, VecS, bool, bool );

// =================================================================================================
// conditional 2-point probability
// =================================================================================================

template <class T, class U> std::tuple<MatD,double> W2 (\
  Mat<T> &W, Mat<U> &I, VecS roi )
{
  return S2_core(W,I,roi,&compare);
}

// -------------------------------------------------------------------------------------------------

template <class T, class U> std::tuple<MatD,MatD> W2 (\
  Mat<T> &W, Mat<U> &I, VecS roi,\
  MatI &mask, bool zeropad, bool periodic )
{
  MatI fmsk(I.shape()); fmsk.zeros();
  return S2_core(W,I,roi,fmsk,mask,zeropad,periodic,&compare);
}

// -------------------------------------------------------------------------------------------------

template <class T, class U> std::tuple<MatD,MatD> W2 (\
  Mat<T> &W, Mat<U> &I, VecS roi,\
  bool zeropad, bool periodic )
{
  MatI fmsk(I.shape()); fmsk.zeros();
  MatI mask(I.shape()); mask.zeros();
  return S2_core(W,I,roi,fmsk,mask,zeropad,periodic,&compare);
}

// -------------------------------------------------------------------------------------------------

template std::tuple<MatD,double> W2 ( MatI &, MatI &, VecS );

template std::tuple<MatD,double> W2 ( MatI &, MatD &, VecS );

template std::tuple<MatD,double> W2 ( MatD &, MatI &, VecS );

template std::tuple<MatD,double> W2 ( MatD &, MatD &, VecS );

template std::tuple<MatD,MatD>   W2 ( MatI &, MatI &, VecS, MatI &, bool, bool );

template std::tuple<MatD,MatD>   W2 ( MatI &, MatD &, VecS, MatI &, bool, bool );

template std::tuple<MatD,MatD>   W2 ( MatD &, MatI &, VecS, MatI &, bool, bool );

template std::tuple<MatD,MatD>   W2 ( MatD &, MatD &, VecS, MatI &, bool, bool );

template std::tuple<MatD,MatD>   W2 ( MatI &, MatI &, VecS, bool, bool );

template std::tuple<MatD,MatD>   W2 ( MatI &, MatD &, VecS, bool, bool );

template std::tuple<MatD,MatD>   W2 ( MatD &, MatI &, VecS, bool, bool );

template std::tuple<MatD,MatD>   W2 ( MatD &, MatD &, VecS, bool, bool );

// =================================================================================================
// weighted 2-point correlation -> collapse to center
// =================================================================================================

template <class T>
std::tuple<MatD,MatD> W2c ( MatI &clus, MatI &cntr, Mat<T> &src, VecS roi,
  MatI &mask, str mode, bool periodic )
{
  if ( src.shape()!=clus.shape() || src.shape()!=cntr.shape() )
    throw std::length_error("'I', 'clus', and 'cntr' are inconsistent");

  if ( src.shape()!=mask.shape() )
    throw std::length_error("'I' and 'mask' are inconsistent");

  for ( auto i : roi )
    if ( i%2==0 )
      throw std::length_error("'roi' must be odd shaped");

  int jpix,label;
  int h,i,j,dh,di,dj,H,I,J,dH,dI,dJ,bH=0,bI=0,bJ=0;

  MatD ret (roi); ret .zeros();
  MatD norm(roi); norm.zeros();

  VecI begin(3),end(3);
  for ( size_t i=0 ; i<3 ; i++ ) { begin[i] = 0; end[i] = 0; }

  VecS mid = midpoint(roi);

  std::tie( H, I, J) = unpack3d(src.shape(),1);
  std::tie(dH,dI,dJ) = unpack3d(mid        ,0);

  src .atleast_3d();
  clus.atleast_3d();
  cntr.atleast_3d();
  mask.atleast_3d();
  ret .atleast_3d();
  norm.atleast_3d();

  // define boundary region to skip
  if ( !periodic ) std::tie(bH,bI,bJ) = unpack3d(mid,0);

  // define the "end"-point of each voxel path
  MatI pnt = stamp_points(roi);

  // loop over ROI
  for ( size_t ipnt=0 ; ipnt<pnt.shape()[0] ; ipnt++ )
  {
    // copy end-point
    for ( size_t i=0 ; i<pnt.shape()[1] ; i++ ) end[i] = pnt(ipnt,i);

    // voxel-path
    MatI pix = path(begin,end,mode);

    // loop over image
    for ( h=bH ; h<(H-bH) ; h++ ) {
      for ( i=bI ; i<(I-bI) ; i++ ) {
        for ( j=bJ ; j<(J-bJ) ; j++ ) {
          if ( cntr(h,i,j) )
          {
            // store the label
            label = cntr(h,i,j);
            // only proceed when the center is inside the cluster
            if ( clus(h,i,j) == label )
            {
              // initialize
              jpix = -1;
              // loop over the voxel-path
              for ( size_t ipix=0 ; ipix<pix.shape()[0] ; ipix++ )
              {
                dh = pix(ipix,0);
                di = pix(ipix,1);
                dj = pix(ipix,2);
                // loop through the voxel-path until the end of a cluster
                if ( clus(P(h+dh,H),P(i+di,I),P(j+dj,J)) != label and jpix<0 ) jpix = 0;
                // store: loop from the beginning of the path and store there
                if ( jpix>=0 ) {
                  if ( !mask(P(h+dh,H),P(i+di,I),P(j+dj,J)) ) {
                    norm(dH+pix(jpix,0),dI+pix(jpix,1),dJ+pix(jpix,2)) += 1.;
                    ret (dH+pix(jpix,0),dI+pix(jpix,1),dJ+pix(jpix,2)) += \
                      compare(src(P(h+dh,H),P(i+di,I),P(j+dj,J)));
                  }
                }
                // update counter
                jpix++;
              }
            }
          }
        }
      }
    }
  }

  // apply normalization
  for ( size_t i=0 ; i<ret.size() ; i++ )
    ret[i] /= std::max(norm[i],1.);

  return std::make_tuple(ret,norm);

}

// -------------------------------------------------------------------------------------------------

template <class T>
std::tuple<MatD,MatD> W2c (
  MatI &clus, MatI &cntr, Mat<T> &I,
  VecS roi, str mode, bool periodic )
{
  MatI mask(I.shape()); mask.zeros();
  return W2c(clus,cntr,I,roi,mask,mode,periodic);
}

// -------------------------------------------------------------------------------------------------

template <class T>
std::tuple<MatD,MatD> W2c (
  MatI &W, Mat<T> &I, VecS roi, MatI &mask,
  str mode, bool periodic )
{
  MatI clus,cntr;

  std::tie(clus,cntr) = clusters(W,periodic);

  return W2c(clus,cntr,I,roi,mask,mode,periodic);
}

// -------------------------------------------------------------------------------------------------

template <class T>
std::tuple<MatD,MatD> W2c (
  MatI &W, Mat<T> &I, VecS roi,
  str mode, bool periodic )
{
  MatI clus,cntr;
  MatI mask(I.shape()); mask.zeros();

  std::tie(clus,cntr) = clusters(W,periodic);

  return W2c(clus,cntr,I,roi,mask,mode,periodic);
}

// -------------------------------------------------------------------------------------------------

template std::tuple<MatD,MatD> W2c ( MatI &, MatI &, MatD &, VecS, MatI &, str, bool );

template std::tuple<MatD,MatD> W2c ( MatI &, MatI &, MatI &, VecS, MatI &, str, bool );

template std::tuple<MatD,MatD> W2c ( MatI &, MatI &, MatD &, VecS, str, bool );

template std::tuple<MatD,MatD> W2c ( MatI &, MatI &, MatI &, VecS, str, bool );

template std::tuple<MatD,MatD> W2c ( MatI &, MatD &, VecS, MatI &, str, bool );

template std::tuple<MatD,MatD> W2c ( MatI &, MatI &, VecS, MatI &, str, bool );

template std::tuple<MatD,MatD> W2c ( MatI &, MatD &, VecS, str, bool );

template std::tuple<MatD,MatD> W2c ( MatI &, MatI &, VecS, str, bool );

// =================================================================================================
// lineal path function
// =================================================================================================

std::tuple<MatD,MatD> L ( MatI &src,
  VecS roi, str mode, bool periodic )
{
  for ( auto i : roi )
    if ( i%2==0 )
      throw std::length_error("'roi' must be odd shaped");

  int h,i,j,dh,di,dj,H,I,J,dH,dI,dJ,bH=0,bI=0,bJ=0;

  MatD ret (roi); ret .zeros();
  MatD norm(roi); norm.zeros();

  VecI begin(3),end(3);
  for ( size_t i=0 ; i<3 ; i++ ) { begin[i] = 0; end[i] = 0; }

  VecS mid = midpoint(roi);

  std::tie( H, I, J) = unpack3d(src.shape(),1);
  std::tie(dH,dI,dJ) = unpack3d(mid        ,0);

  src .atleast_3d();
  ret .atleast_3d();
  norm.atleast_3d();

  // define boundary region to skip
  if ( !periodic )
    std::tie(bH,bI,bJ) = unpack3d(mid,0);

  // define the "end"-point of each voxel path
  MatI pnt = stamp_points(roi);

  // correlation
  for ( size_t ipnt=0 ; ipnt<pnt.shape()[0] ; ipnt++ )
  {
    // copy end-point
    for ( size_t i=0 ; i<pnt.shape()[1] ; i++ )
      end[i] = pnt(ipnt,i);
    // voxel-path
    MatI pix = path(begin,end,mode);

    // loop over image
    for ( h=bH ; h<(H-bH) ; h++ ) {
      for ( i=bI ; i<(I-bI) ; i++ ) {
        for ( j=bJ ; j<(J-bJ) ; j++ ) {

          for ( size_t ipix=0 ; ipix<pix.shape()[0] ; ipix++ ) {
            dh = pix(ipix,0);
            di = pix(ipix,1);
            dj = pix(ipix,2);
            // new voxel in path not "true" in f: terminate this path
            if ( !src(P(h+dh,H),P(i+di,I),P(j+dj,J)) )
              break;
            // positive correlation: add to result
            ret(dh+dH,di+dI,dj+dJ) += 1.;
          }
        }
      }
    }
  }

  // normalization
  for ( size_t ipnt=0 ; ipnt<pnt.shape()[0] ; ipnt++ )
  {
    // copy end-point
    for ( size_t i=0 ; i<pnt.shape()[1] ; i++ )
      end[i] = pnt(ipnt,i);
    // voxel-path
    MatI pix = path(begin,end,mode);

    // loop over voxel-path
    for ( size_t ipix=0 ; ipix<pix.shape()[0] ; ipix++ )
      norm(pix(ipix,0)+dH,pix(ipix,1)+dI,pix(ipix,2)+dJ) += \
        static_cast<double>((H-bH)*(I-bI)*(J-bJ));
  }

  // apply normalization
  for ( size_t i=0 ; i<ret.size() ; i++ )
    ret[i] /= std::max(norm[i],1.);

  return std::make_tuple(ret,norm);
}

// =================================================================================================

} // namespace Image
} // namespace GooseEYE

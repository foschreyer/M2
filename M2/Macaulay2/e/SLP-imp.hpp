// Copyright 2015 Anton Leykin and Mike Stillman

// Anton Leykin's code in this file is in the public domain.

#ifndef _slp_imp_hpp_
#define _slp_imp_hpp_

#include <ctime>
#include <chrono>

// SLEvaluator
template<typename RT>
SLEvaluatorConcrete<RT>::SLEvaluatorConcrete(SLProgram *SLP, M2_arrayint cPos,  M2_arrayint vPos, 
					     const MutableMat< SMat<RT> >* consts /* DMat<RT>& DMat_consts */)
  : mRing(consts->getMat().ring())
{
  ERROR("not implemented");
}    

//copy constructor
template<typename RT>
SLEvaluatorConcrete<RT>::SLEvaluatorConcrete(const SLEvaluatorConcrete<RT>& a)
  : mRing(a.ring()), values(a.values.size())   
{
  slp = a.slp;
  varsPos = a.varsPos;
  for(auto i=values.begin(), j=a.values.begin(); i!=values.end(); ++i,++j)
    ring().init_set(*i,*j);
}

template<typename RT>
SLEvaluatorConcrete<RT>::~SLEvaluatorConcrete()
{
  for(auto v : values) 
    ring().clear(v);
}

template<typename RT>
SLEvaluatorConcrete<RT>::SLEvaluatorConcrete(SLProgram *SLP, M2_arrayint cPos,  M2_arrayint vPos, 
					     const MutableMat< DMat<RT> >* consts /* DMat<RT>& DMat_consts */)
  : mRing(consts->getMat().ring())    
{
  if (consts->n_rows() != 1 || consts->n_cols() != cPos->len)
    ERROR("1-row matrix expected; or numbers of constants don't match");
  slp = SLP;
  // for(int i=0; i<cPos->len; i++) 
  //  constsPos.push_back(slp->inputCounter+cPos->array[i]);
  for(int i=0; i<vPos->len; i++) 
    varsPos.push_back(slp->inputCounter+vPos->array[i]);
  values.resize(slp->inputCounter+slp->mNodes.size());
  for(auto i=values.begin(); i!=values.end(); ++i)
    ring().init(*i);
  // R = consts->get_ring();
  for (int i=0; i<cPos->len; i++) 
    ring().set(values[slp->inputCounter+cPos->array[i]],consts->getMat().entry(0,i));
}

template<typename RT>
SLEvaluator* SLEvaluatorConcrete<RT>::specialize(const MutableMatrix* parameters) const
{
  auto p = dynamic_cast<const MutableMat< DMat<RT> >*>(parameters);
  if (p == nullptr) { 
    ERROR("specialize: expected a dense mutable matrix");
    return nullptr;
  }
  return specialize(p);
}

template<typename RT>
SLEvaluator* SLEvaluatorConcrete<RT>::specialize(const MutableMat< DMat<RT> >* parameters) const
{
  if (parameters->n_cols() != 1 || parameters->n_rows() > varsPos.size())
    ERROR("1-column matrix expected; or #parameters > #vars");  
  auto *e = new SLEvaluatorConcrete<RT>(*this);
  size_t nParams = parameters->n_rows();
  for (int i=0; i<nParams; ++i) 
    ring().set(e->values[varsPos[i]],parameters->getMat().entry(i,0));
  e->varsPos.erase(e->varsPos.begin(),e->varsPos.begin()+nParams);
  return e;
}


template<typename RT>
void SLEvaluatorConcrete<RT>::computeNextNode()
{
  ElementType& v = *vIt;
  switch (*nIt++) {
  case SLProgram::MProduct:
    ring().set_from_long(v,1);
    for (int i=0; i<*numInputsIt; i++)
      ring().mult(v,v,*(vIt+(*inputPositionsIt++)));
    numInputsIt++;
    break;
  case SLProgram::MSum:
    ring().set_zero(v);
    for (int i=0; i<*numInputsIt; i++)
      ring().add(v,v,*(vIt+(*inputPositionsIt++)));
    numInputsIt++;
    break;
  case SLProgram::Det:
    {
      int n = static_cast<int>(sqrt(*numInputsIt++));
      DMat<RT> mat(ring(),n,n);
      for (int i=0; i<n; i++)
        for (int j=0; j<n; j++)
          ring().set(mat.entry(i,j),*(vIt+(*inputPositionsIt++)));
      DMatLinAlg<RT>(mat).determinant(v);      
    }
    break;
  case SLProgram::Divide:
    ring().set(v,*(vIt+(*inputPositionsIt++)));
    ring().divide(v,v,*(vIt+(*inputPositionsIt++)));
    break;
  default: ERROR("unknown node type");
  }
}

template<typename RT>
bool SLEvaluatorConcrete<RT>::evaluate(const MutableMatrix* inputs, MutableMatrix* outputs)
{
  auto inp = dynamic_cast<const MutableMat< DMat<RT> >*>(inputs);
  auto out = dynamic_cast<MutableMat< DMat<RT> >*>(outputs);
  if (inp == nullptr) { 
    ERROR("inputs: expected a dense mutable matrix");
    return false;
  }
  if (out == nullptr) { 
    ERROR("outputs: expected a dense mutable matrix");
    return false;
  }
  if (&ring() != &inp->getMat().ring()) { 
    ERROR("inputs are in a different ring");
    return false;
  }
  if (&ring() != &out->getMat().ring()) { 
    ERROR("outputs are in a different ring");
    return false;
  }
  
  return evaluate(inp->getMat(),out->getMat());
}


template<typename RT> 
bool SLEvaluatorConcrete<RT>::evaluate(const DMat<RT>& inputs, DMat<RT>& outputs)
{
  if (varsPos.size() != inputs.numRows()*inputs.numColumns()) { 
    ERROR("inputs: the number of inputs does not match the number of entries in the inputs matrix");
    std::cout << varsPos.size() << " != " << inputs.numRows()*inputs.numColumns() << std::endl;
    return false;
  }
  size_t i=0;
  for(size_t r=0; r<inputs.numRows(); r++)
    for(size_t c=0; c<inputs.numColumns(); c++)
      ring().set(values[varsPos[i++]],inputs.entry(r,c));
  
  nIt = slp->mNodes.begin();
  numInputsIt = slp->mNumInputs.begin();
  inputPositionsIt = slp->mInputPositions.begin();
  for (vIt = values.begin()+slp->inputCounter; vIt != values.end(); ++vIt) 
    computeNextNode();

  if (slp->mOutputPositions.size() != outputs.numRows()*outputs.numColumns()) { 
    ERROR("outputs: the number of outputs does not match the number of entries in the outputs matrix");
    std::cout <<  slp->mOutputPositions.size() << " != " << outputs.numRows() << " * " << outputs.numColumns() << std::endl;
    return false;
  }
  i=0;
  for(size_t r=0; r<outputs.numRows(); r++)
    for(size_t c=0; c<outputs.numColumns(); c++)
      ring().set(outputs.entry(r,c),values[ap(slp->mOutputPositions[i++])]);
  return true;
}

template<typename RT>
void SLEvaluatorConcrete<RT>::text_out(buffer& o) const { 
  o << "SLEvaluator(slp = ";
  slp->text_out(o);
  o << ", mRing = ";
  ring().text_out(o);
  o << ")" << newline; 
}

template <typename RT>
Homotopy* SLEvaluatorConcrete<RT>::createHomotopy(SLEvaluator* Hxt, SLEvaluator* HxH)
{
  auto castHxt = dynamic_cast < SLEvaluatorConcrete < RT > * > (Hxt);
  auto castHxH = dynamic_cast < SLEvaluatorConcrete < RT > * > (HxH);
  if (not castHxt or not castHxH) { 
    ERROR("expected SLEvaluators in the same ring");
    return nullptr;
  } 
  return new HomotopyConcrete< RT, typename HomotopyAlgorithm<RT>::Algorithm >(*this, *castHxt, *castHxH);
}


template <typename RT>
inline void norm2(const DMat<RT>& M, size_t n, typename RT::RealElementType& result)
{
  const auto& C = M.ring();
  const auto& R = C.real_ring();
  typename RT::RealElementType c;
  R.init(c);
  R.set_zero(result);
  for(size_t i=0; i<n; i++) {
    C.abs_squared(c,M.entry(0,i));
    R.add(result,result,c);
  }
  R.clear(c);
}

enum SolutionStatus {UNDETERMINED, PROCESSING, REGULAR, SINGULAR, INFINITY_FAILED, MIN_STEP_FAILED, ORIGIN_FAILED, INCREASE_PRECISION, DECREASE_PRECISION};

// ****************************** XXX **************************************************
template <typename RT> 
bool HomotopyConcrete< RT, FixedPrecisionHomotopyAlgorithm >::track(const MutableMatrix* inputs, MutableMatrix* outputs, 
                     MutableMatrix* output_extras,  
                     gmp_RR init_dt, gmp_RR min_dt,
                     gmp_RR epsilon, // o.CorrectorTolerance,
                     int max_corr_steps, 
                     gmp_RR infinity_threshold
                   ) 
{
  std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
  size_t solveLinearTime = 0, solveLinearCount = 0, evaluateTime = 0;
 
  // std::cout << "inside HomotopyConcrete<RT,FixedPrecisionHomotopyAlgorithm>::track" << std::endl;
  // double the_smallest_number = 1e-13;
  const Ring* matRing = inputs->get_ring();
  if (outputs->get_ring()!= matRing) { 
    ERROR("outputs and inputs are in different rings");
    return false;
  }
  auto inp = dynamic_cast<const MutableMat< DMat<RT> >*>(inputs);
  auto out = dynamic_cast<MutableMat< DMat<RT> >*>(outputs);
  auto out_extras = dynamic_cast<MutableMat< DMat<M2::ARingZZGMP> >*>(output_extras);
  if (inp == nullptr) { 
    ERROR("inputs: expected a dense mutable matrix");
    return false;
  }
  if (out == nullptr) { 
    ERROR("outputs: expected a dense mutable matrix");
    return false;
  }
  if (out_extras == nullptr) { 
    ERROR("output_extras: expected a dense mutable matrix");
    return false;
  }

  auto& in = inp->getMat();
  auto& ou = out->getMat();
  auto& oe = out_extras->getMat();
  size_t n_sols = in.numColumns();  
  size_t n = in.numRows()-1; // number of x vars

  if (ou.numColumns() != n_sols or ou.numRows() != n+2) { 
    ERROR("output: wrong shape");
    return false;
  }
  if (oe.numColumns() != n_sols or oe.numRows() != 2) { 
    ERROR("output_extras: wrong shape");
    return false;
  }
  
  const RT& C = in.ring();  
  typename RT::RealRingType R = C.real_ring();  

  typedef typename RT::ElementType ElementType;
  typedef typename RT::RealRingType::ElementType RealElementType;
  typedef MatElementaryOps< DMat< RT > > MatOps;
 
  RealElementType t_step;
  RealElementType min_step2;
  RealElementType epsilon2;
  RealElementType infinity_threshold2;
  R.init(t_step);
  R.init(min_step2);
  R.init(epsilon2);
  R.init(infinity_threshold2);
  R.set_from_BigReal(t_step,init_dt); // initial step
  R.set_from_BigReal(min_step2,min_dt);
  R.mult(min_step2, min_step2, min_step2); //min_step^2
  R.set_from_BigReal(epsilon2,epsilon); 
  int tolerance_bits = -R.log2abs(epsilon2);  
  R.mult(epsilon2, epsilon2, epsilon2); //epsilon^2
  R.set_from_BigReal(infinity_threshold2,infinity_threshold); 
  R.mult(infinity_threshold2, infinity_threshold2, infinity_threshold2);
  int num_successes_before_increase = 3;

  RealElementType t0,dt,one_minus_t0,dx_norm2,x_norm2,abs2dc;
  R.init(t0);
  R.init(dt);
  R.init(one_minus_t0);
  R.init(dx_norm2);
  R.init(x_norm2);
  R.init(abs2dc);
  
  // constants
  RealElementType one,two,four,six,one_half,one_sixth;
  RealElementType& dt_factor = one_half; 
  R.init(one);
  R.set_from_long(one,1);    
  R.init(two);
  R.set_from_long(two,2);    
  R.init(four);
  R.set_from_long(four,4);
  R.init(six);
  R.set_from_long(six,6);    
  R.init(one_half);
  R.divide(one_half,one,two);
  R.init(one_sixth);
  R.divide(one_sixth,one,six);

  ElementType c_init,c_end,dc,one_half_dc;
  C.init(c_init);
  C.init(c_end);
  C.init(dc);
  C.init(one_half_dc);

  // think: x_0..x_(n-1), c
  // c = the homotopy continuation parameter "t" upstair, varies on a (staight line) segment of complex plane (from c_init to c_end)  
  // t = a real running in the interval [0,1] 

  DMat<RT> x0c0(C,n+1,1); 
  DMat<RT> x1c1(C,n+1,1);
  DMat<RT> xc(C,n+1,1);
  DMat<RT> HxH(C,n,n+1);
  DMat<RT>& Hxt = HxH; // the matrix has the same shape: reuse memory  
  DMat<RT> LHSmat(C,n,n);
  auto LHS = submatrix(LHSmat); 
  DMat<RT> RHSmat(C,n,1);
  auto RHS = submatrix(RHSmat); 
  DMat<RT> dx(C,n,1);
  DMat<RT> dx1(C,n,1);
  DMat<RT> dx2(C,n,1);
  DMat<RT> dx3(C,n,1);
  DMat<RT> dx4(C,n,1);
  DMat<RT> Jinv_times_random(C,n,1);

  ElementType& c0 = x0c0.entry(n,0);
  ElementType& c1 = x1c1.entry(n,0);  
  ElementType& c = xc.entry(n,0);
  RealElementType& tol2 = epsilon2; // current tolerance squared
  bool linearSolve_success;
  for(size_t s=0; s<n_sols; s++) {
    SolutionStatus status = PROCESSING;
    // set initial solution and initial value of the continuation parameter
    //for(size_t i=0; i<=n; i++)   
    //  C.set(x0c0.entry(i,0), in.entry(i,s));
    submatrix(x0c0) = submatrix(const_cast<DMat<RT>&>(in), 0,s, n+1,1); 
    C.set(c_init,c0);
    C.set(c_end,ou.entry(n,s));

    R.set_zero(t0);
    bool t0equals1 = false;

    // t_step is actually the initial (absolute) length of step on the interval [c_init,c_end]
    // dt is an increment for t on the interval [0,1]
    R.set(dt,t_step);
    C.subtract(dc,c_end,c_init); 
    C.abs(abs2dc,dc); // don't wnat to create new temporary elts: reusing dc and abs2dc
    R.divide(dt,dt,abs2dc);

    int predictor_successes = 0;
    int count = 0; // number of steps
    // track the real segment (1-t)*c0 + t*c1, a\in [0,1]
    while (status == PROCESSING and not t0equals1) {
      if (M2_numericalAlgebraicGeometryTrace>3) {
        buffer o; 
        R.elem_text_out(o,t0,true,false,false);
        std::cout << "t0 = " << o.str();
        o.reset();
        C.elem_text_out(o,c0,true,false,false);
        std::cout << ", c0 = " << o.str() << std::endl;
      }
      R.subtract(one_minus_t0,one,t0);
      if (R.compare_elems(dt,one_minus_t0)>0) {
        R.set(dt,one_minus_t0);
        t0equals1 = true;
        C.subtract(dc,c_end,c0);
        C.set(c1,c_end);
      } else {
        C.subtract(dc,c_end,c0);
        C.mult(dc,dc,dt);
        C.divide(dc,dc,one_minus_t0);
        C.add(c1,c0,dc);
      }
      
      // PREDICTOR in: x0c0,dt,pred_type
      //           out: dx
      
      // make prediction
      //for(size_t i=0; i<n; i++)   
      //  C.set_zero(dx.entry(i,0)); // "zero"-th order predictor
      submatrix(dx) = 0;

      // Runge-Kutta 4th order
      C.mult(one_half_dc, dc, one_half);

      //copy_complex_array<ComplexField>(n+1,x0t0,xt);
      //for(size_t i=0; i<=n; i++)   
      //  C.set(xc.entry(i,0), x0c0.entry(i,0));
      submatrix(xc) = submatrix(x0c0);

      // dx1
      /* evaluate_slpHxt(n,xt,Hxt);
         LHS = Hxt;
         RHS = Hxt+n*n;
         //
         negate_complex_array<ComplexField>(n,RHS);
         solve_via_lapack_without_transposition(n,LHS,1,RHS,dx1);
      */
      std::chrono::steady_clock::time_point startEvaluate = std::chrono::steady_clock::now();
      mHxt.evaluate(xc,Hxt);
      std::chrono::steady_clock::time_point endEvaluate = std::chrono::steady_clock::now();
      evaluateTime += std::chrono::duration_cast<std::chrono::nanoseconds>(endEvaluate - startEvaluate).count(); 
      //MatOps::setFromSubmatrix(Hxt,0,n-1,0,n-1,LHS); // Hx
      //MatOps::setFromSubmatrix(Hxt,0,n-1,n,n,RHS); // Ht
      LHS = submatrix(Hxt, 0,0, n,n);
      RHS = submatrix(Hxt, 0,n, n,1);
      
      MatrixOps::negateInPlace(RHSmat);
      //solve LHS*dx1 = RHS

      std::chrono::steady_clock::time_point startLinear = std::chrono::steady_clock::now();
      linearSolve_success = MatrixOps::solveLinear(LHSmat,RHSmat,dx1);
      std::chrono::steady_clock::time_point endLinear = std::chrono::steady_clock::now();
      solveLinearCount++;
      solveLinearTime += std::chrono::duration_cast<std::chrono::nanoseconds>(endLinear - startLinear).count(); 

      // dx2
      if (linearSolve_success) { 
        // multiply_complex_array_scalar<ComplexField>(n,dx1,one_half*(*dt));
        //for(size_t i=0; i<n; i++)   
        //  C.mult(dx1.entry(i,0),dx1.entry(i,0),one_half_dc);  
        submatrix(dx1) *= one_half_dc;
        // add_to_complex_array<ComplexField>(n,xt,dx1); // x0+.5dx1*dt
        //for(size_t i=0; i<n; i++)   
        //  C.add(xc.entry(i,0),xc.entry(i,0),dx1.entry(i,0));  
        submatrix(xc, 0,0, n,1) += submatrix(dx1);
        // xt[n] += one_half*(*dt); // t0+.5dt
        C.add(c,c,one_half_dc); 
        // evaluate_slpHxt(n,xt,Hxt);
        startEvaluate = std::chrono::steady_clock::now();
        mHxt.evaluate(xc,Hxt);
        endEvaluate = std::chrono::steady_clock::now();
        evaluateTime += std::chrono::duration_cast<std::chrono::nanoseconds>(endEvaluate - startEvaluate).count(); 
        // LHS = Hxt; RHS = Hxt+n*n;
        //negate_complex_array<ComplexField>(n,RHS);
        //MatOps::setFromSubmatrix(Hxt,0,n-1,0,n-1,LHS); // Hx
        //MatOps::setFromSubmatrix(Hxt,0,n-1,n,n,RHS); // Ht
        LHS = submatrix(Hxt, 0,0, n,n);
        RHS = submatrix(Hxt, 0,n, n,1);
        MatrixOps::negateInPlace(RHSmat);
        //solve LHS*dx2 = RHS
        startLinear = std::chrono::steady_clock::now();
        linearSolve_success = MatrixOps::solveLinear(LHSmat,RHSmat,dx2);
        endLinear = std::chrono::steady_clock::now();
        solveLinearCount++;
        solveLinearTime += std::chrono::duration_cast<std::chrono::nanoseconds>(endLinear - startLinear).count(); 
      }

      // dx3
      if (linearSolve_success) { 
        // multiply_complex_array_scalar<ComplexField>(n,dx2,one_half*(*dt));
        //for(size_t i=0; i<n; i++)   
        //  C.mult(dx2.entry(i,0),dx2.entry(i,0),one_half_dc);
        submatrix(dx2) *= one_half_dc;
        
        // copy_complex_array<ComplexField>(n,x0t0,xt); // spare t
        //for(size_t i=0; i<n; i++)   
        //  C.set(xc.entry(i,0), x0c0.entry(i,0));
        submatrix(xc, 0,0, n,1) = submatrix(x0c0,  0,0, n,1);

        // add_to_complex_array<ComplexField>(n,xt,dx2); // x0+.5dx2*dt
        //for(size_t i=0; i<n; i++)   
        //  C.add(xc.entry(i,0),xc.entry(i,0),dx2.entry(i,0));  
        submatrix(xc, 0,0, n,1) += submatrix(dx2);

        // xt[n] += one_half*(*dt); // t0+.5dt (SAME)
        C.add(c,c,one_half_dc);
       
        /* evaluate_slpHxt(n,xt,Hxt);
           LHS = Hxt;
           RHS = Hxt+n*n;
           negate_complex_array<ComplexField>(n,RHS);
           LAPACK_success = LAPACK_success && solve_via_lapack_without_transposition(n,LHS,1,RHS,dx3);
        */
        startEvaluate = std::chrono::steady_clock::now();
        mHxt.evaluate(xc,Hxt);
        endEvaluate = std::chrono::steady_clock::now();
        evaluateTime += std::chrono::duration_cast<std::chrono::nanoseconds>(endEvaluate - startEvaluate).count(); 

        //MatOps::setFromSubmatrix(Hxt,0,n-1,0,n-1,LHS); // Hx
        //MatOps::setFromSubmatrix(Hxt,0,n-1,n,n,RHS); // Ht
        LHS = submatrix(Hxt, 0,0, n,n);
        RHS = submatrix(Hxt, 0,n, n,1);
        MatrixOps::negateInPlace(RHSmat);
        //solve LHS*dx3 = RHS
        startLinear = std::chrono::steady_clock::now();
        linearSolve_success = MatrixOps::solveLinear(LHSmat,RHSmat,dx3);
        endLinear = std::chrono::steady_clock::now();
        solveLinearCount++;
        solveLinearTime += std::chrono::duration_cast<std::chrono::nanoseconds>(endLinear - startLinear).count(); 
      }

      // dx4
      if (linearSolve_success) { 
        // multiply_complex_array_scalar<ComplexField>(n,dx3,*dt);
        // for(size_t i=0; i<n; i++)   
        //  C.mult(dx3.entry(i,0),dx3.entry(i,0),dc);
        submatrix(dx3) *= dc;

        // copy_complex_array<ComplexField>(n+1,x0t0,xt);
        //for(size_t i=0; i<n+1; i++)   
        //  C.set(xc.entry(i,0), x0c0.entry(i,0));
        submatrix(xc) = submatrix(x0c0); // sets c=c0 as well (not needed for dx1,dx2,dx3)
        
        // add_to_complex_array<ComplexField>(n,xt,dx3); // x0+dx3*dt
        //for(size_t i=0; i<n; i++)   
        //  C.add(xc.entry(i,0),xc.entry(i,0),dx3.entry(i,0));  
        submatrix(xc, 0,0, n,1) += submatrix(dx3); 
        // xt[n] += *dt; // t0+dt
        C.add(c,c,dc);
        /*
          evaluate_slpHxt(n,xt,Hxt);
          LHS = Hxt;
          RHS = Hxt+n*n;
          negate_complex_array<ComplexField>(n,RHS);
          LAPACK_success = LAPACK_success && solve_via_lapack_without_transposition(n,LHS,1,RHS,dx4);
        */
        startEvaluate = std::chrono::steady_clock::now();
        mHxt.evaluate(xc,Hxt);
        endEvaluate = std::chrono::steady_clock::now();
        evaluateTime += std::chrono::duration_cast<std::chrono::nanoseconds>(endEvaluate - startEvaluate).count(); 

        //MatOps::setFromSubmatrix(Hxt,0,n-1,0,n-1,LHS); // Hx
        //MatOps::setFromSubmatrix(Hxt,0,n-1,n,n,RHS); // Ht
        LHS = submatrix(Hxt, 0,0, n,n);
        RHS = submatrix(Hxt, 0,n, n,1);
        MatrixOps::negateInPlace(RHSmat);
        //solve LHS*dx4 = RHS
        startLinear = std::chrono::steady_clock::now();
        linearSolve_success = MatrixOps::solveLinear(LHSmat,RHSmat,dx4);
        endLinear = std::chrono::steady_clock::now();
        solveLinearCount++;
        solveLinearTime += std::chrono::duration_cast<std::chrono::nanoseconds>(endLinear - startLinear).count(); 
      }
      
      // "dx1" = .5*dx1*dt, "dx2" = .5*dx2*dt, "dx3" = dx3*dt
      if (linearSolve_success) { 
        // multiply_complex_array_scalar<ComplexField>(n,dx4,*dt);
        //for(size_t i=0; i<n; i++)   
        //  C.mult(dx4.entry(i,0),dx4.entry(i,0),dc);
        submatrix(dx4) *= dc; 
        // multiply_complex_array_scalar<ComplexField>(n,dx1,2);
        // ???
      
        // multiply_complex_array_scalar<ComplexField>(n,dx2,4);
        //for(size_t i=0; i<n; i++)   
        //  C.mult(dx2.entry(i,0),dx2.entry(i,0),four);
        submatrix(dx2) *= four; 
        
        // multiply_complex_array_scalar<ComplexField>(n,dx3,2);
        //for(size_t i=0; i<n; i++)   
        //  C.mult(dx3.entry(i,0),dx3.entry(i,0),two);
        submatrix(dx3) *= two; 

        // add_to_complex_array<ComplexField>(n,dx4,dx1);
        //for(size_t i=0; i<n; i++)   
        //  C.add(dx4.entry(i,0),dx4.entry(i,0),dx1.entry(i,0));
        // add_to_complex_array<ComplexField>(n,dx4,dx2);
        submatrix(dx4) += dx1; 
        //for(size_t i=0; i<n; i++)   
        //  C.add(dx4.entry(i,0),dx4.entry(i,0),dx2.entry(i,0));
        submatrix(dx4) += dx2; 
 
        // add_to_complex_array<ComplexField>(n,dx4,dx3);
        //for(size_t i=0; i<n; i++)   
        //  C.add(dx4.entry(i,0),dx4.entry(i,0),dx3.entry(i,0));
        submatrix(dx4) += dx3; 

        // multiply_complex_array_scalar<ComplexField>(n,dx4,1.0/6);
        // copy_complex_array<ComplexField>(n,dx4,dx);
        //for(size_t i=0; i<n; i++)   
        ///  C.mult(dx.entry(i,0),dx4.entry(i,0),one_sixth);
        submatrix(dx4) *= one_sixth; 
        submatrix(dx) += dx4; 
      }

      // update x0c0
      //for(size_t i=0; i<n; i++)   
      //  C.add(x1c1.entry(i,0),x0c0.entry(i,0),dx.entry(i,0));  
      submatrix(x1c1) = submatrix(x0c0);
      C.add(c1,c0,dc);   
      
      // CORRECTOR
      bool is_successful;
      int n_corr_steps = 0;
      if (linearSolve_success) {
        do {
          n_corr_steps++;
          //
          //std::cout << "evaluate...\n";
          startEvaluate = std::chrono::steady_clock::now();
          mHxH.evaluate(x1c1,HxH);
          endEvaluate = std::chrono::steady_clock::now();
          evaluateTime += std::chrono::duration_cast<std::chrono::nanoseconds>(endEvaluate - startEvaluate).count(); 
        
          //std::cout << "setFromSubmatrix 1...\n";
          MatOps::setFromSubmatrix(HxH,0,n-1,0,n-1,LHSmat); // Hx
          //std::cout << "setFromSubmatrix 2...\n";
          MatOps::setFromSubmatrix(HxH,0,n-1,n,n,RHSmat); // H
          //std::cout << "negate...\n";
          MatrixOps::negateInPlace(RHSmat);
          //solve LHS*dx = RHS

          //std::cout << "solveLinear...\n";
          startLinear = std::chrono::steady_clock::now();
          linearSolve_success = MatrixOps::solveLinear(LHSmat,RHSmat,dx);
          endLinear = std::chrono::steady_clock::now();
          solveLinearCount++;
          solveLinearTime += std::chrono::duration_cast<std::chrono::nanoseconds>(endLinear - startLinear).count(); 
          //std::cout << "solveLinear done\n";

          // x1 += dx
          for(size_t i=0; i<n; i++)   
            C.add(x1c1.entry(i,0),x1c1.entry(i,0),dx.entry(i,0));  
        
          norm2(dx,n,dx_norm2);
          norm2(x1c1,n,x_norm2);
          R.mult(x_norm2,x_norm2,tol2);
          is_successful = R.compare_elems(dx_norm2,x_norm2) < 0;
        } while (not is_successful and n_corr_steps<max_corr_steps);
      }
      //std::cout << "past corrector loop...\n";
      if (not linearSolve_success or not is_successful) {
        // predictor failure
        predictor_successes = 0;
        R.mult(dt,dt,dt_factor);
        t0equals1 = false;
        C.abs_squared(abs2dc,dc);
        if (R.compare_elems(abs2dc,min_step2)<0) 
          status = MIN_STEP_FAILED;
      } else {
        // predictor success
        predictor_successes = predictor_successes + 1;
        // x0c0 = x1c1
        // std::cout << "before...\n";
        //displayMat(x0c0);
        //std::cout << std::endl;
        //displayMat(x1c1);
        MatOps::setFromSubmatrix(x1c1,0,n,0,0,x0c0);
        //std::cout << "after...\n";
        //displayMat(x0c0);
        //std::cout << std::endl;
        //displayMat(x1c1);
        R.add(t0,t0,dt);
        count++;
        if (predictor_successes >= num_successes_before_increase) {
          predictor_successes = 0;
          R.divide(dt,dt,dt_factor);
        }       
      }
      
      norm2(x0c0,n,x_norm2);

      if (not linearSolve_success)
        status = SINGULAR;
      else if (not t0equals1) { // precision check 
#define PRECISION_SAFETY_BITS 10
        //std::cout << "evaluate...\n";
        mHxH.evaluate(x0c0,HxH);
        MatOps::setFromSubmatrix(HxH,0,n-1,0,n-1,LHSmat); // Hx
        for(int i=0; i<n; i++) 
          C.random(RHSmat.entry(i,0));
        
        //std::cout << "solveLinear...\n";
        startLinear = std::chrono::steady_clock::now();
        linearSolve_success = MatrixOps::solveLinear(LHSmat,RHSmat,Jinv_times_random);
        endLinear = std::chrono::steady_clock::now();
        solveLinearCount++;
        solveLinearTime += std::chrono::duration_cast<std::chrono::nanoseconds>(endLinear - startLinear).count(); 
        //std::cout << "solveLinear done\n";
       
        norm2(Jinv_times_random,n,dx_norm2); // this stands for ||J^{-1}||
        //!!! R.add(dx_norm2,dx_norm2,x_norm2); // ||J^{-1}|| + ||x||
                                          //   ||J^{-1}|| should be multiplied by a factor 
                                          //   reflecting an estimate on the error of evaluation of J 
        int precision_needed = PRECISION_SAFETY_BITS + tolerance_bits + R.log2abs(dx_norm2) - 1; // subtract 1 for using squares under "log"
        if (R.get_precision() < precision_needed)    
          status = INCREASE_PRECISION;
        else if (R.get_precision() != 53 and R.get_precision() > 2*precision_needed) 
          status = DECREASE_PRECISION;
      };

      // infinity/origin checks
      if (status == PROCESSING){ 
        if (R.compare_elems(infinity_threshold2,x_norm2) < 0)
          status = INFINITY_FAILED;
        else {
          if (R.is_zero(x_norm2)) status = ORIGIN_FAILED;
          else {
            R.divide(x_norm2,one,x_norm2); // 1/||x||^2
            if (R.compare_elems(infinity_threshold2,x_norm2) < 0)
              status = ORIGIN_FAILED;
          }
        }
      }
    }
    // record the solution
    // set initial solution and initial value of the continuation parameter
    for(size_t i=0; i<=n; i++)   
      C.set(ou.entry(i,s),x0c0.entry(i,0));
    C.set(ou.entry(n+1,s),dc); // store last increment attempted 
    if (status == PROCESSING)
      status = REGULAR;
    oe.ring().set_from_long(oe.entry(0,s),status);
    oe.ring().set_from_long(oe.entry(1,s),count); 
  }


  C.clear(c_init);
  C.clear(c_end);
  C.clear(dc);
  C.clear(one_half_dc);

  R.clear(t0);
  R.clear(dt);
  R.clear(one_minus_t0);
  R.clear(dx_norm2);
  R.clear(x_norm2);
  R.clear(abs2dc);

  R.clear(one);
  R.clear(two);
  R.clear(four);
  R.clear(six);
  R.clear(one_half);
  R.clear(one_sixth);

  R.clear(t_step);
  R.clear(min_step2);
  R.clear(epsilon2);
  R.clear(infinity_threshold2);
  
  std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
  if (M2_numericalAlgebraicGeometryTrace>0) {
    std::cout << "-- track took "
              << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count()
              << "ms.\n";
    std::cout << "-- # of solveLinear calls = " << solveLinearCount << std::endl;
    std::cout << "-- time of solveLinear calls = " << solveLinearTime << " ns." << std::endl;
    std::cout << "-- time of evaluate calls = " << evaluateTime << " ns." << std::endl;
  }
  return true;
}

template <typename RT, typename Algorithm>
bool HomotopyConcrete< RT, Algorithm >::track(const MutableMatrix* inputs, MutableMatrix* outputs, 
                                              MutableMatrix* output_extras,  
                                              gmp_RR init_dt, gmp_RR min_dt,
                                              gmp_RR epsilon, // o.CorrectorTolerance,
                                              int max_corr_steps, 
                                              gmp_RR infinity_threshold
                                              )
{
  ERROR("track: not implemented for this type of ring");
  return false;  
}

template<typename RT, typename Algorithm>
void HomotopyConcrete<RT, Algorithm>::text_out(buffer& o) const { 
  o << "HomotopyConcrete<...,...> : track not implemented" << newline; 
}

template<typename RT>
void HomotopyConcrete<RT, FixedPrecisionHomotopyAlgorithm>::text_out(buffer& o) const { 
  o << "HomotopyConcrete<...,fixed precision>(Hx = ";
  mHx.text_out(o);
  o << ", Hxt = ";
  mHxt.text_out(o);
  o << ", HxH = ";
  mHxH.text_out(o);
  o << ")" << newline; 
}

#endif

// Local Variables:
// compile-command: "make -C $M2BUILDDIR/Macaulay2/e "
// indent-tabs-mode: nil
// End:

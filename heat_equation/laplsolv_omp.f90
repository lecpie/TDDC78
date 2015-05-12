program laplsolv
use omp_lib
!-----------------------------------------------------------------------
! Serial program for solving the heat conduction problem 
! on a square using the Jacobi method. 
! Written by Fredrik Berntsson (frber@math.liu.se) March 2003
! Modified by Berkant Savas (besav@math.liu.se) April 2006
!-----------------------------------------------------------------------
  integer, parameter                  :: n=1000, maxiter=1000
  double precision,parameter          :: tol=1.0E-3
  double precision,dimension(0:n+1,0:n+1) :: T
  double precision,dimension(n)       :: tmp1,tmp2,tmp3
  double precision                    :: error,x
  real                                :: t1,t0
  integer                             :: i,j,k,startcol,endcol,nproc,rem,mychunksize,last
  character(len=20)                   :: str
  double precision, dimension (:,:), allocatable :: firstcols, lastcols 
  integer, dimension(:), allocatable :: startcols_i, endcols_i
  
  ! Set boundary conditions and initial values for the unknowns
  T=0.0D0
  T(0:n+1 , 0)     = 1.0D0
  T(0:n+1 , n+1)   = 1.0D0
  T(n+1   , 0:n+1) = 2.0D0
  

  ! Solve the linear system of equations using the Jacobi method
  call cpu_time(t0)
	nproc = omp_get_max_threads()
	
   print *,'total number of threads : ', omp_get_max_threads()
	
	allocate(startcols_i(nproc),endcols_i(nproc), firstcols(nproc,n), lastcols(nproc,n))
	
	rem = MOD(n,nproc)
	mychunksize = n / nproc
	last = 0
	do i=0,nproc-1
	print *, i
		startcols_i(i) = last
		endcols_i(i) = last + mychunksize
		
		if (i < rem) then
			endcols_i(i) = endcols_i(i) + 1
		end if
		
		last = endcols_i(i)

		firstcols(1:n,i) =  T(1:n, startcols_i(i))
		!lastcols(1:n,i) = T(1:n, endcols_i(i)+1)
		
		
		  print *,'Process ', i, ' starts with ', startcols_i(i), ' and ends with ',  endcols_i(i)


			
	end do
	


  
end program laplsolv

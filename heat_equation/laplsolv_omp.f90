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
  integer                             :: i,j,k,startcol,endcol,nproc,rem,mychunksize,last, myid
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
	
  ! print *,'total number of threads : ', omp_get_max_threads()
	print *, 'value of n: ', n
	allocate(startcols_i(nproc),endcols_i(nproc), firstcols(1:n,0:nproc-1), lastcols(1:n, 0:nproc-1))
	
	rem = MOD(n,nproc)
	mychunksize = n / nproc
	last = 1
	do i=0,nproc-1
	print *, i
		startcols_i(i) = last-1
		endcols_i(i) = last + mychunksize
		
		if (i < rem) then
			endcols_i(i) = endcols_i(i) + 1
		end if
		
		last = endcols_i(i)

		!firstcols(1:n,i) =  T(1:n, startcols_i(i))
		!lastcols(1:n,i) = T(1:n, endcols_i(i))
		
		
		  print *,'Process ', i, ' starts with ', startcols_i(i), ' and ends with ',  endcols_i(i)


	end do
	
	
	! Solve the linear system of equations using the Jacobi method
  call cpu_time(t0)
  
  do k=1,maxiter
     
     !$omp parallel  private(tmp1,tmp2,j,myid) reduction(MAX:error)
          myid = omp_get_thread_num()
          
    firstcols(1:n,myid) =  T(1:n, startcols_i(myid))
	lastcols(1:n,myid) = T(1:n, endcols_i(myid))
     tmp1= firstcols(1:n,myid)
     error=0.0D0
    ! print *, startcols_i(myid)+1, endcols_i(myid)-2
     do j=startcols_i(myid)+1,endcols_i(myid)-2
        tmp2=T(1:n,j)
                
        T(1:n,j)=(T(0:n-1,j)+T(2:n+1,j)+T(1:n,j+1)+tmp1)/4.0D0
     !   !$omp atomic
			error=max(error,maxval(abs(tmp2-T(1:n,j))))

        tmp1=tmp2
     end do
      tmp2=T(1:n,j)
	tmp3 = lastcols(1:n,myid)
     T(1:n,j)=(T(0:n-1,j)+T(2:n+1,j)+tmp3+tmp1)/4.0D0
    ! !$omp atomic
     error=max(error,maxval(abs(tmp2-T(1:n,j))))
    
    
    
     ! 0.498860126200092
     !$omp end parallel 
     if (error<tol) then
        exit
     end if
     
  end do
  
  call cpu_time(t1)

  write(unit=*,fmt=*) 'Time:',t1-t0,'Number of Iterations:',k
  write(unit=*,fmt=*) 'Temperature of element T(1,1)  =',T(1,1)
		

  
end program laplsolv

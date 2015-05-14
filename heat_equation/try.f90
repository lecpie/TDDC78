program test
use omp_lib
  integer  :: rank, n
  !$omp parallel private(n,rank)
	rank = omp_get_thread_num()
	n    = omp_get_num_threads()
	print *, rank, n
  !$omp end parallel
end

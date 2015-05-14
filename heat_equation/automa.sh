#!/bin/bash          
echo Sending Executions to NSC. . . .  
declare -a arrayname=(1 2 3 4 8 12 16)

rm measures.csv

 for i in ${arrayname[@]}; 
	do
		echo "ompsalloc -t 2 -c $i ./laplsolv_omp"
		ompsalloc -t 2 -c $i ./laplsolv_omp
		#sleep 3s
	done

      	
        

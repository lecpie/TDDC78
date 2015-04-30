#!/bin/bash          
echo Sending Executions to NSC. . . .  
declare -a arrayname=(1 2 3 4 8 16 24 32 64 128 192 256 512)

 for i in ${arrayname[@]}; 
	do
		mpprun -n $i ./blurc $1 $2 r.ppm
		#sleep 3s
	done
mv measures.csv measures_$2_r$1.csv
      	
        

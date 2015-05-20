#!/bin/bash          
echo Sending Executions to NSC. . . .  
declare -a arrayname=(1 2 4 8)
#12 16 24 32 64 128 192 256 512 1024)
declare -a arraynode=(1 1 1 1)
#1  1  2   2  )

for ((i=0; i<${#arrayname[@]};i++))  
do
#	echo ${arrayname[i]}
		salloc -N ${arraynode[i]} -t 1 mpprun -n ${arrayname[i]} particlesc
done

#mv measures.csv measures_$2_r$1.csv
      	
        

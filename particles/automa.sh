#!/bin/bash          
echo Sending Executions to NSC. . . .  
declare -a arrayname=(1 2 4 8 12 16 24 32 64 128 256 512 1024)
declare -a arraynode=(1 1 1 1 1 1 2 2 4 8 16 32 64)

for ((i=0; i<${#arrayname[@]};i++))  
do
	salloc -N ${arraynode[i]} -t 10 --exclusive mpprun -n ${arrayname[i]} particlesc
done

mv measures.csv measures_$1.csv
      	
        

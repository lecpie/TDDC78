#!/bin/bash          
echo Sending Executions to NSC. . . .  
declare -a arrayname=(1 2 4 8 12 16 24 32 64 128)
declare -a arraynode=(1 1 1 1 1 1 2 2 4 8)
declare -a arraytime=(20 20 15 10 10 10 8 5 3 1)
for ((i=0; i<${#arrayname[@]};i++))  
do
	salloc -N ${arraynode[i]} -t ${arraytime[i]} --exclusive mpprun -n ${arrayname[i]} particlesc
done

mv measures.csv measures_$1.csv
      	
        

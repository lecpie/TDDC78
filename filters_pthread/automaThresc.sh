#!/bin/bash          
echo Sending Executions to NSC. . . .  
echo Parameter : input image
declare -a arrayname=(1 2 3 4 8 16)

 for i in ${arrayname[@]}; 
	do
echo salloc -N 1 -t 1 ./thresc $1 r.ppm $i
		salloc -N 1 -t 1 ./thresc $1 r.ppm $i
	done
mv measures.csv measuresThreshfilter_$1.csv
      	
        


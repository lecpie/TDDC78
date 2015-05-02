#!/bin/bash          
echo Sending Executions to NSC. . . .  
echo Parameter : input image
declare -a arrayname=(1 2 3 4 8 16 24 32 64 128 192 256 512)

 for i in ${arrayname[@]}; 
	do
		mpprun -n $i ./thresc $1 r.ppm
	done
mv measures.csv measuresThreshfilter_$1.csv
      	
        

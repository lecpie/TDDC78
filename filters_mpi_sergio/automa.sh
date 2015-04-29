#!/bin/bash          
echo Sending Executions to NSC. . . .  
declare -a arrayname=(1 2 3 4 8 16 24 32)
# 64 128 192 256 512)
declare -a arra2=(-l -a -Al)
 for i in ${arrayname[@]}; 
	do
		mpprun -n $i ./blurc $1 im4.ppm r.ppm
      	done
        

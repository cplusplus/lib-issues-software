while read issue_number           
do           
    bin/set_status $issue_number $1         
done

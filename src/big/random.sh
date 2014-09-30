#!/bin/bash

# Generate list of random numbers, roughly uniformly distributed

BYTES_TO_GET=8192;

function generate_random_numbers {
        head -c $BYTES_TO_GET /dev/urandom | 
                tr -dc "0-9\n" | 
                sed -e "s/^0*//" -e "s/^[ \n\t]*$/0/";
};

# Join N number of lines together
#  x
#  y
#  z
# Becomes
#  x y z
# if joinNlines is passed 3
function join_nlines () {
        if [ -z "$1" ] ; then
                echo "joinNlines; argc == 0";
                exit 1;
        fi;

        # join lines
        # with optional inserts inbetween lines
        awk "{
                x=\$0; 
                count = $1;
                count--;
                for(i=0; i<count;i++)
                {
                        getline; 
                        x = x \" \" \$0 ;
                } 
                print x;
        }"
}

function join_nlines_with_subs () {
        if [ -z "$1" ] ; then
                echo "join_nlines_with_subs; argc == 0";
                exit 1;
        fi;
        # echo "<$1> <$2> <$3> <$4>";

        # join lines
        # with optional inserts inbetween lines
        awk "{
                x=\"$2\" \$0; 
                count = $1;
                count--;
                for(i=0; i<count;i++)
                {
                        getline; 
                        x = x \"$3\" \$0 ;
                } 
                print  x  \"$3\" \"$4\";
        }"
}

function remove_zeros {
        sed -e "s/[ \t]0[ \t]/ 1 /" -e "s/^0/1/"
}

function single_test_case_normal () {
        generate_random_numbers | 
                join_nlines_with_subs 2 "$1" "$2" "$3" | 
                dc | 
                join_nlines 3 ;
}

function generate_test_cases {
        echo "ADD";
        single_test_case_normal " " " p " " +p ";
        echo "SUBTRACT";
        single_test_case_normal " " " p " " -p ";
        echo "MULTIPLY";
        single_test_case_normal " " " p " " *p ";

        echo "DIVIDE";
        generate_random_numbers | 
                join_nlines_with_subs 2 " " " p " " /p " | 
                remove_zeros | 
                dc | 
                join_nlines 3 ;
}

generate_test_cases | column -t

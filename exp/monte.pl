#!/usr/bin/perl
# Monte carlo pi test bench in perl

die "usage: $0 iterations\n" if $#ARGV != 0;

sub sum_of_square($$) {
        return ($_[0] * $_[0]) + ($_[1] * $_[1]);
}

sub rrand() {
        return (rand 10000.0)/10000.0;
}

my $iterations = shift ;
my $count = 0;
for (my $i = 0; $i < $iterations; $i++) {
        if(sum_of_square(rrand(), rrand()) <= 1.) {
                $count++;
        }
}
print "pi: ",($count / $iterations) * 4., "\n";


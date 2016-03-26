#!/usr/bin/env perl
# @file    monte.pl
# @brief   Monte Carlo Pi; see the $help variable for a description
# @author  Richard James Howe
# @license WTFPL See https://en.wikipedia.org/wiki/WTFPL
use warnings;
use strict;
use Time::HiRes;
use Getopt::Long;
use Math::Trig;

sub simulate();
my $default_iterations = 40000;
my $iterations         = $default_iterations;
my $verbose            = 0;
my $sexpr              = 0;
my $help               =
"This program approximates pi using Monte Carlo methods. Its purpose is for 
use as a simple bench mark, nontheless there are a few options to the program.

	-h, --help         print out this message and exit
	-s, --sexpression  print out an S-expression \"(time-taken . pi)\"
	-i, --iterations   run for a number of iterations (expects integer)
	-v, --verbose      increase the verbosity of the output

The default number of iterations that the simulation will run for is 
'$default_iterations'. Increasing the number of iterations should increase 
the accuracy of the approximation.

See https://en.wikipedia.org/wiki/Monte_Carlo_method for more details, here 
is a short description taken from there:

\"Consider a circle inscribed in a unit square. Given that the circle and 
the square  have a ratio of areas that is π/4, the value of π can be 
approximated using a Monte Carlo method:

	1) Draw a square on the ground, then inscribe a circle within it.
	2) Uniformly scatter some objects of uniform size (grains of 
	rice or sand) over the square.
	3) Count the number of objects inside the circle and the total 
	number of objects.
	4) The ratio of the two counts is an estimate of the ratio of the 
	two areas, which is π/4. Multiply the result by 4 to estimate π.\"

";

GetOptions(
	"v|verbose"      => sub { $verbose++ },
	"i|iterations=i" => \$iterations,
	"s|sexpression"  => \$sexpr,
	"h|help"         => sub { print $help; exit },
	) or die; 
simulate();

sub monte_carlo_pi($)
{
	my $i = $_[0];
	sub inner() {
		my ($a, $b) = (rand, rand);
		return 1.0 if((($a*$a) + ($b*$b)) < 1.0);
		0.0
	}

	sub outer($) {
		my $i = $_[0];
		my $c = 0.0;
		$c += inner while(--$i);
		$c
	}

	return (outer($i) / $i) * 4.0
}

sub simulate () {
	if($verbose) {
		print "Monte Carlo Pi.\n";
		print "This simulation will run for '$iterations' iterations.\n";
	}

	my $start = Time::HiRes::gettimeofday();
	my $mypi  = monte_carlo_pi($iterations);
	my $end   = Time::HiRes::gettimeofday();
	my $time  = $end - $start;

	if($sexpr) {
		print "($time . $mypi)\n"
	} else {
		print "Time taken:\t$time\nApproximation:\t$mypi\n"
	}

	if($verbose) {
		my $deviation  = abs(pi - $mypi);
		my $devpercent = ($deviation / pi) * 100;
		print "Start Time:\t$start\n";
		print "End Time:\t$end\n";
		print "Deviation:\t$deviation\n";
		print "Error %:\t$devpercent\n";
		print "Done.\n"
	}
}

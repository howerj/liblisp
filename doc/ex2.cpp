/* ex2.cpp: example interpreter in C++, make sure that the lisp header is C++ compatible */
#include <liblisp.h>
#include <iostream>
using namespace std;
int main(int argc, char **argv)
{
	cout << "Hello, C++" << endl;
	return main_lisp(argc, argv);
}

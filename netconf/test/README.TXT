The Yuma Test Harness
===============================================================================

1: Pre-requisites:
    Operating System	Ubuntu 10.04 or later	
    Compiler	        GCC 4.4 or later	
    Boost	        Boost 1.47 or later
    Lib Xml 	        libxml2 / libxml2-dev
    Python	        2.6 Python interpreter
    Libbz2-dev	        1.05-4ubuntu0.1	
    Doxygen		Source code documentation tool
    Graphviz		Graphical Visualisation Tool used by Doxygen
    Texlive-font-utils  Text formatting used by doxygen
    Libncursesw5	5.7 Shared libraries for terminal handling
    libssh2-1-dev	1.2.2-1 SSH2 ssh2 client-side libary

2: The Integration Test Harness
netconf/test/integ-tests

The Integration test harness is built as a stand alone executable that 
includes most of the Yuma agt and ncx sources (which make up the system 
under test). This test harness runs on the local host machine and can be
 used to quickly verify the behaviour of netconfd.

To build and run this test harness:
    1: cd to netconf/test/modules/build-sil
    2: make
    1: cd to netconf/test/integ-tests
    2: make
    3: Either run each executable individually or use the alltests.py
       script to run all of the tests.

Note: tests can be run individually, by specifying the name of the
test suite and test on the command line as follows:
    ./test-simple-edit-running \
        --run_test=simple_get_test_suite_running/edit_slt_entries_test1

For more information on test harness command line options see:
    http://www.boost.org/doc/libs/1_47_0/libs/test/doc/html/utf/user-guide/runtime-config.html

3: The System Test Harness
netconf/test/sys-test

The System test harness is a stand alone program that is capable of 
running full NETCONF sessions against a full Yuma/Netconf Server
(the system under test). The System test harness provides a fast 
way of verifying the behaviour of a full Yuma/Netconf system.
It behaves in the same way as a real Netconf client. To use 
this test harness the Netconf Server must have the appropriate
YANG modules installed. The SIL modules for the YANG libraries
can be loaded, but this is not required.

To make sure the proper modules get loaded, either configure
the netconfd --modpath parameter, set the YUMA_MODPATH environment
variable, or put the YANG files a directory called 'modules' in
your HOME directory.

To build and run this test harness:
    1: cp netconf/test/modules/yang/* $HOME/modules/
    2: cd to netconf/test/sys-test
    3: make
    4: this will build a number of executables for verify either with
       or without the candidate database. 
    5: start the server and make sure at least these flags are set:
       --module=device_test
       --module=simple_list_test
       --module=simple_yang_test
       --access-control=off OR --superuser=<user>
       --target=candidate OR --target=running
       --no-startup
    6: run ./configure-env.py to configure the test client
       - make sure to give IP addresses as numbers, such as
         127.0.0.1 instead of localhost.
       - enter the same user name as the <name> in step 5.
    7: source ./config.sh
    8: run either all-candidate.py or all-running.py, depending on
       which target was selected in step 5.



#!/usr/bin/env python
import sys
import os

# ----------------------------------------------------------------------------|
if __name__ == '__main__':
    print """Generating Configuration Script for remote Yuma Testing...\n"""
        
    ipAddr = raw_input( "Enter Netconf Agent Ip address: ")
    port = raw_input( "Enter Netconf Agent Source Port: ")
    user = raw_input( "Enter Netconf User Name: ")
    passWd = raw_input( "Enter Netconf User's Password: ")

    outFilename = "./config.sh" 
    out=open( outFilename, 'w' )
    out.write( "export YUMA_AGENT_IPADDR="+ipAddr+"\n" )
    out.write( "export YUMA_AGENT_PORT="+port+"\n" )
    out.write( "export YUMA_AGENT_USER="+user+"\n" )
    out.write( "export YUMA_AGENT_PASSWORD="+passWd+"\n" )
    print ( "Environment configuration written to %s" % outFilename )
    print ( "Source %s with the comman '. %s' to configure the test environment"
            % ( outFilename, outFilename ) )





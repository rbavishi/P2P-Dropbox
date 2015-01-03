#!/usr/bin/env python
##Config CLI
from optparse import *
import os
import subprocess
import getpass
def install_ssh_keys(usr_ipaddr):
    print "Please make sure that the service sshd is running at the remote computer. If not, exit the program using Ctrl-C and start the service."
    password=getpass.getpass(prompt='Enter password of remote user: ')
    home_address=os.path.expanduser("~")
    command="sshpass -p"+password+" ssh-copy-id -i "+home_address+"/.ssh/id_rsa.pub -o \"StrictHostKeyChecking no\" "+usr_ipaddr
    p=subprocess.Popen(command,shell=True,stdout=subprocess.PIPE,stderr=subprocess.PIPE)
    p.communicate()

def restart_service():
    p=subprocess.Popen("service dbox restart",shell=True,stdout=subprocess.PIPE)
    
def write_to_conf_file(source,usr_ipaddr,target,service):   
    string=source+"#"+usr_ipaddr+"#"+target+"\n"
    with open("/etc/dropbox.conf",'r') as f:
        s=f.read()
    if s=='' or s=='\n':
        f=open("/etc/dropbox.conf",'w')
        install_ssh_keys(usr_ipaddr)
        f.write(string)
        if service==True:
            restart_service()
    elif string in s:
        print "Folder already added."
    else:
        f=open("/etc/dropbox.conf",'a')
        install_ssh_keys(usr_ipaddr)
        f.write(string)
        if service==True:
            restart_service()
    f.close()
    return 0

def add_folder(source,usr_ipaddr,target,service):
    write_to_conf_file(source,usr_ipaddr,target,service)
def remove_folder(source,usr_ipaddr,target,service):
    with open("/etc/dropbox.conf",'r') as f:
        string=f.read()
    hit=map(str,string.split('\n'))
    f=open("/etc/dropbox.conf",'w')
    for i in hit:
        if i=='':
            continue
        (src,addr,targ)=i.split("#")
        if src==source and usr_ipaddr==addr and target==targ:
            continue
        string=src+"#"+addr+"#"+targ+"\n"
        f.write(string)
    f.close()
    if service==True:
        restart_service()
        
    
def main():
    usage="Usage: %prog <options> [-a|--add]/[-d|--delete] arg1<Source-folder name/address> arg2<username@ip-address> arg3<Dest-folder-name/address>"
    parser=OptionParser(usage)
    parser.add_option("-v","--verbose",action="store_true",dest="verbose",help="Increase verbosity")
    parser.add_option("-a","--add",action="store_true",dest="add",help="Add folder to dropbox configuration")
    parser.add_option("-d","--delete",action="store_true",dest="delete",help="Remove folder from dropbox configuration")
    parser.add_option("-s","--service",action="store_true",dest="service",help="Restarts service dropbox-inotify after adding/removing folder")
    (options,args)=parser.parse_args()
    if len(args)!=3: 
        parser.error("Incorrect no. of arguments")
    if options.verbose:
        print "Updating dropbox.conf file..."
        print "Target Username and IP Address: %s"%(args[1])
        print "Source Folder: %s"%(args[0])
        print "Destination Folder: %s"%(args[2])
    if options.add!=None and options.delete!=None:
        parser.error("Both options cannot be selected simultaneously")
    if options.add==None and options.delete==None:
        parser.error("One option has to be selected")
    if options.add!=None:
        add_folder(args[0],args[1],args[2],options.service)
    else:
        remove_folder(args[0],args[1],args[2],options.service)
    

if __name__ == "__main__" :
    main()
    


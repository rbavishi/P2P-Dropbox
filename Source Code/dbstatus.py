#!/usr/bin/env python
import os,mmap,fcntl
from optparse import *
from subprocess import *
def extract_all():
    with open("/etc/dropbox.conf",'r') as f:
        string=f.read()
    hit=map(str,string.split('\n'))
    main_list=[]
    for i in hit:
        if i=='':
            continue
        (source,addr,target)=i.split("#")
        main_list+=[[source,addr,target]]
    return main_list

def display_currently_synced():
    main_list=extract_all()
    for i in main_list:
        print "User: %s                               Source Folder: %s                              Target Folder: %s"%(i[1],i[0],i[2])

def display_user(usr_addr,addr,synced):
    main_list=extract_all()
    main_folder_address=""
    os.chdir(os.path.expanduser("~"))
    for i in main_list:
        if i[1]==usr_addr:
            main_folder_address=i[0]
    if main_folder_address=="":
        print "User not added."
        return -1
    if addr!="no_arg":
        command="find \""+main_folder_address+"\" -type d"
        p=Popen(command,shell=True,stdout=PIPE)
        main_string=p.communicate()[0]
        main_string=map(str,main_string.split("\n"))
        if not addr in main_string:
            print "Folder doesn't exist/ Folder isn't synced"
            return -1
    else:
          addr=main_folder_address
        
    filename=usr_addr+"_status_file.txt"
    print "Files/Folders being synced: "
    files_syncing=[]
    try:
        f=open(filename,'r')
        
        fcntl.lockf(f,fcntl.LOCK_SH)
        mmap_pointer=mmap.mmap(f.fileno(),0,prot=mmap.PROT_READ,offset=0)
        current_string=mmap_pointer.readline()
        while (current_string!=''):
            current_string=current_string[:len(current_string)-1]
            item_list=map(str,current_string.split("#"))
            if addr in item_list[0]:
                files_syncing+=[[item_list[0]]]
                #item_list[0]+=" "*(50-strlen(item_list[0]))
                #item_list[1]+=" "*(50-strlen(item_list[1]))
                #item_list[2]+=" "*(5-strlen(item_list[2]))
                print "Path: %s                     Rsync started: %s                         Retry Attempts: %s"%(item_list[0],item_list[1],item_list[2])
            current_string=mmap_pointer.readline()
        mmap_pointer.close()
        fcntl.lockf(f,fcntl.LOCK_UN)
    except:
        print ""
    if synced==True:
        command="find \""+addr+"\" -type d"
        p=Popen(command,shell=True,stdout=PIPE)
        main_string=p.communicate()[0]
        main_string=map(str,main_string.split("\n"))
        main_string=list(set(main_string)-set(files_syncing))
        print "Files already synced: "
        for i in main_string:
            if i=="":
                continue
            print "Path: %s"%(i)

def display_all(synced):
    main_list=extract_all()
    for i in main_list:
        print "User: %s Source: %s Target:%s"%(i[1],i[0],i[2])
        display_user(i[1],i[0],synced)
    
def main():
    usage="Usage: %prog <options> [-c|--current] [-a|-all] [-u|--user] arg1 <USER@IP> arg2 <Folder Name>"
    parser=OptionParser(usage)
    parser.add_option("-c","--current",action="store_true",dest="current",help="Display the folders currently being synced.")
    parser.add_option("-a","--all",action="store_true",dest="all",help="Display sync status i.e. files currently being synced (in progress) for all the folders.")
    parser.add_option("-u","--user",action="store_true",dest="user",help="Display sync status for a particular folder synced with user <arg-USERNAME@IP>.")
    parser.add_option("-s","--synced",action="store_true",dest="synced",help="Display synced files as well in addition to files in progress.") 
    (options,args)=parser.parse_args()
    if ((len(args) in [1,2]) and options.user!=True) or (len(args)==0 and options.user==True): 
        parser.error("Incorrect no. of arguments. Please use -u|--user option as well if you're supplying a username.")
    if options.current==options.all==options.user==None:
        parser.error("Please specify an option.")
    if options.all==options.user==True:
        parser.error("[-a|--all] and [-u|--user] cannot be called simultaneously.")
    if options.current==True:
        display_currently_synced()
    if options.all==True:
        display_all(options.synced)
    elif options.user==True:
        if len(args)==1:
            display_user(args[0],"no_arg",options.synced)
        else:
            display_user(args[0],args[1],options.synced)

if __name__=="__main__":
    main()

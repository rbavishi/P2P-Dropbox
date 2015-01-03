#!/usr/bin/env python
from subprocess import *
import os
def main():
    os.chdir(os.path.expanduser("~"))
    with open("/etc/dropbox.conf",'r') as f:
        string=f.read()
    hit=map(str,string.split('\n'))
    for i in hit:
        if i=='':
            continue
        (source,addr,target)=i.split("#")
        try :
            os.makedirs(source)
        except:
            a=10
        p=Popen(["dropbox-inotify","-1",source,addr,target],shell=False,stdout=PIPE,stdin=PIPE,stderr=PIPE)
if __name__=="__main__":
    main()

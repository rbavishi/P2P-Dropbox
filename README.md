P2P-Dropbox
===========

**A Linux application for back-up and syncing of files between two or more peers.**


**OVERVIEW**

* Users have shared folder(s) across different machines, each having a local copy on their computer. Any changes made in the folder of one user are propagated to the folders of all the other users.
* Implementation as a service makes the application run automatically at boot-time and a network detection system along with retry mechanisms ensures syncing is done whenever a connection is available.
* A Command-Line-Interface for easy addition/deletion of shared folders and for viewing the synchronization status.
* Passwordless-login setup to ensure you don't have to enter passwords everytime a change is made.

**MAJOR DESIGN CHOICES**

* Linux [*inotify*](http://man7.org/linux/man-pages/man7/inotify.7.html) API used to detect changes made in files/folders.
* [*rsync*](http://linux.about.com/library/cmd/blcmdl1_rsync.htm) utility used to efficiently propagate changes across shared folders.

For detailed implementation and design details, refer to the document P2P-Dropbox.pdf included.

**SYSTEM COMPATIBILITY**

* Currently, the application has been developed only for the fedora distribution.
* The package *sshpass* needs to be installed.
* In case you are using a proxy, you need to install logmein-hamachi to ensure you have an external IP-Address.

**INSTALLATION PROCEDURE** 

1. Run the install script present in this folder. Run the following commands -

	 $chmod +x install

	 $sudo ./install

 	Running the script as sudo is necessary as access to certain folders is required.

2. The software will be installed. Verify that the sshd and logmein-hamachi services(in case of proxy) are running by typing the following commands.

	**$service sshd status**

	**$sudo service logmein-hamachi start (in case of proxy)**

**USING THE PROGRAM** 	(Please make sure you are connected to the internet)

1. In case you're using a proxy, look up your external ip address by typing the following.

        **$hamachi <enter> **
        Look up your ip in the 'address' section.

2. For testing out the program, you can first try to sync a folder locally. Three command-line tools are included, namely "dbconfig" "dbstatus" and "dropbox-start". We are mainly to use the first two. Type "dbconfig -h" and "dbstatus -h" to get help.

3. For adding a folder, say "[path]/Picasso" which is to be synced to another folder say "[path]/Pollock", with USER being your username, we type:

	__$dbconfig -avs "[path]/Picasso" "USER@<your ip_address>" "[path]/Pollock"__

	It will ask for your own password, as it needs to be able to login passwordless in the future.

	Explanation of command:

	*-a - adds the folder*

	*-v - increase verbosity*

	*-s - restart dbox service*


	If you do not give -s as an option and want to start the syncing, type the following command to start dbox service -

	**$service dbox start**
	
	As a reminder, please make sure the service sshd is running. If it is inactive, type:

	**$sudo service sshd start**

4. (Only for proxy-using networks)If you want to test the software with another computer other than your own. You'll have to first create a network with :

	**$hamachi create [network_name] [password]**

	Then with the other computer, join the network with

	**$hamachi join [network name] [password]**

	Ping the other computer to make sure the network is alive. Then repeat the 3rd step above with only the USER variable changed to the username of the other computer.

5. If you want to look up the syncing status, use the dbstatus command. Type "dbstatus -h" to get help.
	Example:

	**$dbstatus -acs**

	Usage:

	*-c - Show the config file.*

	*-a - Show the syncing status for folders for all the users.*

	*-s - Show the "already-synced" files as well.*

**UNINSTALLING THE SOFTWARE**

1. If you want to uninstall the program, in the project folder, run the uninstall script by:

	__$sudo chmod +x [path\_to\_script]/uninstall__

 	__$sudo [path\_to\_script]/uninstall__


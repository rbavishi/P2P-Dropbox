P2P-Dropbox
===========

Peer-to-peer file syncing application
Project Title - P2P Dropbox
Project Mentor - Mr. Arbaz Khan

**INSTALLATION PROCEDURE** 
1. Run the install script present in this folder. Run the following commands -
	$chmod +x install <enter>
	$sudo ./install   <enter>
	Running the script as sudo is necessary as access to certain folders is required.

2. The software will be installed. Verify that the sshd and logmein-hamachi services are running by typing the following commands.
	$service sshd status
	$sudo service logmein-hamachi start

**USING THE PROGRAM** 	(Please make sure you are connected to the internet)

1. Since within iitk we are connected by intranet, we need to get an external ip address for ourselves to be able to communicate with each other over the internet. That is the reason why we have installed logmein-hamachi. Look up your external ip address by typing the following -
	
	$hamachi <enter>
	Look up your ip in the 'address' section.

2. For testing out the program, you can first try to sync a folder locally. I have included three command-line tools, namely "dbconfig" "dbstatus" and "dropbox-start". We are mainly to use the first two. Type "dbconfig -h" and "dbstatus -h" to get help.

3. For adding a folder, say "Codechef" which is to be synced to another folder say "TestingFold", with USER being your username, we type:
	$dbconfig -avs "./Codechef" "USER@<your ip_address>" "./TestingFold" 
	Here "./" implies that it is in the home folder. 
	It will ask for your own password, as it needs to be able to login passwordless in the future.
	Explanation of command:
	-a - adds the folder
	-v - increase verbosity
	-s - restart dbox service
	If you do not give -s as an option and want to start the syncing, type the following command to start dbox service -
	$service dbox start
	
	As a reminder, please make sure the service sshd is running. If it is inactive, type:
	$sudo service sshd start

4. If you want to test the software with another computer other than your own. You'll have to first create a network with :
	$hamachi create <network_name> <password>
	Then with the other computer, join the network with
	$hamachi join <network name> <password>
	Ping the other computer to make sure the network is alive. Then repeat the 3rd step above with only the USER variable changed to the 		username of the other computer.

5. If you want to look up the syncing status, use the dbstatus command. Type "dbstatus -h" to get help.
	Example:
	$dbstatus -acs
	Usage:
	-c - Show the config file.
	-a - Show the syncing status for folders for all the users.
	-s - Show the "already-synced" files as well.

**UNINSTALLING THE SOFTWARE**

1. If you want to uninstall the program, in the project folder, run the uninstall script by:
	$sudo chmod +x <path_to_script>/uninstall
 	$sudo <path_to_script>/uninstall


# UserKernelCommunication

## About the code

This repository provides test environment to test various mechanism for 
communications between userland and kernelland.

Code to test the following environments are provided:
	* Netlink
	* A character device module

For every mechanism, you will find two implementations:
	* An implementation for user to kernel communication;
	* An implementation for kernel to user communication.

Each implementation has:
	* a kern directory containing the module to be added to the kernel;
	* a user directory containing a binary to communicate with the module.

## Some results

Results are provided in terms of clock cycles.

User to Kernel :
----------------

| 	        |  User Sending   |  Kernel receiving  |
| :------------ | :-------------: | :----------------: |
| Netlink 	|     Colonne     |        Colonne     |
| Char Dev 	|   Alignée au    |      Alignée à     |

Kernel to User:
---------------

| 	        |  Kernel Sending |   User receiving   |
| :------------ | :-------------: | :----------------: |
| Netlink 	|     Colonne     |        Colonne     |
| Char Dev 	|   Alignée au    |      Alignée à     |

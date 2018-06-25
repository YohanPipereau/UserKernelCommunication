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

Results are provided clock cycles/messages and messages are all 1024 bytes long.

User to Kernel :
----------------

| 	        	|  User Sending   |  Kernel receiving  |
| :------------------- 	| :-------------: | :----------------: |
| Netlink 		| 15 000 	  |  |
| Char dev writeop 	| 52 405 	  |  182 	       |

Kernel to User:
---------------

| 	       		|  Kernel Sending |   User receiving   |
| :-------------------- | :-------------: | :----------------: |
| Netlink 		| 3 498 	  | 3 724 	       |
| Char dev writeop 	|  		  |     		|

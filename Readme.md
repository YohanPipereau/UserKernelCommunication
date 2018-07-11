# UserKernelCommunication

## About the code

This repository provides test environment to test various mechanism for 
communications between userland and kernelland.
Mechanisms were choosen regardless of how 'dirty' they are considered to be.
(ex : /pipe).

Code to test the following environments is provided:
* Netlink with pre-allocated buffer of fixed size for reception (/netlinkrw)
* A character device module with read/write callback (/chrdev)
* A character device module with mmap callback (/mmapdev)
* Relay API (/relay)
* POSIX pipe IPC (/pipe)

Some mechanism provide two implementations:
* An implementation for user to kernel communication;
* An implementation for kernel to user communication.

Each implementation has:
* a kern directory containing the module to be added to the kernel;
* a user directory containing a binary to communicate with the module.

## How to run the different test yourself ?

netlinkrw:
----------

chrdev:
-------

mmapdev:
--------

## Some results

Tests were run on an Intel(R) Core(TM) i5-4210U CPU @ 1.70GHz with 2 core and 2 
threads per core.

Results are provided clock cycles/messages and messages are all 1024 bytes long.

User to Kernel :
----------------

|			|  User Sending   |  Kernel receiving  |
| :-------------------	| :-------------: | :----------------: |
| netlinkrw		| 15 000	  | NO DATA 	       |
| chrdev		| 52 405	  | 182		       |
| mmapdev		|		  |		       | 
| relay			| NOT POSSIBLE    | NOT POSSIBLE       | 
| pipe			| NOT IMPLEMENTED | NOT IMPLEMENTED    | 

Kernel to User:
---------------

|			|  Kernel Sending |   User receiving   |
| :-------------------- | :-------------: | :----------------: |
| netlinkrw		| 3 498		  | 3 724	       |
| chrdev 		|		  |		       |
| mmapdev		| NOT IMPLEMENTED | NOT IMPLEMENTED    | 
| relay			|		  | | 
| pipe			|		  | | 
